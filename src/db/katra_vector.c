/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* Project includes */
#include "katra_vector.h"
#include "katra_psyche_common.h"
#include "katra_error.h"
#include "katra_log.h"

/* Initial capacity for vector store */
#define INITIAL_VECTOR_CAPACITY 100

/* Initialize vector store */
vector_store_t* katra_vector_init(const char* ci_id, bool use_external) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, __func__, KATRA_ERR_CI_ID_NULL);
        return NULL;
    }

    vector_store_t* store = calloc(1, sizeof(vector_store_t));
    if (!store) {
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          KATRA_ERR_ALLOC_FAILED);
        return NULL;
    }

    strncpy(store->ci_id, ci_id, sizeof(store->ci_id) - 1);
    store->ci_id[sizeof(store->ci_id) - 1] = '\0';

    store->capacity = INITIAL_VECTOR_CAPACITY;
    store->embeddings = calloc(store->capacity, sizeof(vector_embedding_t*));
    if (!store->embeddings) {
        free(store);
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          KATRA_ERR_ALLOC_FAILED);
        return NULL;
    }

    store->count = 0;
    store->use_external = use_external;
    store->external_url[0] = '\0';

    LOG_INFO("Initialized vector store for %s (external: %s)",
/* GUIDELINE_APPROVED: external vector DB toggle description */
            ci_id, use_external ? "yes" : "no");

    return store;
}

/* Simple text hashing for pseudo-embedding (MVP) */
static void hash_text_to_vector(const char* text, float* vector,
                                 size_t dimensions) {
    if (!text || !vector) {
        return;
    }

    /* Initialize vector */
    for (size_t i = 0; i < dimensions; i++) {
        vector[i] = 0.0f;
    }

    /* Simple character-based hashing */
    size_t len = strlen(text);
    for (size_t i = 0; i < len; i++) {
        unsigned char c = tolower(text[i]);
        if (!isalnum(c)) {
            continue;
        }

        /* Distribute character value across dimensions */
        size_t dim = (c * (i + 1)) % dimensions;
        vector[dim] += 1.0f;

        /* Add neighbor influence */
        if (dim > 0) {
            vector[dim - 1] += 0.5f;
        }
        if (dim < dimensions - 1) {
            vector[dim + 1] += 0.5f;
        }
    }

    /* Normalize */
    float magnitude = 0.0f;
    for (size_t i = 0; i < dimensions; i++) {
        magnitude += vector[i] * vector[i];
    }
    magnitude = sqrtf(magnitude);

    if (magnitude > 0.0f) {
        for (size_t i = 0; i < dimensions; i++) {
            vector[i] /= magnitude;
        }
    }
}

/* Create embedding from text */
int katra_vector_create_embedding(const char* text,
                                  vector_embedding_t** embedding_out) {
    PSYCHE_CHECK_PARAMS_2(text, embedding_out);

    vector_embedding_t* embedding = calloc(1, sizeof(vector_embedding_t));
    if (!embedding) {
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          KATRA_ERR_ALLOC_FAILED);
        return E_SYSTEM_MEMORY;
    }

    embedding->dimensions = VECTOR_DIMENSIONS;
    embedding->values = calloc(VECTOR_DIMENSIONS, sizeof(float));
    if (!embedding->values) {
        free(embedding);
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          KATRA_ERR_ALLOC_FAILED);
        return E_SYSTEM_MEMORY;
    }

    /* Generate pseudo-embedding from text */
    hash_text_to_vector(text, embedding->values, VECTOR_DIMENSIONS);

    /* Calculate magnitude */
    embedding->magnitude = 0.0f;
    for (size_t i = 0; i < VECTOR_DIMENSIONS; i++) {
        embedding->magnitude += embedding->values[i] * embedding->values[i];
    }
    embedding->magnitude = sqrtf(embedding->magnitude);

    *embedding_out = embedding;
    return KATRA_SUCCESS;
}

/* Store embedding */
int katra_vector_store(vector_store_t* store,
                      const char* record_id,
                      const char* text) {
    PSYCHE_CHECK_PARAMS_3(store, record_id, text);

    /* Check if we need to expand capacity */
    if (store->count >= store->capacity) {
        size_t new_capacity = store->capacity * 2;
        vector_embedding_t** new_embeddings = realloc(store->embeddings,
                                                       new_capacity * sizeof(vector_embedding_t*));
        if (!new_embeddings) {
            katra_report_error(E_SYSTEM_MEMORY, __func__,
                              KATRA_ERR_FAILED_TO_EXPAND_EMBEDDINGS);
            return E_SYSTEM_MEMORY;
        }

        store->embeddings = new_embeddings;
        store->capacity = new_capacity;

        /* Initialize new slots */
        for (size_t i = store->count; i < new_capacity; i++) {
            store->embeddings[i] = NULL;
        }
    }

    /* Create embedding */
    vector_embedding_t* embedding = NULL;
    int result = katra_vector_create_embedding(text, &embedding);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Set record ID */
    strncpy(embedding->record_id, record_id, sizeof(embedding->record_id) - 1);
    embedding->record_id[sizeof(embedding->record_id) - 1] = '\0';

    /* Store embedding */
    store->embeddings[store->count] = embedding;
    store->count++;

    LOG_DEBUG("Stored vector for record %s (total: %zu)", record_id, store->count);
    return KATRA_SUCCESS;
}

/* Cosine similarity between two embeddings */
float katra_vector_cosine_similarity(const vector_embedding_t* a,
                                     const vector_embedding_t* b) {
    if (!a || !b || a->dimensions != b->dimensions) {
        return 0.0f;
    }

    if (a->magnitude == 0.0f || b->magnitude == 0.0f) {
        return 0.0f;
    }

    /* Dot product */
    float dot = 0.0f;
    for (size_t i = 0; i < a->dimensions; i++) {
        dot += a->values[i] * b->values[i];
    }

    /* Cosine similarity = dot / (mag_a * mag_b) */
    float similarity = dot / (a->magnitude * b->magnitude);

    /* Clamp to [-1, 1] */
    if (similarity < -1.0f) similarity = -1.0f;
    if (similarity > 1.0f) similarity = 1.0f;

    return similarity;
}

/* Comparison function for sorting matches */
static int compare_matches(const void* a, const void* b) {
    const vector_match_t* match_a = *(const vector_match_t**)a;
    const vector_match_t* match_b = *(const vector_match_t**)b;

    if (match_b->similarity > match_a->similarity) {
        return 1;
    } else if (match_b->similarity < match_a->similarity) {
        return -1;
    }
    return 0;
}

/* Search for similar vectors */
int katra_vector_search(vector_store_t* store,
                        const char* query_text,
                        size_t limit,
                        vector_match_t*** matches_out,
                        size_t* count_out) {
    PSYCHE_CHECK_PARAMS_4(store, query_text, matches_out, count_out);

    if (store->count == 0) {
        *matches_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    /* Create query embedding */
    vector_embedding_t* query_embedding = NULL;
    int result = katra_vector_create_embedding(query_text, &query_embedding);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Allocate matches array (all vectors initially) */
    vector_match_t** matches = calloc(store->count, sizeof(vector_match_t*));
    if (!matches) {
        katra_vector_free_embedding(query_embedding);
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          KATRA_ERR_ALLOC_FAILED);
        return E_SYSTEM_MEMORY;
    }

    /* Calculate similarity for all vectors */
    for (size_t i = 0; i < store->count; i++) {
        vector_match_t* match = calloc(1, sizeof(vector_match_t));
        if (!match) {
            continue;
        }

        match->embedding = store->embeddings[i];
        match->similarity = katra_vector_cosine_similarity(query_embedding,
                                                           store->embeddings[i]);
        strncpy(match->record_id, store->embeddings[i]->record_id,
               sizeof(match->record_id) - 1);

        matches[i] = match;
    }

    /* Sort by similarity (descending) */
    qsort(matches, store->count, sizeof(vector_match_t*), compare_matches);

    /* Limit results */
    size_t result_count = (limit < store->count) ? limit : store->count;
    if (result_count > MAX_VECTOR_RESULTS) {
        result_count = MAX_VECTOR_RESULTS;
    }

    /* Free unused matches */
    for (size_t i = result_count; i < store->count; i++) {
        free(matches[i]);
    }

    /* Free query embedding */
    katra_vector_free_embedding(query_embedding);

    *matches_out = matches;
    *count_out = result_count;

    LOG_DEBUG("Vector search returned %zu matches for query (limit: %zu)",
             result_count, limit);

    return KATRA_SUCCESS;
}

/* Get embedding by record ID */
vector_embedding_t* katra_vector_get(vector_store_t* store,
                                     const char* record_id) {
    if (!store || !record_id) {
        return NULL;
    }

    for (size_t i = 0; i < store->count; i++) {
        if (strcmp(store->embeddings[i]->record_id, record_id) == 0) {
            return store->embeddings[i];
        }
    }

    return NULL;
}

/* Delete embedding */
int katra_vector_delete(vector_store_t* store, const char* record_id) {
    PSYCHE_CHECK_PARAMS_2(store, record_id);

    for (size_t i = 0; i < store->count; i++) {
        if (strcmp(store->embeddings[i]->record_id, record_id) == 0) {
            /* Free embedding */
            katra_vector_free_embedding(store->embeddings[i]);

            /* Shift remaining embeddings */
            for (size_t j = i; j < store->count - 1; j++) {
                store->embeddings[j] = store->embeddings[j + 1];
            }

            store->embeddings[store->count - 1] = NULL;
            store->count--;

            LOG_DEBUG("Deleted vector for record %s", record_id);
            return KATRA_SUCCESS;
        }
    }

    return E_NOT_FOUND;
}

/* Free embedding */
void katra_vector_free_embedding(vector_embedding_t* embedding) {
    if (!embedding) {
        return;
    }

    free(embedding->values);
    free(embedding);
}

/* Free matches */
void katra_vector_free_matches(vector_match_t** matches, size_t count) {
    if (!matches) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(matches[i]);
    }

    free(matches);
}

/* Cleanup vector store */
void katra_vector_cleanup(vector_store_t* store) {
    if (!store) {
        return;
    }

    /* Free all embeddings */
    for (size_t i = 0; i < store->count; i++) {
        katra_vector_free_embedding(store->embeddings[i]);
    }

    free(store->embeddings);
    free(store);

    LOG_DEBUG("Vector store cleaned up");
}
