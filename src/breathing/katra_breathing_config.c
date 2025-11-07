/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_config.c - Configuration and statistics
 *
 * Context configuration, enhanced statistics, context helpers
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_breathing_internal.h"

/* ============================================================================
 * CONTEXT HELPERS
 * ============================================================================ */

memory_context_t* get_current_context(void) {
    if (!breathing_get_initialized()) {
        return NULL;
    }

    memory_context_t* ctx = calloc(1, sizeof(memory_context_t));
    if (!ctx) {
        return NULL;
    }

    const char* ci_id = breathing_get_ci_id();
    const char* session_id = breathing_get_session_id();

    ctx->ci_id = strdup(ci_id);
    ctx->session_id = session_id ? strdup(session_id) : NULL;
    ctx->when = time(NULL);
/* GUIDELINE_APPROVED: default context value */
    ctx->where = "breathing_layer";
    ctx->auto_captured = false;

    return ctx;
}

void free_context(memory_context_t* ctx) {
    if (!ctx) {
        return;
    }

    free(ctx->ci_id);
    free(ctx->session_id);
    free(ctx);
}

/* ============================================================================
 * CONTEXT CONFIGURATION
 * ============================================================================ */

int set_context_config(const context_config_t* config) {
    context_config_t* global_config = breathing_get_config_ptr();

    if (!config) {
        /* Reset to defaults */
        global_config->max_relevant_memories = BREATHING_DEFAULT_RELEVANT_MEMORIES;
        global_config->max_recent_thoughts = BREATHING_DEFAULT_RECENT_THOUGHTS;
        global_config->max_topic_recall = BREATHING_DEFAULT_TOPIC_RECALL;
        global_config->min_importance_relevant = MEMORY_IMPORTANCE_HIGH;
        global_config->max_context_age_days = BREATHING_DEFAULT_CONTEXT_AGE_DAYS;
        LOG_INFO("Context configuration reset to defaults");
        return KATRA_SUCCESS;
    }

    /* Validate ranges */
    if (config->max_relevant_memories > BREATHING_MAX_RELEVANT_LIMIT ||
        config->max_recent_thoughts > BREATHING_MAX_RECENT_LIMIT ||
        config->max_topic_recall > BREATHING_MAX_TOPIC_LIMIT) {
        katra_report_error(E_INVALID_PARAMS, "set_context_config",
                          KATRA_ERR_CONTEXT_LIMITS_TOO_LARGE);
        return E_INVALID_PARAMS;
    }

    if (config->min_importance_relevant < MEMORY_IMPORTANCE_TRIVIAL ||
        config->min_importance_relevant > MEMORY_IMPORTANCE_CRITICAL) {
        katra_report_error(E_INVALID_PARAMS, "set_context_config",
                          KATRA_ERR_INVALID_IMPORTANCE_THRESHOLD);
        return E_INVALID_PARAMS;
    }

    /* Apply configuration */
    *global_config = *config;

    LOG_INFO("Context configuration updated: relevant=%zu, recent=%zu, recall=%zu",
            config->max_relevant_memories,
            config->max_recent_thoughts,
            config->max_topic_recall);

    return KATRA_SUCCESS;
}

context_config_t* get_context_config(void) {
    context_config_t* config = malloc(sizeof(context_config_t));
    if (!config) {
        katra_report_error(E_SYSTEM_MEMORY, "get_context_config",
                          KATRA_ERR_ALLOC_FAILED);
        return NULL;
    }

    *config = *breathing_get_config_ptr();
    return config;
}

/* ============================================================================
 * ENHANCED STATISTICS
 * ============================================================================ */

enhanced_stats_t* get_enhanced_statistics(void) {
    if (!breathing_get_initialized()) {
        return NULL;
    }

    enhanced_stats_t* stats = malloc(sizeof(enhanced_stats_t));
    if (!stats) {
        katra_report_error(E_SYSTEM_MEMORY, "get_enhanced_statistics",
                          KATRA_ERR_ALLOC_FAILED);
        return NULL;
    }

    /* Copy current stats */
    *stats = *breathing_get_stats_ptr();

    /* Calculate session duration */
    enhanced_stats_t* global_stats = breathing_get_stats_ptr();
    if (global_stats->session_start_time > 0) {
        stats->session_duration_seconds =
            (size_t)(time(NULL) - global_stats->session_start_time);
    }

    return stats;
}

int reset_session_statistics(void) {
    LOG_DEBUG("Resetting session statistics");

    enhanced_stats_t* stats = breathing_get_stats_ptr();

    /* Clear all counters but preserve session start time */
    time_t start_time = stats->session_start_time;

    memset(stats, 0, sizeof(enhanced_stats_t));

    /* Restore or set session start time */
    stats->session_start_time = start_time > 0 ? start_time : time(NULL);

    return KATRA_SUCCESS;
}
