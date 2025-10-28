/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing.c - Core breathing layer state and lifecycle
 *
 * This file maintains global state and lifecycle (init/cleanup/session).
 * Functionality is split across multiple files:
 * - katra_breathing_primitives.c: remember, learn, reflect, decide, notice_pattern
 * - katra_breathing_semantic.c: semantic reason parsing
 * - katra_breathing_context.c: relevant_memories, recent_thoughts, recall_about
 * - katra_breathing_config.c: configuration and statistics
 * - katra_breathing_interstitial.c: auto-capture and consolidation
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_continuity.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* ============================================================================
 * GLOBAL STATE - Shared across breathing layer files
 * ============================================================================ */

/* Global breathing context */
static memory_context_t g_context = {0};
static bool g_initialized = false;
static char* g_current_thought = NULL;  /* For mark_significant() */

/* Global context configuration (defaults) */
static context_config_t g_context_config = {
    .max_relevant_memories = 10,
    .max_recent_thoughts = 20,
    .max_topic_recall = 100,
    .min_importance_relevant = MEMORY_IMPORTANCE_HIGH,
    .max_context_age_days = 7
};

/* Global enhanced statistics */
static enhanced_stats_t g_enhanced_stats = {0};

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int breathe_init(const char* ci_id) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "breathe_init", "ci_id is NULL");
        return E_INPUT_NULL;
    }

    if (g_initialized) {
        LOG_DEBUG("Breathing layer already initialized for %s", g_context.ci_id);
        return KATRA_SUCCESS;
    }

    /* Initialize memory subsystem */
    int result = katra_memory_init(ci_id);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Setup context */
    g_context.ci_id = strdup(ci_id);
    if (!g_context.ci_id) {
        katra_report_error(E_SYSTEM_MEMORY, "breathe_init", "Failed to allocate ci_id");
        return E_SYSTEM_MEMORY;
    }

    g_context.session_id = NULL;  /* Set by session_start() */
    g_context.when = time(NULL);
    g_context.where = "breathing_layer";
    g_context.auto_captured = false;

    g_initialized = true;
    LOG_INFO("Breathing layer initialized for CI: %s", ci_id);

    return KATRA_SUCCESS;
}

void breathe_cleanup(void) {
    if (!g_initialized) {
        return;
    }

    LOG_DEBUG("Cleaning up breathing layer for %s", g_context.ci_id);

    /* Auto-consolidate before shutdown */
    auto_consolidate();

    /* Cleanup context */
    free(g_context.ci_id);
    free(g_context.session_id);
    free(g_current_thought);

    memset(&g_context, 0, sizeof(g_context));
    g_initialized = false;
    g_current_thought = NULL;

    /* Cleanup memory subsystem */
    katra_memory_cleanup();
}

/* ============================================================================
 * SESSION MANAGEMENT
 * ============================================================================ */

int session_start(const char* ci_id) {
    int result = breathe_init(ci_id);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Generate session ID */
    char session_id[KATRA_BUFFER_MEDIUM];
    snprintf(session_id, sizeof(session_id), "%s_%ld",
             ci_id, (long)time(NULL));

    g_context.session_id = strdup(session_id);
    if (!g_context.session_id) {
        return E_SYSTEM_MEMORY;
    }

    /* Reset session statistics */
    reset_session_statistics();

    LOG_INFO("Session started: %s", g_context.session_id);

    /* Load yesterday's summary (sunrise) */
    digest_record_t* yesterday = NULL;
    result = katra_sunrise_basic(ci_id, &yesterday);

    if (result == KATRA_SUCCESS && yesterday) {
        LOG_INFO("Yesterday's summary: %s", yesterday->summary);
        katra_digest_free(yesterday);
    }

    /* Load relevant context */
    load_context();

    return KATRA_SUCCESS;
}

int session_end(void) {
    if (!g_initialized) {
        return E_INVALID_STATE;
    }

    LOG_INFO("Ending session: %s", g_context.session_id);

    /* Create daily summary (sunset) */
    int result = katra_sundown_basic(g_context.ci_id, NULL);
    if (result == KATRA_SUCCESS) {
        LOG_INFO("Daily summary created");
    }

    /* Auto-consolidate */
    auto_consolidate();

    return result;
}

/* ============================================================================
 * INTERNAL ACCESSORS - For Level 3 integration (katra_breathing_integration.c)
 * ============================================================================ */

bool katra_breathing_is_initialized(void) {
    return g_initialized;
}

const char* katra_breathing_get_ci_id(void) {
    return g_initialized ? g_context.ci_id : NULL;
}

/* ============================================================================
 * INTERNAL ACCESSORS - For breathing layer split files
 * ============================================================================ */

bool breathing_get_initialized(void) {
    return g_initialized;
}

const char* breathing_get_ci_id(void) {
    return g_initialized ? g_context.ci_id : NULL;
}

const char* breathing_get_session_id(void) {
    return g_initialized ? g_context.session_id : NULL;
}

context_config_t* breathing_get_config_ptr(void) {
    return &g_context_config;
}

enhanced_stats_t* breathing_get_stats_ptr(void) {
    return &g_enhanced_stats;
}

void breathing_track_memory_stored(memory_type_t type, why_remember_t importance) {
    g_enhanced_stats.total_memories_stored++;
    g_enhanced_stats.by_type[type]++;
    g_enhanced_stats.by_importance[importance]++;
    g_enhanced_stats.last_activity_time = time(NULL);
}

void breathing_track_semantic_remember(why_remember_t importance) {
    g_enhanced_stats.total_memories_stored++;
    g_enhanced_stats.semantic_remember_count++;
    g_enhanced_stats.by_type[MEMORY_TYPE_EXPERIENCE]++;
    g_enhanced_stats.by_importance[importance]++;
    g_enhanced_stats.last_activity_time = time(NULL);
}

void breathing_track_relevant_query(void) {
    g_enhanced_stats.relevant_queries++;
    g_enhanced_stats.last_activity_time = time(NULL);
}

void breathing_track_recent_query(void) {
    g_enhanced_stats.recent_queries++;
    g_enhanced_stats.last_activity_time = time(NULL);
}

void breathing_track_topic_query(size_t match_count) {
    g_enhanced_stats.topic_queries++;
    g_enhanced_stats.topic_matches += match_count;
    g_enhanced_stats.last_activity_time = time(NULL);
}

void breathing_track_context_load(size_t memory_count) {
    g_enhanced_stats.context_loads++;

    if (memory_count > g_enhanced_stats.max_context_size) {
        g_enhanced_stats.max_context_size = memory_count;
    }

    size_t total_loads = g_enhanced_stats.context_loads;
    g_enhanced_stats.avg_context_size =
        ((g_enhanced_stats.avg_context_size * (total_loads - 1)) + memory_count) / total_loads;
}
