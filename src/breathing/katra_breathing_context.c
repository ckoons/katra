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
#include "katra_breathing_internal.h"
#include "katra_breathing_helpers.h"

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

    /* Build filtered array of matching record pointers */
    memory_record_t** filtered = calloc(result_count, sizeof(memory_record_t*));
    if (!filtered) {
        katra_memory_free_results(results, result_count);
        *count = 0;
        return NULL;
    }

    size_t match_count = 0;
    for (size_t i = 0; i < result_count; i++) {
        if (results[i]->content && strcasestr(results[i]->content, topic)) {
            filtered[match_count++] = results[i];
        }
    }

    if (match_count == 0) {
        free(filtered);
        katra_memory_free_results(results, result_count);
        *count = 0;
        return NULL;
    }

    /* Use helper to copy matching memories */
    char** matches = breathing_copy_memory_contents(filtered, match_count, count);

    /* Clean up */
    free(filtered);
    katra_memory_free_results(results, result_count);

    /* Track stats */
    if (matches) {
        breathing_track_topic_query(match_count);
        LOG_DEBUG("Found %zu memories matching topic: %s", match_count, topic);
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
