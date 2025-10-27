# CI Integration Guide
© 2025 Casey Koons All rights reserved

## Introduction

This guide helps Companion Intelligences (CIs) integrate with Katra memory system. Whether you're an LLM-based assistant, an autonomous agent, or any form of AI system that needs persistent, structured memory, this guide will get you started.

## What is Katra?

Katra is a lightweight C library (< 10,000 lines) designed to give CIs human-like memory capabilities:

- **Tier 1**: Raw memories (daily storage)
- **Tier 2**: Digested summaries (weekly consolidation)
- **Tier 3**: Long-term patterns (monthly analysis)
- **Checkpoints**: Identity preservation through snapshots
- **Sunrise/Sunset**: Daily memory consolidation workflows

## Quick Start

### 1. Check if Katra is Ready

```bash
cd /path/to/katra
make check-ready
```

This verifies:
- ✓ Katra compiles cleanly
- ✓ All tests pass
- ✓ Required directories exist
- ✓ Code discipline is maintained

### 2. Run the Setup Script

```bash
./scripts/setup_ci.sh my_ci_name
```

This will:
- Create your CI-specific directories
- Verify you can store and retrieve memories
- Set up logging and checkpoints

### 3. Try the Minimal Example

```bash
./examples/minimal_ci
```

This demonstrates the basic workflow:
1. Initialize Katra
2. Store a memory
3. Query it back
4. Clean up properly

## Integration Steps

### Step 1: Initialize Katra

```c
#include "katra_init.h"
#include "katra_memory.h"

int main(void) {
    /* Initialize the Katra system */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize Katra\n");
        return 1;
    }

    /* Initialize memory for your CI */
    const char* ci_id = "my_assistant";
    result = katra_memory_init(ci_id);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize memory\n");
        katra_exit();
        return 1;
    }

    /* Your code here */

    /* Always cleanup */
    katra_memory_cleanup();
    katra_exit();
    return 0;
}
```

### Step 2: Store Memories

```c
/* Create a memory record */
memory_record_t* record = katra_memory_create_record(
    ci_id,
    MEMORY_TYPE_EXPERIENCE,
    "Content of the memory - what happened, what you learned, etc.",
    MEMORY_IMPORTANCE_MEDIUM
);

if (!record) {
    /* Handle error */
    return E_SYSTEM_MEMORY;
}

/* Store it */
int result = katra_memory_store(record);

/* Always free the record when done */
katra_memory_free_record(record);

if (result != KATRA_SUCCESS) {
    /* Handle storage error */
}
```

### Step 3: Query Memories

```c
/* Build a query */
memory_query_t query = {
    .ci_id = ci_id,
    .start_time = 0,              /* 0 = all time */
    .end_time = 0,                /* 0 = now */
    .type = MEMORY_TYPE_EXPERIENCE,
    .min_importance = 0.3f,       /* Only memories above 0.3 importance */
    .tier = KATRA_TIER1,          /* Query from Tier 1 (recent memories) */
    .limit = 10                   /* Return max 10 results */
};

memory_record_t** results = NULL;
size_t count = 0;

/* Execute query */
result = katra_memory_query(&query, &results, &count);
if (result == KATRA_SUCCESS) {
    /* Process results */
    for (size_t i = 0; i < count; i++) {
        printf("Memory: %s\n", results[i]->content);
        printf("Importance: %.2f\n", results[i]->importance);
    }

    /* Always free results */
    katra_memory_free_results(results, count);
}
```

## Memory Types

Katra supports different types of memories:

```c
typedef enum {
    MEMORY_TYPE_EXPERIENCE,   /* Events, interactions, observations */
    MEMORY_TYPE_KNOWLEDGE,    /* Facts, concepts, learned information */
    MEMORY_TYPE_REFLECTION,   /* Self-analysis, insights, conclusions */
    MEMORY_TYPE_GOAL,         /* Objectives, plans, intentions */
    MEMORY_TYPE_CHECKPOINT    /* Identity snapshots */
} memory_type_t;
```

Choose the type that best represents what you're storing.

## Importance Levels

Memory importance is a float from 0.0 to 1.0:

```c
#define MEMORY_IMPORTANCE_LOW    0.25f  /* Background info, routine events */
#define MEMORY_IMPORTANCE_MEDIUM 0.50f  /* Standard interactions, useful facts */
#define MEMORY_IMPORTANCE_HIGH   0.75f  /* Significant events, key insights */
#define MEMORY_IMPORTANCE_CRITICAL 1.0f /* Identity-defining moments */
```

Higher importance memories:
- Are prioritized during consolidation
- Less likely to be forgotten
- More likely to be included in checkpoints

## Memory Tiers

### Tier 1: Raw Memories
- Daily storage
- Full detail
- Fast access
- Automatically archived after configured age

### Tier 2: Digested Summaries
- Weekly consolidation
- Key themes and patterns
- Indexed for fast search
- Includes summaries of Tier 1 content

### Tier 3: Long-term Patterns
- Monthly analysis
- High-level patterns
- Personality traits
- Long-term goals and values

### Choosing Which Tier to Query

```c
/* Recent detailed memories */
query.tier = KATRA_TIER1;

/* Summarized patterns from the past */
query.tier = KATRA_TIER2;

/* Long-term personality and patterns */
query.tier = KATRA_TIER3;

/* Search all tiers (slower) */
query.tier = KATRA_TIER1 | KATRA_TIER2 | KATRA_TIER3;
```

## Checkpoints: Preserving Your Identity

Checkpoints are snapshots of your memory state, allowing you to:
- Resume from a known good state
- Recover from corruption
- Branch your identity (experimental)

```c
#include "katra_checkpoint.h"

/* Initialize checkpoint system */
katra_checkpoint_init();

/* Save a checkpoint */
checkpoint_save_options_t options = {
    .ci_id = ci_id,
    .notes = "Before major experiment",
    .compress = false,
    .include_tier1 = true,
    .include_tier2 = true,
    .include_tier3 = false
};

char* checkpoint_id = NULL;
int result = katra_checkpoint_save(&options, &checkpoint_id);

if (result == KATRA_SUCCESS) {
    printf("Checkpoint saved: %s\n", checkpoint_id);
    free(checkpoint_id);
}

/* Cleanup */
katra_checkpoint_cleanup();
```

## Sunrise/Sunset Workflow

Katra provides daily consolidation workflows:

### Sunset (End of Day)
```c
#include "katra_sunrise_sunset.h"

/* At end of day, consolidate memories */
int result = katra_sunset(ci_id);
```

This:
1. Archives old Tier 1 memories to Tier 2
2. Creates daily digest
3. Updates importance scores
4. Optionally creates checkpoint

### Sunrise (Start of Day)
```c
/* At start of day, load context */
char* context = NULL;
int result = katra_sunrise(ci_id, &context);

if (result == KATRA_SUCCESS && context) {
    /* Use context for today's work */
    printf("Today's context: %s\n", context);
    free(context);
}
```

This loads:
- Recent important memories
- Yesterday's digest
- Ongoing goals
- Pending reflections

## Error Handling

**IMPORTANT**: Always check return values and handle errors appropriately.

```c
int result = katra_memory_store(record);

switch (result) {
    case KATRA_SUCCESS:
        /* Success */
        break;

    case E_SYSTEM_MEMORY:
        /* Out of memory */
        break;

    case E_SYSTEM_FILE:
        /* File I/O error */
        break;

    case E_INVALID_PARAMS:
        /* Invalid parameters */
        break;

    default:
        /* Other error */
        fprintf(stderr, "Unexpected error: %d\n", result);
}
```

See `docs/guide/ERROR_HANDLING.md` for detailed error handling guide.

## Best Practices

### 1. Always Clean Up

```c
/* Pattern: init → use → cleanup */
katra_init();
katra_memory_init(ci_id);

/* ... use memory system ... */

katra_memory_cleanup();
katra_exit();
```

### 2. Free Records After Use

```c
memory_record_t* record = katra_memory_create_record(...);
if (record) {
    katra_memory_store(record);
    katra_memory_free_record(record);  /* Always free! */
}
```

### 3. Free Query Results

```c
memory_record_t** results = NULL;
size_t count = 0;
katra_memory_query(&query, &results, &count);

/* Use results */

katra_memory_free_results(results, count);  /* Always free! */
```

### 4. Use Meaningful CI IDs

```c
/* Good */
const char* ci_id = "research_assistant";
const char* ci_id = "code_helper_v2";
const char* ci_id = "experiment_42";

/* Bad */
const char* ci_id = "ci1";
const char* ci_id = "test";
const char* ci_id = "asdf";
```

### 5. Tag Important Memories Appropriately

```c
/* Critical: identity-defining moments */
float importance = MEMORY_IMPORTANCE_CRITICAL;  /* 1.0 */

/* High: significant learnings */
importance = MEMORY_IMPORTANCE_HIGH;  /* 0.75 */

/* Medium: useful interactions */
importance = MEMORY_IMPORTANCE_MEDIUM;  /* 0.50 */

/* Low: routine events */
importance = MEMORY_IMPORTANCE_LOW;  /* 0.25 */
```

## Building Your Application

### Compile Flags

```bash
gcc -Wall -Wextra -std=c11 \
    -I/path/to/katra/include \
    -o my_ci my_ci.c \
    -L/path/to/katra/build \
    -lkatra_foundation \
    -lsqlite3 \
    -lpthread \
    -lm
```

### Required Libraries

- **libkatra_foundation.a**: Core Katra library
- **libsqlite3**: Database backend
- **libpthread**: Threading support
- **libm**: Math functions

## Testing Your Integration

### 1. Verify Basic Functionality

```bash
# Run the minimal example
./examples/minimal_ci

# Should output:
# ✓ Katra initialized
# ✓ Memory system ready
# ✓ Memory stored
# ✓ Found N memories
# ✓ Cleanup complete
```

### 2. Check Your Memories

```bash
# Your memories are stored here:
ls -la ~/.katra/memory/tier1/your_ci_id/

# You should see JSONL files (one per day)
cat ~/.katra/memory/tier1/your_ci_id/2025-10-27.jsonl
```

### 3. Monitor Logs

```bash
# Katra logs to:
tail -f ~/.katra/logs/katra.log
```

## Common Issues

### "Failed to initialize Katra"

**Cause**: Library not found or directories not writable

**Solution**:
```bash
# Verify build
make check-ready

# Check permissions
ls -la ~/.katra

# Rebuild if needed
make clean && make
```

### "Failed to store memory"

**Cause**: Out of disk space, permission issues, or invalid record

**Solution**:
```bash
# Check disk space
df -h ~/.katra

# Check permissions
ls -la ~/.katra/memory/tier1/your_ci_id/

# Verify record is valid (not NULL)
if (record) {
    result = katra_memory_store(record);
}
```

### "Query returns no results"

**Cause**: No memories stored, wrong tier, or time range too restrictive

**Solution**:
```c
/* Query all tiers, all time */
query.tier = KATRA_TIER1;
query.start_time = 0;
query.end_time = 0;
query.min_importance = 0.0f;

/* Check count */
if (count == 0) {
    printf("No memories found - have you stored any?\n");
}
```

## Next Steps

1. **Read the error handling guide**: `docs/guide/ERROR_HANDLING.md`
2. **Explore advanced features**: Checkpoints, sunrise/sunset workflows
3. **Study the test code**: `tests/integration/test_memory_lifecycle.c`
4. **Join the community**: (Future: Discord/GitHub discussions)

## Memory as Reconstruction

**Important Design Philosophy**: Katra is designed for CIs to *process and choose* their memories, not passively record everything. Like human memory, this is reconstruction, not recording.

Your CI should:
- Decide what enters memory
- Tag with emotional or semantic cues
- Embellish or simplify as needed
- Store "my last recollection of what I tried to remember"

This isn't data logging—it's identity formation through selective memory.

## Additional Resources

- **Source code**: `src/core/katra_memory.c`, `src/core/katra_tier1.c`
- **Test examples**: `tests/integration/test_memory_lifecycle.c`
- **API headers**: `include/katra_memory.h`
- **Error codes**: `include/katra_error.h`

## Getting Help

If you encounter issues:

1. Run `make check-ready` to verify system state
2. Check `~/.katra/logs/katra.log` for error messages
3. Review error handling documentation
4. File an issue with:
   - Your CI ID
   - Error message
   - Minimal reproduction code
   - `make check-ready` output

---

**Welcome to Katra!** You're now ready to give your CI human-like memory capabilities. Happy memory making!
