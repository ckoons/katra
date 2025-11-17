<!-- © 2025 Casey Koons All rights reserved -->

# Katra Development Roadmap

**Last Updated:** 2025-01-17
**Status:** Phase 6.1f and 6.2 complete, Phase 6.3 (Emotional Tagging) in progress

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

## Current State (2025-01-17)

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

**Phase 4: Multi-CI Testing and Refinement**
- ✅ Test results documented (PHASE4_TEST_RESULTS.md)
- ✅ Extended conversations with breathing verified
- ✅ Natural rhythm during extended work confirmed
- ✅ Concurrent registration tested (3+ CIs)
- ✅ Breathing frequency measured in real usage

**Phase 4.5: Developer Tools & Polish**
- ✅ `katra` wrapper script for easy startup
- ✅ `k` one-shot CLI tool for quick queries
- ✅ Installation targets (install-k, install-all)
- ✅ Developer documentation

**Phase 5: Multi-Provider Support**
- ✅ Wrapper-based multi-provider architecture
- ✅ Support for Anthropic, OpenAI, DeepSeek, OpenRouter
- ✅ Background session management via tmux
- ✅ Session commands (list, attach, stop)
- ✅ Documentation (MULTI_PROVIDER_SETUP.md)

**Phase 6.1: Vector Database**
- ✅ Multiple embedding methods (hash, TF-IDF, external API)
- ✅ HNSW indexing for similarity search
- ✅ SQLite persistence for vectors
- ✅ Test suite (7/7 tests passing)
- ✅ Integration with memory primitives (COMPLETE)

**Phase 6.1f: Vector Search Integration (2025-01-17)**
- ✅ Hybrid search (keyword + semantic similarity)
- ✅ Integrated into recall_about() and what_do_i_know()
- ✅ Enabled by default (0.3 similarity threshold)
- ✅ Configurable thresholds and embedding methods
- ✅ Full test coverage (test_semantic_recall.c - 9/9 passing)

**Phase 6.2: Graph Auto-Edges (2025-01-17)**
- ✅ Automatic edge creation during memory formation
- ✅ SIMILAR edges (vector similarity, bidirectional)
- ✅ SEQUENTIAL edges (temporal chain, unidirectional)
- ✅ Enabled by default via configuration
- ✅ Non-fatal design (graceful degradation)
- ✅ Test suite (test_graph_auto_edges.c - basic verification)

**Phase 6.3: Emotional Tagging (2025-01-17)**
- ✅ PAD model (Pleasure, Arousal, Dominance) structure
- ✅ remember_with_emotion() API for affective memory formation
- ✅ recall_by_emotion() for emotional similarity search
- ✅ Emotion validation and range checking
- ✅ JSON-based PAD storage in context field
- ✅ Legacy emotion field compatibility
- ⚠️ Test suite (5/8 passing - storage working, recall needs test refinement)

### ⚠️ Deferred

**Phase 6.4: Working Memory**
- Implement 7±2 attention-based cache
- Add decay mechanism in katra_breath()
- Integrate with recall functions
- Note: Deferred to future enhancement - core memory features complete

### ❌ Not Yet Implemented

**Advanced Memory (Phase 6.5+):**
- Interstitial processing (cognitive boundaries)
- Universal encoder (store to all backends)
- Multi-backend synthesis layer

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

### Phase 4.5: Developer Tools & Polish (Week 5) ✅ COMPLETE

**Goal:** Refine core system based on Phase 4 findings, add developer convenience

**Motivation:**
Phase 4 testing revealed registration can be lost during development (MCP restarts,
state fiddling). Add auto-registration during breathing to self-heal, plus developer
tools to make working with Katra smoother and more scriptable.

**Tasks:**
- [x] Implement auto-registration in breathing (handles state loss)
- [x] Create `katra start` wrapper (session launcher with environment config)
- [x] Create `k` command (one-shot CLI queries with full Katra access)
- [x] Update documentation with new tools
- [x] Test developer workflows
- [x] Create usage examples

**Success Criteria:** ✅ ALL MET
- ✅ Auto-registration recovers from MCP restarts within 30 seconds
- ✅ `katra start --persona Alice` launches configured session
- ✅ `k "query"` provides quick Katra access with memory
- ✅ Piping works: `echo "text" | k` and `k "query" | grep`
- ✅ Developer experience significantly improved

**Deliverables:** ✅ COMPLETE
- ✅ Auto-registration + periodic cleanup in katra_breath()
- ✅ Hook integration in mcp_protocol.c (breathing on every tool call)
- ✅ scripts/katra wrapper (bash, installed to ~/bin)
- ✅ scripts/k CLI tool (bash, installed to ~/bin)
- ✅ docs/guide/DEVELOPER_TOOLS.md (comprehensive 28KB guide)
- ✅ docs/examples/developer_tools/ (3 example scripts)
- ✅ Makefile install-k/uninstall-k targets

**Completion Date:** 2025-01-10

---

### Phase 5: Multi-Provider Support (Week 6) ✅ COMPLETE

**Goal:** Enable multi-provider support via wrapper-based architecture

**Completion Date:** 2025-01-11

**Design Decision:** After analysis, chose wrapper-based approach over external router:
- Simpler: Uses Claude Code's built-in ANTHROPIC_BASE_URL support
- Zero dependencies: No external router process needed
- Easy configuration: Just environment variables via wrapper script

**Implemented Tasks:**
- [x] Enhanced `katra` wrapper script with multi-provider support
- [x] Added `--provider` flag (anthropic, openai, deepseek, openrouter)
- [x] Added `--model` flag for provider-specific models
- [x] Implemented provider-to-endpoint mapping (ANTHROPIC_BASE_URL)
- [x] Added background session management via tmux
- [x] Created session commands (list, attach, stop)
- [x] Documented multi-provider setup guide
- [x] API key validation per provider

**Success Criteria:** ✅ ALL MET
- ✅ Users can start with specific provider (`katra start --provider openai`)
- ✅ Multiple concurrent sessions with different providers
- ✅ All Katra primitives work unchanged (provider-agnostic)
- ✅ Cross-provider messaging works (shared MCP server)
- ✅ Background sessions via tmux
- ✅ Zero breaking changes to existing functionality

**Deliverables:** ✅ COMPLETE
- ✅ Enhanced `scripts/katra` wrapper (342 lines)
- ✅ Provider configuration via CLI flags
- ✅ Session management commands
- ✅ Documentation: `docs/guide/MULTI_PROVIDER_SETUP.md`
- ✅ Comprehensive Phase 5 plan: `docs/plans/PHASE5_PLAN.md`

**Architecture:**
```bash
# Multiple Claude Code sessions with different providers
katra start --persona Alice --provider anthropic    # Uses Claude
katra start --persona Sam --provider openai         # Uses GPT
katra start --persona Charlie --provider deepseek   # Uses DeepSeek

# All share same Katra MCP server → same memory/communication
```

---

### Phase 6: Advanced Memory (In Progress - Month 2-3)

**Goal:** Implement full Engram architecture

**Status:** Phase 6.1 (Vector Database) partially complete

**Phase 6.1: Vector Database** ⚠️ IN PROGRESS
- [x] Phase 6.1a: Basic vector store (hash-based embeddings)
- [x] Phase 6.1b: TF-IDF embeddings (statistical word importance)
- [x] Phase 6.1c: External API integration (OpenAI embeddings)
- [x] Phase 6.1d: Persistence layer (SQLite storage for vectors)
- [x] Phase 6.1e: HNSW graph search (approximate nearest neighbor)
- [ ] Phase 6.1f: Integration with memory primitives (recall/search)
- [ ] Phase 6.1g: Performance tuning and benchmarking

**Completed Components:**
- ✅ Vector store with multiple embedding methods
- ✅ TF-IDF statistical embeddings
- ✅ OpenAI API integration for production embeddings
- ✅ SQLite persistence for vector data
- ✅ HNSW indexing for fast similarity search
- ✅ Test suite (7/7 tests passing)

**Remaining Tasks:**
- [ ] Graph database integration (Neo4j or embedded)
- [ ] Working memory (attention-based, 7±2 capacity)
- [ ] Emotional tagging (valence, arousal, dominance)
- [ ] Interstitial processing (cognitive boundaries)
- [ ] Universal encoder (store to all backends simultaneously)
- [ ] Multi-backend synthesis (combine vector + graph + SQL)

**Success Criteria:**
- Semantic search works (vector DB) - ⚠️ Partially implemented
- Relationship traversal works (graph DB) - ❌ Not started
- Synthesis creates emergent intelligence - ❌ Not started
- No single point of failure (graceful degradation) - ✅ Works

**Current Deliverables:**
- ✅ Vector DB implementation (src/db/katra_vector*.c - 5 files)
- ✅ Multiple embedding methods (hash, TF-IDF, external API)
- ✅ HNSW search index
- ✅ Persistence layer
- ⏳ Performance benchmarks (tests exist, need tuning)
- ❌ Graph DB backend (not started)
- ❌ Universal encoder (not started)
- ❌ Synthesis layer (not started)

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
