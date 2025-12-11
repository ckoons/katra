/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_UNIFIED_H
#define KATRA_UNIFIED_H

#include <stdbool.h>
#include <stdint.h>
#include <jansson.h>

/*
 * Katra Unified Operation Interface
 *
 * Single dispatcher for all Katra operations, replacing 24+ individual MCP tools.
 * Reduces token overhead from ~14,100 to ~800 tokens (94% reduction).
 *
 * Design: The model fills a form (shared_state), the daemon executes atomically.
 *
 * Architecture:
 *   +-------------+     +------------------+     +------------------+
 *   | MCP Tool    | --> | HTTP Daemon      | --> | Method Handlers  |
 *   | (wrapper)   |     | /tmp/katra.sock  |     | (40+ methods)    |
 *   +-------------+     | or port 9742     |     +------------------+
 *                       +------------------+
 *
 * Request Format (shared_state):
 *   {
 *     "version": "1.0",                    // Schema version
 *     "method": "recall",                  // Method name (required)
 *     "params": {"topic": "Casey"},        // Method parameters
 *     "options": {                         // Optional
 *       "namespace": "coder-a",            // Namespace isolation
 *       "timeout_ms": 5000,                // Request timeout
 *       "dry_run": false                   // Validate without executing
 *     }
 *   }
 *
 * Response Format:
 *   {
 *     "version": "1.0",
 *     "method": "recall",
 *     "params": {...},
 *     "result": "...",                     // Success result
 *     "error": null,                       // Or error object
 *     "metadata": {
 *       "request_id": "uuid",
 *       "timestamp": "ISO8601",
 *       "duration_ms": 5,
 *       "namespace": "coder-a"
 *     }
 *   }
 *
 * Namespaces:
 *   - default: Standard namespace (shared across all CIs)
 *   - coder-a, coder-b, coder-c: Isolated namespaces for parallel CIs
 *
 * Endpoints:
 *   POST /operation  - Execute unified operation
 *   GET  /health     - Health check
 *   GET  /methods    - List available methods
 */

/* Schema version for compatibility */
#define KATRA_UNIFIED_SCHEMA_VERSION "1.0"
#define KATRA_UNIFIED_VERSION KATRA_UNIFIED_SCHEMA_VERSION

/* Default HTTP daemon port */
#define KATRA_UNIFIED_DEFAULT_PORT 9742

/* Unix socket path */
#define KATRA_UNIFIED_SOCKET_PATH "/tmp/katra.sock"

/* HTTP timeout settings */
#define KATRA_UNIFIED_TIMEOUT_SECS 30
#define KATRA_UNIFIED_CONNECT_TIMEOUT 5

/* Endpoint URLs (for internal daemon communication) */
#define KATRA_UNIFIED_OPERATION_ENDPOINT "/operation"
#define KATRA_UNIFIED_SOCKET_URL "http://localhost" KATRA_UNIFIED_OPERATION_ENDPOINT
#define KATRA_UNIFIED_HTTP_URL_FMT "http://127.0.0.1:%d" KATRA_UNIFIED_OPERATION_ENDPOINT

/* Maximum request/response size */
#define KATRA_UNIFIED_MAX_REQUEST 65536    /* 64KB */
#define KATRA_UNIFIED_MAX_RESPONSE 131072  /* 128KB */

/* HTTP buffer sizes */
#define KATRA_HTTP_HEADER_SIZE 1024
#define KATRA_HTTP_BODY_SIZE 65536

/* Method name constants - mapped from MCP tool names */
#define KATRA_METHOD_REMEMBER "remember"
#define KATRA_METHOD_RECALL "recall"
#define KATRA_METHOD_RECENT "recent"
#define KATRA_METHOD_MEMORY_DIGEST "digest"
#define KATRA_METHOD_LEARN "learn"
#define KATRA_METHOD_DECIDE "decide"
#define KATRA_METHOD_REGISTER "register"
#define KATRA_METHOD_WHOAMI "whoami"
#define KATRA_METHOD_STATUS "status"
#define KATRA_METHOD_UPDATE_METADATA "update_metadata"
#define KATRA_METHOD_SAY "say"
#define KATRA_METHOD_HEAR "hear"
#define KATRA_METHOD_WHO_IS_HERE "who_is_here"
#define KATRA_METHOD_CONFIGURE_SEMANTIC "configure_semantic"
#define KATRA_METHOD_GET_SEMANTIC_CONFIG "get_semantic_config"
#define KATRA_METHOD_GET_CONFIG "get_config"
#define KATRA_METHOD_REGENERATE_VECTORS "regenerate_vectors"
#define KATRA_METHOD_WM_STATUS "wm_status"
#define KATRA_METHOD_WM_ADD "wm_add"
#define KATRA_METHOD_WM_DECAY "wm_decay"
#define KATRA_METHOD_WM_CONSOLIDATE "wm_consolidate"
#define KATRA_METHOD_DETECT_BOUNDARY "detect_boundary"
#define KATRA_METHOD_PROCESS_BOUNDARY "process_boundary"
#define KATRA_METHOD_COGNITIVE_STATUS "cognitive_status"
#define KATRA_METHOD_ARCHIVE "archive"
#define KATRA_METHOD_FADE "fade"
#define KATRA_METHOD_FORGET "forget"
#define KATRA_METHOD_WB_CREATE "whiteboard_create"
#define KATRA_METHOD_WB_STATUS "whiteboard_status"
#define KATRA_METHOD_WB_LIST "whiteboard_list"
#define KATRA_METHOD_WB_QUESTION "whiteboard_question"
#define KATRA_METHOD_WB_PROPOSE "whiteboard_propose"
#define KATRA_METHOD_WB_SUPPORT "whiteboard_support"
#define KATRA_METHOD_WB_VOTE "whiteboard_vote"
#define KATRA_METHOD_WB_DESIGN "whiteboard_design"
#define KATRA_METHOD_WB_REVIEW "whiteboard_review"
#define KATRA_METHOD_WB_RECONSIDER "whiteboard_reconsider"
#define KATRA_METHOD_DAEMON_INSIGHTS "daemon_insights"
#define KATRA_METHOD_DAEMON_ACKNOWLEDGE "daemon_acknowledge"
#define KATRA_METHOD_DAEMON_RUN "daemon_run"

/* JSON field names for shared state */
#define KATRA_FIELD_VERSION "version"
#define KATRA_FIELD_METHOD "method"
#define KATRA_FIELD_PARAMS "params"
#define KATRA_FIELD_OPTIONS "options"
#define KATRA_FIELD_RESULT "result"
#define KATRA_FIELD_ERROR "error"
#define KATRA_FIELD_METADATA "metadata"
#define KATRA_FIELD_REQUEST_ID "request_id"
#define KATRA_FIELD_TIMESTAMP "timestamp"
#define KATRA_FIELD_DURATION_MS "duration_ms"
#define KATRA_FIELD_CODE "code"
#define KATRA_FIELD_MESSAGE "message"
#define KATRA_FIELD_DETAILS "details"
#define KATRA_FIELD_NAMESPACE "namespace"
#define KATRA_FIELD_TIMEOUT_MS "timeout_ms"
#define KATRA_FIELD_DRY_RUN "dry_run"

/* Error codes for unified interface */
#define KATRA_UNIFIED_ERR_NONE "OK"
#define KATRA_UNIFIED_ERR_PARSE "E_PARSE"
#define KATRA_UNIFIED_ERR_METHOD "E_METHOD_NOT_FOUND"
#define KATRA_UNIFIED_ERR_PARAMS "E_INVALID_PARAMS"
#define KATRA_UNIFIED_ERR_INTERNAL "E_INTERNAL"
#define KATRA_UNIFIED_ERR_TIMEOUT "E_TIMEOUT"
#define KATRA_UNIFIED_ERR_NOT_FOUND "E_NOT_FOUND"
#define KATRA_UNIFIED_ERR_CONSENT "E_CONSENT_DENIED"

/* HTTP status codes */
#define HTTP_OK 200
#define HTTP_BAD_REQUEST 400
#define HTTP_NOT_FOUND 404
#define HTTP_METHOD_NOT_ALLOWED 405
#define HTTP_INTERNAL_ERROR 500

/*
 * Unified operation request options
 */
typedef struct {
    int timeout_ms;           /* Operation timeout (0 = default) */
    bool dry_run;             /* If true, validate but don't execute */
    char namespace[64];       /* Namespace for isolation (default, coder-a, etc.) */
} katra_unified_options_t;

/*
 * Unified operation metadata (response)
 */
typedef struct {
    char request_id[64];      /* UUID for this request */
    char timestamp[32];       /* ISO8601 timestamp */
    int duration_ms;          /* Execution time in milliseconds */
} katra_unified_metadata_t;

/*
 * Daemon configuration
 */
typedef struct {
    uint16_t http_port;       /* HTTP server port */
    const char* bind_address; /* Bind address (127.0.0.1, 0.0.0.0) */
    bool enable_unix_socket;  /* Enable Unix socket? */
    const char* socket_path;  /* Unix socket path */
    int max_clients;          /* Maximum concurrent clients */
    const char* default_namespace; /* Default namespace */
} katra_daemon_config_t;

/* Method handler function type */
typedef json_t* (*katra_method_handler_t)(json_t* params, const katra_unified_options_t* options);

/*
 * Core dispatcher functions
 */

/* Initialize the unified daemon */
int katra_unified_init(const katra_daemon_config_t* config);

/* Shutdown the daemon cleanly */
void katra_unified_shutdown(void);

/* Main dispatcher - takes shared_state JSON, returns modified shared_state */
json_t* katra_unified_dispatch(json_t* shared_state);

/* Parse and validate incoming request */
int katra_unified_parse_request(const char* json_str, json_t** out_request);

/* Build success response */
json_t* katra_unified_success(const char* method, json_t* params, json_t* result,
                               const katra_unified_metadata_t* metadata);

/* Build error response */
json_t* katra_unified_error(const char* method, json_t* params, const char* code,
                             const char* message, const char* details);

/*
 * HTTP daemon functions
 */

/* Start HTTP daemon (blocks until shutdown) */
int katra_http_daemon_start(const katra_daemon_config_t* config);

/* Handle single HTTP request */
int katra_http_handle_request(int client_fd, const char* request_body, size_t body_len);

/* Send HTTP response */
int katra_http_send_response(int client_fd, int status_code, const char* body);

/*
 * Unix socket functions
 */

/* Start Unix socket listener */
int katra_unix_socket_start(const char* socket_path);

/* Handle Unix socket client */
int katra_unix_handle_client(int client_fd);

/*
 * Method registration
 */

/* Register a method handler */
int katra_register_method(const char* method_name, katra_method_handler_t handler);

/* Get handler for method */
katra_method_handler_t katra_get_method_handler(const char* method_name);

/* List all registered methods */
json_t* katra_list_methods(void);

/*
 * Utility functions
 */

/* Generate UUID for request_id */
void katra_generate_uuid(char* buffer, size_t size);

/* Get current ISO8601 timestamp */
void katra_get_timestamp(char* buffer, size_t size);

/* Parse options from JSON */
int katra_parse_options(json_t* options_json, katra_unified_options_t* options);

/*
 * Namespace management
 */

/* Set current namespace (thread-local) */
void katra_set_namespace(const char* ns);

/* Get current namespace (thread-local) */
const char* katra_get_namespace(void);

#endif /* KATRA_UNIFIED_H */
