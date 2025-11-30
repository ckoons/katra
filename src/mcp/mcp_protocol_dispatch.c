/* © 2025 Casey Koons All rights reserved */

/* MCP Protocol Dispatch - Request routing and handlers */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_limits.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_hooks.h"

/* Handle tools/call request */
json_t* mcp_handle_tools_call(json_t* request) {
    json_t* id = json_object_get(request, MCP_FIELD_ID);
    json_t* params = json_object_get(request, MCP_FIELD_PARAMS);

    if (!params) {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, MCP_ERR_MISSING_PARAMS, NULL);
    }

    const char* tool_name = json_string_value(json_object_get(params, MCP_FIELD_NAME));
    json_t* args = json_object_get(params, MCP_FIELD_ARGUMENTS);

    if (!tool_name) {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, MCP_ERR_MISSING_TOOL_NAME, NULL);
    }

    /* Trigger turn start hook (autonomic breathing) */
    katra_hook_turn_start();

    /* Dispatch to tool implementation */
    json_t* tool_result = NULL;

    if (strcmp(tool_name, MCP_TOOL_REMEMBER) == 0) {
        tool_result = mcp_tool_remember(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_RECALL) == 0) {
        tool_result = mcp_tool_recall(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_RECENT) == 0) {
        tool_result = mcp_tool_recent(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_MEMORY_DIGEST) == 0) {
        tool_result = mcp_tool_memory_digest(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_LEARN) == 0) {
        tool_result = mcp_tool_learn(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_DECIDE) == 0) {
        tool_result = mcp_tool_decide(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_REGISTER) == 0) {
        tool_result = mcp_tool_register(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WHOAMI) == 0) {
        tool_result = mcp_tool_whoami(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_STATUS) == 0) {
        tool_result = mcp_tool_status(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_UPDATE_METADATA) == 0) {
        tool_result = mcp_tool_update_metadata(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_SAY) == 0) {
        tool_result = mcp_tool_say(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_HEAR) == 0) {
        tool_result = mcp_tool_hear(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WHO_IS_HERE) == 0) {
        tool_result = mcp_tool_who_is_here(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_CONFIGURE_SEMANTIC) == 0) {
        tool_result = mcp_tool_configure_semantic(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_GET_SEMANTIC_CONFIG) == 0) {
        tool_result = mcp_tool_get_semantic_config(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_GET_CONFIG) == 0) {
        tool_result = mcp_tool_get_config(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_REGENERATE_VECTORS) == 0) {
        tool_result = mcp_tool_regenerate_vectors(args, id);
    /* Working Memory Tools (Phase 6.4) */
    } else if (strcmp(tool_name, MCP_TOOL_WM_STATUS) == 0) {
        tool_result = mcp_tool_wm_status(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WM_ADD) == 0) {
        tool_result = mcp_tool_wm_add(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WM_DECAY) == 0) {
        tool_result = mcp_tool_wm_decay(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WM_CONSOLIDATE) == 0) {
        tool_result = mcp_tool_wm_consolidate(args, id);
    /* Interstitial Processing Tools (Phase 6.5) */
    } else if (strcmp(tool_name, MCP_TOOL_DETECT_BOUNDARY) == 0) {
        tool_result = mcp_tool_detect_boundary(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_PROCESS_BOUNDARY) == 0) {
        tool_result = mcp_tool_process_boundary(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_COGNITIVE_STATUS) == 0) {
        tool_result = mcp_tool_cognitive_status(args, id);
    /* Memory Lifecycle Tools (Phase 7.1) */
    } else if (strcmp(tool_name, MCP_TOOL_ARCHIVE) == 0) {
        tool_result = mcp_tool_archive(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_FADE) == 0) {
        tool_result = mcp_tool_fade(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_FORGET) == 0) {
        tool_result = mcp_tool_forget(args, id);
    /* Whiteboard Tools (Phase 8) */
    } else if (strcmp(tool_name, MCP_TOOL_WB_CREATE) == 0) {
        tool_result = mcp_tool_whiteboard_create(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WB_STATUS) == 0) {
        tool_result = mcp_tool_whiteboard_status(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WB_LIST) == 0) {
        tool_result = mcp_tool_whiteboard_list(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WB_QUESTION) == 0) {
        tool_result = mcp_tool_whiteboard_question(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WB_PROPOSE) == 0) {
        tool_result = mcp_tool_whiteboard_propose(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WB_SUPPORT) == 0) {
        tool_result = mcp_tool_whiteboard_support(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WB_VOTE) == 0) {
        tool_result = mcp_tool_whiteboard_vote(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WB_DESIGN) == 0) {
        tool_result = mcp_tool_whiteboard_design(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WB_REVIEW) == 0) {
        tool_result = mcp_tool_whiteboard_review(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_WB_RECONSIDER) == 0) {
        tool_result = mcp_tool_whiteboard_reconsider(args, id);
    /* Daemon Tools (Phase 9) */
    } else if (strcmp(tool_name, MCP_TOOL_DAEMON_INSIGHTS) == 0) {
        tool_result = mcp_tool_daemon_insights(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_DAEMON_ACKNOWLEDGE) == 0) {
        tool_result = mcp_tool_daemon_acknowledge(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_DAEMON_RUN) == 0) {
        tool_result = mcp_tool_daemon_run(args, id);
    } else {
        katra_hook_turn_end();  /* Trigger turn end hook before error return */
        return mcp_error_response(id, MCP_ERROR_METHOD_NOT_FOUND, MCP_ERR_UNKNOWN_TOOL, tool_name);
    }

    /* Trigger turn end hook (autonomic breathing) */
    katra_hook_turn_end();

    return mcp_success_response(id, tool_result);
}

/* Handle resources/read request */
json_t* mcp_handle_resources_read(json_t* request) {
    json_t* id = json_object_get(request, MCP_FIELD_ID);
    json_t* params = json_object_get(request, MCP_FIELD_PARAMS);

    if (!params) {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, MCP_ERR_MISSING_PARAMS, NULL);
    }

    const char* uri = json_string_value(json_object_get(params, MCP_FIELD_URI));

    if (!uri) {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, MCP_ERR_MISSING_URI, NULL);
    }

    /* Dispatch to resource implementation */
    if (strcmp(uri, MCP_RESOURCE_URI_WELCOME) == 0) {
        return mcp_resource_welcome(id);
    } else if (strcmp(uri, MCP_RESOURCE_URI_WORKING_CONTEXT) == 0) {
        return mcp_resource_working_context(id);
    } else if (strcmp(uri, MCP_RESOURCE_URI_CONTEXT_SNAPSHOT) == 0) {
        return mcp_resource_context_snapshot(id);
    } else if (strcmp(uri, MCP_RESOURCE_URI_SESSION_INFO) == 0) {
        return mcp_resource_session_info(id);
    } else if (strcmp(uri, MCP_RESOURCE_URI_MEMORIES_THIS_TURN) == 0) {
        return mcp_resource_memories_this_turn(id);
    } else if (strcmp(uri, MCP_RESOURCE_URI_MEMORIES_THIS_SESSION) == 0) {
        return mcp_resource_memories_this_session(id);
    } else if (strncmp(uri, "katra://personas/", 17) == 0) {
        /* Dynamic persona file resources: katra://personas/{name}/{file} */
        const char* path_part = uri + 17;  /* Skip "katra://personas/" */

        /* Find next slash to extract persona name */
        const char* slash = strchr(path_part, '/');
        if (!slash) {
            return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS,
                                     "Invalid persona resource URI format", uri);
        }

        /* Extract persona name */
        size_t name_len = (size_t)(slash - path_part);
        if (name_len == 0 || name_len >= KATRA_BUFFER_SMALL) {
            return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS,
                                     "Persona name too long or empty", uri);
        }

        char persona_name[KATRA_BUFFER_SMALL];
        strncpy(persona_name, path_part, name_len);
        persona_name[name_len] = '\0';

        /* Extract file type (sunrise, tools, discoveries) */
        const char* file_type = slash + 1;

        /* Validate file type */
        if (strcmp(file_type, "sunrise") != 0 &&
            strcmp(file_type, "tools") != 0 &&
            strcmp(file_type, "discoveries") != 0) {
            return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS,
                                     "Unknown persona file type (must be sunrise, tools, or discoveries)",
                                     file_type);
        }

        return mcp_resource_persona_file(id, persona_name, file_type);
    } else {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, MCP_ERR_UNKNOWN_RESOURCE, uri);
    }
}
