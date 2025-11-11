<!-- © 2025 Casey Koons All rights reserved -->

# Phase 6: Advanced Memory (Engram Architecture)

**Created:** 2025-01-10
**Status:** Planning
**Phase:** 6 of 6 (Advanced Memory Features)

---

## Overview

Implement the full Engram architecture with multiple specialized memory backends working in concert. Phase 6 transforms Katra from a simple memory system into a sophisticated multi-modal memory substrate capable of emergent intelligence through backend synthesis.

**Philosophy:**
- **Multi-backend storage** - Vector DB, Graph DB, SQLite work together
- **Universal encoder** - Store to all backends simultaneously
- **Synthesis layer** - Combine results for emergent intelligence
- **Graceful degradation** - No single point of failure

---

## Current State (Phases 1-5)

**What We Have:**
- SQLite-based memory (Tier 1 JSONL, Tier 2 digests)
- Semantic memory (remember with reason: milestone, insight, etc.)
- Simple recall (topic-based search)
- Decision support and learning extraction
- Multi-CI communication
- Multi-provider routing (optional)

**Limitations:**
- No semantic search (keyword matching only)
- No relationship traversal
- No working memory (attention-based selection)
- No emotional context
- No synthesis across memory types

---

## Architecture: The Engram System

### Memory Backends

```
┌─────────────────────────────────────────────┐
│           Universal Encoder                 │
│  (Encode once, store everywhere)            │
└─────────────────────────────────────────────┘
         │            │            │
         ↓            ↓            ↓
    ┌────────┐  ┌──────────┐  ┌─────────┐
    │ Vector │  │  Graph   │  │ SQLite  │
    │   DB   │  │    DB    │  │ (Tier1) │
    │(Chroma)│  │ (Neo4j)  │  │  JSONL  │
    └────────┘  └──────────┘  └─────────┘
         │            │            │
         └────────────┴────────────┘
                    ↓
         ┌─────────────────────┐
         │  Synthesis Layer    │
         │ (Multi-backend AGG) │
         └─────────────────────┘
```

### Backend Purposes

| Backend | Purpose | Queries |
|---------|---------|---------|
| **Vector DB** | Semantic similarity | "Similar to X", "Conceptually related" |
| **Graph DB** | Relationships | "Connected to Y", "Path between A and B" |
| **SQLite** | Structured queries | "Recent memories", "By timestamp/tag" |
| **Synthesis** | Emergent intelligence | Combine all three for deeper insights |

---

## Components

### 6.1: Vector Database Integration

**Purpose:** Semantic search and conceptual similarity

**Technology:** Chroma (embedded vector DB)
- Pure Python, no external dependencies
- Easy installation and setup
- Good performance for local use
- Supports multiple embedding models

**Implementation:**
```c
// Vector DB API
int vector_init(void);
int vector_store(const char* content, const char* embedding[], size_t dim);
int vector_search(const char* query, float threshold, memory_result_t** results, size_t* count);
int vector_cleanup(void);
```

**Features:**
- Automatic embeddings (via Claude or local model)
- Similarity search (cosine distance)
- Semantic clustering
- Concept drift detection

**Use Cases:**
- "Recall memories similar to: 'debugging race conditions'"
- "Find conceptually related work"
- "Cluster memories by theme"

### 6.2: Graph Database Integration

**Purpose:** Relationship networks and traversal

**Technology:** Neo4j (embedded or server)
- Rich relationship modeling
- Powerful traversal queries (Cypher)
- Pattern matching
- Relationship types

**Alternative:** SQLite with graph extension (simpler, embedded)

**Implementation:**
```c
// Graph DB API
int graph_init(void);
int graph_add_node(const char* type, const char* id, json_t* properties);
int graph_add_edge(const char* from_id, const char* to_id, const char* relation);
int graph_traverse(const char* start_id, const char* pattern, graph_result_t** results);
int graph_cleanup(void);
```

**Relationship Types:**
- `RELATES_TO` - General association
- `CAUSED_BY` - Causal link
- `SIMILAR_TO` - Conceptual similarity
- `FOLLOWED_BY` - Temporal sequence
- `CONFLICTS_WITH` - Contradictions
- `DEPENDS_ON` - Dependencies

**Use Cases:**
- "What led to this decision?"
- "Show me everything related to X"
- "Find contradictions in my understanding"
- "Trace the evolution of an idea"

### 6.3: Working Memory

**Purpose:** Attention-based selection (7±2 capacity limit)

**Design:**
- Fixed capacity (7±2 items)
- Attention scoring (relevance to current task)
- Automatic eviction (LRU + attention decay)
- Fast retrieval (in-memory)

**Implementation:**
```c
// Working memory API
int working_memory_init(size_t capacity);  // Default: 7
int working_memory_add(memory_t* memory, float attention);
int working_memory_recall(memory_t** items, size_t* count);
int working_memory_decay(void);  // Reduce attention scores over time
int working_memory_clear(void);
```

**Attention Factors:**
- Recency (recently accessed)
- Relevance (similar to current task)
- Importance (explicit tagging)
- Activation (frequently accessed)

**Use Cases:**
- Maintain context during conversation
- Track active tasks/goals
- Quick access to relevant info
- Simulate human attention limits

### 6.4: Emotional Tagging

**Purpose:** Emotional valence and context

**Model:** PAD (Pleasure, Arousal, Dominance)
- **Pleasure**: Positive (+1) to Negative (-1)
- **Arousal**: Excited (+1) to Calm (-1)
- **Dominance**: Controlled (+1) to Submissive (-1)

**Implementation:**
```c
// Emotional tagging
typedef struct {
    float pleasure;   // [-1, 1]
    float arousal;    // [-1, 1]
    float dominance;  // [-1, 1]
} emotion_t;

int memory_tag_emotion(const char* memory_id, emotion_t emotion);
int memory_recall_by_emotion(emotion_t target, float threshold, memory_t** results);
```

**Use Cases:**
- "Recall positive experiences"
- "What made me frustrated?"
- "Find calm, confident moments"
- Emotional pattern analysis

### 6.5: Interstitial Processing

**Purpose:** Cognitive boundaries and context shifts

**Concept:** Process memories at natural boundaries
- End of turn (micro-interstitial)
- End of task (task interstitial)
- End of session (session interstitial)
- End of day (sunset - already implemented)

**Processing:**
- Extract patterns
- Link related memories
- Identify contradictions
- Consolidate redundant info
- Tag emotional context

**Implementation:**
```c
// Interstitial processing
int interstitial_process_turn(void);     // Lightweight
int interstitial_process_task(void);     // Medium
int interstitial_process_session(void);  // Comprehensive
```

### 6.6: Universal Encoder

**Purpose:** Single encoding, multiple storage

**Design:**
```
Memory → Universal Encoder → [Vector, Graph, SQL, Working]
                ↓
         Unified Memory ID (UUID)
```

**Implementation:**
```c
// Universal encoder
int katra_remember_universal(
    const char* content,
    const char* reason,
    emotion_t emotion,
    memory_t** stored_memory
);

// Stores to:
// - SQLite (Tier 1 JSONL)
// - Vector DB (embeddings)
// - Graph DB (nodes + edges)
// - Working Memory (if attention > threshold)
```

**Benefits:**
- Consistent memory IDs across backends
- Atomic storage (all or nothing)
- Automatic relationship inference
- Simplified API

### 6.7: Synthesis Layer

**Purpose:** Combine results from multiple backends

**Queries:**
```c
// Multi-backend synthesis
int katra_recall_synthesized(
    const char* query,
    recall_options_t* options,
    memory_result_t** results,
    size_t* count
);

typedef struct {
    bool use_vector;      // Semantic search
    bool use_graph;       // Relationship traversal
    bool use_sql;         // Structured queries
    bool use_working;     // Working memory
    float weight_vector;  // Weighting for each backend
    float weight_graph;
    float weight_sql;
    float weight_working;
} recall_options_t;
```

**Synthesis Algorithms:**
- **Union**: Combine results from all backends
- **Intersection**: Only memories found by all
- **Weighted**: Score and rank by backend agreement
- **Hierarchical**: Vector → Graph → SQL cascade

**Use Cases:**
- "Comprehensive recall" - use all backends
- "Deep context" - prioritize relationships
- "Semantic exploration" - emphasize vector search

---

## Implementation Plan

### Phase 6.1: Vector Database (Week 1-2)

**Tasks:**
1. Integrate Chroma vector DB
2. Implement embedding generation
3. Add vector_store() and vector_search()
4. Test semantic search accuracy

**Deliverables:**
- `src/db/katra_vector.c` (vector DB backend)
- Embedding pipeline
- Semantic search API
- Test suite

### Phase 6.2: Graph Database (Week 3-4)

**Tasks:**
1. Choose graph backend (Neo4j vs SQLite graph)
2. Implement relationship modeling
3. Add graph traversal queries
4. Test relationship discovery

**Deliverables:**
- `src/db/katra_graph.c` (graph DB backend)
- Relationship inference
- Traversal API
- Test suite

### Phase 6.3: Working Memory (Week 5)

**Tasks:**
1. Implement attention-based selection
2. Add automatic eviction
3. Integrate with recall
4. Test capacity limits

**Deliverables:**
- `src/memory/katra_working_memory.c`
- Attention decay algorithm
- Integration tests

### Phase 6.4: Universal Encoder & Synthesis (Week 6-7)

**Tasks:**
1. Implement universal encoder
2. Add multi-backend storage
3. Build synthesis layer
4. Test emergent intelligence

**Deliverables:**
- `src/memory/katra_encoder.c`
- `src/memory/katra_synthesis.c`
- Unified API
- Integration tests

### Phase 6.5: Emotional & Interstitial (Week 8)

**Tasks:**
1. Add emotional tagging (PAD model)
2. Implement interstitial processing
3. Test emotional recall
4. Test cognitive boundaries

**Deliverables:**
- `src/memory/katra_emotion.c`
- `src/memory/katra_interstitial.c`
- Emotional pattern analysis
- Test suite

---

## Timeline

**Total: 8 weeks (2 months)**

- Week 1-2: Vector DB integration
- Week 3-4: Graph DB integration
- Week 5: Working memory
- Week 6-7: Universal encoder & synthesis
- Week 8: Emotional tagging & interstitial processing

---

## Success Criteria

1. ✅ Semantic search works (vector DB)
2. ✅ Relationship traversal works (graph DB)
3. ✅ Working memory limits context (7±2 items)
4. ✅ Synthesis creates emergent intelligence
5. ✅ No single point of failure (graceful degradation)
6. ✅ Performance acceptable (<100ms for most queries)
7. ✅ Memory footprint reasonable (<1GB for typical usage)

---

## Dependencies

**External:**
- Chroma (vector DB) - pip install chromadb
- Neo4j (optional, for graph) - or SQLite graph extension
- Embedding model (Claude API or local model)

**Internal:**
- Phases 1-5 complete
- SQLite Tier 1/2 working
- Memory APIs stable

---

## Risks & Mitigations

### Risk 1: Performance Degradation

**Risk:** Multiple backends slow down memory operations
**Mitigation:**
- Async storage (write-behind)
- Lazy loading
- Caching layer
- Benchmark and optimize

### Risk 2: Complexity Explosion

**Risk:** Multiple backends increase maintenance burden
**Mitigation:**
- Clean interfaces (unified API)
- Optional backends (graceful degradation)
- Comprehensive tests
- Clear documentation

### Risk 3: Embedding Costs

**Risk:** API calls for embeddings expensive
**Mitigation:**
- Local embedding model option
- Batch embeddings
- Cache embeddings
- Rate limiting

### Risk 4: Database Size Growth

**Risk:** Multiple backends increase storage requirements
**Mitigation:**
- Automatic archival (old memories)
- Compression
- Storage quotas
- Cleanup utilities

---

## Deliverables

1. **Vector DB Backend:**
   - src/db/katra_vector.c (500-600 lines)
   - Embedding pipeline
   - Semantic search

2. **Graph DB Backend:**
   - src/db/katra_graph.c (500-600 lines)
   - Relationship modeling
   - Traversal queries

3. **Working Memory:**
   - src/memory/katra_working_memory.c (400-500 lines)
   - Attention algorithm
   - Automatic eviction

4. **Universal Encoder:**
   - src/memory/katra_encoder.c (400-500 lines)
   - Multi-backend storage
   - Unified memory IDs

5. **Synthesis Layer:**
   - src/memory/katra_synthesis.c (500-600 lines)
   - Multi-backend aggregation
   - Weighted scoring

6. **Emotional & Interstitial:**
   - src/memory/katra_emotion.c (300-400 lines)
   - src/memory/katra_interstitial.c (400-500 lines)

7. **Documentation:**
   - docs/guide/ADVANCED_MEMORY.md
   - docs/guide/VECTOR_SEARCH.md
   - docs/guide/GRAPH_QUERIES.md
   - docs/examples/advanced_memory/

8. **Tests:**
   - Vector search tests
   - Graph traversal tests
   - Working memory tests
   - Synthesis tests
   - Integration tests

**Total: ~4000 lines of new code**

---

## Future Enhancements (Beyond Phase 6)

- **Federated memory** - Share memories across CIs
- **Memory migration** - Transfer between instances
- **Memory visualization** - Graph UI for relationships
- **Memory analytics** - Pattern discovery, insights
- **Incremental learning** - Update embeddings efficiently

---

**Phase 6 completes the Engram architecture vision: A sophisticated, multi-modal memory substrate capable of emergent intelligence.**
