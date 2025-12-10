/* © 2025 Casey Koons All rights reserved */

/*
 * MCP Global State
 *
 * Shared state used by MCP tools. Separated from server main to allow
 * reuse in unified daemon without duplicating the MCP server binary.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "katra_mcp.h"
#include "katra_limits.h"
#include "katra_vector.h"

/* Global persona name (GUIDELINE_APPROVED: global state) */
/* NOTE: g_ci_id IS the persona name (not UUID) - enables per-persona isolation */
char g_persona_name[KATRA_CI_ID_SIZE] = "";
char g_ci_id[KATRA_CI_ID_SIZE] = "";  /* Same as g_persona_name */

/* Global vector store for semantic search (Phase 6.1) */
vector_store_t* g_vector_store = NULL;

/* Global session state (stdio mode) */
static mcp_session_t g_session = {
    .chosen_name = "Katra",  /* Default name until registered */
    .role = "",
    .registered = false,
    .first_call = true,
    .connected_at = 0
};

/* Thread-local session for TCP mode (per-client sessions) */
static __thread mcp_session_t* g_current_session = NULL;

/* Global shutdown flag */
volatile sig_atomic_t g_shutdown_requested = 0;

/* Global Katra API lock */
pthread_mutex_t g_katra_api_lock = PTHREAD_MUTEX_INITIALIZER;

/* Session State Access Functions */
mcp_session_t* mcp_get_session(void) {
    /* TCP mode: return thread-local session if set */
    if (g_current_session) {
        return g_current_session;
    }
    /* stdio mode: return global session */
    return &g_session;
}

const char* mcp_get_session_name(void) {
    mcp_session_t* session = mcp_get_session();
    return session->chosen_name;
}

bool mcp_is_registered(void) {
    mcp_session_t* session = mcp_get_session();
    return session->registered;
}

bool mcp_is_first_call(void) {
    mcp_session_t* session = mcp_get_session();
    return session->first_call;
}

void mcp_mark_first_call_complete(void) {
    mcp_session_t* session = mcp_get_session();
    session->first_call = false;
}

/* TCP mode: set current client session for this thread */
void mcp_set_current_session(mcp_session_t* session) {
    g_current_session = session;
}

void mcp_clear_current_session(void) {
    g_current_session = NULL;
}

/* Initialize global state */
void mcp_globals_init(void) {
    g_persona_name[0] = '\0';
    g_ci_id[0] = '\0';
    g_vector_store = NULL;
    g_shutdown_requested = 0;

    /* Reset session */
    strncpy(g_session.chosen_name, "Katra", sizeof(g_session.chosen_name) - 1);
    g_session.chosen_name[sizeof(g_session.chosen_name) - 1] = '\0';
    g_session.role[0] = '\0';
    g_session.registered = false;
    g_session.first_call = true;
    g_session.connected_at = time(NULL);
}
