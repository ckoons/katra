/* © 2025 Casey Koons All rights reserved */

#ifndef MCP_TOOLS_COMMON_H
#define MCP_TOOLS_COMMON_H

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "katra_vector.h"
#include "katra_mcp.h"

/* External globals from katra_mcp_server.c */
extern char g_persona_name[256];
extern char g_ci_id[256];
extern vector_store_t* g_vector_store;

/* Global mutex for Katra API access (defined in mcp_tools_memory.c) */
extern pthread_mutex_t g_katra_api_lock;

/*
 * Strict Identity Mode
 *
 * When KATRA_STRICT_IDENTITY=1, operations fail if ci_name is not
 * explicitly provided. This prevents silent misattribution of
 * messages and memories in multi-CI environments.
 *
 * Enable via environment variable: KATRA_STRICT_IDENTITY=1
 */
static inline bool mcp_strict_identity_mode(void) {
    const char* strict = getenv("KATRA_STRICT_IDENTITY");
    return (strict && (strcmp(strict, "1") == 0 || strcmp(strict, "true") == 0));
}

/*
 * Helper: Get CI name from args or session
 * Priority: args.ci_name > session_name (unless strict mode)
 *
 * In strict mode (KATRA_STRICT_IDENTITY=1):
 *   - Returns NULL if ci_name not in args
 *   - Caller MUST check for NULL and return appropriate error
 *
 * In normal mode:
 *   - Falls back to session_name if ci_name not provided
 *   - May cause identity issues in multi-CI environments
 */
static inline const char* mcp_get_ci_name_from_args(json_t* args) {
    if (args) {
        const char* ci_name = json_string_value(json_object_get(args, "ci_name"));
        if (ci_name && strlen(ci_name) > 0) {
            return ci_name;
        }
    }

    /* Strict mode: require explicit ci_name */
    if (mcp_strict_identity_mode()) {
        return NULL;
    }

    /* Normal mode: fall back to session name */
    return mcp_get_session_name();
}

#endif /* MCP_TOOLS_COMMON_H */
