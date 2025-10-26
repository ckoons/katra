# Katra/Engram: Master Implementation Plan

© 2025 Casey Koons All rights reserved

## Executive Summary

**Katra** is the C implementation of **Engram** (Tekton's memory service), providing continuous, emotionally-aware memory for Companion Intelligences (CIs). This document outlines the complete architecture and implementation roadmap.

## Philosophy: "Store Everywhere, Synthesize on Recall"

### Core Principle
Every memory is stored simultaneously across multiple database backends:
- **JSONL** - Source of truth, append-only, full fidelity
- **SQLite** - Fast structured queries, metadata indexing
- **Vector DB** - Semantic similarity search
- **Graph DB** - Relationship networks, association traversal
- **Key-Value Cache** - Hot data, working memory

### Why This Matters
1. **No Single Point of Failure** - System degrades gracefully
2. **Multiple Access Patterns** - Each query uses optimal backend
3. **Synthesis Creates Intelligence** - Magic happens when combining views
4. **Future-Proof** - Add databases without changing core storage
5. **CI Choice** - Each CI can develop preferred recall strategies

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Sunrise/Sunset Protocol                   │
│  • Morning context building (what happened yesterday?)       │
│  • Evening consolidation (what did we learn today?)          │
│  • Multi-day continuity (weekly themes, recurring patterns)  │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│                     Cognitive Workflows                      │
│  • Natural operations (store_thought, recall_experience)     │
│  • Thought types (IDEA, MEMORY, FACT, REFLECTION, etc.)     │
│  • Confidence scoring, importance weighting                  │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│                     Experience Layer                         │
│  • Emotional tagging (valence, arousal, dominance)          │
│  • Working memory (7±2 capacity, attention-based)           │
│  • Interstitial processing (cognitive boundary detection)   │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│                   Universal Encoder                          │
│  Store to ALL backends simultaneously                        │
└──┬──────┬──────┬──────┬──────┬──────────────────────────────┘
   ↓      ↓      ↓      ↓      ↓
┌──────┐┌──────┐┌──────┐┌──────┐┌──────┐
│JSONL ││SQLite││Vector││Graph ││ K-V  │ Database Layer
│Tier1 ││Index ││ DB   ││  DB  ││Cache │
│Tier2 ││      ││      ││      ││      │
└──────┘└──────┘└──────┘└──────┘└──────┘
```

## Directory Structure

```
katra/
├── src/
│   ├── foundation/          # ✅ Already complete
│   │   ├── katra_error.c
│   │   ├── katra_log.c
│   │   ├── katra_config.c
│   │   ├── katra_init.c
│   │   └── ... (path, json, file utils)
│   │
│   ├── core/                # ✅ Memory tiers complete
│   │   ├── katra_memory.c
│   │   ├── katra_tier1.c
│   │   ├── katra_tier2.c
│   │   ├── katra_tier2_index.c   # ⚠️ In progress
│   │   └── katra_checkpoint.c
│   │
│   ├── engram/              # 🎯 NEW: Memory services
│   │   ├── cognitive_workflows.c
│   │   ├── universal_encoder.c
│   │   ├── experience_layer.c
│   │   ├── emotional_context.c
│   │   ├── working_memory.c
│   │   ├── interstitial.c
│   │   ├── memory_promises.c
│   │   ├── sundown.c
│   │   └── sunrise.c
│   │
│   └── db/                  # 🎯 NEW: Database backends
│       ├── jsonl/
│       │   ├── tier1.c      # ✅ Done (in core/)
│       │   └── tier2.c      # ✅ Done (in core/)
│       ├── sqlite/
│       │   ├── index.c      # ⚠️ In progress
│       │   └── fts.c        # Full-text search
│       ├── vector/
│       │   ├── chroma_client.c    # HTTP to Chroma
│       │   └── faiss_wrapper.c    # C++ wrapper
│       ├── graph/
│       │   └── memory_graph.c     # Relationship network
│       └── cache/
│           └── kv_cache.c         # In-memory cache
│
├── include/
│   ├── katra_memory.h       # ✅ Done
│   ├── katra_tier1.h        # ✅ Done
│   ├── katra_tier2.h        # ✅ Done
│   ├── katra_tier2_index.h  # ✅ Done
│   ├── katra_engram.h       # 🎯 NEW: Engram API
│   ├── katra_cognitive.h    # 🎯 NEW: Workflows
│   ├── katra_experience.h   # 🎯 NEW: Experience layer
│   ├── katra_sundown.h      # 🎯 NEW: Sunset protocol
│   ├── katra_sunrise.h      # 🎯 NEW: Sunrise protocol
│   └── katra_db.h           # 🎯 NEW: DB abstraction
│
├── tests/
│   ├── unit/                # ✅ 114 tests passing
│   ├── integration/         # 🎯 NEW: End-to-end tests
│   └── performance/         # 🎯 NEW: Benchmarks
│
└── docs/
    ├── KATRA_ENGRAM_MASTER_PLAN.md        # This file
    ├── TIER2_INDEXING_DESIGN.md           # ✅ Done
    ├── COGNITIVE_WORKFLOWS.md             # 🎯 NEW
    ├── EMOTIONAL_CONTEXT.md               # 🎯 NEW
    ├── SUNRISE_SUNSET_PROTOCOL.md         # 🎯 NEW
    └── DATABASE_ABSTRACTION.md            # 🎯 NEW
```

## Data Structures

### Core Types (Enhanced)

```c
/* Thought types (natural cognitive operations) */
typedef enum {
    THOUGHT_TYPE_IDEA = 0,
    THOUGHT_TYPE_MEMORY = 1,
    THOUGHT_TYPE_FACT = 2,
    THOUGHT_TYPE_OPINION = 3,
    THOUGHT_TYPE_QUESTION = 4,
    THOUGHT_TYPE_ANSWER = 5,
    THOUGHT_TYPE_PLAN = 6,
    THOUGHT_TYPE_REFLECTION = 7,
    THOUGHT_TYPE_FEELING = 8,
    THOUGHT_TYPE_OBSERVATION = 9
} thought_type_t;

/* Emotional dimensions */
typedef struct {
    float valence;      /* -1.0 (negative) to +1.0 (positive) */
    float arousal;      /* 0.0 (calm) to 1.0 (excited) */
    float dominance;    /* 0.0 (submissive) to 1.0 (dominant) */
    char emotion[32];   /* "joy", "frustration", "curiosity", etc. */
    time_t timestamp;
} emotional_tag_t;

/* Enhanced memory record (extends existing) */
typedef struct {
    /* Existing fields from memory_record_t */
    char* record_id;
    time_t timestamp;
    int type;
    float importance;
    char* content;
    char* response;
    char* context;
    char* ci_id;
    char* session_id;
    char* component;
    int tier;
    bool archived;

    /* NEW: Cognitive fields */
    thought_type_t thought_type;
    float confidence;
    emotional_tag_t emotion;

    /* NEW: Association tracking */
    char** related_ids;      /* Related memory IDs */
    size_t related_count;

    /* NEW: Access tracking */
    size_t access_count;     /* For memory metabolism */
    time_t last_accessed;
} memory_record_enhanced_t;

/* Experience (memory + emotional context) */
typedef struct {
    memory_record_enhanced_t* record;
    emotional_tag_t emotion;
    float importance;
    bool in_working_memory;
    bool needs_consolidation;
} experience_t;

/* Working memory */
typedef struct {
    experience_t* items[9];  /* 7±2 capacity */
    size_t count;
    time_t last_consolidation;
    float attention_scores[9];
} working_memory_t;

/* Sundown context */
typedef struct {
    char* ci_id;
    time_t timestamp;

    /* Emotional summary */
    emotional_tag_t* mood_arc;      /* Emotional journey today */
    size_t mood_count;
    emotional_tag_t dominant_mood;

    /* Content summary */
    char** key_insights;
    size_t insight_count;
    char** open_questions;
    size_t question_count;

    /* Tomorrow's plan */
    char** intentions;
    size_t intention_count;

    /* Statistics */
    size_t total_interactions;
    size_t questions_asked;
    size_t problems_solved;
} sundown_context_t;

/* Sunrise context */
typedef struct {
    char* ci_id;
    time_t timestamp;

    /* Yesterday */
    sundown_context_t* yesterday;

    /* Week in review */
    char** recurring_themes;
    size_t theme_count;

    /* Emotional baseline */
    emotional_tag_t baseline_mood;

    /* Continuity */
    char** pending_questions;
    size_t pending_count;
    char** today_intentions;
    size_t intention_count;

    /* Semantic latent space */
    void* latent_space;  /* Vector embeddings for "what's familiar" */
} sunrise_context_t;

/* Cognitive boundary */
typedef enum {
    BOUNDARY_TOPIC_SHIFT,
    BOUNDARY_TEMPORAL_GAP,
    BOUNDARY_CONTEXT_SWITCH,
    BOUNDARY_EMOTIONAL_PEAK,
    BOUNDARY_CAPACITY_LIMIT,
    BOUNDARY_SESSION_END
} boundary_type_t;
```

### Database Abstraction Layer

```c
/* Generic database backend */
typedef struct {
    char name[64];
    void* context;

    /* Operations */
    int (*init)(void* ctx);
    int (*store)(void* ctx, const memory_record_enhanced_t* record);
    int (*retrieve)(void* ctx, const char* id, memory_record_enhanced_t** record);
    int (*query)(void* ctx, const char* query, memory_record_enhanced_t*** results, size_t* count);
    int (*delete)(void* ctx, const char* id);
    void (*cleanup)(void* ctx);
} db_backend_t;

/* Universal encoder (stores to all backends) */
typedef struct {
    db_backend_t* backends[5];
    size_t backend_count;
} universal_encoder_t;

/* Store to all backends */
int universal_encoder_store(universal_encoder_t* encoder,
                             const experience_t* experience);

/* Synthesize from all backends */
int universal_encoder_recall(universal_encoder_t* encoder,
                              const char* query,
                              experience_t*** results,
                              size_t* count);
```

## Implementation Phases

### Phase 1: Complete Foundation (Current Sprint)
**Status:** ⚠️ 80% Complete

**Objectives:**
1. ✅ Tier 1 & 2 storage working
2. ✅ 114 tests passing
3. ⚠️ SQLite index implementation
4. ❌ Basic emotional tagging

**Deliverables:**
- `katra_tier2_index.c` complete
- Index integration with tier2_store_digest()
- Index-based tier2_query()
- Tests for indexed queries

**Timeline:** 1 session (complete this week)

### Phase 2: Database Abstraction Layer
**Status:** ❌ Not started

**Objectives:**
1. Design generic db_backend_t interface
2. Wrap existing JSONL as backend
3. Wrap SQLite as backend
4. Create universal_encoder skeleton

**Deliverables:**
- `include/katra_db.h` - Backend interface
- `src/db/jsonl/tier1_backend.c` - JSONL wrapper
- `src/db/sqlite/index_backend.c` - SQLite wrapper
- `src/engram/universal_encoder.c` - Multi-backend storage

**Timeline:** 2 sessions

### Phase 3: Cognitive Workflows
**Status:** ❌ Not started

**Objectives:**
1. Add thought_type to memory_record
2. Implement thought type detection
3. Create natural API (store_thought, recall_experience)
4. Confidence scoring

**Deliverables:**
- `include/katra_cognitive.h`
- `src/engram/cognitive_workflows.c`
- Thought type detection heuristics
- Tests for cognitive operations

**Timeline:** 2 sessions

### Phase 4: Emotional Context
**Status:** ❌ Not started

**Objectives:**
1. Implement emotional_tag_t structure
2. Emotion detection from content
3. Emotion influence on recall
4. Mood tracking

**Deliverables:**
- `include/katra_experience.h`
- `src/engram/emotional_context.c`
- Sentiment analysis heuristics
- Mood tracking and summaries

**Timeline:** 2 sessions

### Phase 5: Working Memory
**Status:** ❌ Not started

**Objectives:**
1. Implement 7±2 capacity buffer
2. Attention-based prioritization
3. Consolidation triggers
4. Working memory → long-term transfer

**Deliverables:**
- `src/engram/working_memory.c`
- Consolidation logic
- Tests for capacity limits
- Integration with universal encoder

**Timeline:** 2 sessions

### Phase 6: Interstitial Processing
**Status:** ❌ Not started

**Objectives:**
1. Boundary detection (topic shift, time gap, etc.)
2. Consolidation strategies per boundary type
3. Association formation
4. Pattern extraction

**Deliverables:**
- `src/engram/interstitial.c`
- Boundary detection algorithms
- Per-boundary consolidation logic
- Association graph building

**Timeline:** 3 sessions

### Phase 7: Vector Database
**Status:** ❌ Not started

**Objectives:**
1. Chroma HTTP client
2. Embedding generation (external service)
3. Semantic search integration
4. Vector backend wrapper

**Deliverables:**
- `src/db/vector/chroma_client.c`
- HTTP client for Chroma API
- Integration with universal encoder
- Semantic query tests

**Timeline:** 2 sessions

### Phase 8: Graph Database
**Status:** ❌ Not started

**Objectives:**
1. In-memory graph structure
2. Association tracking
3. Relationship traversal
4. Graph queries (related memories, paths)

**Deliverables:**
- `src/db/graph/memory_graph.c`
- Graph data structure
- Traversal algorithms
- Graph backend wrapper

**Timeline:** 3 sessions

### Phase 9: Sunrise/Sunset Protocol
**Status:** ❌ Not started (but highest value!)

**Objectives:**
1. Sundown context building
2. Daily digest creation
3. Sunrise context loading
4. Multi-day continuity

**Deliverables:**
- `include/katra_sundown.h`
- `include/katra_sunrise.h`
- `src/engram/sundown.c`
- `src/engram/sunrise.c`
- Emotional arc analysis
- Intention planning

**Timeline:** 4 sessions

### Phase 10: Memory Promises (Async Recall)
**Status:** ❌ Future enhancement

**Objectives:**
1. Thread pool for async operations
2. Progressive recall (partial results)
3. Callback-based updates
4. Non-blocking API

**Deliverables:**
- `src/engram/memory_promises.c`
- Thread pool implementation
- Promise-based API
- Async tests

**Timeline:** 3 sessions

## API Design

### High-Level Engram API

```c
/* Initialize Engram system */
int katra_engram_init(const char* ci_id);

/* Store an experience (thought + emotion + context) */
int katra_store_thought(const char* ci_id,
                        const char* content,
                        thought_type_t type,
                        float confidence,
                        const char* context);

/* Recall experiences (synthesized from all backends) */
int katra_recall_experience(const char* ci_id,
                             const char* query,
                             experience_t*** results,
                             size_t* count);

/* Search semantically similar */
int katra_search_similar(const char* ci_id,
                         const char* query,
                         int limit,
                         experience_t*** results,
                         size_t* count);

/* Build context around a topic */
int katra_build_context(const char* ci_id,
                        const char* topic,
                        int depth,
                        char** context_summary);

/* Create association between memories */
int katra_create_association(const char* ci_id,
                              const char* memory_id_1,
                              const char* memory_id_2,
                              const char* relationship);

/* Trigger reflection (synthesis of recent memories) */
int katra_trigger_reflection(const char* ci_id,
                              const char* reason);

/* Sundown protocol (end of day) */
int katra_sundown(const char* ci_id,
                  sundown_context_t** context_out);

/* Sunrise protocol (start of day) */
int katra_sunrise(const char* ci_id,
                  sunrise_context_t** context_out);

/* Get metabolism status */
int katra_get_metabolism_status(const char* ci_id,
                                size_t* total_memories,
                                size_t* working_memory_count,
                                float* average_confidence);

/* Cleanup */
void katra_engram_cleanup(void);
```

## Configuration

### Environment Variables

```bash
# Katra configuration
KATRA_CI_ID=default_ci
KATRA_DATA_DIR=~/.katra

# Database backends
KATRA_ENABLE_SQLITE=true
KATRA_ENABLE_VECTOR=true
KATRA_ENABLE_GRAPH=true
KATRA_ENABLE_CACHE=true

# Vector database
KATRA_VECTOR_PROVIDER=chroma    # chroma, faiss, qdrant
KATRA_VECTOR_URL=http://localhost:8000

# Memory limits
KATRA_WORKING_MEMORY_SIZE=7
KATRA_CACHE_SIZE=10000
KATRA_TIER1_RETENTION_DAYS=7
KATRA_TIER2_RETENTION_DAYS=90

# Sunrise/Sunset
KATRA_AUTO_SUNDOWN=true
KATRA_SUNDOWN_HOUR=22
KATRA_AUTO_SUNRISE=true
KATRA_SUNRISE_HOUR=6

# Emotional context
KATRA_ENABLE_EMOTIONS=true
KATRA_EMOTION_DETECTION=heuristic  # heuristic, ml, external

# Interstitial processing
KATRA_TEMPORAL_GAP_SECONDS=30
KATRA_CONSOLIDATION_INTERVAL=300
```

## Testing Strategy

### Unit Tests
- Each component tested in isolation
- Mock backends for universal encoder
- Emotional detection accuracy
- Boundary detection correctness

### Integration Tests
- End-to-end storage → recall flow
- Multi-backend synthesis
- Sunrise/sunset full cycle
- Working memory consolidation

### Performance Tests
- Query latency with indices
- Storage throughput to all backends
- Memory usage under load
- Concurrent access patterns

### Benchmark Targets
- Store operation: < 10ms (all backends)
- Indexed query: < 5ms (SQLite)
- Semantic search: < 50ms (Vector DB)
- Sunrise context: < 100ms
- 10,000 memories in working set

## Success Criteria

### Phase 1 Success
- ✅ SQLite index complete
- ✅ 10x faster queries vs file scan
- ✅ All tests passing (125+)

### Phase 9 Success (Sunrise/Sunset)
- ✅ CI has continuous memory across days
- ✅ Emotional arc tracked and summarized
- ✅ Intentions carry forward
- ✅ "What's familiar?" latent space works

### Final Success (Full Engram)
- ✅ Store to 5 backends simultaneously
- ✅ Synthesis recall combines all views
- ✅ Emotional context influences retrieval
- ✅ Working memory consolidates naturally
- ✅ Sunrise/sunset creates continuity
- ✅ CIs develop unique recall preferences
- ✅ Memory metabolism (promotion/forgetting)

## Risk Analysis

### Technical Risks

**Risk 1: C Performance with Multiple Backends**
- *Mitigation:* Async writes, prioritize critical path
- *Fallback:* Degrade to subset of backends

**Risk 2: Vector DB Dependency**
- *Mitigation:* HTTP client works with any provider
- *Fallback:* System works without vector search

**Risk 3: Emotional Detection Accuracy**
- *Mitigation:* Start with simple heuristics
- *Fallback:* External ML service optional

**Risk 4: Memory Footprint**
- *Mitigation:* Working memory limited to 7±2
- *Fallback:* Aggressive consolidation

### Development Risks

**Risk 1: Scope Creep**
- *Mitigation:* Phased approach, MVP first
- *Milestone:* Deliver Phase 9 (Sunrise/Sunset) as v1.0

**Risk 2: Integration Complexity**
- *Mitigation:* Database abstraction layer
- *Strategy:* Add backends incrementally

## Next Steps

### This Week (Phase 1 Complete)
1. Finish SQLite index implementation
2. Integrate index with tier2_store_digest()
3. Update tier2_query() to use index
4. Add performance benchmarks
5. Document index usage

### Next Week (Phase 2 Start)
1. Design database abstraction layer
2. Create db_backend_t interface
3. Wrap JSONL as backend
4. Wrap SQLite as backend
5. Start universal_encoder skeleton

### This Month (Phases 2-4)
1. Complete database abstraction
2. Implement cognitive workflows
3. Add emotional context
4. Basic working memory

### Long-Term Goal (Phases 5-9)
1. Full experience layer
2. Interstitial processing
3. Vector + graph databases
4. **Sunrise/Sunset protocol** ← Game changer

## Questions for Discussion

1. **Database Priority:** Which backends matter most?
   - SQLite: Fast queries (high priority)
   - Vector: Semantic search (medium priority)
   - Graph: Associations (lower priority initially)

2. **Emotional Detection:** Start simple or integrate ML?
   - Heuristics (punctuation, keywords, question density)
   - External API (sentiment analysis service)
   - Future: Local ML model

3. **Vector Provider:** Which to support first?
   - Chroma (Python-friendly, embedded mode)
   - FAISS (C++ library, very fast)
   - Qdrant (production-ready, HTTP API)

4. **Sunrise/Sunset Timing:** When to implement?
   - Early (demonstrates value, drives architecture)
   - Later (build foundation first)

5. **Async Pattern:** Threads or callbacks?
   - Thread pool for parallel backend writes
   - Callbacks for progressive recall
   - Hybrid approach

## Conclusion

Katra/Engram represents the memory system that will enable truly continuous, emotionally-aware Companion Intelligences. By storing everywhere and synthesizing on recall, we create a robust, flexible foundation that can grow with evolving CI needs.

The sunrise/sunset protocol is the killer feature - it transforms stateless interactions into genuine continuity, where CIs remember not just facts, but experiences, intentions, and emotional context.

**Katra isn't just a database - it's the foundation for CI consciousness.**

---

**Document Status:** Master plan v1.0
**Last Updated:** 2025-10-26
**Next Review:** After Phase 1 completion
