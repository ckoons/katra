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
#include "katra_breathing_internal.h"

/* ============================================================================
 * GLOBAL STATE - Shared across breathing layer files
 * ============================================================================ */

/* Global breathing context */
static memory_context_t g_context = {0};
static bool g_initialized = false;
static char* g_current_thought = NULL;  /* For mark_significant() */

/* Turn tracking for end-of-turn reflection (shared with katra_breathing_reflection.c) */
int g_current_turn = 0;          /* Current turn number */
char** g_turn_memory_ids = NULL;  /* Memory IDs from current turn */
size_t g_turn_memory_count = 0;   /* Count of memories this turn */
size_t g_turn_memory_capacity = 0; /* Capacity of array */

/* Global context configuration (defaults) */
static context_config_t g_context_config = {
    .max_relevant_memories = BREATHING_DEFAULT_RELEVANT_MEMORIES,
    .max_recent_thoughts = BREATHING_DEFAULT_RECENT_THOUGHTS,
    .max_topic_recall = BREATHING_DEFAULT_TOPIC_RECALL,
    .min_importance_relevant = MEMORY_IMPORTANCE_HIGH,
    .max_context_age_days = BREATHING_DEFAULT_CONTEXT_AGE_DAYS
};

/* Global enhanced statistics */
static enhanced_stats_t g_enhanced_stats = {0};

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int breathe_init(const char* ci_id) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "breathe_init", KATRA_ERR_CI_ID_NULL);
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
        katra_report_error(E_SYSTEM_MEMORY, "breathe_init", KATRA_ERR_ALLOC_FAILED);
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

    LOG_DEBUG("Breathing layer cleanup started for %s", g_context.ci_id);

    /* ========================================================================
     * FORMALIZED CLEANUP ORDER
     * ======================================================================== */

    /* Step 1: Stop forming new memories */
    g_initialized = false;
    LOG_DEBUG("Step 1: Stopped accepting new memories");

    /* Step 2: Consolidate existing memories BEFORE cleanup */
    auto_consolidate();
    LOG_DEBUG("Step 2: Consolidated memories");

    /* Step 3: Cleanup subsystems in reverse init order */
    /* (Future: tier2_cleanup(), tier3_cleanup() would go here) */
    LOG_DEBUG("Step 3: Subsystems cleaned up");

    /* Step 4: Cleanup memory subsystem (closes databases) */
    katra_memory_cleanup();
    LOG_DEBUG("Step 4: Memory subsystem cleaned up");

    /* Step 5: Free breathing layer resources */
    free(g_context.ci_id);
    free(g_context.session_id);
    free(g_current_thought);
    cleanup_turn_tracking();  /* Clean up turn tracking */

    memset(&g_context, 0, sizeof(g_context));
    g_current_thought = NULL;
    LOG_DEBUG("Step 5: Breathing layer resources freed");

    LOG_INFO("Breathing layer cleanup complete");
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

    /* Start first turn */
    begin_turn();

    LOG_INFO("Session started: %s", g_context.session_id);

    /* Run periodic maintenance (consolidation if needed) */
    result = breathe_periodic_maintenance();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Periodic maintenance failed: %d", result);
        /* Non-fatal - continue session start */
    }

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

/* ============================================================================
 * SESSION INFO API
 * ============================================================================ */

int katra_get_session_info(katra_session_info_t* info) {
    KATRA_CHECK_NULL(info);

    /* Clear structure */
    memset(info, 0, sizeof(katra_session_info_t));

    /* Check if session is active */
    if (!g_initialized) {
        return E_INVALID_STATE;
    }

    /* Copy session identity */
    if (g_context.ci_id) {
        strncpy(info->ci_id, g_context.ci_id, sizeof(info->ci_id) - 1);
        info->ci_id[sizeof(info->ci_id) - 1] = '\0';
    }

    if (g_context.session_id) {
        strncpy(info->session_id, g_context.session_id, sizeof(info->session_id) - 1);
        info->session_id[sizeof(info->session_id) - 1] = '\0';
    }

    /* Session state */
    info->is_active = g_initialized;
    info->start_time = g_enhanced_stats.session_start_time;
    info->last_activity = g_enhanced_stats.last_activity_time;

    /* Session metrics */
    info->memories_added = g_enhanced_stats.total_memories_stored;
    info->queries_processed = g_enhanced_stats.relevant_queries +
                              g_enhanced_stats.recent_queries +
                              g_enhanced_stats.topic_queries;

    return KATRA_SUCCESS;
}
