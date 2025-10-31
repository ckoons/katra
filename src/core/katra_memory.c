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
            ci_id, (long)time(NULL), rand() % MEMORY_ID_RANDOM_MAX);

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
    record->pattern_summary = NULL;      /* No pattern context yet */

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
    katra_free_string_array(record->connected_memory_ids, record->connection_count);

    /* Free Phase 3 pattern fields */
    free(record->pattern_id);
    free(record->pattern_summary);

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

/* Check if memory subsystem is initialized (accessor for metacognitive module) */
bool katra_memory_is_initialized(void) {
    return memory_initialized;
}
