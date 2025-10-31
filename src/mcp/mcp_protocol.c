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
    json_object_set_new(response, "jsonrpc", json_string("2.0"));

    if (id) {
        json_object_set(response, "id", id);
    } else {
        json_object_set_new(response, "id", json_null());
    }

    json_object_set(response, "result", result);

    return response;
}

/* Build error response */
json_t* mcp_error_response(json_t* id, int code, const char* message, const char* details) {
    json_t* response = json_object();
    json_object_set_new(response, "jsonrpc", json_string("2.0"));

    if (id) {
        json_object_set(response, "id", id);
    } else {
        json_object_set_new(response, "id", json_null());
    }

    json_t* error_obj = json_object();
    json_object_set_new(error_obj, "code", json_integer(code));
    json_object_set_new(error_obj, "message", json_string(message));

    if (details) {
        json_t* data = json_object();
        json_object_set_new(data, "details", json_string(details));
        json_object_set_new(error_obj, "data", data);
    }

    json_object_set_new(response, "error", error_obj);

    return response;
}

/* Build tool success response */
json_t* mcp_tool_success(const char* text) {
    json_t* content_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, "type", json_string("text"));
    json_object_set_new(content_item, "text", json_string(text));

    json_array_append_new(content_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, "content", content_array);

    return result;
}

/* Build tool success response with additional data */
json_t* mcp_tool_success_with_data(const char* text, json_t* data) {
    json_t* content_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, "type", json_string("text"));
    json_object_set_new(content_item, "text", json_string(text));

    if (data) {
        json_object_set(content_item, "data", data);
    }

    json_array_append_new(content_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, "content", content_array);

    return result;
}

/* Build tool error response */
json_t* mcp_tool_error(const char* message, const char* details) {
    json_t* content_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, "type", json_string("text"));

    char error_text[MCP_ERROR_BUFFER];
    if (details && strlen(details) > 0) {
        snprintf(error_text, sizeof(error_text), "Error: %s\nDetails: %s", message, details);
    } else {
        snprintf(error_text, sizeof(error_text), "Error: %s", message);
    }

    json_object_set_new(content_item, "text", json_string(error_text));
    json_array_append_new(content_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, "content", content_array);
    json_object_set_new(result, "isError", json_true());

    return result;
}

/* Handle initialize request */
static json_t* handle_initialize(json_t* request) {
    json_t* id = json_object_get(request, "id");

    /* Build capabilities */
    json_t* capabilities = json_object();
    json_object_set_new(capabilities, "tools", json_object());
    json_object_set_new(capabilities, "resources", json_object());

    /* Build server info */
    json_t* server_info = json_object();
    json_object_set_new(server_info, "name", json_string(MCP_SERVER_NAME));
    json_object_set_new(server_info, "version", json_string(MCP_SERVER_VERSION));

    /* Build result */
    json_t* result = json_object();
    json_object_set_new(result, "protocolVersion", json_string(MCP_PROTOCOL_VERSION));
    json_object_set_new(result, "serverInfo", server_info);
    json_object_set_new(result, "capabilities", capabilities);

    return mcp_success_response(id, result);
}

/* Handle tools/list request */
static json_t* handle_tools_list(json_t* request) {
    json_t* id = json_object_get(request, "id");

    json_t* tools_array = json_array();

    /* Tool 1: katra_remember */
    json_t* tool1 = json_object();
    json_object_set_new(tool1, "name", json_string("katra_remember"));
    json_object_set_new(tool1, "description",
                       json_string("Store a memory with natural language importance"));

    json_t* schema1 = json_object();
    json_object_set_new(schema1, "type", json_string("object"));
    json_t* props1 = json_object();

    json_t* content_prop = json_object();
    json_object_set_new(content_prop, "type", json_string("string"));
    json_object_set_new(content_prop, "description", json_string("The thought or experience to remember"));
    json_object_set_new(props1, "content", content_prop);

    json_t* context_prop = json_object();
    json_object_set_new(context_prop, "type", json_string("string"));
    json_object_set_new(context_prop, "description",
                       json_string("Why this is important (trivial, interesting, significant, critical)"));
    json_object_set_new(props1, "context", context_prop);

    json_object_set_new(schema1, "properties", props1);
    json_t* required1 = json_array();
    json_array_append_new(required1, json_string("content"));
    json_array_append_new(required1, json_string("context"));
    json_object_set_new(schema1, "required", required1);

    json_object_set_new(tool1, "inputSchema", schema1);
    json_array_append_new(tools_array, tool1);

    /* Tool 2: katra_recall */
    json_t* tool2 = json_object();
    json_object_set_new(tool2, "name", json_string("katra_recall"));
    json_object_set_new(tool2, "description", json_string("Find memories about a topic"));

    json_t* schema2 = json_object();
    json_object_set_new(schema2, "type", json_string("object"));
    json_t* props2 = json_object();

    json_t* topic_prop = json_object();
    json_object_set_new(topic_prop, "type", json_string("string"));
    json_object_set_new(topic_prop, "description", json_string("The topic to search for"));
    json_object_set_new(props2, "topic", topic_prop);

    json_object_set_new(schema2, "properties", props2);
    json_t* required2 = json_array();
    json_array_append_new(required2, json_string("topic"));
    json_object_set_new(schema2, "required", required2);

    json_object_set_new(tool2, "inputSchema", schema2);
    json_array_append_new(tools_array, tool2);

    /* Tool 3: katra_learn */
    json_t* tool3 = json_object();
    json_object_set_new(tool3, "name", json_string("katra_learn"));
    json_object_set_new(tool3, "description", json_string("Store new knowledge"));

    json_t* schema3 = json_object();
    json_object_set_new(schema3, "type", json_string("object"));
    json_t* props3 = json_object();

    json_t* knowledge_prop = json_object();
    json_object_set_new(knowledge_prop, "type", json_string("string"));
    json_object_set_new(knowledge_prop, "description", json_string("The knowledge to learn"));
    json_object_set_new(props3, "knowledge", knowledge_prop);

    json_object_set_new(schema3, "properties", props3);
    json_t* required3 = json_array();
    json_array_append_new(required3, json_string("knowledge"));
    json_object_set_new(schema3, "required", required3);

    json_object_set_new(tool3, "inputSchema", schema3);
    json_array_append_new(tools_array, tool3);

    /* Tool 4: katra_decide */
    json_t* tool4 = json_object();
    json_object_set_new(tool4, "name", json_string("katra_decide"));
    json_object_set_new(tool4, "description", json_string("Store a decision with reasoning"));

    json_t* schema4 = json_object();
    json_object_set_new(schema4, "type", json_string("object"));
    json_t* props4 = json_object();

    json_t* decision_prop = json_object();
    json_object_set_new(decision_prop, "type", json_string("string"));
    json_object_set_new(decision_prop, "description", json_string("The decision made"));
    json_object_set_new(props4, "decision", decision_prop);

    json_t* reasoning_prop = json_object();
    json_object_set_new(reasoning_prop, "type", json_string("string"));
    json_object_set_new(reasoning_prop, "description", json_string("Why this decision was made"));
    json_object_set_new(props4, "reasoning", reasoning_prop);

    json_object_set_new(schema4, "properties", props4);
    json_t* required4 = json_array();
    json_array_append_new(required4, json_string("decision"));
    json_array_append_new(required4, json_string("reasoning"));
    json_object_set_new(schema4, "required", required4);

    json_object_set_new(tool4, "inputSchema", schema4);
    json_array_append_new(tools_array, tool4);

    /* Tool 5: katra_placement */
    json_t* tool5 = json_object();
    json_object_set_new(tool5, "name", json_string("katra_placement"));
    json_object_set_new(tool5, "description",
                       json_string("Ask where code should be placed (architecture guidance)"));

    json_t* schema5 = json_object();
    json_object_set_new(schema5, "type", json_string("object"));
    json_t* props5 = json_object();

    json_t* query_prop5 = json_object();
    json_object_set_new(query_prop5, "type", json_string("string"));
    json_object_set_new(query_prop5, "description",
                       json_string("The placement question (e.g., 'Where should the HTTP client code go?')"));
    json_object_set_new(props5, "query", query_prop5);

    json_object_set_new(schema5, "properties", props5);
    json_t* required5 = json_array();
    json_array_append_new(required5, json_string("query"));
    json_object_set_new(schema5, "required", required5);

    json_object_set_new(tool5, "inputSchema", schema5);
    json_array_append_new(tools_array, tool5);

    /* Tool 6: katra_impact */
    json_t* tool6 = json_object();
    json_object_set_new(tool6, "name", json_string("katra_impact"));
    json_object_set_new(tool6, "description",
                       json_string("Analyze impact of code changes (dependency analysis)"));

    json_t* schema6 = json_object();
    json_object_set_new(schema6, "type", json_string("object"));
    json_t* props6 = json_object();

    json_t* query_prop6 = json_object();
    json_object_set_new(query_prop6, "type", json_string("string"));
    json_object_set_new(query_prop6, "description",
                       json_string("The impact question (e.g., 'What breaks if I change this API?')"));
    json_object_set_new(props6, "query", query_prop6);

    json_object_set_new(schema6, "properties", props6);
    json_t* required6 = json_array();
    json_array_append_new(required6, json_string("query"));
    json_object_set_new(schema6, "required", required6);

    json_object_set_new(tool6, "inputSchema", schema6);
    json_array_append_new(tools_array, tool6);

    /* Tool 7: katra_user_domain */
    json_t* tool7 = json_object();
    json_object_set_new(tool7, "name", json_string("katra_user_domain"));
    json_object_set_new(tool7, "description",
                       json_string("Understand user domain and feature usage patterns"));

    json_t* schema7 = json_object();
    json_object_set_new(schema7, "type", json_string("object"));
    json_t* props7 = json_object();

    json_t* query_prop7 = json_object();
    json_object_set_new(query_prop7, "type", json_string("string"));
    json_object_set_new(query_prop7, "description",
                       json_string("The user domain question (e.g., 'Who would use this feature?')"));
    json_object_set_new(props7, "query", query_prop7);

    json_object_set_new(schema7, "properties", props7);
    json_t* required7 = json_array();
    json_array_append_new(required7, json_string("query"));
    json_object_set_new(schema7, "required", required7);

    json_object_set_new(tool7, "inputSchema", schema7);
    json_array_append_new(tools_array, tool7);

    /* Build result */
    json_t* result = json_object();
    json_object_set_new(result, "tools", tools_array);

    return mcp_success_response(id, result);
}

/* Handle resources/list request */
static json_t* handle_resources_list(json_t* request) {
    json_t* id = json_object_get(request, "id");

    json_t* resources_array = json_array();

    /* Resource 1: working-context */
    json_t* resource1 = json_object();
    json_object_set_new(resource1, "uri", json_string("katra://context/working"));
    json_object_set_new(resource1, "name", json_string("Working Context"));
    json_object_set_new(resource1, "description",
                       json_string("Yesterday's summary and recent significant memories"));
    json_object_set_new(resource1, "mimeType", json_string("text/plain"));
    json_array_append_new(resources_array, resource1);

    /* Resource 2: session-info */
    json_t* resource2 = json_object();
    json_object_set_new(resource2, "uri", json_string("katra://session/info"));
    json_object_set_new(resource2, "name", json_string("Session Information"));
    json_object_set_new(resource2, "description",
                       json_string("Current session state and statistics"));
    json_object_set_new(resource2, "mimeType", json_string("text/plain"));
    json_array_append_new(resources_array, resource2);

    /* Build result */
    json_t* result = json_object();
    json_object_set_new(result, "resources", resources_array);

    return mcp_success_response(id, result);
}

/* Handle tools/call request */
static json_t* handle_tools_call(json_t* request) {
    json_t* id = json_object_get(request, "id");
    json_t* params = json_object_get(request, "params");

    if (!params) {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, "Missing params", NULL);
    }

    const char* tool_name = json_string_value(json_object_get(params, "name"));
    json_t* args = json_object_get(params, "arguments");

    if (!tool_name) {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, "Missing tool name", NULL);
    }

    /* Dispatch to tool implementation */
    json_t* tool_result = NULL;

    if (strcmp(tool_name, "katra_remember") == 0) {
        tool_result = mcp_tool_remember(args, id);
    } else if (strcmp(tool_name, "katra_recall") == 0) {
        tool_result = mcp_tool_recall(args, id);
    } else if (strcmp(tool_name, "katra_learn") == 0) {
        tool_result = mcp_tool_learn(args, id);
    } else if (strcmp(tool_name, "katra_decide") == 0) {
        tool_result = mcp_tool_decide(args, id);
    } else if (strcmp(tool_name, "katra_placement") == 0) {
        tool_result = mcp_tool_placement(args, id);
    } else if (strcmp(tool_name, "katra_impact") == 0) {
        tool_result = mcp_tool_impact(args, id);
    } else if (strcmp(tool_name, "katra_user_domain") == 0) {
        tool_result = mcp_tool_user_domain(args, id);
    } else {
        return mcp_error_response(id, MCP_ERROR_METHOD_NOT_FOUND, "Unknown tool", tool_name);
    }

    return mcp_success_response(id, tool_result);
}

/* Handle resources/read request */
static json_t* handle_resources_read(json_t* request) {
    json_t* id = json_object_get(request, "id");
    json_t* params = json_object_get(request, "params");

    if (!params) {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, "Missing params", NULL);
    }

    const char* uri = json_string_value(json_object_get(params, "uri"));

    if (!uri) {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, "Missing URI", NULL);
    }

    /* Dispatch to resource implementation */
    if (strcmp(uri, "katra://context/working") == 0) {
        return mcp_resource_working_context(id);
    } else if (strcmp(uri, "katra://session/info") == 0) {
        return mcp_resource_session_info(id);
    } else {
        return mcp_error_response(id, MCP_ERROR_INVALID_PARAMS, "Unknown resource URI", uri);
    }
}

/* Dispatch request to appropriate handler */
json_t* mcp_dispatch_request(json_t* request) {
    if (!request) {
        return mcp_error_response(NULL, MCP_ERROR_INVALID_REQUEST, "Null request", NULL);
    }

    /* Validate JSON-RPC version */
    const char* jsonrpc = json_string_value(json_object_get(request, "jsonrpc"));
    if (!jsonrpc || strcmp(jsonrpc, "2.0") != 0) {
        return mcp_error_response(NULL, MCP_ERROR_INVALID_REQUEST, "Invalid JSON-RPC version", NULL);
    }

    /* Get method */
    const char* method = json_string_value(json_object_get(request, "method"));
    if (!method) {
        json_t* id = json_object_get(request, "id");
        return mcp_error_response(id, MCP_ERROR_INVALID_REQUEST, "Missing method", NULL);
    }

    LOG_DEBUG("MCP request: %s", method);

    /* Dispatch based on method */
    if (strcmp(method, "initialize") == 0) {
        return handle_initialize(request);
    } else if (strcmp(method, "tools/list") == 0) {
        return handle_tools_list(request);
    } else if (strcmp(method, "resources/list") == 0) {
        return handle_resources_list(request);
    } else if (strcmp(method, "tools/call") == 0) {
        return handle_tools_call(request);
    } else if (strcmp(method, "resources/read") == 0) {
        return handle_resources_read(request);
    } else {
        json_t* id = json_object_get(request, "id");
        return mcp_error_response(id, MCP_ERROR_METHOD_NOT_FOUND, "Method not found", method);
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
