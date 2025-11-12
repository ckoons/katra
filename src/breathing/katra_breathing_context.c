/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_context.c - Context loading operations
 *
 * Automatic memory surfacing: relevant_memories, recent_thoughts, recall_about
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_string_literals.h"
#include "katra_breathing_internal.h"
#include "katra_breathing_helpers.h"
#include "katra_breathing_search.h"  /* Phase 6.1f: hybrid search */

/* ============================================================================
 * CONTEXT LOADING - Memories surface automatically
 * ============================================================================ */

char** relevant_memories(size_t* count) {
    if (!breathing_get_initialized() || !count) {
        if (count) *count = 0;
        return NULL;
    }

    context_config_t* config = breathing_get_config_ptr();

    /* Calculate start time based on max_context_age_days */
    time_t start_time = 0;
    if (config->max_context_age_days > 0) {
        start_time = time(NULL) - (config->max_context_age_days * SECONDS_PER_DAY);
    }

    /* Query recent high-importance memories using configured limits */
    memory_query_t query = {
        .ci_id = breathing_get_ci_id(),
        .start_time = start_time,
        .end_time = 0,
        .type = 0,  /* All types */
        .min_importance = config->min_importance_relevant,
        .tier = KATRA_TIER1,
        .limit = config->max_relevant_memories
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS || result_count == 0) {
        *count = 0;
        return NULL;
    }

    /* Copy memory contents using helper */
    char** thoughts = breathing_copy_memory_contents(results, result_count, count);

    /* Free query results - we own the string copies now */
    katra_memory_free_results(results, result_count);

    /* Track stats */
    if (thoughts) {
        breathing_track_relevant_query();
    }

    return thoughts;
}

char** recent_thoughts(size_t limit, size_t* count) {
    if (!breathing_get_initialized() || !count) {
        if (count) *count = 0;
        return NULL;
    }

    memory_query_t query = {
        .ci_id = breathing_get_ci_id(),
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = limit
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS || result_count == 0) {
        *count = 0;
        return NULL;
    }

    /* Copy memory contents using helper */
    char** thoughts = breathing_copy_memory_contents(results, result_count, count);

    /* Free query results - we own the string copies now */
    katra_memory_free_results(results, result_count);

    /* Track stats */
    if (thoughts) {
        breathing_track_recent_query();
    }

    return thoughts;
}

char** recall_about(const char* topic, size_t* count) {
    if (!breathing_get_initialized() || !count || !topic) {
        if (count) *count = 0;
        return NULL;
    }

    context_config_t* config = breathing_get_config_ptr();

    /* Calculate start time based on max_context_age_days */
    time_t start_time = 0;
    if (config->max_context_age_days > 0) {
        start_time = time(NULL) - (config->max_context_age_days * SECONDS_PER_DAY);
    }

    /* Query recent memories using configured search depth */
    memory_query_t query = {
        .ci_id = breathing_get_ci_id(),
        .start_time = start_time,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = config->max_topic_recall
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS || result_count == 0) {
        *count = 0;
        return NULL;
    }

    /* Use hybrid or keyword-only search based on config (Phase 6.1f) */
    char** matches = NULL;
    size_t match_count = 0;

    if (config->use_semantic_search) {
        matches = hybrid_search(topic, results, result_count, &match_count);
    } else {
        matches = keyword_search_only(topic, results, result_count, &match_count);
    }

    /* Clean up query results */
    katra_memory_free_results(results, result_count);

    /* Update output count */
    *count = match_count;

    /* Track stats */
    if (matches) {
        breathing_track_topic_query(match_count);
        LOG_DEBUG("Found %zu memories matching topic: %s", match_count, topic);
    }

    return matches;
}

char** what_do_i_know(const char* concept, size_t* count) {
    if (!breathing_get_initialized() || !count || !concept) {
        if (count) *count = 0;
        return NULL;
    }

    context_config_t* config = breathing_get_config_ptr();

    /* Calculate start time based on max_context_age_days */
    time_t start_time = 0;
    if (config->max_context_age_days > 0) {
        start_time = time(NULL) - (config->max_context_age_days * SECONDS_PER_DAY);
    }

    /* Query recent KNOWLEDGE memories only */
    memory_query_t query = {
        .ci_id = breathing_get_ci_id(),
        .start_time = start_time,
        .end_time = 0,
        .type = MEMORY_TYPE_KNOWLEDGE,  /* Only knowledge */
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = config->max_topic_recall
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS || result_count == 0) {
        *count = 0;
        return NULL;
    }

    /* Use hybrid or keyword-only search based on config (Phase 6.1f) */
    char** matches = NULL;
    size_t match_count = 0;

    if (config->use_semantic_search) {
        matches = hybrid_search(concept, results, result_count, &match_count);
    } else {
        matches = keyword_search_only(concept, results, result_count, &match_count);
    }

    /* Clean up query results */
    katra_memory_free_results(results, result_count);

    /* Update output count */
    *count = match_count;

    /* Track stats */
    if (matches) {
        breathing_track_topic_query(match_count);
        LOG_DEBUG("Found %zu knowledge items matching concept: %s", match_count, concept);
    }

    return matches;
}

void free_memory_list(char** list, size_t count) {
    if (!list) {
        return;
    }

    /* Free each string in the list */
    for (size_t i = 0; i < count; i++) {
        free(list[i]);
    }

    /* Free the array itself */
    free(list);
}

/* ============================================================================
 * CROSS-SESSION CONTINUITY
 * ============================================================================ */

char** recall_previous_session(const char* ci_id, size_t limit, size_t* count) {
    if (!ci_id || !count) {
        if (count) *count = 0;
        katra_report_error(E_INPUT_NULL, "recall_previous_session", KATRA_ERR_NULL_PARAMETER);
        return NULL;
    }

    if (!breathing_get_initialized()) {
        *count = 0;
        return NULL;
    }

    /* Get current session ID to exclude it */
    const char* current_session = breathing_get_session_id();
    if (!current_session) {
        /* No current session - can query all sessions */
        LOG_DEBUG("No current session - querying all recent memories");
    }

    /* Query recent memories for this CI */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = 0,  /* All types */
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = limit * 2  /* Query 2x to filter current session */
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS || result_count == 0) {
        *count = 0;
        return NULL;
    }

    /* Build filtered array - exclude current session, find previous session */
    const char* prev_session_id = NULL;
    memory_record_t** filtered = calloc(limit, sizeof(memory_record_t*));
    if (!filtered) {
        katra_memory_free_results(results, result_count);
        *count = 0;
        return NULL;
    }

    size_t match_count = 0;
    for (size_t i = 0; i < result_count && match_count < limit; i++) {
        const char* session = results[i]->session_id;

        /* Skip memories without session_id */
        if (!session) {
            continue;
        }

        /* Skip current session */
        if (current_session && strcmp(session, current_session) == 0) {
            continue;
        }

        /* Found a previous session memory */
        if (prev_session_id == NULL) {
            /* First previous session we encountered */
            prev_session_id = session;
            filtered[match_count++] = results[i];
        } else if (strcmp(session, prev_session_id) == 0) {
            /* Same previous session */
            filtered[match_count++] = results[i];
        }
        /* Else: Different previous session (even older), skip */
    }

    if (match_count == 0) {
        free(filtered);
        katra_memory_free_results(results, result_count);
        *count = 0;
        LOG_DEBUG("No previous session found");
        return NULL;
    }

    /* Use helper to copy matching memories */
    char** prev_memories = breathing_copy_memory_contents(filtered, match_count, count);

    /* Clean up */
    free(filtered);
    katra_memory_free_results(results, result_count);

    if (prev_memories) {
        LOG_INFO("Recalled %zu memories from previous session: %s",
                match_count, prev_session_id ? prev_session_id : STR_UNKNOWN);
    }

    return prev_memories;
}
