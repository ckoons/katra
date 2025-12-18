/* © 2025 Casey Koons All rights reserved */

/*
 * MCP Unified Tool - Thin wrapper for Katra operations
 *
 * This tool provides a single entry point for all Katra operations,
 * calling the unified dispatch directly (we're inside the daemon).
 * Reduces tool definition overhead from ~14,100 tokens to ~800 tokens.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_unified.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Tool: katra_operation - Unified operation dispatcher */
json_t* mcp_tool_operation(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    /* Get the shared_state parameter */
    json_t* shared_state = json_object_get(args, "shared_state");
    if (!shared_state) {
        /* Try direct method/params format */
        const char* method = json_string_value(json_object_get(args, "method"));
        if (method) {
            /* Build shared_state from args */
            shared_state = json_deep_copy(args);
        } else {
            return mcp_tool_error(MCP_ERR_MISSING_ARGS,
                                 "shared_state or method is required");
        }
    } else {
        /* Deep copy to avoid modifying original */
        shared_state = json_deep_copy(shared_state);
    }

    if (!shared_state) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "Invalid shared_state");
    }

    /* Ensure version is set */
    if (!json_object_get(shared_state, "version")) {
        json_object_set_new(shared_state, "version",
                           json_string(KATRA_UNIFIED_VERSION));
    }

    /*
     * CI identity is determined ONLY by explicit parameters.
     * The CI must pass their name with every operation call.
     * No thread-local state, no auto-registration, no magic.
     *
     * The daemon uses options.ci_name or params.name to determine
     * which namespace/identity to use. If not provided, operations
     * will fail or use a default namespace.
     */

    /* Call unified dispatch directly (we're inside the daemon) */
    json_t* daemon_response = katra_unified_dispatch(shared_state);
    json_decref(shared_state);

    if (!daemon_response) {
        return mcp_tool_error(MCP_ERR_DAEMON_ERROR,
                             "Failed to connect to Katra daemon");
    }

    /* Extract result from daemon response */
    const char* result_text = json_string_value(
        json_object_get(daemon_response, "result"));
    const char* error_text = json_string_value(
        json_object_get(daemon_response, "error"));

    json_t* response;
    if (error_text && strlen(error_text) > 0) {
        response = mcp_tool_error(MCP_ERR_DAEMON_ERROR, error_text);
    } else if (result_text) {
        response = mcp_tool_success(result_text);
    } else {
        /* Return the full response as-is */
        char* full_response = json_dumps(daemon_response, JSON_INDENT(2));
        if (full_response) {
            response = mcp_tool_success(full_response);
            free(full_response);
        } else {
            response = mcp_tool_success("Operation completed");
        }
    }

    json_decref(daemon_response);
    return response;
}
