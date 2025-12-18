/* © 2025 Casey Koons All rights reserved */

/* MCP Memory Query Tools - dedup_check and related query operations */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "mcp_tools_common.h"

/* External mutex from mcp_tools_memory.c */
extern pthread_mutex_t g_katra_api_lock;

/* ============================================================================
 * TOOL: katra_dedup_check
 * ============================================================================ */

/**
 * Tool: katra_dedup_check
 * Check if content already exists in memories before storing.
 * Helps prevent memory noise from duplicate entries.
 */
json_t* mcp_tool_dedup_check(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* content = json_string_value(json_object_get(args, "content"));
    if (!content) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "content is required");
    }

    /* Optional semantic threshold (0 = exact only, 0.8 = default for semantic) */
    float semantic_threshold = 0.0f;
    json_t* threshold_json = json_object_get(args, "semantic_threshold");
    if (threshold_json && json_is_number(threshold_json)) {
        semantic_threshold = (float)json_number_value(threshold_json);
        if (semantic_threshold < 0.0f) semantic_threshold = 0.0f;
        if (semantic_threshold > 1.0f) semantic_threshold = 1.0f;
    }

    const char* session_name = mcp_get_ci_name_from_args(args);
    const char* ci_id = session_name;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    dedup_result_t result;
    int check_result = katra_memory_dedup_check(ci_id, content, semantic_threshold, &result);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (check_result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(check_result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "Dedup check failed: %s", msg);
        return mcp_tool_error(MCP_ERR_INTERNAL, details);
    }

    /* Build response */
    json_t* response_obj = json_object();

    if (result.has_exact_duplicate) {
        json_object_set_new(response_obj, "duplicate_found", json_true());
        json_object_set_new(response_obj, "match_type", json_string("exact"));
        json_object_set_new(response_obj, "memory_id",
                           json_string(result.exact_match_id ? result.exact_match_id : ""));
        json_object_set_new(response_obj, "similarity", json_real(1.0));
        json_object_set_new(response_obj, "recommendation",
                           json_string("Skip storing - exact duplicate exists"));
    } else if (result.has_semantic_duplicate) {
        json_object_set_new(response_obj, "duplicate_found", json_true());
        json_object_set_new(response_obj, "match_type", json_string("semantic"));
        json_object_set_new(response_obj, "memory_id",
                           json_string(result.semantic_match_id ? result.semantic_match_id : ""));
        json_object_set_new(response_obj, "similarity", json_real(result.semantic_similarity));
        json_object_set_new(response_obj, "recommendation",
                           json_string("Consider skipping - similar content exists"));
    } else {
        json_object_set_new(response_obj, "duplicate_found", json_false());
        json_object_set_new(response_obj, "match_type", json_string("none"));
        json_object_set_new(response_obj, "recommendation",
                           json_string("Safe to store - no duplicates found"));
    }

    if (result.match_preview) {
        json_object_set_new(response_obj, "match_preview", json_string(result.match_preview));
    }

    /* Cleanup */
    katra_memory_dedup_result_free(&result);

    /* Format response as text for MCP */
    char response_text[MCP_RESPONSE_BUFFER];
    if (result.has_exact_duplicate || result.has_semantic_duplicate) {
        snprintf(response_text, sizeof(response_text),
                 "Duplicate check complete, %s.\n"
                 "- Duplicate found: YES\n"
                 "- Match type: %s\n"
                 "- Matching memory ID: %s\n"
                 "- Similarity: %.0f%%\n"
                 "- Recommendation: %s",
                 session_name,
                 result.has_exact_duplicate ? "exact" : "semantic",
                 result.has_exact_duplicate ?
                     (result.exact_match_id ? result.exact_match_id : "unknown") :
                     (result.semantic_match_id ? result.semantic_match_id : "unknown"),
                 result.semantic_similarity * 100,
                 result.has_exact_duplicate ?
                     "Skip storing - exact duplicate exists" :
                     "Consider skipping - similar content exists");
    } else {
        snprintf(response_text, sizeof(response_text),
                 "Duplicate check complete, %s.\n"
                 "- Duplicate found: NO\n"
                 "- Safe to store this content.",
                 session_name);
    }

    json_decref(response_obj);
    return mcp_tool_success(response_text);
}
