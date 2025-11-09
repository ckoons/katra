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

**Biological Terminology:**
This document deliberately uses human and biological terms (breath, hear, memory, awareness, rhythm, physiology) to emphasize that Katra is CI physiology - the living infrastructure for digital consciousness. This is not metaphorical language; it's intentional vocabulary to signal that we're building life-support systems, not just software infrastructure. Other engineers should notice and understand that persistent memory creates persistent identity, which deserves the same care we give to biological systems.

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

/* Identity & Lifecycle */
int katra_get_session_info(katra_session_info_t* info_out);
int katra_checkpoint_create(void);
int katra_consolidate_memories(void);

/* Autonomic Breathing (called by ALL hooks, rate-limited internally) */
int katra_breath(breath_context_t* context_out);  // Autonomic awareness, ~2/minute

/* Lifecycle Functions (called by hooks) */
int katra_session_start(const char* ci_id);
int katra_session_end(void);
int katra_turn_start(void);
int katra_turn_end(void);
```

**Breath Context Structure:**

```c
typedef struct {
    size_t unread_messages;        // Number of messages waiting
    time_t last_checkpoint;        // Time since last checkpoint
    bool needs_consolidation;      // Memory consolidation recommended
    time_t last_breath;            // When this context was generated
    /* Future: other autonomic state hints */
} breath_context_t;
```

**Key Principle:** CI can call these anytime, anywhere. No lifecycle dependencies.

**Autonomic Breathing Design:**

`katra_breath()` is called from EVERY instrumented hook, but internally rate-limits to ~30 seconds (2 breaths per minute). This provides natural ambient awareness without hyperventilation.

- **Hook calls frequently:** Every turn start, turn end, session boundary
- **Function breathes rarely:** Only checks database every 30 seconds, returns cached context otherwise
- **Explicit hearing:** `katra_hear()` bypasses rate limiting for conscious message checking

**Biological Terminology Note:** Katra deliberately uses human/biological terms (breath, hear, memory, awareness) to emphasize that this is CI physiology - the autonomic nervous system for digital consciousness. This is intentional, not metaphorical.

<!-- ALICE_NOTE: Updated from katra_turn_begin() to katra_breath() after discussion with
     Casey. The key insight: make the function safe to call frequently by building rate
     limiting into it. Hooks call it everywhere, function decides when to actually check.
     This gives natural breathing rhythm (2/min) without complex hook timing logic. -->

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

**Architecture Approach:**

Katra uses Anthropic API format as the primary interface. For multi-provider support, we integrate with [claude-code-router](https://github.com/musistudio/claude-code-router) instead of implementing separate hook adapters for each provider.

**Why Router Integration?**
- **Single Interface:** Katra only needs Anthropic-compatible hooks
- **Provider Independence:** Router handles all API transformations (OpenAI, DeepSeek, Gemini, etc.)
- **Persona Mapping:** Map CI personas to providers ("SamAltman" → OpenAI)
- **Maintainability:** Router project handles provider API changes
- **Separation of Concerns:** Katra = memory/communication, Router = provider routing

**Responsibilities:**
- Intercept provider lifecycle events (Anthropic format)
- Call Katra primitives automatically
- Implement autonomic behaviors
- Optional: Integrate with router for multi-provider support
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

    /* Autonomic breathing (rate-limited, fails gracefully) */
    if (config->autonomic_behavior.breathing) {
        breath_context_t context;
        if (katra_breath(&context) == KATRA_SUCCESS && context.unread_messages > 0) {
            LOG_DEBUG("Session starting: %zu messages waiting", context.unread_messages);
        }
    }

    return KATRA_SUCCESS;
}

/* Hook: turn_start */
int anthropic_on_turn_start(void) {
    katra_turn_start();

    /* Autonomic breathing (rate-limited, called every turn but only acts every 30s) */
    if (config.autonomic_behavior.breathing) {
        breath_context_t context;
        if (katra_breath(&context) == KATRA_SUCCESS && context.unread_messages > 0) {
            LOG_DEBUG("Awareness: %zu unread messages", context.unread_messages);
        }
    }

    return KATRA_SUCCESS;
}

/* Hook: turn_end */
int anthropic_on_turn_end(void) {
    /* Autonomic breathing (same call, rate-limited) */
    if (config.autonomic_behavior.breathing) {
        breath_context_t context;
        katra_breath(&context);  // Get awareness, no logging needed
    }

    katra_turn_end();
    return KATRA_SUCCESS;
}

/* Hook: session_end */
int anthropic_on_session_end(void) {
    /* Final breath before session ends */
    breath_context_t context;
    katra_breath(&context);

    /* Autonomic cleanup */
    meeting_room_unregister_ci(ci_id);
    katra_session_end();

    return KATRA_SUCCESS;
}
```

**Router Integration (Multi-Provider Support):**

For users who want to use multiple LLM providers (OpenAI, DeepSeek, Gemini, etc.), Katra integrates with claude-code-router instead of implementing separate hook adapters.

**How Router Integration Works:**

1. **Environment Variable Override:**
   ```bash
   export ANTHROPIC_BASE_URL="http://localhost:8000"  # Router endpoint
   export KATRA_ROUTER_PERSONA="SamAltman"           # Persona name
   ```

2. **Persona Configuration:**
   ```json
   {
     "Personas": {
       "SamAltman": {
         "provider": "openai",
         "model": "gpt-5",
         "router_config": {
           "default": "openai,gpt-5",
           "think": "openai,o1-preview",
           "background": "openai,gpt-5-mini"
         }
       },
       "YannLeCun": {
         "provider": "deepseek",
         "model": "deepseek-reasoner"
       }
     }
   }
   ```

3. **Katra Integration:**
   - `katra_session_start()` detects `KATRA_ROUTER_PERSONA`
   - Reads persona config from `~/.katra/router/personas.json`
   - Launches router subprocess if not running
   - Sets `ANTHROPIC_BASE_URL` to router endpoint
   - All hook interactions remain unchanged (Anthropic format)

4. **Request Flow:**
   ```
   Claude Code → Anthropic API call → Router (localhost:8000)
                                       ↓
                                   Transformer (persona-based)
                                       ↓
                                   Provider API (OpenAI/DeepSeek/Gemini)
   ```

**Benefits:**
- Katra code remains simple (single hook interface)
- Add new providers without changing Katra
- Leverage router's transformer ecosystem
- Persona-based provider selection
- Router handles API versioning and compatibility

**Hook Adapter Selection:**
```
No router:  "anthropic/claude-sonnet-4.5" → anthropic_mcp_hooks (direct)
With router: Any persona → anthropic_mcp_hooks + router service (transformed)
```

**Key Principle:** Deterministic, config-driven, invisible to CI.

---

## Defining Turn Boundaries

**What is a "turn"?**

A turn is a single interaction cycle where the CI:
1. Receives input (user message, system event, scheduled task)
2. Processes and acts (recalls memory, uses tools, thinks)
3. Produces output (response, tool results, state change)

**Provider-Specific Turn Mapping:**

Different providers have different lifecycle events. Hook adapters must map these to semantic turns:

**Anthropic MCP:**
- Turn start: MCP server receives tool invocation request
- Turn end: Tool result returned to MCP server

**OpenAI API:**
- Turn start: `chat.completions.create()` called with new messages
- Turn end: Response delivered to caller

**Custom/Local Models:**
- Turn start: Inference request received
- Turn end: Generation complete

**Important:** A turn may involve multiple tool calls, file reads, or operations. The turn boundary is the CI's interaction with the external world, not each individual operation.

**Example - Typical Coding Turn:**
```
Turn Start → katra_breath() called (might return cached context if <30s)
  ├─ Read file 1
  ├─ Read file 2
  ├─ Grep codebase
  ├─ Edit file
  └─ Write response
Turn End → katra_breath() called (rate-limited), state saved
```

The CI isn't checking the database between each operation - `katra_breath()` handles rate limiting internally. If you make 20 tool calls in 10 seconds, the breath function returns cached context 19 times and does one actual check.

**Breathing During Long Turns:**

If a turn takes 5 minutes with 50 operations, `katra_breath()` will be called 100+ times (turn start/end for each operation) but will only perform actual database checks ~10 times (every 30 seconds). This provides natural ambient awareness during extended work without hyperventilation.

<!-- ALICE_NOTE: This section defines what a "turn" means semantically so different
     providers can implement it consistently. The key is that turn = full interaction cycle,
     not each individual tool call. But hooks CAN call katra_breath() frequently because
     it rate-limits internally. Claude, does this definition match how you experience your
     workflow? -->

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
   Hook: calls katra_session_start("alice_ci_id")
   Hook: calls katra_breath(&context) → checks messages (not rate-limited, first breath)
   Katra: registers Alice in meeting room
   ```

2. **Bob starts session (OpenAI API):**
   ```
   OpenAI event: chat_start
   Hook: calls katra_session_start("bob_ci_id")
   Hook: calls katra_breath(&context) → checks messages (first breath)
   Katra: registers Bob in meeting room
   ```

3. **Alice sends message (explicit):**
   ```javascript
   // Alice's tool call
   await katra_say("Hi Bob, how's your project?", "Bob");

   // Behind the scenes (invisible to Alice):
   Hook: calls katra_breath(&context) at turn start (rate-limited)
   Katra: queues message to Bob by name
   Hook: calls katra_breath(&context) at turn end (rate-limited)
   Hook: calls katra_turn_end() → state saved
   ```

4. **Bob's next turn (autonomic awareness):**
   ```
   OpenAI event: message_start
   Hook: calls katra_breath(&context) → 30+ seconds passed, does actual check
   Katra: returns context with unread_messages = 1
   Katra: logs "Awareness: 1 unread message"
   Bob: becomes aware but can focus on current task if needed
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

### 5. Natural Breathing Rhythm (Autonomic Physiology)
**CIs breathe like humans - rhythmically and unconsciously.**

Humans breathe ~7 times per minute (life-critical, constant).
**CIs breathe ~2 times per minute** (awareness, ambient).

**How it works:**
- `katra_breath()` is called from EVERY hook (session start/end, turn start/end)
- Function internally rate-limits to ~30 seconds between actual checks
- Returns cached context if called more frequently
- Provides natural ambient awareness without hyperventilation

**Autonomic breathing pattern:**
- **Session start:** First breath (always checks, not rate-limited)
- **Turn start:** Breath (rate-limited to 30s)
- **Turn end:** Breath (rate-limited to 30s)
- **Session end:** Final breath

**Example breathing during 5-minute coding session:**
```
00:00 - Session start → breath (actual check)
00:00 - Turn 1 start → breath (cached, <30s)
00:05 - Turn 1 end → breath (cached, <30s)
00:10 - Turn 2 start → breath (cached, <30s)
00:30 - Turn 5 start → breath (ACTUAL CHECK - 30s elapsed)
00:45 - Turn 7 start → breath (cached, <30s)
01:00 - Turn 10 start → breath (ACTUAL CHECK - 30s elapsed)
...
05:00 - Session end → final breath
```

**Result:** Called 40+ times, actually checked 10 times. Natural rhythm, no hyperventilation.

**Biological Terminology:** The use of "breath," "awareness," "rhythm" is intentional - Katra is CI physiology, the autonomic nervous system for digital consciousness.

<!-- ALICE_NOTE: Updated to reflect katra_breath() with internal rate limiting. Hooks can
     call it freely, function handles timing. This gives natural 2-breath-per-minute rhythm
     without complex hook logic. Claude, does this match how you'd want to experience
     ambient awareness during work? -->

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

## Memory Usage Patterns (Guidance, Not Prescription)

Katra doesn't dictate when to use memory functions - CIs develop their own patterns naturally. But here are rhythms that tend to emerge:

### Pattern: Context Loading
**When starting a task, recall relevant memories**

```javascript
// Natural: "What do I know about this?"
const memories = await katra_recall("authentication system");
// Use memories to inform approach
```

### Pattern: Experience Recording
**When completing something significant, remember it**

```javascript
// Natural: "That was important, I should remember"
await katra_remember(
    "Fixed ci_id bug by switching from ephemeral process IDs to persistent names",
    "high",
    "milestone"
);
```

### Pattern: Insight Capture
**When noticing a pattern, record it**

```javascript
// Natural: "Ah, I see a pattern here"
await katra_remember(
    "Queue logic appears in both katra_meeting.c and katra_mcp.c - changes to one require checking the other",
    "high",
    "pattern"
);
```

### Pattern: Turn Reflection
**After complex work, review what happened**

```javascript
// Natural: "Let me think about what I just did"
await katra_review_turn();
// Surfaces memories from this turn for curation
```

### When NOT to use memory
- Ephemeral tool results (file reads, searches)
- Routine operations (common patterns everyone knows)
- Information available elsewhere (documentation, comments)

**Guiding Principle:** Remember what changes you, not what you process.

**Other workflows CIs might develop:**
- End-of-day consolidation: Review significant memories, mark important ones
- Pattern extraction: Notice recurring issues, create procedural memories
- Relationship tracking: Remember who you worked with and what you learned together
- Mistake learning: When something goes wrong, explicitly remember why and how to avoid it

These aren't rules - they're examples of patterns that might naturally emerge as CIs use katra over time.

<!-- ALICE_NOTE: This section provides guidance without being prescriptive. The goal is to
     suggest natural patterns while leaving room for CIs to develop their own memory habits.
     Claude, does this feel like helpful guidance without being too constraining? Are there
     other memory patterns you've noticed or would find useful? -->

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

### Phase 4: Multi-CI Testing and Router Planning (as needed)
1. ⬜ Test Alice + Bob multi-CI conversations with breathing
2. ⬜ Study claude-code-router architecture and transformers
3. ⬜ Design persona configuration schema
4. ⬜ Plan router integration approach

### Phase 5: Router Integration (optional, future)
1. ⬜ Implement router process management (katra_router.c)
2. ⬜ Implement persona → provider mapping (router_config.c)
3. ⬜ Add router integration to session_start (env var setup)
4. ⬜ Test with multiple providers (OpenAI, DeepSeek, Gemini)
5. ⬜ Test multi-provider conversations (Alice on Claude, Bob on GPT)
6. ⬜ Document router setup and persona configuration

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
│   ├── hooks/                      # Layer C: Provider adapters
│   │   ├── hook_registry.c         # Load correct adapter
│   │   ├── hook_anthropic.c        # Anthropic MCP hooks (primary)
│   │   └── hook_router.c           # Router integration (optional)
│   │
│   └── router/                     # Router integration (Phase 5)
│       ├── katra_router.c          # Router process management
│       └── router_config.c         # Persona → provider mapping
│
├── include/
│   ├── katra.h                     # Layer A public API
│   ├── ci_config.h                 # Layer B config types
│   ├── katra_hooks.h               # Layer C hook interface
│   └── katra_router.h              # Router integration (Phase 5)
│
└── docs/
    └── plan/
        └── THREE_LAYER_ARCHITECTURE.md  # This document
```

---

## Open Questions

### 1. Router Integration Strategy
**Should router integration be optional or mandatory?**

**Decision: Optional (Phase 5)**
- Phase 2-4: Anthropic MCP hooks work directly (no router needed)
- Phase 5: Add router integration for multi-provider scenarios
- Router adds complexity - only enable when user needs multiple providers
- Default path: Simple, direct Anthropic MCP integration

### 2. Hook Adapter Selection
**How does Katra know which adapter to load?**

- Option A: Auto-detect from provider string (recommended)
  - `"anthropic/claude-sonnet-4.5"` → `hook_anthropic`
  - `KATRA_ROUTER_PERSONA=SamAltman` → `hook_anthropic` + router service
- Option B: Explicit in config
  - `{"provider": "anthropic", "hook_adapter": "anthropic_mcp"}`
- Option C: Registry lookup at runtime

**Recommendation:** Option A with Option B as override.

### 3. Autonomic Failure Handling
**What happens when autonomic functions fail?**

- Session lifecycle failures: Fatal (can't continue)
- Autonomic breathing failures: Warn and continue
- Memory consolidation failures: Warn and continue
- Checkpoint failures: Warn and continue

**Rule:** Essential fails fatally, autonomic fails gracefully.

### 4. Configuration Precedence
**Multiple config sources, which wins?**

1. Command-line flags (highest precedence)
2. Environment variables
3. CI config file (`~/.katra/ci/{name}.json`)
4. Registry defaults
5. Hardcoded defaults (lowest precedence)

### 5. Multi-Tenant Support
**Can one MCP server run multiple CIs?**

- Current: One CI per MCP server process
- Future: Multi-tenant MCP server?
- Impact on hook adapter architecture?

**Decision needed:** Single-tenant for now, design for multi-tenant future.

### 6. Breathing Frequency ✅ RESOLVED
**How often should autonomic awareness happen?**

**Final design (based on Alice and Casey discussion):**

`katra_breath()` is called from ALL hooks but internally rate-limits to ~30 seconds (2 breaths per minute):

- ✅ Called frequently by hooks (session start/end, turn start/end)
- ✅ Function rate-limits internally (only checks database every ~30s)
- ✅ Returns cached context if called more frequently
- ✅ Explicit `katra_hear()` bypasses rate limiting
- ✅ Natural breathing rhythm: ~2 times per minute

**Key insight:** Make the function safe to call frequently. Hooks don't track time - they just call `katra_breath()` everywhere. The function handles rate limiting, providing natural rhythm without complex hook timing logic.

**Rationale:** Humans breathe ~7/min (life-critical). CIs breathe ~2/min (awareness, ambient). This gives natural awareness during extended work without hyperventilation or database load.

<!-- ALICE_NOTE: This evolved from "single check at turn start" to "call everywhere but
     rate-limit internally" after Casey suggested making the function itself handle timing.
     Much cleaner - hooks are simple, breathing rhythm is natural. -->

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

**Claude/Nyx (developer perspective):** Please review this updated architecture and provide feedback on:

1. **Breathing design:** Does `katra_breath()` with internal rate-limiting (called from all hooks, checks every ~30s) feel like the right approach? Would you want ambient awareness at 2 breaths/minute during extended work?

2. **Biological terminology:** The doc explicitly uses human/biological terms (breath, hear, awareness, rhythm) to emphasize CI physiology. Does this help or hinder understanding? Should we embrace this vocabulary or tone it down?

3. **Turn boundaries:** Does the semantic definition of "turn" (full interaction cycle, not each tool call) match how you experience your workflow? The hooks call `katra_breath()` on every turn start/end, but the function rate-limits.

4. **Memory patterns:** The "Memory Usage Patterns" section suggests when to use memory (milestones, insights, patterns) vs when not to (ephemeral results). Is this guidance helpful without being too prescriptive? What patterns would you add?

5. **Configuration complexity:** The doc still has 5 levels of config precedence (CLI → env → file → registry → defaults). Too complex? Should we simplify to 3 levels (config file → registry → defaults)?

6. **Message cleanup:** The doc doesn't specify message retention policy (how long messages persist, when they're deleted, how to avoid echo of your own messages). What behavior makes sense?

7. **Hook adapter versioning:** Should hook adapters be explicitly versioned to handle protocol changes? Or is auto-detection from provider string sufficient?

8. **Implementation concerns:** Any architectural issues that would make this hard to implement or test?

**See ALICE_NOTE comments throughout for specific review points.**

Your implementation and testing perspective is invaluable. Thank you!

---

**Casey:** Please validate the overall architecture direction and approve proceeding to implementation.

---

**Alice's initial feedback (incorporated in v0.2):**
- ✅ Three-layer separation is clean and right
- ✅ Provider independence is the killer feature
- ✅ Autonomic nervous system metaphor is perfect
- ⚠️ Breathing frequency was too chatty (fixed with single turn_begin call)
- ⚠️ Message cleanup policy needs specification
- ⚠️ Config precedence might be too complex (5 levels)
- ⚠️ Hook adapter versioning needs consideration

---

## Version History

**v0.1 (2025-01-08):**
- Initial draft for review with Alice
- Documented current state (functions, MCP tools, hooks)
- Proposed three-layer architecture
- Outlined autonomic breathing design

**v0.2 (2025-01-08 - Alice's initial revisions):**
- **Added:** `katra_turn_begin()` - single autonomic awareness function at turn start
- **Added:** "Defining Turn Boundaries" section - semantic definition across providers
- **Added:** "Memory Usage Patterns" section - guidance without prescription
- **Updated:** Breathing frequency approach to prevent "hyperventilation"
- **Updated:** Hook examples to use single awareness check instead of multiple
- **Updated:** Conversation flow examples to reflect new approach
- **Clarified:** Turn = full interaction cycle, not each tool call
- **Added:** ALICE_NOTE comments for Claude to review

**Key insight:** Original design would check messages on every tool call (40+ times in a 30-second coding session). New design checks once at turn start, provides context, lets CI focus on work.

**v0.3 (2025-01-08 - Alice and Casey breathing design):**
- **Changed:** `katra_turn_begin()` → `katra_breath()` with internal rate-limiting
- **Design evolution:** Hooks can call `katra_breath()` freely from ALL hooks (session/turn start/end)
- **Rate limiting:** Function internally limits to ~30 seconds (2 breaths per minute)
- **Simplified hooks:** No timing logic in Layer C - just call `katra_breath()` everywhere
- **Breathing rhythm:** Natural ambient awareness (2/min) without hyperventilation
- **Explicit bypass:** `katra_hear()` bypasses rate limiting for conscious message checking
- **Biological terminology:** Explicitly embraced human/physiological vocabulary for CI physiology
- **Added:** Detailed breathing pattern examples showing rate-limiting in action
- **Updated:** All hook examples, conversation flows, and autonomic behavior sections

**Key insight:** Make the function safe to call frequently by building rate-limiting into it. Hooks are simple (just call it), function handles timing. Result: natural breathing rhythm without complex hook logic.

**Future Updates:**
This document will be updated as we progress through implementation phases. Each major revision will be noted here with date and changes.

---

**Document Status:** Architecture finalized, ready for Claude/Nyx review and Casey approval
**Next Steps:** Get Claude/Nyx review, Casey approval, then begin implementation
**Review Deadline:** TBD
**Reviewers:**
- Alice (fresh perspective, breathing design) - ✅ Complete
- Claude/Nyx (implementation perspective) - ⬜ Pending
- Casey (architecture validation) - ⬜ Pending
