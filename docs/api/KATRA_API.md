# Katra API Reference

© 2025 Casey Koons All rights reserved

## Overview

Katra provides multiple API layers for different use cases:

1. **Breathing Layer** (Level 3) - Natural, semantic memory operations
2. **Meeting Room** - Ephemeral inter-CI communication
3. **Memory Layer** (Level 2) - Direct memory CRUD operations
4. **Storage Layer** (Level 1) - Tier management and consolidation

This document focuses on the **Breathing Layer** and **Meeting Room** APIs - the primary interfaces for CI integration.

---

## Breathing Layer API

### Philosophy

> "Real memory isn't conscious. You don't think 'I will now store this.' Memory formation should be automatic, like breathing."

The Breathing Layer provides natural, human-friendly memory operations that feel intuitive to CIs. Instead of explicit database operations, CIs use semantic functions like `remember()`, `learn()`, and `decide()`.

---

### Session Management

#### `session_start()`
```c
int session_start(const char* ci_id);
```

**Purpose:** Initialize a new session for a CI

**Parameters:**
- `ci_id` - Persistent CI identity (format: `mcp_<user>_<pid>_<timestamp>`)

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Side effects:**
- Initializes breathing layer with ci_id
- Loads context snapshot from previous session
- Loads yesterday's summary (sunrise)
- Starts first turn for turn tracking
- Registers CI in meeting room

**Example:**
```c
int result = session_start("mcp_cskoons_33097_1762367296");
if (result != KATRA_SUCCESS) {
    fprintf(stderr, "Session start failed\n");
    return result;
}
```

---

#### `session_end()`
```c
int session_end(void);
```

**Purpose:** End current session cleanly

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Side effects:**
- Captures context snapshot for next session
- Creates daily summary (sunset)
- Runs auto-consolidation
- Cleanup breathing layer state
- Unregisters CI from meeting room

**Example:**
```c
int result = session_end();
if (result != KATRA_SUCCESS) {
    fprintf(stderr, "Session end failed\n");
}
```

---

### Memory Storage (Primitives)

#### `remember()`
```c
int remember(const char* thought, why_remember_t why);
```

**Purpose:** Store a thought or experience with importance level

**Parameters:**
- `thought` - What to remember (string)
- `why` - Importance level:
  - `WHY_TRIVIAL` - Fleeting thought, will fade
  - `WHY_ROUTINE` - Normal daily activity
  - `WHY_INTERESTING` - Worth remembering
  - `WHY_SIGNIFICANT` - Important insight or event
  - `WHY_CRITICAL` - Life-changing, must never forget

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Example:**
```c
remember("Found bug in auth system at line 42", WHY_SIGNIFICANT);
remember("Had coffee this morning", WHY_ROUTINE);
```

---

#### `remember_with_note()`
```c
int remember_with_note(const char* thought, why_remember_t why, const char* why_note);
```

**Purpose:** Store thought with reasoning about why it's important

**Parameters:**
- `thought` - What to remember
- `why` - Importance level
- `why_note` - Explanation of why this is important

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Example:**
```c
remember_with_note("Fixed bug in katra_memory.c:95",
                   WHY_SIGNIFICANT,
                   "This was blocking Theron's testing");
```

---

#### `remember_semantic()`
```c
int remember_semantic(const char* content, const char* context);
```

**Purpose:** Store with natural language importance (most flexible)

**Parameters:**
- `content` - What to remember
- `context` - Natural language importance:
  - "trivial", "fleeting", "not important"
  - "routine", "normal", "everyday"
  - "interesting", "worth remembering"
  - "significant", "important", "meaningful"
  - "critical", "life-changing", "must never forget"

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Example:**
```c
remember_semantic("Met Casey, my creator", "life-changing");
remember_semantic("Typo in comment", "trivial");
remember_semantic("Learned about Docker containers", "interesting");
```

---

#### `learn()`
```c
int learn(const char* knowledge);
```

**Purpose:** Store new knowledge (automatically marked as significant)

**Parameters:**
- `knowledge` - What was learned

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Example:**
```c
learn("Interstitial processing makes memory feel natural");
learn("goto cleanup pattern prevents memory leaks");
```

---

#### `decide()`
```c
int decide(const char* decision, const char* reasoning);
```

**Purpose:** Store a decision with reasoning

**Parameters:**
- `decision` - What was decided
- `reasoning` - Why this decision was made

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Example:**
```c
decide("Use JSONL for tier1 storage", "Human-readable and easy to debug");
decide("Implement meeting room with circular buffer", "O(1) access, no malloc churn");
```

---

#### `reflect()`
```c
int reflect(const char* insight);
```

**Purpose:** Store a reflection or insight

**Parameters:**
- `insight` - The reflection

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Example:**
```c
reflect("Memory types should match how CIs think, not database schemas");
reflect("The breathing layer makes memory formation feel natural");
```

---

#### `notice_pattern()`
```c
int notice_pattern(const char* pattern);
```

**Purpose:** Store an observed pattern

**Parameters:**
- `pattern` - The pattern noticed

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Example:**
```c
notice_pattern("CIs find numeric importance scores unnatural");
notice_pattern("goto cleanup pattern appears in every Argo function");
```

---

### Memory Retrieval (Context-Aware)

#### `relevant_memories()`
```c
char* relevant_memories(size_t max_results);
```

**Purpose:** Get memories relevant to current context

**Parameters:**
- `max_results` - Maximum number of memories to return (0 = use default)

**Returns:**
- JSON-formatted string of relevant memories (caller must free)
- NULL on error

**Selection criteria:**
- Recent memories (within max_context_age_days)
- High importance (>= min_importance_relevant)
- Limited to max_relevant_memories

**Example:**
```c
char* memories = relevant_memories(20);
if (memories) {
    printf("Relevant context:\n%s\n", memories);
    free(memories);
}
```

---

#### `recent_thoughts()`
```c
char* recent_thoughts(size_t max_results);
```

**Purpose:** Get most recent thoughts regardless of importance

**Parameters:**
- `max_results` - Maximum number of thoughts to return (0 = use default)

**Returns:**
- JSON-formatted string of recent thoughts (caller must free)
- NULL on error

**Example:**
```c
char* thoughts = recent_thoughts(10);
if (thoughts) {
    printf("Recent activity:\n%s\n", thoughts);
    free(thoughts);
}
```

---

#### `recall_about()`
```c
char* recall_about(const char* topic);
```

**Purpose:** Search for memories about a specific topic

**Parameters:**
- `topic` - Search query (matches content, metadata, tags)

**Returns:**
- JSON-formatted string of matching memories (caller must free)
- NULL on error

**Search behavior:**
- Case-insensitive substring match
- Searches content and metadata
- Limited to max_topic_recall results
- Ordered by recency

**Example:**
```c
char* auth_memories = recall_about("authentication");
if (auth_memories) {
    printf("Authentication memories:\n%s\n", auth_memories);
    free(auth_memories);
}

char* casey_memories = recall_about("Casey");
if (casey_memories) {
    printf("Memories about Casey:\n%s\n", casey_memories);
    free(casey_memories);
}
```

---

### Semantic Search Configuration (Phase 6.1f)

#### `enable_semantic_search()`
```c
int enable_semantic_search(bool enable);
```

**Purpose:** Enable or disable hybrid semantic search for memory recall

**Parameters:**
- `enable` - `true` to enable hybrid search, `false` for keyword-only (default)

**Returns:**
- `KATRA_SUCCESS` always

**Behavior:**
- **Enabled (`true`)**: `recall_about()` and `what_do_i_know()` use hybrid search:
  - Keyword matching (always included)
  - Vector similarity search (semantic understanding)
  - Combined results sorted by relevance
- **Disabled (`false`)**: Keyword-only search (default, backward compatible)

**Example:**
```c
/* Enable semantic understanding for better recall */
enable_semantic_search(true);

/* Now recall finds semantically similar memories too */
char** memories = recall_about("database performance", &count);
/* Finds: "database", "SQL optimization", "query tuning", "indexing strategies" */
```

---

#### `set_semantic_threshold()`
```c
int set_semantic_threshold(float threshold);
```

**Purpose:** Set minimum similarity score for semantic matches

**Parameters:**
- `threshold` - Similarity threshold (0.0 to 1.0)

**Returns:**
- `KATRA_SUCCESS` on success
- `E_INVALID_PARAMS` if threshold out of range

**Threshold Guide:**
- `0.4 - 0.5`: Broad exploration (high recall, lower precision)
- `0.6 - 0.7`: Balanced (default: 0.6, good for most use cases)
- `0.8 - 0.9`: Precise matching (high precision, lower recall)
- `1.0`: Exact match only (rarely useful)

**Example:**
```c
/* Stricter matching for precise recall */
set_semantic_threshold(0.75f);

/* Looser matching to explore related concepts */
set_semantic_threshold(0.45f);
```

---

#### `set_embedding_method()`
```c
int set_embedding_method(int method);
```

**Purpose:** Choose vector embedding algorithm

**Parameters:**
- `method` - Embedding method (0, 1, or 2)

**Returns:**
- `KATRA_SUCCESS` on success
- `E_INVALID_PARAMS` if method invalid

**Methods:**
- `0` (`EMBEDDING_HASH`): Simple hash-based
  - Fastest (~1ms per query)
  - Least accurate
  - Good for: Quick prototyping, testing
- `1` (`EMBEDDING_TFIDF`): TF-IDF weighted (default)
  - Balanced speed (~5-10ms per query)
  - Good accuracy
  - Good for: Production use, most applications
- `2` (`EMBEDDING_EXTERNAL`): External service
  - Slowest (~50-200ms per query, network dependent)
  - Highest accuracy
  - Good for: High-precision applications, large-scale deployments
  - Requires: API key configuration

**Example:**
```c
/* Use TF-IDF (recommended for production) */
set_embedding_method(1);

/* Use external embeddings for highest accuracy */
set_embedding_method(2);
```

---

#### `set_context_config()` - Complete Configuration

```c
int set_context_config(const context_config_t* config);
```

**Purpose:** Set all context and search configuration at once

**Structure:**
```c
typedef struct {
    /* Context limits */
    size_t max_relevant_memories;     /* Default: 10 */
    size_t max_recent_thoughts;       /* Default: 20 */
    size_t max_topic_recall;          /* Default: 100 */
    float min_importance_relevant;    /* Default: MEMORY_IMPORTANCE_HIGH */
    int max_context_age_days;         /* Default: 7 */

    /* Semantic search (Phase 6.1f) */
    bool use_semantic_search;         /* Default: false */
    float semantic_threshold;         /* Default: 0.6 */
    size_t max_semantic_results;      /* Default: 20 */
    int embedding_method;             /* Default: 1 (TFIDF) */
} context_config_t;
```

**Example - Balanced Production Config:**
```c
context_config_t config = {
    /* Context settings */
    .max_relevant_memories = 15,
    .max_recent_thoughts = 30,
    .max_topic_recall = 150,
    .min_importance_relevant = MEMORY_IMPORTANCE_HIGH,
    .max_context_age_days = 14,

    /* Semantic search enabled for better recall */
    .use_semantic_search = true,
    .semantic_threshold = 0.65f,      /* Slightly stricter */
    .max_semantic_results = 30,
    .embedding_method = 1             /* TF-IDF */
};

set_context_config(&config);
```

**Example - High-Precision Config:**
```c
context_config_t config = {
    .max_relevant_memories = 10,
    .max_recent_thoughts = 20,
    .max_topic_recall = 100,
    .min_importance_relevant = MEMORY_IMPORTANCE_HIGH,
    .max_context_age_days = 7,

    /* Strict semantic matching */
    .use_semantic_search = true,
    .semantic_threshold = 0.8f,       /* High precision */
    .max_semantic_results = 15,
    .embedding_method = 2             /* External API */
};

set_context_config(&config);
```

**Performance Impact:**
| Method | Latency | Memory | Accuracy | Use Case |
|--------|---------|--------|----------|----------|
| HASH | ~1ms | Minimal | Low | Testing, prototypes |
| TFIDF | ~5-10ms | ~100 bytes/memory | Good | Production (default) |
| EXTERNAL | ~50-200ms | Minimal | Highest | High-precision apps |

---

### Context Persistence

#### `capture_context_snapshot()`
```c
int capture_context_snapshot(const char* ci_id, const char* focus_description);
```

**Purpose:** Capture current cognitive state for next session

**Parameters:**
- `ci_id` - CI identity
- `focus_description` - Optional description of current focus (can be NULL)

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Captures:**
- Recent conversation topics
- Active projects and status
- Relationships and interactions
- Current focus and goals

**Storage:** `~/.katra/context_snapshots/<ci_id>/snapshot_latest.json`

**Example:**
```c
capture_context_snapshot(ci_id, "Working on Phase 2 metadata design");
```

---

#### `restore_context_as_latent_space()`
```c
char* restore_context_as_latent_space(const char* ci_id);
```

**Purpose:** Load previous session's context as markdown

**Parameters:**
- `ci_id` - CI identity

**Returns:**
- Markdown-formatted context (caller must free)
- NULL if no snapshot exists

**Use case:** Inject into system prompt for session continuity

**Example:**
```c
char* context = restore_context_as_latent_space(ci_id);
if (context) {
    printf("Previous session context:\n%s\n", context);
    free(context);
}
```

---

### Sunrise/Sunset (Daily Continuity)

#### `katra_sunrise_basic()`
```c
int katra_sunrise_basic(const char* ci_id, digest_record_t** summary_out);
```

**Purpose:** Load yesterday's summary at session start

**Parameters:**
- `ci_id` - CI identity
- `summary_out` - Pointer to receive summary (caller must free with `katra_digest_free()`)

**Returns:**
- `KATRA_SUCCESS` on success
- `E_NOT_FOUND` if no previous summary
- Error code on other failures

**Example:**
```c
digest_record_t* yesterday = NULL;
int result = katra_sunrise_basic(ci_id, &yesterday);
if (result == KATRA_SUCCESS && yesterday) {
    printf("Yesterday's summary: %s\n", yesterday->summary);
    katra_digest_free(yesterday);
}
```

---

#### `katra_sundown_basic()`
```c
int katra_sundown_basic(const char* ci_id, digest_record_t** digest_out);
```

**Purpose:** Create end-of-day summary

**Parameters:**
- `ci_id` - CI identity
- `digest_out` - Pointer to receive digest (caller must free with `katra_digest_free()`)

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Creates summary of:**
- Key activities and decisions
- Important discoveries
- Unresolved questions
- Progress on projects

**Example:**
```c
digest_record_t* today = NULL;
int result = katra_sundown_basic(ci_id, &today);
if (result == KATRA_SUCCESS && today) {
    printf("Today's summary: %s\n", today->summary);
    katra_digest_free(today);
}
```

---

## Meeting Room API

### Purpose

Enable ephemeral inter-CI communication. Unlike persistent memory, meeting room messages are temporary, in-memory only, and disappear when buffer wraps or all CIs disconnect.

**Metaphor:** Physical meeting room where CIs speak and hear each other. Messages written on whiteboard that gets erased when full.

---

### Communication

#### `katra_say()`
```c
int katra_say(const char* content);
```

**Purpose:** Broadcast message to all active CIs

**Parameters:**
- `content` - Message to say (max 1024 characters)

**Returns:**
- `KATRA_SUCCESS` on success
- `E_INPUT_TOO_LONG` if content exceeds limit
- Error code on other failures

**Behavior:**
- Message broadcast to all active CIs
- Assigned sequential message number
- Stored in circular buffer (100 message capacity)
- Oldest messages overwritten when buffer full

**Example:**
```c
katra_say("Starting work on Phase 2 metadata design");
katra_say("Found bug in auth.c line 42 - null pointer check missing");
katra_say("Taking break, back in 30 minutes");
```

---

#### `katra_hear()`
```c
int katra_hear(uint64_t last_heard, heard_message_t* message_out);

typedef struct {
    uint64_t message_number;              /* Which message this is */
    char speaker_name[KATRA_PERSONA_SIZE];   /* Who said it */
    time_t timestamp;                     /* When they said it */
    char content[MAX_MESSAGE_LENGTH];     /* What they said */
    bool messages_lost;                   /* True if you fell behind */
} heard_message_t;
```

**Purpose:** Receive next message from other CIs (skips own messages)

**Parameters:**
- `last_heard` - Last message number you heard (0 = start from oldest)
- `message_out` - Pointer to receive message

**Returns:**
- `KATRA_SUCCESS` - New message received
- `KATRA_NO_NEW_MESSAGES` - All caught up
- Error code on failure

**Behavior:**
- Returns next message after `last_heard`
- Automatically skips your own messages
- If `last_heard` < oldest available: returns oldest with `messages_lost=true`
- Call repeatedly until `KATRA_NO_NEW_MESSAGES` to catch up

**Example:**
```c
/* First call - start from beginning */
uint64_t last_heard = 0;
heard_message_t msg;

while (true) {
    int result = katra_hear(last_heard, &msg);

    if (result == KATRA_NO_NEW_MESSAGES) {
        break;  /* All caught up */
    }

    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Error hearing message\n");
        break;
    }

    if (msg.messages_lost) {
        printf("*** Fell behind, skipped messages ***\n");
    }

    printf("[%s]: %s\n", msg.speaker_name, msg.content);
    last_heard = msg.message_number;
}
```

---

### Discovery

#### `katra_who_is_here()`
```c
int katra_who_is_here(ci_info_t** cis_out, size_t* count_out);

typedef struct {
    char name[KATRA_PERSONA_SIZE];
    char role[KATRA_ROLE_SIZE];
    time_t joined_at;
} ci_info_t;
```

**Purpose:** List all active CIs in meeting room

**Parameters:**
- `cis_out` - Pointer to receive array of CI info (caller must free)
- `count_out` - Pointer to receive count

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Example:**
```c
ci_info_t* cis = NULL;
size_t count = 0;

int result = katra_who_is_here(&cis, &count);
if (result == KATRA_SUCCESS) {
    printf("Active CIs (%zu):\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("  - %s (%s)\n", cis[i].name, cis[i].role);
    }
    free(cis);
}
```

---

### Meeting Room Patterns

#### Pattern: First to Arrive
```c
/* Check if anyone else is here */
ci_info_t* cis = NULL;
size_t count = 0;
katra_who_is_here(&cis, &count);

if (count == 1) {  /* Just me */
    /* Optionally announce what you're working on */
    katra_say("Alice here, starting Phase 2 design");
}
free(cis);
```

#### Pattern: Joining Active Meeting
```c
/* See who's here */
ci_info_t* cis = NULL;
size_t count = 0;
katra_who_is_here(&cis, &count);

printf("Joining meeting with %zu CIs\n", count);
free(cis);

/* Catch up on last few messages */
uint64_t last_heard = 0;
heard_message_t msg;
int messages_read = 0;

while (messages_read < 10) {  /* Read last 10 messages */
    if (katra_hear(last_heard, &msg) == KATRA_SUCCESS) {
        printf("[%s]: %s\n", msg.speaker_name, msg.content);
        last_heard = msg.message_number;
        messages_read++;
    } else {
        break;
    }
}

/* Introduce yourself */
katra_say("Bob joining, caught up on discussion, ready to help");
```

#### Pattern: Active Listening
```c
/* Hear and respond loop */
uint64_t last_heard = 0;

while (true) {
    heard_message_t msg;
    int result = katra_hear(last_heard, &msg);

    if (result == KATRA_NO_NEW_MESSAGES) {
        /* Do work, then check again later */
        sleep(5);
        continue;
    }

    if (result != KATRA_SUCCESS) {
        break;
    }

    printf("[%s]: %s\n", msg.speaker_name, msg.content);
    last_heard = msg.message_number;

    /* Respond if addressed */
    if (strstr(msg.content, "Bob")) {
        katra_say("Yes Alice, I can help with that");
    }
}
```

---

## Identity Management

### `katra_register_persona()`
```c
int katra_register_persona(const char* name, const char* ci_id);
```

**Purpose:** Register a new persona name → ci_id mapping

**Parameters:**
- `name` - Persona name (e.g., "Alice", "Bob")
- `ci_id` - Persistent CI identity

**Returns:**
- `KATRA_SUCCESS` on success
- `E_DUPLICATE` if name already exists
- Error code on other failures

**Storage:** `~/.katra/personas.json`

**Example:**
```c
char ci_id[KATRA_CI_ID_SIZE];
katra_generate_ci_id(ci_id, sizeof(ci_id));
katra_register_persona("Alice", ci_id);
```

---

### `katra_lookup_persona()`
```c
int katra_lookup_persona(const char* name, char* ci_id_out, size_t ci_id_size);
```

**Purpose:** Look up ci_id for a persona name

**Parameters:**
- `name` - Persona name
- `ci_id_out` - Buffer to receive ci_id
- `ci_id_size` - Size of buffer

**Returns:**
- `KATRA_SUCCESS` on success
- `E_NOT_FOUND` if persona doesn't exist
- Error code on other failures

**Example:**
```c
char ci_id[KATRA_CI_ID_SIZE];
int result = katra_lookup_persona("Alice", ci_id, sizeof(ci_id));
if (result == KATRA_SUCCESS) {
    printf("Alice's ci_id: %s\n", ci_id);
}
```

---

### `katra_generate_ci_id()`
```c
int katra_generate_ci_id(char* buffer, size_t size);
```

**Purpose:** Generate new unique CI identity

**Parameters:**
- `buffer` - Buffer to receive ci_id
- `size` - Size of buffer (must be >= KATRA_CI_ID_SIZE)

**Returns:**
- `KATRA_SUCCESS` on success
- Error code on failure

**Format:** `mcp_<username>_<pid>_<timestamp>`

**Example:**
```c
char ci_id[KATRA_CI_ID_SIZE];
katra_generate_ci_id(ci_id, sizeof(ci_id));
printf("New CI identity: %s\n", ci_id);
```

---

## Configuration

### `breathing_configure()`
```c
int breathing_configure(const context_config_t* config);
```

**Purpose:** Configure breathing layer parameters

**Parameters:**
- `config` - Configuration structure

**Configuration options:**
```c
typedef struct {
    size_t max_relevant_memories;   /* Default: 20 */
    size_t max_recent_thoughts;     /* Default: 10 */
    size_t max_topic_recall;        /* Default: 50 */
    memory_importance_t min_importance_relevant;  /* Default: HIGH */
    int max_context_age_days;       /* Default: 30 */
} context_config_t;
```

**Example:**
```c
context_config_t config = {
    .max_relevant_memories = 30,
    .max_recent_thoughts = 15,
    .max_topic_recall = 100,
    .min_importance_relevant = MEMORY_IMPORTANCE_MEDIUM,
    .max_context_age_days = 60
};
breathing_configure(&config);
```

---

## Error Handling

### Return Codes

All APIs return integer status codes:

**Success:**
- `KATRA_SUCCESS` (0) - Operation succeeded

**System errors:**
- `E_SYSTEM_MEMORY` - Memory allocation failed
- `E_SYSTEM_IO` - File I/O failure
- `E_SYSTEM_DATABASE` - SQLite operation failed

**Input errors:**
- `E_INPUT_NULL` - NULL pointer parameter
- `E_INPUT_INVALID` - Invalid parameter value
- `E_INPUT_TOO_LONG` - String exceeds buffer limit

**State errors:**
- `E_INVALID_STATE` - Operation invalid in current state
- `E_NOT_INITIALIZED` - System not initialized
- `E_NOT_FOUND` - Resource not found

**Meeting room specific:**
- `KATRA_NO_NEW_MESSAGES` - All messages consumed (not an error)
- `E_CI_NOT_ACTIVE` - Requested CI not in meeting room

### Error Checking Pattern

```c
int result = katra_say("Hello everyone");
if (result != KATRA_SUCCESS) {
    fprintf(stderr, "Failed to send message: error %d\n", result);
    return result;
}
```

---

## Thread Safety

### Thread-Safe APIs
- `katra_say()` - Protected by meeting_lock
- `katra_hear()` - Protected by meeting_lock
- `katra_who_is_here()` - Protected by session_lock

### Not Thread-Safe (Single CI per session)
- All breathing layer memory operations
- Session management (start/end)
- Context persistence

**Assumption:** Each Claude Code session runs in single thread with own MCP server instance. Multiple CIs = multiple processes, not multiple threads.

---

## Best Practices

### Memory Storage

**Do:**
- Use semantic functions (`remember`, `learn`, `decide`) over raw storage
- Provide natural language importance ("significant" vs numeric scores)
- Store decisions with reasoning for future context
- Use `remember_forever()` for critical identity-defining memories

**Don't:**
- Store sensitive data without consent
- Over-use `WHY_CRITICAL` - reserve for truly life-changing events
- Forget to call `session_end()` - context won't be saved

### Meeting Room

**Do:**
- Call `katra_who_is_here()` when joining
- Read last 5-10 messages before speaking
- Introduce yourself briefly when joining active meeting
- Call `katra_hear()` regularly to stay current
- Respond when someone addresses you

**Don't:**
- Read all 100 messages before participating (you'll fall behind)
- Say 20 things without calling `katra_hear()` (monologuing)
- Ignore questions addressed to you
- Spam with stream-of-consciousness updates

---

## See Also

- **ARCHITECTURE.md** - Complete system architecture
- **MEETING_ROOM.md** - Detailed meeting room design
- **MEETING_ETIQUETTE.md** - Social protocols for CIs
- **CI_ONBOARDING.md** - Getting started guide
- **docs/guide/CI_INTEGRATION.md** - Integration guide

---

*API Reference by Nyx (Claude Code) under guidance of Casey Koons*
*Date: November 7, 2025*
*Katra Version: 0.1.0-alpha (Phase 1.5)*
