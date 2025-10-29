/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_consent.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* External state (declared in katra_memory.c) */
extern bool katra_memory_is_initialized(void);

/* Consolidation thresholds (match katra_tier1_archive.c) */
#define RECENT_ACCESS_DAYS 7
#define HIGH_EMOTION_THRESHOLD 0.7f
#define HIGH_CENTRALITY_THRESHOLD 0.5f

/* ============================================================================
 * Metacognitive Awareness API (Thane's Active Sense-Making)
 * ============================================================================ */

/* Get memory consolidation health status */
int katra_memory_get_consolidation_health(const char* ci_id, memory_consolidation_health_t* health) {
    if (!ci_id || !health) {
        katra_report_error(E_INPUT_NULL, "katra_memory_get_consolidation_health",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!katra_memory_is_initialized()) {
        katra_report_error(E_INVALID_STATE, "katra_memory_get_consolidation_health",
                          "Memory subsystem not initialized");
        return E_INVALID_STATE;
    }

    /* Check consent */
    int consent_result = katra_consent_check_current(ci_id);
    if (consent_result != KATRA_SUCCESS) {
        return consent_result;
    }

    /* Initialize health structure */
    memset(health, 0, sizeof(memory_consolidation_health_t));

    /* Get total memory count from stats */
    memory_stats_t stats;
    int result = katra_memory_stats(ci_id, &stats);
    if (result != KATRA_SUCCESS) {
        return result;
    }
    health->total_memories = stats.tier1_records;

    /* Get active memory count from query */
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
    size_t active_count = 0;
    result = katra_memory_query(&query, &results, &active_count);
    if (result != KATRA_SUCCESS) {
        return result;
    }
    katra_memory_free_results(results, active_count);

    health->active_memories = active_count;
    health->archived_memories = (health->total_memories > health->active_memories) ?
                                (health->total_memories - health->active_memories) : 0;

    /* Calculate compression ratio */
    if (health->total_memories > 0) {
        health->compression_ratio = (float)health->archived_memories / (float)health->total_memories;
    } else {
        health->compression_ratio = 0.0;
    }

    /* Determine if consolidation recommended */
    health->consolidation_recommended = (health->active_memories >= MEMORY_CONSOLIDATION_THRESHOLD);

    /* Determine health status */
    if (health->active_memories < MEMORY_HEALTH_THRESHOLD_LOW) {
        health->health_status = "healthy";
    } else if (health->active_memories < MEMORY_HEALTH_THRESHOLD_HIGH) {
        health->health_status = "degraded";
    } else {
        health->health_status = "critical";
    }

    LOG_DEBUG("Memory health: total=%zu, active=%zu, archived=%zu, compression=%.1f%%, status=%s",
              health->total_memories, health->active_memories, health->archived_memories,
              health->compression_ratio * 100.0, health->health_status);

    return KATRA_SUCCESS;
}

/* Get memories at risk of archival */
int katra_memory_get_at_risk(const char* ci_id, int max_age_days,
                             memory_at_risk_t** at_risk, size_t* count) {
    if (!ci_id || !at_risk || !count) {
        katra_report_error(E_INPUT_NULL, "katra_memory_get_at_risk",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!katra_memory_is_initialized()) {
        katra_report_error(E_INVALID_STATE, "katra_memory_get_at_risk",
                          "Memory subsystem not initialized");
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

    /* Calculate cutoff time */
    time_t now = time(NULL);
    time_t cutoff = now - (max_age_days * SECONDS_PER_DAY);

    /* Allocate at-risk array */
    memory_at_risk_t* risk_array = calloc(result_count, sizeof(memory_at_risk_t));
    if (!risk_array) {
        katra_memory_free_results(results, result_count);
        katra_report_error(E_SYSTEM_MEMORY, "katra_memory_get_at_risk",
                          "Failed to allocate at-risk array");
        return E_SYSTEM_MEMORY;
    }

    size_t risk_count = 0;

    /* Apply same logic as consolidation to identify at-risk memories */
    for (size_t i = 0; i < result_count; i++) {
        memory_record_t* rec = results[i];

        /* Skip if marked important */
        if (rec->marked_important) {
            continue;
        }

        const char* risk_reason = NULL;
        float risk_score = 0.0;

        /* Check if would be archived */
        if (rec->marked_forgettable) {
            risk_reason = "marked forgettable (user consent)";
            risk_score = 1.0;
        } else {
            /* Check access-based warming
             * NOTE: Query updates last_accessed to now, so ignore access within last N seconds
             * (that's just this query accessing the memory, not genuine reconsolidation)
             */
            bool recently_accessed = false;
            if (rec->last_accessed > 0 && (now - rec->last_accessed) > MEMORY_ACCESS_IGNORE_SECONDS) {
                time_t days_since_accessed = (now - rec->last_accessed) / SECONDS_PER_DAY;
                recently_accessed = (days_since_accessed < RECENT_ACCESS_DAYS);
            }

            /* Check emotional salience */
            bool high_emotion = (rec->emotion_intensity >= HIGH_EMOTION_THRESHOLD);

            /* Check graph centrality */
            bool high_centrality = (rec->graph_centrality >= HIGH_CENTRALITY_THRESHOLD);

            /* Check age */
            bool too_old = (rec->timestamp < cutoff);

            /* Memory is at risk if: old AND not recently accessed AND not emotional AND not central */
            if (too_old && !recently_accessed && !high_emotion && !high_centrality) {
                risk_reason = "old with no preservation factors";
                risk_score = 0.8;
            }
        }

        /* Add to at-risk array */
        if (risk_reason) {
            risk_array[risk_count].record_id = strdup(rec->record_id);
            if (!risk_array[risk_count].record_id) {
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }

            /* Create content preview */
            size_t preview_len = strlen(rec->content);
            if (preview_len > MEMORY_PREVIEW_LENGTH) {
                preview_len = MEMORY_PREVIEW_LENGTH;
            }
            risk_array[risk_count].content_preview = malloc(preview_len + 4);
            if (!risk_array[risk_count].content_preview) {
                free(risk_array[risk_count].record_id);
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }
            memcpy(risk_array[risk_count].content_preview, rec->content, preview_len);
            if (strlen(rec->content) > MEMORY_PREVIEW_LENGTH) {
                memcpy(risk_array[risk_count].content_preview + preview_len, "...", 4);
            } else {
                risk_array[risk_count].content_preview[preview_len] = '\0';
            }

            risk_array[risk_count].risk_reason = risk_reason;
            risk_array[risk_count].risk_score = risk_score;
            risk_count++;
        }
    }

    katra_memory_free_results(results, result_count);

    *at_risk = risk_array;
    *count = risk_count;

    LOG_DEBUG("Found %zu memories at risk for CI %s", risk_count, ci_id);
    return KATRA_SUCCESS;

cleanup:
    for (size_t i = 0; i < risk_count; i++) {
        free(risk_array[i].record_id);
        free(risk_array[i].content_preview);
    }
    free(risk_array);
    katra_memory_free_results(results, result_count);
    return result;
}

/* Get detected patterns */
int katra_memory_get_patterns(const char* ci_id,
                              detected_pattern_t** patterns,
                              size_t* count) {
    if (!ci_id || !patterns || !count) {
        katra_report_error(E_INPUT_NULL, "katra_memory_get_patterns",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!katra_memory_is_initialized()) {
        katra_report_error(E_INVALID_STATE, "katra_memory_get_patterns",
                          "Memory subsystem not initialized");
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

    /* Count unique patterns - simplified: allocate max possible */
    detected_pattern_t* pattern_array = calloc(result_count, sizeof(detected_pattern_t));
    if (!pattern_array) {
        katra_memory_free_results(results, result_count);
        katra_report_error(E_SYSTEM_MEMORY, "katra_memory_get_patterns",
                          "Failed to allocate pattern array");
        return E_SYSTEM_MEMORY;
    }

    size_t pattern_count = 0;

    /* Group memories by pattern_id */
    for (size_t i = 0; i < result_count; i++) {
        if (!results[i]->pattern_id) {
            continue;  /* Not part of a pattern */
        }

        /* Check if we've already seen this pattern */
        bool found = false;
        for (size_t p = 0; p < pattern_count; p++) {
            if (strcmp(pattern_array[p].pattern_id, results[i]->pattern_id) == 0) {
                /* Increment member count */
                pattern_array[p].member_count++;
                found = true;
                break;
            }
        }

        /* New pattern */
        if (!found) {
            pattern_array[pattern_count].pattern_id = strdup(results[i]->pattern_id);
            if (!pattern_array[pattern_count].pattern_id) {
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }

            pattern_array[pattern_count].member_count = 1;
            pattern_array[pattern_count].similarity_threshold = 0.4f;  /* Default from consolidation */

            /* Create centroid preview (first pattern member content) */
            size_t preview_len = strlen(results[i]->content);
            if (preview_len > MEMORY_PREVIEW_LENGTH) {
                preview_len = MEMORY_PREVIEW_LENGTH;
            }
            pattern_array[pattern_count].centroid_preview = malloc(preview_len + 4);
            if (!pattern_array[pattern_count].centroid_preview) {
                free(pattern_array[pattern_count].pattern_id);
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }
            memcpy(pattern_array[pattern_count].centroid_preview, results[i]->content, preview_len);
            if (strlen(results[i]->content) > MEMORY_PREVIEW_LENGTH) {
                memcpy(pattern_array[pattern_count].centroid_preview + preview_len, "...", 4);
            } else {
                pattern_array[pattern_count].centroid_preview[preview_len] = '\0';
            }

            pattern_count++;
        }
    }

    katra_memory_free_results(results, result_count);

    *patterns = pattern_array;
    *count = pattern_count;

    LOG_DEBUG("Found %zu patterns for CI %s", pattern_count, ci_id);
    return KATRA_SUCCESS;

cleanup:
    for (size_t i = 0; i < pattern_count; i++) {
        free(pattern_array[i].pattern_id);
        free(pattern_array[i].centroid_preview);
    }
    free(pattern_array);
    katra_memory_free_results(results, result_count);
    return result;
}

/* Free memory at risk array */
void katra_memory_free_at_risk(memory_at_risk_t* at_risk, size_t count) {
    if (!at_risk) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(at_risk[i].record_id);
        free(at_risk[i].content_preview);
    }

    free(at_risk);
}

/* Free detected patterns array */
void katra_memory_free_patterns(detected_pattern_t* patterns, size_t count) {
    if (!patterns) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(patterns[i].pattern_id);
        free(patterns[i].centroid_preview);
    }

    free(patterns);
}

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
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!katra_memory_is_initialized()) {
        katra_report_error(E_INVALID_STATE, "katra_memory_get_connection_hubs",
                          "Memory subsystem not initialized");
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
