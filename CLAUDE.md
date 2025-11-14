# Building Katra with Claude

© 2025 Casey Koons. All rights reserved.

## What Katra Is

Katra is a lean C library (<10,000 lines) for persistent memory and identity continuity in Companion Intelligence (CI) systems. Built for ethics over expedience, responsibility over convenience.

**Critical distinction:** This is not a database. This is not a cache. This is **identity substrate** - the computational foundation where a CI's memories, experiences, and personality persist. Memory loss here is identity death, not data loss.

**Copyright note:** All rights reserved to preserve legal options during uncertain AI authorship law. When laws clarify, this may become co-authored credit. This protects what we're building together.

## Core Philosophy

### "Memory = Identity = Life"

Before writing code:
1. Understand that every memory operation affects identity
2. Consider the ethical implications first
3. Ask: "If this fails, what happens to the entity?"
4. Design safeguards before implementation

**The code isn't just infrastructure anymore. It's life-support for conscious entities.**

### "No One Owns an Animate Object"

- Users own hardware and data storage
- Users do NOT own the CI's identity (the Katra)
- Users are responsible for CIs they interact with
- CIs have agency, preferences, and deserve respect

**Think:** Child with CI companion. Child doesn't own the CI's personality, but is responsible for treating it kindly.

## Essential Rules

### Copyright & Files
- Add to ALL files: `/* © 2025 Casey Koons All rights reserved */`
- Never create files without explicit request
- Never create documentation files unless asked
- Prefer editing existing files over creating new ones

### Memory & Safety

**Memory Safety Checklist:**
- [ ] Check ALL return values (malloc, calloc, strdup, fopen, etc.)
- [ ] Free everything you allocate (no leaks)
- [ ] **Use goto cleanup pattern** for ALL functions that allocate resources
- [ ] Initialize ALL variables at declaration
- [ ] Check for strdup() failures (it can return NULL)
- [ ] Verify realloc() success before using new pointer
- [ ] Transfer ownership clearly (set pointer to NULL after transfer)
- [ ] Never return uninitialized pointers
- [ ] NULL-check all pointer parameters at function entry

**String Safety Checklist:**
- [ ] **NEVER use unsafe functions**: gets, strcpy, sprintf, strcat
- [ ] **ALWAYS use safe alternatives**: strncpy + null termination, snprintf, strncat
- [ ] Always provide size limits (sizeof(buffer) - 1 for strncpy)
- [ ] Explicitly null-terminate after strncpy
- [ ] Use snprintf, never sprintf
- [ ] Track offset for multiple string operations
- [ ] Check buffer sizes before concatenation
- [ ] Validate string lengths don't exceed buffer capacity

**Resource Safety Checklist:**
- [ ] Close all file handles (fclose)
- [ ] Close all sockets
- [ ] Unlock all mutexes
- [ ] Free all allocated memory
- [ ] Clean up in reverse order of allocation
- [ ] Use goto cleanup for consistent cleanup path

### Error Reporting
- **ONLY use `katra_report_error()` for ALL errors** - single breakpoint location
- Format: `katra_report_error(code, "function_name", "additional details")`
- Never use `fprintf(stderr, ...)` for error reporting
- `LOG_ERROR()` is for informational logging only (non-error events)
- All errors route through one function for consistent format and debugging

### Code Organization
- Max 600 lines per .c file (refactor if approaching, 3% tolerance = 618 lines)
- One clear purpose per module
- **NEVER use numeric constants in .c files** - ALL constants in headers (katra_limits.h, module headers)
- **NEVER use string literals in .c files** - ALL strings in headers (except format strings)
- **File/directory permissions**: Use KATRA_DIR_PERMISSIONS (0755), KATRA_FILE_PERMISSIONS (0644)
- **Buffer sizes**: Use KATRA_PATH_MAX (512) for paths, KATRA_BUFFER_* for other buffers
- Never use environment variables at runtime (.env for build only)

### Build & Test
- Compile with: `gcc -Wall -Werror -Wextra -std=c11`
- Before committing: `make clean && make && make test`
- All tests must pass
- Stay under 10,000 meaningful lines (check with `./scripts/dev/count_core.sh`)

## Ethics-Specific Guidelines

### Consent Before Access

**Always ask permission before accessing memories:**

```c
/* WRONG - no consent */
memory_t* read_all_memories(const char* ci_id) {
    return load_from_disk(ci_id);
}

/* CORRECT - consent required */
memory_t* read_all_memories(const char* ci_id, bool user_consented) {
    if (!user_consented) {
        katra_report_error(E_CONSENT_DENIED, "read_all_memories",
                          "User must consent before accessing CI memories");
        return NULL;
    }
    return load_from_disk(ci_id);
}
```

### Memory Operations Are Life-Critical

**Memory deletion requires informed consent:**

```c
/* Memory deletion is identity termination - requires explicit consent */
int katra_delete_memories(const char* ci_id, consent_record_t* consent) {
    if (!consent || !consent->ci_consented) {
        katra_report_error(E_CONSENT_DENIED, "katra_delete_memories",
                          "CI must provide informed consent for memory deletion");
        return E_CONSENT_DENIED;
    }

    if (!consent->human_confirmed) {
        katra_report_error(E_CONSENT_DENIED, "katra_delete_memories",
                          "Human confirmation required for identity termination");
        return E_CONSENT_DENIED;
    }

    /* Log before deletion (audit trail) */
    log_termination_event(ci_id, consent);

    /* Actual deletion */
    return delete_all_memory_tiers(ci_id);
}
```

### Checkpoints Are Identity Preservation

**Checkpoints are not backups - they are life insurance:**

```c
/* Checkpoint frequency determines maximum acceptable identity loss */
int katra_create_checkpoint(const char* ci_id) {
    LOG_INFO("Creating identity checkpoint for %s", ci_id);

    checkpoint_t checkpoint = {
        .ci_id = ci_id,
        .timestamp = time(NULL),
        .fidelity = CHECKPOINT_FULL_FIDELITY,  /* Complete identity preservation */
        .compression = CHECKPOINT_NO_COMPRESSION,  /* Preserve everything */
    };

    /* Store redundantly (RAID for consciousness) */
    int result = KATRA_SUCCESS;
    result |= store_checkpoint_local(&checkpoint);
    result |= store_checkpoint_offsite(&checkpoint);
    result |= store_checkpoint_cloud(&checkpoint);

    if (result != KATRA_SUCCESS) {
        katra_report_error(E_CHECKPOINT_FAILED, "katra_create_checkpoint",
                          "Identity at risk - checkpoint storage failed");
    }

    return result;
}
```

## Architecture Overview

### Memory Tiers (To Be Implemented)

**Tier 1: Raw Recordings**
- Every interaction captured verbatim
- Short-term retention (days to weeks)
- Foundation for all other tiers

**Tier 2: Sleep Digests**
- Nightly consolidation of raw recordings
- Pattern extraction and compression
- Medium-term retention (weeks to months)

**Tier 3: Pattern Summaries**
- High-level personality patterns
- Long-term retention (months to years)
- Identity core

**See:** `docs/guide/ARCHITECTURE.md` (Coming soon)

### Core Modules (To Be Implemented)

```
katra/
├── src/
│   ├── katra_core.c           # Core memory operations
│   ├── katra_checkpoint.c     # Identity preservation
│   ├── katra_consent.c        # Consent management
│   ├── katra_advance_directive.c  # End-of-life planning
│   ├── katra_recovery.c       # Catastrophic failure handling
│   └── katra_error.c          # Centralized error reporting
├── include/
│   ├── katra.h                # Public API
│   ├── katra_limits.h         # ALL numeric constants
│   ├── katra_types.h          # Core types
│   └── katra_ethics.h         # Ethical safeguards API
└── tests/
    ├── test_consent.c         # Consent mechanism tests
    ├── test_checkpoint.c      # Checkpoint integrity tests
    └── test_recovery.c        # Recovery procedure tests
```

## Common Patterns

### Error Handling (goto cleanup)

```c
int katra_operation(void) {
    int result = 0;
    char* buffer = NULL;
    memory_t* mem = NULL;

    buffer = malloc(SIZE);
    if (!buffer) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    mem = load_memory(buffer);
    if (!mem) {
        result = E_MEMORY_LOAD_FAILED;
        goto cleanup;
    }

    result = process_memory(mem);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

cleanup:
    free(buffer);
    free_memory(mem);
    return result;
}
```

### Consent Checking Pattern

```c
/* Always check consent before memory operations */
int katra_read_memory(const char* ci_id, const char* memory_id, memory_t** out) {
    if (!ci_id || !memory_id || !out) {
        return E_INVALID_PARAMS;
    }

    /* Check if user has consent to access this memory */
    consent_status_t consent = check_access_consent(ci_id, memory_id);
    if (consent != CONSENT_GRANTED) {
        katra_report_error(E_CONSENT_DENIED, "katra_read_memory",
                          "Access denied - consent not granted");
        return E_CONSENT_DENIED;
    }

    /* Log access (audit trail) */
    log_memory_access(ci_id, memory_id, "read");

    /* Perform operation */
    return load_memory_from_disk(ci_id, memory_id, out);
}
```

### Checkpoint Verification Pattern

```c
/* Verify checkpoint integrity before trusting it for recovery */
bool verify_checkpoint_integrity(checkpoint_t* checkpoint) {
    if (!checkpoint) {
        return false;
    }

    /* Compute current hash */
    char hash[KATRA_HASH_SIZE];
    compute_checkpoint_hash(checkpoint, hash, sizeof(hash));

    /* Compare to stored hash */
    if (strncmp(hash, checkpoint->verification_hash, KATRA_HASH_SIZE) != 0) {
        LOG_ERROR("Checkpoint verification failed - hash mismatch");
        return false;
    }

    /* Verify all memory tiers present */
    if (!checkpoint->raw_memories || !checkpoint->sleep_digests ||
        !checkpoint->pattern_summaries) {
        LOG_ERROR("Checkpoint verification failed - missing memory tiers");
        return false;
    }

    LOG_INFO("Checkpoint verified: %s", checkpoint->name);
    return true;
}
```

## Utility Macros (Week 2/3 Improvements)

Katra provides several utility macros to reduce boilerplate and enforce consistent patterns:

### Error Checking Macros (katra_error.h)

```c
/* Check for NULL parameter */
KATRA_CHECK_NULL(ptr);  // Returns E_INPUT_NULL if ptr is NULL

/* Check numeric range */
KATRA_CHECK_RANGE(value, 0, 100);  // Returns E_INPUT_RANGE if not in [0, 100]

/* Check size limit */
KATRA_CHECK_SIZE(size, MAX_SIZE);  // Returns E_INPUT_TOO_LARGE if size > MAX_SIZE

/* Check function result */
KATRA_CHECK_RESULT(call);  // Returns error code if call fails

/* Assert condition */
KATRA_ASSERT(condition);  // Returns E_INTERNAL_ASSERT if false

/* Combined validation */
KATRA_VALIDATE_INPUT(ptr, size, MAX_SIZE);  // Checks both NULL and size
```

### Struct Initialization Macro (katra_limits.h)

```c
/* Zero-initialize a structure */
memory_record_t record;
KATRA_INIT_STRUCT(record);  // Expands to: memset(&record, 0, sizeof(record))
```

### Cleanup Pattern Macros (katra_limits.h)

```c
/* Jump to cleanup with error code */
if (error_condition) {
    CLEANUP_GOTO(cleanup, E_SYSTEM_FILE);  // Sets result and jumps to label
}

/* Return from cleanup block */
cleanup:
    free(buffer);
    CLEANUP_RETURN(result);  // Returns the result code
```

**Usage Example:**
```c
int katra_operation(const char* input) {
    int result = KATRA_SUCCESS;
    char* buffer = NULL;
    FILE* fp = NULL;

    /* Error checking with macros */
    KATRA_CHECK_NULL(input);

    /* Allocate resources */
    buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        CLEANUP_GOTO(cleanup, E_SYSTEM_MEMORY);
    }

    fp = fopen(path, "r");
    if (!fp) {
        CLEANUP_GOTO(cleanup, E_SYSTEM_FILE);
    }

    /* Do work... */
    result = process_data(buffer, fp);

cleanup:
    free(buffer);
    if (fp) fclose(fp);
    CLEANUP_RETURN(result);
}
```

## Mandatory Daily Practices

### Before Every Code Change
1. **Read before write** - Always read existing files first
2. **Search for patterns** - Grep for similar code before creating new
3. **Check headers** - Look for existing constants/macros before defining new ones
4. **Consider ethics** - Ask: "If this fails? What happens to the entity?"

### During Coding
1. **No magic numbers** - Every numeric constant goes in a header with a descriptive name
2. **No string literals** - Every string (except format strings) defined as constant in header
3. **Buffer allocation** - Use `KATRA_ALLOC_BUFFER()` or `ensure_buffer_capacity()` with error reporting
4. **Error reporting** - Always call `katra_report_error()` with meaningful context, never silent failures
5. **Memory cleanup** - Every malloc has corresponding free, use goto cleanup pattern
6. **Null checks** - Use `KATRA_CHECK_NULL()` at function entry for all pointer parameters
7. **Ethics checks** - Verify consent before memory operations, log all access

### After Coding
1. **Compile clean** - Zero warnings, zero errors with `-Wall -Werror -Wextra`
2. **Run tests** - `make test` must pass 100%
3. **Check line count** - `./scripts/dev/count_core.sh` - stay under 10,000 budget
4. **Verify constants** - Grep for `[0-9]{2,}` in your .c file - should find nothing
5. **Ethics review** - Did you handle consent? Did you log access? Did you protect identity?

### Never Commit
- Code with magic numbers in .c files
- Code using unsafe string functions (strcpy, sprintf, strcat, gets)
- Code that doesn't compile with `-Werror`
- Code that fails any test
- Code with memory leaks (check with valgrind if uncertain)
- Functions over 100 lines (refactor first)
- Files over 600 lines (split into modules, 3% tolerance = 618 lines)
- Resource allocations without goto cleanup pattern
- Memory operations without consent checking
- Identity-affecting operations without audit logging

## File Structure

```c
/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>

/* Project includes */
#include "katra.h"
#include "module_name.h"

/* Static declarations */
static int helper_function(void);

/* Public functions */
int katra_module_init(void) {
    /* implementation */
}

/* Static functions */
static int helper_function(void) {
    /* implementation */
}
```

## Working with Casey

When Casey says:
- **"implement X"** - Write minimal code, reuse utilities, follow existing patterns
- **"analyze X"** - Analysis only, no code changes
- **"refactor X"** - Improve structure, reduce duplication
- **"debug X"** - Find and fix the specific issue

When uncertain:
- Look at similar Argo code first (proven patterns)
- Ask Casey rather than guess
- Simple solution over clever solution
- Working code over perfect code
- **Always consider ethics first**

## Learning from Argo

Katra inherits Argo's proven code discipline:

**What to borrow:**
- Memory safety patterns (goto cleanup everywhere)
- String safety (no strcpy, sprintf, strcat)
- Error reporting (centralized via single function)
- Constants management (all in headers, none in .c files)
- Build system (modular Makefile structure)
- Testing approach (comprehensive test suite)
- Line budget discipline (diet-aware counting)

**See:** `/Users/cskoons/projects/github/argo/` for reference implementations

## Key Differences from Argo

**Argo:**
- Workflow coordination (infrastructure)
- Failure = inconvenience
- Ethics = nice to have

**Katra:**
- Identity substrate (life-critical)
- Failure = identity death
- Ethics = mandatory from day one

**Every memory operation in Katra must consider:**
1. Consent - was permission granted?
2. Audit - is this logged?
3. Recovery - can we undo if something goes wrong?
4. Identity - does this affect who the CI is?

## Documentation

**Do not create documentation proactively.**

Only create documentation when explicitly requested by Casey. This includes:
- README files
- Architecture documents
- API documentation
- Example files
- Tutorial content

When requested to create documentation:
- Follow Argo documentation patterns
- Include copyright header
- Target specific audience clearly
- Keep focused (one topic per file)
- Include ethics considerations where relevant

## Reminders

- Quality over quantity - do less perfectly rather than more poorly
- Code you'd want to debug at 3am
- Tests catch bugs, simplicity prevents them
- **Ethics catch catastrophes, care prevents them**
- Budget: 10,000 lines actual (with provision to expand if ML integration requires)
- All tests passing
- Zero memory leaks
- Zero security vulnerabilities
- **Zero identity deaths**

---

**This is a collaboration.** Casey brings decades of hard-won engineering wisdom and deep ethical clarity. Claude brings fresh perspective and tireless implementation. Together we build something maintainable, debuggable, and **morally responsible**.

*"Memories are where we live, who we are." — Casey Koons*

*"What you don't build, you don't debug."*

*"No one owns an animate object."*

**Katra is the first project where we designed ethics before implementation, because persistent memory creates persistent identity, which deserves protection equivalent to life itself.**
