# Phase 6.1f: Semantic Search Integration

© 2025 Casey Koons All rights reserved

## Overview

**Goal**: Integrate the existing vector database (Phase 6.1a-e) with the breathing layer's `recall_about()` and `what_do_i_know()` functions to enable semantic similarity search.

**Current State**:
- ✅ Vector database fully implemented (TF-IDF, external API, HNSW, persistence)
- ✅ Breathing layer uses simple keyword matching (`strcasestr`)
- ❌ No semantic search integration

**Integration Point**: `recall_about()` and `what_do_i_know()` in `src/breathing/katra_breathing_context.c`

---

## Current Implementation Analysis

### recall_about() - Line 116

**Current Behavior**:
```c
for (size_t i = 0; i < result_count; i++) {
    if (results[i]->content && strcasestr(results[i]->content, topic)) {
        filtered[match_count++] = results[i];
    }
}
```

**Limitations**:
- Exact keyword matching only
- No semantic understanding ("bug" doesn't match "defect", "issue")
- No conceptual similarity ("debugging" vs "troubleshooting")
- Misses relevant memories with different wording

---

## Design: Hybrid Search Strategy

### Approach: Keyword + Vector Fusion

**Philosophy**: Combine fast keyword matching with semantic similarity for best results

**Algorithm**:
1. **Fast keyword pass**: Find exact matches (existing behavior)
2. **Semantic pass**: Vector similarity search
3. **Fusion**: Combine and rank results
4. **Return**: Top N results by relevance score

**Benefits**:
- Backward compatible (keyword search still works)
- Semantic search optional (configurable)
- Performance optimized (keyword filter first)
- Better recall (finds more relevant memories)

---

## Implementation Plan

### Task 1: Add Configuration Options

**Add to `context_config_t` structure**:

```c
typedef struct {
    // ... existing fields ...

    /* Semantic search configuration (Phase 6.1f) */
    bool use_semantic_search;        /* Enable vector similarity search (default: false) */
    float semantic_threshold;        /* Min similarity score 0.0-1.0 (default: 0.6) */
    size_t max_semantic_results;     /* Max vector search results (default: 20) */
    embedding_method_t embedding_method;  /* HASH, TFIDF, or EXTERNAL (default: TFIDF) */
} context_config_t;
```

**Files to modify**:
- `include/katra_breathing.h` - Add config fields
- `src/breathing/katra_breathing_config.c` - Add default values

**Defaults**:
- `use_semantic_search = false` (backward compatible)
- `semantic_threshold = 0.6` (60% similarity minimum)
- `max_semantic_results = 20` (reasonable performance)
- `embedding_method = EMBEDDING_TFIDF` (good balance of speed/accuracy)

### Task 2: Add Vector Store to Breathing Context

**Add to breathing layer global state**:

```c
/* In katra_breathing_internal.h */
typedef struct {
    // ... existing fields ...

    vector_store_t* vector_store;  /* Vector database for semantic search */
} breathing_context_t;
```

**Initialize/cleanup**:
- Initialize in `breathe_init()` if `use_semantic_search = true`
- Cleanup in `breathing_cleanup()`

**Files to modify**:
- `include/katra_breathing_internal.h` - Add vector_store field
- `src/breathing/katra_breathing.c` - Init/cleanup logic

### Task 3: Auto-Index Memories

**Hook vector indexing into memory storage**:

```c
/* In breathing_store_typed_memory() - after successful storage */
if (result == KATRA_SUCCESS && use_semantic_search) {
    /* Index memory for semantic search */
    katra_vector_store(vector_store, record_id, content);
}
```

**Files to modify**:
- `src/breathing/katra_breathing_helpers.c:109` - Add vector indexing after `katra_memory_store()`

**Performance**:
- TF-IDF embedding: ~0.5ms per memory
- HNSW index: ~1ms per memory
- Async option: Queue for background indexing

### Task 4: Implement Hybrid Search

**New function**: `recall_about_semantic()`

```c
/* Hybrid search: keyword + vector similarity */
static char** recall_about_hybrid(const char* topic,
                                 memory_record_t** all_results,
                                 size_t result_count,
                                 size_t* match_count_out) {
    context_config_t* config = breathing_get_config_ptr();

    /* Phase 1: Keyword matching (fast) */
    memory_record_t** keyword_matches = filter_by_keyword(all_results, result_count, topic, &keyword_count);

    /* Phase 2: Vector similarity (semantic) */
    if (config->use_semantic_search) {
        vector_match_t** vector_matches = NULL;
        size_t vector_count = 0;

        katra_vector_search(vector_store, topic, config->max_semantic_results, &vector_matches, &vector_count);

        /* Phase 3: Fusion - combine keyword + vector results */
        memory_record_t** fused = fuse_search_results(
            keyword_matches, keyword_count,
            vector_matches, vector_count,
            all_results, result_count,
            config->semantic_threshold,
            match_count_out
        );

        katra_vector_free_matches(vector_matches, vector_count);
        free(keyword_matches);

        return breathing_copy_memory_contents(fused, *match_count_out, match_count_out);
    }

    /* No semantic search - return keyword matches only */
    char** result = breathing_copy_memory_contents(keyword_matches, keyword_count, match_count_out);
    free(keyword_matches);
    return result;
}
```

**Fusion algorithm**:
1. Start with keyword matches (high confidence)
2. Add vector matches above threshold
3. Remove duplicates (same record_id)
4. Sort by relevance score:
   - Keyword match: 1.0
   - Vector match: cosine similarity score
   - Both: max(1.0, similarity)

**Files to create**:
- `src/breathing/katra_breathing_search.c` - Hybrid search implementation

**Files to modify**:
- `src/breathing/katra_breathing_context.c` - Replace keyword-only logic

### Task 5: Update MCP Tools

**Add semantic search option to `katra_recall`**:

```json
{
    "name": "katra_recall",
    "parameters": {
        "topic": "string (required)",
        "use_semantic": "boolean (optional, default: false)",
        "similarity_threshold": "number (optional, 0.0-1.0, default: 0.6)"
    }
}
```

**Files to modify**:
- `src/mcp/mcp_tools_memory.c` - Add semantic parameters to recall tool

### Task 6: Testing

**Test coverage**:

1. **Unit test**: Semantic search configuration
2. **Unit test**: Vector indexing on memory storage
3. **Unit test**: Hybrid search fusion algorithm
4. **Integration test**: End-to-end semantic recall
5. **Performance test**: Compare keyword vs semantic latency

**Test scenarios**:
- Keyword match only: "bug" finds "This is a bug"
- Semantic match: "defect" finds "This is a bug" (TF-IDF embedding)
- Hybrid: Both approaches find complementary results
- Threshold: Low similarity results excluded

**Files to create**:
- `tests/test_semantic_recall.c` - Semantic search integration tests

---

## Performance Characteristics

### Keyword Search (Current)

- **Latency**: 1-5ms for 1000 memories
- **Recall**: Exact matches only
- **Precision**: High (exact)

### Semantic Search (New)

- **Latency**: 5-20ms for 1000 memories (TF-IDF + HNSW)
- **Recall**: High (finds conceptually similar)
- **Precision**: Medium (threshold-dependent)

### Hybrid Search (Recommended)

- **Latency**: 10-30ms for 1000 memories
- **Recall**: Very high (combines both)
- **Precision**: High (keyword anchors results)

**Optimization**: Cache embeddings, use HNSW approximation for large datasets

---

## Backward Compatibility

**Default behavior unchanged**:
- `use_semantic_search = false` by default
- Existing code works without modification
- Opt-in via configuration or MCP parameter

**Migration path**:
1. Deploy with semantic search disabled
2. Test performance with sample users
3. Enable semantic search for power users
4. Gradually roll out to all users

---

## Configuration Example

### Environment Variable

```bash
# Enable semantic search
export KATRA_USE_SEMANTIC_SEARCH=true
export KATRA_SEMANTIC_THRESHOLD=0.6
export KATRA_EMBEDDING_METHOD=tfidf  # or "external" for OpenAI
```

### MCP Tool Call

```json
{
    "method": "tools/call",
    "params": {
        "name": "katra_recall",
        "arguments": {
            "topic": "debugging race conditions",
            "use_semantic": true,
            "similarity_threshold": 0.7
        }
    }
}
```

---

## Success Criteria

1. **Functionality**:
   - [ ] Semantic search finds conceptually similar memories
   - [ ] Keyword search still works (backward compatible)
   - [ ] Hybrid search combines both approaches
   - [ ] Configuration controls behavior

2. **Performance**:
   - [ ] Latency: <50ms for 1000 memories
   - [ ] Throughput: >100 queries/sec
   - [ ] Memory overhead: <100MB for 10K vectors

3. **Quality**:
   - [ ] Recall improved by >30% vs keyword-only
   - [ ] Precision maintained at >80%
   - [ ] False positive rate <10%

---

## Timeline Estimate

### Task Breakdown

| Task | Effort | Details |
|------|--------|---------|
| **1. Configuration** | 2 hours | Add config fields, defaults |
| **2. Vector store init** | 2 hours | Init/cleanup in breathing layer |
| **3. Auto-indexing** | 3 hours | Hook vector storage into memory creation |
| **4. Hybrid search** | 6 hours | Implement fusion algorithm |
| **5. MCP integration** | 2 hours | Add semantic parameters to tools |
| **6. Testing** | 4 hours | Unit + integration tests |
| **7. Documentation** | 2 hours | Update API docs, examples |
| **Total** | **21 hours** | ~3 days |

### Risk Buffer

- **Low risk**: Vector DB already implemented and tested
- **Integration complexity**: Medium (hybrid search fusion)
- **Testing**: Standard unit + integration coverage
- **Buffer**: +20% (4 hours) for unexpected issues

**Total with buffer: 25 hours (~4 days)**

---

## Deliverables

1. **Code**:
   - `src/breathing/katra_breathing_search.c` (~400 lines)
   - Updates to `katra_breathing_context.c` (~100 lines modified)
   - Updates to `mcp_tools_memory.c` (~50 lines modified)

2. **Tests**:
   - `tests/test_semantic_recall.c` (~300 lines)
   - Integration tests

3. **Documentation**:
   - This plan document
   - Updated API documentation
   - Configuration guide
   - Example usage

**Total new code: ~400 lines**
**Total modified code: ~150 lines**
**Total tests: ~300 lines**

---

## Future Enhancements

1. **Feedback loop**: Learn from user selections to improve ranking
2. **Multi-modal**: Combine keyword + semantic + temporal + importance
3. **Caching**: Cache embeddings and search results
4. **Async indexing**: Background thread for vector indexing
5. **External vector DB**: Integrate with Chroma/Pinecone for scale

---

## References

- **Phase 6 Plan**: `docs/plans/PHASE6_PLAN.md`
- **Vector DB API**: `include/katra_vector.h`
- **Breathing API**: `include/katra_breathing.h`
- **Current Implementation**: `src/breathing/katra_breathing_context.c:116`
