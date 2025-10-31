/* © 2025 Casey Koons All rights reserved */

/* MCP Phase 5 Tools - placement, impact, user_domain */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_phase5.h"
#include "katra_error.h"
#include "katra_log.h"

/* External mutex from mcp_tools.c */
extern pthread_mutex_t g_katra_api_lock;

/* Helper: Execute Phase 5 query with specified type */
static json_t* execute_phase5_query(const char* query_text, query_type_t type, const char* type_name) {
    if (!query_text) {
        return mcp_tool_error("Missing required argument", "'query' is required");
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error("Internal error", "Failed to acquire mutex lock");
    }

    composition_query_t* query = katra_phase5_create_query(query_text, type);
    if (!query) {
        pthread_mutex_unlock(&g_katra_api_lock);
        const char* msg = "Failed to create composition query";
        const char* details = "Memory allocation failed or invalid query parameters";
        return mcp_tool_error(msg, details);
    }

    int result = katra_phase5_compose(query);

    char* response_text = NULL;
    float confidence = 0.0f;

    if (result == KATRA_SUCCESS && query->result) {
        if (query->result->recommendation && strlen(query->result->recommendation) > 0) {
            response_text = strdup(query->result->recommendation);
        }
        confidence = query->result->confidence.overall;
    }

    katra_phase5_free_query(query);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "%s. %s", msg, suggestion);
        return mcp_tool_error("Composition query failed", details);
    }

    if (!response_text || strlen(response_text) == 0) {
        char default_msg[MCP_RESPONSE_BUFFER];
        snprintf(default_msg, sizeof(default_msg),
                "No %s recommendation available for this query. "
                "Try providing more context or reformulating the question.",
                type_name);
        free(response_text);
        return mcp_tool_success(default_msg);
    }

    /* Build response with confidence */
    char full_response[MCP_RESPONSE_BUFFER];
    snprintf(full_response, sizeof(full_response),
            "%s\n\nConfidence: %.1f%%",
            response_text,
            confidence * 100.0f);

    free(response_text);
    return mcp_tool_success(full_response);
}

/* Tool: katra_placement */
json_t* mcp_tool_placement(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error("Missing required arguments", "");
    }

    const char* query_text = json_string_value(json_object_get(args, "query"));
    return execute_phase5_query(query_text, QUERY_TYPE_PLACEMENT, "placement");
}

/* Tool: katra_impact */
json_t* mcp_tool_impact(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error("Missing required arguments", "");
    }

    const char* query_text = json_string_value(json_object_get(args, "query"));
    return execute_phase5_query(query_text, QUERY_TYPE_IMPACT, "impact");
}

/* Tool: katra_user_domain */
json_t* mcp_tool_user_domain(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error("Missing required arguments", "");
    }

    const char* query_text = json_string_value(json_object_get(args, "query"));
    return execute_phase5_query(query_text, QUERY_TYPE_USER_DOMAIN, "user domain");
}
