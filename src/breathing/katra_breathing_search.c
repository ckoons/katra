/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_search.c - Hybrid search (keyword + semantic)
 *
 * Phase 6.1f: Integrates vector similarity search with keyword matching
 * for improved recall relevance.
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_breathing_internal.h"
#include "katra_breathing_helpers.h"
#include "katra_breathing_search.h"
#include "katra_memory.h"
#include "katra_vector.h"
#include "katra_log.h"
#include "katra_error.h"
#include "katra_limits.h"

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

/* Combined search result with relevance score */
typedef struct {
    char* record_id;
    float relevance;  /* 0.0-1.0: 1.0 = perfect keyword match, <1.0 = semantic */
    bool from_keyword;
    bool from_semantic;
} search_result_t;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/* Find record by ID in array */
static memory_record_t* find_record_by_id(memory_record_t** records,
                                          size_t count,
                                          const char* record_id) {
    for (size_t i = 0; i < count; i++) {
        if (records[i] && records[i]->record_id &&
            strcmp(records[i]->record_id, record_id) == 0) {
            return records[i];
        }
    }
    return NULL;
}

/* Add or update search result */
static int add_or_update_result(search_result_t** results,
                                size_t* count,
                                size_t* capacity,
                                const char* record_id,
                                float relevance,
                                bool from_keyword,
                                bool from_semantic) {
    /* Check if result already exists */
    for (size_t i = 0; i < *count; i++) {
        if (strcmp((*results)[i].record_id, record_id) == 0) {
            /* Update existing result */
            (*results)[i].relevance = (relevance > (*results)[i].relevance) ?
                                     relevance : (*results)[i].relevance;
            (*results)[i].from_keyword |= from_keyword;
            (*results)[i].from_semantic |= from_semantic;
            return KATRA_SUCCESS;
        }
    }

    /* Add new result */
    if (*count >= *capacity) {
        size_t new_capacity = (*capacity == 0) ? INITIAL_CAPACITY_FALLBACK : (*capacity * 2);
        search_result_t* new_results = realloc(*results,
                                               new_capacity * sizeof(search_result_t));
        if (!new_results) {
            return E_SYSTEM_MEMORY;
        }
        *results = new_results;
        *capacity = new_capacity;
    }

    (*results)[*count].record_id = strdup(record_id);
    if (!(*results)[*count].record_id) {
        return E_SYSTEM_MEMORY;
    }

    (*results)[*count].relevance = relevance;
    (*results)[*count].from_keyword = from_keyword;
    (*results)[*count].from_semantic = from_semantic;
    (*count)++;

    return KATRA_SUCCESS;
}

/* Compare function for sorting by relevance (descending) */
static int compare_relevance(const void* a, const void* b) {
    const search_result_t* ra = (const search_result_t*)a;
    const search_result_t* rb = (const search_result_t*)b;

    /* Sort by relevance descending */
    if (ra->relevance > rb->relevance) return -1;
    if (ra->relevance < rb->relevance) return 1;
    return 0;
}

/* ============================================================================
 * HYBRID SEARCH IMPLEMENTATION
 * ============================================================================ */

char** hybrid_search(const char* topic,
                    memory_record_t** all_results,
                    size_t all_count,
                    size_t* match_count_out) {
    if (!topic || !all_results || !match_count_out || all_count == 0) {
        if (match_count_out) *match_count_out = 0;
        return NULL;
    }

    context_config_t* config = breathing_get_config_ptr();
    search_result_t* combined_results = NULL;
    size_t combined_count = 0;
    size_t combined_capacity = 0;

    /* Phase 1: Keyword matching (always enabled) */
    for (size_t i = 0; i < all_count; i++) {
        if (all_results[i]->content && strcasestr(all_results[i]->content, topic)) {
            add_or_update_result(&combined_results, &combined_count, &combined_capacity,
                                all_results[i]->record_id,
                                1.0f,  /* Perfect relevance for keyword match */
                                true, false);
        }
    }

    LOG_DEBUG("Keyword search found %zu matches for: %s", combined_count, topic);

    /* Phase 2: Semantic similarity search (if enabled) */
    if (config->use_semantic_search) {
        vector_store_t* vector_store = breathing_get_vector_store();
        if (vector_store) {
            vector_match_t** vector_matches = NULL;
            size_t vector_count = 0;

            int result = katra_vector_search(vector_store, topic,
                                            config->max_semantic_results,
                                            &vector_matches, &vector_count);

            if (result == KATRA_SUCCESS && vector_matches) {
                size_t above_threshold = 0;
                for (size_t i = 0; i < vector_count; i++) {
                    /* Only add if similarity above threshold */
                    if (vector_matches[i]->similarity >= config->semantic_threshold) {
                        add_or_update_result(&combined_results, &combined_count,
                                           &combined_capacity,
                                           vector_matches[i]->record_id,
                                           vector_matches[i]->similarity,
                                           false, true);
                        above_threshold++;
                    }
                }
                LOG_DEBUG("Semantic search found %zu/%zu matches above threshold %.2f (total vectors: %zu)",
                         above_threshold, vector_count, config->semantic_threshold,
                         vector_store->count);

                katra_vector_free_matches(vector_matches, vector_count);
            }
        }
    }

    /* Phase 3: Sort by relevance */
    if (combined_count > 0) {
        qsort(combined_results, combined_count, sizeof(search_result_t), compare_relevance);
    }

    /* Phase 4: Convert to memory record array */
    if (combined_count == 0) {
        free(combined_results);
        *match_count_out = 0;
        return NULL;
    }

    /* Build final array of matching records */
    memory_record_t** final_records = calloc(combined_count, sizeof(memory_record_t*));
    if (!final_records) {
        /* Cleanup */
        for (size_t i = 0; i < combined_count; i++) {
            free(combined_results[i].record_id);
        }
        free(combined_results);
        *match_count_out = 0;
        return NULL;
    }

    size_t final_count = 0;
    for (size_t i = 0; i < combined_count; i++) {
        memory_record_t* record = find_record_by_id(all_results, all_count,
                                                    combined_results[i].record_id);
        if (record) {
            final_records[final_count++] = record;
        }
        free(combined_results[i].record_id);
    }
    free(combined_results);

    /* Convert to string array using existing helper */
    char** result_strings = breathing_copy_memory_contents(final_records, final_count,
                                                           match_count_out);
    free(final_records);

    LOG_DEBUG("Hybrid search returned %zu results", *match_count_out);

    return result_strings;
}

/* ============================================================================
 * KEYWORD-ONLY SEARCH (FALLBACK)
 * ============================================================================ */

char** keyword_search_only(const char* topic,
                           memory_record_t** all_results,
                           size_t all_count,
                           size_t* match_count_out) {
    if (!topic || !all_results || !match_count_out || all_count == 0) {
        if (match_count_out) *match_count_out = 0;
        return NULL;
    }

    /* Build filtered array of matching record pointers */
    memory_record_t** filtered = calloc(all_count, sizeof(memory_record_t*));
    if (!filtered) {
        *match_count_out = 0;
        return NULL;
    }

    size_t match_count = 0;
    for (size_t i = 0; i < all_count; i++) {
        if (all_results[i]->content && strcasestr(all_results[i]->content, topic)) {
            filtered[match_count++] = all_results[i];
        }
    }

    if (match_count == 0) {
        free(filtered);
        *match_count_out = 0;
        return NULL;
    }

    /* Use helper to copy matching memories */
    char** matches = breathing_copy_memory_contents(filtered, match_count, match_count_out);
    free(filtered);

    return matches;
}
