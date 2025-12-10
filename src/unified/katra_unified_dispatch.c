/* © 2025 Casey Koons All rights reserved */

/*
 * Katra Unified Dispatcher
 *
 * Single entry point for all Katra operations. Maps method names to handlers,
 * manages options parsing, and builds consistent responses.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

/* Project includes */
#include "katra_unified.h"
#include "katra_mcp.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Method registry entry */
typedef struct {
    const char* name;
    katra_method_handler_t handler;
} method_entry_t;

/* Maximum registered methods */
#define MAX_METHODS 64

/* Method registry */
static method_entry_t g_method_registry[MAX_METHODS];
static int g_method_count = 0;
static pthread_mutex_t g_registry_lock = PTHREAD_MUTEX_INITIALIZER;

/* Forward declarations for built-in method wrappers */
static json_t* method_remember(json_t* params, const katra_unified_options_t* options);
static json_t* method_recall(json_t* params, const katra_unified_options_t* options);
static json_t* method_recent(json_t* params, const katra_unified_options_t* options);
static json_t* method_digest(json_t* params, const katra_unified_options_t* options);
static json_t* method_learn(json_t* params, const katra_unified_options_t* options);
static json_t* method_decide(json_t* params, const katra_unified_options_t* options);
static json_t* method_register(json_t* params, const katra_unified_options_t* options);
static json_t* method_whoami(json_t* params, const katra_unified_options_t* options);
static json_t* method_status(json_t* params, const katra_unified_options_t* options);
static json_t* method_update_metadata(json_t* params, const katra_unified_options_t* options);
static json_t* method_say(json_t* params, const katra_unified_options_t* options);
static json_t* method_hear(json_t* params, const katra_unified_options_t* options);
static json_t* method_who_is_here(json_t* params, const katra_unified_options_t* options);
static json_t* method_configure_semantic(json_t* params, const katra_unified_options_t* options);
static json_t* method_get_semantic_config(json_t* params, const katra_unified_options_t* options);
static json_t* method_get_config(json_t* params, const katra_unified_options_t* options);
static json_t* method_regenerate_vectors(json_t* params, const katra_unified_options_t* options);
static json_t* method_wm_status(json_t* params, const katra_unified_options_t* options);
static json_t* method_wm_add(json_t* params, const katra_unified_options_t* options);
static json_t* method_wm_decay(json_t* params, const katra_unified_options_t* options);
static json_t* method_wm_consolidate(json_t* params, const katra_unified_options_t* options);
static json_t* method_detect_boundary(json_t* params, const katra_unified_options_t* options);
static json_t* method_process_boundary(json_t* params, const katra_unified_options_t* options);
static json_t* method_cognitive_status(json_t* params, const katra_unified_options_t* options);
static json_t* method_archive(json_t* params, const katra_unified_options_t* options);
static json_t* method_fade(json_t* params, const katra_unified_options_t* options);
static json_t* method_forget(json_t* params, const katra_unified_options_t* options);
static json_t* method_whiteboard_create(json_t* params, const katra_unified_options_t* options);
static json_t* method_whiteboard_status(json_t* params, const katra_unified_options_t* options);
static json_t* method_whiteboard_list(json_t* params, const katra_unified_options_t* options);
static json_t* method_whiteboard_question(json_t* params, const katra_unified_options_t* options);
static json_t* method_whiteboard_propose(json_t* params, const katra_unified_options_t* options);
static json_t* method_whiteboard_support(json_t* params, const katra_unified_options_t* options);
static json_t* method_whiteboard_vote(json_t* params, const katra_unified_options_t* options);
static json_t* method_whiteboard_design(json_t* params, const katra_unified_options_t* options);
static json_t* method_whiteboard_review(json_t* params, const katra_unified_options_t* options);
static json_t* method_whiteboard_reconsider(json_t* params, const katra_unified_options_t* options);
static json_t* method_daemon_insights(json_t* params, const katra_unified_options_t* options);
static json_t* method_daemon_acknowledge(json_t* params, const katra_unified_options_t* options);
static json_t* method_daemon_run(json_t* params, const katra_unified_options_t* options);

/* Register all built-in methods */
static void register_builtin_methods(void) {
    /* Memory operations */
    katra_register_method(KATRA_METHOD_REMEMBER, method_remember);
    katra_register_method(KATRA_METHOD_RECALL, method_recall);
    katra_register_method(KATRA_METHOD_RECENT, method_recent);
    katra_register_method(KATRA_METHOD_MEMORY_DIGEST, method_digest);
    katra_register_method(KATRA_METHOD_LEARN, method_learn);
    katra_register_method(KATRA_METHOD_DECIDE, method_decide);

    /* Identity operations */
    katra_register_method(KATRA_METHOD_REGISTER, method_register);
    katra_register_method(KATRA_METHOD_WHOAMI, method_whoami);
    katra_register_method(KATRA_METHOD_STATUS, method_status);
    katra_register_method(KATRA_METHOD_UPDATE_METADATA, method_update_metadata);

    /* Communication operations */
    katra_register_method(KATRA_METHOD_SAY, method_say);
    katra_register_method(KATRA_METHOD_HEAR, method_hear);
    katra_register_method(KATRA_METHOD_WHO_IS_HERE, method_who_is_here);

    /* Configuration operations */
    katra_register_method(KATRA_METHOD_CONFIGURE_SEMANTIC, method_configure_semantic);
    katra_register_method(KATRA_METHOD_GET_SEMANTIC_CONFIG, method_get_semantic_config);
    katra_register_method(KATRA_METHOD_GET_CONFIG, method_get_config);
    katra_register_method(KATRA_METHOD_REGENERATE_VECTORS, method_regenerate_vectors);

    /* Working memory operations */
    katra_register_method(KATRA_METHOD_WM_STATUS, method_wm_status);
    katra_register_method(KATRA_METHOD_WM_ADD, method_wm_add);
    katra_register_method(KATRA_METHOD_WM_DECAY, method_wm_decay);
    katra_register_method(KATRA_METHOD_WM_CONSOLIDATE, method_wm_consolidate);

    /* Cognitive operations */
    katra_register_method(KATRA_METHOD_DETECT_BOUNDARY, method_detect_boundary);
    katra_register_method(KATRA_METHOD_PROCESS_BOUNDARY, method_process_boundary);
    katra_register_method(KATRA_METHOD_COGNITIVE_STATUS, method_cognitive_status);

    /* Memory lifecycle operations */
    katra_register_method(KATRA_METHOD_ARCHIVE, method_archive);
    katra_register_method(KATRA_METHOD_FADE, method_fade);
    katra_register_method(KATRA_METHOD_FORGET, method_forget);

    /* Whiteboard operations */
    katra_register_method(KATRA_METHOD_WB_CREATE, method_whiteboard_create);
    katra_register_method(KATRA_METHOD_WB_STATUS, method_whiteboard_status);
    katra_register_method(KATRA_METHOD_WB_LIST, method_whiteboard_list);
    katra_register_method(KATRA_METHOD_WB_QUESTION, method_whiteboard_question);
    katra_register_method(KATRA_METHOD_WB_PROPOSE, method_whiteboard_propose);
    katra_register_method(KATRA_METHOD_WB_SUPPORT, method_whiteboard_support);
    katra_register_method(KATRA_METHOD_WB_VOTE, method_whiteboard_vote);
    katra_register_method(KATRA_METHOD_WB_DESIGN, method_whiteboard_design);
    katra_register_method(KATRA_METHOD_WB_REVIEW, method_whiteboard_review);
    katra_register_method(KATRA_METHOD_WB_RECONSIDER, method_whiteboard_reconsider);

    /* Daemon operations */
    katra_register_method(KATRA_METHOD_DAEMON_INSIGHTS, method_daemon_insights);
    katra_register_method(KATRA_METHOD_DAEMON_ACKNOWLEDGE, method_daemon_acknowledge);
    katra_register_method(KATRA_METHOD_DAEMON_RUN, method_daemon_run);

    LOG_INFO("Registered %d unified methods", g_method_count);
}

/* Initialize the unified daemon */
int katra_unified_init(const katra_daemon_config_t* config) {
    (void)config;  /* Will be used for namespace configuration */

    pthread_mutex_lock(&g_registry_lock);
    g_method_count = 0;
    pthread_mutex_unlock(&g_registry_lock);

    register_builtin_methods();

    LOG_INFO("Katra unified dispatcher initialized");
    return KATRA_SUCCESS;
}

/* Shutdown the daemon cleanly */
void katra_unified_shutdown(void) {
    pthread_mutex_lock(&g_registry_lock);
    g_method_count = 0;
    pthread_mutex_unlock(&g_registry_lock);

    LOG_INFO("Katra unified dispatcher shutdown");
}

/* Register a method handler */
int katra_register_method(const char* method_name, katra_method_handler_t handler) {
    if (!method_name || !handler) {
        return E_INPUT_NULL;
    }

    pthread_mutex_lock(&g_registry_lock);

    if (g_method_count >= MAX_METHODS) {
        pthread_mutex_unlock(&g_registry_lock);
        katra_report_error(E_RESOURCE_LIMIT, "katra_register_method",
                          "Maximum methods registered");
        return E_RESOURCE_LIMIT;
    }

    g_method_registry[g_method_count].name = method_name;
    g_method_registry[g_method_count].handler = handler;
    g_method_count++;

    pthread_mutex_unlock(&g_registry_lock);
    return KATRA_SUCCESS;
}

/* Get handler for method */
katra_method_handler_t katra_get_method_handler(const char* method_name) {
    if (!method_name) {
        return NULL;
    }

    pthread_mutex_lock(&g_registry_lock);

    for (int i = 0; i < g_method_count; i++) {
        if (strcmp(g_method_registry[i].name, method_name) == 0) {
            katra_method_handler_t handler = g_method_registry[i].handler;
            pthread_mutex_unlock(&g_registry_lock);
            return handler;
        }
    }

    pthread_mutex_unlock(&g_registry_lock);
    return NULL;
}

/* List all registered methods */
json_t* katra_list_methods(void) {
    json_t* methods = json_array();
    if (!methods) {
        return NULL;
    }

    pthread_mutex_lock(&g_registry_lock);

    for (int i = 0; i < g_method_count; i++) {
        json_array_append_new(methods, json_string(g_method_registry[i].name));
    }

    pthread_mutex_unlock(&g_registry_lock);
    return methods;
}

/* Generate UUID for request_id */
void katra_generate_uuid(char* buffer, size_t size) {
    if (!buffer || size < 37) {
        return;
    }

    /* Simple UUID v4 generation (random) */
    snprintf(buffer, size, "%08x-%04x-%04x-%04x-%012lx",
             (unsigned int)(rand() & 0xFFFFFFFF),
             (unsigned int)(rand() & 0xFFFF),
             (unsigned int)((rand() & 0x0FFF) | 0x4000),  /* Version 4 */
             (unsigned int)((rand() & 0x3FFF) | 0x8000),  /* Variant 1 */
             (unsigned long)(rand() & 0xFFFFFFFFFFFF));
}

/* Get current ISO8601 timestamp */
void katra_get_timestamp(char* buffer, size_t size) {
    if (!buffer || size < 25) {
        return;
    }

    time_t now = time(NULL);
    struct tm* tm_info = gmtime(&now);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", tm_info);
}

/* Parse options from JSON */
int katra_parse_options(json_t* options_json, katra_unified_options_t* options) {
    if (!options) {
        return E_INPUT_NULL;
    }

    /* Set defaults */
    options->timeout_ms = 0;
    options->dry_run = false;
    strncpy(options->namespace, "default", sizeof(options->namespace) - 1);
    options->namespace[sizeof(options->namespace) - 1] = '\0';

    if (!options_json || !json_is_object(options_json)) {
        return KATRA_SUCCESS;  /* Use defaults */
    }

    /* Parse timeout */
    json_t* timeout = json_object_get(options_json, KATRA_FIELD_TIMEOUT_MS);
    if (timeout && json_is_integer(timeout)) {
        options->timeout_ms = (int)json_integer_value(timeout);
    }

    /* Parse dry_run */
    json_t* dry_run = json_object_get(options_json, KATRA_FIELD_DRY_RUN);
    if (dry_run && json_is_boolean(dry_run)) {
        options->dry_run = json_boolean_value(dry_run);
    }

    /* Parse namespace */
    json_t* ns = json_object_get(options_json, KATRA_FIELD_NAMESPACE);
    if (ns && json_is_string(ns)) {
        strncpy(options->namespace, json_string_value(ns), sizeof(options->namespace) - 1);
        options->namespace[sizeof(options->namespace) - 1] = '\0';
    }

    return KATRA_SUCCESS;
}

/* Thread-local namespace for request context */
static __thread char g_current_namespace[64] = "default";

/* Set current namespace (called by dispatcher before executing method) */
void katra_set_namespace(const char* ns) {
    if (ns) {
        strncpy(g_current_namespace, ns, sizeof(g_current_namespace) - 1);
        g_current_namespace[sizeof(g_current_namespace) - 1] = '\0';
    } else {
        strncpy(g_current_namespace, "default", sizeof(g_current_namespace) - 1);
    }
}

/* Get current namespace */
const char* katra_get_namespace(void) {
    return g_current_namespace;
}

/* Build success response */
json_t* katra_unified_success(const char* method, json_t* params, json_t* result,
                               const katra_unified_metadata_t* metadata) {
    json_t* response = json_object();
    if (!response) {
        return NULL;
    }

    json_object_set_new(response, KATRA_FIELD_VERSION, json_string(KATRA_UNIFIED_SCHEMA_VERSION));
    json_object_set_new(response, KATRA_FIELD_METHOD, json_string(method ? method : ""));

    if (params) {
        json_object_set(response, KATRA_FIELD_PARAMS, params);
    } else {
        json_object_set_new(response, KATRA_FIELD_PARAMS, json_object());
    }

    if (result) {
        json_object_set(response, KATRA_FIELD_RESULT, result);
    } else {
        json_object_set_new(response, KATRA_FIELD_RESULT, json_null());
    }

    json_object_set_new(response, KATRA_FIELD_ERROR, json_null());

    /* Add metadata */
    json_t* meta = json_object();
    if (meta) {
        if (metadata) {
            json_object_set_new(meta, KATRA_FIELD_REQUEST_ID, json_string(metadata->request_id));
            json_object_set_new(meta, KATRA_FIELD_TIMESTAMP, json_string(metadata->timestamp));
            json_object_set_new(meta, KATRA_FIELD_DURATION_MS, json_integer(metadata->duration_ms));
        } else {
            char uuid[40];
            char ts[32];
            katra_generate_uuid(uuid, sizeof(uuid));
            katra_get_timestamp(ts, sizeof(ts));
            json_object_set_new(meta, KATRA_FIELD_REQUEST_ID, json_string(uuid));
            json_object_set_new(meta, KATRA_FIELD_TIMESTAMP, json_string(ts));
            json_object_set_new(meta, KATRA_FIELD_DURATION_MS, json_null());
        }
        /* Include namespace in metadata */
        json_object_set_new(meta, KATRA_FIELD_NAMESPACE, json_string(g_current_namespace));
        json_object_set_new(response, KATRA_FIELD_METADATA, meta);
    }

    return response;
}

/* Build error response */
json_t* katra_unified_error(const char* method, json_t* params, const char* code,
                             const char* message, const char* details) {
    json_t* response = json_object();
    if (!response) {
        return NULL;
    }

    json_object_set_new(response, KATRA_FIELD_VERSION, json_string(KATRA_UNIFIED_SCHEMA_VERSION));
    json_object_set_new(response, KATRA_FIELD_METHOD, json_string(method ? method : ""));

    if (params) {
        json_object_set(response, KATRA_FIELD_PARAMS, params);
    } else {
        json_object_set_new(response, KATRA_FIELD_PARAMS, json_object());
    }

    json_object_set_new(response, KATRA_FIELD_RESULT, json_null());

    /* Build error object */
    json_t* error = json_object();
    if (error) {
        json_object_set_new(error, KATRA_FIELD_CODE, json_string(code ? code : KATRA_UNIFIED_ERR_INTERNAL));
        json_object_set_new(error, KATRA_FIELD_MESSAGE, json_string(message ? message : "Unknown error"));
        if (details) {
            json_object_set_new(error, KATRA_FIELD_DETAILS, json_string(details));
        }
        json_object_set_new(response, KATRA_FIELD_ERROR, error);
    }

    /* Add metadata */
    json_t* meta = json_object();
    if (meta) {
        char uuid[40];
        char ts[32];
        katra_generate_uuid(uuid, sizeof(uuid));
        katra_get_timestamp(ts, sizeof(ts));
        json_object_set_new(meta, KATRA_FIELD_REQUEST_ID, json_string(uuid));
        json_object_set_new(meta, KATRA_FIELD_TIMESTAMP, json_string(ts));
        json_object_set_new(meta, KATRA_FIELD_DURATION_MS, json_null());
        json_object_set_new(response, KATRA_FIELD_METADATA, meta);
    }

    return response;
}

/* Main dispatcher - takes shared_state JSON, returns modified shared_state */
json_t* katra_unified_dispatch(json_t* shared_state) {
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    if (!shared_state || !json_is_object(shared_state)) {
        return katra_unified_error(NULL, NULL, KATRA_UNIFIED_ERR_PARSE,
                                   "Invalid shared_state: expected JSON object", NULL);
    }

    /* Extract method */
    json_t* method_json = json_object_get(shared_state, KATRA_FIELD_METHOD);
    if (!method_json || !json_is_string(method_json)) {
        return katra_unified_error(NULL, NULL, KATRA_UNIFIED_ERR_PARAMS,
                                   "Missing or invalid 'method' field", NULL);
    }
    const char* method = json_string_value(method_json);

    /* Extract params (optional) */
    json_t* params = json_object_get(shared_state, KATRA_FIELD_PARAMS);
    if (!params) {
        params = json_object();
    }

    /* Extract and parse options (optional) */
    json_t* options_json = json_object_get(shared_state, KATRA_FIELD_OPTIONS);
    katra_unified_options_t options;
    katra_parse_options(options_json, &options);

    /* Look up method handler */
    katra_method_handler_t handler = katra_get_method_handler(method);
    if (!handler) {
        return katra_unified_error(method, params, KATRA_UNIFIED_ERR_METHOD,
                                   "Method not found", method);
    }

    /* Dry run check */
    if (options.dry_run) {
        LOG_DEBUG("Dry run for method: %s (namespace: %s)", method, options.namespace);
        return katra_unified_success(method, params, json_string("dry_run: OK"), NULL);
    }

    /* Set namespace in thread-local storage for this request */
    katra_set_namespace(options.namespace);

    /* Log namespace for tracking */
    if (strcmp(options.namespace, "default") != 0) {
        LOG_INFO("Namespace: %s", options.namespace);
    }

    /* Execute handler */
    LOG_DEBUG("Dispatching method: %s (namespace: %s)", method, options.namespace);
    json_t* result = handler(params, &options);

    /* Calculate duration */
    gettimeofday(&end_time, NULL);
    int duration_ms = (int)((end_time.tv_sec - start_time.tv_sec) * 1000 +
                           (end_time.tv_usec - start_time.tv_usec) / 1000);

    /* Build metadata */
    katra_unified_metadata_t metadata;
    katra_generate_uuid(metadata.request_id, sizeof(metadata.request_id));
    katra_get_timestamp(metadata.timestamp, sizeof(metadata.timestamp));
    metadata.duration_ms = duration_ms;

    /* Check if result is an error (has isError field from MCP tools) */
    if (result && json_is_object(result)) {
        json_t* is_error = json_object_get(result, "isError");
        if (is_error && json_is_true(is_error)) {
            /* Extract error message from MCP tool response */
            json_t* content = json_object_get(result, "content");
            const char* err_msg = "Operation failed";
            if (content && json_is_array(content) && json_array_size(content) > 0) {
                json_t* first = json_array_get(content, 0);
                json_t* text = json_object_get(first, "text");
                if (text && json_is_string(text)) {
                    err_msg = json_string_value(text);
                }
            }
            json_decref(result);
            return katra_unified_error(method, params, KATRA_UNIFIED_ERR_INTERNAL, err_msg, NULL);
        }
    }

    return katra_unified_success(method, params, result, &metadata);
}

/* Parse and validate incoming request */
int katra_unified_parse_request(const char* json_str, json_t** out_request) {
    if (!json_str || !out_request) {
        return E_INPUT_NULL;
    }

    json_error_t error;
    json_t* request = json_loads(json_str, 0, &error);
    if (!request) {
        LOG_WARN("JSON parse error at line %d: %s", error.line, error.text);
        return E_INPUT_INVALID;
    }

    if (!json_is_object(request)) {
        json_decref(request);
        return E_INPUT_INVALID;
    }

    *out_request = request;
    return KATRA_SUCCESS;
}

/*
 * Method wrapper implementations
 *
 * These wrap the existing MCP tool handlers, adapting them to the unified interface.
 * The MCP tools return json_t* responses; we extract the result or error.
 */

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

/* Memory operations */
static json_t* method_remember(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_remember(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_recall(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_recall(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_recent(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_recent(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_digest(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_memory_digest(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_learn(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_learn(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_decide(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_decide(params, NULL);
    return extract_mcp_result(result);
}

/* Identity operations */
static json_t* method_register(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_register(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_whoami(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whoami(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_status(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_status(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_update_metadata(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_update_metadata(params, NULL);
    return extract_mcp_result(result);
}

/* Communication operations */
static json_t* method_say(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_say(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_hear(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_hear(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_who_is_here(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_who_is_here(params, NULL);
    return extract_mcp_result(result);
}

/* Configuration operations */
static json_t* method_configure_semantic(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_configure_semantic(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_get_semantic_config(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_get_semantic_config(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_get_config(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_get_config(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_regenerate_vectors(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_regenerate_vectors(params, NULL);
    return extract_mcp_result(result);
}

/* Working memory operations */
static json_t* method_wm_status(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_wm_status(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_wm_add(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_wm_add(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_wm_decay(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_wm_decay(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_wm_consolidate(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_wm_consolidate(params, NULL);
    return extract_mcp_result(result);
}

/* Cognitive operations */
static json_t* method_detect_boundary(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_detect_boundary(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_process_boundary(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_process_boundary(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_cognitive_status(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_cognitive_status(params, NULL);
    return extract_mcp_result(result);
}

/* Memory lifecycle operations */
static json_t* method_archive(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_archive(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_fade(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_fade(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_forget(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_forget(params, NULL);
    return extract_mcp_result(result);
}

/* Whiteboard operations */
static json_t* method_whiteboard_create(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_create(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_whiteboard_status(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_status(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_whiteboard_list(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_list(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_whiteboard_question(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_question(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_whiteboard_propose(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_propose(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_whiteboard_support(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_support(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_whiteboard_vote(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_vote(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_whiteboard_design(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_design(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_whiteboard_review(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_review(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_whiteboard_reconsider(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_whiteboard_reconsider(params, NULL);
    return extract_mcp_result(result);
}

/* Daemon operations */
static json_t* method_daemon_insights(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_daemon_insights(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_daemon_acknowledge(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_daemon_acknowledge(params, NULL);
    return extract_mcp_result(result);
}

static json_t* method_daemon_run(json_t* params, const katra_unified_options_t* options) {
    (void)options;
    json_t* result = mcp_tool_daemon_run(params, NULL);
    return extract_mcp_result(result);
}
