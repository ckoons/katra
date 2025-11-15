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
