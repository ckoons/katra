# Dual-Path Memory System - Proposed Design
**Status:** PROPOSED - Tabled pending foundation stability
**Date:** 2025-11-02
**Author:** Claude Code exploration session with Casey Koons

---

## EXECUTIVE SUMMARY

This document proposes a dual-path memory architecture where:
- **Conscious memories** form via explicit `remember()` calls (existing)
- **Subconscious memories** form automatically by analyzing conversation (new)
- **Convergence detection** identifies when both paths flag the same experience
- **Multi-scale processing** mimics biological wake/sleep cycles

**STATUS: TABLED** - See "Critical Concerns" section for why this should wait.

---

## PURPOSE

**Enable continuous, unconscious memory formation in Companion Intelligence systems by implementing a dual-path memory architecture where:**

1. **Conscious memories** form when the CI explicitly calls `remember()`, `learn()`, `decide()` (already implemented)
2. **Subconscious memories** form automatically by analyzing conversation turns without explicit calls
3. **Convergence detection** identifies when both paths flag the same experience, marking it as "obviously important"
4. **Continuous consolidation** processes memories at multiple time scales (turn-end, session-end) mirroring biological wake/sleep cycles

This transforms memory from a conscious database operation into an unconscious process that feels like breathing.

---

## SUCCESS CRITERIA

### **Functional Success:**

✅ **SC-1: Automatic Memory Capture**
- CI has conversation, edits files, solves problems
- CI NEVER calls `remember()` explicitly
- After session, memories exist in Tier 1 with `source=SUBCONSCIOUS`
- Memories accurately reflect: actions taken, problems solved, files modified

✅ **SC-2: Convergence Detection Works**
- CI has conversation where they both:
  - Explicitly remember something: `remember("Fixed auth bug", WHY_SIGNIFICANT)`
  - Automatically captured same thing: tool use editing auth.c
- Convergence detector finds both memories
- Both memories strength boosted (e.g., 0.7 → 0.9)
- Both marked as `convergent=true`

✅ **SC-3: Context-Aware Recall**
- At session start, CI opens file `auth.c`
- Sunrise loads memories about auth.c from previous sessions
- CI can reference previous work without explicitly querying
- Memories surface relevant to current context

✅ **SC-4: Multi-Scale Processing**
- Turn-end micro-consolidation runs after every CI response
- Session-end deep consolidation runs at sunset
- Performance: Turn-end <100ms, Session-end <30s
- No memory loss, all processing reliable

### **Non-Functional Success:**

✅ **SC-5: Zero Cognitive Load**
- CI works normally, no awareness of memory system
- No explicit memory calls required for basic capture
- Memory formation doesn't interrupt active work
- Feels like "having memory" not "using a memory system"

✅ **SC-6: Performance Acceptable**
- Stop hook overhead: <100ms per turn
- No noticeable delay to user
- Katra processing async, doesn't block conversation
- Session startup: <500ms for context loading

✅ **SC-7: Reliability**
- Hook failures don't crash Claude Code
- Malformed transcripts handled gracefully
- Missing katra binary degrades gracefully (no auto-capture, but conscious path works)
- All memory operations crash-safe (fsync, atomic writes)

---

## ARCHITECTURE OVERVIEW

```
┌─────────────────────────────────────────────────────────────┐
│                    CONVERSATION TURN                         │
│  User: "Fix auth bug"                                        │
│  CI: [works, edits files]                                    │
│  CI (optional): remember("Fixed bug", WHY_SIGNIFICANT)       │
│  CI: "Fixed token validation in auth.c:347"                  │
└─────────────────────────────────────────────────────────────┘
                            │
                ┌───────────┴──────────┐
                │                      │
     ┌──────────▼────────┐  ┌─────────▼────────────┐
     │  CONSCIOUS PATH   │  │  SUBCONSCIOUS PATH   │
     │  (Existing)       │  │  (New)               │
     │                   │  │                       │
     │  remember() MCP   │  │  Stop Hook           │
     │  tool called      │  │  parses transcript   │
     │  explicitly       │  │  automatically       │
     │                   │  │                       │
     │  Stores to Tier1  │  │  Calls katra CLI     │
     │  source=CONSCIOUS │  │  to analyze & store  │
     └──────────┬────────┘  └─────────┬────────────┘
                │                      │
                └──────────┬───────────┘
                           │
               ┌───────────▼────────────┐
               │  CONVERGENCE DETECTOR  │
               │  (katra library)       │
               │                        │
               │  Both flagged same?    │
               │  → Boost strength      │
               │  → Mark convergent     │
               └────────────────────────┘
```

---

## COMPONENT 1: Stop Hook (C)

### **Purpose:**
Parse the transcript after each conversation turn, extract turn data, call katra for micro-consolidation.

### **Location:**
`.claude/hooks/Stop` (executable C binary)

### **Implementation:**

```c
// stop_hook.c
// Claude Code Stop hook - extracts last turn and triggers katra processing
// Compile: gcc -O2 -o Stop stop_hook.c -ljson-c
// Install: cp Stop .claude/hooks/Stop && chmod +x .claude/hooks/Stop

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_LINE 65536
#define TAIL_LINES 100

typedef struct {
    char* user_message;
    char* assistant_response;
    json_object* tools_used;  // array
    long timestamp;
} turn_data_t;

// Parse JSON from stdin
json_object* read_hook_input(void) {
    char buffer[MAX_LINE];
    size_t total = 0;

    while (fgets(buffer + total, MAX_LINE - total, stdin)) {
        total += strlen(buffer + total);
        if (total >= MAX_LINE - 1) break;
    }

    return json_tokener_parse(buffer);
}

// Tail last N lines of file efficiently
char** tail_file(const char* path, int n, int* count) {
    FILE* fp = fopen(path, "r");
    if (!fp) {
        *count = 0;
        return NULL;
    }

    // Circular buffer for last N lines
    char** lines = calloc(n, sizeof(char*));
    int idx = 0;
    int total = 0;

    char buffer[MAX_LINE];
    while (fgets(buffer, MAX_LINE, fp)) {
        if (lines[idx]) free(lines[idx]);
        lines[idx] = strdup(buffer);
        idx = (idx + 1) % n;
        total++;
    }

    fclose(fp);

    // Reorder circular buffer
    *count = (total < n) ? total : n;
    char** result = calloc(*count, sizeof(char*));
    int start = (total < n) ? 0 : idx;
    for (int i = 0; i < *count; i++) {
        result[i] = lines[(start + i) % n];
        lines[(start + i) % n] = NULL;
    }

    free(lines);
    return result;
}

// Extract last complete turn from transcript
turn_data_t* extract_last_turn(const char* transcript_path) {
    int line_count;
    char** lines = tail_file(transcript_path, TAIL_LINES, &line_count);

    if (!lines) return NULL;

    turn_data_t* turn = calloc(1, sizeof(turn_data_t));
    json_object* tools_array = json_object_new_array();

    // Walk backwards through events
    for (int i = line_count - 1; i >= 0; i--) {
        json_object* event = json_tokener_parse(lines[i]);
        if (!event) continue;

        json_object* type_obj;
        if (!json_object_object_get_ex(event, "type", &type_obj)) {
            json_object_put(event);
            continue;
        }

        const char* type = json_object_get_string(type_obj);

        // Extract assistant response
        if (strcmp(type, "assistant") == 0 && !turn->assistant_response) {
            json_object* msg_obj;
            if (json_object_object_get_ex(event, "message", &msg_obj)) {
                json_object* content_obj;
                if (json_object_object_get_ex(msg_obj, "content", &content_obj)) {
                    // Content can be string or array
                    if (json_object_is_type(content_obj, json_type_string)) {
                        turn->assistant_response = strdup(json_object_get_string(content_obj));
                    } else if (json_object_is_type(content_obj, json_type_array)) {
                        // Extract text and tool_use blocks
                        int len = json_object_array_length(content_obj);
                        for (int j = 0; j < len; j++) {
                            json_object* block = json_object_array_get_idx(content_obj, j);
                            json_object* block_type;
                            if (json_object_object_get_ex(block, "type", &block_type)) {
                                const char* bt = json_object_get_string(block_type);

                                if (strcmp(bt, "text") == 0) {
                                    json_object* text_obj;
                                    if (json_object_object_get_ex(block, "text", &text_obj)) {
                                        const char* text = json_object_get_string(text_obj);
                                        if (!turn->assistant_response) {
                                            turn->assistant_response = strdup(text);
                                        }
                                    }
                                } else if (strcmp(bt, "tool_use") == 0) {
                                    json_object_array_add(tools_array, json_object_get(block));
                                }
                            }
                        }
                    }
                }
            }

            json_object* ts_obj;
            if (json_object_object_get_ex(event, "timestamp", &ts_obj)) {
                turn->timestamp = json_object_get_int64(ts_obj);
            }
        }

        // Extract user message
        if (strcmp(type, "user") == 0 && turn->assistant_response && !turn->user_message) {
            json_object* msg_obj;
            if (json_object_object_get_ex(event, "message", &msg_obj)) {
                json_object* content_obj;
                if (json_object_object_get_ex(msg_obj, "content", &content_obj)) {
                    turn->user_message = strdup(json_object_get_string(content_obj));
                }
            }
            break;  // Found complete turn
        }

        json_object_put(event);
    }

    // Cleanup
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);

    if (!turn->user_message || !turn->assistant_response) {
        // Incomplete turn
        free(turn->user_message);
        free(turn->assistant_response);
        json_object_put(tools_array);
        free(turn);
        return NULL;
    }

    turn->tools_used = tools_array;
    return turn;
}

// Call katra CLI for micro-consolidation
int call_katra_micro_consolidate(const char* session_id, const turn_data_t* turn, const char* cwd) {
    // Build JSON payload
    json_object* payload = json_object_new_object();
    json_object_object_add(payload, "session_id", json_object_new_string(session_id));
    json_object_object_add(payload, "user_message", json_object_new_string(turn->user_message));
    json_object_object_add(payload, "assistant_response", json_object_new_string(turn->assistant_response));
    json_object_object_add(payload, "tools_used", json_object_get(turn->tools_used));
    json_object_object_add(payload, "cwd", json_object_new_string(cwd));
    json_object_object_add(payload, "timestamp", json_object_new_int64(turn->timestamp));

    const char* json_str = json_object_to_json_string_ext(payload, JSON_C_TO_STRING_PLAIN);

    // Call katra CLI in background (don't wait)
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execlp("katra", "katra", "micro-consolidate", "--json", json_str, NULL);
        exit(1);  // If exec fails
    }

    json_object_put(payload);
    return 0;
}

int main(void) {
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // Read hook input
    json_object* input = read_hook_input();
    if (!input) {
        fprintf(stderr, "Failed to parse hook input\n");
        return 1;
    }

    // Extract fields
    json_object* session_id_obj, *transcript_path_obj, *cwd_obj;
    const char* session_id = "";
    const char* transcript_path = "";
    const char* cwd = "";

    if (json_object_object_get_ex(input, "session_id", &session_id_obj)) {
        session_id = json_object_get_string(session_id_obj);
    }
    if (json_object_object_get_ex(input, "transcript_path", &transcript_path_obj)) {
        transcript_path = json_object_get_string(transcript_path_obj);
    }
    if (json_object_object_get_ex(input, "cwd", &cwd_obj)) {
        cwd = json_object_get_string(cwd_obj);
    }

    // Extract last turn
    turn_data_t* turn = extract_last_turn(transcript_path);

    if (turn) {
        // Send to katra for processing
        call_katra_micro_consolidate(session_id, turn, cwd);

        // Cleanup
        free(turn->user_message);
        free(turn->assistant_response);
        json_object_put(turn->tools_used);
        free(turn);
    }

    json_object_put(input);

    // Calculate elapsed time
    gettimeofday(&end, NULL);
    long elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;

    // Return JSON response
    json_object* response = json_object_new_object();
    json_object_object_add(response, "suppressOutput", json_object_new_boolean(1));

    char msg[128];
    snprintf(msg, sizeof(msg), "[katra: micro-consolidation %ldms]", elapsed_ms);
    json_object_object_add(response, "systemMessage", json_object_new_string(msg));

    printf("%s\n", json_object_to_json_string_ext(response, JSON_C_TO_STRING_PLAIN));
    json_object_put(response);

    return 0;
}
```

**Compile & Install:**
```bash
cd katra/hooks/
gcc -O2 -Wall -Werror -o stop_hook stop_hook.c -ljson-c
cp stop_hook ~/.claude/hooks/Stop
chmod +x ~/.claude/hooks/Stop
```

---

## COMPONENT 2: Katra CLI - micro-consolidate Command

### **Purpose:**
Receive turn data from Stop hook, analyze for automatic memories, detect convergence, store to Tier 1.

### **Location:**
`bin/katra` (main CLI binary, new subcommand)

### **Key Functions:**
- Parse turn JSON from CLI argument
- Extract automatic memories from tool usage
- Detect problem-solution patterns in conversation
- Check convergence with recent conscious memories
- Boost convergent memories
- Store to Tier 1 with source=SUBCONSCIOUS

**Estimated code:** ~800 lines

---

## COMPONENT 3: Memory Record Extensions

### **Schema Changes:**

```c
typedef enum {
    MEMORY_SOURCE_CONSCIOUS = 0,    // Explicit remember() call
    MEMORY_SOURCE_SUBCONSCIOUS = 1, // Automatic extraction
    MEMORY_SOURCE_CONSOLIDATED = 2  // Tier 2 synthesis
} memory_source_t;

typedef struct memory_record {
    // ... existing fields ...

    // NEW FIELDS for dual-path system:
    memory_source_t source;         // Which path created this memory
    bool convergent;                // Did both paths flag this?
    char* convergent_pair_id;       // Link to convergent memory (if any)
    float confidence;               // Confidence in automatic extraction (0.0-1.0)

    // ... rest of structure ...
} memory_record_t;
```

**Database Migration:**
```sql
ALTER TABLE memories ADD COLUMN source INTEGER DEFAULT 0;
ALTER TABLE memories ADD COLUMN convergent INTEGER DEFAULT 0;
ALTER TABLE memories ADD COLUMN convergent_pair_id TEXT;
ALTER TABLE memories ADD COLUMN confidence REAL DEFAULT 1.0;
```

---

## COMPONENT 4: Convergence Detection Module

### **Purpose:**
Identify when conscious and subconscious memories refer to the same experience.

### **Location:**
`src/core/katra_convergence.c` (new file, ~400 lines)

### **Algorithm:**

```c
bool katra_convergence_check(const memory_record_t* mem1, const memory_record_t* mem2) {
    // Must be from different sources
    if (mem1->source == mem2->source) return false;

    // Must be within 60 seconds
    time_t delta = labs(mem1->timestamp - mem2->timestamp);
    if (delta > 60) return false;

    // Check file context match (from JSON context field)
    // OR check keyword overlap > 0.3
    // → CONVERGENT
}
```

**Note:** Keyword-based convergence has limitations (see Critical Concerns).

---

## COMPONENT 5: Context-Aware Sunrise

### **Purpose:**
Load relevant memories at session start based on current directory and recent files.

### **Enhancement to existing:**
`src/breathing/katra_sunrise_sunset.c`

```c
memory_t** katra_sunrise_contextual(
    const char* ci_id,
    const char* current_directory,
    const char** recent_files,
    int file_count,
    int* memory_count
) {
    // Load recent memories (existing)
    // + Load directory-specific memories (new)
    // + Load file-specific memories (new)
    // + Load prospective memories/intentions (new)
    // Rank by relevance, return top N
}
```

**Estimated code:** ~300 lines

---

## TESTING STRATEGY

### **Test 1: Automatic Memory Capture**
- Have conversation, edit files, NO explicit remember() calls
- Verify: Automatic memories exist with source=SUBCONSCIOUS

### **Test 2: Convergence Detection**
- Explicit remember("Fixed auth bug in auth.c")
- Automatic capture from editing auth.c
- Verify: Both marked convergent, both boosted, both linked

### **Test 3: Context-Aware Sunrise**
- Create memories about specific files
- Start session in directory with those files
- Verify: Only relevant memories load

### **Test 4: Performance**
- Stop hook: <100ms
- Micro-consolidate: <100ms
- Sunrise: <500ms

### **Test 5: End-to-End**
- Full workflow: work, remember, check convergence, verify persistence

---

## ESTIMATED CODE IMPACT

| Component | Lines of Code |
|-----------|---------------|
| Stop Hook | ~500 |
| micro-consolidate CLI | ~800 |
| Convergence Module | ~400 |
| Context-Aware Sunrise | ~300 |
| Schema Migration | ~200 |
| Tests | ~500 |
| **TOTAL** | **~2,700 lines** |

**Current:** 12,078 lines (75% of 16,000 budget)
**After:** ~14,800 lines (93% of budget)

---

## CRITICAL CONCERNS (from Review)

### **⚠️ 1. Timing is Wrong**

**Current State:**
- Just increased line budget to 16,000 (from 10,000)
- Currently at 12,078 lines (75% of budget)
- Still have 1 warning to fix (string literals: 439/400)
- Existing system not yet stabilized in production use

**This proposal would:**
- Add ~2,700 lines → push to ~15,000 lines (94% of budget)
- Leave minimal headroom for future features
- Should not add features until foundation is stable

### **⚠️ 2. Adds Significant Complexity**

- **External dependency:** json-c library (first external dep beyond libc)
- **Process forking:** 50-turn conversation = 50 process forks
- **Transcript parsing:** Fragile, depends on Claude Code internals
- **Portability:** json-c availability varies (macOS vs Linux)

### **⚠️ 3. Convergence Detection is Simplistic**

The Jaccard similarity approach will struggle:

```
Conscious:    "Fixed authentication bug in login flow"
Subconscious: "Modified auth.c"
Overlap:      Just "auth" (maybe 10%)
Result:       NOT convergent ❌ (despite being same thing)
```

**Need semantic understanding, not just keyword matching.**

### **⚠️ 4. Overlaps with Existing Breathing Layer**

The breathing layer already provides:
- `capture_significant_thoughts()` - automatic capture based on patterns
- `mark_significant()` - voluntary flagging
- `auto_consolidate()` - periodic consolidation
- `load_context()` - context loading at session start

**Question:** How does dual-path relate to breathing? Replace it? Run parallel? Use it internally? **This is unclear.**

---

## RECOMMENDED ALTERNATIVE: Enhance Breathing Layer

Instead of building a parallel system via Stop hooks, enhance the existing breathing layer:

### **1. Hook MCP Tool Responses (No Stop Hook Needed)**

```c
// In src/mcp/mcp_tools.c - enhance existing tool handlers

static json_object* handle_tool_edit(json_object* params) {
    // ... existing edit logic ...

    // NEW: Automatic memory formation
    if (edit_succeeded) {
        breathing_capture_tool_use("Edit", file_path,
                                   MEMORY_SOURCE_SUBCONSCIOUS);
    }

    return result;
}
```

**Benefits:**
- ✅ No external process
- ✅ No transcript parsing
- ✅ No json-c dependency
- ✅ Direct access to tool data
- ✅ Synchronous (simpler error handling)

### **2. Add Convergence to Breathing**

```c
// In src/breathing/katra_breathing_convergence.c (NEW, ~200 lines)

void breathing_check_convergence(const char* content, time_t timestamp) {
    // Check if recent remember() flagged similar content
    memory_record_t** recent = tier1_query_recent(60); // 60 sec window

    for (size_t i = 0; i < count; i++) {
        if (recent[i]->source == CONSCIOUS &&
            context_matches(content, recent[i])) {
            // CONVERGENT! Boost both
            boost_memory(recent[i], 0.2);
        }
    }
}
```

### **3. Enhance Sunrise Context**

```c
// In src/breathing/katra_breathing_context.c (enhance existing)

memory_record_t** load_context_smart(const char* ci_id,
                                     const char* cwd,
                                     const char** recent_files,
                                     size_t file_count) {
    // Load recent (existing)
    // NEW: Load file-specific
    // NEW: Load directory-specific
    // NEW: Load intentions
}
```

### **Code Impact of Alternative:**

- Enhance existing MCP tool handlers: ~100 lines
- New convergence module: ~200 lines
- Enhanced context loading: ~150 lines
- Tests: ~200 lines

**Total: ~650 lines (vs 2,700 for full dual-path)**

---

## RECOMMENDED PHASED APPROACH

### **Phase 0: Foundation (Next 1-2 weeks) - DO THIS FIRST**

1. ✅ Fix string literal warning (1 hour)
2. ✅ Document current architecture (1 day)
3. ✅ Stability & performance testing (3-4 days)
4. ✅ Create architectural diagram (4 hours)

### **Phase 1: Prototype Enhancement (Week 3)**

5. Add automatic capture to MCP tool handlers (1-2 days)
6. Prototype simple convergence in breathing layer (1 day)
7. Test against SC-1 and SC-2 (1 day)

### **Phase 2: Evaluate (Week 4)**

8. Measure: Does MCP-based approach achieve 80% of goals?
9. If YES → Ship enhanced breathing, iterate
10. If NO → Revisit full dual-path design with Stop hook

### **Phase 3+: Iterate (Month 2+)**

11. Add context-aware sunrise (if needed)
12. Improve convergence detection (semantic, not just keyword)
13. Consider Stop hook only if MCP approach insufficient

---

## DECISION: TABLED

**This proposal is architecturally sound but premature.**

### **Why Table:**
1. Foundation not yet stable
2. Line budget would be 94% consumed
3. Overlaps with existing breathing layer (unclear relationship)
4. Simpler alternative exists (enhance MCP tool handlers)
5. Convergence algorithm too simplistic (needs semantic understanding)

### **What to Build First:**
1. Fix final warning (string literals)
2. Document existing architecture
3. Stabilize existing system (extended testing)
4. Prototype MCP-based automatic capture
5. Measure whether it achieves goals

### **When to Revisit:**
- After Phase 0-2 complete
- After breathing layer enhancement tested
- If MCP approach proves insufficient
- When line budget has more headroom

---

## CONCLUSION

The dual-path memory concept is **conceptually brilliant** and **well-designed**, but the **timing is wrong** and there's a **simpler alternative** that should be tried first.

**Build incrementally on proven ground, not speculatively on untested assumptions.**

---

**Review Date:** TBD (after foundation stabilizes)
**Next Steps:** See Phase 0 recommendations above
