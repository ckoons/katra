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
#include "katra_error.h"
#include "katra_log.h"

/* Global shutdown flag */
volatile sig_atomic_t g_shutdown_requested = 0;

/* Signal handler */
void mcp_signal_handler(int signum) {
    (void)signum;  /* Signal number not used */
    g_shutdown_requested = 1;

    /* Write to stderr (async-signal-safe) */
    const char* msg = "Shutdown requested\n";
    ssize_t result = write(STDERR_FILENO, msg, strlen(msg));
    (void)result;  /* Suppress unused result warning */
}

/* Generate unique per-instance CI identity */
static int generate_ci_id(char* buffer, size_t size) {
    if (!buffer || size == 0) {
        return E_INPUT_NULL;
    }

    const char* user = getenv("USER");
    if (!user) {
        user = "unknown";
    }

    pid_t pid = getpid();
    time_t now = time(NULL);

    int written = snprintf(buffer, size, "mcp_%s_%d_%ld", user, pid, (long)now);

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
        buffer[strcspn(buffer, "\n")] = '\0';

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
                                                       "Parse error",
                                                       "Invalid JSON-RPC 2.0 request");
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

    /* Generate unique CI identity */
    char ci_id[128];
    int result = generate_ci_id(ci_id, sizeof(ci_id));
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to generate CI identity: %s\n",
                katra_error_message(result));
        return EXIT_FAILURE;
    }

    /* GUIDELINE_APPROVED: startup diagnostic message */
    fprintf(stderr, "Katra MCP Server starting with CI identity: %s\n", ci_id);

    /* Initialize server */
    result = mcp_server_init(ci_id);
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
