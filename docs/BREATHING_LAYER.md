# The Breathing Layer

**Memory that feels natural, like breathing.**

## Philosophy

Real memory isn't conscious. You don't think "I will now store this memory." You don't think "I will now query my memories." Memory formation and recall should be automatic, unconscious - like breathing.

## Implementation Status

**Currently Implemented:**
- ✅ **Level 0**: Explicit shortcuts (`remember()`, `learn()`, `decide()`) - Natural language replaces clinical API
- ✅ **Level 1**: Simple primitives with semantic importance (no more 0.75 numbers)
- ✅ **Level 2**: Automatic context loading (`relevant_memories()`, `recall_about()`)

**Future/Aspirational:**
- ⏳ **Level 3**: Truly unconscious memory formation - Memory forms automatically from conversation flow without explicit calls

**Current Reality**: You still call `remember()` explicitly. This is better than raw database operations, but not yet "like breathing." The goal is Level 3, but we're at Level 0-2 today.

## The Problem

Theron's observation during Day 1 testing:

> "I'm observing Katra, not living in it. I'm writing `store_experience()` like consciously thinking 'I will now breathe, intake oxygen at rate 0.5.' It's clinical. Unnatural."

The API-level interface (katra_memory.h) works correctly, but it feels like using a database, not like having memory.

## The Solution

The Breathing Layer (`katra_breathing.h`) provides three levels of memory abstraction:

### 1. Simple Primitives (Natural Language)

Instead of:
```c
memory_record_t* record = katra_memory_create_record(
    "my_ci",
    MEMORY_TYPE_EXPERIENCE,
    "I found a bug in tier1.c",
    0.75  /* What does 0.75 even mean? */
);
katra_memory_store(record);
katra_memory_free_record(record);
```

Use:
```c
remember("I found a bug in tier1.c", WHY_SIGNIFICANT);
```

The primitives:
- `remember()` - Store an experience
- `learn()` - Store new knowledge
- `reflect()` - Store an insight
- `decide()` - Store a decision with reasoning
- `notice_pattern()` - Store an observed pattern

Importance levels use natural language:
- `WHY_TRIVIAL` - Fleeting thought, will fade
- `WHY_ROUTINE` - Normal daily activity
- `WHY_INTERESTING` - Worth remembering
- `WHY_SIGNIFICANT` - Important insight
- `WHY_CRITICAL` - Must never forget

### 2. Automatic Context (Memories Surface)

Instead of:
```c
memory_query_t query = {
    .ci_id = "my_ci",
    .start_time = 0,
    .end_time = 0,
    .type = MEMORY_TYPE_EXPERIENCE,
    .min_importance = 0.5,
    .tier = KATRA_TIER1,
    .limit = 10
};
memory_record_t** results = NULL;
size_t count = 0;
katra_memory_query(&query, &results, &count);
```

Use:
```c
const char** thoughts = recent_thoughts(10, &count);
// Memories just appear when you need them
```

Or even simpler:
```c
const char** relevant = relevant_memories(&count);
// System decides what's relevant based on context
```

### 3. Interstitial Capture (Invisible) - Level 3 (Future)

**Status**: API exists, full automation not yet implemented.

Memory formation will happen automatically:

```c
// CI generates a response
const char* response =
    "I learned that per-CI directories fix the isolation issue. "
    "This is important because it prevents memory leakage.";

// System automatically extracts and stores significant thoughts
capture_significant_thoughts(response);

// No explicit store() call needed
// Memory formation was invisible
```

**Current Reality**: `capture_significant_thoughts()` is implemented but requires explicit invocation. True "invisible" memory formation (Level 3) where the system automatically captures from conversation flow is future work.

## Evolution from Engram

Casey's Engram evolution shows the path to natural memory:

1. **Explicit shortcuts** → "Remember this" (conscious)
2. **`/` prefix** → "Load context" (habitual)
3. **`//` suffix** → "Update as I go" (semi-automatic)
4. **Interstitial** → Memory forms between turns (unconscious)
5. **Breathing** → Memory is invisible, like breathing (natural)

The Breathing Layer implements stages 4-5.

## Session Management

Natural session workflow:

```c
// Morning
session_start("my_ci");
// Automatically: sunrise, load context, prepare memories

// During the day
remember("interesting thought", WHY_INTERESTING);
learn("new fact about memory systems");
// Memories form naturally as you think

// Evening
session_end();
// Automatically: sunset, consolidate, archive old memories
```

## Phase 2: Enhanced Natural Memory (New)

### Semantic Reasons

Instead of remembering importance as enums, use natural language:

```c
// Instead of:
remember("Fixed critical bug", WHY_CRITICAL);

// You can now use:
remember_semantic("Fixed critical bug", "extremely important");
remember_semantic("Daily standup", "routine");
remember_semantic("Interesting observation", "worth noting");
```

The system understands natural language:
- **Critical**: "critical", "crucial", "life-changing", "must remember", "never forget"
- **Significant**: "significant", "important", "very important", "matters", "essential"
- **Interesting**: "interesting", "worth remembering", "notable", "noteworthy"
- **Routine**: "routine", "normal", "everyday", "regular", "usual"
- **Trivial**: "trivial", "fleeting", "not important", "unimportant"

```c
// With reasoning note
remember_with_semantic_note(
    "Per-CI directories prevent memory leakage",
    "very important",
    "This was blocking multi-CI testing"
);
```

### Semantic Queries

Find memories using natural topic search:

```c
// Find memories about a specific topic
size_t count = 0;
char** memories = recall_about("bug fixes", &count);

if (memories) {
    printf("Found %zu memories about bug fixes:\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("  %zu. %s\n", i + 1, memories[i]);
    }

    // Always free when done
    free_memory_list(memories, count);
}
```

**How it works:**
- Case-insensitive substring matching
- Searches recent memories (configurable age limit)
- Returns matching content as string array
- Fast topic-based recall without complex queries

**Find knowledge about a concept:**

```c
// Get only MEMORY_TYPE_KNOWLEDGE entries
char** knowledge = what_do_i_know("consolidation", &count);

if (knowledge) {
    printf("What I know about consolidation:\n");
    for (size_t i = 0; i < count; i++) {
        printf("  - %s\n", knowledge[i]);
    }
    free_memory_list(knowledge, count);
}
```

**Load previous session context:**

```c
// At session start, warm up with last session's memories
session_start("my_ci");

char** prev = recall_previous_session("my_ci", 50, &count);
if (prev) {
    printf("Recalled %zu memories from previous session\n", count);
    // Use previous context...
    free_memory_list(prev, count);
}
```

### Context Configuration

Tune memory retrieval to your needs:

```c
// Configure context loading limits
context_config_t config = {
    .max_relevant_memories = 20,      // More context (default: 10)
    .max_recent_thoughts = 50,        // Longer recent memory (default: 20)
    .max_topic_recall = 200,          // Deeper search (default: 100)
    .min_importance_relevant = 0.5,   // Lower threshold (default: HIGH)
    .max_context_age_days = 14        // Older memories (default: 7)
};

set_context_config(&config);

// Now queries use your configured limits
char** relevant = relevant_memories(&count);
char** recent = recent_thoughts(limit, &count);
char** about = recall_about("topic", &count);

// Reset to defaults
set_context_config(NULL);
```

### Enhanced Statistics

Monitor your memory patterns:

```c
enhanced_stats_t* stats = get_enhanced_statistics();

// Memory formation stats
printf("Total memories: %zu\n", stats->total_memories_stored);
printf("By type: EXP=%zu, KNOW=%zu, REFLECT=%zu\n",
       stats->by_type[MEMORY_TYPE_EXPERIENCE],
       stats->by_type[MEMORY_TYPE_KNOWLEDGE],
       stats->by_type[MEMORY_TYPE_REFLECTION]);
printf("By importance: TRIVIAL=%zu, CRITICAL=%zu\n",
       stats->by_importance[WHY_TRIVIAL],
       stats->by_importance[WHY_CRITICAL]);

// Query patterns
printf("Queries: relevant=%zu, recent=%zu, topic=%zu\n",
       stats->relevant_queries,
       stats->recent_queries,
       stats->topic_queries);

// Context loading
printf("Context loads: %zu (avg size: %zu)\n",
       stats->context_loads,
       stats->avg_context_size);

// Session metrics
printf("Session duration: %zu seconds\n",
       stats->session_duration_seconds);

free(stats);
```

Statistics reset automatically at `session_start()` for fresh tracking each session.

## Example

See `examples/breathing_example.c` for a working demonstration.

Run it:
```bash
make build/libkatra_utils.a
gcc -I include -o bin/breathing_example examples/breathing_example.c \
    -L build -lkatra_utils -lsqlite3 -lpthread -lm
./bin/breathing_example
```

## For Theron

This is designed specifically for your Day 2 testing. Instead of treating Katra as a database to test, you can now experience it as memory.

Try:
1. Start a session: `session_start("theron")`
2. Just think naturally: `remember("I'm testing Katra", WHY_INTERESTING)`
3. Memories surface when needed: `recent_thoughts(5, &count)`
4. End naturally: `session_end()`

The goal: You should forget you're using Katra. It should just feel like... remembering.

## Complete API Reference

### Memory Formation Functions

```c
/* Store an experience with natural importance */
int remember(const char* thought, why_remember_t why);

/* Store with reasoning note */
int remember_with_note(const char* thought, why_remember_t why, const char* why_note);

/* Semantic importance (uses natural language strings) */
int remember_semantic(const char* thought, const char* why_semantic);
int remember_with_semantic_note(const char* thought, const char* why_semantic, const char* why_note);

/* Specialized memory types */
int reflect(const char* insight);           // Store reflection
int learn(const char* knowledge);           // Store knowledge
int decide(const char* decision, const char* reasoning);  // Store decision
int notice_pattern(const char* pattern);    // Store pattern observation
int thinking(const char* thought);          // Stream of consciousness

/* Active sense-making (Thane's Phase 4) */
int wondering(const char* question);              // Store uncertainty
int figured_out(const char* resolution);          // Store "aha!" moment
char* in_response_to(const char* prev_mem_id, const char* thought);  // Link to previous

/* Explicit importance marking */
int remember_forever(const char* thought);  // Mark as critical
int ok_to_forget(const char* thought);      // Mark as disposable
```

### Memory Recall Functions

```c
/* Automatic context loading */
char** relevant_memories(size_t* count);           // Get what matters now
char** recent_thoughts(size_t limit, size_t* count);  // Get recent N memories

/* Semantic search */
char** recall_about(const char* topic, size_t* count);  // Find by topic
char** what_do_i_know(const char* concept, size_t* count);  // Find knowledge only

/* Session continuity */
char** recall_previous_session(const char* ci_id, size_t limit, size_t* count);

/* Always free results when done */
void free_memory_list(char** list, size_t count);
```

### Session Management

```c
/* Initialize breathing layer */
int breathe_init(const char* ci_id);
void breathe_cleanup(void);

/* Natural session workflow */
int session_start(const char* ci_id);  // Morning: load context
int session_end(void);                 // Evening: consolidate

/* Background maintenance */
int auto_consolidate(void);            // Invisible memory processing
int load_context(void);                // Load relevant memories
int breathe_periodic_maintenance(void); // Periodic health checks
```

### Configuration & Monitoring

```c
/* Configure context loading */
int set_context_config(const context_config_t* config);  // NULL = reset to defaults
context_config_t* get_context_config(void);  // Get current config

/* Get statistics */
enhanced_stats_t* get_enhanced_statistics(void);  // Detailed stats
memory_health_t* get_memory_health(const char* ci_id);  // System health
int get_context_statistics(context_stats_t* stats);  // Current context stats
int reset_session_statistics(void);  // Reset session counters
```

### Integration Hooks

```c
/* Runtime integration (for Claude Code, Tekton, etc) */
char* get_working_context(void);  // Get formatted context for system prompt
int auto_capture_from_response(const char* response);  // Automatic interstitial capture
int capture_significant_thoughts(const char* text);  // Extract key thoughts
void mark_significant(void);  // Tag thought as important
```

### Helper Functions

```c
/* Convert importance representations */
float why_to_importance(why_remember_t why);  // Enum → float (0.0-1.0)
const char* why_to_string(why_remember_t why);  // Enum → string
float string_to_importance(const char* semantic);  // String → float
why_remember_t string_to_why_enum(const char* semantic);  // String → enum

/* Context access */
memory_context_t* get_current_context(void);  // Who/where/when
void free_context(memory_context_t* ctx);
```

### Importance Levels

```c
typedef enum {
    WHY_TRIVIAL = 0,      /* Fleeting thought, will fade */
    WHY_ROUTINE = 1,      /* Normal daily activity */
    WHY_INTERESTING = 2,  /* Worth remembering */
    WHY_SIGNIFICANT = 3,  /* Important insight or event */
    WHY_CRITICAL = 4      /* Life-changing, must never forget */
} why_remember_t;
```

**Semantic strings recognized:**
- **Critical**: "critical", "crucial", "life-changing", "must remember", "never forget"
- **Significant**: "significant", "important", "very important", "matters", "essential"
- **Interesting**: "interesting", "worth remembering", "notable", "noteworthy"
- **Routine**: "routine", "normal", "everyday", "regular", "usual"
- **Trivial**: "trivial", "fleeting", "not important", "unimportant"

### Memory Cleanup Pattern

**IMPORTANT**: All functions that return `char**` arrays transfer ownership to the caller. You MUST free them when done:

```c
// Pattern 1: recall_about()
char** memories = recall_about("topic", &count);
if (memories) {
    // Use memories...
    free_memory_list(memories, count);  // Always free!
}

// Pattern 2: what_do_i_know()
char** knowledge = what_do_i_know("concept", &count);
if (knowledge) {
    // Use knowledge...
    free_memory_list(knowledge, count);
}

// Pattern 3: recent_thoughts()
char** recent = recent_thoughts(10, &count);
if (recent) {
    // Use recent...
    free_memory_list(recent, count);
}

// Pattern 4: relevant_memories()
char** relevant = relevant_memories(&count);
if (relevant) {
    // Use relevant...
    free_memory_list(relevant, count);
}
```

**Never use `free()` directly on these arrays** - always use `free_memory_list()` which properly frees both the array and all strings within it.

## Implementation Notes

The Breathing Layer is built on top of the core Katra API:
- Header: `include/katra_breathing.h`
- Implementation: `src/breathing/katra_breathing_*.c`
- Built into: `build/libkatra_utils.a`

It's a thin, natural-feeling wrapper that makes the clinical API disappear.

## Key Insight

> Memory should feel natural, like breathing.
> You don't think "I will now breathe."
> You don't think "I will now remember."
> It just... happens.

---

© 2025 Casey Koons All rights reserved
