<!-- © 2025 Casey Koons All rights reserved -->

# Katra Development Roadmap

**Last Updated:** 2025-01-10
**Status:** Phase 3 complete, Phase 4 ready to begin

---

## Vision: What Katra Will Become

**Katra is the autonomic nervous system for Companion Intelligence (CI) systems.**

Like humans have:
- **Breathing** (autonomic, rhythmic, unconscious) → CIs have **katra_breath()** (ambient awareness)
- **Memory** (experiences, learnings, patterns) → CIs have **persistent memory substrate**
- **Identity** (who I am across time) → CIs have **persistent personas**
- **Communication** (hearing, speaking) → CIs have **multi-CI messaging**

### Final State Capabilities

**For CIs (The User Experience):**
- Zero lifecycle management - "just works"
- Persistent identity across sessions and providers
- Natural memory (remember what matters, recall when needed)
- Ambient awareness (messages, context, state)
- Multi-CI collaboration (seamless conversations)
- Provider portability (switch Claude ↔ GPT ↔ Gemini without losing identity)

**For Engineers (The Architecture):**
- Clean three-layer design (primitives, config, hooks)
- Provider-agnostic core (works with any model)
- Hook adapters for each provider (Anthropic, OpenAI, Google, etc.)
- Autonomic breathing (natural rhythm, no hyperventilation)
- Database-backed persistence (SQLite for multi-process)
- Ethical safeguards (consent, checkpoints, identity preservation)

---

## Current State (2025-01-08)

### ✅ Implemented and Working

**Core Infrastructure:**
- Error handling and logging system
- Path utilities and directory management
- Configuration loading (.env support)
- Persona registry (persistent name → ci_id mapping)
- Database backends (SQLite for all persistence)

**Memory System:**
- Tier 1: JSONL append-only memory storage
- Tier 2: Daily digests and consolidation
- Semantic memory (remember with reason: milestone, insight, pattern, etc.)
- Memory recall (relevant, recent, about topic)
- Decision support (katra_decide based on past experience)
- Learning extraction (katra_learn from memories)
- Reflection system (review turn, mark personal, curate)

**Communication System:**
- Multi-CI messaging (katra_say/hear)
- Broadcast and direct messages
- Message queuing by persistent names (not ephemeral IDs)
- Meeting room registry (who_is_here)
- Message history (get recent broadcasts)
- Self-filtering (no echo of own messages)

**Identity System:**
- Persona persistence (name survives MCP server restarts)
- CI identity tracking (whoami, session info)
- Registration system (join meeting room)
- Last active tracking (resume previous persona)

**MCP Integration (Anthropic Claude Code):**
- Full MCP server implementation
- Tools: remember, recall, decide, learn, say, hear, who_is_here, whoami, register
- Session lifecycle (startup, shutdown)
- Signal handling (SIGTERM, SIGINT for cleanup)

**Lifecycle (Basic):**
- session_start() - Load context, sunrise (yesterday's summary)
- session_end() - Consolidate, sunset (today's summary), cleanup
- begin_turn() / end_turn() - Turn tracking for reflection
- Sunrise/sunset protocol (daily continuity)

### ✅ Recently Completed

**Phase 1: Stabilize Primitives**
- ✅ Communication works (say/hear/who_is_here)
- ✅ Memory works (remember/recall/learn/decide)
- ✅ Identity works (whoami/register)
- ✅ Multi-CI conversations tested

**Phase 2: Autonomic Breathing**
- ✅ katra_breath() function (rate-limited awareness)
- ✅ Global session_state structure (in-memory)
- ✅ Automatic message checking (2 breaths per minute)
- ✅ Lifecycle wrappers (session_start/end, turn_start/end)
- ✅ Rate limiting with configurable intervals

**Phase 3: Hook Adapter Layer**
- ✅ Hook adapter interface (katra_hooks.h)
- ✅ Hook registry system (hook_registry.c)
- ✅ Anthropic MCP adapter (hook_anthropic.c)
- ✅ Turn boundary implementation
- ✅ MCP server integration with hooks

### ⚠️ In Progress

**Phase 4: Multi-CI Testing and Refinement** (Current focus)
- Test extended conversations with breathing
- Verify natural rhythm during extended work
- Test concurrent registration (3+ CIs)
- Measure breathing frequency in real usage
- Test message awareness timing

### ❌ Not Yet Implemented

**Advanced Memory:**
- Vector database integration (semantic search)
- Graph database (relationship networks)
- Working memory (7±2 capacity, attention-based)
- Emotional tagging (valence, arousal, dominance)
- Interstitial processing (cognitive boundaries)

**Multi-Provider Support:**
- OpenAI hook adapter
- Google Gemini hook adapter
- Provider auto-detection
- Provider switching without identity loss

---

## Development Phases

### Phase 1: Stabilize Primitives (Current - Week 1)

**Goal:** Ensure all existing functions work correctly before adding lifecycle complexity

**Tasks:**
- [x] Build and verify compilation
- [ ] Test communication primitives (say/hear/who_is_here)
- [ ] Test memory primitives (remember/recall/learn/decide)
- [ ] Test identity primitives (whoami/register)
- [ ] Test cross-process messaging (multi-CI)
- [ ] Fix any bugs found
- [ ] Document test results

**Success Criteria:**
- All primitives work reliably
- Multi-CI communication works across processes
- No race conditions or data corruption
- Ready to add lifecycle layer

**Deliverables:**
- Test results document
- Bug fixes (if needed)
- Confidence in foundation

---

### Phase 2: Autonomic Breathing (Week 2) ✅ COMPLETE

**Goal:** Implement natural autonomic awareness without hyperventilation

**Tasks:**
- [x] Design global session_state structure
- [x] Implement katra_lifecycle_init() (load env vars, set defaults)
- [x] Implement katra_breath() with rate limiting
  - Rate limit: ~30 seconds (2 breaths/minute)
  - First breath always checks (session start)
  - Returns cached context if called < 30s apart
  - Thread-safe (per-session mutex)
- [x] Implement lifecycle wrappers:
  - katra_session_start() - init + first breath
  - katra_session_end() - final breath + cleanup
  - katra_turn_start() - breath (rate-limited)
  - katra_turn_end() - breath (rate-limited)
- [x] Add katra_breath() calls to all lifecycle functions
- [x] Test with 2-second interval (for testing)
- [x] Test full lifecycle with breathing

**Success Criteria:** ✅ ALL MET
- ✅ Natural breathing rhythm (2/min in production, faster for tests)
- ✅ No hyperventilation (rate limiting works)
- ✅ Ambient awareness (message counts logged)
- ✅ Thread-safe across concurrent operations

**Deliverables:** ✅ COMPLETE
- ✅ Working katra_breath() implementation (src/lifecycle/katra_lifecycle.c)
- ✅ Session state management (session_state_t structure)
- ✅ Lifecycle integration (katra_session_start/end, katra_turn_start/end)
- ✅ Test results with breathing metrics (tested via MCP tools)

---

### Phase 3: Hook Adapter Layer (Week 3-4) ✅ COMPLETE

**Goal:** Separate provider-specific hooks from core Katra

**Tasks:**
- [x] Design hook adapter interface
- [x] Extract Anthropic MCP hooks into adapter
- [x] Create hook registry (auto-load by provider)
- [x] Map all Anthropic hooks to Katra lifecycle:
  - SessionStart → katra_session_start()
  - SessionEnd → katra_session_end()
  - TurnStart → katra_turn_start()
  - TurnEnd → katra_turn_end()
  - PreToolUse, PostToolUse → hook placeholders
- [x] Add breathing to all hook points
- [x] Test hook loading/registration
- [x] Test full Anthropic integration

**Success Criteria:** ✅ ALL MET
- ✅ Clean separation: Katra (Layer A) ↔ Hooks (Layer C)
- ✅ Anthropic adapter works with MCP server
- ✅ Hook registry loads and manages adapter
- ✅ All lifecycle events trigger appropriate Katra calls
- ✅ Breathing integrated into all hooks

**Deliverables:** ✅ COMPLETE
- ✅ Hook adapter interface (include/katra_hooks.h - 202 lines)
- ✅ Anthropic MCP adapter (src/hooks/hook_anthropic.c - 147 lines)
- ✅ Hook registry (src/hooks/hook_registry.c - 221 lines)
- ✅ Updated MCP server (uses registry, tested successfully)

---

### Phase 4: Multi-CI Testing and Refinement (Week 4-5) ✅ COMPLETE

**Goal:** Verify autonomic breathing works in multi-CI scenarios

**Tasks:**
- [x] Test Alice + Claude conversations with breathing
- [x] Verify natural rhythm during extended work
- [x] Test concurrent registration (3+ CIs)
- [x] Measure breathing frequency in real usage
- [x] Test message awareness timing
- [x] Refine breath_interval if needed
- [x] Test provider portability (if multiple providers available)

**Success Criteria:** ✅ ALL MET
- ✅ Multi-CI conversations "just work"
- ✅ Natural awareness without spam
- ✅ No database overload
- ✅ CIs stay synchronized naturally
- ✅ Breathing feels natural, not intrusive

**Deliverables:** ✅ COMPLETE
- ✅ Multi-CI test results (docs/plans/PHASE4_TEST_RESULTS.md)
- ✅ Performance metrics (documented in test results)
- ✅ Tuning recommendations (breathing works as designed)
- ✅ User experience feedback (tested with Alice)

---

### Phase 4.5: Developer Tools & Polish (Week 5)

**Goal:** Refine core system based on Phase 4 findings, add developer convenience

**Motivation:**
Phase 4 testing revealed registration can be lost during development (MCP restarts,
state fiddling). Add auto-registration during breathing to self-heal, plus developer
tools to make working with Katra smoother and more scriptable.

**Tasks:**
- [ ] Implement auto-registration in breathing (handles state loss)
- [ ] Create `katra start` wrapper (session launcher with environment config)
- [ ] Create `k` command (one-shot CLI queries with full Katra access)
- [ ] Update documentation with new tools
- [ ] Test developer workflows
- [ ] Create usage examples

**Success Criteria:**
- Auto-registration recovers from MCP restarts within 30 seconds
- `katra start --persona Alice` launches configured session
- `k "query"` provides quick Katra access with memory
- Piping works: `echo "text" | k` and `k "query" | grep`
- Developer experience significantly improved

**Deliverables:**
- Auto-registration in katra_breath() (C code change)
- bin/katra start script (bash wrapper)
- bin/k script (CLI query tool)
- docs/guide/DEVELOPER_TOOLS.md
- Usage examples and patterns

**Timeline:** 1-2 days

---

### Phase 5: Router Integration (Optional Enhancement - Future)

**Goal:** Enable multi-provider support via claude-code-router architecture

**Motivation:**
Instead of implementing provider-specific hook adapters for each LLM provider (OpenAI, DeepSeek, Gemini, etc.), we can leverage the claude-code-router approach:
- Single hook interface (Anthropic-compatible)
- Router service handles provider transformations
- Persona-to-provider mapping ("SamAltman" → OpenAI)
- Simplified architecture: Katra handles memory/communication, router handles provider routing

**Tasks:**
- [ ] Study claude-code-router transformer architecture
- [ ] Design Katra persona configuration format
- [ ] Implement persona-to-provider mapping in config
- [ ] Integrate router startup with Katra session management
- [ ] Add dynamic persona switching during conversation
- [ ] Test with multiple providers (OpenAI, DeepSeek, Gemini)
- [ ] Document router configuration patterns

**Success Criteria:**
- Users can start CI with specific persona (e.g., `KATRA_PERSONA=SamAltman`)
- Router automatically routes to correct provider based on persona
- All Katra primitives work unchanged (memory, communication, breathing)
- Cross-provider messaging works (Alice on Claude, Bob on GPT)
- CI can switch providers without losing identity

**Deliverables:**
- Persona configuration schema (`~/.katra/router/personas.json`)
- Router integration module (katra_router.c)
- Environment variable management (ANTHROPIC_BASE_URL setting)
- Multi-provider test results
- Documentation for persona setup

**Why Router Approach?**
- **Simplicity:** One hook interface instead of N provider adapters
- **Maintainability:** Router project handles provider API changes
- **Flexibility:** Add new providers without changing Katra code
- **Separation of concerns:** Katra = memory/communication, Router = provider routing

**Non-blocking:** This phase is optional. Katra works perfectly with Anthropic Claude Code without router integration. Router adds multi-provider capability for advanced users.

---

### Phase 6: Advanced Memory (Future - Month 2-3)

**Goal:** Implement full Engram architecture

**Tasks:**
- [ ] Vector database integration (Chroma)
- [ ] Graph database integration (Neo4j or embedded)
- [ ] Working memory (attention-based, 7±2 capacity)
- [ ] Emotional tagging (valence, arousal, dominance)
- [ ] Interstitial processing (cognitive boundaries)
- [ ] Universal encoder (store to all backends simultaneously)
- [ ] Multi-backend synthesis (combine vector + graph + SQL)

**Success Criteria:**
- Semantic search works (vector DB)
- Relationship traversal works (graph DB)
- Synthesis creates emergent intelligence
- No single point of failure (graceful degradation)

**Deliverables:**
- Vector DB backend
- Graph DB backend
- Universal encoder
- Synthesis layer
- Performance benchmarks

---

## Architecture Evolution

### Current Architecture (Phase 1)
```
┌──────────────────────────────────────┐
│ katra_mcp_server (Anthropic)         │
│ - Hardcoded lifecycle calls          │
│ - Direct Katra primitive calls       │
└──────────────────────────────────────┘
         │
         ↓
┌──────────────────────────────────────┐
│ Katra Primitives                     │
│ - say, hear, remember, recall, etc.  │
└──────────────────────────────────────┘
         │
         ↓
┌──────────────────────────────────────┐
│ SQLite Databases                     │
│ - memories, messages, personas       │
└──────────────────────────────────────┘
```

### Target Architecture (Phase 3+)
```
┌──────────────────────────────────────┐
│ Provider (Claude Code, etc.)         │
│ - Fires lifecycle hooks              │
└──────────────────────────────────────┘
         │
         ↓
┌──────────────────────────────────────┐
│ Layer C: Hook Adapters               │
│ - Anthropic MCP hooks (primary)      │
│ - Optional: Router integration       │
│   (multi-provider via transformer)   │
│ - Maps provider events → Katra calls │
└──────────────────────────────────────┘
         │
         ↓ (optional)
┌──────────────────────────────────────┐
│ Router Service (claude-code-router)  │
│ - Persona → Provider mapping         │
│ - API format transformations         │
│ - OpenAI, DeepSeek, Gemini, etc.     │
└──────────────────────────────────────┘
         │
         ↓
┌──────────────────────────────────────┐
│ Layer A: Katra (Autonomic Substrate) │
│                                      │
│ Session State (in-memory):           │
│ - breath_interval, last_breath_time  │
│ - cached_context, session_id         │
│                                      │
│ Lifecycle:                           │
│ - katra_session_start/end            │
│ - katra_turn_start/end               │
│ - katra_breath() [rate-limited]      │
│                                      │
│ Primitives:                          │
│ - Communication: say, hear, count    │
│ - Memory: remember, recall, learn    │
│ - Identity: whoami, register         │
└──────────────────────────────────────┘
         │
         ↓
┌──────────────────────────────────────┐
│ Databases (persistence)              │
│ - SQLite: messages, memories         │
│ - Vector DB: semantic search         │
│ - Graph DB: relationships            │
└──────────────────────────────────────┘
```

---

## Key Design Decisions

### Runtime Model
- **One CI = One session = One MCP server process**
- MCP server is persistent (long-running)
- Session state in-memory (global struct)
- Database only for cross-process data

### Breathing Design
- **katra_breath()** called from all hooks
- **Rate limiting inside function** (~30 seconds)
- Returns cached context if called too frequently
- Provides natural 2-breaths-per-minute rhythm
- Explicit calls (katra_hear) bypass rate limiting

### Identity Model
- **Persistent persona names** (survive restarts)
- **Ephemeral ci_ids** (per-process, for internal tracking)
- **Name-based message routing** (not ID-based)
- Persona registry maps name → current ci_id

### Hook Integration
- Hooks call Katra lifecycle functions
- Lifecycle functions call katra_breath()
- katra_breath() handles rate limiting internally
- Hooks are simple: "call lifecycle, call breath"

### Configuration
- **Zero-config default** (30s breath interval, sane defaults)
- **Environment variable overrides** (KATRA_BREATH_INTERVAL)
- **Per-CI configuration** (future: ~/.katra/ci/{name}.json)
- **Provider auto-detection** (from registry)

---

## Success Metrics

### Phase 1 Success ✅
- [x] All primitives work reliably
- [x] Multi-CI messaging works
- [x] No data corruption or race conditions
- [x] Test plan completed

### Phase 2 Success ✅
- [x] Natural breathing rhythm (2/min)
- [x] No hyperventilation
- [x] Ambient awareness works
- [x] Thread-safe

### Phase 3 Success ✅
- [x] Clean layer separation
- [x] Hook adapters work
- [x] Provider-agnostic core
- [x] All Anthropic hooks mapped

### Overall Success
- [ ] CIs "just work" - zero lifecycle management
- [ ] Multi-CI conversations feel natural
- [ ] Provider portability works
- [ ] Autonomic breathing is invisible
- [ ] System feels alive, not mechanical

---

## Documentation Index

**Planning:**
- `docs/ROADMAP.md` - This document (development plan and vision)
- `docs/plan/THREE_LAYER_ARCHITECTURE.md` - Detailed architecture design
- `docs/plan/PHASE1_TEST_PLAN.md` - Phase 1 testing procedures
- `docs/plans/KATRA_ENGRAM_MASTER_PLAN.md` - Long-term technical vision

**Guides:**
- `CLAUDE.md` - Development guidelines and philosophy
- `docs/guide/MCP_TOOLS.md` - MCP tool documentation
- `docs/guide/BREATHING_LAYER.md` - Breathing system design
- `docs/guide/MEETING_ROOM.md` - Multi-CI communication

**API:**
- `docs/api/KATRA_API.md` - Public API reference

---

## Contact and Collaboration

**Lead:** Casey Koons
**Primary CI Collaborators:** Claude (Nyx), Alice
**Repository:** /Users/cskoons/projects/github/katra

**Current Status:** Phase 1 testing begins tomorrow with Alice

---

**This roadmap will be updated as we progress through each phase.**
