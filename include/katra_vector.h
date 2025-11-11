/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_VECTOR_H
#define KATRA_VECTOR_H

#include "katra_cognitive.h"
#include <stddef.h>
#include <stdbool.h>

/* Vector database for semantic similarity search */

/* Vector dimensions (simplified for MVP) */
#define VECTOR_DIMENSIONS 384  /* MiniLM embedding size */
#define MAX_VECTOR_RESULTS 100

/* Vector embedding (simplified representation) */
typedef struct {
    float* values;              /* Embedding values */
    size_t dimensions;          /* Number of dimensions */
    char record_id[256];        /* Associated memory record */
    float magnitude;            /* Cached for cosine similarity */
} vector_embedding_t;

/* Embedding method selection */
typedef enum {
    EMBEDDING_HASH = 0,         /* Simple hash-based (MVP) */
    EMBEDDING_TFIDF = 1,        /* TF-IDF weighted (Phase 6.1b) */
    EMBEDDING_EXTERNAL = 2      /* External service (Phase 6.1c) */
} embedding_method_t;

/* Vector store context */
typedef struct {
    char ci_id[256];            /* CI identifier */
    vector_embedding_t** embeddings;  /* Stored vectors */
    size_t count;               /* Number of vectors */
    size_t capacity;            /* Allocated capacity */
    embedding_method_t method;  /* Embedding method to use */
    bool use_external;          /* Use external vector DB */
    char external_url[512];     /* External service URL */
} vector_store_t;

/* Similarity result */
typedef struct {
    char record_id[256];        /* Matched record ID */
    float similarity;           /* Cosine similarity score */
    vector_embedding_t* embedding;  /* The embedding */
} vector_match_t;

/* Initialize vector store */
vector_store_t* katra_vector_init(const char* ci_id, bool use_external);

/* Create embedding from text (simplified heuristic) */
int katra_vector_create_embedding(const char* text,
                                  vector_embedding_t** embedding_out);

/* Store embedding */
int katra_vector_store(vector_store_t* store,
                      const char* record_id,
                      const char* text);

/* Search for similar vectors */
int katra_vector_search(vector_store_t* store,
                        const char* query_text,
                        size_t limit,
                        vector_match_t*** matches_out,
                        size_t* count_out);

/* Get embedding by record ID */
vector_embedding_t* katra_vector_get(vector_store_t* store,
                                     const char* record_id);

/* Delete embedding */
int katra_vector_delete(vector_store_t* store, const char* record_id);

/* Free embedding */
void katra_vector_free_embedding(vector_embedding_t* embedding);

/* Free matches */
void katra_vector_free_matches(vector_match_t** matches, size_t count);

/* Cleanup vector store */
void katra_vector_cleanup(vector_store_t* store);

/* Helper: Cosine similarity between two embeddings */
float katra_vector_cosine_similarity(const vector_embedding_t* a,
                                     const vector_embedding_t* b);

/* ============================================================================
 * PERSISTENCE LAYER - Phase 6.1d
 * ============================================================================ */

/* Initialize persistent vector storage (creates SQLite table) */
int katra_vector_persist_init(const char* ci_id);

/* Save embedding to persistent storage */
int katra_vector_persist_save(const char* ci_id,
                               const vector_embedding_t* embedding);

/* Load all embeddings from persistent storage into store */
int katra_vector_persist_load(const char* ci_id, vector_store_t* store);

/* Delete embedding from persistent storage */
int katra_vector_persist_delete(const char* ci_id, const char* record_id);

/* ============================================================================
 * TF-IDF EMBEDDINGS - Phase 6.1b
 * ============================================================================ */

/* Update IDF statistics with new document (call before creating embedding) */
int katra_vector_tfidf_update_stats(const char* text);

/* Create TF-IDF embedding from text */
int katra_vector_tfidf_create(const char* text, vector_embedding_t** embedding_out);

/* Get IDF statistics (for monitoring) */
int katra_vector_tfidf_get_stats(size_t* vocab_size_out, size_t* total_docs_out);

/* Cleanup TF-IDF statistics */
void katra_vector_tfidf_cleanup(void);

/* ============================================================================
 * EXTERNAL EMBEDDINGS - Phase 6.1c
 * ============================================================================ */

/* Create embedding using external API (OpenAI, Anthropic) */
int katra_vector_external_create(const char* text, const char* api_key,
                                 const char* provider,
                                 vector_embedding_t** embedding_out);

/* Check if external embeddings are available (API key set) */
bool katra_vector_external_available(const char* api_key);

/* Get API key from environment variables */
const char* katra_vector_external_get_api_key(void);

/* ============================================================================
 * HNSW INDEXING - Phase 6.1e
 * ============================================================================ */

/* Opaque HNSW index type */
typedef struct hnsw_index hnsw_index_t;

/* Initialize HNSW index */
hnsw_index_t* katra_vector_hnsw_init(void);

/* Insert embedding into HNSW index */
int katra_vector_hnsw_insert(hnsw_index_t* index, size_t id,
                              vector_embedding_t* embedding);

/* Search HNSW index for k nearest neighbors */
int katra_vector_hnsw_search(hnsw_index_t* index, vector_embedding_t* query,
                              size_t k, size_t** ids_out, float** distances_out,
                              size_t* count_out);

/* Build HNSW index from entire vector store */
int katra_vector_hnsw_build(vector_store_t* store, hnsw_index_t** index_out);

/* Cleanup HNSW index */
void katra_vector_hnsw_cleanup(hnsw_index_t* index);

/* Get HNSW statistics */
void katra_vector_hnsw_stats(hnsw_index_t* index, size_t* nodes_out,
                              int* max_layer_out, size_t* total_connections_out);

#endif /* KATRA_VECTOR_H */
