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
#include "katra_graph.h"  /* Phase 6.2: Graph auto-edges */

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

        /* Semantic search defaults (Phase 6.1f) */
        global_config->use_semantic_search = true;  /* Enabled by default (Phase 6.1f) */
        global_config->semantic_threshold = SEMANTIC_DEFAULT_THRESHOLD;  /* 30% similarity */
        global_config->max_semantic_results = SEMANTIC_DEFAULT_MAX_RESULTS;
        global_config->embedding_method = 1;          /* EMBEDDING_TFIDF (good balance) */

        /* Graph auto-edges defaults (Phase 6.2) */
        global_config->auto_graph_edges = true;  /* Enabled by default - builds memory associations */
        global_config->graph_similarity_threshold = GRAPH_DEFAULT_SIMILARITY_THRESHOLD;  /* 50% */
        global_config->graph_max_similar_edges = 5;  /* Limit to top 5 most similar memories */
        global_config->graph_temporal_window_sec = GRAPH_DEFAULT_TEMPORAL_WINDOW_SEC;  /* 5 min */

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

    /* Handle auto_graph_edges state change (Phase 6.2) */
    bool was_enabled = global_config->auto_graph_edges;
    bool now_enabled = config->auto_graph_edges;

    if (was_enabled && !now_enabled) {
        /* Disabling auto-edges - cleanup graph store */
        graph_store_t* graph_store = breathing_get_graph_store();
        if (graph_store) {
            katra_graph_cleanup(graph_store);
            breathing_set_graph_store(NULL);
            LOG_INFO("Graph store disabled and cleaned up");
        }
    } else if (!was_enabled && now_enabled) {
        /* Enabling auto-edges - initialize graph store */
        const char* ci_id = breathing_get_ci_id();
        if (ci_id) {
            graph_store_t* new_graph_store = katra_graph_init(ci_id);
            breathing_set_graph_store(new_graph_store);
            if (new_graph_store) {
                LOG_INFO("Graph store initialized for automatic edge creation");
            } else {
                LOG_WARN("Graph store init failed (continuing without auto-edges)");
            }
        }
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

/* ============================================================================
 * SEMANTIC SEARCH CONFIGURATION (Phase 6.1f)
 * ============================================================================ */

int enable_semantic_search(bool enable) {
    context_config_t* config = breathing_get_config_ptr();
    config->use_semantic_search = enable;

    LOG_INFO("Semantic search %s", enable ? "enabled" : "disabled");
    return KATRA_SUCCESS;
}

int set_semantic_threshold(float threshold) {
    if (threshold < 0.0f || threshold > 1.0f) {
        katra_report_error(E_INVALID_PARAMS, "set_semantic_threshold",
                          "Threshold must be between 0.0 and 1.0");
        return E_INVALID_PARAMS;
    }

    context_config_t* config = breathing_get_config_ptr();
    config->semantic_threshold = threshold;

    LOG_INFO("Semantic threshold set to %.2f", threshold);
    return KATRA_SUCCESS;
}

int set_embedding_method(int method) {
    if (method < 0 || method > 2) {
        katra_report_error(E_INVALID_PARAMS, "set_embedding_method",
                          "Method must be 0=HASH, 1=TFIDF, 2=EXTERNAL");
        return E_INVALID_PARAMS;
    }

    context_config_t* config = breathing_get_config_ptr();
    config->embedding_method = method;

    const char* method_names[] = {"HASH", "TFIDF", "EXTERNAL"};
    LOG_INFO("Embedding method set to %s", method_names[method]);
    return KATRA_SUCCESS;
}
