/* © 2025 Casey Koons All rights reserved */

/**
 * katra_breathing_integration.c - Level 3 Integration API
 *
 * Runtime hooks for invisible memory formation.
 * Designed for integration into CI runtimes (Claude Code, Tekton, etc.)
 *
 * This file contains Level 3 functions that make memory truly invisible:
 * - get_working_context() - Auto-load context for system prompt
 * - auto_capture_from_response() - Invisible memory formation
 * - get_context_statistics() - Monitor integration health
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_continuity.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Global state for Level 3 integration */
static size_t g_session_captures = 0;  /* Auto-captures this session */

/* Forward declarations - implemented in katra_breathing.c */
extern bool katra_breathing_is_initialized(void);
extern const char* katra_breathing_get_ci_id(void);
extern int remember(const char* thought, why_remember_t why);

/* ============================================================================
 * KEYWORD ARRAYS - Pattern detection for significance detection
 * ============================================================================ */

/* GUIDELINE_APPROVED: Significance markers for automatic capture */
const char* const BREATHING_SIGNIFICANCE_MARKERS[] = {
    "important", "significant", "critical", "crucial", /* GUIDELINE_APPROVED */
    "learned", "realized", "discovered", "understood", /* GUIDELINE_APPROVED */
    "insight", "pattern", "decided", "concluded", /* GUIDELINE_APPROVED */
    "breakthrough", "key point", "essential", "must remember", /* GUIDELINE_APPROVED */
    NULL /* GUIDELINE_APPROVED */
};
/* GUIDELINE_APPROVED_END */

char* get_working_context(void) {
    if (!katra_breathing_is_initialized()) {
        return NULL;
    }

    const char* ci_id = katra_breathing_get_ci_id();
    if (!ci_id) {
        return NULL;
    }

    /* Allocate buffer for formatted context */
    size_t buffer_size = KATRA_BUFFER_LARGE * 4;  /* 64KB */
    char* context = malloc(buffer_size);
    if (!context) {
        katra_report_error(E_SYSTEM_MEMORY, "get_working_context",
                          KATRA_ERR_ALLOC_FAILED);
        return NULL;
    }

    size_t offset = 0;

    /* Header */
    offset += snprintf(context + offset, buffer_size - offset,
/* GUIDELINE_APPROVED: markdown formatting constants */
                      "# Working Memory Context\n\n");

    /* Yesterday's summary */
    digest_record_t* yesterday = NULL;
    int result = katra_sunrise_basic(ci_id, &yesterday);
    if (result == KATRA_SUCCESS && yesterday && yesterday->summary) {
        offset += snprintf(context + offset, buffer_size - offset,
                          "## Yesterday's Summary\n%s\n\n",
                          yesterday->summary);
        katra_digest_free(yesterday);
    }

    /* Personal collection memories (always included - identity-defining) */
    memory_query_t personal_query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 0,  /* No limit - include all personal memories */
        .filter_personal = true,
        .personal_value = true
    };

    memory_record_t** personal_results = NULL;
    size_t personal_count = 0;

    result = katra_memory_query(&personal_query, &personal_results, &personal_count);
    if (result == KATRA_SUCCESS && personal_count > 0) {
        offset += snprintf(context + offset, buffer_size - offset,
                          "## Personal Collection\n");

        /* Group by collection path for better organization */
        for (size_t i = 0; i < personal_count; i++) {
            if (!personal_results[i]->content) continue;

            const char* collection = personal_results[i]->collection ?
                                    personal_results[i]->collection : "Uncategorized";

            offset += snprintf(context + offset, buffer_size - offset,
                              "- [%s] %s\n", collection, personal_results[i]->content);

            /* Safety check - stop if buffer nearly full */
            if (offset >= buffer_size - KATRA_BUFFER_GROWTH_THRESHOLD) {
                offset += snprintf(context + offset, buffer_size - offset,
                                  "... (truncated)\n");
                break;
            }
        }

        offset += snprintf(context + offset, buffer_size - offset, "\n");
        katra_memory_free_results(personal_results, personal_count);
    }

    /* Recent significant memories */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = MEMORY_IMPORTANCE_HIGH,
        .tier = KATRA_TIER1,
        .limit = KATRA_INITIAL_CAPACITY_SMALL
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    result = katra_memory_query(&query, &results, &result_count);
    if (result == KATRA_SUCCESS && result_count > 0) {
        offset += snprintf(context + offset, buffer_size - offset,
                          "## Recent Significant Memories\n");

        for (size_t i = 0; i < result_count; i++) {
            if (!results[i]->content) continue;

            /* GUIDELINE_APPROVED: Enum-to-string mapping for memory_type_t */
            const char* type_str = "Experience"; /* GUIDELINE_APPROVED */
            switch (results[i]->type) {
                case MEMORY_TYPE_KNOWLEDGE: type_str = "Knowledge"; break; /* GUIDELINE_APPROVED */
                case MEMORY_TYPE_REFLECTION: type_str = "Reflection"; break; /* GUIDELINE_APPROVED */
                case MEMORY_TYPE_PATTERN: type_str = "Pattern"; break; /* GUIDELINE_APPROVED */
                case MEMORY_TYPE_DECISION: type_str = "Decision"; break; /* GUIDELINE_APPROVED */
                case MEMORY_TYPE_GOAL: type_str = "Goal"; break; /* GUIDELINE_APPROVED */
                default: break;
            }
            /* GUIDELINE_APPROVED_END */

            offset += snprintf(context + offset, buffer_size - offset,
                              "- [%s] %s", type_str, results[i]->content);

            /* Add importance note if present */
            if (results[i]->importance_note) {
                offset += snprintf(context + offset, buffer_size - offset,
                                  " (Why: %s)", results[i]->importance_note);
            }

            offset += snprintf(context + offset, buffer_size - offset, "\n");

            /* Safety check - stop if buffer nearly full */
            if (offset >= buffer_size - KATRA_BUFFER_GROWTH_THRESHOLD) {
                offset += snprintf(context + offset, buffer_size - offset,
                                  "... (truncated)\n");
                break;
            }
        }

        katra_memory_free_results(results, result_count);
    }

    /* Active goals and decisions */
    memory_query_t goal_query = {
        .ci_id = ci_id,
        .start_time = time(NULL) - (CONTEXT_WINDOW_DAYS * SECONDS_PER_DAY),  /* Last 7 days */
        .end_time = 0,
        .type = MEMORY_TYPE_GOAL,
        .min_importance = MEMORY_IMPORTANCE_MEDIUM,
        .tier = KATRA_TIER1,
        .limit = 5
    };

    results = NULL;
    result_count = 0;

    result = katra_memory_query(&goal_query, &results, &result_count);
    if (result == KATRA_SUCCESS && result_count > 0) {
        offset += snprintf(context + offset, buffer_size - offset,
                          "\n## Active Goals\n");

        for (size_t i = 0; i < result_count; i++) {
            if (results[i]->content) {
                offset += snprintf(context + offset, buffer_size - offset,
                                  "- %s\n", results[i]->content);
            }
        }

        katra_memory_free_results(results, result_count);
    }

    LOG_DEBUG("Generated working context: %zu bytes", offset);
    return context;
}

int auto_capture_from_response(const char* response) {
    if (!katra_breathing_is_initialized()) {
        /* Not an error - just not initialized yet */
        return KATRA_SUCCESS;
    }

    if (!response || strlen(response) == 0) {
        return KATRA_SUCCESS;
    }

    /* Check for significance markers (case-insensitive) */
    bool is_significant = false;
    for (int i = 0; BREATHING_SIGNIFICANCE_MARKERS[i] != NULL; i++) {
        /* Simple case-insensitive search */
        const char* p = response;
        size_t marker_len = strlen(BREATHING_SIGNIFICANCE_MARKERS[i]);
        while (*p) {
            if (strncasecmp(p, BREATHING_SIGNIFICANCE_MARKERS[i], marker_len) == 0) {
                is_significant = true;
                break;
            }
            p++;
        }
        if (is_significant) break;
    }

    if (!is_significant) {
        return KATRA_SUCCESS;  /* Nothing to capture */
    }

    /* Extract sentences containing significance markers */
    /* Simple implementation: store entire response if significant */
    /* TODO: Implement sentence-level extraction */

    LOG_DEBUG("Auto-capturing significant response: %.50s...", response);

    int result = remember(response, WHY_INTERESTING);
    if (result == KATRA_SUCCESS) {
        g_session_captures++;
        LOG_INFO("Auto-captured thought #%zu this session", g_session_captures);
    }

    return result;
}

int get_context_statistics(context_stats_t* stats) {
    if (!stats) {
        katra_report_error(E_INPUT_NULL, "get_context_statistics", "stats is NULL");
        return E_INPUT_NULL;
    }

    if (!katra_breathing_is_initialized()) {
        memset(stats, 0, sizeof(context_stats_t));
        return E_INVALID_STATE;
    }

    const char* ci_id = katra_breathing_get_ci_id();
    if (!ci_id) {
        memset(stats, 0, sizeof(context_stats_t));
        return E_INVALID_STATE;
    }

    /* Query all recent memories */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = time(NULL) - (CONTEXT_WINDOW_DAYS * SECONDS_PER_DAY),  /* Last 7 days */
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 0  /* No limit */
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS) {
        memset(stats, 0, sizeof(context_stats_t));
        return result;
    }

    /* Calculate statistics */
    stats->memory_count = result_count;
    stats->context_bytes = 0;
    stats->last_memory_time = 0;
    stats->session_captures = g_session_captures;

    for (size_t i = 0; i < result_count; i++) {
        if (results[i]->content) {
            stats->context_bytes += strlen(results[i]->content);
        }
        if (results[i]->response) {
            stats->context_bytes += strlen(results[i]->response);
        }
        if (results[i]->timestamp > stats->last_memory_time) {
            stats->last_memory_time = results[i]->timestamp;
        }
    }

    katra_memory_free_results(results, result_count);

    LOG_DEBUG("Context stats: %zu memories, %zu bytes, %zu auto-captures",
             stats->memory_count, stats->context_bytes, stats->session_captures);

    return KATRA_SUCCESS;
}
