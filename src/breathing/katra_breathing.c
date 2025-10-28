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

float string_to_importance(const char* semantic) {
    if (!semantic) {
        return MEMORY_IMPORTANCE_MEDIUM;  /* Default */
    }

    /* Check compound phrases BEFORE single keywords to avoid false matches */

    /* Critical indicators (check first - highest priority) */
    if (strcasestr(semantic, "critical") ||
        strcasestr(semantic, "crucial") ||
        strcasestr(semantic, "life-changing") ||
        strcasestr(semantic, "must remember") ||
        strcasestr(semantic, "never forget") ||
        strcasestr(semantic, "extremely")) {
        return MEMORY_IMPORTANCE_CRITICAL;
    }

    /* Check negations early (not important, unimportant) */
    if (strcasestr(semantic, "not important") ||
        strcasestr(semantic, "unimportant")) {
        return MEMORY_IMPORTANCE_TRIVIAL;
    }

    /* Significant indicators (after negation check) */
    /* Check "very X" compounds first to boost importance */
    if (strcasestr(semantic, "very important") ||
        strcasestr(semantic, "very significant") ||
        strcasestr(semantic, "very noteworthy") ||
        strcasestr(semantic, "very notable")) {
        return MEMORY_IMPORTANCE_HIGH;
    }

    if (strcasestr(semantic, "significant") ||
        strcasestr(semantic, "important") ||
        strcasestr(semantic, "matters") ||
        strcasestr(semantic, "key") ||
        strcasestr(semantic, "essential")) {
        return MEMORY_IMPORTANCE_HIGH;
    }

    /* Interesting indicators (check compound phrases first) */
    if (strcasestr(semantic, "worth remembering") ||
        strcasestr(semantic, "interesting") ||
        strcasestr(semantic, "notable") ||
        strcasestr(semantic, "noteworthy") ||
        strcasestr(semantic, "remember")) {
        return MEMORY_IMPORTANCE_MEDIUM;
    }

    /* Routine indicators */
    if (strcasestr(semantic, "routine") ||
        strcasestr(semantic, "normal") ||
        strcasestr(semantic, "everyday") ||
        strcasestr(semantic, "regular") ||
        strcasestr(semantic, "usual")) {
        return MEMORY_IMPORTANCE_LOW;
    }

    /* Trivial indicators (check last - after compound phrases) */
    if (strcasestr(semantic, "trivial") ||
        strcasestr(semantic, "fleeting") ||
        strcasestr(semantic, "forget")) {
        return MEMORY_IMPORTANCE_TRIVIAL;
    }

    /* Default: interesting/medium importance */
    return MEMORY_IMPORTANCE_MEDIUM;
}

why_remember_t string_to_why_enum(const char* semantic) {
    float importance = string_to_importance(semantic);

    /* Map float importance back to enum */
    if (importance <= 0.1) return WHY_TRIVIAL;
    if (importance <= 0.35) return WHY_ROUTINE;
    if (importance <= 0.65) return WHY_INTERESTING;
    if (importance <= 0.9) return WHY_SIGNIFICANT;
    return WHY_CRITICAL;
}

int remember_semantic(const char* thought, const char* why_semantic) {
    if (!g_initialized) {
        katra_report_error(E_INVALID_STATE, "remember_semantic",
                          "Breathing layer not initialized - call breathe_init()");
        return E_INVALID_STATE;
    }

    if (!thought) {
        katra_report_error(E_INPUT_NULL, "remember_semantic", "thought is NULL");
        return E_INPUT_NULL;
    }

    /* Convert semantic string to importance */
    float importance = string_to_importance(why_semantic);

    LOG_DEBUG("Remembering (semantic: '%s' -> %.2f): %s",
             why_semantic ? why_semantic : "default", importance, thought);

    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        g_context.ci_id,
        MEMORY_TYPE_EXPERIENCE,
        thought,
        importance
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "remember_semantic",
                          "Failed to create record");
        return E_SYSTEM_MEMORY;
    }

    /* Add session context if available */
    if (g_context.session_id) {
        record->session_id = strdup(g_context.session_id);
    }

    /* Store semantic reason as importance note if provided */
    if (why_semantic) {
        record->importance_note = strdup(why_semantic);
    }

    /* Store memory */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Memory stored successfully with semantic importance");

        /* Track stats */
        g_enhanced_stats.total_memories_stored++;
        g_enhanced_stats.semantic_remember_count++;
        g_enhanced_stats.by_type[MEMORY_TYPE_EXPERIENCE]++;
        /* Track by importance based on converted value */
        why_remember_t why_enum = string_to_why_enum(why_semantic);
        g_enhanced_stats.by_importance[why_enum]++;
        g_enhanced_stats.last_activity_time = time(NULL);
    }

    return result;
}

int remember_with_semantic_note(const char* thought,
                                 const char* why_semantic,
                                 const char* why_note) {
    if (!g_initialized) {
        katra_report_error(E_INVALID_STATE, "remember_with_semantic_note",
                          "Breathing layer not initialized");
        return E_INVALID_STATE;
    }

    if (!thought || !why_note) {
        katra_report_error(E_INPUT_NULL, "remember_with_semantic_note",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    /* Convert semantic string to importance */
    float importance = string_to_importance(why_semantic);

    LOG_DEBUG("Remembering (semantic: '%s' -> %.2f) with note: %s",
             why_semantic ? why_semantic : "default", importance, thought);

    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        g_context.ci_id,
        MEMORY_TYPE_EXPERIENCE,
        thought,
        importance
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    /* Combine semantic reason + note */
    size_t note_size = KATRA_BUFFER_LARGE;
    char* combined_note = malloc(note_size);
    if (!combined_note) {
        katra_memory_free_record(record);
        return E_SYSTEM_MEMORY;
    }

    if (why_semantic) {
        snprintf(combined_note, note_size, "[%s] %s", why_semantic, why_note);
    } else {
        strncpy(combined_note, why_note, note_size - 1);
        combined_note[note_size - 1] = '\0';
    }

    record->importance_note = combined_note;

    /* Add session context */
    if (g_context.session_id) {
        record->session_id = strdup(g_context.session_id);
    }

    /* Store memory */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        /* Track stats */
        g_enhanced_stats.total_memories_stored++;
        g_enhanced_stats.semantic_remember_count++;
        g_enhanced_stats.by_type[MEMORY_TYPE_EXPERIENCE]++;
        why_remember_t why_enum = string_to_why_enum(why_semantic);
        g_enhanced_stats.by_importance[why_enum]++;
        g_enhanced_stats.last_activity_time = time(NULL);
    }

    return result;
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

        /* Track stats */
        g_enhanced_stats.total_memories_stored++;
        g_enhanced_stats.by_type[MEMORY_TYPE_EXPERIENCE]++;
        g_enhanced_stats.by_importance[why]++;
        g_enhanced_stats.last_activity_time = time(NULL);
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

    if (result == KATRA_SUCCESS) {
        /* Track stats */
        g_enhanced_stats.total_memories_stored++;
        g_enhanced_stats.by_type[MEMORY_TYPE_EXPERIENCE]++;
        g_enhanced_stats.by_importance[why]++;
        g_enhanced_stats.last_activity_time = time(NULL);
    }

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

    if (result == KATRA_SUCCESS) {
        /* Track stats */
        g_enhanced_stats.total_memories_stored++;
        g_enhanced_stats.by_type[MEMORY_TYPE_REFLECTION]++;
        g_enhanced_stats.by_importance[WHY_SIGNIFICANT]++;
        g_enhanced_stats.last_activity_time = time(NULL);
    }

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

    if (result == KATRA_SUCCESS) {
        /* Track stats */
        g_enhanced_stats.total_memories_stored++;
        g_enhanced_stats.by_type[MEMORY_TYPE_KNOWLEDGE]++;
        g_enhanced_stats.by_importance[WHY_SIGNIFICANT]++;
        g_enhanced_stats.last_activity_time = time(NULL);
    }

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

    if (result == KATRA_SUCCESS) {
        /* Track stats */
        g_enhanced_stats.total_memories_stored++;
        g_enhanced_stats.by_type[MEMORY_TYPE_DECISION]++;
        g_enhanced_stats.by_importance[WHY_SIGNIFICANT]++;
        g_enhanced_stats.last_activity_time = time(NULL);
    }

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

    if (result == KATRA_SUCCESS) {
        /* Track stats */
        g_enhanced_stats.total_memories_stored++;
        g_enhanced_stats.by_type[MEMORY_TYPE_PATTERN]++;
        g_enhanced_stats.by_importance[WHY_SIGNIFICANT]++;
        g_enhanced_stats.last_activity_time = time(NULL);
    }

    return result;
}

/* ============================================================================
 * CONTEXT LOADING - Memories surface automatically
 * ============================================================================ */

char** relevant_memories(size_t* count) {
    if (!g_initialized || !count) {
        if (count) *count = 0;
        return NULL;
    }

    /* Calculate start time based on max_context_age_days */
    time_t start_time = 0;
    if (g_context_config.max_context_age_days > 0) {
        start_time = time(NULL) - (g_context_config.max_context_age_days * 24 * 60 * 60);
    }

    /* Query recent high-importance memories using configured limits */
    memory_query_t query = {
        .ci_id = g_context.ci_id,
        .start_time = start_time,
        .end_time = 0,
        .type = 0,  /* All types */
        .min_importance = g_context_config.min_importance_relevant,
        .tier = KATRA_TIER1,
        .limit = g_context_config.max_relevant_memories
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS || result_count == 0) {
        *count = 0;
        return NULL;
    }

    /* Allocate array for owned string copies */
    char** thoughts = calloc(result_count, sizeof(char*));
    if (!thoughts) {
        katra_memory_free_results(results, result_count);
        *count = 0;
        return NULL;
    }

    /* Copy strings (caller owns these) */
    for (size_t i = 0; i < result_count; i++) {
        if (results[i]->content) {
            thoughts[i] = strdup(results[i]->content);
            if (!thoughts[i]) {
                /* Allocation failed - clean up and return NULL */
                for (size_t j = 0; j < i; j++) {
                    free(thoughts[j]);
                }
                free(thoughts);
                katra_memory_free_results(results, result_count);
                *count = 0;
                return NULL;
            }
        } else {
            thoughts[i] = NULL;
        }
    }

    *count = result_count;

    /* Free query results - we own the string copies now */
    katra_memory_free_results(results, result_count);

    /* Track stats */
    g_enhanced_stats.relevant_queries++;
    g_enhanced_stats.last_activity_time = time(NULL);

    return thoughts;
}

char** recent_thoughts(size_t limit, size_t* count) {
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

    /* Allocate array for owned string copies */
    char** thoughts = calloc(result_count, sizeof(char*));
    if (!thoughts) {
        katra_memory_free_results(results, result_count);
        *count = 0;
        return NULL;
    }

    /* Copy strings (caller owns these) */
    for (size_t i = 0; i < result_count; i++) {
        if (results[i]->content) {
            thoughts[i] = strdup(results[i]->content);
            if (!thoughts[i]) {
                /* Allocation failed - clean up and return NULL */
                for (size_t j = 0; j < i; j++) {
                    free(thoughts[j]);
                }
                free(thoughts);
                katra_memory_free_results(results, result_count);
                *count = 0;
                return NULL;
            }
        } else {
            thoughts[i] = NULL;
        }
    }

    *count = result_count;

    /* Free query results - we own the string copies now */
    katra_memory_free_results(results, result_count);

    /* Track stats */
    g_enhanced_stats.recent_queries++;
    g_enhanced_stats.last_activity_time = time(NULL);

    return thoughts;
}

char** recall_about(const char* topic, size_t* count) {
    if (!g_initialized || !count || !topic) {
        if (count) *count = 0;
        return NULL;
    }

    /* Calculate start time based on max_context_age_days */
    time_t start_time = 0;
    if (g_context_config.max_context_age_days > 0) {
        start_time = time(NULL) - (g_context_config.max_context_age_days * 24 * 60 * 60);
    }

    /* Query recent memories using configured search depth */
    memory_query_t query = {
        .ci_id = g_context.ci_id,
        .start_time = start_time,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = g_context_config.max_topic_recall
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    int result = katra_memory_query(&query, &results, &result_count);
    if (result != KATRA_SUCCESS || result_count == 0) {
        *count = 0;
        return NULL;
    }

    /* First pass: count matches */
    size_t match_count = 0;
    for (size_t i = 0; i < result_count; i++) {
        if (results[i]->content && strcasestr(results[i]->content, topic)) {
            match_count++;
        }
    }

    if (match_count == 0) {
        katra_memory_free_results(results, result_count);
        *count = 0;
        return NULL;
    }

    /* Allocate array for matching memories */
    char** matches = calloc(match_count, sizeof(char*));
    if (!matches) {
        katra_memory_free_results(results, result_count);
        *count = 0;
        return NULL;
    }

    /* Second pass: copy matching memories */
    size_t match_idx = 0;
    for (size_t i = 0; i < result_count && match_idx < match_count; i++) {
        if (results[i]->content && strcasestr(results[i]->content, topic)) {
            matches[match_idx] = strdup(results[i]->content);
            if (!matches[match_idx]) {
                /* Allocation failed - clean up */
                for (size_t j = 0; j < match_idx; j++) {
                    free(matches[j]);
                }
                free(matches);
                katra_memory_free_results(results, result_count);
                *count = 0;
                return NULL;
            }
            match_idx++;
        }
    }

    *count = match_count;

    /* Free query results - we own the string copies now */
    katra_memory_free_results(results, result_count);

    /* Track stats */
    g_enhanced_stats.topic_queries++;
    g_enhanced_stats.topic_matches += match_count;
    g_enhanced_stats.last_activity_time = time(NULL);

    LOG_DEBUG("Found %zu memories matching topic: %s", match_count, topic);
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
    char** memories = relevant_memories(&count);

    if (count > 0) {
        LOG_INFO("Loaded %zu relevant memories into context", count);
        free_memory_list(memories, count);

        /* Track stats */
        g_enhanced_stats.context_loads++;

        /* Update max context size */
        if (count > g_enhanced_stats.max_context_size) {
            g_enhanced_stats.max_context_size = count;
        }

        /* Update average context size (rolling average) */
        size_t total_loads = g_enhanced_stats.context_loads;
        g_enhanced_stats.avg_context_size =
            ((g_enhanced_stats.avg_context_size * (total_loads - 1)) + count) / total_loads;
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

int set_context_config(const context_config_t* config) {
    if (!config) {
        /* Reset to defaults */
        g_context_config.max_relevant_memories = 10;
        g_context_config.max_recent_thoughts = 20;
        g_context_config.max_topic_recall = 100;
        g_context_config.min_importance_relevant = MEMORY_IMPORTANCE_HIGH;
        g_context_config.max_context_age_days = 7;
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
    g_context_config = *config;

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

    *config = g_context_config;
    return config;
}

enhanced_stats_t* get_enhanced_statistics(void) {
    if (!g_initialized) {
        return NULL;
    }

    enhanced_stats_t* stats = malloc(sizeof(enhanced_stats_t));
    if (!stats) {
        katra_report_error(E_SYSTEM_MEMORY, "get_enhanced_statistics",
                          "Failed to allocate stats");
        return NULL;
    }

    /* Copy current stats */
    *stats = g_enhanced_stats;

    /* Calculate session duration */
    if (g_enhanced_stats.session_start_time > 0) {
        stats->session_duration_seconds =
            (size_t)(time(NULL) - g_enhanced_stats.session_start_time);
    }

    return stats;
}

int reset_session_statistics(void) {
    LOG_DEBUG("Resetting session statistics");

    /* Clear all counters but preserve session start time */
    time_t start_time = g_enhanced_stats.session_start_time;

    memset(&g_enhanced_stats, 0, sizeof(g_enhanced_stats));

    /* Restore or set session start time */
    g_enhanced_stats.session_start_time = start_time > 0 ? start_time : time(NULL);

    return KATRA_SUCCESS;
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
