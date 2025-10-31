/* © 2025 Casey Koons All rights reserved */

/* MCP Resources - working-context, session-info */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_breathing.h"
#include "katra_error.h"
#include "katra_log.h"

/* External mutex from mcp_tools.c */
extern pthread_mutex_t g_katra_api_lock;

/* Resource: working-context */
json_t* mcp_resource_working_context(json_t* id) {
    pthread_mutex_lock(&g_katra_api_lock);
    char* context = get_working_context();
    pthread_mutex_unlock(&g_katra_api_lock);

    if (!context) {
        return mcp_error_response(id, MCP_ERROR_INTERNAL,
                                 "Failed to get working context",
                                 "Memory allocation failed or session not active");
    }

    /* Build resource response */
    json_t* contents_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, "uri", json_string("katra://context/working"));
    json_object_set_new(content_item, "mimeType", json_string("text/plain"));
    json_object_set_new(content_item, "text", json_string(context));

    json_array_append_new(contents_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, "contents", contents_array);

    /* CRITICAL: Free malloc'd string from get_working_context() */
    free(context);

    return mcp_success_response(id, result);
}

/* Resource: session-info */
json_t* mcp_resource_session_info(json_t* id) {
    katra_session_info_t info;

    pthread_mutex_lock(&g_katra_api_lock);
    int katra_result = katra_get_session_info(&info);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (katra_result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(katra_result);
        return mcp_error_response(id, MCP_ERROR_INTERNAL,
                                 "Failed to get session info", msg);
    }

    /* Format session info as text */
    char session_text[MCP_RESPONSE_BUFFER];
    char start_time_str[64];
    char last_activity_str[64];

    /* Format timestamps */
    struct tm* tm_info = localtime(&info.start_time);
    if (tm_info) {
        strftime(start_time_str, sizeof(start_time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strncpy(start_time_str, "unknown", sizeof(start_time_str) - 1);
    }

    tm_info = localtime(&info.last_activity);
    if (tm_info) {
        strftime(last_activity_str, sizeof(last_activity_str), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        strncpy(last_activity_str, "unknown", sizeof(last_activity_str) - 1);
    }

    /* Calculate session duration */
    time_t now = time(NULL);
    long duration_seconds = (long)(now - info.start_time);
    long duration_minutes = duration_seconds / 60;
    long duration_hours = duration_minutes / 60;

    snprintf(session_text, sizeof(session_text),
            "Katra Session Information\n"
            "========================\n\n"
            "Session ID: %s\n"
            "CI Identity: %s\n"
            "Status: %s\n\n"
            "Timing\n"
            "------\n"
            "Started: %s\n"
            "Duration: %ldh %ldm\n"
            "Last Activity: %s\n\n"
            "Activity\n"
            "--------\n"
            "Memories Added: %zu\n"
            "Queries Processed: %zu\n",
            info.session_id,
            info.ci_id,
            info.is_active ? "Active" : "Inactive",
            start_time_str,
            duration_hours, duration_minutes % 60,
            last_activity_str,
            info.memories_added,
            info.queries_processed);

    /* Build resource response */
    json_t* contents_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, "uri", json_string("katra://session/info"));
    json_object_set_new(content_item, "mimeType", json_string("text/plain"));
    json_object_set_new(content_item, "text", json_string(session_text));

    json_array_append_new(contents_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, "contents", contents_array);

    return mcp_success_response(id, result);
}
