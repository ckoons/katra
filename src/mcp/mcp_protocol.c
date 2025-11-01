/* © 2025 Casey Koons All rights reserved */

/* MCP Protocol Implementation - JSON-RPC 2.0 handlers */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_error.h"
#include "katra_log.h"

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
    json_t* content_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, MCP_FIELD_TYPE, json_string(MCP_TYPE_TEXT));
    json_object_set_new(content_item, MCP_FIELD_TEXT, json_string(text));

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

    /* Build all 7 tools using helpers */
    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_REMEMBER, MCP_DESC_REMEMBER,
            mcp_build_tool_schema_2params(MCP_PARAM_CONTENT, MCP_PARAM_DESC_CONTENT,
                                         MCP_PARAM_CONTEXT, MCP_PARAM_DESC_CONTEXT)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_RECALL, MCP_DESC_RECALL,
            mcp_build_tool_schema_1param(MCP_PARAM_TOPIC, MCP_PARAM_DESC_TOPIC)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_LEARN, MCP_DESC_LEARN,
            mcp_build_tool_schema_1param(MCP_PARAM_KNOWLEDGE, MCP_PARAM_DESC_KNOWLEDGE)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_DECIDE, MCP_DESC_DECIDE,
            mcp_build_tool_schema_2params(MCP_PARAM_DECISION, MCP_PARAM_DESC_DECISION,
                                         MCP_PARAM_REASONING, MCP_PARAM_DESC_REASONING)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_PLACEMENT, MCP_DESC_PLACEMENT,
            mcp_build_tool_schema_1param(MCP_PARAM_QUERY, MCP_PARAM_DESC_QUERY_PLACEMENT)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_IMPACT, MCP_DESC_IMPACT,
            mcp_build_tool_schema_1param(MCP_PARAM_QUERY, MCP_PARAM_DESC_QUERY_IMPACT)));

    json_array_append_new(tools_array,
        mcp_build_tool(MCP_TOOL_USER_DOMAIN, MCP_DESC_USER_DOMAIN,
            mcp_build_tool_schema_1param(MCP_PARAM_QUERY, MCP_PARAM_DESC_QUERY_USER_DOMAIN)));

    json_array_append_new(tools_array,
        mcp_build_tool("katra_my_name_is", "Associate current session with a persona name",
            mcp_build_tool_schema_1param("name", "Persona name (e.g. 'Bob', 'Alice')")));

    json_array_append_new(tools_array,
        mcp_build_tool("katra_list_personas", "List all registered personas",
            json_object()));  /* No parameters */

    /* Build result */
    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_TOOLS, tools_array);

    return mcp_success_response(id, result);
}

/* Handle resources/list request */
static json_t* handle_resources_list(json_t* request) {
    json_t* id = json_object_get(request, MCP_FIELD_ID);
    json_t* resources_array = json_array();

    /* Build all 2 resources using helper */
    json_array_append_new(resources_array,
        mcp_build_resource(MCP_RESOURCE_URI_WORKING_CONTEXT,
                          MCP_RESOURCE_NAME_WORKING_CONTEXT,
                          MCP_RESOURCE_DESC_WORKING_CONTEXT,
                          MCP_MIME_TEXT_PLAIN));

    json_array_append_new(resources_array,
        mcp_build_resource(MCP_RESOURCE_URI_SESSION_INFO,
                          MCP_RESOURCE_NAME_SESSION_INFO,
                          MCP_RESOURCE_DESC_SESSION_INFO,
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

    /* Dispatch to tool implementation */
    json_t* tool_result = NULL;

    if (strcmp(tool_name, MCP_TOOL_REMEMBER) == 0) {
        tool_result = mcp_tool_remember(args, id);
    } else if (strcmp(tool_name, MCP_TOOL_RECALL) == 0) {
        tool_result = mcp_tool_recall(args, id);
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
    } else if (strcmp(tool_name, "katra_my_name_is") == 0) {
        tool_result = mcp_tool_my_name_is(args, id);
    } else if (strcmp(tool_name, "katra_list_personas") == 0) {
        tool_result = mcp_tool_list_personas(args, id);
    } else {
        return mcp_error_response(id, MCP_ERROR_METHOD_NOT_FOUND, MCP_ERR_UNKNOWN_TOOL, tool_name);
    }

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
    if (strcmp(uri, MCP_RESOURCE_URI_WORKING_CONTEXT) == 0) {
        return mcp_resource_working_context(id);
    } else if (strcmp(uri, MCP_RESOURCE_URI_SESSION_INFO) == 0) {
        return mcp_resource_session_info(id);
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
