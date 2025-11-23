/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_MCP_TCP_H
#define KATRA_MCP_TCP_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "katra_mcp.h"

/* Default TCP port for MCP server */
#define KATRA_MCP_DEFAULT_PORT 3141

/* Maximum concurrent client connections */
#define KATRA_MCP_MAX_CLIENTS 32

/* TCP server configuration */
typedef struct {
    uint16_t port;
    const char* bind_address;  /* "localhost" or "127.0.0.1" */
    int max_clients;
    bool enable_health_check;
} mcp_tcp_config_t;

/* Client connection state */
typedef struct {
    int socket_fd;
    char persona_name[64];
    char ci_id[64];
    bool registered;
    time_t connected_at;
    mcp_session_t session;  /* Per-client session state */
} mcp_tcp_client_t;

/* Initialize TCP server configuration from file */
int mcp_tcp_load_config(mcp_tcp_config_t* config, const char* config_file);

/* Start TCP server (multi-tenant, blocks until shutdown) */
int mcp_tcp_server_start(const mcp_tcp_config_t* config);

/* Handle a single TCP client connection */
void mcp_tcp_handle_client(mcp_tcp_client_t* client);

/* Health check endpoint handler */
int mcp_tcp_handle_health_check(int client_fd);

#endif /* KATRA_MCP_TCP_H */
