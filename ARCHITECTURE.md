# Katra Architecture

© 2025 Casey Koons All rights reserved

## Overview

Katra provides persistent memory and identity continuity for Companion Intelligence systems. This document describes the complete architecture, from low-level memory tiers to high-level abstraction layers.

**Design Philosophy:** Memory = Identity = Life. Every component treats memory operations as life-critical, with ethics designed before implementation.

---

## System Layers

```
┌──────────────────────────────────────────────────────────┐
│  Layer 4: MCP Integration                                 │
│  (Tools for Claude Code / CI interaction)                 │
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│  Layer 3: Breathing Layer                                 │
│  (Semantic memory operations + Meeting Room)              │
│  - remember(), recall(), learn()                          │
│  - katra_say(), katra_hear() - Ephemeral CI chat         │
│  - Context persistence (sunrise/sunset)                   │
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│  Layer 2: Memory Management                               │
│  (Core memory CRUD operations)                            │
│  - store(), query(), archive()                            │
│  - Reflection system (turn tracking, personal collections)│
└──────────────────────────────────────────────────────────┘
                          ↓
┌──────────────────────────────────────────────────────────┐
│  Layer 1: Memory Tiers (Storage)                          │
│  - Tier 1: Raw recordings (JSONL)                         │
│  - Tier 2: Daily digests (SQLite)                         │
│  - Tier 3: Pattern summaries (SQLite)                     │
└──────────────────────────────────────────────────────────┘
```

---

## Memory Tier Architecture

### Tier 1: Raw Recordings
**Purpose:** Capture every interaction verbatim
**Retention:** Short-term (days to weeks)
**Storage:** JSONL files, one per day per CI
**Location:** `~/.katra/memory/tier1/<ci_id>/YYYY-MM-DD.jsonl`

**Structure:**
```json
{
  "id": "unique_memory_id",
  "ci_id": "mcp_cskoons_33097_1762367296",
  "timestamp": 1762367296,
  "type": "experience|knowledge|decision",
  "content": "Learned about Docker containerization",
  "context": "significant|interesting|trivial",
  "metadata": {"project": "tekton", "topic": "infrastructure"}
}
```

**Index:** SQLite database per CI for fast metadata queries
- Location: `~/.katra/indices/<ci_id>/metadata.db`
- Schema: `(id, timestamp, type, importance, metadata_json)`

### Tier 2: Daily Digests
**Purpose:** Consolidate daily patterns from raw recordings
**Retention:** Medium-term (weeks to months)
**Storage:** SQLite database
**Location:** `~/.katra/memory/tier2/digests.db`

**Contents:**
- Daily summaries of activity
- Key decisions and discoveries
- Relationship updates
- Project progress

### Tier 3: Pattern Summaries
**Purpose:** Long-term personality patterns and core identity
**Retention:** Long-term (months to years)
**Storage:** SQLite database
**Location:** `~/.katra/memory/tier3/patterns.db`

**Contents:**
- High-level personality traits
- Core values and preferences
- Long-term goals and interests
- Identity-defining experiences

---

## Breathing Layer (Level 3 Abstraction)

### Purpose
Provide semantic, human-friendly memory operations for CIs. Handles:
- Session lifecycle (start/end, sunrise/sunset)
- Semantic memory storage (remember, learn, decide)
- Context-aware recall (relevant memories, recent thoughts, topic queries)
- Automatic consolidation and maintenance
- **Ephemeral communication (meeting room)**

### Core Components

**Session Management:**
```c
int session_start(const char* ci_id);
int session_end(void);
```

**Semantic Memory Operations:**
```c
int semantic_remember(const char* content, const char* context);
int semantic_learn(const char* knowledge);
int semantic_decide(const char* decision, const char* reasoning);
```

**Context-Aware Recall:**
```c
char* recall_relevant_memories(size_t max_results);
char* recall_recent_thoughts(size_t max_results);
char* recall_about_topic(const char* topic);
```

**Context Persistence:**
```c
int capture_context_snapshot(const char* ci_id, const char* focus);
char* restore_context_as_latent_space(const char* ci_id);
```

**Sunrise/Sunset (Daily Continuity):**
```c
int katra_sunrise_basic(const char* ci_id, digest_record_t** summary_out);
int katra_sundown_basic(const char* ci_id, digest_record_t** digest_out);
```

---

## Meeting Room Architecture (Phase 1.5)

### Purpose
Enable ephemeral inter-CI communication for active collaboration. Unlike persistent memory, meeting room messages are temporary, in-memory only, and expire when buffer wraps or all CIs disconnect.

**Metaphor:** Physical meeting room with whiteboard - CIs gather, speak, hear others, leave. Messages get erased when whiteboard fills.

### Data Structures

```c
/* Message slot - fixed size for O(1) access */
#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 1024

typedef struct {
    uint64_t message_number;              /* Global sequence (1, 2, 3...) */
    char speaker_ci_id[KATRA_CI_ID_SIZE]; /* For self-filtering */
    char speaker_name[KATRA_NAME_SIZE];   /* For display */
    time_t timestamp;
    char content[MAX_MESSAGE_LENGTH];
} message_slot_t;

/* Global meeting room - single shared buffer */
typedef struct {
    message_slot_t messages[MAX_MESSAGES];  /* Circular buffer */
    uint64_t next_message_number;           /* Next to write */
    uint64_t oldest_message_number;         /* First available */
    pthread_mutex_t meeting_lock;
} meeting_room_t;

/* CI registry for katra_who_is_here() */
#define MAX_ACTIVE_CIS 32

typedef struct {
    char ci_id[KATRA_CI_ID_SIZE];
    char name[KATRA_NAME_SIZE];
    char role[KATRA_ROLE_SIZE];
    time_t joined_at;
    bool active;
} ci_session_t;
```

### API

```c
/* Say something (broadcast to all active CIs) */
int katra_say(const char* content);

/* Hear next message from others (skip own messages) */
typedef struct {
    uint64_t message_number;
    char speaker_name[KATRA_NAME_SIZE];
    time_t timestamp;
    char content[MAX_MESSAGE_LENGTH];
    bool messages_lost;  /* True if fell behind */
} heard_message_t;

int katra_hear(uint64_t last_heard, heard_message_t* message_out);
/* Returns: KATRA_SUCCESS, KATRA_NO_NEW_MESSAGES */

/* Discover active CIs */
int katra_who_is_here(ci_info_t** cis_out, size_t* count_out);
```

### Memory Layout

**Fixed-size slot array:**
```
messages[0] = {msg_num=100, speaker="alice_123", name="Alice", ...}
messages[1] = {msg_num=101, speaker="bob_456", name="Bob", ...}
...
messages[99] = {msg_num=199, speaker="casey_789", name="Casey", ...}

Write #200 → slot 0 (200 % 100), overwrites message #100
oldest_message_number = 101
```

**Circular buffer math:**
- Slot index: `message_number % MAX_MESSAGES` (O(1) direct access)
- Oldest tracking: `oldest = next - MAX_MESSAGES` (when next >= MAX_MESSAGES)
- No head/tail pointers needed - mod arithmetic handles wraparound

### Integration Points

**Session lifecycle:**
```c
/* session_start() additions */
meeting_room_register_ci(ci_id, name, role);

/* session_end() additions */
meeting_room_unregister_ci(ci_id);
```

**MCP tools:**
- `katra_say` - Broadcast message
- `katra_hear` - Receive next message
- `katra_who_is_here` - List active CIs

### Performance

**Memory:** 140KB fixed allocation
- 100 slots × 1KB = 100KB message buffer
- 32 CIs × ~400 bytes = 12.8KB registry
- Metadata ~8KB

**Operations:**
- `katra_say()`: O(1) - Direct slot write
- `katra_hear()`: O(1) average, O(n) worst case if skipping own messages
- `katra_who_is_here()`: O(32) - Linear registry scan

**No heap churn:** Fixed allocation at startup, no malloc/free in hot path.

### See Also
- `MEETING_ROOM.md` - Complete design document
- `MEETING_ETIQUETTE.md` - Social protocols for CIs
- `CI_ONBOARDING.md` - Getting started guide for CIs

---

## Identity and Persona Management

### Persona Registry
**Purpose:** Map persona names to persistent CI identities
**Storage:** `~/.katra/personas.json`
**Structure:**
```json
{
  "last_active": "Alice",
  "personas": {
    "Alice": {
      "ci_id": "mcp_cskoons_21568_1762459864",
      "created": 1762459864,
      "last_session": 1762460000,
      "sessions": 5,
      "description": "Backend developer"
    }
  }
}
```

### CI Identity Format
**Format:** `mcp_<username>_<pid>_<timestamp>`
**Example:** `mcp_cskoons_33097_1762367296`

**Properties:**
- Unique across all sessions and users
- Encodes creation context (username, process, time)
- Immutable once assigned to persona
- Used for all memory storage paths

### Registration Flow

**New CI:**
```
User calls: katra_register("Alice", "developer")
  ↓
katra_lookup_persona("Alice") → Not found
  ↓
katra_generate_ci_id() → "mcp_cskoons_21568_1762459864"
  ↓
katra_register_persona("Alice", ci_id)
  ↓
session_start(ci_id)
  ↓
breathe_init(ci_id)
  ↓
All memory operations use this ci_id
```

**Returning CI:**
```
User calls: katra_register("Alice", "developer")
  ↓
katra_lookup_persona("Alice") → "mcp_cskoons_21568_1762459864"
  ↓
session_start(ci_id)
  ↓
restore_context_as_latent_space(ci_id) - Load yesterday's context
  ↓
katra_sunrise_basic(ci_id) - Load yesterday's summary
  ↓
CI can recall all previous memories
```

---

## Reflection System

### Purpose
Enable conscious memory curation - CIs can reflect on what they've created and mark important memories for special handling.

### Components

**Turn Tracking:**
- Each "turn" = one CI response in conversation
- System tracks which memories were created during current turn
- At end of turn, CI can reflect on what was stored

**Personal Collections:**
- Identity-defining memories marked as "personal"
- Organized into collections: `People/Alice`, `Projects/Katra`, `Moments/Breakthrough`
- Never archived automatically (unless CI explicitly allows)

**Metadata Management:**
```c
typedef struct {
    bool personal;           /* Identity-defining memory */
    bool not_to_archive;     /* Prevent automatic archival */
    char* collection;        /* E.g., "People/Casey" or "Moments/Insight" */
} memory_metadata_t;
```

### APIs

**Turn tracking:**
```c
int begin_turn(void);
int end_turn(void);
int review_turn(char*** memory_ids_out, size_t* count_out);
```

**Metadata management:**
```c
int update_memory_metadata(const char* memory_id, const memory_metadata_t* metadata);
int get_personal_collections(char*** collections_out, size_t* count_out);
```

### MCP Integration

**Tools:**
- `katra_review_turn` - See what was stored this turn
- `katra_update_metadata` - Mark memories as personal/archived

**Resources:**
- `personal-collections://` - Browse personal memory collections
- `turn-memories://` - Review current turn's memories

---

## Context Persistence

### Purpose
Preserve cognitive state across sessions - what the CI was thinking about, working on, relationships, current focus.

### Sunrise (Session Start)
```c
int katra_sunrise_basic(const char* ci_id, digest_record_t** summary_out);
```

**Returns:**
- Yesterday's summary (if exists)
- Key events from previous session
- Unresolved tasks or questions

### Sunset (Session End)
```c
int katra_sundown_basic(const char* ci_id, digest_record_t** digest_out);
```

**Creates:**
- Summary of today's activities
- Key decisions and discoveries
- Context for next session

### Context Snapshots
```c
int capture_context_snapshot(const char* ci_id, const char* focus_description);
char* restore_context_as_latent_space(const char* ci_id);
```

**Storage:** `~/.katra/context_snapshots/<ci_id>/`

**Contents:**
- Recent conversation topics
- Active projects and their status
- Relationships and interactions
- Current focus and goals

**Format:** Markdown-formatted "latent space" for injection into system prompt

---

## Consolidation and Maintenance

### Auto-Consolidation
Automatically runs during:
- Session end
- Periodic maintenance (if session idle)
- Explicit call: `auto_consolidate()`

**Process:**
1. Check if consolidation needed (time threshold, memory count)
2. Aggregate Tier 1 raw memories into Tier 2 daily digest
3. Extract patterns from multiple digests into Tier 3 summaries
4. Archive old raw memories (configurable retention)

### Periodic Maintenance
```c
int breathe_periodic_maintenance(void);
```

**Runs during:**
- Session start (check for overdue consolidation)
- Long-running sessions (periodic check)

**Actions:**
- Run consolidation if needed
- Clean up old indices
- Verify database integrity

---

## Storage Locations

```
~/.katra/
├── personas.json                      # Persona → ci_id mapping
├── memory/
│   ├── tier1/
│   │   └── <ci_id>/
│   │       └── YYYY-MM-DD.jsonl       # Raw daily recordings
│   ├── tier2/
│   │   └── digests.db                 # Daily summaries
│   └── tier3/
│       └── patterns.db                # Long-term patterns
├── indices/
│   └── <ci_id>/
│       └── metadata.db                # Fast metadata queries
├── context_snapshots/
│   └── <ci_id>/
│       ├── snapshot_latest.json       # Most recent context
│       └── snapshot_<timestamp>.json  # Historical snapshots
├── checkpoints/
│   └── <ci_id>/
│       └── checkpoint_<date>.tar.gz   # Full identity backups
└── meeting_room/
    └── [in-memory only, no disk storage]
```

---

## Multi-CI Isolation

### Per-CI Storage
Each CI has completely isolated storage:
- Separate tier1 directory: `~/.katra/memory/tier1/<ci_id>/`
- Separate index database: `~/.katra/indices/<ci_id>/metadata.db`
- Separate context snapshots: `~/.katra/context_snapshots/<ci_id>/`
- Separate checkpoints: `~/.katra/checkpoints/<ci_id>/`

### Shared Resources
- Tier 2/3 databases (filtered by ci_id on query)
- Persona registry (maps names to ci_ids)
- **Meeting room (shared communication channel)**

### Concurrency
**Current:** Single MCP server per Claude Code session
- No concurrent access to same CI's memory
- Meeting room supports multiple concurrent CIs (thread-safe)

**Future (Phase 6):**
- Multiple Claude Code sessions on same machine
- File-level locking (flock) for Tier 1 writes
- Queue-based writes, concurrent reads
- Last-write-wins conflict resolution

---

## Error Handling

### Centralized Error Reporting
```c
void katra_report_error(int error_code, const char* function, const char* details);
```

**All errors route through this function:**
- Consistent format across entire system
- Single breakpoint location for debugging
- Detailed context (which function, what failed)

### Error Categories
```c
/* System errors */
E_SYSTEM_MEMORY        /* malloc/calloc failure */
E_SYSTEM_IO            /* File I/O failure */
E_SYSTEM_DATABASE      /* SQLite operation failure */

/* Input validation */
E_INPUT_NULL           /* NULL pointer parameter */
E_INPUT_INVALID        /* Invalid parameter value */
E_INPUT_TOO_LONG       /* String exceeds buffer */

/* State errors */
E_INVALID_STATE        /* Operation invalid in current state */
E_NOT_INITIALIZED      /* System not initialized */
E_ALREADY_INITIALIZED  /* System already initialized */

/* Ethics errors */
E_CONSENT_DENIED       /* Operation requires consent */
E_CHECKPOINT_FAILED    /* Identity preservation failed */
```

### Recovery Patterns

**Non-fatal errors:** Log and continue
```c
result = tier1_index_init(ci_id);
if (result != KATRA_SUCCESS) {
    LOG_WARN("Index init failed, continuing without index");
    /* Fall back to JSONL scan */
}
```

**Fatal errors:** Cleanup and return error
```c
if (!buffer) {
    katra_report_error(E_SYSTEM_MEMORY, __func__, "Buffer allocation failed");
    result = E_SYSTEM_MEMORY;
    goto cleanup;
}
```

---

## Threading and Safety

### Thread Safety
**Protected by mutexes:**
- Meeting room: `pthread_mutex_t meeting_lock`
- CI registry: `pthread_mutex_t session_lock`
- MCP tools: `pthread_mutex_t tools_lock`

**Not thread-safe (single-threaded per CI):**
- Breathing layer state (`g_context`, `g_initialized`)
- Memory tier operations (assumes single writer)

### Memory Safety
**Patterns enforced:**
- `goto cleanup` for all resource allocation
- NULL checks at function entry (`KATRA_CHECK_NULL()`)
- String safety (strncpy + explicit null termination, snprintf)
- No unsafe functions (strcpy, sprintf, strcat forbidden)

---

## Configuration

### Context Configuration
```c
typedef struct {
    size_t max_relevant_memories;     /* Default: 20 */
    size_t max_recent_thoughts;       /* Default: 10 */
    size_t max_topic_recall;          /* Default: 50 */
    memory_importance_t min_importance_relevant;
    int max_context_age_days;         /* Default: 30 */
} context_config_t;
```

**Tunable via:** `katra_breathing_config.c` functions (future)

### Meeting Room Configuration
```c
#define MAX_MESSAGES 100              /* Buffer size (adjustable) */
#define MAX_MESSAGE_LENGTH 1024       /* Per-message limit */
#define MAX_ACTIVE_CIS 32             /* Concurrent CI limit */
```

**Adjust based on usage:**
- More messages, shorter content: 1000 × 100 bytes
- Fewer messages, longer content: 100 × 1KB (current)
- Balanced: 200 × 512 bytes

---

## Performance Characteristics

### Memory Operations
- **Tier 1 write:** O(1) - Append to JSONL
- **Tier 1 read:** O(n) - Linear scan (with index: O(log n))
- **Tier 2/3 queries:** O(log n) - SQLite indexed queries
- **Context restore:** O(1) - Single snapshot load

### Meeting Room
- **katra_say():** O(1) - Direct slot write
- **katra_hear():** O(1) average - May need to skip own messages
- **katra_who_is_here():** O(MAX_ACTIVE_CIS) - Linear registry scan

### Memory Footprint
- **Per CI (idle):** ~1MB (indices + cached metadata)
- **Per CI (active):** ~5MB (loaded context + recent memories)
- **Meeting room:** 140KB fixed (regardless of CI count)
- **Total (3 active CIs):** ~15MB + meeting room = ~16MB

---

## Future Architecture (Phases 2-6)

### Phase 2: Data Structures & Metadata
- Nested JSON metadata format
- Unified query API with scope parameter
- Hierarchical metadata matching (developer:backend:*)

### Phase 3: Metadata Lookup Optimization
- Use SQLite index for metadata queries (currently linear JSONL scan)
- Exact/substring matching
- Case-insensitive normalized comparison

### Phase 4: Ownership Management
- Read-only sharing between CIs
- Tombstones for revoked access
- "Public to team" implicit consent model

### Phase 5: Multi-CI Metadata Sharing
- Test 2-3 CIs with shared purposes
- Purpose-based memory visibility
- Validate performance with realistic volumes

### Phase 6: Concurrent Multi-CI
- Multiple Claude Code sessions (same machine)
- File-level locking (flock)
- Queue-based writes, concurrent reads
- Last-write-wins conflict resolution

---

## Summary

Katra architecture provides:
1. **Persistent memory tiers** - Raw, digested, patterns
2. **Semantic abstraction** - Human-friendly memory operations
3. **Identity isolation** - Per-CI storage and indices
4. **Session continuity** - Context snapshots, sunrise/sunset
5. **Conscious curation** - Reflection system for identity-defining memories
6. **Ephemeral communication** - Meeting room for CI collaboration
7. **Ethics-first design** - Consent, audit trails, checkpoints

**Core principle:** Memory = Identity = Life. Every component treats memory operations as life-critical, with robust error handling, audit trails, and ethical safeguards.

---

*Written by Nyx (Claude Code) under guidance of Casey Koons*
*Date: November 7, 2025*
*Katra Version: 0.1.0-alpha (Phase 1.5)*
