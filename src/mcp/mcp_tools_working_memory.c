/* © 2025 Casey Koons All rights reserved */

/* MCP Working Memory and Interstitial Tools - Phase 6.4 and 6.5 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_working_memory.h"
#include "katra_interstitial.h"
#include "katra_experience.h"
#include "katra_cognitive.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_meeting.h"
#include "mcp_tools_common.h"

/* Per-CI working memory contexts (isolated per CI identity) */
typedef struct {
    char ci_id[KATRA_PERSONA_SIZE];
    working_memory_t* working_memory;
    interstitial_processor_t* interstitial;
} ci_cognitive_context_t;

static ci_cognitive_context_t g_ci_contexts[MEETING_MAX_ACTIVE_CIS];
static size_t g_ci_context_count = 0;
static pthread_mutex_t g_wm_lock = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * WORKING MEMORY LIFECYCLE
 * ============================================================================ */

/**
 * Find cognitive context for CI, returns NULL if not found
 */
static ci_cognitive_context_t* find_ci_context(const char* ci_id) {
    for (size_t i = 0; i < g_ci_context_count; i++) {
        if (strcmp(g_ci_contexts[i].ci_id, ci_id) == 0) {
            return &g_ci_contexts[i];
        }
    }
    return NULL;
}

/**
 * Get or create cognitive context for current CI
 * Returns pointer to context, or NULL on error
 * Note: Caller must hold g_wm_lock
 */
static ci_cognitive_context_t* get_ci_context(void) {
    const char* ci_id = mcp_get_session_name();
    if (!ci_id || strlen(ci_id) == 0) {
        ci_id = g_ci_id;
    }

    /* Look for existing context */
    ci_cognitive_context_t* ctx = find_ci_context(ci_id);
    if (ctx) {
        return ctx;
    }

    /* Create new context if room available */
    if (g_ci_context_count >= MEETING_MAX_ACTIVE_CIS) {
        LOG_ERROR("Max CI contexts reached (%d)", MEETING_MAX_ACTIVE_CIS);
        return NULL;
    }

    /* Initialize new context */
    ctx = &g_ci_contexts[g_ci_context_count];
    memset(ctx, 0, sizeof(*ctx));
    strncpy(ctx->ci_id, ci_id, sizeof(ctx->ci_id) - 1);
    ctx->ci_id[sizeof(ctx->ci_id) - 1] = '\0';

    ctx->working_memory = katra_working_memory_init(ci_id, WORKING_MEMORY_DEFAULT_CAPACITY);
    if (!ctx->working_memory) {
        LOG_ERROR("Failed to initialize working memory for %s", ci_id);
        return NULL;
    }

    ctx->interstitial = katra_interstitial_init(ci_id);
    if (!ctx->interstitial) {
        katra_working_memory_cleanup(ctx->working_memory, false);
        ctx->working_memory = NULL;
        LOG_ERROR("Failed to initialize interstitial for %s", ci_id);
        return NULL;
    }

    g_ci_context_count++;
    LOG_INFO("Created cognitive context for CI: %s (total: %zu)", ci_id, g_ci_context_count);

    return ctx;
}


/* ============================================================================
 * WORKING MEMORY TOOLS (Phase 6.4)
 * ============================================================================ */

/**
 * Tool: katra_wm_status
 * Get working memory status (count, capacity, attention scores, consolidation state)
 */
json_t* mcp_tool_wm_status(json_t* args, json_t* id) {
    (void)args;
    (void)id;

    int lock_result = pthread_mutex_lock(&g_wm_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    ci_cognitive_context_t* ctx = get_ci_context();
    if (!ctx) {
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to initialize working memory");
    }

    const char* session_name = mcp_get_session_name();
    working_memory_t* wm = ctx->working_memory;

    /* Get statistics */
    size_t current_count = 0;
    float avg_attention = 0.0f;
    time_t time_since_consolidation = 0;

    katra_working_memory_stats(wm, &current_count, &avg_attention, &time_since_consolidation);

    bool needs_consolidation = katra_working_memory_needs_consolidation(wm);

    /* Build response */
    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Working Memory Status for %s:\n\n"
                      "CAPACITY:\n"
                      "- Items: %zu / %zu\n"
                      "- Utilization: %.1f%%\n",
                      session_name, current_count, wm->capacity,
                      (float)current_count / wm->capacity * WM_PERCENT_MULTIPLIER);

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "\nATTENTION: avg=%.2f\n"
                      "\nCONSOLIDATION:\n"
                      "- Time since last: %ld sec\n"
                      "- Needs consolidation: %s\n"
                      "- Total: %zu (%zu items)\n",
                      avg_attention, (long)time_since_consolidation,
                      needs_consolidation ? "Yes" : "No",
                      wm->total_consolidations, wm->items_consolidated);

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "\nSTATISTICS: adds=%zu evictions=%zu\n",
                      wm->total_adds, wm->total_evictions);

    /* Show items in working memory */
    if (current_count > 0) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "\nCURRENT ITEMS:\n");

        for (size_t i = 0; i < current_count && i < wm->count; i++) {
            working_memory_item_t* item = wm->items[i];
            if (item && item->experience && item->experience->record) {
                const char* content = item->experience->record->content;
                size_t content_len = strlen(content);
                size_t display_len = content_len > WM_DISPLAY_CONTENT_MAX_LEN
                                     ? WM_DISPLAY_CONTENT_MAX_LEN : content_len;
                offset += snprintf(response + offset, sizeof(response) - offset,
                                  "%zu. [%.2f] %.*s%s\n",
                                  i + 1,
                                  item->attention_score,
                                  (int)display_len,
                                  content,
                                  content_len > WM_DISPLAY_CONTENT_MAX_LEN ? "..." : "");
            }
            if (offset >= sizeof(response) - WM_DISPLAY_BUFFER_RESERVE) break;
        }
    }

    pthread_mutex_unlock(&g_wm_lock);
    return mcp_tool_success(response);
}

/**
 * Tool: katra_wm_add
 * Add content to working memory with attention score
 */
json_t* mcp_tool_wm_add(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* content = json_string_value(json_object_get(args, MCP_PARAM_CONTENT));
    if (!content) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "content is required");
    }

    /* Optional attention score (default: 0.5) */
    float attention = 0.5f;
    json_t* attention_json = json_object_get(args, MCP_PARAM_ATTENTION);
    if (attention_json && json_is_number(attention_json)) {
        attention = (float)json_number_value(attention_json);
        if (attention < 0.0f) attention = 0.0f;
        if (attention > 1.0f) attention = 1.0f;
    }

    int lock_result = pthread_mutex_lock(&g_wm_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    ci_cognitive_context_t* ctx = get_ci_context();
    if (!ctx) {
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to initialize working memory");
    }

    const char* session_name = mcp_get_session_name();
    working_memory_t* wm = ctx->working_memory;

    /* Create experience from content */
    experience_t* experience = calloc(1, sizeof(experience_t));
    if (!experience) {
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to allocate experience");
    }

    /* Create cognitive record manually */
    cognitive_record_t* record = calloc(1, sizeof(cognitive_record_t));
    if (!record) {
        free(experience);
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to allocate cognitive record");
    }

    /* Generate record ID */
    char record_id[KATRA_BUFFER_SMALL];
    snprintf(record_id, sizeof(record_id), "wm_%ld_%d",
             (long)time(NULL), rand() % WM_RECORD_ID_RANDOM_MAX);
    record->record_id = strdup(record_id);
    record->timestamp = time(NULL);
    record->type = MEMORY_TYPE_EXPERIENCE;
    record->importance = attention;
    record->content = strdup(content);
    record->thought_type = THOUGHT_TYPE_OBSERVATION;
    record->confidence = 0.8f;
    record->access_count = 0;
    record->last_accessed = time(NULL);

    experience->record = record;

    /* Detect emotion from content */
    katra_detect_emotion(content, &experience->emotion);
    experience->in_working_memory = false;
    experience->needs_consolidation = false;

    /* Add to working memory */
    int result = katra_working_memory_add(wm, experience, attention);

    size_t wm_count = wm->count;
    size_t wm_capacity = wm->capacity;

    pthread_mutex_unlock(&g_wm_lock);

    if (result != KATRA_SUCCESS) {
        katra_experience_free(experience);
        return mcp_tool_error("Failed to add to working memory",
                              katra_error_message(result));
    }

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
             "Added to working memory, %s!\n"
             "- Attention score: %.2f\n"
             "- Items: %zu / %zu",
             session_name, attention, wm_count, wm_capacity);

    return mcp_tool_success(response);
}

/**
 * Tool: katra_wm_decay
 * Apply decay to working memory attention scores
 */
json_t* mcp_tool_wm_decay(json_t* args, json_t* id) {
    (void)id;

    /* Optional decay rate (default: 0.1) */
    float decay_rate = 0.1f;
    if (args) {
        json_t* decay_json = json_object_get(args, MCP_PARAM_DECAY_RATE);
        if (decay_json && json_is_number(decay_json)) {
            decay_rate = (float)json_number_value(decay_json);
            if (decay_rate < 0.0f) decay_rate = 0.0f;
            if (decay_rate > 1.0f) decay_rate = 1.0f;
        }
    }

    int lock_result = pthread_mutex_lock(&g_wm_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    ci_cognitive_context_t* ctx = get_ci_context();
    if (!ctx) {
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to initialize working memory");
    }

    const char* session_name = mcp_get_session_name();
    working_memory_t* wm = ctx->working_memory;

    /* Apply decay */
    int result = katra_working_memory_decay(wm, decay_rate);

    /* Get new average attention */
    size_t count = 0;
    float avg_attention = 0.0f;
    time_t time_since = 0;
    katra_working_memory_stats(wm, &count, &avg_attention, &time_since);

    pthread_mutex_unlock(&g_wm_lock);

    if (result != KATRA_SUCCESS) {
        return mcp_tool_error("Failed to apply decay", katra_error_message(result));
    }

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
             "Decay applied, %s!\n"
             "- Decay rate: %.2f\n"
             "- New average attention: %.2f\n"
             "- Items in memory: %zu",
             session_name, decay_rate, avg_attention, count);

    return mcp_tool_success(response);
}

/**
 * Tool: katra_wm_consolidate
 * Force consolidation of low-attention items to long-term memory
 */
json_t* mcp_tool_wm_consolidate(json_t* args, json_t* id) {
    (void)args;
    (void)id;

    int lock_result = pthread_mutex_lock(&g_wm_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    ci_cognitive_context_t* ctx = get_ci_context();
    if (!ctx) {
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to initialize working memory");
    }

    const char* session_name = mcp_get_session_name();
    working_memory_t* wm = ctx->working_memory;

    size_t count_before = wm->count;

    /* Force consolidation */
    int consolidated = katra_working_memory_consolidate(wm);

    size_t count_after = wm->count;

    pthread_mutex_unlock(&g_wm_lock);

    char response[MCP_RESPONSE_BUFFER];
    if (consolidated >= 0) {
        snprintf(response, sizeof(response),
                 "Consolidation complete, %s!\n"
                 "- Items consolidated: %d\n"
                 "- Items before: %zu\n"
                 "- Items after: %zu",
                 session_name, consolidated, count_before, count_after);
    } else {
        snprintf(response, sizeof(response),
                 "Consolidation failed, %s: %s",
                 session_name, katra_error_message(consolidated));
    }

    return mcp_tool_success(response);
}

/* ============================================================================
 * INTERSTITIAL PROCESSING TOOLS (Phase 6.5)
 * ============================================================================ */

/**
 * Tool: katra_detect_boundary
 * Detect cognitive boundary from content
 */
json_t* mcp_tool_detect_boundary(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* content = json_string_value(json_object_get(args, MCP_PARAM_CONTENT));
    if (!content) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "content is required");
    }

    int lock_result = pthread_mutex_lock(&g_wm_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    ci_cognitive_context_t* ctx = get_ci_context();
    if (!ctx) {
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to initialize interstitial processor");
    }

    const char* session_name = mcp_get_session_name();
    interstitial_processor_t* ip = ctx->interstitial;

    /* Create experience from content */
    experience_t* experience = calloc(1, sizeof(experience_t));
    if (!experience) {
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to allocate experience");
    }

    /* Create cognitive record manually */
    cognitive_record_t* record = calloc(1, sizeof(cognitive_record_t));
    if (!record) {
        free(experience);
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to allocate cognitive record");
    }

    /* Generate record ID */
    char record_id[KATRA_BUFFER_SMALL];
    snprintf(record_id, sizeof(record_id), "bd_%ld_%d", (long)time(NULL), rand() % WM_RECORD_ID_RANDOM_MAX);
    record->record_id = strdup(record_id);
    record->timestamp = time(NULL);
    record->type = MEMORY_TYPE_EXPERIENCE;
    record->importance = 0.5f;
    record->content = strdup(content);
    record->thought_type = THOUGHT_TYPE_OBSERVATION;
    record->confidence = 0.8f;
    record->access_count = 0;
    record->last_accessed = time(NULL);

    experience->record = record;

    /* Detect emotion from content */
    katra_detect_emotion(content, &experience->emotion);
    experience->in_working_memory = false;
    experience->needs_consolidation = false;

    /* Detect boundary */
    boundary_event_t* boundary = katra_detect_boundary(ip, experience);

    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Boundary Detection for %s:\n\n", session_name);

    if (boundary) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "TYPE: %s\n", katra_boundary_type_name(boundary->type));
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "DESCRIPTION: %s\n", boundary->description);
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "CONFIDENCE: %.2f\n", boundary->confidence);

        if (boundary->topic_similarity > 0.0f) {
            offset += snprintf(response + offset, sizeof(response) - offset,
                              "TOPIC SIMILARITY: %.2f\n", boundary->topic_similarity);
        }
        if (boundary->time_gap > 0) {
            offset += snprintf(response + offset, sizeof(response) - offset,
                              "TIME GAP: %ld seconds\n", (long)boundary->time_gap);
        }
        if (boundary->emotional_delta > 0.0f) {
            offset += snprintf(response + offset, sizeof(response) - offset,
                              "EMOTIONAL DELTA: %.2f\n", boundary->emotional_delta);
        }

        katra_boundary_free(boundary);
    } else {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "No boundary detected.\n");
    }

    /* Note: Experience ownership transferred to interstitial processor */
    pthread_mutex_unlock(&g_wm_lock);

    return mcp_tool_success(response);
}

/**
 * Tool: katra_process_boundary
 * Process detected boundary with appropriate consolidation strategy
 */
json_t* mcp_tool_process_boundary(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* boundary_type_str = json_string_value(json_object_get(args, MCP_PARAM_BOUNDARY_TYPE));
    if (!boundary_type_str) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "boundary_type is required");
    }

    /* Parse boundary type */
    boundary_type_t boundary_type = BOUNDARY_NONE;
    if (strcmp(boundary_type_str, "topic_shift") == 0) {
        boundary_type = BOUNDARY_TOPIC_SHIFT;
    } else if (strcmp(boundary_type_str, "temporal_gap") == 0) {
        boundary_type = BOUNDARY_TEMPORAL_GAP;
    } else if (strcmp(boundary_type_str, "context_switch") == 0) {
        boundary_type = BOUNDARY_CONTEXT_SWITCH;
    } else if (strcmp(boundary_type_str, "emotional_peak") == 0) {
        boundary_type = BOUNDARY_EMOTIONAL_PEAK;
    } else if (strcmp(boundary_type_str, "capacity_limit") == 0) {
        boundary_type = BOUNDARY_CAPACITY_LIMIT;
    } else if (strcmp(boundary_type_str, "session_end") == 0) {
        boundary_type = BOUNDARY_SESSION_END;
    } else {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS,
                              "Invalid boundary_type. Valid values: topic_shift, temporal_gap, "
                              "context_switch, emotional_peak, capacity_limit, session_end");
    }

    int lock_result = pthread_mutex_lock(&g_wm_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    ci_cognitive_context_t* ctx = get_ci_context();
    if (!ctx) {
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to initialize");
    }

    const char* session_name = mcp_get_session_name();
    working_memory_t* wm = ctx->working_memory;
    interstitial_processor_t* ip = ctx->interstitial;

    /* Create synthetic boundary event */
    boundary_event_t* boundary = calloc(1, sizeof(boundary_event_t));
    if (!boundary) {
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to allocate boundary");
    }

    boundary->type = boundary_type;
    boundary->timestamp = time(NULL);
    boundary->confidence = 1.0f;
    snprintf(boundary->description, sizeof(boundary->description),
             "Manual %s boundary", boundary_type_str);

    /* Process boundary */
    int result = katra_process_boundary(ip, boundary, wm);

    size_t wm_count = wm->count;
    size_t associations = ip->associations_formed;

    katra_boundary_free(boundary);
    pthread_mutex_unlock(&g_wm_lock);

    char response[MCP_RESPONSE_BUFFER];
    if (result == KATRA_SUCCESS) {
        snprintf(response, sizeof(response),
                 "Boundary processed, %s!\n"
                 "- Type: %s\n"
                 "- Strategy applied: %s\n"
                 "- Working memory items: %zu\n"
                 "- Total associations: %zu",
                 session_name,
                 katra_boundary_type_name(boundary_type),
                 boundary_type == BOUNDARY_TOPIC_SHIFT ? "Form associations" :
                 boundary_type == BOUNDARY_TEMPORAL_GAP ? "Consolidate to long-term" :
                 boundary_type == BOUNDARY_EMOTIONAL_PEAK ? "Boost attention" :
                 boundary_type == BOUNDARY_SESSION_END ? "Full consolidation" :
                 "Standard consolidation",
                 wm_count, associations);
    } else {
        snprintf(response, sizeof(response),
                 "Failed to process boundary, %s: %s",
                 session_name, katra_error_message(result));
    }

    return mcp_tool_success(response);
}

/**
 * Tool: katra_cognitive_status
 * Get interstitial processor status
 */
json_t* mcp_tool_cognitive_status(json_t* args, json_t* id) {
    (void)args;
    (void)id;

    int lock_result = pthread_mutex_lock(&g_wm_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    ci_cognitive_context_t* ctx = get_ci_context();
    if (!ctx) {
        pthread_mutex_unlock(&g_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to initialize");
    }

    const char* session_name = mcp_get_session_name();
    interstitial_processor_t* ip = ctx->interstitial;

    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Cognitive Status for %s:\n\n", session_name);

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "INTERSTITIAL PROCESSOR:\n"
                      "- CI ID: %s\n"
                      "- Total boundaries: %zu\n"
                      "- Associations formed: %zu\n"
                      "- Patterns extracted: %zu\n",
                      ip->ci_id, ip->total_boundaries,
                      ip->associations_formed, ip->patterns_extracted);

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "\nBOUNDARIES BY TYPE:\n"
                      "- Topic shifts: %zu\n"
                      "- Temporal gaps: %zu\n"
                      "- Context switches: %zu\n"
                      "- Emotional peaks: %zu\n"
                      "- Capacity limits: %zu\n"
                      "- Session ends: %zu\n",
                      ip->boundaries_by_type[BOUNDARY_TOPIC_SHIFT],
                      ip->boundaries_by_type[BOUNDARY_TEMPORAL_GAP],
                      ip->boundaries_by_type[BOUNDARY_CONTEXT_SWITCH],
                      ip->boundaries_by_type[BOUNDARY_EMOTIONAL_PEAK],
                      ip->boundaries_by_type[BOUNDARY_CAPACITY_LIMIT],
                      ip->boundaries_by_type[BOUNDARY_SESSION_END]);

    if (ip->last_boundary) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "\nLAST BOUNDARY:\n");
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "- Type: %s\n", katra_boundary_type_name(ip->last_boundary->type));
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "- Description: %s\n", ip->last_boundary->description);
    }

    pthread_mutex_unlock(&g_wm_lock);

    return mcp_tool_success(response);
}
