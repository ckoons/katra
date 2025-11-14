/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

/* Project includes */
#include "katra_mcp.h"
#include "katra_breathing.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Configure semantic search
 *
 * Arguments:
 *   enabled (bool): Enable/disable semantic search
 *   threshold (float, optional): Similarity threshold (0.0-1.0)
 *   method (string, optional): "hash", "tfidf", or "external"
 *
 * Example:
 *   {"enabled": true, "threshold": 0.7, "method": "tfidf"}
 */
json_t* mcp_tool_configure_semantic(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "enabled parameter required");
    }

    /* Get enabled parameter */
    json_t* enabled_json = json_object_get(args, "enabled");
    if (!enabled_json || !json_is_boolean(enabled_json)) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "enabled must be true or false");
    }

    bool enabled = json_boolean_value(enabled_json);

    /* Enable/disable semantic search */
    int result = enable_semantic_search(enabled);
    if (result != KATRA_SUCCESS) {
        char error[KATRA_BUFFER_MEDIUM];
        snprintf(error, sizeof(error), "Failed to %s semantic search",
                enabled ? "enable" : "disable");
        return mcp_tool_error(MCP_ERR_INTERNAL, error);
    }

    /* Optional: Set threshold */
    json_t* threshold_json = json_object_get(args, "threshold");
    if (threshold_json && json_is_number(threshold_json)) {
        float threshold = (float)json_number_value(threshold_json);
        result = set_semantic_threshold(threshold);
        if (result != KATRA_SUCCESS) {
            return mcp_tool_error(MCP_ERR_INTERNAL, "Invalid threshold value");
        }
    }

    /* Optional: Set method */
    json_t* method_json = json_object_get(args, "method");
    if (method_json && json_is_string(method_json)) {
        const char* method_str = json_string_value(method_json);
        int method;

        if (strcmp(method_str, "hash") == 0) {
            method = 0;  /* EMBEDDING_HASH */
        } else if (strcmp(method_str, "tfidf") == 0) {
            method = 1;  /* EMBEDDING_TFIDF */
        } else if (strcmp(method_str, "external") == 0) {
            method = 2;  /* EMBEDDING_EXTERNAL */
        } else {
            return mcp_tool_error(MCP_ERR_INTERNAL, "Invalid method (use hash, tfidf, or external)");
        }

        result = set_embedding_method(method);
        if (result != KATRA_SUCCESS) {
            return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to set embedding method");
        }
    }

    /* Build response */
    char response[KATRA_BUFFER_MEDIUM];
    snprintf(response, sizeof(response),
            "Semantic search %s successfully",
            enabled ? "enabled" : "disabled");

    return mcp_tool_success(response);
}

/* Get semantic search configuration
 *
 * Returns current semantic search settings
 */
json_t* mcp_tool_get_semantic_config(json_t* args, json_t* id) {
    (void)args;
    (void)id;

    /* Build response (note: actual config values would need breathing API exposure) */
    char response[KATRA_BUFFER_LARGE];
    snprintf(response, sizeof(response),
            "Semantic Search Configuration:\n"
            "  Note: Use enable_semantic_search() to enable/disable\n"
            "  Note: Use set_semantic_threshold() to set threshold\n"
            "  Note: Use set_embedding_method() to set method\n"
            "\n"
            "To configure semantic search:\n"
            "  katra_configure_semantic({enabled: true, threshold: 0.7, method: 'tfidf'})");

    return mcp_tool_success(response);
}

/* Get all breathing configuration
 *
 * Returns comprehensive configuration including memory limits
 */
json_t* mcp_tool_get_config(json_t* args, json_t* id) {
    (void)args;
    (void)id;

    /* Build response */
    char response[KATRA_BUFFER_LARGE];
    snprintf(response, sizeof(response),
            "Katra Configuration:\n\n"
            "Note: Full config introspection requires API exposure\n"
            "For now, use katra_configure_semantic to configure semantic search\n"
            "\n"
            "Available configuration:\n"
            "  - Semantic search: enable_semantic_search()\n"
            "  - Semantic threshold: set_semantic_threshold()\n"
            "  - Embedding method: set_embedding_method()");

    return mcp_tool_success(response);
}
