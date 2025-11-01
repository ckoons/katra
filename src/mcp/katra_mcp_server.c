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
#include "katra_init.h"
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_identity.h"
#include "katra_error.h"
#include "katra_log.h"

/* Global persona name (set during initialization) */
char g_persona_name[256] = "";
char g_ci_id[256] = "";

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

/* Generate unique per-instance CI identity */
static int generate_ci_id(char* buffer, size_t size) {
    if (!buffer || size == 0) {
        return E_INPUT_NULL;
    }

    const char* user = getenv(MCP_ENV_USER);
    if (!user) {
        user = MCP_CI_ID_UNKNOWN_USER;
    }

    pid_t pid = getpid();
    time_t now = time(NULL);

    int written = snprintf(buffer, size, MCP_CI_ID_FMT, MCP_CI_ID_PREFIX, user, pid, (long)now);

    if (written < 0 || (size_t)written >= size) {
        return E_BUFFER_OVERFLOW;
    }

    return KATRA_SUCCESS;
}

/* Initialize MCP server */
int mcp_server_init(const char* ci_id) {
    if (!ci_id) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Error: ci_id is NULL\n");
        return E_INPUT_NULL;
    }

    /* Step 1: Initialize Katra */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to initialize Katra: %s\n",
                katra_error_message(result));
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

    /* Step 3: Initialize breathing layer */
    result = breathe_init(ci_id);
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to initialize breathing layer: %s\n",
                katra_error_message(result));
        katra_memory_cleanup();
        katra_exit();
        return result;
    }

    /* Step 4: Start session */
    result = session_start(ci_id);
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to start session: %s\n",
                katra_error_message(result));
        breathe_cleanup();
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
    session_end();
    breathe_cleanup();
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
    const char* env_name = getenv("KATRA_NAME");

    if (env_name && strlen(env_name) > 0) {
        /* Priority 1: KATRA_NAME environment variable */
        strncpy(g_persona_name, env_name, sizeof(g_persona_name) - 1);

        /* Look up in persona registry */
        result = katra_lookup_persona(env_name, g_ci_id, sizeof(g_ci_id));

        if (result == KATRA_SUCCESS) {
            /* Found existing persona */
            /* GUIDELINE_APPROVED: startup diagnostic message */
            fprintf(stderr, "Katra MCP Server resuming persona '%s' with CI identity: %s\n",
                    g_persona_name, g_ci_id);

            /* Update session count */
            katra_update_persona_session(g_persona_name);
        } else {
            /* Not found - create new persona with this name */
            result = generate_ci_id(g_ci_id, sizeof(g_ci_id));
            if (result != KATRA_SUCCESS) {
                /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
                fprintf(stderr, "Failed to generate CI identity: %s\n",
                        katra_error_message(result));
                return EXIT_FAILURE;
            }

            /* Register as new persona */
            katra_register_persona(g_persona_name, g_ci_id);

            /* GUIDELINE_APPROVED: startup diagnostic message */
            fprintf(stderr, "Katra MCP Server created new persona '%s' with CI identity: %s\n",
                    g_persona_name, g_ci_id);
        }
    }
    else {
        /* Priority 2: last_active from persona registry */
        char last_active_name[256];
        result = katra_get_last_active(last_active_name, sizeof(last_active_name),
                                       g_ci_id, sizeof(g_ci_id));

        if (result == KATRA_SUCCESS) {
            /* Resume last active persona */
            strncpy(g_persona_name, last_active_name, sizeof(g_persona_name) - 1);

            /* GUIDELINE_APPROVED: startup diagnostic message */
            fprintf(stderr, "Katra MCP Server resuming last active persona '%s' with CI identity: %s\n",
                    g_persona_name, g_ci_id);

            /* Update session count */
            katra_update_persona_session(g_persona_name);
        }
        else {
            /* Priority 3: Generate anonymous persona */
            result = generate_ci_id(g_ci_id, sizeof(g_ci_id));
            if (result != KATRA_SUCCESS) {
                /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
                fprintf(stderr, "Failed to generate CI identity: %s\n",
                        katra_error_message(result));
                return EXIT_FAILURE;
            }

            /* Create anonymous persona name */
            time_t now = time(NULL);
            snprintf(g_persona_name, sizeof(g_persona_name), "anonymous_%ld", (long)now);

            /* Register anonymous persona */
            katra_register_persona(g_persona_name, g_ci_id);

            /* GUIDELINE_APPROVED: startup diagnostic message */
            fprintf(stderr, "Katra MCP Server created anonymous persona '%s' with CI identity: %s\n",
                    g_persona_name, g_ci_id);
        }
    }

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
