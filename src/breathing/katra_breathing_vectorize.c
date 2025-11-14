/* © 2025 Casey Koons All rights reserved */

/* Vector regeneration for semantic search */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_breathing_internal.h"
#include "katra_memory.h"
#include "katra_vector.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_psyche_common.h"

/* Regenerate all vectors from existing memories */
int regenerate_vectors(void) {
    int result;


    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, __func__,
                          "Breathing layer not initialized");
        return E_INVALID_STATE;
    }

    const char* ci_id = breathing_get_ci_id();
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, __func__, "No CI ID available");
        return E_INPUT_NULL;
    }

    LOG_INFO("Starting vector regeneration for %s", ci_id);

    /* Ensure semantic search is enabled and vector store exists */
    context_config_t* config = breathing_get_config_ptr();
    bool was_disabled = !config->use_semantic_search;
    if (was_disabled) {
        config->use_semantic_search = true;
    }

    /* Initialize vector store if needed */
    result = breathing_init_vector_store();
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize vector store: %d\n", result);
        return result;
    }


    /* Clear existing vectors */
    result = katra_vector_persist_clear(ci_id);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "WARN: Failed to clear existing vectors: %d\n", result);
        LOG_WARN("Failed to clear existing vectors: %d (continuing anyway)", result);
    }

    /* Get vector store */
    vector_store_t* store = breathing_get_vector_store();
    if (!store) {
        katra_report_error(E_INVALID_STATE, __func__,
                          "Vector store not available");
        return E_INVALID_STATE;
    }

    size_t total_success = 0;
    size_t total_skip = 0;

    /* PASS 1: Build IDF statistics from all memories */
    fprintf(stderr, "Pass 1: Building IDF statistics...\n");
    for (int tier = KATRA_TIER1; tier <= KATRA_TIER2; tier++) {
        memory_query_t query = {
            .ci_id = (char*)ci_id,
            .start_time = 0,
            .end_time = 0,
            .type = 0,
            .min_importance = 0.0,
            .tier = tier,
            .limit = 50000
        };

        memory_record_t** results = NULL;
        size_t count = 0;

        result = katra_memory_query(&query, &results, &count);
        if (result != KATRA_SUCCESS) {
            LOG_WARN("Failed to query Tier %d memories: %d", tier, result);
            continue;
        }

        /* Update IDF stats for all memories */
        for (size_t i = 0; i < count; i++) {
            if (!results[i]->content || strlen(results[i]->content) == 0) {
                continue;
            }
            katra_vector_tfidf_update_stats(results[i]->content);
        }

        katra_memory_free_results(results, count);
    }
    fprintf(stderr, "Pass 1 complete: IDF stats built\n");

    /* PASS 2: Create embeddings using the IDF statistics */
    fprintf(stderr, "Pass 2: Creating vector embeddings...\n");
    for (int tier = KATRA_TIER1; tier <= KATRA_TIER2; tier++) {
        memory_query_t query = {
            .ci_id = (char*)ci_id,
            .start_time = 0,
            .end_time = 0,
            .type = 0,
            .min_importance = 0.0,
            .tier = tier,
            .limit = 50000
        };

        memory_record_t** results = NULL;
        size_t count = 0;

        result = katra_memory_query(&query, &results, &count);
        if (result != KATRA_SUCCESS) {
            LOG_WARN("Failed to query Tier %d memories: %d", tier, result);
            continue;
        }

        /* Vectorize each memory WITHOUT updating IDF stats (already built in Pass 1) */
        for (size_t i = 0; i < count; i++) {
            if (!results[i]->content || strlen(results[i]->content) == 0) {
                total_skip++;
                continue;
            }

            /* Create embedding using existing IDF stats (is_query=TRUE prevents stats update) */
            vector_embedding_t* embedding = NULL;
            result = katra_vector_tfidf_create(results[i]->content, &embedding);
            if (result != KATRA_SUCCESS) {
                LOG_WARN("Failed to create embedding for %s: %d",
                       results[i]->record_id, result);
                continue;
            }

            /* Set record ID */
            strncpy(embedding->record_id, results[i]->record_id,
                   sizeof(embedding->record_id) - 1);
            embedding->record_id[sizeof(embedding->record_id) - 1] = '\0';

            /* Add to store */
            if (store->count >= store->capacity) {
                size_t new_capacity = store->capacity * 2;
                vector_embedding_t** new_embeddings = realloc(store->embeddings,
                                                               new_capacity * sizeof(vector_embedding_t*));
                if (!new_embeddings) {
                    katra_vector_free_embedding(embedding);
                    continue;
                }
                store->embeddings = new_embeddings;
                store->capacity = new_capacity;
            }
            store->embeddings[store->count] = embedding;
            store->count++;

            /* Persist */
            result = katra_vector_persist_save(ci_id, embedding);
            if (result == KATRA_SUCCESS) {
                total_success++;

                if (total_success % 1000 == 0) {
                    LOG_INFO("Vectorized %zu total memories...", total_success);
                }
            } else {
                LOG_WARN("Failed to persist %s: %d",
                       results[i]->record_id, result);
            }
        }

        katra_memory_free_results(results, count);
    }

    fprintf(stderr, "\nVector regeneration complete: %zu created, %zu skipped\n",
            total_success, total_skip);

    /* Return number of vectors created */
    return (int)total_success;
}
