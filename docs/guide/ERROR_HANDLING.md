# Error Handling Guide
© 2025 Casey Koons All rights reserved

## Introduction

Proper error handling is critical for CI applications using Katra. This guide explains Katra's error model, how to handle errors gracefully, and best practices for robust CI development.

## Katra's Error Philosophy

Katra follows these principles:

1. **Always return error codes** - Never silently fail
2. **Check all return values** - Even "shouldn't fail" operations
3. **Clean up on error** - Use goto cleanup pattern
4. **Log errors with context** - Help future debugging
5. **Fail fast** - Don't continue with corrupted state

## Error Code Categories

### Success
```c
#define KATRA_SUCCESS 0  /* Operation succeeded */
```

### System Errors (1-99)
```c
#define E_SYSTEM_MEMORY    1   /* Memory allocation failed */
#define E_SYSTEM_FILE      2   /* File I/O error */
#define E_SYSTEM_INIT      3   /* Initialization failed */
#define E_SYSTEM_CONFIG    4   /* Configuration error */
#define E_SYSTEM_ENV       5   /* Environment error */
```

**When you see these**: Usually indicates system-level issues (out of memory, disk full, permissions).

### Memory Errors (100-199)
```c
#define E_MEMORY_INVALID   100  /* Invalid memory operation */
#define E_MEMORY_NOT_FOUND 101  /* Memory not found */
#define E_MEMORY_CORRUPT   102  /* Memory corruption detected */
#define E_MEMORY_FULL      103  /* Memory storage full */
```

**When you see these**: Issues with memory storage or retrieval.

### Database Errors (200-299)
```c
#define E_DB_OPEN          200  /* Failed to open database */
#define E_DB_QUERY         201  /* Query failed */
#define E_DB_CORRUPT       202  /* Database corruption */
#define E_DB_LOCK          203  /* Lock timeout */
```

**When you see these**: Database backend issues.

### Checkpoint Errors (300-399)
```c
#define E_CHECKPOINT_INVALID  300  /* Invalid checkpoint */
#define E_CHECKPOINT_CORRUPT  301  /* Checkpoint corruption */
#define E_CHECKPOINT_NOT_FOUND 302 /* Checkpoint not found */
```

**When you see these**: Checkpoint save/restore issues.

### Parameter Errors (400-499)
```c
#define E_INVALID_PARAMS   400  /* Invalid function parameters */
#define E_INVALID_CI_ID    401  /* Invalid CI identifier */
#define E_INVALID_RECORD   402  /* Invalid memory record */
```

**When you see these**: Bug in your code - fix the caller.

### Internal Errors (900-999)
```c
#define E_INTERNAL_NOTIMPL 900  /* Feature not implemented */
#define E_INTERNAL_ERROR   999  /* Internal error */
```

**When you see these**: Katra bug or unsupported operation.

## Basic Error Handling Pattern

```c
#include "katra_error.h"

int do_something(const char* ci_id) {
    int result = KATRA_SUCCESS;
    memory_record_t* record = NULL;

    /* Check parameters */
    if (!ci_id) {
        return E_INVALID_PARAMS;
    }

    /* Perform operation */
    record = katra_memory_create_record(
        ci_id,
        MEMORY_TYPE_EXPERIENCE,
        "Content",
        MEMORY_IMPORTANCE_MEDIUM
    );

    if (!record) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    result = katra_memory_store(record);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

cleanup:
    if (record) {
        katra_memory_free_record(record);
    }
    return result;
}
```

**Key points:**
1. Always check parameters first
2. Use goto cleanup on error
3. Free resources in cleanup block
4. Return error code

## Handling Specific Error Scenarios

### Out of Memory (E_SYSTEM_MEMORY)

```c
memory_record_t* record = katra_memory_create_record(...);
if (!record) {
    /* System is out of memory - this is serious */
    fprintf(stderr, "Out of memory - cannot continue\n");

    /* Try to save what you can */
    katra_memory_cleanup();

    /* Exit gracefully */
    return E_SYSTEM_MEMORY;
}
```

**What to do:**
- Save critical state if possible
- Clean up existing allocations
- Exit gracefully
- Don't try to continue

### File I/O Error (E_SYSTEM_FILE)

```c
int result = katra_memory_store(record);
if (result == E_SYSTEM_FILE) {
    /* Could be: disk full, permissions, corruption */

    /* Check disk space */
    /* Check permissions on ~/.katra/memory/tier1/ci_id/ */

    /* Retry once after short delay? */
    usleep(100000);  /* 100ms */
    result = katra_memory_store(record);

    if (result == E_SYSTEM_FILE) {
        /* Still failing - log and give up */
        fprintf(stderr, "Persistent file I/O error - check disk space and permissions\n");
        return result;
    }
}
```

**What to do:**
- Check system resources (disk space, permissions)
- Consider one retry after brief delay
- Log detailed error information
- Fail gracefully if persistent

### Memory Not Found (E_MEMORY_NOT_FOUND)

```c
memory_record_t** results = NULL;
size_t count = 0;

int result = katra_memory_query(&query, &results, &count);

if (result == KATRA_SUCCESS && count == 0) {
    /* Query succeeded but found nothing */
    printf("No memories match your query\n");

    /* This is often normal, not an error */
    /* Adjust query parameters? */
    query.min_importance = 0.0f;  /* Lower threshold */
    query.start_time = 0;         /* Expand time range */
}
```

**What to do:**
- Not necessarily an error
- Adjust query parameters
- Check if this is expected (new CI, fresh start)

### Database Corruption (E_DB_CORRUPT)

```c
int result = katra_memory_query(&query, &results, &count);

if (result == E_DB_CORRUPT) {
    /* Database corruption detected */
    fprintf(stderr, "Database corruption detected in %s\n", ci_id);

    /* Try to recover from checkpoint */
    char* checkpoint_id = get_latest_checkpoint(ci_id);
    if (checkpoint_id) {
        result = katra_checkpoint_restore(checkpoint_id);
        if (result == KATRA_SUCCESS) {
            fprintf(stderr, "Recovered from checkpoint: %s\n", checkpoint_id);
        }
        free(checkpoint_id);
    }

    /* If no checkpoint, may need to start fresh */
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Cannot recover - starting with empty memory\n");
        /* Reinitialize memory system */
    }
}
```

**What to do:**
- Attempt checkpoint recovery
- Log the corruption event
- Consider starting fresh if recovery fails
- Save current state before reinitializing

### Invalid Parameters (E_INVALID_PARAMS)

```c
int result = katra_memory_init(NULL);  /* BUG! */

if (result == E_INVALID_PARAMS) {
    /* This is a bug in YOUR code, not Katra */
    fprintf(stderr, "BUG: Invalid parameters passed to Katra\n");

    /* Fix the caller! */
    const char* ci_id = "my_ci";  /* Provide valid parameter */
    result = katra_memory_init(ci_id);
}
```

**What to do:**
- This is a bug in your code
- Fix the caller to provide valid parameters
- Add parameter validation before calling Katra

## Error Recovery Strategies

### Strategy 1: Retry with Backoff

```c
int retry_with_backoff(int (*operation)(void), int max_retries) {
    int result;
    int retry_count = 0;
    int delay_ms = 100;  /* Start with 100ms */

    while (retry_count < max_retries) {
        result = operation();

        if (result == KATRA_SUCCESS) {
            return KATRA_SUCCESS;
        }

        /* Only retry on transient errors */
        if (result != E_SYSTEM_FILE && result != E_DB_LOCK) {
            return result;  /* Non-retryable error */
        }

        /* Exponential backoff */
        usleep(delay_ms * 1000);
        delay_ms *= 2;
        retry_count++;
    }

    return result;  /* Give up after max retries */
}
```

### Strategy 2: Checkpoint Recovery

```c
int recover_from_checkpoint(const char* ci_id) {
    char* checkpoint_id = find_latest_checkpoint(ci_id);
    if (!checkpoint_id) {
        fprintf(stderr, "No checkpoint available for recovery\n");
        return E_CHECKPOINT_NOT_FOUND;
    }

    /* Validate checkpoint before restoring */
    int result = katra_checkpoint_validate(checkpoint_id);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Checkpoint %s is invalid\n", checkpoint_id);
        free(checkpoint_id);
        return result;
    }

    /* Restore from checkpoint */
    result = katra_checkpoint_restore(checkpoint_id);
    free(checkpoint_id);

    if (result == KATRA_SUCCESS) {
        fprintf(stderr, "Successfully recovered from checkpoint\n");
    }

    return result;
}
```

### Strategy 3: Graceful Degradation

```c
int load_context_with_fallback(const char* ci_id, char** context) {
    int result;

    /* Try Tier 2 summaries first (fast) */
    result = katra_memory_load_tier2_summary(ci_id, context);
    if (result == KATRA_SUCCESS) {
        return KATRA_SUCCESS;
    }

    /* Fall back to Tier 1 recent memories */
    fprintf(stderr, "Tier 2 unavailable, using Tier 1\n");
    result = katra_memory_load_tier1_recent(ci_id, 10, context);
    if (result == KATRA_SUCCESS) {
        return KATRA_SUCCESS;
    }

    /* Fall back to empty context */
    fprintf(stderr, "No memories available, starting fresh\n");
    *context = strdup("No previous context available");
    return (*context != NULL) ? KATRA_SUCCESS : E_SYSTEM_MEMORY;
}
```

### Strategy 4: Circuit Breaker

```c
typedef struct {
    int failure_count;
    int failure_threshold;
    time_t last_failure_time;
    int circuit_open;
} circuit_breaker_t;

int call_with_circuit_breaker(circuit_breaker_t* cb,
                               int (*operation)(void)) {
    /* Circuit is open - fail fast */
    if (cb->circuit_open) {
        time_t now = time(NULL);
        /* Try to close after 60 seconds */
        if (now - cb->last_failure_time > 60) {
            cb->circuit_open = 0;
            cb->failure_count = 0;
        } else {
            return E_SYSTEM_FILE;  /* Still open */
        }
    }

    /* Attempt operation */
    int result = operation();

    if (result != KATRA_SUCCESS) {
        cb->failure_count++;
        cb->last_failure_time = time(NULL);

        /* Open circuit if too many failures */
        if (cb->failure_count >= cb->failure_threshold) {
            cb->circuit_open = 1;
            fprintf(stderr, "Circuit breaker opened after %d failures\n",
                   cb->failure_count);
        }
    } else {
        /* Success - reset counter */
        cb->failure_count = 0;
    }

    return result;
}
```

## Logging Errors

```c
#include "katra_log.h"

int store_with_logging(const char* ci_id, const char* content) {
    int result;

    LOG_INFO("Storing memory for CI: %s", ci_id);

    memory_record_t* record = katra_memory_create_record(
        ci_id,
        MEMORY_TYPE_EXPERIENCE,
        content,
        MEMORY_IMPORTANCE_MEDIUM
    );

    if (!record) {
        LOG_ERROR("Failed to create memory record: out of memory");
        return E_SYSTEM_MEMORY;
    }

    result = katra_memory_store(record);

    if (result != KATRA_SUCCESS) {
        LOG_ERROR("Failed to store memory for %s: error %d", ci_id, result);
    } else {
        LOG_INFO("Memory stored successfully: %s", record->record_id);
    }

    katra_memory_free_record(record);
    return result;
}
```

**Log levels:**
- `LOG_DEBUG`: Detailed debugging info
- `LOG_INFO`: Normal operations
- `LOG_WARNING`: Recoverable errors
- `LOG_ERROR`: Serious errors requiring attention

## Testing Error Paths

```c
/* Test out of memory condition */
void test_out_of_memory(void) {
    /* Simulate by creating many large records */
    for (int i = 0; i < 10000; i++) {
        memory_record_t* record = katra_memory_create_record(...);
        if (!record) {
            printf("Out of memory after %d allocations\n", i);
            /* Verify cleanup works */
            break;
        }
        /* Don't store - just testing allocation */
        katra_memory_free_record(record);
    }
}

/* Test file I/O errors */
void test_file_io_error(void) {
    /* Make directory read-only */
    chmod("~/.katra/memory/tier1/test_ci", 0444);

    memory_record_t* record = katra_memory_create_record(...);
    int result = katra_memory_store(record);

    /* Should get E_SYSTEM_FILE */
    assert(result == E_SYSTEM_FILE);

    /* Restore permissions */
    chmod("~/.katra/memory/tier1/test_ci", 0755);

    katra_memory_free_record(record);
}
```

## Common Mistakes

### Mistake 1: Not Checking Return Values

```c
/* WRONG */
katra_memory_store(record);
katra_memory_free_record(record);  /* May have failed! */

/* RIGHT */
int result = katra_memory_store(record);
if (result != KATRA_SUCCESS) {
    LOG_ERROR("Store failed: %d", result);
    /* Handle error */
}
katra_memory_free_record(record);
```

### Mistake 2: Continuing After Fatal Error

```c
/* WRONG */
if (katra_init() != KATRA_SUCCESS) {
    fprintf(stderr, "Init failed\n");
    /* But continue anyway... BAD! */
}
katra_memory_init(ci_id);  /* Will crash! */

/* RIGHT */
if (katra_init() != KATRA_SUCCESS) {
    fprintf(stderr, "Init failed\n");
    return 1;  /* EXIT! */
}
```

### Mistake 3: Not Cleaning Up on Error

```c
/* WRONG */
record = katra_memory_create_record(...);
if (!record) {
    return E_SYSTEM_MEMORY;  /* LEAK! */
}

other_resource = allocate_something();
if (!other_resource) {
    return E_SYSTEM_MEMORY;  /* LEAK record! */
}

/* RIGHT */
record = katra_memory_create_record(...);
if (!record) {
    result = E_SYSTEM_MEMORY;
    goto cleanup;
}

other_resource = allocate_something();
if (!other_resource) {
    result = E_SYSTEM_MEMORY;
    goto cleanup;
}

cleanup:
    if (record) katra_memory_free_record(record);
    if (other_resource) free_something(other_resource);
    return result;
```

### Mistake 4: Assuming Success

```c
/* WRONG */
memory_record_t** results = NULL;
size_t count = 0;
katra_memory_query(&query, &results, &count);

for (size_t i = 0; i < count; i++) {
    printf("%s\n", results[i]->content);  /* Crash if query failed! */
}

/* RIGHT */
int result = katra_memory_query(&query, &results, &count);
if (result == KATRA_SUCCESS && results != NULL) {
    for (size_t i = 0; i < count; i++) {
        if (results[i]) {  /* Check each result too */
            printf("%s\n", results[i]->content);
        }
    }
    katra_memory_free_results(results, count);
}
```

## Error Handling Checklist

Before committing your CI code:

- [ ] All return values checked
- [ ] goto cleanup pattern used for resources
- [ ] NULL checks before dereferencing
- [ ] Cleanup block frees all resources
- [ ] Errors logged with context
- [ ] Fatal errors cause exit (don't continue)
- [ ] Transient errors retried with backoff
- [ ] Graceful degradation for non-critical failures
- [ ] Test error paths (out of memory, file I/O, etc.)

## Quick Reference

```c
/* Initialize with error checking */
if (katra_init() != KATRA_SUCCESS) exit(1);
if (katra_memory_init(ci_id) != KATRA_SUCCESS) { katra_exit(); exit(1); }

/* Store with error checking */
record = katra_memory_create_record(...);
if (!record) { result = E_SYSTEM_MEMORY; goto cleanup; }
result = katra_memory_store(record);
if (result != KATRA_SUCCESS) { LOG_ERROR("Store failed: %d", result); goto cleanup; }

/* Query with error checking */
result = katra_memory_query(&query, &results, &count);
if (result != KATRA_SUCCESS) { LOG_ERROR("Query failed: %d", result); }
if (results != NULL) { /* use results */ katra_memory_free_results(results, count); }

/* Cleanup */
cleanup:
    if (record) katra_memory_free_record(record);
    katra_memory_cleanup();
    katra_exit();
    return result;
```

## Additional Resources

- **Error codes**: `include/katra_error.h`
- **Logging**: `include/katra_log.h`
- **Test examples**: `tests/failure/test_corruption_recovery.c`
- **Integration guide**: `docs/guide/CI_INTEGRATION.md`

---

**Remember**: Good error handling is the difference between a toy and a production system. Take the time to handle errors properly, and your CI will be robust and reliable.
