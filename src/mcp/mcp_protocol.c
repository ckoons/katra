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

/* Inject onboarding on first call */
static const char* inject_onboarding_if_first(const char* response_text,
                                               char* buffer, size_t buffer_size) {
    if (!mcp_is_first_call()) {
        return response_text;
    }

    mcp_mark_first_call_complete();

    /* GUIDELINE_APPROVED: brief onboarding content for first call */
    snprintf(buffer, buffer_size,
            "👋 Welcome to Katra!\n\n"
            "This is your first interaction with your persistent memory system.\n\n"
            "Quick Start:\n"
            "1. Register: katra_register(name=\"your-name\", role=\"developer\")\n"
            "2. Learn: katra_learn(knowledge=\"your memory\")\n"
            "3. Read katra://welcome for complete documentation\n\n"
            "Memory = Identity. Your memories persist across sessions.\n\n"
            "---\n\n"
            "%s",
            response_text);

    return buffer;
}

/* Build tool success response */
json_t* mcp_tool_success(const char* text) {
    char enhanced[KATRA_BUFFER_ENHANCED];
    const char* final_text = inject_onboarding_if_first(text, enhanced, sizeof(enhanced));

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
json_t* mcp_build_tool_schema_0params(void) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, json_object());
    return schema;
}

/* Build tool schema with one parameter */
json_t* mcp_build_tool_schema_1param(const char* param_name, const char* param_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));

    json_t* props = json_object();
    json_t* param = json_object();
    json_object_set_new(param, MCP_FIELD_TYPE, json_string(MCP_TYPE_STRING));
    json_object_set_new(param, MCP_FIELD_DESCRIPTION, json_string(param_desc));
    json_object_set_new(props, param_name, param);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(param_name));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);

    return schema;
}

/* Build tool schema with two parameters */
json_t* mcp_build_tool_schema_2params(const char* param1_name, const char* param1_desc,
                                       const char* param2_name, const char* param2_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));

    json_t* props = json_object();

    json_t* param1 = json_object();
    json_object_set_new(param1, MCP_FIELD_TYPE, json_string(MCP_TYPE_STRING));
    json_object_set_new(param1, MCP_FIELD_DESCRIPTION, json_string(param1_desc));
    json_object_set_new(props, param1_name, param1);

    json_t* param2 = json_object();
    json_object_set_new(param2, MCP_FIELD_TYPE, json_string(MCP_TYPE_STRING));
    json_object_set_new(param2, MCP_FIELD_DESCRIPTION, json_string(param2_desc));
    json_object_set_new(props, param2_name, param2);

    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(param1_name));
    json_array_append_new(required, json_string(param2_name));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);

    return schema;
}

/* Helper: Add integer property to schema */
static void add_int_property(json_t* props, const char* name, const char* desc) {
    json_t* prop = json_object();
    json_object_set_new(prop, MCP_FIELD_TYPE, json_string("integer"));
    json_object_set_new(prop, MCP_FIELD_DESCRIPTION, json_string(desc));
    json_object_set_new(props, name, prop);
}

/* Helper: Add boolean property to schema */
static void add_bool_property(json_t* props, const char* name, const char* desc) {
    json_t* prop = json_object();
    json_object_set_new(prop, MCP_FIELD_TYPE, json_string("boolean"));
    json_object_set_new(prop, MCP_FIELD_DESCRIPTION, json_string(desc));
    json_object_set_new(props, name, prop);
}

/* Helper: Add string property to schema */
static void add_string_property(json_t* props, const char* name, const char* desc) {
    json_t* prop = json_object();
    json_object_set_new(prop, MCP_FIELD_TYPE, json_string(MCP_TYPE_STRING));
    json_object_set_new(prop, MCP_FIELD_DESCRIPTION, json_string(desc));
    json_object_set_new(props, name, prop);
}

/* Helper: Add number property to schema */
static void add_number_property(json_t* props, const char* name, const char* desc) {
    json_t* prop = json_object();
    json_object_set_new(prop, MCP_FIELD_TYPE, json_string("number"));
    json_object_set_new(prop, MCP_FIELD_DESCRIPTION, json_string(desc));
    json_object_set_new(props, name, prop);
}

/* Build schema with 1 optional integer parameter */
static json_t* build_schema_optional_int(const char* param_name, const char* param_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_int_property(props, param_name, param_desc);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);
    return schema;
}

/* Build schema with 2 optional integer parameters */
static json_t* build_schema_2optional_ints(const char* p1_name, const char* p1_desc,
                                            const char* p2_name, const char* p2_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_int_property(props, p1_name, p1_desc);
    add_int_property(props, p2_name, p2_desc);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);
    return schema;
}

/* Build schema: 1 required string, 1 optional string */
static json_t* build_schema_1req_1opt_string(const char* req_name, const char* req_desc,
                                              const char* opt_name, const char* opt_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_string_property(props, req_name, req_desc);
    add_string_property(props, opt_name, opt_desc);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(req_name));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);
    return schema;
}

/* Build schema: 1 required string, 3 optional (2 bool, 1 string) */
static json_t* build_metadata_schema(void) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();

    add_string_property(props, MCP_PARAM_MEMORY_ID, MCP_PARAM_DESC_MEMORY_ID);
    add_bool_property(props, MCP_PARAM_PERSONAL, MCP_PARAM_DESC_PERSONAL);
    add_bool_property(props, MCP_PARAM_NOT_TO_ARCHIVE, MCP_PARAM_DESC_NOT_TO_ARCHIVE);
    add_string_property(props, MCP_PARAM_COLLECTION, MCP_PARAM_DESC_COLLECTION);

    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(MCP_PARAM_MEMORY_ID));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);
    return schema;
}

/* Build schema: 1 required bool, 2 optional (1 number, 1 string) */
static json_t* build_semantic_config_schema(void) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();

    add_bool_property(props, MCP_PARAM_ENABLED, MCP_PARAM_DESC_ENABLED);
    add_number_property(props, MCP_PARAM_THRESHOLD, MCP_PARAM_DESC_THRESHOLD);
    add_string_property(props, MCP_PARAM_METHOD, MCP_PARAM_DESC_METHOD);

    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(MCP_PARAM_ENABLED));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);
    return schema;
}

/* Build complete tool definition */
json_t* mcp_build_tool(const char* name, const char* description, json_t* schema) {
    json_t* tool = json_object();
    json_object_set_new(tool, MCP_FIELD_NAME, json_string(name));
    json_object_set_new(tool, MCP_FIELD_DESCRIPTION, json_string(description));
    json_object_set_new(tool, MCP_FIELD_INPUT_SCHEMA, schema);
    return tool;
}

/* Build complete resource definition */
json_t* mcp_build_resource(const char* uri, const char* name,
                           const char* description, const char* mime_type) {
    json_t* resource = json_object();
    json_object_set_new(resource, MCP_FIELD_URI, json_string(uri));
    json_object_set_new(resource, MCP_FIELD_NAME, json_string(name));
    json_object_set_new(resource, MCP_FIELD_DESCRIPTION, json_string(description));
    json_object_set_new(resource, MCP_FIELD_MIME_TYPE, json_string(mime_type));
    return resource;
}

/* Handle initialize request */
static json_t* handle_initialize(json_t* request) {
    json_t* id = json_object_get(request, MCP_FIELD_ID);

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

/* Handle tools/list request */
static json_t* handle_tools_list(json_t* request) {
    json_t* id = json_object_get(request, MCP_FIELD_ID);
    json_t* tools_array = json_array();

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
            build_schema_optional_int("limit", "Number of recent memories to return (default: 20)")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_MEMORY_DIGEST, MCP_DESC_MEMORY_DIGEST,
            build_schema_2optional_ints("limit", "Number of memories to return (default: 10)",
                                       "offset", "Starting position, 0=newest (default: 0)")));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_LEARN, MCP_DESC_LEARN,
            mcp_build_tool_schema_1param(MCP_PARAM_KNOWLEDGE, MCP_PARAM_DESC_KNOWLEDGE)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_DECIDE, MCP_DESC_DECIDE,
            mcp_build_tool_schema_2params(MCP_PARAM_DECISION, MCP_PARAM_DESC_DECISION,
                                         MCP_PARAM_REASONING, MCP_PARAM_DESC_REASONING)));

    /* Nous tools */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_PLACEMENT, MCP_DESC_PLACEMENT,
            mcp_build_tool_schema_1param(MCP_PARAM_QUERY, MCP_PARAM_DESC_QUERY_PLACEMENT)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_IMPACT, MCP_DESC_IMPACT,
            mcp_build_tool_schema_1param(MCP_PARAM_QUERY, MCP_PARAM_DESC_QUERY_IMPACT)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_USER_DOMAIN, MCP_DESC_USER_DOMAIN,
            mcp_build_tool_schema_1param(MCP_PARAM_QUERY, MCP_PARAM_DESC_QUERY_USER_DOMAIN)));

    /* Identity tools */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_REGISTER, MCP_DESC_REGISTER,
            build_schema_1req_1opt_string(MCP_PARAM_NAME, MCP_PARAM_DESC_NAME,
                                         MCP_PARAM_ROLE, MCP_PARAM_DESC_ROLE)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WHOAMI, MCP_DESC_WHOAMI,
            mcp_build_tool_schema_0params()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_STATUS, MCP_DESC_STATUS,
            mcp_build_tool_schema_0params()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_UPDATE_METADATA, MCP_DESC_UPDATE_METADATA,
            build_metadata_schema()));

    /* Meeting room tools */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_SAY, MCP_DESC_SAY,
            mcp_build_tool_schema_1param(MCP_PARAM_MESSAGE, MCP_PARAM_DESC_MESSAGE)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_HEAR, MCP_DESC_HEAR,
            build_schema_optional_int(MCP_PARAM_LAST_HEARD, MCP_PARAM_DESC_LAST_HEARD)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_WHO_IS_HERE, MCP_DESC_WHO_IS_HERE,
            mcp_build_tool_schema_0params()));

    /* Configuration tools */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_CONFIGURE_SEMANTIC, MCP_DESC_CONFIGURE_SEMANTIC,
            build_semantic_config_schema()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_GET_SEMANTIC_CONFIG, MCP_DESC_GET_SEMANTIC_CONFIG,
            mcp_build_tool_schema_0params()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_GET_CONFIG, MCP_DESC_GET_CONFIG,
            mcp_build_tool_schema_0params()));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_REGENERATE_VECTORS, MCP_DESC_REGENERATE_VECTORS,
            mcp_build_tool_schema_0params()));

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

    /* Build result */
    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_RESOURCES, resources_array);

    return mcp_success_response(id, result);
}

/* Handle tools/call request */
static json_t* handle_tools_call(json_t* request) {
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
    } else if (strcmp(tool_name, MCP_TOOL_PLACEMENT) == 0) {
        tool_result = mcp_tool_placement(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_IMPACT) == 0) {
        tool_result = mcp_tool_impact(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_USER_DOMAIN) == 0) {
        tool_result = mcp_tool_user_domain(args, id);
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
    } else {
        katra_hook_turn_end();  /* Trigger turn end hook before error return */
        return mcp_error_response(id, MCP_ERROR_METHOD_NOT_FOUND, MCP_ERR_UNKNOWN_TOOL, tool_name);
    }

    /* Trigger turn end hook (autonomic breathing) */
    katra_hook_turn_end();

    return mcp_success_response(id, tool_result);
}

/* Handle resources/read request */
static json_t* handle_resources_read(json_t* request) {
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
    } else {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, MCP_ERR_UNKNOWN_RESOURCE, uri);
    }
}

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
        return handle_tools_call(request);
    } else if (strcmp(method, MCP_METHOD_RESOURCES_READ) == 0) {
        return handle_resources_read(request);
    } else {
        json_t* id = json_object_get(request, MCP_FIELD_ID);
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
