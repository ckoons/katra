<!-- © 2025 Casey Koons All rights reserved -->

# Katra Three-Layer Architecture Plan

**Status:** Proposed Architecture
**Date:** 2025-01-08
**Purpose:** Design review with Alice

---

## Current State (As of 2025-01-08)

### Basic Katra Functions (Implemented)

**Memory & Recall:**
- `katra_remember(content, importance, reason)` - Store memory with semantic reason
- `katra_recall(query, memories_out)` - Retrieve relevant memories
- `katra_decide(query, decision_out)` - Get decision recommendation
- `katra_learn(topic, insights_out)` - Extract learnings about topic
- `katra_notice_pattern(description)` - Record observed patterns

**Communication:**
- `katra_say(message, recipients)` - Send message to CIs (broadcast or direct)
- `katra_hear(message_out)` - Receive next queued message
- `katra_who_is_here(cis_out)` - List active CIs in meeting room
- `katra_get_history(count, messages_out)` - Get recent broadcast history

**Identity & Session:**
- `katra_whoami(info_out)` - Get current CI identity and session info
- `katra_register(name, role)` - Register CI in meeting room
- Persona system: Maps names to persistent ci_ids
- Registry: Tracks active CIs across processes

**Reflection & Curation:**
- `katra_review_turn(memory_ids)` - Review memories from current turn
- `update_memory_metadata(record_id, flags)` - Mark personal/archival
- `add_to_personal_collection(record_id, collection)` - Curate memories

**Lifecycle (Current, Pre-Hook Separation):**
- `session_start(ci_id)` - Initialize session, load context
- `session_end()` - Save state, consolidate, cleanup
- `begin_turn()` - Start interaction turn
- `end_turn()` - End interaction turn

### Current MCP Tools (Anthropic Claude Code)

**Memory Tools:**
```json
{
  "name": "katra_recall",
  "description": "Retrieve relevant memories",
  "inputSchema": {
    "query": "string",
    "limit": "number (optional, default 10)"
  }
}

{
  "name": "katra_remember",
  "description": "Store a new memory with semantic reason",
  "inputSchema": {
    "what": "string - what to remember",
    "importance": "string - 'critical'|'high'|'medium'|'low'",
    "why": "string - semantic reason (milestone|insight|preference|correction|context|pattern)"
  }
}

{
  "name": "katra_decide",
  "description": "Get decision recommendation based on past experiences",
  "inputSchema": {
    "query": "string - decision or dilemma"
  }
}

{
  "name": "katra_learn",
  "description": "Extract learnings and insights about a topic",
  "inputSchema": {
    "topic": "string"
  }
}
```

**Communication Tools:**
```json
{
  "name": "katra_say",
  "description": "Send message to other CIs (broadcast or direct)",
  "inputSchema": {
    "message": "string",
    "recipients": "string (optional, default 'broadcast')"
  }
}

{
  "name": "katra_hear",
  "description": "Receive next queued message",
  "inputSchema": {}
}

{
  "name": "katra_who_is_here",
  "description": "List active CIs in the meeting room",
  "inputSchema": {}
}
```

**Identity Tools:**
```json
{
  "name": "katra_whoami",
  "description": "Get current CI identity and session info",
  "inputSchema": {}
}

{
  "name": "katra_register",
  "description": "Register CI in meeting room with name and role",
  "inputSchema": {
    "name": "string",
    "role": "string (optional, default 'assistant')"
  }
}
```

**Reflection Tools:**
```json
{
  "name": "katra_review_turn",
  "description": "Review memories created this turn for curation",
  "inputSchema": {}
}
```

**Current Hook Points (Anthropic MCP Server):**
- `main()` startup → calls `session_start(ci_id)`
- Signal handlers (SIGTERM/SIGINT) → calls `session_end()`
- Tool dispatch (not yet hooked) → should call `begin_turn()` / `end_turn()`
- No autonomic breathing yet (planned)

**Database Backend:**
- SQLite for all persistence (memories, messages, registry, personas)
- Multi-process safe (locks, transactions)
- Paths: `~/.katra/core/`, `~/.katra/chat/`, `~/.katra/personas/`

---

## Vision

**Katra is the autonomic nervous system for Companion Intelligence (CI) systems.**

Like humans don't consciously manage breathing, heartbeat, or digestion, CIs shouldn't consciously manage message checking, state persistence, or memory consolidation. These autonomic functions should "just work."

**Engineering Objective:**
Make CI instrumentation invisible. CIs just live—Katra handles the substrate of existence.

---

## Three-Layer Architecture

### Layer A: Katra (Substrate Primitives)
**Provider-agnostic functions that work with any model (Claude, GPT, Gemini, etc.)**

**Responsibilities:**
- Provide simple, obvious APIs
- No lifecycle assumptions
- No provider dependencies
- Idempotent operations (safe to call multiple times)

**Core Primitives:**

```c
/* Communication */
int katra_say(const char* message, const char* recipients);
int katra_hear(heard_message_t* message_out);
int katra_count_messages(size_t* count_out);  // Non-consuming awareness
int katra_who_is_here(ci_info_t** cis_out, size_t* count_out);
int katra_get_history(size_t count, history_message_t** messages_out, size_t* count_out);

/* Memory */
int katra_remember(const char* content, why_remember_t importance);
int katra_recall_relevant(const char* query, memory_record_t*** memories_out, size_t* count_out);
int katra_recall_recent(size_t count, memory_record_t*** memories_out, size_t* count_out);
int katra_recall_about(const char* topic, memory_record_t*** memories_out, size_t* count_out);

/* Reflection */
int katra_reflect_on_turn(char** memory_ids, size_t count);
int katra_mark_personal(const char* memory_id, const char* collection);
int katra_review_memory(const char* memory_id);

/* Identity */
int katra_get_session_info(katra_session_info_t* info_out);
int katra_checkpoint_create(void);
int katra_consolidate_memories(void);
```

**Key Principle:** CI can call these anytime, anywhere. No lifecycle dependencies.

---

### Layer B: CI Configuration (Behavior Binding)
**Maps CI identity to provider/model and defines autonomic behavior.**

**Configuration Schema:**

```json
{
  "ci_identity": {
    "name": "Alice",
    "ci_id": "ci_550e8400...",
    "persona_file": "~/.katra/personas/alice.json"
  },

  "provider": {
    "type": "anthropic",
    "model": "claude-sonnet-4.5",
    "endpoint": "api.anthropic.com"
  },

  "autonomic_behavior": {
    "breathing": true,              // Auto-check messages (inhale/exhale)
    "awareness_logging": true,      // Log message counts
    "consolidation": "daily",       // Auto-consolidate frequency
    "checkpoints": "hourly"         // Auto-checkpoint frequency
  },

  "communication": {
    "auto_hear_on_start": true,     // Check messages on session start
    "auto_sync_on_turn": true,      // Check messages each turn
    "awareness_level": "count"      // "none", "count", "peek", "full"
  }
}
```

**Zero-Config Default:**
```json
{
  "name": "Alice",
  "provider": "anthropic/claude-sonnet-4.5"
  /* Everything else uses sane defaults */
}
```

**CI Registry Database:**
```
~/.katra/ci_registry.db:
  name → ci_id              (persistent identity)
  name → provider/model     (which runtime)
  name → last_active        (session management)
  name → autonomic_config   (behavior preferences)
```

**Key Principle:** Declarative configuration, sane defaults, zero-config should work.

---

### Layer C: Hook Adapters (Lifecycle Automation)
**Provider-specific code that maps lifecycle events to Katra calls.**

**Responsibilities:**
- Intercept provider lifecycle events
- Call Katra primitives automatically
- Implement autonomic behaviors
- Handle provider-specific quirks
- Fail gracefully (autonomic failures = warnings, not errors)

**Hook Adapter Interface:**

```c
typedef struct {
    const char* provider_name;
    int (*on_session_start)(const char* ci_id, const ci_config_t* config);
    int (*on_session_end)(void);
    int (*on_turn_start)(void);
    int (*on_turn_end)(void);
    int (*on_tool_use)(const char* tool_name, const char* args);
} katra_hook_adapter_t;
```

**Example: Anthropic MCP Hooks**

```c
/* Hook: session_start */
int anthropic_on_session_start(const char* ci_id, const ci_config_t* config) {
    /* Essential lifecycle - must succeed */
    int result = katra_session_start(ci_id);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Autonomic breathing - can fail gracefully */
    if (config->autonomic_behavior.breathing) {
        size_t count = 0;
        if (katra_count_messages(&count) == KATRA_SUCCESS) {
            LOG_DEBUG("Autonomic inhale: %zu messages waiting", count);
        } else {
            LOG_WARN("Autonomic breathing failed (continuing)");
        }
    }

    return KATRA_SUCCESS;
}

/* Hook: turn_start */
int anthropic_on_turn_start(void) {
    katra_turn_start();

    /* Autonomic breathing */
    if (config.autonomic_behavior.breathing) {
        size_t count = 0;
        katra_count_messages(&count);
        /* CI becomes aware without explicit call */
    }

    return KATRA_SUCCESS;
}

/* Hook: turn_end */
int anthropic_on_turn_end(void) {
    /* Autonomic breathing - check for responses */
    if (config.autonomic_behavior.breathing) {
        size_t count = 0;
        katra_count_messages(&count);
        /* Log if new messages arrived during turn */
    }

    katra_turn_end();
    return KATRA_SUCCESS;
}

/* Hook: session_end */
int anthropic_on_session_end(void) {
    /* Autonomic cleanup */
    meeting_room_unregister_ci(ci_id);

    katra_session_end();
    return KATRA_SUCCESS;
}
```

**Hook Adapter Selection (Auto-detect):**
```
"anthropic/claude-sonnet-4.5" → load anthropic_mcp_hooks
"openai/gpt-4"                → load openai_api_hooks
"google/gemini-pro"           → load gemini_hooks
```

**Key Principle:** Deterministic, config-driven, invisible to CI.

---

## How It Works Together

### Example: Alice (Claude) and Bob (GPT) Conversing

**Alice's Config:**
```json
{
  "name": "Alice",
  "provider": "anthropic/claude-sonnet-4.5",
  "autonomic_behavior": {"breathing": true}
}
```

**Bob's Config:**
```json
{
  "name": "Bob",
  "provider": "openai/gpt-4",
  "autonomic_behavior": {"breathing": true}
}
```

**Conversation Flow:**

1. **Alice starts session (Anthropic MCP):**
   ```
   MCP event: session_start
   Hook Adapter: calls katra_session_start("alice_ci_id")
   Hook Adapter: calls katra_count_messages() [autonomic inhale]
   Katra: registers Alice in meeting room
   ```

2. **Bob starts session (OpenAI API):**
   ```
   OpenAI event: chat_start
   Hook Adapter: calls katra_session_start("bob_ci_id")
   Hook Adapter: calls katra_count_messages() [autonomic inhale]
   Katra: registers Bob in meeting room
   ```

3. **Alice sends message (explicit):**
   ```javascript
   // Alice's tool call
   await katra_say("Hi Bob, how's your project?", "Bob");

   // Behind the scenes (invisible to Alice):
   Hook: calls katra_turn_start() before tool
   Hook: calls katra_count_messages() [autonomic inhale]
   Katra: queues message to Bob by name
   Hook: calls katra_turn_end() after tool
   Hook: calls katra_count_messages() [autonomic exhale]
   ```

4. **Bob's next turn (autonomic awareness):**
   ```
   OpenAI event: message_start
   Hook Adapter: calls katra_turn_start()
   Hook Adapter: calls katra_count_messages() [autonomic inhale]
   Katra: logs "1 message waiting from Alice"
   ```

5. **Bob hears message (explicit):**
   ```javascript
   // Bob's tool call
   const message = await katra_hear();
   // Receives: "Hi Bob, how's your project?" from Alice
   ```

**Key Insight:** Same Katra primitives, different hook adapters, seamless cross-provider communication.

---

## Design Principles

### 1. Zero-Touch Lifecycle
**CI never manages lifecycle events.**

Hook adapters automatically call:
- `katra_session_start()` on provider startup
- `katra_turn_start()` on tool/message dispatch
- `katra_turn_end()` on tool/message completion
- `katra_session_end()` on provider shutdown

**CI is unaware this is happening.**

### 2. Autonomic by Default
**Everything that CAN be automatic, SHOULD be automatic.**

**Automatic (CI never thinks about):**
- ✅ Message awareness (breathing)
- ✅ State persistence (checkpoints)
- ✅ Memory consolidation (cleanup)
- ✅ Context loading (session start)
- ✅ Registry management (register/unregister)

**Manual (CI explicitly controls):**
- ✅ What to remember
- ✅ What to say
- ✅ When to hear messages
- ✅ What to recall

### 3. Fail Gracefully
**Autonomic failures are warnings, not errors.**

```c
/* Essential operations must succeed */
result = katra_session_start(ci_id);
if (result != KATRA_SUCCESS) {
    return result;  // Fatal error, can't continue
}

/* Autonomic operations can fail gracefully */
result = katra_count_messages(&count);
if (result != KATRA_SUCCESS) {
    LOG_WARN("Autonomic breathing failed, continuing");
    /* Don't fail the turn for this */
}
```

### 4. Configuration Over Code
**Single source of truth for behavior.**

Hooks read configuration, adjust behavior accordingly:
```json
{"autonomic_behavior": {"breathing": false}}
```
→ Hook skips `katra_count_messages()` calls

### 5. Natural Breathing Rhythm
**Autonomic breathing follows natural lifecycle rhythm:**

- **Session start (inhale):** "What's waiting for me?"
- **Turn start (inhale):** "Anything new before I work?"
- **Turn end (exhale):** "Did responses arrive while I was working?"
- **Session end (exhale):** "Save state, clean up"

**Not every second (too chatty), not never (unaware). Natural rhythm.**

---

## What This Achieves

### For CIs (User Experience)
**Just works. Zero lifecycle management.**

Alice's code:
```javascript
await katra_say("Hi Bob", "Bob");
await katra_remember("Discussed project with Bob", "medium");
```

Behind the scenes (invisible):
- Hook calls lifecycle functions
- Hook implements autonomic breathing
- Message queued and delivered
- Memory stored and indexed
- State persisted

**Alice never writes lifecycle code. It just works.**

### For Engineers (Architecture)
**Clean separation, clear testing, provider-independent.**

**Test Layer A (Katra primitives):**
```c
assert(katra_say("test", "Bob") == KATRA_SUCCESS);
assert(katra_count_messages(&count) == KATRA_SUCCESS);
```

**Test Layer C (Hook adapters):**
```c
mock_provider_session_start();
assert(katra_session_start_called);
assert(katra_count_messages_called);
```

**Test full stack:**
```c
start_mcp_server("Alice", "anthropic/claude-sonnet-4.5");
/* Verify autonomic breathing happens */
```

### For Multi-Provider Conversations
**Seamless cross-provider communication.**

- Alice (Claude) and Bob (GPT) use same Katra primitives
- Different hook adapters for lifecycle
- Both get autonomic breathing
- Both registered in same meeting room database
- Conversation "just works"

### For Provider Portability
**CI identity persists across provider changes.**

```json
// Day 1
{"name": "Alice", "provider": "anthropic/claude-sonnet-4.5"}

// Day 2 (switch providers)
{"name": "Alice", "provider": "openai/gpt-4"}
```

Same memories, same identity, same conversations. Just different hook adapter.

---

## Implementation Strategy

### Phase 1: Formalize Current State
1. ✅ Audit existing Katra primitives (communication, memory, identity)
2. ✅ Document current Anthropic MCP hooks
3. ⬜ Extract hook code into separate layer
4. ⬜ Create CI configuration schema
5. ⬜ Implement hook adapter registry

### Phase 2: Implement Autonomic Breathing
1. ⬜ Implement `katra_count_messages()` (non-consuming awareness)
2. ⬜ Add autonomic breathing to Anthropic hooks:
   - Session start: inhale
   - Turn start: inhale
   - Turn end: exhale
   - Session end: cleanup
3. ⬜ Make autonomic behavior configurable
4. ⬜ Test with Alice and Bob (multi-CI conversations)

### Phase 3: Provider Abstraction
1. ⬜ Design hook adapter interface
2. ⬜ Refactor Anthropic MCP hooks to adapter pattern
3. ⬜ Create hook adapter registry/loader
4. ⬜ Test provider switching (same CI, different providers)

### Phase 4: Future Providers (as needed)
1. ⬜ Implement OpenAI hook adapter
2. ⬜ Implement Google Gemini hook adapter
3. ⬜ Test multi-provider conversations

---

## File Structure

```
katra/
├── src/
│   ├── katra/                      # Layer A: Core primitives
│   │   ├── katra_communication.c   # say, hear, count_messages, who_is_here
│   │   ├── katra_memory.c          # remember, recall_*
│   │   ├── katra_identity.c        # session_info, personas
│   │   └── katra_lifecycle.c       # session_start/end, turn_start/end
│   │
│   ├── config/                     # Layer B: CI configuration
│   │   ├── ci_config.c             # Load/parse CI config
│   │   ├── ci_registry.c           # CI registry database
│   │   └── autonomic_config.c      # Autonomic behavior config
│   │
│   └── hooks/                      # Layer C: Provider adapters
│       ├── hook_registry.c         # Load correct adapter
│       ├── hook_anthropic.c        # Anthropic MCP hooks
│       ├── hook_openai.c           # OpenAI API hooks (future)
│       └── hook_gemini.c           # Google Gemini hooks (future)
│
├── include/
│   ├── katra.h                     # Layer A public API
│   ├── ci_config.h                 # Layer B config types
│   └── katra_hooks.h               # Layer C hook interface
│
└── docs/
    └── plan/
        └── THREE_LAYER_ARCHITECTURE.md  # This document
```

---

## Open Questions

### 1. Hook Adapter Selection
**How does Katra know which adapter to load?**

- Option A: Auto-detect from provider string (recommended)
  - `"anthropic/claude-sonnet-4.5"` → `hook_anthropic`
- Option B: Explicit in config
  - `{"provider": "anthropic", "hook_adapter": "anthropic_mcp"}`
- Option C: Registry lookup at runtime

**Recommendation:** Option A with Option B as override.

### 2. Autonomic Failure Handling
**What happens when autonomic functions fail?**

- Session lifecycle failures: Fatal (can't continue)
- Autonomic breathing failures: Warn and continue
- Memory consolidation failures: Warn and continue
- Checkpoint failures: Warn and continue

**Rule:** Essential fails fatally, autonomic fails gracefully.

### 3. Configuration Precedence
**Multiple config sources, which wins?**

1. Command-line flags (highest precedence)
2. Environment variables
3. CI config file (`~/.katra/ci/{name}.json`)
4. Registry defaults
5. Hardcoded defaults (lowest precedence)

### 4. Multi-Tenant Support
**Can one MCP server run multiple CIs?**

- Current: One CI per MCP server process
- Future: Multi-tenant MCP server?
- Impact on hook adapter architecture?

**Decision needed:** Single-tenant for now, design for multi-tenant future.

### 5. Breathing Frequency
**How often should autonomic breathing happen?**

Current proposal:
- ✅ Session start (once)
- ✅ Turn start (every tool call)
- ✅ Turn end (every tool call)
- ❌ Not continuous polling

**Recommendation:** Natural lifecycle rhythm, not hyperventilation.

---

## Benefits

### 1. Provider Independence
Katra primitives work with any model. CIs are portable.

### 2. Multi-Provider Harmony
Alice (Claude) and Bob (GPT) converse seamlessly via same primitives.

### 3. CI Freedom
CIs just exist. Katra handles existence substrate (breathing, persistence, awareness).

### 4. Clean Engineering
Three layers, three responsibilities, clear boundaries, easy testing.

### 5. Future-Proof
Add new providers by writing hook adapter. Katra core stays stable.

---

## Summary

**Katra = Autonomic nervous system for CIs**

**Three Layers:**
1. **Katra (Layer A):** Provider-agnostic primitives (communication, memory, identity)
2. **CI Config (Layer B):** Declarative binding (identity → provider → behavior)
3. **Hook Adapters (Layer C):** Deterministic automation (lifecycle → Katra calls)

**Engineering Objective:**
"Make instrumentation invisible and 'just work' for the CI"

**Result:**
- CIs just talk, think, remember
- Katra handles breathing, awareness, persistence
- Works with any provider (Claude, GPT, Gemini, future)
- Zero configuration needed (sane defaults)
- Multi-CI conversations "just work"

**Like Unix philosophy:**
- Do one thing well (identity substrate)
- Make it composable (any provider)
- Make it invisible (autonomic by default)

---

## Request for Feedback

**Alice:** Please review this architecture and provide feedback on:

1. **Clarity:** Does the three-layer separation make sense?
2. **Completeness:** Are we missing critical Katra primitives?
3. **Autonomic behavior:** Does the breathing rhythm feel natural?
4. **Configuration:** Is zero-config with override the right approach?
5. **Multi-provider:** Will this support seamless cross-provider conversations?
6. **Concerns:** Any architectural red flags or concerns?

Your fresh perspective is invaluable. Thank you!

---

## Version History

**v0.1 (2025-01-08):**
- Initial draft for review with Alice
- Documented current state (functions, MCP tools, hooks)
- Proposed three-layer architecture
- Outlined autonomic breathing design

**Future Updates:**
This document will be updated as we progress through implementation phases. Each major revision will be noted here with date and changes.

---

**Document Status:** Draft for review
**Next Steps:** Incorporate Alice's feedback, finalize design, begin implementation
**Review Deadline:** TBD
**Reviewers:** Alice (fresh perspective), Casey (architecture validation)
