# Tier 2 Indexing and Database Design

© 2025 Casey Koons All rights reserved

## Current State

**What Works:**
- JSONL storage for digests (append-only, simple)
- Linear scan of digest files for queries
- Full filtering support (CI ID, time, type, themes, keywords)
- Works well for small to medium data volumes

**Limitations:**
- O(n) query time - scans all digest files
- No semantic search capabilities
- Theme/keyword search requires full file reads
- No similarity or relevance ranking

## Proposed Enhancements

### 1. SQLite Index (Fast Metadata Queries)

**Purpose:** Fast lookups by structured fields (CI ID, time, type)

**Schema:**
```sql
CREATE TABLE digests (
    digest_id TEXT PRIMARY KEY,
    ci_id TEXT NOT NULL,
    timestamp INTEGER NOT NULL,
    period_type INTEGER NOT NULL,
    period_id TEXT NOT NULL,
    digest_type INTEGER NOT NULL,
    source_record_count INTEGER,
    questions_asked INTEGER,
    archived INTEGER DEFAULT 0,
    file_path TEXT NOT NULL,
    file_offset INTEGER NOT NULL,
    FOREIGN KEY (ci_id) REFERENCES ci_instances(ci_id)
);

CREATE INDEX idx_ci_time ON digests(ci_id, timestamp DESC);
CREATE INDEX idx_period ON digests(period_type, period_id);
CREATE INDEX idx_type ON digests(digest_type);

CREATE TABLE themes (
    digest_id TEXT NOT NULL,
    theme TEXT NOT NULL,
    FOREIGN KEY (digest_id) REFERENCES digests(digest_id)
);

CREATE INDEX idx_themes ON themes(theme, digest_id);

CREATE TABLE keywords (
    digest_id TEXT NOT NULL,
    keyword TEXT NOT NULL,
    FOREIGN KEY (digest_id) REFERENCES digests(digest_id)
);

CREATE INDEX idx_keywords ON keywords(keyword, digest_id);
```

**Location:** `~/.katra/memory/tier2/index/digests.db`

**Query Flow:**
1. SQLite query finds matching digest_ids + file locations
2. Read only the specific digest records from JSONL files
3. Return parsed digests

**Advantages:**
- O(log n) lookups instead of O(n) scans
- Efficient joins and compound queries
- Atomic transactions for consistency
- Built-in backup/recovery (SQLite features)

### 2. Vector Database (Semantic Search)

**Purpose:** Semantic similarity search on digest content

**Implementation Options:**

#### Option A: Chroma (Recommended)
- Python-based, embedded mode available
- Built for LLM applications
- Automatic embedding generation
- Simple API

```python
# Store digest
collection.add(
    ids=[digest.digest_id],
    documents=[digest.summary],
    metadatas=[{
        "ci_id": digest.ci_id,
        "period_id": digest.period_id,
        "timestamp": digest.timestamp
    }]
)

# Query semantically
results = collection.query(
    query_texts=["what did we discuss about memory systems?"],
    n_results=10,
    where={"ci_id": "my_ci"}
)
```

#### Option B: FAISS (Facebook AI Similarity Search)
- C++ library with Python bindings
- Extremely fast for large datasets
- Lower-level, more control
- No metadata filtering built-in

#### Option C: Qdrant (Production-ready)
- REST API or embedded mode
- Built-in filtering
- Disk persistence
- Rust-based (fast, safe)

**Recommendation:** Start with Chroma for simplicity, migrate to Qdrant if scaling needed.

**Location:** `~/.katra/memory/tier2/vectors/`

### 3. Full-Text Search (Content Search)

**Purpose:** Fast text search across summaries and insights

**Implementation Options:**

#### Option A: SQLite FTS5
- Built into SQLite (no extra dependencies)
- Good for moderate text search needs
- Integrated with existing SQL index

```sql
CREATE VIRTUAL TABLE digest_fts USING fts5(
    digest_id UNINDEXED,
    summary,
    key_insights,
    content='digests',
    content_rowid='rowid'
);

-- Query
SELECT digest_id FROM digest_fts
WHERE digest_fts MATCH 'memory AND (storage OR retrieval)'
ORDER BY rank;
```

#### Option B: Elasticsearch
- Industry standard for full-text search
- Excellent ranking and highlighting
- Overkill for single-user system
- Resource intensive

**Recommendation:** SQLite FTS5 for simplicity and integration.

### 4. Hybrid Architecture (Recommended)

**Three-Layer Query System:**

```
┌─────────────────────────────────────────┐
│  Query Interface (tier2_query)          │
└──────────────┬──────────────────────────┘
               ↓
┌──────────────────────────────────────────┐
│  Query Router                             │
│  • Structured query → SQLite              │
│  • Semantic query → Vector DB             │
│  • Text search → FTS5                     │
│  • Combine and rank results               │
└──────────────┬───────────────────────────┘
               ↓
       ┌───────┴───────┬───────────────┐
       ↓               ↓               ↓
┌─────────────┐ ┌──────────────┐ ┌─────────┐
│ SQLite Index│ │  Vector DB   │ │ JSONL   │
│ .db files   │ │  embeddings  │ │ digests │
└─────────────┘ └──────────────┘ └─────────┘
```

**Query Examples:**

```c
// 1. Structured query (SQLite)
digest_query_t query = {
    .ci_id = "ci_id",
    .start_time = week_ago,
    .period_type = PERIOD_TYPE_WEEKLY
};
tier2_query(&query, &results, &count);

// 2. Semantic search (Vector DB)
digest_query_t query = {
    .ci_id = "ci_id",
    .semantic_query = "discussions about memory architecture"
};
tier2_query_semantic(&query, &results, &count);

// 3. Hybrid query (both)
digest_query_t query = {
    .ci_id = "ci_id",
    .start_time = week_ago,
    .semantic_query = "memory systems",
    .rerank_by_relevance = true
};
tier2_query_hybrid(&query, &results, &count);
```

## Implementation Plan

### Phase 1: SQLite Index (Immediate)

**Files to Create:**
- `src/core/katra_tier2_index.c` - SQLite index management
- `include/katra_tier2_index.h` - Index API
- `tests/unit/test_tier2_index.c` - Index tests

**Functions:**
```c
// Initialize index database
int tier2_index_init(const char* ci_id);

// Add digest to index (called from tier2_store_digest)
int tier2_index_add(const digest_record_t* digest,
                    const char* file_path, long offset);

// Query index for matching digest_ids
int tier2_index_query(const digest_query_t* query,
                      char*** digest_ids, size_t* count);

// Load specific digests by ID
int tier2_load_by_ids(const char** digest_ids, size_t count,
                      digest_record_t*** results, size_t* result_count);
```

**Integration:**
1. `tier2_store_digest()` calls `tier2_index_add()` after writing JSONL
2. `tier2_query()` calls `tier2_index_query()` first, then loads specific records
3. Fallback to file scan if index doesn't exist or is out of sync

**Benefits:**
- 10-100x faster queries for large digest collections
- Maintain backward compatibility (JSONL still primary storage)
- Gradual migration (index built on first query if missing)

### Phase 2: Vector Database (Future)

**Options for Integration:**

#### Option A: Python Bridge
- Write Python wrapper for vector operations
- C calls Python via libpython
- Use Chroma in embedded mode

#### Option B: Native C++ (FAISS)
- Compile FAISS C++ library
- Write C bindings
- More complex but no Python dependency

#### Option C: HTTP API (Qdrant)
- Run Qdrant server
- HTTP calls from C (using existing HTTP utilities)
- Best for multi-user/distributed setup

**Recommendation:** Start with Option C (Qdrant HTTP API) for clean separation.

### Phase 3: Full-Text Search (Future)

**Integration with SQLite:**
- Add FTS5 virtual table to existing index database
- Populate during `tier2_index_add()`
- Query alongside structured fields

## File Size Considerations

**Current Approach:**
- One function per file principle
- 600 line limit per file

**With Indexing:**
- `katra_tier2.c` (450 lines) - Core digest operations
- `katra_tier2_json.c` (274 lines) - JSON serialization
- `katra_tier2_index.c` (NEW, ~400 lines) - SQLite index
- `katra_tier2_query.c` (NEW, ~300 lines) - Query routing and optimization
- `katra_tier2_vector.c` (FUTURE, ~300 lines) - Vector DB integration

All files stay well under 600 line limit.

## Configuration

**Add to .env.katra:**
```bash
# Tier 2 indexing
TIER2_INDEX_ENABLED=true
TIER2_INDEX_REBUILD_ON_START=false

# Vector database (future)
TIER2_VECTOR_ENABLED=false
TIER2_VECTOR_PROVIDER=qdrant  # chroma, faiss, qdrant
TIER2_VECTOR_URL=http://localhost:6333

# Full-text search
TIER2_FTS_ENABLED=true
```

## Migration Strategy

**Backward Compatibility:**
1. JSONL remains primary storage format
2. Index is supplementary (can be rebuilt anytime)
3. If index missing/corrupt, fall back to file scan
4. Existing code continues to work

**Index Rebuild:**
```c
// Rebuild index from JSONL files
int tier2_index_rebuild(const char* ci_id);

// Called on first query if index doesn't exist
// Or manually via katra_admin tool
```

## Performance Expectations

**Current (File Scan):**
- 1,000 digests: ~100ms query time
- 10,000 digests: ~1s query time
- 100,000 digests: ~10s query time

**With SQLite Index:**
- 1,000 digests: ~5ms query time (20x faster)
- 10,000 digests: ~10ms query time (100x faster)
- 100,000 digests: ~50ms query time (200x faster)

**With Vector Search:**
- Semantic similarity: ~50-100ms per query
- Returns most relevant results, not just filtered matches
- Particularly valuable for "find similar" operations

## Next Steps

**Immediate (This Session):**
1. Create `katra_tier2_index.c` with SQLite integration
2. Update `tier2_store_digest()` to maintain index
3. Update `tier2_query()` to use index when available
4. Add index tests to test suite

**Near Term (Next Session):**
1. Add query performance benchmarks
2. Implement index rebuild tool
3. Add index statistics/health checks

**Future:**
1. Vector database integration (when semantic search needed)
2. Multi-user index synchronization
3. Distributed query capabilities

Would you like me to start implementing the SQLite index now?
