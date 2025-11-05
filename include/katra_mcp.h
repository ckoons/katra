/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_MCP_H
#define KATRA_MCP_H

#include <stdbool.h>
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

/* JSON-RPC Field Names */
#define MCP_FIELD_JSONRPC "jsonrpc"
#define MCP_FIELD_ID "id"
#define MCP_FIELD_METHOD "method"
#define MCP_FIELD_PARAMS "params"
#define MCP_FIELD_RESULT "result"
#define MCP_FIELD_ERROR "error"
#define MCP_FIELD_CODE "code"
#define MCP_FIELD_MESSAGE "message"
#define MCP_FIELD_DATA "data"
#define MCP_FIELD_DETAILS "details"
#define MCP_JSONRPC_VERSION "2.0"

/* Tool/Resource Common Fields */
#define MCP_FIELD_NAME "name"
#define MCP_FIELD_DESCRIPTION "description"
#define MCP_FIELD_TYPE "type"
#define MCP_FIELD_TEXT "text"
#define MCP_FIELD_CONTENT "content"
#define MCP_FIELD_CONTENTS "contents"
#define MCP_FIELD_IS_ERROR "isError"

/* Tool Schema Fields */
#define MCP_FIELD_INPUT_SCHEMA "inputSchema"
#define MCP_FIELD_PROPERTIES "properties"
#define MCP_FIELD_REQUIRED "required"
#define MCP_FIELD_TOOLS "tools"
#define MCP_FIELD_ARGUMENTS "arguments"

/* Resource Schema Fields */
#define MCP_FIELD_URI "uri"
#define MCP_FIELD_MIME_TYPE "mimeType"
#define MCP_FIELD_RESOURCES "resources"

/* Capabilities Fields */
#define MCP_FIELD_CAPABILITIES "capabilities"
#define MCP_FIELD_SERVER_INFO "serverInfo"
#define MCP_FIELD_PROTOCOL_VERSION "protocolVersion"
#define MCP_FIELD_VERSION "version"

/* JSON Schema Types */
#define MCP_TYPE_OBJECT "object"
#define MCP_TYPE_STRING "string"
#define MCP_TYPE_TEXT "text"
#define MCP_MIME_TEXT_PLAIN "text/plain"

/* Tool Names */
#define MCP_TOOL_REMEMBER "katra_remember"
#define MCP_TOOL_RECALL "katra_recall"
#define MCP_TOOL_LEARN "katra_learn"
#define MCP_TOOL_DECIDE "katra_decide"
#define MCP_TOOL_PLACEMENT "katra_placement"
#define MCP_TOOL_IMPACT "katra_impact"
#define MCP_TOOL_USER_DOMAIN "katra_user_domain"
#define MCP_TOOL_REVIEW_TURN "katra_review_turn"
#define MCP_TOOL_UPDATE_METADATA "katra_update_metadata"
#define MCP_TOOL_REGISTER "katra_register"
#define MCP_TOOL_WHOAMI "katra_whoami"

/* Tool Descriptions */
#define MCP_DESC_REMEMBER "Store a memory with natural language importance"
#define MCP_DESC_RECALL "Find memories about a topic"
#define MCP_DESC_LEARN "Store new knowledge"
#define MCP_DESC_DECIDE "Store a decision with reasoning"
#define MCP_DESC_PLACEMENT "Ask where code should be placed (architecture guidance)"
#define MCP_DESC_IMPACT "Analyze impact of code changes (dependency analysis)"
#define MCP_DESC_USER_DOMAIN "Understand user domain and feature usage patterns"
#define MCP_DESC_REVIEW_TURN "Get memories created this turn for conscious reflection"
#define MCP_DESC_UPDATE_METADATA "Update memory metadata (personal, collection, archival flags)"
#define MCP_DESC_REGISTER "Register your name and role for this session"
#define MCP_DESC_WHOAMI "Get your identity information for this session"

/* Tool Parameter Names */
#define MCP_PARAM_CONTENT "content"
#define MCP_PARAM_CONTEXT "context"
#define MCP_PARAM_TOPIC "topic"
#define MCP_PARAM_KNOWLEDGE "knowledge"
#define MCP_PARAM_DECISION "decision"
#define MCP_PARAM_REASONING "reasoning"
#define MCP_PARAM_QUERY "query"
#define MCP_PARAM_MEMORY_ID "memory_id"
#define MCP_PARAM_PERSONAL "personal"
#define MCP_PARAM_NOT_TO_ARCHIVE "not_to_archive"
#define MCP_PARAM_COLLECTION "collection"
#define MCP_PARAM_NAME "name"
#define MCP_PARAM_ROLE "role"

/* Tool Parameter Descriptions */
#define MCP_PARAM_DESC_CONTENT "The thought or experience to remember"
#define MCP_PARAM_DESC_CONTEXT "Why this is important (trivial, interesting, significant, critical)"
#define MCP_PARAM_DESC_TOPIC "The topic to search for"
#define MCP_PARAM_DESC_KNOWLEDGE "The knowledge to learn"
#define MCP_PARAM_DESC_DECISION "The decision made"
#define MCP_PARAM_DESC_REASONING "Why this decision was made"
#define MCP_PARAM_DESC_QUERY_PLACEMENT "The placement question (e.g., 'Where should the HTTP client code go?')"
#define MCP_PARAM_DESC_QUERY_IMPACT "The impact question (e.g., 'What breaks if I change this API?')"
#define MCP_PARAM_DESC_QUERY_USER_DOMAIN "The user domain question (e.g., 'Who would use this feature?')"
#define MCP_PARAM_DESC_MEMORY_ID "Memory record ID to update"
#define MCP_PARAM_DESC_PERSONAL "Mark as personal collection memory (true/false, optional)"
#define MCP_PARAM_DESC_NOT_TO_ARCHIVE "Prevent automatic archival (true/false, optional)"
#define MCP_PARAM_DESC_COLLECTION "Collection path like 'People/Casey' or 'Moments/Breakthrough' (optional)"
#define MCP_PARAM_DESC_NAME "Your chosen name for this session (e.g., 'Claude-Dev', 'Nyx', 'Bob')"
#define MCP_PARAM_DESC_ROLE "Your role (e.g., 'developer', 'tester', 'assistant')"

/* Resource URIs */
#define MCP_RESOURCE_URI_WELCOME "katra://welcome"
#define MCP_RESOURCE_URI_WORKING_CONTEXT "katra://context/working"
#define MCP_RESOURCE_URI_CONTEXT_SNAPSHOT "katra://context/snapshot"
#define MCP_RESOURCE_URI_SESSION_INFO "katra://session/info"
#define MCP_RESOURCE_URI_MEMORIES_THIS_TURN "katra://memories/this-turn"
#define MCP_RESOURCE_URI_MEMORIES_THIS_SESSION "katra://memories/this-session"

/* Resource Names */
#define MCP_RESOURCE_NAME_WELCOME "⭐ Getting Started with Katra"
#define MCP_RESOURCE_NAME_WORKING_CONTEXT "Working Context"
#define MCP_RESOURCE_NAME_CONTEXT_SNAPSHOT "Context Snapshot"
#define MCP_RESOURCE_NAME_SESSION_INFO "Session Information"
#define MCP_RESOURCE_NAME_MEMORIES_THIS_TURN "Memories From This Turn"
#define MCP_RESOURCE_NAME_MEMORIES_THIS_SESSION "Memories From This Session"

/* Resource Descriptions */
#define MCP_RESOURCE_DESC_WELCOME "New to Katra? Start here! Complete getting-started guide"
#define MCP_RESOURCE_DESC_WORKING_CONTEXT "Yesterday's summary and recent significant memories"
#define MCP_RESOURCE_DESC_CONTEXT_SNAPSHOT "Cognitive state snapshot for session continuity (focus, questions, accomplishments)"
#define MCP_RESOURCE_DESC_SESSION_INFO "Current session state and statistics"
#define MCP_RESOURCE_DESC_MEMORIES_THIS_TURN "All memories created during the current turn (for reflection)"
#define MCP_RESOURCE_DESC_MEMORIES_THIS_SESSION "All memories created during the current session"

/* Error Messages */
#define MCP_ERR_MISSING_PARAMS "Missing params"
#define MCP_ERR_MISSING_TOOL_NAME "Missing tool name"
#define MCP_ERR_UNKNOWN_TOOL "Unknown tool"
#define MCP_ERR_MISSING_URI "Missing URI"
#define MCP_ERR_UNKNOWN_RESOURCE "Unknown resource URI"
#define MCP_ERR_NULL_REQUEST "Null request"
#define MCP_ERR_INVALID_JSONRPC "Invalid JSON-RPC version"
#define MCP_ERR_MISSING_METHOD "Missing method"
#define MCP_ERR_METHOD_NOT_FOUND "Method not found"
#define MCP_ERR_PARSE_ERROR "Parse error"
#define MCP_ERR_INVALID_REQUEST "Invalid JSON-RPC 2.0 request"
#define MCP_ERR_MISSING_ARGS "Missing required arguments"
#define MCP_ERR_MISSING_ARG_QUERY "Missing required argument"
#define MCP_ERR_QUERY_REQUIRED "'query' is required"
#define MCP_ERR_INTERNAL "Internal error"
#define MCP_ERR_MUTEX_LOCK "Failed to acquire mutex lock"
#define MCP_ERR_CREATE_QUERY "Failed to create composition query"
#define MCP_ERR_CREATE_QUERY_DETAILS "Memory allocation failed or invalid query parameters"
#define MCP_ERR_COMPOSE_FAILED "Composition query failed"
#define MCP_ERR_BOTH_REQUIRED "Both 'content' and 'context' are required"
#define MCP_ERR_TOPIC_REQUIRED "'topic' is required"
#define MCP_ERR_KNOWLEDGE_REQUIRED "'knowledge' is required"
#define MCP_ERR_DECISION_REASONING_REQUIRED "Both 'decision' and 'reasoning' are required"
#define MCP_ERR_STORE_MEMORY_FAILED "Failed to store memory"
#define MCP_ERR_STORE_KNOWLEDGE_FAILED "Failed to store knowledge"
#define MCP_ERR_STORE_DECISION_FAILED "Failed to store decision"
#define MCP_ERR_GET_CONTEXT_FAILED "Failed to get working context"
#define MCP_ERR_CONTEXT_DETAILS "Memory allocation failed or session not active"
#define MCP_ERR_GET_SESSION_FAILED "Failed to get session info"

/* Success Messages */
#define MCP_MSG_MEMORY_STORED "Memory stored successfully"
#define MCP_MSG_KNOWLEDGE_STORED "Knowledge stored successfully"
#define MCP_MSG_DECISION_STORED "Decision stored successfully"
#define MCP_MSG_NO_MEMORIES "No memories found for topic"

/* Format Strings (GUIDELINE_APPROVED: format strings for dynamic content) */
#define MCP_FMT_ERROR_WITH_DETAILS "Error: %s\nDetails: %s"
#define MCP_FMT_ERROR_SIMPLE "Error: %s"
#define MCP_FMT_NO_RECOMMENDATION "No %s recommendation available for this query. Try providing more context or reformulating the question."
#define MCP_FMT_WITH_CONFIDENCE "%s\n\nConfidence: %.1f%%"
#define MCP_FMT_FOUND_MEMORIES "Found %zu memories:\n"
#define MCP_FMT_FOUND_MEMORIES_TRUNCATED "Found %zu memories (showing first %d):\n"
#define MCP_FMT_MEMORY_ITEM "\n%zu. %s"
#define MCP_FMT_TRUNCATED "\n... (truncated for display)"
#define MCP_FMT_KATRA_ERROR "%s. %s"

/* Method Names */
#define MCP_METHOD_INITIALIZE "initialize"
#define MCP_METHOD_TOOLS_LIST "tools/list"
#define MCP_METHOD_TOOLS_CALL "tools/call"
#define MCP_METHOD_RESOURCES_LIST "resources/list"
#define MCP_METHOD_RESOURCES_READ "resources/read"

/* Shutdown Message (GUIDELINE_APPROVED: signal handler output) */
#define MCP_MSG_SHUTDOWN "Shutdown requested\n"

/* CI ID Components (GUIDELINE_APPROVED: CI identity generation) */
#define MCP_CI_ID_PREFIX "mcp_"
#define MCP_CI_ID_FMT "%s%s_%d_%ld"
#define MCP_CI_ID_UNKNOWN_USER "unknown"
#define MCP_ENV_USER "USER"

/* Newline Character (GUIDELINE_APPROVED: input processing) */
#define MCP_CHAR_NEWLINE "\n"

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

/* Schema Builders */
json_t* mcp_build_tool_schema_0params(void);
json_t* mcp_build_tool_schema_1param(const char* param_name, const char* param_desc);
json_t* mcp_build_tool_schema_2params(const char* param1_name, const char* param1_desc,
                                       const char* param2_name, const char* param2_desc);
json_t* mcp_build_tool(const char* name, const char* description, json_t* schema);
json_t* mcp_build_resource(const char* uri, const char* name,
                           const char* description, const char* mime_type);

/* Tool Implementations */
json_t* mcp_tool_remember(json_t* args, json_t* id);
json_t* mcp_tool_recall(json_t* args, json_t* id);
json_t* mcp_tool_learn(json_t* args, json_t* id);
json_t* mcp_tool_decide(json_t* args, json_t* id);

/* Nous Tool Implementations */
json_t* mcp_tool_placement(json_t* args, json_t* id);
json_t* mcp_tool_impact(json_t* args, json_t* id);
json_t* mcp_tool_user_domain(json_t* args, json_t* id);

/* Persona Tool Implementations */
json_t* mcp_tool_my_name_is(json_t* args, json_t* id);
json_t* mcp_tool_list_personas(json_t* args, json_t* id);

/* Reflection Tool Implementations */
json_t* mcp_tool_review_turn(json_t* args, json_t* id);
json_t* mcp_tool_update_metadata(json_t* args, json_t* id);

/* Resource Implementations */
json_t* mcp_resource_welcome(json_t* id);
json_t* mcp_resource_working_context(json_t* id);
json_t* mcp_resource_context_snapshot(json_t* id);
json_t* mcp_resource_session_info(json_t* id);
json_t* mcp_resource_memories_this_turn(json_t* id);
json_t* mcp_resource_memories_this_session(json_t* id);

/* Server Lifecycle */
int mcp_server_init(const char* ci_id);
void mcp_server_cleanup(void);
void mcp_main_loop(void);

/* Signal Handling */
void mcp_signal_handler(int signum);

/* Session State Management */
typedef struct {
    char chosen_name[128];      /* Registered name for this session */
    char role[64];              /* CI role (developer, tester, assistant) */
    bool registered;            /* Has CI registered this session? */
    bool first_call;            /* Is this the first tool/resource call? */
    time_t connected_at;        /* Connection timestamp */
} mcp_session_t;

/* Session state access */
mcp_session_t* mcp_get_session(void);
const char* mcp_get_session_name(void);
bool mcp_is_registered(void);
bool mcp_is_first_call(void);
void mcp_mark_first_call_complete(void);

/* Session Tools */
json_t* mcp_tool_register(json_t* args, json_t* id);
json_t* mcp_tool_whoami(json_t* args, json_t* id);

/* Global Mutex for Katra API Access */
extern pthread_mutex_t g_katra_api_lock;

/* Shutdown Flag */
extern volatile sig_atomic_t g_shutdown_requested;

#endif /* KATRA_MCP_H */
