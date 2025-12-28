/* © 2025 Casey Koons All rights reserved */

/*
 * Katra Method Wrappers
 *
 * Wrapper implementations that adapt MCP tool handlers to the unified interface.
 * Each wrapper calls the corresponding MCP tool and extracts the result text.
 *
 * IMPORTANT: CI identity is determined ONLY by explicit parameters.
 * The namespace from options is injected as ci_name into every call.
 * No global state, no thread-local state, no magic.
 */

/* System includes */
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_unified.h"
#include "katra_mcp.h"
#include "katra_module.h"
#include "katra_limits.h"

/*
 * Helper: Inject ci_name from options->namespace into params
 *
 * This ensures every MCP tool call has explicit CI identity.
 * The namespace IS the CI name - they are the same thing.
 * Returns a new json object that caller must NOT decref (managed internally).
 */
static json_t* inject_ci_name(json_t* params, const katra_unified_options_t* options) {
    if (!params) {
        params = json_object();
    }

    /* If caller already provided ci_name, respect it */
    if (json_object_get(params, "ci_name")) {
        return params;
    }

    /* Inject namespace as ci_name */
    if (options && options->namespace[0] != '\0' &&
        strcmp(options->namespace, "default") != 0) {
        json_object_set_new(params, "ci_name", json_string(options->namespace));
    }

    return params;
}

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
 * Memory operations - all receive ci_name via inject_ci_name()
 */

json_t* katra_method_remember(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_remember(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_recall(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_recall(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_recent(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_recent(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_digest(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_memory_digest(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_learn(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_learn(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_decide(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_decide(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

/*
 * Identity operations - ci_name injected for proper namespace isolation
 */

json_t* katra_method_register(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_register(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whoami(json_t* params, const katra_unified_options_t* options) {
    fprintf(stderr, "DEBUG: katra_method_whoami entered\n"); fflush(stderr);
    json_t* injected = inject_ci_name(params, options);
    fprintf(stderr, "DEBUG: inject_ci_name done, injected=%p\n", (void*)injected); fflush(stderr);
    json_t* result = mcp_tool_whoami(injected, NULL);
    fprintf(stderr, "DEBUG: mcp_tool_whoami done, result=%p\n", (void*)result); fflush(stderr);
    json_t* extracted = extract_mcp_result(result);
    fprintf(stderr, "DEBUG: extract_mcp_result done, extracted=%p\n", (void*)extracted); fflush(stderr);
    return extracted;
}

json_t* katra_method_status(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_status(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_update_metadata(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_update_metadata(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

/*
 * Communication operations - ci_name identifies the speaker/listener
 */

json_t* katra_method_say(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_say(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_hear(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_hear(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_who_is_here(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_who_is_here(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

/*
 * Configuration operations - ci_name for per-CI configuration
 */

json_t* katra_method_configure_semantic(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_configure_semantic(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_get_semantic_config(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_get_semantic_config(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_get_config(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_get_config(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_regenerate_vectors(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_regenerate_vectors(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

/*
 * Working memory operations - ci_name for per-CI working memory
 */

json_t* katra_method_wm_status(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_wm_status(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_wm_add(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_wm_add(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_wm_decay(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_wm_decay(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_wm_consolidate(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_wm_consolidate(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

/*
 * Cognitive operations - ci_name for per-CI cognitive state
 */

json_t* katra_method_detect_boundary(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_detect_boundary(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_process_boundary(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_process_boundary(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_cognitive_status(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_cognitive_status(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

/*
 * Memory lifecycle operations - ci_name for per-CI memory management
 */

json_t* katra_method_archive(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_archive(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_fade(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_fade(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_forget(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_forget(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

/*
 * Whiteboard operations - ci_name identifies participant
 */

json_t* katra_method_whiteboard_create(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_whiteboard_create(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_status(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_whiteboard_status(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_list(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_whiteboard_list(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_question(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_whiteboard_question(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_propose(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_whiteboard_propose(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_support(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_whiteboard_support(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_vote(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_whiteboard_vote(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_design(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_whiteboard_design(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_review(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_whiteboard_review(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_whiteboard_reconsider(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_whiteboard_reconsider(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

/*
 * Daemon operations - ci_name for per-CI daemon state
 */

json_t* katra_method_daemon_insights(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_daemon_insights(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_daemon_acknowledge(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_daemon_acknowledge(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

json_t* katra_method_daemon_run(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_daemon_run(inject_ci_name(params, options), NULL);
    return extract_mcp_result(result);
}

/*
 * Team and sharing operations - namespace isolation for multi-CI
 */

json_t* katra_method_team_create(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_team_create(inject_ci_name(params, options));
    return extract_mcp_result(result);
}

json_t* katra_method_team_join(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_team_join(inject_ci_name(params, options));
    return extract_mcp_result(result);
}

json_t* katra_method_team_leave(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_team_leave(inject_ci_name(params, options));
    return extract_mcp_result(result);
}

json_t* katra_method_team_list(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_team_list(inject_ci_name(params, options));
    return extract_mcp_result(result);
}

json_t* katra_method_set_isolation(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_set_isolation(inject_ci_name(params, options));
    return extract_mcp_result(result);
}

json_t* katra_method_share_with(json_t* params, const katra_unified_options_t* options) {
    json_t* result = mcp_tool_share_with(inject_ci_name(params, options));
    return extract_mcp_result(result);
}

/*
 * Dynamic module operations - load/unload modules at runtime
 */

json_t* katra_method_modules_list(json_t* params, const katra_unified_options_t* options) {
    json_t* injected = inject_ci_name(params, options);
    const char* ci_name = json_string_value(json_object_get(injected, "ci_name"));
    json_t* result = katra_mcp_modules_list(injected, ci_name);
    return extract_mcp_result(result);
}

json_t* katra_method_modules_load(json_t* params, const katra_unified_options_t* options) {
    json_t* injected = inject_ci_name(params, options);
    const char* ci_name = json_string_value(json_object_get(injected, "ci_name"));
    json_t* result = katra_mcp_modules_load(injected, ci_name);
    return extract_mcp_result(result);
}

json_t* katra_method_modules_unload(json_t* params, const katra_unified_options_t* options) {
    json_t* injected = inject_ci_name(params, options);
    const char* ci_name = json_string_value(json_object_get(injected, "ci_name"));
    json_t* result = katra_mcp_modules_unload(injected, ci_name);
    return extract_mcp_result(result);
}

json_t* katra_method_modules_reload(json_t* params, const katra_unified_options_t* options) {
    json_t* injected = inject_ci_name(params, options);
    const char* ci_name = json_string_value(json_object_get(injected, "ci_name"));
    json_t* result = katra_mcp_modules_reload(injected, ci_name);
    return extract_mcp_result(result);
}

json_t* katra_method_modules_info(json_t* params, const katra_unified_options_t* options) {
    json_t* injected = inject_ci_name(params, options);
    const char* ci_name = json_string_value(json_object_get(injected, "ci_name"));
    json_t* result = katra_mcp_modules_info(injected, ci_name);
    return extract_mcp_result(result);
}
