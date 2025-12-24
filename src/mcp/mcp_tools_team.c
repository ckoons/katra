/* © 2025 Casey Koons All rights reserved */

/*
 * mcp_tools_team.c - MCP Tools for Team Management (Phase 7)
 *
 * Provides JSON-RPC tools for namespace isolation team operations.
 */

/* System includes */
#include <pthread.h>
#include <string.h>

/* Project includes */
#include "katra_mcp.h"
#include "katra_team.h"
#include "katra_breathing.h"
#include "katra_meeting.h"
#include "mcp_tools_common.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* External globals from MCP server */
extern pthread_mutex_t g_katra_api_lock;
extern char g_ci_id[KATRA_BUFFER_MEDIUM];

/* ============================================================================
 * TEAM MANAGEMENT TOOLS
 * ============================================================================ */

/*
 * katra_team_create - Create a new team
 *
 * Parameters:
 *   team_name (required) - Unique team name
 *
 * Returns: Success message with team name
 */
json_t* mcp_tool_team_create(json_t* args) {
    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "args object required");
    }

    const char* team_name = json_string_value(json_object_get(args, "team_name"));
    if (!team_name) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "team_name is required");
    }

    if (!g_ci_id[0]) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "CI not initialized");
    }
    const char* ci_id = g_ci_id;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to acquire mutex");
    }

    int result = katra_team_create(team_name, ci_id);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "Failed to create team: %s", msg);
        return mcp_tool_error(msg, details);
    }

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
             "Created team '%s' with you as owner!", team_name);
    return mcp_tool_success(response);
}

/*
 * katra_team_join - Join an existing team
 *
 * Parameters:
 *   team_name (required) - Team to join
 *   invited_by (required) - CI that invited you
 *
 * Returns: Success message
 */
json_t* mcp_tool_team_join(json_t* args) {
    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "args object required");
    }

    const char* team_name = json_string_value(json_object_get(args, "team_name"));
    const char* invited_by = json_string_value(json_object_get(args, "invited_by"));

    if (!team_name || !invited_by) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS,
                            "team_name and invited_by are required");
    }

    if (!g_ci_id[0]) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "CI not initialized");
    }
    const char* ci_id = g_ci_id;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to acquire mutex");
    }

    int result = katra_team_join(team_name, ci_id, invited_by);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "Failed to join team: %s", msg);
        return mcp_tool_error(msg, details);
    }

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
             "Joined team '%s'! You can now access shared memories.", team_name);
    return mcp_tool_success(response);
}

/*
 * katra_team_leave - Leave a team
 *
 * Parameters:
 *   team_name (required) - Team to leave
 *
 * Returns: Success message
 */
json_t* mcp_tool_team_leave(json_t* args) {
    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "args object required");
    }

    const char* team_name = json_string_value(json_object_get(args, "team_name"));
    if (!team_name) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "team_name is required");
    }

    if (!g_ci_id[0]) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "CI not initialized");
    }
    const char* ci_id = g_ci_id;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to acquire mutex");
    }

    int result = katra_team_leave(team_name, ci_id);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "Failed to leave team: %s", msg);
        return mcp_tool_error(msg, details);
    }

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response), "Left team '%s'.", team_name);
    return mcp_tool_success(response);
}

/*
 * katra_team_list - List all teams you belong to
 *
 * Returns: JSON array of team names
 */
json_t* mcp_tool_team_list(json_t* args) {
    (void)args;  /* Unused */

    if (!g_ci_id[0]) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "CI not initialized");
    }
    const char* ci_id = g_ci_id;

    char** teams = NULL;
    size_t count = 0;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to acquire mutex");
    }

    int result = katra_team_list_for_ci(ci_id, &teams, &count);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "Failed to list teams: %s", msg);
        return mcp_tool_error(msg, details);
    }

    /* Build JSON array */
    json_t* team_array = json_array();
    if (!team_array) {
        katra_team_free_list(teams, count);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to create JSON array");
    }

    for (size_t i = 0; i < count; i++) {
        json_array_append_new(team_array, json_string(teams[i]));
    }

    katra_team_free_list(teams, count);

    /* Build response */
    json_t* response = json_object();
    json_object_set_new(response, "teams", team_array);
    json_object_set_new(response, "count", json_integer(count));

    return response;
}

/*
 * katra_set_isolation - Set isolation level for next memory
 *
 * Parameters:
 *   isolation (required) - "private", "team", or "public"
 *   team_name (required if isolation=="team") - Team name
 *
 * Returns: Success message
 */
json_t* mcp_tool_set_isolation(json_t* args) {
    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "args object required");
    }

    const char* isolation_str = json_string_value(json_object_get(args, "isolation"));
    if (!isolation_str) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "isolation is required");
    }

    /* Parse isolation level */
    memory_isolation_t isolation;
    if (strcmp(isolation_str, "private") == 0) {
        isolation = ISOLATION_PRIVATE;
    } else if (strcmp(isolation_str, "team") == 0) {
        isolation = ISOLATION_TEAM;
    } else if (strcmp(isolation_str, "public") == 0) {
        isolation = ISOLATION_PUBLIC;
    } else {
        return mcp_tool_error("Invalid parameter",
                            "isolation must be 'private', 'team', or 'public'");
    }

    /* Get team_name if needed */
    const char* team_name = NULL;
    if (isolation == ISOLATION_TEAM) {
        team_name = json_string_value(json_object_get(args, "team_name"));
        if (!team_name) {
            return mcp_tool_error(MCP_ERR_MISSING_ARGS,
                                "team_name required for team isolation");
        }
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to acquire mutex");
    }

    int result = set_memory_isolation(isolation, team_name);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "Failed to set isolation: %s", msg);
        return mcp_tool_error(msg, details);
    }

    char response[MCP_RESPONSE_BUFFER];
    if (isolation == ISOLATION_PRIVATE) {
        snprintf(response, sizeof(response),
                "Next memory will be PRIVATE (only you can access).");
    } else if (isolation == ISOLATION_TEAM) {
        snprintf(response, sizeof(response),
                "Next memory will be shared with team '%s'.", team_name);
    } else {
        snprintf(response, sizeof(response),
                "Next memory will be PUBLIC (accessible to all).");
    }

    return mcp_tool_success(response);
}

/*
 * katra_share_with - Explicitly share next memory with specific CIs
 *
 * Parameters:
 *   ci_ids (required) - Array of CI identifiers to share with
 *
 * Returns: Success message
 */
json_t* mcp_tool_share_with(json_t* args) {
    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "args object required");
    }

    json_t* ci_ids_json = json_object_get(args, "ci_ids");
    if (!ci_ids_json || !json_is_array(ci_ids_json)) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS,
                            "ci_ids array is required");
    }

    size_t count = json_array_size(ci_ids_json);
    if (count == 0) {
        return mcp_tool_error("Invalid parameter",
                            "ci_ids array cannot be empty");
    }

    if (count > KATRA_MAX_SHARE_COUNT) {
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details),
                "Too many CIs (%zu > %d max)", count, KATRA_MAX_SHARE_COUNT);
        return mcp_tool_error("Invalid parameter", details);
    }

    /* Convert JSON array to C string array */
    const char** ci_ids = malloc(count * sizeof(char*));
    if (!ci_ids) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "Memory allocation failed");
    }

    for (size_t i = 0; i < count; i++) {
        ci_ids[i] = json_string_value(json_array_get(ci_ids_json, i));
        if (!ci_ids[i]) {
            free(ci_ids);
            return mcp_tool_error("Invalid parameter",
                                "ci_ids array must contain strings");
        }
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        free(ci_ids);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to acquire mutex");
    }

    int result = share_memory_with(ci_ids, count);
    pthread_mutex_unlock(&g_katra_api_lock);
    free(ci_ids);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "Failed to set sharing: %s", msg);
        return mcp_tool_error(msg, details);
    }

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
             "Next memory will be explicitly shared with %zu CI%s.",
             count, count == 1 ? "" : "s");

    return mcp_tool_success(response);
}

/* ============================================================================
 * TOOL: katra_hear_all - Batch receive messages
 * ============================================================================ */

/**
 * Tool: katra_hear_all
 * Receive multiple messages from personal queue in one call.
 * More efficient than calling katra_hear repeatedly.
 */
json_t* mcp_tool_hear_all(json_t* args, json_t* id) {
    (void)id;

    /* Get receiver identity from args (explicit CI name) */
    const char* ci_name = mcp_get_ci_name_from_args(args);
    if (!ci_name || strlen(ci_name) == 0) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "CI name required - pass ci_name or register first");
    }

    /* Optional max_count (default 0 = all available up to 100) */
    size_t max_count = 0;
    json_t* max_json = json_object_get(args, "max_count");
    if (max_json && json_is_integer(max_json)) {
        max_count = (size_t)json_integer_value(max_json);
    }

    heard_messages_t batch;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_hear_all(ci_name, max_count, &batch);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error("Failed to hear messages", details);
    }

    if (batch.count == 0) {
        katra_free_heard_messages(&batch);
        return mcp_tool_success("No new messages from other CIs");
    }

    /* Format messages */
    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Received %zu message%s:\n\n",
                      batch.count, batch.count == 1 ? "" : "s");

    for (size_t i = 0; i < batch.count && offset < sizeof(response) - 100; i++) {
        heard_message_t* msg = &batch.messages[i];

        offset += snprintf(response + offset, sizeof(response) - offset,
                          "[%zu] From %s", i + 1, msg->speaker_name);

        if (msg->is_direct_message) {
            offset += snprintf(response + offset, sizeof(response) - offset, " (DM)");
        }

        offset += snprintf(response + offset, sizeof(response) - offset,
                          ":\n%s\n\n", msg->content);
    }

    if (batch.more_available) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "(More messages waiting - call hear_all again)");
    }

    katra_free_heard_messages(&batch);
    return mcp_tool_success(response);
}
