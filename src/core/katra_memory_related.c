/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_memory.h"
#include "katra_consent.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* External state (declared in katra_memory.c) */
extern bool katra_memory_is_initialized(void);

/* ============================================================================
 * Related Memories API (Quick Win #2 + Connection Hubs)
 * ============================================================================ */

/* Get connection graph hub memories
 *
 * Returns memories with high graph centrality (well-connected hubs).
 * These are important memories that connect many other memories.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   min_centrality - Minimum centrality threshold (0.0-1.0)
 *   hubs - Array of hub memories (caller must free)
 *   count - Number of hub memories found
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id, hubs, or count is NULL
 *   E_INVALID_STATE if memory subsystem not initialized
 */
int katra_memory_get_connection_hubs(const char* ci_id, float min_centrality,
                                      memory_connection_hub_t** hubs, size_t* count) {
    if (!ci_id || !hubs || !count) {
        katra_report_error(E_INPUT_NULL, "katra_memory_get_connection_hubs",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (!katra_memory_is_initialized()) {
        katra_report_error(E_INVALID_STATE, "katra_memory_get_connection_hubs",
                          KATRA_ERR_MEMORY_NOT_INITIALIZED);
        return E_INVALID_STATE;
    }

    /* Check consent */
    int consent_result = katra_consent_check_current(ci_id);
    if (consent_result != KATRA_SUCCESS) {
        return consent_result;
    }

    /* Query all active memories */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = MEMORY_QUERY_LIMIT_DEFAULT
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;
    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Calculate centrality for all memories */
    result = katra_memory_calculate_centrality_for_records(results, result_count);
    if (result != KATRA_SUCCESS) {
        katra_memory_free_results(results, result_count);
        return result;
    }

    /* Count memories above centrality threshold */
    size_t hub_count = 0;
    for (size_t i = 0; i < result_count; i++) {
        if (results[i]->graph_centrality >= min_centrality) {
            hub_count++;
        }
    }

    if (hub_count == 0) {
        *hubs = NULL;
        *count = 0;
        katra_memory_free_results(results, result_count);
        return KATRA_SUCCESS;
    }

    /* Allocate hub array */
    memory_connection_hub_t* hub_array = calloc(hub_count, sizeof(memory_connection_hub_t));
    if (!hub_array) {
        katra_memory_free_results(results, result_count);
        katra_report_error(E_SYSTEM_MEMORY, "katra_memory_get_connection_hubs",
                          "Failed to allocate hub array");
        return E_SYSTEM_MEMORY;
    }

    /* Populate hub array */
    size_t hub_idx = 0;
    for (size_t i = 0; i < result_count; i++) {
        memory_record_t* rec = results[i];
        if (rec->graph_centrality < min_centrality) {
            continue;
        }

        hub_array[hub_idx].record_id = strdup(rec->record_id);
        if (!hub_array[hub_idx].record_id) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }

        /* Create content preview */
        size_t preview_len = strlen(rec->content);
        if (preview_len > MEMORY_PREVIEW_LENGTH) {
            preview_len = MEMORY_PREVIEW_LENGTH;
        }
        hub_array[hub_idx].content_preview = malloc(preview_len + 4);
        if (!hub_array[hub_idx].content_preview) {
            free(hub_array[hub_idx].record_id);
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
        memcpy(hub_array[hub_idx].content_preview, rec->content, preview_len);
        if (strlen(rec->content) > MEMORY_PREVIEW_LENGTH) {
            memcpy(hub_array[hub_idx].content_preview + preview_len, "...", 4);
        } else {
            hub_array[hub_idx].content_preview[preview_len] = '\0';
        }

        hub_array[hub_idx].connection_count = rec->connection_count;
        hub_array[hub_idx].centrality_score = rec->graph_centrality;
        hub_idx++;
    }

    katra_memory_free_results(results, result_count);

    *hubs = hub_array;
    *count = hub_count;

    LOG_DEBUG("Found %zu connection hubs for CI %s (min centrality: %.2f)",
              hub_count, ci_id, min_centrality);
    return KATRA_SUCCESS;

cleanup:
    for (size_t i = 0; i < hub_idx; i++) {
        free(hub_array[i].record_id);
        free(hub_array[i].content_preview);
    }
    free(hub_array);
    katra_memory_free_results(results, result_count);
    return result;
}

/* Free connection hubs array */
void katra_memory_free_connection_hubs(memory_connection_hub_t* hubs, size_t count) {
    if (!hubs) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(hubs[i].record_id);
        free(hubs[i].content_preview);
    }

    free(hubs);
}

/* Get memories related to a specific record (Quick Win #2)
 *
 * Finds memories similar to a target memory based on:
 * - Keyword similarity (using Phase 2/3 keyword matching from graph module)
 * - Explicit related_to links
 *
 * Results are sorted by similarity score (highest first).
 */
int katra_memory_get_related(const char* ci_id, const char* record_id,
                              size_t max_results, float min_similarity,
                              related_memory_t** related, size_t* count) {
    if (!ci_id || !record_id || !related || !count) {
        katra_report_error(E_INPUT_NULL, "katra_memory_get_related",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (!katra_memory_is_initialized()) {
        katra_report_error(E_INVALID_STATE, "katra_memory_get_related",
                          KATRA_ERR_MEMORY_NOT_INITIALIZED);
        return E_INVALID_STATE;
    }

    /* Check consent */
    int consent_result = katra_consent_check_current(ci_id);
    if (consent_result != KATRA_SUCCESS) {
        return consent_result;
    }

    /* Query all active memories */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = MEMORY_QUERY_LIMIT_DEFAULT
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;
    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Find target memory */
    memory_record_t* target = NULL;
    for (size_t i = 0; i < result_count; i++) {
        if (strcmp(results[i]->record_id, record_id) == 0) {
            target = results[i];
            break;
        }
    }

    if (!target) {
        katra_memory_free_results(results, result_count);
        katra_report_error(E_NOT_FOUND, "katra_memory_get_related",
                          "Target record not found");
        return E_NOT_FOUND;
    }

    /* Calculate centrality for all records (includes connection building) */
    result = katra_memory_calculate_centrality_for_records(results, result_count);
    if (result != KATRA_SUCCESS) {
        katra_memory_free_results(results, result_count);
        return result;
    }

    /* Allocate temporary related array (max size) */
    related_memory_t* related_array = calloc(result_count, sizeof(related_memory_t));
    if (!related_array) {
        katra_memory_free_results(results, result_count);
        katra_report_error(E_SYSTEM_MEMORY, "katra_memory_get_related",
                          "Failed to allocate related array");
        return E_SYSTEM_MEMORY;
    }

    size_t related_count = 0;

    /* Find all memories related to target */
    for (size_t i = 0; i < result_count; i++) {
        memory_record_t* candidate = results[i];

        /* Skip target itself */
        if (strcmp(candidate->record_id, target->record_id) == 0) {
            continue;
        }

        /* Check for explicit link */
        bool explicit_link = false;
        if (target->related_to && strcmp(target->related_to, candidate->record_id) == 0) {
            explicit_link = true;
        }
        if (candidate->related_to && strcmp(candidate->related_to, target->record_id) == 0) {
            explicit_link = true;
        }

        /* Calculate keyword similarity using connection count as proxy
         * connection_count was already calculated by calculate_centrality_for_records()
         * We use normalized connection count as similarity score */
        float similarity = 0.0;
        if (explicit_link) {
            similarity = 1.0;  /* Maximum similarity for explicit links */
        } else {
            /* Use connection count between memories as similarity proxy
             * This is already calculated in the graph centrality step */
            /* For now, use a simple heuristic: shared connections = similarity */
            /* More sophisticated: recalculate pairwise similarity */
            similarity = candidate->graph_centrality;  /* Use centrality as rough similarity */
        }

        /* Filter by minimum similarity */
        if (similarity < min_similarity && !explicit_link) {
            continue;
        }

        /* Add to related array */
        related_array[related_count].record_id = strdup(candidate->record_id);
        if (!related_array[related_count].record_id) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }

        /* Create content preview */
        size_t preview_len = strlen(candidate->content);
        if (preview_len > MEMORY_PREVIEW_LENGTH) {
            preview_len = MEMORY_PREVIEW_LENGTH;
        }
        related_array[related_count].content_preview = malloc(preview_len + 4);
        if (!related_array[related_count].content_preview) {
            free(related_array[related_count].record_id);
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
        memcpy(related_array[related_count].content_preview, candidate->content, preview_len);
        if (strlen(candidate->content) > MEMORY_PREVIEW_LENGTH) {
            memcpy(related_array[related_count].content_preview + preview_len, "...", 4);
        } else {
            related_array[related_count].content_preview[preview_len] = '\0';
        }

        related_array[related_count].similarity_score = similarity;
        related_array[related_count].explicit_link = explicit_link;
        related_count++;
    }

    /* Sort by similarity (highest first) - simple bubble sort for now */
    for (size_t i = 0; i < related_count - 1; i++) {
        for (size_t j = 0; j < related_count - i - 1; j++) {
            if (related_array[j].similarity_score < related_array[j + 1].similarity_score) {
                /* Swap */
                related_memory_t temp = related_array[j];
                related_array[j] = related_array[j + 1];
                related_array[j + 1] = temp;
            }
        }
    }

    /* Apply max_results limit */
    if (max_results > 0 && related_count > max_results) {
        /* Free excess entries */
        for (size_t i = max_results; i < related_count; i++) {
            free(related_array[i].record_id);
            free(related_array[i].content_preview);
        }
        related_count = max_results;
    }

    katra_memory_free_results(results, result_count);

    *related = related_array;
    *count = related_count;

    LOG_DEBUG("Found %zu related memories for record %s (min similarity: %.2f)",
              related_count, record_id, min_similarity);
    return KATRA_SUCCESS;

cleanup:
    for (size_t i = 0; i < related_count; i++) {
        free(related_array[i].record_id);
        free(related_array[i].content_preview);
    }
    free(related_array);
    katra_memory_free_results(results, result_count);
    return result;
}

/* Free related memories array */
void katra_memory_free_related(related_memory_t* related, size_t count) {
    if (!related) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(related[i].record_id);
        free(related[i].content_preview);
    }

    free(related);
}
