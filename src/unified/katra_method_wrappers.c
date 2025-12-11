/* © 2025 Casey Koons All rights reserved */

/*
 * Katra Method Wrappers
 *
 * Wrapper implementations that adapt MCP tool handlers to the unified interface.
 * Each wrapper calls the corresponding MCP tool and extracts the result text.
 */

/* System includes */
#include <stdlib.h>

/* Project includes */
#include "katra_unified.h"
#include "katra_mcp.h"

/* Helper: Extract result from MCP tool response */
static json_t* extract_mcp_result(json_t* mcp_response) {
    if (!mcp_response) {
        return NULL;
    }

    /* MCP tool responses have content array with text */
    json_t* content = json_object_get(mcp_response, "content");
    if (content && json_is_array(content) && json_array_size(content) > 0) {
        json_t* first = json_array_get(content, 0);
        json_t* text = json_object_get(first, "text");
        if (text && json_is_string(text)) {
            json_t* result = json_string(json_string_value(text));
            json_decref(mcp_response);
            return result;
        }
    }

    /* Return as-is if not standard format */
    return mcp_response;
}

/*
 * Memory operations
 */

json_t* katra_method_remember(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_remember(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_recall(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_recall(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_recent(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_recent(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_digest(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_memory_digest(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_learn(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_learn(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_decide(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_decide(params, NULL);
    return extract_mcp_result(result);
}

/*
 * Identity operations
 */

json_t* katra_method_register(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_register(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whoami(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whoami(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_status(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_status(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_update_metadata(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_update_metadata(params, NULL);
    return extract_mcp_result(result);
}

/*
 * Communication operations
 */

json_t* katra_method_say(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_say(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_hear(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_hear(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_who_is_here(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_who_is_here(params, NULL);
    return extract_mcp_result(result);
}

/*
 * Configuration operations
 */

json_t* katra_method_configure_semantic(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_configure_semantic(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_get_semantic_config(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_get_semantic_config(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_get_config(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_get_config(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_regenerate_vectors(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_regenerate_vectors(params, NULL);
    return extract_mcp_result(result);
}

/*
 * Working memory operations
 */

json_t* katra_method_wm_status(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_wm_status(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_wm_add(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_wm_add(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_wm_decay(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_wm_decay(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_wm_consolidate(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_wm_consolidate(params, NULL);
    return extract_mcp_result(result);
}

/*
 * Cognitive operations
 */

json_t* katra_method_detect_boundary(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_detect_boundary(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_process_boundary(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_process_boundary(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_cognitive_status(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_cognitive_status(params, NULL);
    return extract_mcp_result(result);
}

/*
 * Memory lifecycle operations
 */

json_t* katra_method_archive(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_archive(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_fade(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_fade(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_forget(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_forget(params, NULL);
    return extract_mcp_result(result);
}

/*
 * Whiteboard operations
 */

json_t* katra_method_whiteboard_create(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_create(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_status(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_status(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_list(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_list(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_question(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_question(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_propose(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_propose(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_support(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_support(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_vote(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_vote(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_design(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_design(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_review(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_review(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_reconsider(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_reconsider(params, NULL);
    return extract_mcp_result(result);
}

/*
 * Daemon operations
 */

json_t* katra_method_daemon_insights(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_daemon_insights(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_daemon_acknowledge(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_daemon_acknowledge(params, NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_daemon_run(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_daemon_run(params, NULL);
    return extract_mcp_result(result);
}
