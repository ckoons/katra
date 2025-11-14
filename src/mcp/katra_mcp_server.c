/* © 2025 Casey Koons All rights reserved */

/* Katra MCP Server - Main entry point and lifecycle management */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_limits.h"
#include "katra_init.h"
#include "katra_breathing.h"
#include "katra_lifecycle.h"
#include "katra_hooks.h"
#include "katra_memory.h"
#include "katra_identity.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_meeting.h"
#include "katra_vector.h"

/* Global persona name (set during initialization) */
/* GUIDELINE_APPROVED: global state initialization constants */
/*
 * IMPORTANT: g_ci_id IS the persona name (not a UUID or separate identifier)
 * Throughout Katra core, "ci_id" is legacy terminology - it literally contains
 * the persona's name like "Kari" or "Alice-Tester". This enables:
 *   - Directory isolation: ~/.katra/memory/tier1/{persona_name}/
 *   - Database filtering: WHERE ci_id = 'persona_name'
 *   - File-based separation of memories per persona
 */
char g_persona_name[KATRA_CI_ID_SIZE] = "";
char g_ci_id[KATRA_CI_ID_SIZE] = "";  /* Same as g_persona_name */

/* Global vector store for semantic search (Phase 6.1) */
vector_store_t* g_vector_store = NULL;

/* Global session state */
static mcp_session_t g_session = {
    .chosen_name = "Katra",  /* Default name until registered */
    .role = "",
    .registered = false,
    .first_call = true,
    .connected_at = 0
};

/* Global shutdown flag */
volatile sig_atomic_t g_shutdown_requested = 0;

/* Signal handler */
void mcp_signal_handler(int signum) {
    (void)signum;  /* Signal number not used */
    g_shutdown_requested = 1;

    /* Write to stderr (async-signal-safe) */
    ssize_t result = write(STDERR_FILENO, MCP_MSG_SHUTDOWN, strlen(MCP_MSG_SHUTDOWN));
    (void)result;  /* Suppress unused result warning */
}

/* Session State Access Functions */
mcp_session_t* mcp_get_session(void) {
    return &g_session;
}

const char* mcp_get_session_name(void) {
    return g_session.chosen_name;
}

bool mcp_is_registered(void) {
    return g_session.registered;
}

bool mcp_is_first_call(void) {
    return g_session.first_call;
}

void mcp_mark_first_call_complete(void) {
    g_session.first_call = false;
}

/* Note: generate_ci_id() moved to katra_identity.c as katra_generate_ci_id() */

/* Initialize MCP server */
int mcp_server_init(const char* ci_id) {
    if (!ci_id) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Error: ci_id is NULL\n");
        return E_INPUT_NULL;
    }

    /* Initialize session timestamp */
    g_session.connected_at = time(NULL);

    /* Step 1: Initialize Katra */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to initialize Katra: %s\n",
                katra_error_message(result));
        return result;
    }

    /* Step 1.5: Initialize logging system */
    result = log_init(NULL);  /* Use default log directory */
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize logging: %s\n",
                katra_error_message(result));
        katra_exit();
        return result;
    }

    /* Step 2: Initialize Katra memory */
    result = katra_memory_init(ci_id);
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to initialize Katra memory: %s\n",
                katra_error_message(result));
        katra_exit();
        return result;
    }

    /* Step 3: Initialize lifecycle layer (autonomic breathing) */
    result = katra_lifecycle_init();
    if (result != KATRA_SUCCESS && result != E_ALREADY_INITIALIZED) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to initialize lifecycle layer: %s\n",
                katra_error_message(result));
        katra_memory_cleanup();
        katra_exit();
        return result;
    }

    /* Step 3a: Initialize hook registry */
    result = katra_hooks_init();
    if (result != KATRA_SUCCESS && result != E_ALREADY_INITIALIZED) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to initialize hook registry: %s\n",
                katra_error_message(result));
        katra_lifecycle_cleanup();
        katra_memory_cleanup();
        katra_exit();
        return result;
    }

    /* Step 3b: Register Anthropic MCP adapter */
    const katra_hook_adapter_t* anthropic_adapter = katra_hook_anthropic_adapter();
    result = katra_hooks_register(anthropic_adapter);
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to register Anthropic adapter: %s\n",
                katra_error_message(result));
        katra_hooks_cleanup();
        katra_lifecycle_cleanup();
        katra_memory_cleanup();
        katra_exit();
        return result;
    }

    /* Step 4: Initialize chat/meeting room database */
    result = meeting_room_init();
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic */
        fprintf(stderr, "Failed to initialize meeting room: %s\n",
                katra_error_message(result));
        katra_hooks_cleanup();
        katra_lifecycle_cleanup();
        katra_memory_cleanup();
        katra_exit();
        return result;
    }

    /* Step 4.5: Initialize vector database for semantic search (Phase 6.1) */
    g_vector_store = katra_vector_init(ci_id, false);
    if (!g_vector_store) {
        /* Non-fatal: semantic search will be disabled */
        LOG_WARN("Vector database initialization failed, semantic search disabled");
    } else {
        LOG_INFO("Vector database initialized for semantic search");

        /* Step 4.6: Auto-regenerate vectors if needed (Phase 6.1f) */
        /* Note: Semantic search is enabled by default, so check if vectors need building */
        if (g_vector_store->count < 10) {
            /* Vector count is very low - likely need regeneration */
            LOG_INFO("Auto-regenerating vectors (current count: %zu)", g_vector_store->count);
            fprintf(stderr, "Regenerating semantic search vectors...\n");

            int regen_result = regenerate_vectors();
            if (regen_result > 0) {
                LOG_INFO("Vector regeneration complete: %d vectors created", regen_result);
                fprintf(stderr, "Vector regeneration complete: %d vectors\n", regen_result);
            } else if (regen_result < 0) {
                LOG_WARN("Vector regeneration failed: %d", regen_result);
            }
        }
    }

    /* Step 5: Start session with autonomic breathing */
    result = katra_session_start(ci_id);
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to start session: %s\n",
                katra_error_message(result));
        meeting_room_cleanup();
        katra_hooks_cleanup();
        katra_lifecycle_cleanup();
        katra_memory_cleanup();
        katra_exit();
        return result;
    }

    LOG_INFO("MCP server initialized successfully for CI: %s", ci_id);
    return KATRA_SUCCESS;
}

/* Cleanup MCP server */
void mcp_server_cleanup(void) {
    LOG_INFO("MCP server cleanup started");

    /* Cleanup in reverse order of initialization */
    katra_session_end();   /* Wraps session_end + final breath + breathe_cleanup */

    /* Cleanup vector database */
    if (g_vector_store) {
        katra_vector_cleanup(g_vector_store);
        g_vector_store = NULL;
    }

    meeting_room_cleanup();
    katra_hooks_cleanup();  /* Hook registry cleanup */
    katra_lifecycle_cleanup();  /* Lifecycle layer cleanup */
    katra_memory_cleanup();
    katra_exit();

    LOG_INFO("MCP server cleanup complete");
}

/* Main loop - read JSON-RPC requests from stdin */
void mcp_main_loop(void) {
    char buffer[MCP_MAX_LINE];

    LOG_INFO("MCP server main loop started");

    while (!g_shutdown_requested && fgets(buffer, sizeof(buffer), stdin)) {
        /* Remove trailing newline */
        buffer[strcspn(buffer, MCP_CHAR_NEWLINE)] = '\0';

        /* Skip empty lines */
        if (strlen(buffer) == 0) {
            continue;
        }

        LOG_DEBUG("MCP request received: %.100s...", buffer);

        /* Parse JSON request */
        json_t* request = mcp_parse_request(buffer);

        if (!request) {
            /* Parse error */
            json_t* error_response = mcp_error_response(NULL, MCP_ERROR_PARSE,
                                                       MCP_ERR_PARSE_ERROR,
                                                       MCP_ERR_INVALID_REQUEST);
            mcp_send_response(error_response);
            json_decref(error_response);
            continue;
        }

        /* Dispatch request */
        json_t* response = mcp_dispatch_request(request);
        json_decref(request);

        if (response) {
            mcp_send_response(response);
            json_decref(response);
        }
    }

    if (g_shutdown_requested) {
        LOG_INFO("MCP server main loop exiting (shutdown requested)");
    } else {
        LOG_INFO("MCP server main loop exiting (stdin closed)");
    }
}

/* Main entry point */
int main(void) {
    int exit_code = EXIT_SUCCESS;

    /* Setup signal handlers */
    signal(SIGTERM, mcp_signal_handler);
    signal(SIGINT, mcp_signal_handler);
    signal(SIGPIPE, SIG_IGN);

    /* Initialize persona registry */
    int result = katra_identity_init();
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to initialize persona registry: %s\n",
                katra_error_message(result));
        return EXIT_FAILURE;
    }

    /* Determine CI identity using persona system */
    const char* env_name = getenv("KATRA_PERSONA");

    if (env_name && strlen(env_name) > 0) {
        /* Priority 1: KATRA_PERSONA environment variable */
        strncpy(g_persona_name, env_name, sizeof(g_persona_name) - 1);

        /* ALWAYS use persona name as ci_id (identity preservation fix) */
        strncpy(g_ci_id, g_persona_name, sizeof(g_ci_id) - 1);
        g_ci_id[sizeof(g_ci_id) - 1] = '\0';

        /* Look up in persona registry to check if exists */
        char old_ci_id[KATRA_CI_ID_SIZE];
        result = katra_lookup_persona(env_name, old_ci_id, sizeof(old_ci_id));

        if (result == KATRA_SUCCESS) {
            /* Found existing persona - update to use name-based ci_id */
            if (strcmp(old_ci_id, g_ci_id) != 0) {
                /* Old PID-based ci_id detected - update to name-based */
                /* GUIDELINE_APPROVED: startup diagnostic message */
                fprintf(stderr, "Migrating persona '%s' from old ci_id '%s' to name-based '%s'\n",
                        g_persona_name, old_ci_id, g_ci_id);

                /* Update persona registry with new ci_id */
                katra_register_persona(g_persona_name, g_ci_id);
            } else {
                /* Already using name-based ci_id */
                /* GUIDELINE_APPROVED: startup diagnostic message */
                fprintf(stderr, "Katra MCP Server resuming persona '%s' with CI identity: %s\n",
                        g_persona_name, g_ci_id);
            }

            /* Update session count */
            katra_update_persona_session(g_persona_name);
        } else {
            /* Not found - register new persona */
            result = katra_register_persona(g_persona_name, g_ci_id);
            if (result != KATRA_SUCCESS) {
                /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
                fprintf(stderr, "Failed to register persona: %s\n",
                        katra_error_message(result));
                return EXIT_FAILURE;
            }

            /* GUIDELINE_APPROVED: startup diagnostic message */
            fprintf(stderr, "Katra MCP Server created new persona '%s' with CI identity: %s\n",
                    g_persona_name, g_ci_id);
        }
    }
    else {
        /* Priority 2: last_active from persona registry */
        char last_active_name[KATRA_CI_ID_SIZE];
        result = katra_get_last_active(last_active_name, sizeof(last_active_name),
                                       g_ci_id, sizeof(g_ci_id));

        if (result == KATRA_SUCCESS) {
            /* Resume last active persona */
            strncpy(g_persona_name, last_active_name, sizeof(g_persona_name) - 1);

            /* Store old ci_id from registry */
            char old_ci_id[KATRA_CI_ID_SIZE];
            strncpy(old_ci_id, g_ci_id, sizeof(old_ci_id) - 1);
            old_ci_id[sizeof(old_ci_id) - 1] = '\0';

            /* ALWAYS use persona name as ci_id (identity preservation fix) */
            strncpy(g_ci_id, g_persona_name, sizeof(g_ci_id) - 1);
            g_ci_id[sizeof(g_ci_id) - 1] = '\0';

            /* Check if ci_id needs migration */
            if (strcmp(old_ci_id, g_ci_id) != 0) {
                /* Old PID-based ci_id detected - update to name-based */
                /* GUIDELINE_APPROVED: startup diagnostic message */
                fprintf(stderr, "Migrating persona '%s' from old ci_id '%s' to name-based '%s'\n",
                        g_persona_name, old_ci_id, g_ci_id);

                /* Update persona registry with new ci_id */
                katra_register_persona(g_persona_name, g_ci_id);
            } else {
                /* Already using name-based ci_id */
                /* GUIDELINE_APPROVED: startup diagnostic message */
                fprintf(stderr, "Katra MCP Server resuming last active persona '%s' with CI identity: %s\n",
                        g_persona_name, g_ci_id);
            }

            /* Update session count */
            katra_update_persona_session(g_persona_name);
        }
        else {
            /* Priority 3: Create anonymous persona (use timestamp-based name as ci_id) */
            time_t now = time(NULL);
            snprintf(g_persona_name, sizeof(g_persona_name), "anonymous_%ld", (long)now);

            /* Use persona name as ci_id for consistency */
            strncpy(g_ci_id, g_persona_name, sizeof(g_ci_id) - 1);
            g_ci_id[sizeof(g_ci_id) - 1] = '\0';

            /* Register anonymous persona */
            result = katra_register_persona(g_persona_name, g_ci_id);
            if (result != KATRA_SUCCESS) {
                /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
                fprintf(stderr, "Failed to register anonymous persona: %s\n",
                        katra_error_message(result));
                return EXIT_FAILURE;
            }

            /* GUIDELINE_APPROVED: startup diagnostic message */
            fprintf(stderr, "Katra MCP Server created anonymous persona '%s' with CI identity: %s\n",
                    g_persona_name, g_ci_id);
        }
    }

    /* Update session name from loaded persona */
    strncpy(g_session.chosen_name, g_persona_name, sizeof(g_session.chosen_name) - 1);
    g_session.chosen_name[sizeof(g_session.chosen_name) - 1] = '\0';

    /* Initialize server with determined ci_id */
    result = mcp_server_init(g_ci_id);
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Server initialization failed: %s\n",
                katra_error_message(result));
        return EXIT_FAILURE;
    }

    /* Run main loop */
    mcp_main_loop();

    /* Cleanup */
    mcp_server_cleanup();

    /* GUIDELINE_APPROVED: shutdown diagnostic message */
    fprintf(stderr, "Katra MCP Server stopped\n");
    return exit_code;
}
