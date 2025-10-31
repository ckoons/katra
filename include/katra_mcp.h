/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_MCP_H
#define KATRA_MCP_H

#include <jansson.h>
#include <pthread.h>

/* MCP Protocol Constants */
#define MCP_PROTOCOL_VERSION "2024-11-05"
#define MCP_SERVER_NAME "katra-mcp"
#define MCP_SERVER_VERSION "1.0.0"

/* Buffer sizes */
#define MCP_MAX_LINE 32768        /* 32KB per JSON-RPC message */
#define MCP_ERROR_BUFFER 512      /* Error message buffer */
#define MCP_RESPONSE_BUFFER 4096  /* Response text buffer */

/* JSON-RPC Error Codes */
#define MCP_ERROR_PARSE -32700        /* Parse error */
#define MCP_ERROR_INVALID_REQUEST -32600  /* Invalid Request */
#define MCP_ERROR_METHOD_NOT_FOUND -32601 /* Method not found */
#define MCP_ERROR_INVALID_PARAMS -32602   /* Invalid params */
#define MCP_ERROR_INTERNAL -32603         /* Internal error */
#define MCP_ERROR_SERVER -32000           /* Server error */

/* Result limits */
#define MCP_MAX_RECALL_RESULTS 100  /* Max results from recall_about() */

/* Protocol Functions */
json_t* mcp_parse_request(const char* json_str);
json_t* mcp_dispatch_request(json_t* request);
int mcp_send_response(json_t* response);

/* Response Builders */
json_t* mcp_success_response(json_t* id, json_t* result);
json_t* mcp_error_response(json_t* id, int code, const char* message, const char* details);

/* Tool Response Builders */
json_t* mcp_tool_success(const char* text);
json_t* mcp_tool_success_with_data(const char* text, json_t* data);
json_t* mcp_tool_error(const char* message, const char* details);

/* Tool Implementations */
json_t* mcp_tool_remember(json_t* args, json_t* id);
json_t* mcp_tool_recall(json_t* args, json_t* id);
json_t* mcp_tool_learn(json_t* args, json_t* id);
json_t* mcp_tool_decide(json_t* args, json_t* id);

/* Phase 5 Tool Implementations */
json_t* mcp_tool_placement(json_t* args, json_t* id);
json_t* mcp_tool_impact(json_t* args, json_t* id);
json_t* mcp_tool_user_domain(json_t* args, json_t* id);

/* Resource Implementations */
json_t* mcp_resource_working_context(json_t* id);
json_t* mcp_resource_session_info(json_t* id);

/* Server Lifecycle */
int mcp_server_init(const char* ci_id);
void mcp_server_cleanup(void);
void mcp_main_loop(void);

/* Signal Handling */
void mcp_signal_handler(int signum);

/* Global Mutex for Katra API Access */
extern pthread_mutex_t g_katra_api_lock;

/* Shutdown Flag */
extern volatile sig_atomic_t g_shutdown_requested;

#endif /* KATRA_MCP_H */
