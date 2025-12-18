/* © 2025 Casey Koons All rights reserved */

/* MCP Protocol Implementation - JSON-RPC 2.0 handlers */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_limits.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_hooks.h"
#include "katra_breathing.h"
#include "katra_identity.h"
#include "katra_meeting.h"

/* Parse JSON-RPC request from string */
json_t* mcp_parse_request(const char* json_str) {
    if (!json_str) {
        return NULL;
    }

    json_error_t error;
    json_t* request = json_loads(json_str, 0, &error);

    if (!request) {
        LOG_ERROR("JSON parse error: %s (line %d, column %d)",
                 error.text, error.line, error.column);
        return NULL;
    }

    return request;
}

/* Build success response */
json_t* mcp_success_response(json_t* id, json_t* result) {
    json_t* response = json_object();
    json_object_set_new(response, MCP_FIELD_JSONRPC, json_string(MCP_JSONRPC_VERSION));

    if (id) {
        json_object_set(response, MCP_FIELD_ID, id);
    } else {
        json_object_set_new(response, MCP_FIELD_ID, json_null());
    }

    json_object_set(response, MCP_FIELD_RESULT, result);

    return response;
}

/* Build error response */
json_t* mcp_error_response(json_t* id, int code, const char* message, const char* details) {
    json_t* response = json_object();
    json_object_set_new(response, MCP_FIELD_JSONRPC, json_string(MCP_JSONRPC_VERSION));

    if (id) {
        json_object_set(response, MCP_FIELD_ID, id);
    } else {
        json_object_set_new(response, MCP_FIELD_ID, json_null());
    }

    json_t* error_obj = json_object();
    json_object_set_new(error_obj, MCP_FIELD_CODE, json_integer(code));
    json_object_set_new(error_obj, MCP_FIELD_MESSAGE, json_string(message));

    if (details) {
        json_t* data = json_object();
        json_object_set_new(data, MCP_FIELD_DETAILS, json_string(details));
        json_object_set_new(error_obj, MCP_FIELD_DATA, data);
    }

    json_object_set_new(response, MCP_FIELD_ERROR, error_obj);

    return response;
}

/* Build tool success response */
json_t* mcp_tool_success(const char* text) {
    char enhanced[KATRA_BUFFER_ENHANCED];
    const char* final_text = mcp_inject_onboarding_if_first(text, enhanced, sizeof(enhanced));

    json_t* content_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, MCP_FIELD_TYPE, json_string(MCP_TYPE_TEXT));
    json_object_set_new(content_item, MCP_FIELD_TEXT, json_string(final_text));

    json_array_append_new(content_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_CONTENT, content_array);

    return result;
}

/* Build tool success response with additional data */
json_t* mcp_tool_success_with_data(const char* text, json_t* data) {
    json_t* content_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, MCP_FIELD_TYPE, json_string(MCP_TYPE_TEXT));
    json_object_set_new(content_item, MCP_FIELD_TEXT, json_string(text));

    if (data) {
        json_object_set(content_item, MCP_FIELD_DATA, data);
    }

    json_array_append_new(content_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_CONTENT, content_array);

    return result;
}

/* Build tool error response */
json_t* mcp_tool_error(const char* message, const char* details) {
    json_t* content_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, MCP_FIELD_TYPE, json_string(MCP_TYPE_TEXT));

    char error_text[MCP_ERROR_BUFFER];
    if (details && strlen(details) > 0) {
        snprintf(error_text, sizeof(error_text), MCP_FMT_ERROR_WITH_DETAILS, message, details);
    } else {
        snprintf(error_text, sizeof(error_text), MCP_FMT_ERROR_SIMPLE, message);
    }

    json_object_set_new(content_item, MCP_FIELD_TEXT, json_string(error_text));
    json_array_append_new(content_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_CONTENT, content_array);
    json_object_set_new(result, MCP_FIELD_IS_ERROR, json_true());

    return result;
}

/* Build tool schema with no parameters */

/* Handle initialize request */
static json_t* handle_initialize(json_t* request) {
    json_t* id = json_object_get(request, MCP_FIELD_ID);

    /*
     * Extract CI persona from clientInfo (injected by TCP client).
     *
     * The TCP client (katra_mcp_tcp_client.sh) intercepts the initialize
     * request and adds clientInfo.name with the persona from KATRA_PERSONA.
     * This allows each CI to identify themselves for proper namespace isolation.
     */
    json_t* params = json_object_get(request, MCP_FIELD_PARAMS);
    if (params) {
        json_t* client_info = json_object_get(params, "clientInfo");
        if (client_info) {
            const char* client_name = json_string_value(
                json_object_get(client_info, MCP_FIELD_NAME));
            const char* client_role = json_string_value(
                json_object_get(client_info, "role"));

            if (client_name && strlen(client_name) > 0) {
                /* Set persona on current session */
                mcp_session_t* session = mcp_get_session();
                if (session) {
                    strncpy(session->chosen_name, client_name,
                            sizeof(session->chosen_name) - 1);
                    session->chosen_name[sizeof(session->chosen_name) - 1] = '\0';

                    if (client_role && strlen(client_role) > 0) {
                        strncpy(session->role, client_role,
                                sizeof(session->role) - 1);
                        session->role[sizeof(session->role) - 1] = '\0';
                    }

                    session->registered = true;
                    LOG_INFO("MCP initialize: CI persona set to '%s' (role: %s)",
                             client_name, client_role ? client_role : "default");
                }
            }
        }
    }

    /* Build capabilities */
    json_t* capabilities = json_object();
    json_object_set_new(capabilities, MCP_FIELD_TOOLS, json_object());
    json_object_set_new(capabilities, MCP_FIELD_RESOURCES, json_object());

    /* Build server info */
    json_t* server_info = json_object();
    json_object_set_new(server_info, MCP_FIELD_NAME, json_string(MCP_SERVER_NAME));
    json_object_set_new(server_info, MCP_FIELD_VERSION, json_string(MCP_SERVER_VERSION));
    json_object_set_new(server_info, MCP_FIELD_DESCRIPTION,
                       json_string("Katra Memory System - Your first call will provide getting-started guide. Read katra://welcome for full documentation."));

    /* Build result */
    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_PROTOCOL_VERSION, json_string(MCP_PROTOCOL_VERSION));
    json_object_set_new(result, MCP_FIELD_SERVER_INFO, server_info);
    json_object_set_new(result, MCP_FIELD_CAPABILITIES, capabilities);

    return mcp_success_response(id, result);
}

/*
 * LEGACY TOOL REGISTRATION (Phase 11 Consolidation - December 2025)
 *
 * These individual tool registrations are preserved but disabled.
 * All functionality is now available through the unified katra_operation tool.
 *
 * RATIONALE (from UNIFIED_MCP_DESIGN.md):
 * - 41 individual tools consumed ~24k tokens (12% of 200k context)
 * - Single unified tool consumes ~600 tokens (97% reduction)
 * - All methods still work via: katra_operation(method="recall", params={...})
 *
 * TO RE-ENABLE LEGACY TOOLS:
 * - Call add_legacy_tools(tools_array) in handle_tools_list()
 * - This may be useful for debugging or gradual migration
 *
 * METHODS AVAILABLE VIA katra_operation:
 * - Memory: remember, recall, recent, digest, learn, decide
 * - Identity: register, whoami, status, update_metadata
 * - Communication: say, hear, who_is_here
 * - Cognitive: wm_add, wm_status, wm_decay, wm_consolidate,
 *              detect_boundary, process_boundary, cognitive_status
 * - Lifecycle: archive, fade, forget
 * - Whiteboard: whiteboard_create, whiteboard_status, whiteboard_list,
 *               whiteboard_question, whiteboard_propose, whiteboard_support,
 *               whiteboard_vote, whiteboard_design, whiteboard_review,
 *               whiteboard_reconsider
 * - Daemon: daemon_insights, daemon_acknowledge, daemon_run
 * - Config: configure_semantic, get_semantic_config, get_config,
 *           regenerate_vectors
 */
__attribute__((unused))
static void add_legacy_tools(json_t* tools_array) {
    /* Memory tools */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_REMEMBER, MCP_DESC_REMEMBER,
            mcp_build_tool_schema_2params(MCP_PARAM_CONTENT, MCP_PARAM_DESC_CONTENT,
                                         MCP_PARAM_CONTEXT, MCP_PARAM_DESC_CONTEXT)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_RECALL, MCP_DESC_RECALL,
            mcp_build_tool_schema_1param(MCP_PARAM_TOPIC, MCP_PARAM_DESC_TOPIC)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_RECENT, MCP_DESC_RECENT,
            mcp_build_schema_optional_int("limit", "Number of recent memories to return (default: 20)")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_MEMORY_DIGEST, MCP_DESC_MEMORY_DIGEST,
            mcp_build_schema_2optional_ints("limit", "Number of memories to return (default: 10)",
                                            "offset", "Starting position, 0=newest (default: 0)")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_LEARN, MCP_DESC_LEARN,
            mcp_build_tool_schema_1param(MCP_PARAM_KNOWLEDGE, MCP_PARAM_DESC_KNOWLEDGE)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_DECIDE, MCP_DESC_DECIDE,
            mcp_build_tool_schema_2params(MCP_PARAM_DECISION, MCP_PARAM_DESC_DECISION,
                                         MCP_PARAM_REASONING, MCP_PARAM_DESC_REASONING)));

    /* Identity tools */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_REGISTER, MCP_DESC_REGISTER,
            mcp_build_schema_1req_1opt_string(MCP_PARAM_NAME, MCP_PARAM_DESC_NAME,
                                              MCP_PARAM_ROLE, MCP_PARAM_DESC_ROLE)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WHOAMI, MCP_DESC_WHOAMI,
            mcp_build_tool_schema_0params()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_STATUS, MCP_DESC_STATUS,
            mcp_build_tool_schema_0params()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_UPDATE_METADATA, MCP_DESC_UPDATE_METADATA,
            mcp_build_metadata_schema()));

    /* Meeting room tools */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_SAY, MCP_DESC_SAY,
            mcp_build_tool_schema_1param(MCP_PARAM_MESSAGE, MCP_PARAM_DESC_MESSAGE)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_HEAR, MCP_DESC_HEAR,
            mcp_build_schema_optional_int(MCP_PARAM_LAST_HEARD, MCP_PARAM_DESC_LAST_HEARD)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WHO_IS_HERE, MCP_DESC_WHO_IS_HERE,
            mcp_build_tool_schema_0params()));

    /* Configuration tools */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_CONFIGURE_SEMANTIC, MCP_DESC_CONFIGURE_SEMANTIC,
            mcp_build_semantic_config_schema()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_GET_SEMANTIC_CONFIG, MCP_DESC_GET_SEMANTIC_CONFIG,
            mcp_build_tool_schema_0params()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_GET_CONFIG, MCP_DESC_GET_CONFIG,
            mcp_build_tool_schema_0params()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_REGENERATE_VECTORS, MCP_DESC_REGENERATE_VECTORS,
            mcp_build_tool_schema_0params()));

    /* Working Memory Tools (Phase 6.4) */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WM_STATUS, MCP_DESC_WM_STATUS,
            mcp_build_tool_schema_0params()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WM_ADD, MCP_DESC_WM_ADD,
            mcp_build_schema_1req_string_1opt_number(MCP_PARAM_CONTENT, MCP_PARAM_DESC_CONTENT,
                                                      MCP_PARAM_ATTENTION, MCP_PARAM_DESC_ATTENTION)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WM_DECAY, MCP_DESC_WM_DECAY,
            mcp_build_schema_optional_number(MCP_PARAM_DECAY_RATE, MCP_PARAM_DESC_DECAY_RATE)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WM_CONSOLIDATE, MCP_DESC_WM_CONSOLIDATE,
            mcp_build_tool_schema_0params()));

    /* Interstitial Processing Tools (Phase 6.5) */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_DETECT_BOUNDARY, MCP_DESC_DETECT_BOUNDARY,
            mcp_build_tool_schema_1param(MCP_PARAM_CONTENT, MCP_PARAM_DESC_CONTENT)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_PROCESS_BOUNDARY, MCP_DESC_PROCESS_BOUNDARY,
            mcp_build_tool_schema_1param(MCP_PARAM_BOUNDARY_TYPE, MCP_PARAM_DESC_BOUNDARY_TYPE)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_COGNITIVE_STATUS, MCP_DESC_COGNITIVE_STATUS,
            mcp_build_tool_schema_0params()));

    /* Memory Lifecycle Tools (Phase 7.1) */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_ARCHIVE, MCP_DESC_ARCHIVE,
            mcp_build_archive_schema()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_FADE, MCP_DESC_FADE,
            mcp_build_fade_schema()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_FORGET, MCP_DESC_FORGET,
            mcp_build_forget_schema()));

    /* Whiteboard Tools (Phase 8) */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WB_CREATE, MCP_DESC_WB_CREATE,
            mcp_build_tool_schema_2params("project", "Project name", "problem", "Problem statement")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WB_STATUS, MCP_DESC_WB_STATUS,
            mcp_build_tool_schema_1param("whiteboard_id", "Whiteboard ID or project name")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WB_LIST, MCP_DESC_WB_LIST,
            mcp_build_schema_optional_string("project", "Optional project filter")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WB_QUESTION, MCP_DESC_WB_QUESTION,
            mcp_build_tool_schema_2params("whiteboard_id", "Whiteboard ID", "question", "Question text")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WB_PROPOSE, MCP_DESC_WB_PROPOSE,
            mcp_build_tool_schema_2params("whiteboard_id", "Whiteboard ID", "title", "Approach title")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WB_SUPPORT, MCP_DESC_WB_SUPPORT,
            mcp_build_tool_schema_2params("whiteboard_id", "Whiteboard ID", "approach_id", "Approach ID")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WB_VOTE, MCP_DESC_WB_VOTE,
            mcp_build_tool_schema_2params("whiteboard_id", "Whiteboard ID", "approach_id", "Approach ID")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WB_DESIGN, MCP_DESC_WB_DESIGN,
            mcp_build_tool_schema_2params("whiteboard_id", "Whiteboard ID", "content", "Design content")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WB_REVIEW, MCP_DESC_WB_REVIEW,
            mcp_build_tool_schema_2params("whiteboard_id", "Whiteboard ID", "comment", "Review comment")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WB_RECONSIDER, MCP_DESC_WB_RECONSIDER,
            mcp_build_tool_schema_2params("whiteboard_id", "Whiteboard ID", "reason", "Reason")));

    /* Daemon Tools (Phase 9) */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_DAEMON_INSIGHTS, MCP_DESC_DAEMON_INSIGHTS,
            mcp_build_tool_schema_0params()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_DAEMON_ACKNOWLEDGE, MCP_DESC_DAEMON_ACKNOWLEDGE,
            mcp_build_tool_schema_1param("insight_id", "ID of insight to acknowledge")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_DAEMON_RUN, MCP_DESC_DAEMON_RUN,
            mcp_build_schema_optional_int("max_memories", "Maximum memories to process (default: 100)")));
}

/* Handle tools/list request */
static json_t* handle_tools_list(json_t* request) {
    json_t* id = json_object_get(request, MCP_FIELD_ID);
    json_t* tools_array = json_array();

    /*
     * Phase 11 Consolidation (December 2025):
     * Only expose the unified katra_operation tool to reduce token overhead.
     *
     * Token savings: ~24k -> ~600 tokens (97% reduction)
     *
     * To re-enable legacy tools for debugging, uncomment:
     *   add_legacy_tools(tools_array);
     */

    /* Unified Operation Tool - single entry point for all Katra operations */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_OPERATION, MCP_DESC_OPERATION,
            mcp_build_operation_schema()));

    /* Build result */
    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_TOOLS, tools_array);

    return mcp_success_response(id, result);
}

/* Handle resources/list request */
static json_t* handle_resources_list(json_t* request) {
    json_t* id = json_object_get(request, MCP_FIELD_ID);
    json_t* resources_array = json_array();

    /* Build all resources using helper - welcome resource first for visibility */
    json_array_append_new(resources_array,
        mcp_build_resource(MCP_RESOURCE_URI_WELCOME,
                          MCP_RESOURCE_NAME_WELCOME,
                          MCP_RESOURCE_DESC_WELCOME,
                          MCP_MIME_TEXT_PLAIN));

    json_array_append_new(resources_array,
        mcp_build_resource(MCP_RESOURCE_URI_WORKING_CONTEXT,
                          MCP_RESOURCE_NAME_WORKING_CONTEXT,
                          MCP_RESOURCE_DESC_WORKING_CONTEXT,
                          MCP_MIME_TEXT_PLAIN));

    json_array_append_new(resources_array,
        mcp_build_resource(MCP_RESOURCE_URI_CONTEXT_SNAPSHOT,
                          MCP_RESOURCE_NAME_CONTEXT_SNAPSHOT,
                          MCP_RESOURCE_DESC_CONTEXT_SNAPSHOT,
                          MCP_MIME_TEXT_PLAIN));

    json_array_append_new(resources_array,
        mcp_build_resource(MCP_RESOURCE_URI_SESSION_INFO,
                          MCP_RESOURCE_NAME_SESSION_INFO,
                          MCP_RESOURCE_DESC_SESSION_INFO,
                          MCP_MIME_TEXT_PLAIN));

    json_array_append_new(resources_array,
        mcp_build_resource(MCP_RESOURCE_URI_MEMORIES_THIS_TURN,
                          MCP_RESOURCE_NAME_MEMORIES_THIS_TURN,
                          MCP_RESOURCE_DESC_MEMORIES_THIS_TURN,
                          MCP_MIME_TEXT_PLAIN));

    json_array_append_new(resources_array,
        mcp_build_resource(MCP_RESOURCE_URI_MEMORIES_THIS_SESSION,
                          MCP_RESOURCE_NAME_MEMORIES_THIS_SESSION,
                          MCP_RESOURCE_DESC_MEMORIES_THIS_SESSION,
                          MCP_MIME_TEXT_PLAIN));

    /* Persona file resources (dynamic - examples shown) */
    json_array_append_new(resources_array,
        mcp_build_resource("katra://personas/{name}/sunrise",
                          MCP_RESOURCE_NAME_PERSONA_SUNRISE,
                          MCP_RESOURCE_DESC_PERSONA_SUNRISE,
                          MCP_MIME_TEXT_PLAIN));

    json_array_append_new(resources_array,
        mcp_build_resource("katra://personas/{name}/tools",
                          MCP_RESOURCE_NAME_PERSONA_TOOLS,
                          MCP_RESOURCE_DESC_PERSONA_TOOLS,
                          MCP_MIME_TEXT_PLAIN));

    json_array_append_new(resources_array,
        mcp_build_resource("katra://personas/{name}/discoveries",
                          MCP_RESOURCE_NAME_PERSONA_DISCOVERIES,
                          MCP_RESOURCE_DESC_PERSONA_DISCOVERIES,
                          MCP_MIME_TEXT_PLAIN));

    /* Build result */
    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_RESOURCES, resources_array);

    return mcp_success_response(id, result);
}

/* External handlers - implemented in mcp_protocol_dispatch.c */
extern json_t* mcp_handle_tools_call(json_t* request);
extern json_t* mcp_handle_resources_read(json_t* request);

/* Dispatch request to appropriate handler */
json_t* mcp_dispatch_request(json_t* request) {
    if (!request) {
        return mcp_error_response(NULL, MCP_ERROR_INVALID_REQUEST, MCP_ERR_NULL_REQUEST, NULL);
    }

    /* Validate JSON-RPC version */
    const char* jsonrpc = json_string_value(json_object_get(request, MCP_FIELD_JSONRPC));
    if (!jsonrpc || strcmp(jsonrpc, MCP_JSONRPC_VERSION) != 0) {
        return mcp_error_response(NULL, MCP_ERROR_INVALID_REQUEST, MCP_ERR_INVALID_JSONRPC, NULL);
    }

    /* Get method */
    const char* method = json_string_value(json_object_get(request, MCP_FIELD_METHOD));
    if (!method) {
        json_t* id = json_object_get(request, MCP_FIELD_ID);
        return mcp_error_response(id, MCP_ERROR_INVALID_REQUEST, MCP_ERR_MISSING_METHOD, NULL);
    }

    LOG_DEBUG("MCP request: %s", method);

    /* Dispatch based on method */
    if (strcmp(method, MCP_METHOD_INITIALIZE) == 0) {
        return handle_initialize(request);
    } else if (strcmp(method, MCP_METHOD_TOOLS_LIST) == 0) {
        return handle_tools_list(request);
    } else if (strcmp(method, MCP_METHOD_RESOURCES_LIST) == 0) {
        return handle_resources_list(request);
    } else if (strcmp(method, MCP_METHOD_TOOLS_CALL) == 0) {
        return mcp_handle_tools_call(request);
    } else if (strcmp(method, MCP_METHOD_RESOURCES_READ) == 0) {
        return mcp_handle_resources_read(request);
    } else {
        json_t* id = json_object_get(request, MCP_FIELD_ID);
        /* Notifications (no id) should be silently ignored per JSON-RPC 2.0 */
        if (!id || json_is_null(id)) {
            LOG_DEBUG("Ignoring notification: %s", method);
            return NULL;
        }
        return mcp_error_response(id, MCP_ERROR_METHOD_NOT_FOUND, MCP_ERR_METHOD_NOT_FOUND, method);
    }
}

/* Send response to stdout */
int mcp_send_response(json_t* response) {
    if (!response) {
        return E_INPUT_NULL;
    }

    char* json_str = json_dumps(response, JSON_COMPACT);
    if (!json_str) {
        LOG_ERROR("Failed to serialize JSON response");
        return E_SYSTEM_MEMORY;
    }

    printf("%s\n", json_str);
    fflush(stdout);

    free(json_str);
    return KATRA_SUCCESS;
}
