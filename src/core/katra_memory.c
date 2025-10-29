/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

/* Project includes */
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_tier2.h"
#include "katra_consent.h"
#include "katra_core_common.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_env_utils.h"

/* Global state */
static bool memory_initialized = false;
static bool tier2_enabled = false;
static char current_ci_id[KATRA_BUFFER_MEDIUM];

/* Initialize memory subsystem */
int katra_memory_init(const char* ci_id) {
    int result = KATRA_SUCCESS;

    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_memory_init", "ci_id is NULL");
        return E_INPUT_NULL;
    }

    if (memory_initialized) {
        LOG_DEBUG("Memory subsystem already initialized");
        return KATRA_SUCCESS;
    }

    LOG_INFO("Initializing memory subsystem for CI: %s", ci_id);

    /* Store CI ID */
    SAFE_STRNCPY(current_ci_id, ci_id);

    /* Initialize consent system */
    result = katra_consent_init();
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_memory_init", "Consent init failed");
        return result;
    }

    /* Set consent context to this CI */
    result = katra_consent_set_context(ci_id);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_memory_init", "Failed to set consent context");
        return result;
    }

    /* Initialize Tier 1 (raw recordings) */
    result = tier1_init(ci_id);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_memory_init", "Tier 1 init failed");
        return result;
    }

    /* Initialize Tier 2 (sleep digests) */
    result = tier2_init(ci_id);
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Tier 2 initialization failed: %d (archiving disabled)", result);
        tier2_enabled = false;
        /* Non-fatal - tier1 will just grow larger without archiving */
    } else {
        LOG_INFO("Tier 2 initialized successfully");
        tier2_enabled = true;
    }

    /* TODO: Initialize Tier 3 (pattern summaries) - Phase 2.3 */

    memory_initialized = true;
    LOG_INFO("Memory subsystem initialized successfully");

    return KATRA_SUCCESS;
}

/* Cleanup memory subsystem */
void katra_memory_cleanup(void) {
    if (!memory_initialized) {
        return;
    }

    LOG_DEBUG("Cleaning up memory subsystem");

    /* Cleanup all tiers in reverse order */
    if (tier2_enabled) {
        tier2_cleanup();
        tier2_enabled = false;
    }
    tier1_cleanup();
    /* TODO: tier3_cleanup() - Phase 2.3 */

    /* Cleanup consent system */
    katra_consent_cleanup();

    memory_initialized = false;
    current_ci_id[0] = '\0';
}

/* Store memory record */
int katra_memory_store(const memory_record_t* record) {
    if (!record) {
        katra_report_error(E_INPUT_NULL, "katra_memory_store", "record is NULL");
        return E_INPUT_NULL;
    }

    if (!memory_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_memory_store",
                          "Memory subsystem not initialized");
        return E_INVALID_STATE;
    }

    /* Validate record */
    if (!record->ci_id || !record->content) {
        katra_report_error(E_INPUT_NULL, "katra_memory_store",
                          "Record missing required fields");
        return E_INPUT_NULL;
    }

    /* Validate importance range */
    if (record->importance < 0.0 || record->importance > 1.0) {
        katra_report_error(E_INPUT_RANGE, "katra_memory_store",
                          "Importance must be 0.0-1.0");
        return E_INPUT_RANGE;
    }

    LOG_DEBUG("Storing memory record: type=%d, importance=%.2f",
              record->type, record->importance);

    /* Route to appropriate tier */
    int result = KATRA_SUCCESS;

    switch (record->tier) {
        case KATRA_TIER1:
            result = tier1_store(record);
            break;

        case KATRA_TIER2:
            /* TODO: tier2_store(record) - Phase 2.2 */
            katra_report_error(E_INTERNAL_NOTIMPL, "katra_memory_store",
                              "Tier 2 not yet implemented");
            result = E_INTERNAL_NOTIMPL;
            break;

        case KATRA_TIER3:
            /* TODO: tier3_store(record) - Phase 2.3 */
            katra_report_error(E_INTERNAL_NOTIMPL, "katra_memory_store",
                              "Tier 3 not yet implemented");
            result = E_INTERNAL_NOTIMPL;
            break;

        default:
            katra_report_error(E_INPUT_INVALID, "katra_memory_store",
                              "Invalid tier specified");
            result = E_INPUT_INVALID;
            break;
    }

    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Memory record stored successfully");
    }

    return result;
}

/* Query memory records */
int katra_memory_query(const memory_query_t* query,
                       memory_record_t*** results,
                       size_t* count) {
    if (!query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "katra_memory_query",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!query->ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_memory_query",
                          "query->ci_id is NULL");
        return E_INPUT_NULL;
    }

    if (!memory_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_memory_query",
                          "Memory subsystem not initialized");
        return E_INVALID_STATE;
    }

    /* Check consent - can current CI access target CI's memories? */
    int consent_result = katra_consent_check_current(query->ci_id);
    if (consent_result != KATRA_SUCCESS) {
        return consent_result;  /* Returns E_CONSENT_REQUIRED if blocked */
    }

    *results = NULL;
    *count = 0;

    LOG_DEBUG("Querying memory: ci=%s, tier=%d", query->ci_id, query->tier);

    /* Query appropriate tier(s) */
    int result = KATRA_SUCCESS;

    if (query->tier == KATRA_TIER1 || query->tier == 0) {
        /* Query Tier 1 */
        result = tier1_query(query, results, count);
        if (result != KATRA_SUCCESS) {
            return result;
        }
    }

    /* TODO: Query Tier 2 if requested - Phase 2.2 */
    /* TODO: Query Tier 3 if requested - Phase 2.3 */

    /* Update access tracking for all retrieved memories (Thane's reconsolidation) */
    time_t now = time(NULL);
    for (size_t i = 0; i < *count; i++) {
        if ((*results)[i]) {
            (*results)[i]->last_accessed = now;
            (*results)[i]->access_count++;
        }
    }

    LOG_DEBUG("Query returned %zu results", *count);
    return KATRA_SUCCESS;
}

/* Get memory statistics */
int katra_memory_stats(const char* ci_id, memory_stats_t* stats) {
    if (!ci_id || !stats) {
        katra_report_error(E_INPUT_NULL, "katra_memory_stats",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!memory_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_memory_stats",
                          "Memory subsystem not initialized");
        return E_INVALID_STATE;
    }

    /* Check consent */
    int consent_result = katra_consent_check_current(ci_id);
    if (consent_result != KATRA_SUCCESS) {
        return consent_result;
    }

    /* Initialize stats */
    memset(stats, 0, sizeof(memory_stats_t));

    /* Get Tier 1 stats */
    size_t tier1_records = 0;
    size_t tier1_bytes = 0;
    int result = tier1_stats(ci_id, &tier1_records, &tier1_bytes);
    if (result == KATRA_SUCCESS) {
        stats->tier1_records = tier1_records;
        stats->bytes_used += tier1_bytes;
    }

    /* TODO: Get Tier 2 stats - Phase 2.2 */
    /* TODO: Get Tier 3 stats - Phase 2.3 */

    stats->total_records = stats->tier1_records +
                          stats->tier2_records +
                          stats->tier3_records;

    LOG_DEBUG("Memory stats: total=%zu, tier1=%zu, bytes=%zu",
              stats->total_records, stats->tier1_records, stats->bytes_used);

    return KATRA_SUCCESS;
}

/* Archive old memories */
int katra_memory_archive(const char* ci_id, int max_age_days, size_t* archived_count) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_memory_archive",
                          "ci_id is NULL");
        return E_INPUT_NULL;
    }

    if (!memory_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_memory_archive",
                          "Memory subsystem not initialized");
        return E_INVALID_STATE;
    }

    /* Check consent */
    int consent_result = katra_consent_check_current(ci_id);
    if (consent_result != KATRA_SUCCESS) {
        return consent_result;
    }

    LOG_INFO("Archiving memories older than %d days for CI: %s",
             max_age_days, ci_id);

    /* Archive Tier 1 → Tier 2 */
    int archived = tier1_archive(ci_id, max_age_days);

    if (archived < 0) {
        /* tier1_archive returned error code */
        return archived;
    }

    /* Success - set output parameter if provided */
    if (archived_count) {
        *archived_count = (size_t)archived;
    }

    LOG_INFO("Archived %zu memory records", (size_t)archived);
    return KATRA_SUCCESS;
}

/* Create memory record (helper) */
memory_record_t* katra_memory_create_record(const char* ci_id,
                                             memory_type_t type,
                                             const char* content,
                                             float importance) {
    if (!ci_id || !content) {
        return NULL;
    }

    memory_record_t* record;
    ALLOC_OR_RETURN_NULL(record, memory_record_t);

    /* Generate unique ID: ci_id_timestamp_random */
    char id_buffer[KATRA_BUFFER_MEDIUM];
    snprintf(id_buffer, sizeof(id_buffer), "%s_%ld_%d",
            ci_id, (long)time(NULL), rand() % 10000);

    /* GUIDELINE_APPROVED - Aggregate NULL check pattern */
    record->record_id = strdup(id_buffer);
    record->ci_id = strdup(ci_id);
    record->content = strdup(content);

    if (!record->record_id || !record->ci_id || !record->content) {
    /* GUIDELINE_APPROVED_END */
        katra_memory_free_record(record);
        katra_report_error(E_SYSTEM_MEMORY, "katra_memory_create_record",
                          "Failed to allocate strings");
        return NULL;
    }

    record->timestamp = time(NULL);
    record->type = type;
    record->importance = importance;
    record->importance_note = NULL;  /* Optional field - set by caller if needed */
    record->tier = KATRA_TIER1;  /* Default to Tier 1 */
    record->archived = false;

    /* Initialize Thane's Phase 1 fields */
    record->last_accessed = 0;          /* Not yet accessed */
    record->access_count = 0;           /* No accesses yet */
    record->emotion_intensity = 0.0;    /* No emotion detected yet */
    record->emotion_type = NULL;        /* No emotion type */
    record->marked_important = false;   /* Not marked important */
    record->marked_forgettable = false; /* Not marked forgettable */

    /* Initialize Thane's Phase 2 fields */
    record->connected_memory_ids = NULL; /* No connections yet */
    record->connection_count = 0;        /* No connections */
    record->graph_centrality = 0.0;      /* Not yet calculated */

    /* Initialize Thane's Phase 3 fields */
    record->pattern_id = NULL;           /* Not part of pattern yet */
    record->pattern_frequency = 0;       /* No pattern frequency */
    record->is_pattern_outlier = false;  /* Not an outlier */
    record->semantic_similarity = 0.0;   /* No similarity calculated */

    /* Initialize Thane's Phase 4 fields - formation context */
    record->context_question = NULL;     /* No formation question */
    record->context_resolution = NULL;   /* No resolution */
    record->context_uncertainty = NULL;  /* No uncertainty */
    record->related_to = NULL;           /* Not related to another memory */

    return record;
}

/* Create memory record with formation context (Thane's active sense-making) */
memory_record_t* katra_memory_create_with_context(
    const char* ci_id,
    memory_type_t type,
    const char* content,
    float importance,
    const char* context_question,
    const char* context_resolution,
    const char* context_uncertainty,
    const char* related_to) {

    /* Create base record using existing function */
    memory_record_t* record = katra_memory_create_record(ci_id, type, content, importance);
    if (!record) {
        return NULL;
    }

    int result = KATRA_SUCCESS;

    /* Add formation context fields (all optional) */
    if (context_question) {
        record->context_question = strdup(context_question);
        if (!record->context_question) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    if (context_resolution) {
        record->context_resolution = strdup(context_resolution);
        if (!record->context_resolution) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    if (context_uncertainty) {
        record->context_uncertainty = strdup(context_uncertainty);
        if (!record->context_uncertainty) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    if (related_to) {
        record->related_to = strdup(related_to);
        if (!record->related_to) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    return record;

cleanup:
    katra_memory_free_record(record);
    katra_report_error(result, "katra_memory_create_with_context",
                      "Failed to allocate context fields");
    return NULL;
}

/* Free memory record */
void katra_memory_free_record(memory_record_t* record) {
    if (!record) {
        return;
    }

    free(record->record_id);
    free(record->ci_id);
    free(record->content);
    free(record->response);
    free(record->context);
    free(record->session_id);
    free(record->component);
    free(record->importance_note);
    free(record->emotion_type);

    /* Free Phase 2 connection array */
    if (record->connected_memory_ids) {
        for (size_t i = 0; i < record->connection_count; i++) {
            free(record->connected_memory_ids[i]);
        }
        free(record->connected_memory_ids);
    }

    /* Free Phase 3 pattern fields */
    free(record->pattern_id);

    /* Free Phase 4 formation context fields */
    free(record->context_question);
    free(record->context_resolution);
    free(record->context_uncertainty);
    free(record->related_to);

    free(record);
}

/* Free memory query results */
void katra_memory_free_results(memory_record_t** results, size_t count) {
    if (!results) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        katra_memory_free_record(results[i]);
    }

    free(results);
}

/* Check if tier2 is enabled */
bool katra_memory_tier2_enabled(void) {
    return tier2_enabled;
}

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

    if (!memory_initialized) {
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
        .limit = 10000  /* High limit to get all active memories */
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

    /* Determine if consolidation recommended (threshold: 100 active memories) */
    health->consolidation_recommended = (health->active_memories >= 100);

    /* Determine health status */
    if (health->active_memories < 50) {
        health->health_status = "healthy";
    } else if (health->active_memories < 200) {
        health->health_status = "degraded";
    } else {
        health->health_status = "critical";
    }

    LOG_DEBUG("Memory health: total=%zu, active=%zu, archived=%zu, compression=%.1f%%, status=%s",
              health->total_memories, health->active_memories, health->archived_memories,
              health->compression_ratio * 100.0, health->health_status);

    return KATRA_SUCCESS;
}

/* Consolidation thresholds (match katra_tier1_archive.c) */
#define RECENT_ACCESS_DAYS 7
#define HIGH_EMOTION_THRESHOLD 0.7f
#define HIGH_CENTRALITY_THRESHOLD 0.5f

/* Get memories at risk of archival */
int katra_memory_get_at_risk(const char* ci_id, int max_age_days,
                             memory_at_risk_t** at_risk, size_t* count) {
    if (!ci_id || !at_risk || !count) {
        katra_report_error(E_INPUT_NULL, "katra_memory_get_at_risk",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!memory_initialized) {
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
        .limit = 10000
    };
    memory_record_t** results = NULL;
    size_t result_count = 0;
    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Calculate cutoff time */
    time_t now = time(NULL);
    time_t cutoff = now - (max_age_days * 24 * 3600);

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
            /* Check access-based warming */
            bool recently_accessed = false;
            if (rec->last_accessed > 0) {
                time_t days_since_accessed = (now - rec->last_accessed) / (24 * 3600);
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

            /* Create content preview (first 100 chars) */
            size_t preview_len = strlen(rec->content);
            if (preview_len > 100) preview_len = 100;
            risk_array[risk_count].content_preview = malloc(preview_len + 4);
            if (!risk_array[risk_count].content_preview) {
                free(risk_array[risk_count].record_id);
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }
            memcpy(risk_array[risk_count].content_preview, rec->content, preview_len);
            if (strlen(rec->content) > 100) {
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

    if (!memory_initialized) {
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
        .limit = 10000
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
            if (preview_len > 100) preview_len = 100;
            pattern_array[pattern_count].centroid_preview = malloc(preview_len + 4);
            if (!pattern_array[pattern_count].centroid_preview) {
                free(pattern_array[pattern_count].pattern_id);
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }
            memcpy(pattern_array[pattern_count].centroid_preview, results[i]->content, preview_len);
            if (strlen(results[i]->content) > 100) {
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
