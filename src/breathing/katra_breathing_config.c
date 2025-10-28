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
        global_config->max_relevant_memories = 10;
        global_config->max_recent_thoughts = 20;
        global_config->max_topic_recall = 100;
        global_config->min_importance_relevant = MEMORY_IMPORTANCE_HIGH;
        global_config->max_context_age_days = 7;
        LOG_INFO("Context configuration reset to defaults");
        return KATRA_SUCCESS;
    }

    /* Validate ranges */
    if (config->max_relevant_memories > 1000 ||
        config->max_recent_thoughts > 1000 ||
        config->max_topic_recall > 10000) {
        katra_report_error(E_INVALID_PARAMS, "set_context_config",
                          "Context limits too large");
        return E_INVALID_PARAMS;
    }

    if (config->min_importance_relevant < 0.0 ||
        config->min_importance_relevant > 1.0) {
        katra_report_error(E_INVALID_PARAMS, "set_context_config",
                          "Invalid importance threshold");
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
                          "Failed to allocate config");
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
                          "Failed to allocate stats");
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
