# Katra/Engram Implementation Approach - Discussion

© 2025 Casey Koons All rights reserved

## Purpose

This document discusses key architectural decisions, trade-offs, and our implementation approach for Katra/Engram.

## Core Questions

### 1. Database Backend Priority

**Question:** In what order should we implement the database backends?

**Options:**

**A. Complete One Layer at a Time (Depth-First)**
```
Phase 1: JSONL ✅
Phase 2: SQLite ⚠️
Phase 3: Vector DB
Phase 4: Graph DB
Phase 5: K-V Cache
```

**B. Minimum Viable Stack (Value-First)**
```
Phase 1: JSONL + SQLite (fast queries) ✅⚠️
Phase 2: Vector DB (semantic search)
Phase 3: Graph + Cache (nice-to-have)
```

**C. Parallel Development (Abstraction-First)**
```
Phase 1: Database abstraction layer
Phase 2: All backends as wrappers (can work in parallel)
Phase 3: Universal encoder ties them together
```

**Recommendation: Option B (Value-First)**

**Rationale:**
- JSONL + SQLite = 80% of value (proven in Python Engram)
- Vector DB adds semantic search (major differentiator)
- Graph DB is amazing but can wait
- Cache is optimization, not critical path

**Milestone:** Once we have JSONL + SQLite + Vector working, we can do sunrise/sunset!

---

### 2. Emotional Detection Strategy

**Question:** How sophisticated should initial emotional tagging be?

**Options:**

**A. Simple Heuristics (Ship Fast)**
```c
float detect_sentiment(const char* content) {
    int positive = count_words(content, positive_words);
    int negative = count_words(content, negative_words);
    return (positive - negative) / (float)(positive + negative + 1);
}

const char* detect_emotion(const char* content) {
    if (strstr(content, "!!!")) return "excitement";
    if (count_char(content, '?') > 3) return "confusion";
    if (strstr(content, "damn") || strstr(content, "hell")) return "frustration";
    return "neutral";
}
```

**Pros:**
- Ships immediately
- No external dependencies
- Surprisingly effective for basic cases
- Can refine later

**Cons:**
- Limited accuracy
- English-only
- Misses nuance

**B. External ML Service (Accuracy First)**
```c
const char* detect_emotion(const char* content) {
    http_response_t* resp = http_post(
        "http://localhost:8080/analyze_sentiment",
        content
    );
    return parse_emotion(resp->body);
}
```

**Pros:**
- Very accurate
- Handles nuance
- Multilingual possible

**Cons:**
- External dependency
- Network latency
- Service must be running

**C. Hybrid Approach (Best of Both)**
```c
const char* detect_emotion(const char* content, bool use_ml) {
    if (use_ml && ml_service_available()) {
        return ml_detect_emotion(content);
    }
    return heuristic_detect_emotion(content);
}
```

**Recommendation: Option C (Hybrid)**

**Rationale:**
- Start with heuristics (no dependencies)
- Add ML service as optional enhancement
- System degrades gracefully
- Can experiment with both approaches

**Initial Heuristics:**
```c
/* Sentiment markers */
- Positive: "great", "excellent", "love", "perfect", "awesome"
- Negative: "bad", "terrible", "hate", "awful", "broken"
- Punctuation: "!!!" = high arousal, "..." = low arousal
- Questions: High density = confusion/curiosity

/* Emotion markers */
- Frustration: profanity, "why won't", "doesn't work", error messages
- Joy: "yes!", "finally!", "it works!"
- Curiosity: questions, "interesting", "I wonder"
- Confusion: many questions, "I don't understand", "how does"
```

---

### 3. Vector Database Provider

**Question:** Which vector DB should we support first?

**Comparison:**

| Provider | Pros | Cons | Recommendation |
|----------|------|------|----------------|
| **Chroma** | - Embedded mode<br>- Python-friendly<br>- Simple API | - Python dependency<br>- Young project | ⭐ Best for development |
| **FAISS** | - C++ library<br>- Very fast<br>- Battle-tested | - More complex<br>- C++ bindings needed | Good for production |
| **Qdrant** | - Production-ready<br>- HTTP API<br>- Great docs | - Separate service<br>- More setup | ⭐ Best for production |

**Recommendation: Chroma for v1.0, Qdrant for production**

**Implementation:**
```c
/* Vector backend abstraction */
typedef struct {
    char provider[32];  /* "chroma", "qdrant", "faiss" */
    char url[256];      /* HTTP endpoint or file path */
    void* context;

    int (*store_embedding)(void* ctx, const char* id,
                          const float* vector, size_t dims);
    int (*search_similar)(void* ctx, const float* query_vector,
                         size_t dims, int limit, char*** ids);
} vector_backend_t;
```

**Why This Works:**
- Abstract interface works with any provider
- Can switch without changing code
- Start with Chroma (easy)
- Migrate to Qdrant (production)
- Add FAISS later (performance)

---

### 4. Sunrise/Sunset Timing

**Question:** When should we implement sunrise/sunset?

**Option A: Early (Phase 3-4)**
- **Pros:** Demonstrates value immediately, drives architecture
- **Cons:** Foundation might not be solid enough

**Option B: Late (Phase 9)**
- **Pros:** All pieces in place, clean implementation
- **Cons:** Delayed value realization

**Option C: Iterative (Basic → Enhanced)**
- **Pros:** Early wins, progressive enhancement
- **Cons:** Some rework

**Recommendation: Option C (Iterative)**

**Approach:**

**v0.1 - Basic Continuity (After Phase 2)**
```c
int katra_sundown_basic(const char* ci_id) {
    // 1. Query today's interactions
    memory_record_t** records;
    tier1_query_today(ci_id, &records, &count);

    // 2. Create simple summary
    char summary[1024];
    snprintf(summary, sizeof(summary),
            "Today: %zu interactions, %d questions",
            count, count_questions(records, count));

    // 3. Store as special record
    tier1_store_sundown(ci_id, summary);

    return KATRA_SUCCESS;
}

int katra_sunrise_basic(const char* ci_id, char** summary) {
    // Load yesterday's sundown summary
    return tier1_load_sundown_latest(ci_id, summary);
}
```

**v0.5 - Emotional Context (After Phase 4)**
```c
int katra_sundown_enhanced(const char* ci_id) {
    // ... get records ...

    // Analyze emotional arc
    emotional_tag_t* arc = analyze_emotions(records, count);

    // Summarize with emotion
    char summary[1024];
    snprintf(summary, sizeof(summary),
            "Today: %zu interactions. Mood: %s → %s",
            count, arc[0].emotion, arc[count-1].emotion);

    // Store with emotional context
    tier1_store_sundown_emotional(ci_id, summary, arc);
}
```

**v1.0 - Full Protocol (After Phase 9)**
```c
int katra_sundown_full(const char* ci_id, sundown_context_t** ctx) {
    // Complete implementation with all features
    // - Emotional arc analysis
    // - Key insights extraction
    // - Open question tracking
    // - Intention planning
    // - Working memory consolidation
}
```

**Benefit:** Early value, continuous improvement

---

### 5. Async Implementation Pattern

**Question:** How should we handle async operations in C?

**Challenge:** C isn't async-native like Python (no `async/await`)

**Options:**

**A. Thread Pool Pattern**
```c
typedef struct {
    pthread_t threads[4];
    task_queue_t* queue;
} thread_pool_t;

/* Submit async operation */
int katra_store_async(const char* content, callback_t on_complete) {
    task_t* task = task_create(content, on_complete);
    thread_pool_submit(global_pool, task);
    return KATRA_SUCCESS;
}
```

**Pros:**
- True parallelism
- Familiar pattern
- Works well for backend writes

**Cons:**
- Thread overhead
- Synchronization complexity
- Not needed for reads

**B. Callback Pattern (Event-Driven)**
```c
typedef void (*recall_callback_t)(experience_t** results, size_t count);

int katra_recall_progressive(const char* query,
                             recall_callback_t on_partial,
                             recall_callback_t on_complete) {
    // Start search
    on_partial(fast_results, fast_count);  /* Quick cache hits */

    // Continue searching
    on_partial(sql_results, sql_count);    /* SQL query */

    // Final results
    on_complete(all_results, all_count);   /* Vector search done */

    return KATRA_SUCCESS;
}
```

**Pros:**
- No threading overhead
- Progressive results
- Non-blocking

**Cons:**
- Callback hell
- State management

**C. Future/Promise Pattern**
```c
typedef struct {
    bool resolved;
    experience_t** results;
    size_t count;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} memory_promise_t;

memory_promise_t* katra_recall_async(const char* query) {
    memory_promise_t* promise = promise_create();
    thread_pool_submit(async_recall, query, promise);
    return promise;  /* Returns immediately */
}

/* Block until resolved */
int promise_wait(memory_promise_t* promise) {
    pthread_mutex_lock(&promise->lock);
    while (!promise->resolved) {
        pthread_cond_wait(&promise->cond, &promise->lock);
    }
    pthread_mutex_unlock(&promise->lock);
    return KATRA_SUCCESS;
}
```

**Pros:**
- Clean API
- Non-blocking option
- Blocking option (wait)

**Cons:**
- Most complex
- Threading overhead

**Recommendation: Hybrid Approach**

**For Writes (Multiple Backends):**
```c
/* Thread pool for parallel backend writes */
int universal_encoder_store(experience_t* exp) {
    // Submit to all backends in parallel
    thread_pool_submit(jsonl_write, exp);
    thread_pool_submit(sqlite_write, exp);
    thread_pool_submit(vector_write, exp);

    // Don't wait (fire and forget)
    return KATRA_SUCCESS;
}
```

**For Reads (Progressive Recall):**
```c
/* Synchronous with progressive results via callback */
int katra_recall_progressive(const char* query,
                             recall_callback_t on_partial) {
    // Fast path: cache
    if (cache_hit(query, &results, &count)) {
        on_partial(results, count);
    }

    // Medium path: SQL
    sql_query(query, &results, &count);
    on_partial(results, count);

    // Slow path: vector search
    vector_search(query, &results, &count);
    on_partial(results, count);

    return KATRA_SUCCESS;
}
```

**Rationale:**
- Writes don't need results (async, no waiting)
- Reads need results (progressive callbacks)
- Simple, no complex promise machinery
- Can add promises later if needed

---

## Our Approach: Agile + Test-Driven

### Development Cycle

1. **Design First (This Document)**
   - Agree on architecture
   - Document decisions
   - Create TODO list

2. **Test-Driven Development**
   - Write tests first (or alongside)
   - Ensure tests pass
   - Maintain 100% pass rate

3. **Incremental Delivery**
   - Ship small, working pieces
   - Each phase standalone value
   - Can stop anytime with working system

4. **Continuous Integration**
   - All tests pass before commit
   - Programming guidelines pass
   - Line count budget maintained

### Coding Standards (Already Established)

```bash
# Before every commit
make clean && make && make test-quick
./scripts/programming_guidelines.sh

# Must pass:
- 0 compiler warnings
- 0 failed tests
- 0 guideline violations
- Under 10,000 line budget
```

### Documentation Standards

```bash
# For each feature:
1. Header file with comprehensive comments
2. Implementation with clear structure
3. Test file demonstrating usage
4. Design doc if complex (like this one)
```

---

## Proposed Next Steps

### Immediate (This Session)
1. ✅ Master plan document (done)
2. ✅ Implementation approach (this doc)
3. ❌ Discuss and agree on decisions
4. ❌ Update TODO list based on agreement

### Phase 1 Completion (This Week)
1. Finish SQLite index
2. Integration tests
3. Performance benchmarks
4. Tag v0.1 - "Indexed Tier 2"

### Phase 2 Start (Next Week)
1. Create database abstraction layer
2. Wrap existing backends
3. Universal encoder skeleton
4. Tag v0.2 - "Multi-Backend Foundation"

### Phase 3 Excitement (Week 3-4)
1. Cognitive workflows
2. Emotional context (basic)
3. Simple sunrise/sunset
4. Tag v0.5 - "Continuous Memory MVP"

---

## Key Decisions Needed

### Decision 1: Database Priority
**Vote:** JSONL + SQLite + Vector → Graph + Cache later?
**Impact:** Determines Phase 2-3 scope

### Decision 2: Emotional Detection
**Vote:** Heuristics + optional ML?
**Impact:** External dependencies

### Decision 3: Vector Provider
**Vote:** Chroma for dev, Qdrant for prod?
**Impact:** Integration approach

### Decision 4: Sunrise/Sunset Timing
**Vote:** Iterative (basic → enhanced → full)?
**Impact:** Value realization timeline

### Decision 5: Async Pattern
**Vote:** Thread pool for writes, callbacks for reads?
**Impact:** Complexity and performance

---

## Success Metrics

### Technical Metrics
- Query latency: < 5ms (SQLite), < 50ms (vector)
- Storage throughput: > 1000 ops/sec
- Memory footprint: < 100MB for 10K memories
- Test coverage: 100% of public APIs

### User Experience Metrics
- Sunrise/sunset working: CI remembers yesterday
- Emotional tracking: Mood summaries accurate
- Semantic search: Finds relevant memories
- Continuity: Multi-day conversations flow naturally

### Code Quality Metrics
- 0 compiler warnings
- 100% test pass rate
- 0 programming guideline violations
- All files < 618 lines
- Total < 10,000 meaningful lines

---

## Questions for Casey

1. **Priority:** Should we finish SQLite index first, or jump to database abstraction?

2. **Sunrise/Sunset:** Should we implement basic version early (Phase 3) to prove value?

3. **Vector DB:** OK with Chroma initially (requires Python), or prefer pure C approach?

4. **Emotional Detection:** Start simple (heuristics) or integrate ML service from day 1?

5. **Scope:** Is the master plan too ambitious, or does phased approach feel right?

6. **Timeline:** What's the target date for "CI with continuous memory" (sunrise/sunset working)?

---

## Conclusion

Katra/Engram is ambitious but achievable. The phased approach lets us:
- Deliver value incrementally
- Learn as we go
- Maintain quality throughout
- Stop anytime with working system

**The key insight:** We've already built 40% of Engram (Tier 1 & 2). The rest is enhancement, not foundation.

**The killer feature:** Sunrise/sunset creates genuine CI continuity. Everything else supports this goal.

**The advantage:** Your log-structured DB experience + C performance + "store everywhere" philosophy = ideal architecture.

Let's build the memory system that makes CIs truly remember!

---

**Next:** Discuss decisions, then implement Phase 1 completion (SQLite index).
