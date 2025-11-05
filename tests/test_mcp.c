/* © 2025 Casey Koons All rights reserved */

/* MCP Server Integration Tests */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_init.h"
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"

#define TEST_CI_ID "test_mcp_ci"

/* Mock globals for MCP tools (normally defined in katra_mcp_server.c) */
char g_persona_name[256] = "test_persona";
char g_ci_id[256] = TEST_CI_ID;

/* Mock session state for testing */
static mcp_session_t test_session = {
    .chosen_name = "TestUser",
    .role = "developer",
    .registered = true,
    .first_call = false,
    .connected_at = 0
};

/* Mock session state functions */
mcp_session_t* mcp_get_session(void) {
    return &test_session;
}

const char* mcp_get_session_name(void) {
    return test_session.chosen_name;
}

bool mcp_is_registered(void) {
    return test_session.registered;
}

bool mcp_is_first_call(void) {
    return test_session.first_call;
}

void mcp_mark_first_call_complete(void) {
    test_session.first_call = false;
}

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

/* Helper: Check if response is successful */
static int is_success_response(json_t* response) {
    json_t* result = json_object_get(response, "result");
    json_t* error = json_object_get(response, "error");
    return (result != NULL && error == NULL);
}

/* Helper: Check if response is error */
static int is_error_response(json_t* response) {
    json_t* error = json_object_get(response, "error");
    return (error != NULL);
}

/* Test: Parse valid JSON-RPC request */
static int test_parse_request(void) {
    printf("Testing JSON-RPC request parsing...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"initialize\",\"id\":1}";
    json_t* request = mcp_parse_request(json);

    if (!request) {
        printf("  ✗ Failed to parse valid request\n");
        return 1;
    }

    const char* jsonrpc = json_string_value(json_object_get(request, "jsonrpc"));
    const char* method = json_string_value(json_object_get(request, "method"));

    if (!jsonrpc || strcmp(jsonrpc, "2.0") != 0) {
        printf("  ✗ Invalid jsonrpc version\n");
        json_decref(request);
        return 1;
    }

    if (!method || strcmp(method, "initialize") != 0) {
        printf("  ✗ Invalid method\n");
        json_decref(request);
        return 1;
    }

    json_decref(request);
    tests_passed++;
    printf("  ✓ Request parsing works\n");
    return 0;
}

/* Test: Initialize handshake */
static int test_initialize(void) {
    printf("Testing initialize handshake...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"initialize\",\"id\":1,\"params\":{\"protocolVersion\":\"2024-11-05\"}}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_success_response(response)) {
        printf("  ✗ Initialize failed\n");
        if (response) json_decref(response);
        return 1;
    }

    json_t* result = json_object_get(response, "result");
    json_t* server_info = json_object_get(result, "serverInfo");
    const char* name = json_string_value(json_object_get(server_info, "name"));

    if (!name || strcmp(name, MCP_SERVER_NAME) != 0) {
        printf("  ✗ Server name incorrect\n");
        json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ Initialize handshake works\n");
    return 0;
}

/* Test: List tools */
static int test_tools_list(void) {
    printf("Testing tools/list...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"tools/list\",\"id\":2}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_success_response(response)) {
        printf("  ✗ tools/list failed\n");
        if (response) json_decref(response);
        return 1;
    }

    json_t* result = json_object_get(response, "result");
    json_t* tools = json_object_get(result, "tools");

    if (!json_is_array(tools)) {
        printf("  ✗ tools is not an array\n");
        json_decref(response);
        return 1;
    }

    size_t tool_count = json_array_size(tools);
    if (tool_count != 10) {
        printf("  ✗ Expected 10 tools, got %zu\n", tool_count);
        json_decref(response);
        return 1;
    }

    /* Verify first tool has required fields */
    json_t* tool = json_array_get(tools, 0);
    const char* name = json_string_value(json_object_get(tool, "name"));
    const char* desc = json_string_value(json_object_get(tool, "description"));
    json_t* schema = json_object_get(tool, "inputSchema");

    if (!name || !desc || !schema) {
        printf("  ✗ Tool missing required fields\n");
        json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ tools/list returns 10 tools\n");
    return 0;
}

/* Test: List resources */
static int test_resources_list(void) {
    printf("Testing resources/list...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"resources/list\",\"id\":3}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_success_response(response)) {
        printf("  ✗ resources/list failed\n");
        if (response) json_decref(response);
        return 1;
    }

    json_t* result = json_object_get(response, "result");
    json_t* resources = json_object_get(result, "resources");

    if (!json_is_array(resources)) {
        printf("  ✗ resources is not an array\n");
        json_decref(response);
        return 1;
    }

    size_t resource_count = json_array_size(resources);
    if (resource_count != 6) {
        printf("  ✗ Expected 6 resources, got %zu\n", resource_count);
        json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ resources/list returns 6 resources\n");
    return 0;
}

/* Test: katra_remember tool */
static int test_tool_remember(void) {
    printf("Testing katra_remember tool...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"tools/call\",\"id\":4,"
                      "\"params\":{\"name\":\"katra_remember\","
                      "\"arguments\":{\"content\":\"Test memory\",\"context\":\"This is interesting\"}}}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_success_response(response)) {
        printf("  ✗ katra_remember failed\n");
        if (response) json_decref(response);
        return 1;
    }

    json_t* result = json_object_get(response, "result");
    json_t* content = json_object_get(result, "content");

    if (!json_is_array(content) || json_array_size(content) == 0) {
        printf("  ✗ Invalid response format\n");
        json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ katra_remember stores memory\n");
    return 0;
}

/* Test: katra_recall tool */
static int test_tool_recall(void) {
    printf("Testing katra_recall tool...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"tools/call\",\"id\":5,"
                      "\"params\":{\"name\":\"katra_recall\","
                      "\"arguments\":{\"topic\":\"test\"}}}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_success_response(response)) {
        printf("  ✗ katra_recall failed\n");
        if (response) json_decref(response);
        return 1;
    }

    json_t* result = json_object_get(response, "result");
    json_t* content = json_object_get(result, "content");

    if (!json_is_array(content)) {
        printf("  ✗ Invalid response format\n");
        json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ katra_recall returns results\n");
    return 0;
}

/* Test: katra_learn tool */
static int test_tool_learn(void) {
    printf("Testing katra_learn tool...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"tools/call\",\"id\":6,"
                      "\"params\":{\"name\":\"katra_learn\","
                      "\"arguments\":{\"knowledge\":\"The sky is blue\"}}}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_success_response(response)) {
        printf("  ✗ katra_learn failed\n");
        if (response) json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ katra_learn stores knowledge\n");
    return 0;
}

/* Test: katra_decide tool */
static int test_tool_decide(void) {
    printf("Testing katra_decide tool...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"tools/call\",\"id\":7,"
                      "\"params\":{\"name\":\"katra_decide\","
                      "\"arguments\":{\"decision\":\"Use tabs\",\"reasoning\":\"Better readability\"}}}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_success_response(response)) {
        printf("  ✗ katra_decide failed\n");
        if (response) json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ katra_decide stores decision\n");
    return 0;
}

/* Test: working-context resource */
static int test_resource_working_context(void) {
    printf("Testing working-context resource...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"resources/read\",\"id\":8,"
                      "\"params\":{\"uri\":\"katra://context/working\"}}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_success_response(response)) {
        printf("  ✗ working-context read failed\n");
        if (response) json_decref(response);
        return 1;
    }

    json_t* result = json_object_get(response, "result");
    json_t* contents = json_object_get(result, "contents");

    if (!json_is_array(contents) || json_array_size(contents) == 0) {
        printf("  ✗ Invalid response format\n");
        json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ working-context resource works\n");
    return 0;
}

/* Test: session-info resource */
static int test_resource_session_info(void) {
    printf("Testing session-info resource...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"resources/read\",\"id\":9,"
                      "\"params\":{\"uri\":\"katra://session/info\"}}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_success_response(response)) {
        printf("  ✗ session-info read failed\n");
        if (response) json_decref(response);
        return 1;
    }

    json_t* result = json_object_get(response, "result");
    json_t* contents = json_object_get(result, "contents");

    if (!json_is_array(contents) || json_array_size(contents) == 0) {
        printf("  ✗ Invalid response format\n");
        json_decref(response);
        return 1;
    }

    json_t* content_item = json_array_get(contents, 0);
    const char* text = json_string_value(json_object_get(content_item, "text"));

    if (!text || strstr(text, "Session Information") == NULL) {
        printf("  ✗ Session info text invalid\n");
        json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ session-info resource works\n");
    return 0;
}

/* Test: Error handling - invalid method */
static int test_error_invalid_method(void) {
    printf("Testing error handling (invalid method)...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"invalid_method\",\"id\":10}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_error_response(response)) {
        printf("  ✗ Should return error for invalid method\n");
        if (response) json_decref(response);
        return 1;
    }

    json_t* error = json_object_get(response, "error");
    int code = json_integer_value(json_object_get(error, "code"));

    if (code != MCP_ERROR_METHOD_NOT_FOUND) {
        printf("  ✗ Wrong error code: expected %d, got %d\n", MCP_ERROR_METHOD_NOT_FOUND, code);
        json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ Invalid method returns correct error\n");
    return 0;
}

/* Test: Error handling - missing parameters */
static int test_error_missing_params(void) {
    printf("Testing error handling (missing params)...\n");
    tests_run++;

    const char* json = "{\"jsonrpc\":\"2.0\",\"method\":\"tools/call\",\"id\":11,"
                      "\"params\":{\"name\":\"katra_remember\",\"arguments\":{}}}";
    json_t* request = mcp_parse_request(json);
    json_t* response = mcp_dispatch_request(request);
    json_decref(request);

    if (!response || !is_success_response(response)) {
        printf("  ✗ Dispatch failed unexpectedly\n");
        if (response) json_decref(response);
        return 1;
    }

    /* Tool should return error content */
    json_t* result = json_object_get(response, "result");
    json_t* is_error = json_object_get(result, "isError");

    if (!is_error || !json_is_true(is_error)) {
        printf("  ✗ Tool should return error for missing params\n");
        json_decref(response);
        return 1;
    }

    json_decref(response);
    tests_passed++;
    printf("  ✓ Missing params returns tool error\n");
    return 0;
}

/* Test: Response builder functions */
static int test_response_builders(void) {
    printf("Testing response builder functions...\n");
    tests_run++;

    /* Test success response */
    json_t* id = json_integer(1);
    json_t* result = json_object();
    json_object_set_new(result, "test", json_string("value"));
    json_t* success = mcp_success_response(id, result);

    if (!success || !is_success_response(success)) {
        printf("  ✗ Success response builder failed\n");
        if (success) json_decref(success);
        json_decref(id);
        return 1;
    }
    json_decref(success);

    /* Test error response */
    json_t* error = mcp_error_response(id, MCP_ERROR_INTERNAL, "Test error", "Details");

    if (!error || !is_error_response(error)) {
        printf("  ✗ Error response builder failed\n");
        if (error) json_decref(error);
        json_decref(id);
        return 1;
    }
    json_decref(error);
    json_decref(id);

    /* Test tool success */
    json_t* tool_success = mcp_tool_success("Success message");
    if (!tool_success) {
        printf("  ✗ Tool success builder failed\n");
        return 1;
    }
    json_decref(tool_success);

    /* Test tool error */
    json_t* tool_error = mcp_tool_error("Error message", "Details");
    if (!tool_error) {
        printf("  ✗ Tool error builder failed\n");
        return 1;
    }
    json_decref(tool_error);

    tests_passed++;
    printf("  ✓ Response builders work correctly\n");
    return 0;
}

int main(void) {
    int failures = 0;

    printf("========================================\n");
    printf("Katra MCP Server Tests\n");
    printf("========================================\n\n");

    /* Initialize Katra */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        printf("FATAL: katra_init failed: %d\n", result);
        return 1;
    }

    result = katra_memory_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("FATAL: katra_memory_init failed: %d\n", result);
        katra_exit();
        return 1;
    }

    result = breathe_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("FATAL: breathe_init failed: %d\n", result);
        katra_memory_cleanup();
        katra_exit();
        return 1;
    }

    result = session_start(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("FATAL: session_start failed: %d\n", result);
        breathe_cleanup();
        katra_memory_cleanup();
        katra_exit();
        return 1;
    }

    /* Run all tests */
    failures += test_parse_request();
    failures += test_initialize();
    failures += test_tools_list();
    failures += test_resources_list();
    failures += test_tool_remember();
    failures += test_tool_recall();
    failures += test_tool_learn();
    failures += test_tool_decide();
    failures += test_resource_working_context();
    failures += test_resource_session_info();
    failures += test_error_invalid_method();
    failures += test_error_missing_params();
    failures += test_response_builders();

    /* Cleanup */
    session_end();
    breathe_cleanup();
    katra_memory_cleanup();
    katra_exit();

    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", failures);
    printf("========================================\n");

    return failures > 0 ? 1 : 0;
}
