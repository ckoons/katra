/* © 2025 Casey Koons All rights reserved */

/* MCP Interstitial Processing Tools - Phase 6.5 */

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

/* Constants for record ID generation */
#define WM_RECORD_ID_RANDOM_MAX 10000

/* External access to CI context */
extern pthread_mutex_t mcp_wm_lock;
extern void* mcp_get_ci_cognitive_context_for(const char* ci_id);
extern working_memory_t* mcp_ctx_get_working_memory(void* ctx);
extern interstitial_processor_t* mcp_ctx_get_interstitial(void* ctx);

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

    const char* session_name = mcp_get_ci_name_from_args(args);

    int lock_result = pthread_mutex_lock(&mcp_wm_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    void* ctx = mcp_get_ci_cognitive_context_for(session_name);
    if (!ctx) {
        pthread_mutex_unlock(&mcp_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to initialize interstitial processor");
    }

    interstitial_processor_t* ip = mcp_ctx_get_interstitial(ctx);

    /* Create experience from content */
    experience_t* experience = calloc(1, sizeof(experience_t));
    if (!experience) {
        pthread_mutex_unlock(&mcp_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to allocate experience");
    }

    /* Create cognitive record manually */
    cognitive_record_t* record = calloc(1, sizeof(cognitive_record_t));
    if (!record) {
        free(experience);
        pthread_mutex_unlock(&mcp_wm_lock);
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
    pthread_mutex_unlock(&mcp_wm_lock);

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

    const char* session_name = mcp_get_ci_name_from_args(args);

    int lock_result = pthread_mutex_lock(&mcp_wm_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    void* ctx = mcp_get_ci_cognitive_context_for(session_name);
    if (!ctx) {
        pthread_mutex_unlock(&mcp_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to initialize");
    }

    working_memory_t* wm = mcp_ctx_get_working_memory(ctx);
    interstitial_processor_t* ip = mcp_ctx_get_interstitial(ctx);

    /* Create synthetic boundary event */
    boundary_event_t* boundary = calloc(1, sizeof(boundary_event_t));
    if (!boundary) {
        pthread_mutex_unlock(&mcp_wm_lock);
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
    pthread_mutex_unlock(&mcp_wm_lock);

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
    (void)id;

    const char* session_name = mcp_get_ci_name_from_args(args);

    int lock_result = pthread_mutex_lock(&mcp_wm_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    void* ctx = mcp_get_ci_cognitive_context_for(session_name);
    if (!ctx) {
        pthread_mutex_unlock(&mcp_wm_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to initialize");
    }

    interstitial_processor_t* ip = mcp_ctx_get_interstitial(ctx);

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

    pthread_mutex_unlock(&mcp_wm_lock);

    return mcp_tool_success(response);
}
