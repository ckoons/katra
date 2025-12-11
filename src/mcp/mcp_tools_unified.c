/* © 2025 Casey Koons All rights reserved */

/*
 * MCP Unified Tool - Thin wrapper for Katra operations
 *
 * This tool provides a single entry point for all Katra operations,
 * forwarding requests to the unified HTTP daemon for processing.
 * Reduces tool definition overhead from ~14,100 tokens to ~800 tokens.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_unified.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Check if Unix socket exists */
static bool unix_socket_available(void) {
    struct stat sb;
    if (stat(KATRA_UNIFIED_SOCKET_PATH, &sb) == 0) {
        return S_ISSOCK(sb.st_mode);
    }
    return false;
}

/* Response buffer for curl */
typedef struct {
    char* data;
    size_t size;
} response_buffer_t;

/* Curl write callback */
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t real_size = size * nmemb;
    response_buffer_t* mem = (response_buffer_t*)userp;

    char* ptr = realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) {
        return 0;  /* Out of memory */
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = '\0';

    return real_size;
}

/* Forward operation to unified daemon via Unix socket or HTTP */
static json_t* forward_to_daemon(json_t* shared_state) {
    CURL* curl = NULL;
    CURLcode res;
    json_t* result = NULL;
    response_buffer_t response = {NULL, 0};
    struct curl_slist* headers = NULL;
    char* request_body = NULL;
    bool use_unix_socket = false;

    /* Convert shared_state to JSON string */
    request_body = json_dumps(shared_state, JSON_COMPACT);
    if (!request_body) {
        katra_report_error(E_SYSTEM_MEMORY, "forward_to_daemon",
                          "Failed to serialize request");
        return NULL;
    }

    /* Initialize response buffer */
    response.data = malloc(1);
    if (!response.data) {
        free(request_body);
        return NULL;
    }
    response.data[0] = '\0';

    /* Initialize curl */
    curl = curl_easy_init();
    if (!curl) {
        free(request_body);
        free(response.data);
        katra_report_error(E_SYSTEM_MEMORY, "forward_to_daemon",
                          "Failed to initialize curl");
        return NULL;
    }

    /* Set up headers */
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!headers) {
        curl_easy_cleanup(curl);
        free(request_body);
        free(response.data);
        return NULL;
    }

    /* Check if Unix socket is available (preferred for local fast path) */
    use_unix_socket = unix_socket_available();
    if (use_unix_socket) {
        /* Use Unix socket for local communication */
        curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, KATRA_UNIFIED_SOCKET_PATH);
        curl_easy_setopt(curl, CURLOPT_URL, KATRA_UNIFIED_SOCKET_URL);
        LOG_DEBUG("Using Unix socket: %s", KATRA_UNIFIED_SOCKET_PATH);
    } else {
        /* Fall back to HTTP */
        char url[KATRA_PATH_MAX];
        snprintf(url, sizeof(url), KATRA_UNIFIED_HTTP_URL_FMT,
                 KATRA_UNIFIED_DEFAULT_PORT);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        LOG_DEBUG("Using HTTP: port %d", KATRA_UNIFIED_DEFAULT_PORT);
    }

    /* Configure curl */
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, KATRA_UNIFIED_TIMEOUT_SECS);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, KATRA_UNIFIED_CONNECT_TIMEOUT);

    /* Perform request */
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        LOG_ERROR("Daemon request failed: %s", curl_easy_strerror(res));
        /* Return error as JSON */
        result = json_object();
        json_object_set_new(result, "error",
                           json_string(curl_easy_strerror(res)));
    } else {
        /* Parse response */
        json_error_t error;
        result = json_loads(response.data, 0, &error);
        if (!result) {
            LOG_ERROR("Failed to parse daemon response: %s", error.text);
            result = json_object();
            json_object_set_new(result, "error",
                               json_string("Failed to parse daemon response"));
        }
    }

    /* Cleanup */
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(request_body);
    free(response.data);

    return result;
}

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

    /* Forward to daemon */
    json_t* daemon_response = forward_to_daemon(shared_state);
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
