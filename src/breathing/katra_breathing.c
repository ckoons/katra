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
#include "katra_breathing_context_persist.h"
#include "katra_meeting.h"
#include "katra_graph.h"  /* Phase 6.2: Graph auto-edges */
#include "katra_vector.h"  /* Phase 6.1f: semantic search */

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
    .max_context_age_days = BREATHING_DEFAULT_CONTEXT_AGE_DAYS,
    /* Semantic search defaults (Phase 6.1f) */
    .use_semantic_search = true,       /* Enabled by default - core feature */
    .semantic_threshold = 0.3f,        /* 30% similarity - Ami's testing shows optimal balance */
    .max_semantic_results = 20,        /* Reasonable limit */
    .embedding_method = 1,             /* EMBEDDING_TFIDF */
    /* Graph auto-edges defaults (Phase 6.2) */
    .auto_graph_edges = true,          /* Enabled by default - builds memory associations */
    .graph_similarity_threshold = 0.5f,  /* 50% similarity for SIMILAR edges - stricter than recall */
    .graph_max_similar_edges = 5,      /* Limit to top 5 most similar memories */
    .graph_temporal_window_sec = 300,  /* 5 minutes for SEQUENTIAL edge detection */
    /* Working memory budget defaults (Phase 2) */
    .working_memory_enabled = WORKING_MEMORY_DEFAULT_ENABLED,  /* Enabled by default */
    .working_memory_soft_limit = WORKING_MEMORY_SOFT_LIMIT,    /* Archive at 35 */
    .working_memory_hard_limit = WORKING_MEMORY_HARD_LIMIT,    /* Delete at 50 */
    .working_memory_batch_size = WORKING_MEMORY_BATCH_SIZE     /* Process 10 at a time */
};

/* Global enhanced statistics */
static enhanced_stats_t g_enhanced_stats = {0};

/* Global vector store for semantic search (Phase 6.1f) */
static vector_store_t* g_vector_store = NULL;

/* Global graph store for memory associations (Phase 6.2) */
static graph_store_t* g_graph_store = NULL;

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
/* GUIDELINE_APPROVED: default context value */
    g_context.where = "breathing_layer";
    g_context.auto_captured = false;

    g_initialized = true;
    LOG_INFO("Breathing layer initialized for CI: %s", ci_id);

    /* Initialize context persistence subsystem */
    result = context_persist_init(ci_id);
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Context persistence init failed: %d (continuing without it)", result);
        /* Non-fatal - continue without context persistence */
    }

    /* Initialize vector store if semantic search is enabled (Phase 6.1f) */
    if (g_context_config.use_semantic_search) {
        g_vector_store = katra_vector_init(ci_id, false);  /* false = use local, not external */
        if (g_vector_store) {
            /* Set embedding method from config */
            g_vector_store->method = (embedding_method_t)g_context_config.embedding_method;

            /* Initialize persistence for vector store */
            result = katra_vector_persist_init(ci_id);
            if (result == KATRA_SUCCESS) {
                /* Load previously stored embeddings */
                katra_vector_persist_load(ci_id, g_vector_store);
                LOG_INFO("Vector store initialized with %zu embeddings", g_vector_store->count);
            }
        } else {
            LOG_WARN("Vector store init failed (continuing without semantic search)");
        }
    }

    /* Initialize graph store if auto-edges are enabled (Phase 6.2) */
    if (g_context_config.auto_graph_edges) {
        g_graph_store = katra_graph_init(ci_id);
        if (g_graph_store) {
            LOG_INFO("Graph store initialized for automatic edge creation");
        } else {
            LOG_WARN("Graph store init failed (continuing without auto-edges)");
        }
    }

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

    /* Step 5: Cleanup context persistence */
    context_persist_cleanup();
    LOG_DEBUG("Step 5: Context persistence cleaned up");

    /* Step 5.5: Cleanup vector store (Phase 6.1f) */
    if (g_vector_store) {
        katra_vector_cleanup(g_vector_store);
        g_vector_store = NULL;
        LOG_DEBUG("Step 5.5: Vector store cleaned up");
    }

    /* Step 5.6: Cleanup graph store (Phase 6.2) */
    if (g_graph_store) {
        katra_graph_cleanup(g_graph_store);
        g_graph_store = NULL;
        LOG_DEBUG("Step 5.6: Graph store cleaned up");
    }

    /* Step 6: Free breathing layer resources */
    free(g_context.ci_id);
    free(g_context.session_id);
    free(g_current_thought);
    cleanup_turn_tracking();  /* Clean up turn tracking */

    memset(&g_context, 0, sizeof(g_context));
    g_current_thought = NULL;
    LOG_DEBUG("Step 6: Breathing layer resources freed");

    /* Step 7: Reset configuration to defaults */
    set_context_config(NULL);  /* NULL resets to defaults */
    LOG_DEBUG("Step 7: Configuration reset to defaults");

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

    /* Restore context snapshot (latent space for session startup) */
    char* latent_space = restore_context_as_latent_space(ci_id);
    if (latent_space) {
        LOG_INFO("Restored context snapshot (%zu bytes latent space)", strlen(latent_space));
        /* TODO: Integrate latent space with system prompt in future MCP enhancement */
        free(latent_space);
    } else {
        LOG_DEBUG("No previous context snapshot found for %s", ci_id);
    }

    /* Load relevant context */
    load_context();

    /* Autonomic function: Check for waiting messages (awareness only, don't consume) */
    /* TODO: Implement katra_count_messages() API for non-consuming message count */
    /* For now, log that session is ready to receive messages */
    LOG_DEBUG("Session start complete - ready to receive messages");

    return KATRA_SUCCESS;
}

/**
 * clear_session_scoped_memories() - Delete all session-scoped working memories
 *
 * Called during session_end() to clean up temporary working memory.
 * Session-scoped memories are marked with session_scoped=true flag.
 *
 * Returns:
 *   Positive number = count of memories deleted
 *   0 = no session-scoped memories found
 *   Negative = error code
 */
static int clear_session_scoped_memories(void) {
    /* Delete session-scoped memories via memory layer */
    size_t deleted_count = 0;
    int result = katra_memory_delete_session_scoped(g_context.ci_id, &deleted_count);

    if (result == KATRA_SUCCESS) {
        /* Return positive count on success */
        return (int)deleted_count;
    } else {
        /* Return negative error code on failure */
        return result;
    }
}

int session_end(void) {
    if (!g_initialized) {
        return E_INVALID_STATE;
    }

    LOG_INFO("Ending session: %s", g_context.session_id);

    /* Capture context snapshot for session continuity */
    int snapshot_result = capture_context_snapshot(g_context.ci_id, NULL);
    if (snapshot_result == KATRA_SUCCESS) {
        LOG_INFO("Context snapshot captured");
    } else {
        LOG_WARN("Context snapshot failed: %d (continuing shutdown)", snapshot_result);
    }

    /* Create daily summary (sunset) */
    int sundown_result = katra_sundown_basic(g_context.ci_id, NULL);
    if (sundown_result == KATRA_SUCCESS) {
        LOG_INFO("Daily summary created");
    }

    /* Auto-consolidate */
    auto_consolidate();

    /* Clear session-scoped memories (working memory) - Phase 1: Tag-Based Memory */
    int cleanup_result = clear_session_scoped_memories();
    if (cleanup_result > 0) {
        LOG_INFO("Cleared %d session-scoped memories (working memory)", cleanup_result);
    } else if (cleanup_result < 0) {
        LOG_WARN("Session memory cleanup failed: %d (continuing shutdown)", cleanup_result);
    }

    /* Autonomic cleanup: Unregister from meeting room registry */
    int unregister_result = meeting_room_unregister_ci(g_context.ci_id);
    if (unregister_result == KATRA_SUCCESS) {
        LOG_DEBUG("Unregistered from meeting room");
    } else {
        LOG_WARN("Meeting room unregister failed: %d (continuing shutdown)", unregister_result);
    }

    /* Cleanup breathing layer to allow re-initialization with new identity */
    breathe_cleanup();

    /* Return snapshot result (most critical for continuity) */
    return snapshot_result;
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

vector_store_t* breathing_get_vector_store(void) {
    return g_vector_store;
}

graph_store_t* breathing_get_graph_store(void) {
    return g_graph_store;
}

void breathing_set_graph_store(graph_store_t* store) {
    g_graph_store = store;
}

int breathing_init_vector_store(void) {
    if (g_vector_store) {
        return KATRA_SUCCESS;  /* Already initialized */
    }

    const char* ci_id = breathing_get_ci_id();
    if (!ci_id) {
        return E_INVALID_STATE;
    }

    g_vector_store = katra_vector_init(ci_id, false);
    if (!g_vector_store) {
        return E_SYSTEM_MEMORY;
    }

    g_vector_store->method = (embedding_method_t)g_context_config.embedding_method;

    int result = katra_vector_persist_init(ci_id);
    if (result == KATRA_SUCCESS) {
        katra_vector_persist_load(ci_id, g_vector_store);
    }

    return KATRA_SUCCESS;
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
