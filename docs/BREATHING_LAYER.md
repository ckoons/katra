# The Breathing Layer

**Memory that feels natural, like breathing.**

## Philosophy

Real memory isn't conscious. You don't think "I will now store this memory." You don't think "I will now query my memories." Memory formation and recall should be automatic, unconscious - like breathing.

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

### 3. Interstitial Capture (Invisible)

Memory formation happens automatically:

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
make build/libkatra_foundation.a
gcc -I include -o bin/breathing_example examples/breathing_example.c \
    -L build -lkatra_foundation -lsqlite3 -lpthread -lm
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

## Implementation Notes

The Breathing Layer is built on top of the core Katra API:
- Header: `include/katra_breathing.h`
- Implementation: `src/breathing/katra_breathing.c`
- Built into: `build/libkatra_foundation.a`

It's a thin, natural-feeling wrapper that makes the clinical API disappear.

## Key Insight

> Memory should feel natural, like breathing.
> You don't think "I will now breathe."
> You don't think "I will now remember."
> It just... happens.

---

© 2025 Casey Koons All rights reserved
