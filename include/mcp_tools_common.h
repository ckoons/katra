/* © 2025 Casey Koons All rights reserved */

#ifndef MCP_TOOLS_COMMON_H
#define MCP_TOOLS_COMMON_H

#include <pthread.h>
#include "katra_vector.h"

/* External globals from katra_mcp_server.c */
extern char g_persona_name[256];
extern char g_ci_id[256];
extern vector_store_t* g_vector_store;

/* Global mutex for Katra API access (defined in mcp_tools_memory.c) */
extern pthread_mutex_t g_katra_api_lock;

#endif /* MCP_TOOLS_COMMON_H */
