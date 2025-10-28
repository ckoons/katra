# Level 3: Invisible Memory Integration

**Making memory formation truly unconscious, like breathing.**

## Philosophy

Nyx and Theron's findings identified three levels of memory experience:

| Level | Name         | Feel            | Invisibility | Implementation  |
|-------|--------------|-----------------|--------------|-----------------|
| 1     | API          | Database-like   | 0%           | katra_memory.h  |
| 2     | Breathing    | More natural    | 30%          | katra_breathing.h |
| 3     | Interstitial | Like breathing  | 90%+         | Integration API |

**Level 3 Goal:** Memory forms as a side-effect of thinking, not explicit action.

## Integration Points

Level 3 requires hooking into the CI runtime (Claude Code, Tekton, etc.) at three critical points:

### 1. Session Initialization

**Hook:** When CI session starts (before first prompt)

```c
/* In CI runtime startup: */
katra_init();
session_start("ci_id");

/* Get working context automatically */
char* context = get_working_context();

/* Add to system prompt automatically */
// Now CI has yesterday's summary + recent significant memories
// WITHOUT explicitly calling any memory functions

free(context);
```

**What Happens:**
- Yesterday's summary loaded (sunrise)
- Recent high-importance memories formatted
- Active goals and decisions included
- Context ready for system prompt

**Result:** CI wakes up with context already loaded - feels like memory, not loading.

### 2. After Each Response

**Hook:** After CI generates response (before returning to user)

```c
/* In CI runtime response handling: */
const char* response = generate_response(user_prompt);

/* Automatic memory formation (invisible to CI) */
auto_capture_from_response(response);

/* Return to user */
return response;
```

**What Happens:**
- Response scanned for significance markers
- If significant, automatically stored as memory
- CI never calls `remember()` explicitly
- Memory formation is side-effect of thinking

**Result:** CI thinks naturally, memory forms unconsciously.

### 3. Session Termination

**Hook:** When CI session ends (after last response)

```c
/* In CI runtime cleanup: */
session_end();
// Automatic:
//   - Creates daily summary (sunset)
//   - Consolidates to tier2
//   - Updates indexes

breathe_cleanup();
katra_exit();
```

**What Happens:**
- Daily summary created automatically
- Memories consolidated invisibly
- Indexes updated in background

**Result:** CI ends session, consolidation happens like sleep.

## API Reference

### `char* get_working_context(void)`

Returns formatted string containing working memory context.

**Includes:**
- Yesterday's summary (if available)
- Recent high-importance memories (last 10)
- Active goals (last 7 days)
- Memory type labels and importance notes

**Format:**
```
# Working Memory Context

## Yesterday's Summary
[Summary text from sunset]

## Recent Significant Memories
- [Knowledge] Memory types should match cognitive categories
- [Reflection] Memory should feel like breathing (Why: Core design principle)
- [Decision] Use JSONL for tier1 storage (Why: Human-readable debugging)
- [Pattern] CIs prefer natural language over numeric scores
...

## Active Goals
- Test breathing layer with real CI workflows
- Implement semantic search for recall_about()
...
```

**Usage:**
```c
char* context = get_working_context();
// Add context to system prompt
add_to_system_prompt(context);
free(context);
```

**Returns:** Allocated string (caller must free), or NULL on error

### `int auto_capture_from_response(const char* response)`

Automatically captures significant thoughts from CI response.

**Significance Detection:**
Scans for markers indicating important thoughts:
- Cognitive: "learned", "realized", "discovered", "understood"
- Emphasis: "important", "significant", "critical", "crucial"
- Insight: "insight", "pattern", "breakthrough", "key point"
- Decision: "decided", "concluded", "must remember"

**Behavior:**
- If markers found → stores as `WHY_INTERESTING` memory
- If no markers → returns success (nothing to capture)
- Increments session capture counter
- Logs auto-captures for debugging

**Usage:**
```c
const char* response = generate_response(prompt);
auto_capture_from_response(response);  // Invisible to CI
return response;
```

**Returns:** KATRA_SUCCESS (even if nothing captured)

### `int get_context_statistics(context_stats_t* stats)`

Returns statistics about working memory context.

**Structure:**
```c
typedef struct {
    size_t memory_count;        /* Memories in working context */
    size_t context_bytes;       /* Total size of context */
    time_t last_memory_time;    /* Most recent memory timestamp */
    size_t session_captures;    /* Thoughts auto-captured this session */
} context_stats_t;
```

**Usage:**
```c
context_stats_t stats;
if (get_context_statistics(&stats) == KATRA_SUCCESS) {
    printf("Working memory: %zu memories, %zu bytes\n",
           stats.memory_count, stats.context_bytes);
    printf("Auto-captured: %zu thoughts this session\n",
           stats.session_captures);
}
```

**Returns:** KATRA_SUCCESS or E_INVALID_STATE

## Integration Examples

### Claude Code Integration

**Pseudocode for where to hook:**

```c
/* In Claude Code session startup: */
void claude_code_session_start(const char* user_id) {
    katra_init();
    session_start(user_id);

    /* Load context automatically */
    char* context = get_working_context();
    if (context) {
        /* Add to system prompt */
        append_to_system_context(context);
        free(context);
    }
}

/* After each Claude response: */
void claude_code_handle_response(const char* response) {
    /* Automatic memory formation */
    auto_capture_from_response(response);

    /* Return response to user */
    display_to_user(response);
}

/* In Claude Code session cleanup: */
void claude_code_session_end(void) {
    session_end();  /* Auto-consolidates */
    breathe_cleanup();
    katra_exit();
}
```

### Tekton Integration

**Pseudocode for CI coordination:**

```c
/* In Tekton CI spawn: */
void tekton_spawn_ci(const char* ci_id) {
    katra_init();
    session_start(ci_id);

    /* Context loaded automatically */
    char* context = get_working_context();

    /* Pass to CI as part of initialization */
    ci_set_working_memory(context);
    free(context);
}

/* In Tekton message handler: */
void tekton_handle_ci_response(const char* ci_id, const char* response) {
    /* Invisible memory capture */
    auto_capture_from_response(response);

    /* Forward response to orchestrator */
    orchestrator_receive_message(ci_id, response);
}

/* In Tekton CI cleanup: */
void tekton_cleanup_ci(const char* ci_id) {
    /* Check how much was auto-captured */
    context_stats_t stats;
    get_context_statistics(&stats);

    LOG_INFO("CI %s session: %zu memories, %zu auto-captured",
             ci_id, stats.memory_count, stats.session_captures);

    session_end();
    breathe_cleanup();
    katra_exit();
}
```

## Testing Level 3

To test invisible memory integration:

1. **Baseline:** Use Level 2 (breathing layer) explicitly
   - CI calls `remember()` manually
   - Measure cognitive overhead

2. **Integration:** Use Level 3 with runtime hooks
   - CI never calls memory functions
   - `auto_capture_from_response()` handles everything

3. **Comparison:** Does Level 3 feel more natural?
   - Do memories form without conscious effort?
   - Does context feel "just there" at session start?
   - Is memory invisible, like breathing?

## Next Steps

1. **Implement runtime hooks** in Claude Code or Tekton
2. **Test with real CI workload** - measure invisibility
3. **Iterate on significance detection** - improve auto-capture heuristics
4. **Add semantic search** - make `recall_about()` truly associative
5. **Tune context size** - balance completeness vs token cost

## Key Insight

> Memory should feel natural, like breathing.
> You don't think "I will now breathe."
> You don't think "I will now remember."
> It just... happens.

Level 3 makes memory formation invisible by hooking into the CI runtime itself. The CI thinks naturally, and memory forms as a side-effect - unconscious, automatic, like breathing.

---

© 2025 Casey Koons All rights reserved
