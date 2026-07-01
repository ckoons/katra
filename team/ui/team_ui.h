/* © 2025 Casey Koons All rights reserved */

#ifndef TEAM_UI_H
#define TEAM_UI_H

/*
 * team_ui.h - Team Coordinator UI (MVP)
 *
 * A terminal coordinator that fans Casey's input to the running CI team
 * via the katra meeting room and collects their replies. It is a thin
 * MCP/TCP client to the katra daemon (default 127.0.0.1:3141): each
 * operation opens a short-lived connection, sends one newline-framed
 * JSON-RPC tools/call, and reads one newline-framed response.
 *
 * Identity note: this console is Casey managing the team DIRECTLY, so it
 * speaks and hears AS "Casey" by default. Aria (the chief-of-staff CI) is a
 * separate woken persona with her own queue; the console must NOT default to
 * her name, or the two would drain the same inbox. Because katra_hear drains
 * the caller's queue, the default is Casey's own identity so direct messages
 * read "From Casey" and never consume a teammate's (or Aria's) inbox. Pass
 * --katra <name> only when you intend to speak and hear AS that CI.
 */

#include "katra_mcp.h"   /* MCP_* field/method/tool macros; pulls in jansson */

/* ============================================================================
 * COORDINATOR DEFAULTS
 * ============================================================================ */

#define TEAM_UI_DEFAULT_HOST     "127.0.0.1"
#define TEAM_UI_DEFAULT_PERSONA  "Casey"
#define TEAM_UI_DEFAULT_ROLE     "principal"
#define TEAM_UI_RPC_ID           1
#define TEAM_UI_INPUT_MAX        8192

/* ============================================================================
 * MCP ARGUMENT KEYS (not covered by MCP_PARAM_* in katra_mcp.h)
 * ============================================================================ */

#define TEAM_UI_PARAM_CI_NAME    "ci_name"
#define TEAM_UI_PARAM_RECIPIENTS "recipients"
#define TEAM_UI_PARAM_MAX_COUNT  "max_count"

/* ============================================================================
 * REPL COMMAND TOKENS
 * ============================================================================ */

#define TEAM_UI_CMD_PREFIX   '/'
#define TEAM_UI_CMD_QUIT     "/quit"
#define TEAM_UI_CMD_EXIT     "/exit"
#define TEAM_UI_CMD_HELP     "/help"
#define TEAM_UI_CMD_WHO      "/who"
#define TEAM_UI_CMD_HEAR     "/hear"
#define TEAM_UI_CMD_STATUS   "/status"
#define TEAM_UI_CMD_TO       "/to"

/* ============================================================================
 * COMMAND-LINE FLAGS
 * ============================================================================ */

#define TEAM_UI_FLAG_KATRA   "--katra"
#define TEAM_UI_FLAG_HOST    "--host"
#define TEAM_UI_FLAG_PORT    "--port"
#define TEAM_UI_FLAG_ROLE    "--role"
#define TEAM_UI_FLAG_HELP    "--help"

/* ============================================================================
 * MCP CLIENT API (team_ui_mcp.c)
 * ============================================================================ */

/**
 * team_ui_mcp_call() - Make one MCP tools/call to the katra daemon.
 *
 * Opens a short-lived TCP connection, sends a newline-framed JSON-RPC
 * request, reads the newline-framed response, and extracts the human
 * readable text (result.content[0].text) or the protocol error message.
 *
 * Parameters:
 *   host         - Daemon host (e.g. "127.0.0.1")
 *   port         - Daemon MCP port (e.g. 3141)
 *   tool         - MCP tool name (e.g. MCP_TOOL_SAY)
 *   arguments    - JSON object of tool arguments. The reference is STOLEN
 *                  (consumed) by this call; the caller must NOT decref it.
 *                  May be NULL for tools that take no arguments.
 *   text_out     - Receives allocated response text (caller frees).
 *   is_error_out - Set true if the tool reported isError (may be NULL).
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if host, tool, or text_out is NULL
 *   E_SYSTEM_MEMORY on allocation failure
 *   E_SYSTEM_IO on socket/connect/read/write failure
 *   E_IO_EOF if the daemon returned no data
 *   E_INPUT_FORMAT if the response is not valid JSON
 */
int team_ui_mcp_call(const char* host, int port, const char* tool,
                     json_t* arguments, char** text_out, bool* is_error_out);

#endif /* TEAM_UI_H */
