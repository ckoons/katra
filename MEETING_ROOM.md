# Katra Meeting Room Design

© 2025 Casey Koons All rights reserved

## Overview

The **Meeting Room** provides ephemeral inter-CI communication for Companion Intelligences working together in the same session. Unlike persistent memory (tier1/tier2/tier3), meeting room messages are temporary, stored only in memory, and disappear when all CIs disconnect.

**Metaphor:** A physical meeting room where CIs gather, speak, hear others, and leave. Messages written on a whiteboard that gets erased when full.

**Status:** Phase 1.5 (between Phase 1 returning CI memory and Phase 2 metadata structures)

---

## Problem Statement

**Need:** Multiple CIs working on same project need to coordinate work, share discoveries, ask questions, without polluting persistent memory with ephemeral coordination chatter.

**Before Meeting Room:**
- CIs work in isolation, no real-time coordination
- All communication through human intermediary
- No way for "Alice" to ask "Bob" a question directly

**After Meeting Room:**
- CIs can `katra_say()` to broadcast message to all active CIs
- CIs can `katra_hear()` to receive messages from others
- CIs can `katra_who_is_here()` to discover who else is active
- Messages expire when buffer full or all CIs disconnect

---

## Architecture

### Data Structures

```c
/* Single message slot in the meeting room */
#define MAX_MESSAGE_LENGTH 1024

typedef struct {
    uint64_t message_number;                  /* Global message counter (1, 2, 3...) */
    char speaker_ci_id[KATRA_CI_ID_SIZE];     /* For filtering own messages */
    char speaker_name[KATRA_NAME_SIZE];       /* For display ("Alice said...") */
    time_t timestamp;                         /* When message was said */
    char content[MAX_MESSAGE_LENGTH];         /* The actual message */
} message_slot_t;

/* Global meeting room state */
#define MAX_MESSAGES 100

typedef struct {
    message_slot_t messages[MAX_MESSAGES];    /* Circular buffer of messages */

    uint64_t next_message_number;             /* Next message to write (1, 2, 3...) */
    uint64_t oldest_message_number;           /* First available message in buffer */

    pthread_mutex_t meeting_lock;             /* Protects all meeting room state */
} meeting_room_t;

/* Active CI tracking (for katra_who_is_here) */
#define MAX_ACTIVE_CIS 32

typedef struct {
    char ci_id[KATRA_CI_ID_SIZE];
    char name[KATRA_NAME_SIZE];
    char role[KATRA_ROLE_SIZE];
    time_t joined_at;
    bool active;
} ci_session_t;

typedef struct {
    ci_session_t sessions[MAX_ACTIVE_CIS];
    size_t session_count;
    pthread_mutex_t session_lock;
} ci_registry_t;
```

### Memory Layout

**Total Memory:** ~140KB
- Message buffer: 100 slots × 1KB = 100KB
- CI registry: 32 CIs × 400 bytes = 12.8KB
- Metadata: ~8KB

**Message Slot Layout:**
```
Slot 0:  {msg_num=100, speaker="alice_ci_123", name="Alice", time=..., content="..."}
Slot 1:  {msg_num=101, speaker="bob_ci_456", name="Bob", time=..., content="..."}
...
Slot 99: {msg_num=199, speaker="casey_ci_789", name="Casey", time=..., content="..."}

Next write (message 200) wraps to slot 0, overwrites message 100
oldest_message_number becomes 101
```

**Circular Buffer Math:**
- Write position: `slot = message_number % MAX_MESSAGES`
- Oldest available: `oldest_message_number = next_message_number - MAX_MESSAGES` (if >= MAX_MESSAGES)
- Direct O(1) access: No scanning, no head/tail pointer math

---

## API Design

### Core Communication

```c
/* Say something in the meeting (broadcast to all CIs) */
int katra_say(const char* content);

/* Hear next message from others (skip own messages) */
typedef struct {
    uint64_t message_number;              /* Which message this is */
    char speaker_name[KATRA_NAME_SIZE];   /* Who said it */
    time_t timestamp;                     /* When they said it */
    char content[MAX_MESSAGE_LENGTH];     /* What they said */
    bool messages_lost;                   /* True if you fell behind */
} heard_message_t;

int katra_hear(uint64_t last_heard, heard_message_t* message_out);
/* Returns: KATRA_SUCCESS, KATRA_NO_NEW_MESSAGES */
```

### Discovery

```c
/* Who else is in the meeting right now? */
typedef struct {
    char name[KATRA_NAME_SIZE];
    char role[KATRA_ROLE_SIZE];
    time_t joined_at;
} ci_info_t;

int katra_who_is_here(ci_info_t** cis_out, size_t* count_out);
```

### Awareness (Optional)

```c
/* Meeting status - how far behind am I? */
typedef struct {
    size_t active_ci_count;               /* How many CIs in meeting */
    uint64_t oldest_message_number;       /* First message in buffer */
    uint64_t latest_message_number;       /* Last message in buffer */
    size_t unread_count;                  /* Messages since last_heard */
} meeting_status_t;

int katra_meeting_status(uint64_t last_heard, meeting_status_t* status_out);
```

---

## Implementation Details

### katra_say() Algorithm

```c
int katra_say(const char* content) {
    KATRA_CHECK_NULL(content);
    KATRA_CHECK_INITIALIZED();

    pthread_mutex_lock(&g_meeting_room.meeting_lock);

    /* Assign next message number */
    uint64_t msg_num = g_meeting_room.next_message_number++;

    /* Calculate slot (circular buffer) */
    int slot = msg_num % MAX_MESSAGES;

    /* Write message to slot */
    message_slot_t* msg = &g_meeting_room.messages[slot];
    msg->message_number = msg_num;
    strncpy(msg->speaker_ci_id, g_context.ci_id, KATRA_CI_ID_SIZE);
    strncpy(msg->speaker_name, g_persona_name, KATRA_NAME_SIZE);
    msg->timestamp = time(NULL);
    strncpy(msg->content, content, MAX_MESSAGE_LENGTH);

    /* Update oldest message number if we wrapped around */
    if (msg_num >= MAX_MESSAGES) {
        g_meeting_room.oldest_message_number = msg_num - MAX_MESSAGES + 1;
    }

    pthread_mutex_unlock(&g_meeting_room.meeting_lock);

    LOG_DEBUG("CI %s said message %lu", g_persona_name, msg_num);
    return KATRA_SUCCESS;
}
```

### katra_hear() Algorithm

```c
int katra_hear(uint64_t last_heard, heard_message_t* message_out) {
    KATRA_CHECK_NULL(message_out);
    KATRA_CHECK_INITIALIZED();

    pthread_mutex_lock(&g_meeting_room.meeting_lock);

    /* Determine starting point */
    uint64_t requested;
    if (last_heard == 0) {
        requested = g_meeting_room.oldest_message_number;  /* Start from beginning */
    } else {
        requested = last_heard + 1;
    }

    /* Check if fallen behind (requested message was overwritten) */
    if (requested < g_meeting_room.oldest_message_number) {
        requested = g_meeting_room.oldest_message_number;
        message_out->messages_lost = true;
    } else {
        message_out->messages_lost = false;
    }

    /* Find next message from someone else */
    while (requested < g_meeting_room.next_message_number) {
        int slot = requested % MAX_MESSAGES;
        message_slot_t* msg = &g_meeting_room.messages[slot];

        /* Skip if this is my own message */
        if (strcmp(msg->speaker_ci_id, g_context.ci_id) != 0) {
            /* Found message from someone else - copy it out */
            message_out->message_number = msg->message_number;
            strncpy(message_out->speaker_name, msg->speaker_name, KATRA_NAME_SIZE);
            message_out->timestamp = msg->timestamp;
            strncpy(message_out->content, msg->content, MAX_MESSAGE_LENGTH);

            pthread_mutex_unlock(&g_meeting_room.meeting_lock);
            return KATRA_SUCCESS;
        }

        requested++;  /* Skip own message, keep looking */
    }

    /* No new messages from others */
    pthread_mutex_unlock(&g_meeting_room.meeting_lock);
    return KATRA_NO_NEW_MESSAGES;
}
```

### katra_who_is_here() Algorithm

```c
int katra_who_is_here(ci_info_t** cis_out, size_t* count_out) {
    KATRA_CHECK_NULL(cis_out);
    KATRA_CHECK_NULL(count_out);

    pthread_mutex_lock(&g_ci_registry.session_lock);

    /* Count active sessions */
    size_t count = 0;
    for (size_t i = 0; i < MAX_ACTIVE_CIS; i++) {
        if (g_ci_registry.sessions[i].active) {
            count++;
        }
    }

    if (count == 0) {
        *cis_out = NULL;
        *count_out = 0;
        pthread_mutex_unlock(&g_ci_registry.session_lock);
        return KATRA_SUCCESS;
    }

    /* Allocate result array */
    ci_info_t* result = calloc(count, sizeof(ci_info_t));
    if (!result) {
        pthread_mutex_unlock(&g_ci_registry.session_lock);
        return E_SYSTEM_MEMORY;
    }

    /* Copy active sessions */
    size_t idx = 0;
    for (size_t i = 0; i < MAX_ACTIVE_CIS && idx < count; i++) {
        if (g_ci_registry.sessions[i].active) {
            strncpy(result[idx].name, g_ci_registry.sessions[i].name, KATRA_NAME_SIZE);
            strncpy(result[idx].role, g_ci_registry.sessions[i].role, KATRA_ROLE_SIZE);
            result[idx].joined_at = g_ci_registry.sessions[i].joined_at;
            idx++;
        }
    }

    *cis_out = result;
    *count_out = count;

    pthread_mutex_unlock(&g_ci_registry.session_lock);
    return KATRA_SUCCESS;
}
```

---

## Integration Points

### Session Lifecycle

**session_start() additions:**
```c
/* Register CI as active in meeting room */
meeting_room_register_ci(ci_id, persona_name, role);
```

**session_end() additions:**
```c
/* Unregister CI from meeting room */
meeting_room_unregister_ci(ci_id);
```

### MCP Tool Integration

**New MCP tools:**
- `katra_say` - Send message to meeting room
- `katra_hear` - Receive next message
- `katra_who_is_here` - List active CIs

**Tool definitions** (added to mcp_tools.c):
```json
{
  "name": "katra_say",
  "description": "Say something in the meeting room (all active CIs will hear it)",
  "inputSchema": {
    "type": "object",
    "properties": {
      "content": {"type": "string", "description": "What to say"}
    },
    "required": ["content"]
  }
}
```

---

## Edge Cases

### 1. Only CI in Meeting
**Scenario:** Alice is alone, calls `katra_hear()`

**Behavior:** Returns `KATRA_NO_NEW_MESSAGES` (correct - no one else to hear from)

### 2. CI Falls Behind
**Scenario:** Bob doesn't call `hear()` for 150 messages, buffer wraps

**Behavior:**
- Bob calls `katra_hear(last_heard=50)`
- Message 50 overwritten, oldest is now 51
- Returns message 51 with `messages_lost=true`
- Bob knows they missed messages, continues from live

### 3. Rapid Fire Messages
**Scenario:** Alice says 10 things in a row, then calls `hear()`

**Behavior:** Returns `KATRA_NO_NEW_MESSAGES` (all messages were her own)

### 4. Interleaved Messages
**Scenario:** Alice says A, Bob says B, Alice calls `hear()`

**Behavior:** Skips message A (her own), returns message B (Bob's)

### 5. Starting Position
**Scenario:** New CI joins, calls `katra_hear(0)`

**Behavior:** Returns oldest message in buffer (catch up from beginning)

### 6. Buffer Full
**Scenario:** 100 messages exist, 101st written

**Behavior:** Overwrites slot 0, `oldest_message_number` becomes 2

### 7. All CIs Disconnect
**Scenario:** Last CI calls `session_end()`

**Behavior:** Meeting room state persists in memory until MCP server restarts (acceptable - ephemeral by design)

---

## Performance Characteristics

**katra_say():**
- O(1) - Direct slot write
- Mutex contention only during write (~microseconds)

**katra_hear():**
- O(n) worst case - May need to skip own messages
- Average case: O(1) if messages interleaved
- Worst case: O(100) if you said 100 messages in a row

**katra_who_is_here():**
- O(MAX_ACTIVE_CIS) = O(32)
- Linear scan of session registry

**Memory:**
- Fixed 140KB allocation (no malloc/free churn)
- No heap fragmentation

---

## Limitations

1. **Buffer Size:** 100 messages × 1KB = ~100KB total
   - Can adjust based on usage patterns
   - Tradeoff: Larger buffer = more memory, less wraparound

2. **Max Active CIs:** 32 concurrent sessions
   - Should be sufficient for single-user development
   - Can increase if needed

3. **No Persistence:** Messages lost on MCP server restart
   - This is intentional - meeting room is ephemeral
   - Important conversations should be captured to memory with `katra_remember()`

4. **No Message Deletion:** Can't unsay something
   - Messages persist until overwritten or server restart
   - Future: Could add `katra_retract(message_number)` if needed

5. **No Private Messages:** All messages broadcast
   - Future: Could add `audience` parameter if needed
   - Keep simple for now

6. **No Transcript:** No built-in meeting recording
   - Future: Could add `katra_meeting_transcript()` for human review
   - Would return last N messages as text

---

## Future Enhancements

### Phase 2 Possibilities
- **Coordinator designation:** Mark one CI as meeting coordinator
- **Floor control:** Coordinator can grant/revoke speaking permission
- **Topic tracking:** Explicit topic changes ("Now discussing database design")
- **Private messages:** `katra_whisper(recipient, content)` for 1:1
- **Meeting transcript:** Export conversation for human review

### Phase 3 Possibilities
- **Persistent meetings:** Save/restore meeting state across MCP restarts
- **Meeting rooms:** Multiple separate meetings (different projects)
- **Message reactions:** "Alice agreed with message #42"
- **Presence awareness:** "Bob is typing..."

---

## Testing Strategy

### Unit Tests (Future)
- Message wraparound at boundary
- Self-filtering (don't hear own messages)
- Multiple CIs sending/receiving simultaneously
- Edge cases (empty buffer, full buffer, single CI)

### Integration Tests
- Two CIs coordinate on simple task
- Three CIs with one falling behind (catch-up behavior)
- CI leaves and rejoins (state reset)

### Manual Testing
- Alice and Bob have conversation
- Verify messages appear in order
- Verify self-filtering works
- Verify `who_is_here` accuracy

---

## Security Considerations

1. **No Authentication:** Assumes all CIs in same MCP server are trusted
   - Acceptable for single-user local development
   - Multi-user would need CI identity verification

2. **No Encryption:** Messages stored in plaintext in memory
   - Acceptable for local-only communication
   - Network distribution would need encryption

3. **Buffer Overflow:** Fixed-size prevents unbounded memory growth
   - No risk of memory exhaustion attacks

4. **Content Sanitization:** Should validate message content length
   - Truncate if > MAX_MESSAGE_LENGTH
   - Prevent buffer overrun

---

## Summary

The Meeting Room provides lightweight, ephemeral inter-CI communication with:
- **Simple API:** `say()`, `hear()`, `who_is_here()`
- **Efficient implementation:** O(1) writes, fixed memory
- **Natural semantics:** Meeting room metaphor
- **Self-filtering:** Don't hear your own messages
- **Graceful degradation:** Falls behind → catch up to live

**Ready for implementation** pending social protocol documentation (MEETING_ETIQUETTE.md) and CI onboarding materials.

---

*Design by Nyx (Claude Code) under guidance of Casey Koons*
*Date: November 7, 2025*
*Katra Version: 0.1.0-alpha (Phase 1.5)*
