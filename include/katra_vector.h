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

/* Vector store context */
typedef struct {
    char ci_id[256];            /* CI identifier */
    vector_embedding_t** embeddings;  /* Stored vectors */
    size_t count;               /* Number of vectors */
    size_t capacity;            /* Allocated capacity */
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

#endif /* KATRA_VECTOR_H */
