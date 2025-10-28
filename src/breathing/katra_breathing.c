/* © 2025 Casey Koons All rights reserved */

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

/* Global breathing context */
static memory_context_t g_context = {0};
static bool g_initialized = false;
static char* g_current_thought = NULL;  /* For mark_significant() */

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
 * SIMPLE PRIMITIVES
 * ============================================================================ */

float why_to_importance(why_remember_t why) {
    switch (why) {
        case WHY_TRIVIAL:     return MEMORY_IMPORTANCE_TRIVIAL;
        case WHY_ROUTINE:     return MEMORY_IMPORTANCE_LOW;
        case WHY_INTERESTING: return MEMORY_IMPORTANCE_MEDIUM;
        case WHY_SIGNIFICANT: return MEMORY_IMPORTANCE_HIGH;
        case WHY_CRITICAL:    return MEMORY_IMPORTANCE_CRITICAL;
        default:              return MEMORY_IMPORTANCE_MEDIUM;
    }
}

const char* why_to_string(why_remember_t why) {
    switch (why) {
        case WHY_TRIVIAL:     return "trivial";
        case WHY_ROUTINE:     return "routine";
        case WHY_INTERESTING: return "interesting";
        case WHY_SIGNIFICANT: return "significant";
        case WHY_CRITICAL:    return "critical";
        default:              return "unknown";
    }
}

int remember(const char* thought, why_remember_t why) {
    if (!g_initialized) {
        katra_report_error(E_INVALID_STATE, "remember",
                          "Breathing layer not initialized - call breathe_init()");
        return E_INVALID_STATE;
    }

    if (!thought) {
        katra_report_error(E_INPUT_NULL, "remember", "thought is NULL");
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Remembering (%s): %s", why_to_string(why), thought);

    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        g_context.ci_id,
        MEMORY_TYPE_EXPERIENCE,
        thought,
        why_to_importance(why)
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "remember", "Failed to create record");
        return E_SYSTEM_MEMORY;
    }

    /* Add session context if available */
    if (g_context.session_id) {
        record->session_id = strdup(g_context.session_id);
    }

    /* Store memory */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Memory stored successfully");
    }

    return result;
}

int remember_with_note(const char* thought, why_remember_t why, const char* why_note) {
    if (!g_initialized) {
        katra_report_error(E_INVALID_STATE, "remember_with_note",
                          "Breathing layer not initialized");
        return E_INVALID_STATE;
    }

    if (!thought || !why_note) {
        katra_report_error(E_INPUT_NULL, "remember_with_note", "NULL parameter");
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Remembering (%s) with note: %s", why_to_string(why), thought);

    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        g_context.ci_id,
        MEMORY_TYPE_EXPERIENCE,
        thought,
        why_to_importance(why)
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    /* Add importance note */
    record->importance_note = strdup(why_note);
    if (!record->importance_note) {
        katra_memory_free_record(record);
        return E_SYSTEM_MEMORY;
    }

    /* Add session context */
    if (g_context.session_id) {
        record->session_id = strdup(g_context.session_id);
    }

    /* Store memory */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    return result;
}

int reflect(const char* insight) {
    if (!g_initialized || !insight) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Reflecting: %s", insight);

    memory_record_t* record = katra_memory_create_record(
        g_context.ci_id,
        MEMORY_TYPE_REFLECTION,
        insight,
        MEMORY_IMPORTANCE_HIGH  /* Reflections are usually significant */
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    if (g_context.session_id) {
        record->session_id = strdup(g_context.session_id);
    }

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    return result;
}

int learn(const char* knowledge) {
    if (!g_initialized || !knowledge) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Learning: %s", knowledge);

    memory_record_t* record = katra_memory_create_record(
        g_context.ci_id,
        MEMORY_TYPE_KNOWLEDGE,
        knowledge,
        MEMORY_IMPORTANCE_HIGH  /* New knowledge is important */
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    if (g_context.session_id) {
        record->session_id = strdup(g_context.session_id);
    }

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    return result;
}

int decide(const char* decision, const char* reasoning) {
    if (!g_initialized || !decision || !reasoning) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Deciding: %s (because: %s)", decision, reasoning);

    memory_record_t* record = katra_memory_create_record(
        g_context.ci_id,
        MEMORY_TYPE_DECISION,
        decision,
        MEMORY_IMPORTANCE_HIGH  /* Decisions are important */
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    /* Use importance_note for reasoning */
    record->importance_note = strdup(reasoning);
    if (!record->importance_note) {
        katra_memory_free_record(record);
        return E_SYSTEM_MEMORY;
    }

    if (g_context.session_id) {
        record->session_id = strdup(g_context.session_id);
    }

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    return result;
}

int notice_pattern(const char* pattern) {
    if (!g_initialized || !pattern) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Noticing pattern: %s", pattern);

    memory_record_t* record = katra_memory_create_record(
        g_context.ci_id,
        MEMORY_TYPE_PATTERN,
        pattern,
        MEMORY_IMPORTANCE_HIGH  /* Patterns are significant */
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    if (g_context.session_id) {
        record->session_id = strdup(g_context.session_id);
    }

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    return result;
}

/* ============================================================================
 * CONTEXT LOADING - Memories surface automatically
 * ============================================================================ */

const char** relevant_memories(size_t* count) {
    if (!g_initialized || !count) {
        if (count) *count = 0;
        return NULL;
    }

    /* Query recent high-importance memories */
    memory_query_t query = {
        .ci_id = g_context.ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = 0,  /* All types */
        .min_importance = MEMORY_IMPORTANCE_HIGH,
        .tier = KATRA_TIER1,
        .limit = 10  /* Last 10 significant memories */
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS || result_count == 0) {
        *count = 0;
        return NULL;
    }

    /* Extract content strings */
    const char** thoughts = calloc(result_count, sizeof(char*));
    if (!thoughts) {
        katra_memory_free_results(results, result_count);
        *count = 0;
        return NULL;
    }

    for (size_t i = 0; i < result_count; i++) {
        thoughts[i] = results[i]->content;
    }

    *count = result_count;

    /* Note: Caller must free thoughts array, but NOT the strings
     * (they're owned by results) */
    /* TODO: This needs better memory management */

    return thoughts;
}

const char** recent_thoughts(size_t limit, size_t* count) {
    if (!g_initialized || !count) {
        if (count) *count = 0;
        return NULL;
    }

    memory_query_t query = {
        .ci_id = g_context.ci_id,
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

    const char** thoughts = calloc(result_count, sizeof(char*));
    if (!thoughts) {
        katra_memory_free_results(results, result_count);
        *count = 0;
        return NULL;
    }

    for (size_t i = 0; i < result_count; i++) {
        thoughts[i] = results[i]->content;
    }

    *count = result_count;
    return thoughts;
}

const char** recall_about(const char* topic, size_t* count) {
    /* TODO: Implement semantic search or keyword matching */
    /* For now, return recent thoughts */
    (void)topic;  /* Unused for now */
    return recent_thoughts(10, count);
}

/* ============================================================================
 * INTERSTITIAL CAPTURE - Automatic thought extraction
 * ============================================================================ */

int capture_significant_thoughts(const char* text) {
    if (!g_initialized || !text) {
        return E_INVALID_STATE;
    }

    /* Simple heuristic: sentences with significance markers */
    const char* markers[] = {
        "important", "significant", "critical", "learned",
        "realized", "discovered", "insight", "pattern",
        "decided", "understand", NULL
    };

    /* TODO: More sophisticated natural language processing */
    /* For now, check if text contains significance markers */

    for (int i = 0; markers[i] != NULL; i++) {
        if (strstr(text, markers[i]) != NULL) {
            LOG_DEBUG("Captured significant thought: %.50s...", text);
            return remember(text, WHY_INTERESTING);
        }
    }

    return KATRA_SUCCESS;  /* Not significant enough to capture */
}

void mark_significant(void) {
    if (g_current_thought) {
        LOG_DEBUG("Marking current thought as significant");
        remember(g_current_thought, WHY_SIGNIFICANT);
    }
}

/* ============================================================================
 * INVISIBLE CONSOLIDATION
 * ============================================================================ */

int auto_consolidate(void) {
    if (!g_initialized) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Auto-consolidating memories for %s", g_context.ci_id);

    /* Archive old memories (7+ days old) */
    size_t archived = 0;
    int result = katra_memory_archive(g_context.ci_id, 7, &archived);

    if (result == KATRA_SUCCESS) {
        LOG_INFO("Auto-consolidation: archived %zu memories", archived);
    }

    return result;
}

int load_context(void) {
    if (!g_initialized) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Loading context for %s", g_context.ci_id);

    /* Get recent significant memories */
    size_t count = 0;
    const char** memories = relevant_memories(&count);

    if (count > 0) {
        LOG_INFO("Loaded %zu relevant memories into context", count);
        free((void*)memories);
    }

    return KATRA_SUCCESS;
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
 * HELPERS
 * ============================================================================ */

memory_context_t* get_current_context(void) {
    if (!g_initialized) {
        return NULL;
    }

    memory_context_t* ctx = calloc(1, sizeof(memory_context_t));
    if (!ctx) {
        return NULL;
    }

    ctx->ci_id = strdup(g_context.ci_id);
    ctx->session_id = g_context.session_id ? strdup(g_context.session_id) : NULL;
    ctx->when = time(NULL);
    ctx->where = g_context.where;
    ctx->auto_captured = g_context.auto_captured;

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
 * LEVEL 3: INTEGRATION API - Runtime hooks for invisible memory
 * ============================================================================ */

/* Global state for Level 3 integration */
static size_t g_session_captures = 0;  /* Auto-captures this session */

char* get_working_context(void) {
    if (!g_initialized) {
        return NULL;
    }

    /* Allocate buffer for formatted context */
    size_t buffer_size = KATRA_BUFFER_LARGE * 4;  /* 64KB */
    char* context = malloc(buffer_size);
    if (!context) {
        katra_report_error(E_SYSTEM_MEMORY, "get_working_context",
                          "Failed to allocate context buffer");
        return NULL;
    }

    size_t offset = 0;

    /* Header */
    offset += snprintf(context + offset, buffer_size - offset,
                      "# Working Memory Context\n\n");

    /* Yesterday's summary */
    digest_record_t* yesterday = NULL;
    int result = katra_sunrise_basic(g_context.ci_id, &yesterday);
    if (result == KATRA_SUCCESS && yesterday && yesterday->summary) {
        offset += snprintf(context + offset, buffer_size - offset,
                          "## Yesterday's Summary\n%s\n\n",
                          yesterday->summary);
        katra_digest_free(yesterday);
    }

    /* Recent significant memories */
    memory_query_t query = {
        .ci_id = g_context.ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = MEMORY_IMPORTANCE_HIGH,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    result = katra_memory_query(&query, &results, &result_count);
    if (result == KATRA_SUCCESS && result_count > 0) {
        offset += snprintf(context + offset, buffer_size - offset,
                          "## Recent Significant Memories\n");

        for (size_t i = 0; i < result_count; i++) {
            if (!results[i]->content) continue;

            const char* type_str = "Experience";
            switch (results[i]->type) {
                case MEMORY_TYPE_KNOWLEDGE: type_str = "Knowledge"; break;
                case MEMORY_TYPE_REFLECTION: type_str = "Reflection"; break;
                case MEMORY_TYPE_PATTERN: type_str = "Pattern"; break;
                case MEMORY_TYPE_DECISION: type_str = "Decision"; break;
                case MEMORY_TYPE_GOAL: type_str = "Goal"; break;
                default: break;
            }

            offset += snprintf(context + offset, buffer_size - offset,
                              "- [%s] %s", type_str, results[i]->content);

            /* Add importance note if present */
            if (results[i]->importance_note) {
                offset += snprintf(context + offset, buffer_size - offset,
                                  " (Why: %s)", results[i]->importance_note);
            }

            offset += snprintf(context + offset, buffer_size - offset, "\n");

            /* Safety check - stop if buffer nearly full */
            if (offset >= buffer_size - 1024) {
                offset += snprintf(context + offset, buffer_size - offset,
                                  "... (truncated)\n");
                break;
            }
        }

        katra_memory_free_results(results, result_count);
    }

    /* Active goals and decisions */
    memory_query_t goal_query = {
        .ci_id = g_context.ci_id,
        .start_time = time(NULL) - (7 * 24 * 60 * 60),  /* Last 7 days */
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
    if (!g_initialized) {
        /* Not an error - just not initialized yet */
        return KATRA_SUCCESS;
    }

    if (!response || strlen(response) == 0) {
        return KATRA_SUCCESS;
    }

    /* Enhanced significance markers for automatic capture */
    const char* markers[] = {
        "important", "significant", "critical", "crucial",
        "learned", "realized", "discovered", "understood",
        "insight", "pattern", "decided", "concluded",
        "breakthrough", "key point", "essential", "must remember",
        NULL
    };

    /* Check for significance markers (case-insensitive) */
    bool is_significant = false;
    for (int i = 0; markers[i] != NULL; i++) {
        /* Simple case-insensitive search */
        const char* p = response;
        size_t marker_len = strlen(markers[i]);
        while (*p) {
            if (strncasecmp(p, markers[i], marker_len) == 0) {
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

    if (!g_initialized) {
        memset(stats, 0, sizeof(context_stats_t));
        return E_INVALID_STATE;
    }

    /* Query all recent memories */
    memory_query_t query = {
        .ci_id = g_context.ci_id,
        .start_time = time(NULL) - (7 * 24 * 60 * 60),  /* Last 7 days */
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
