/* © 2025 Casey Koons All rights reserved */

/*
 * Katra Unified Daemon - Main Entry Point
 *
 * Starts the unified HTTP daemon for Katra operations.
 *
 * Usage:
 *   katra-unified-daemon [--port PORT] [--bind ADDRESS]
 *
 * Environment variables:
 *   KATRA_UNIFIED_PORT - HTTP port (default: 9742)
 *   KATRA_UNIFIED_BIND - Bind address (default: 127.0.0.1)
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

/* Project includes */
#include "katra_unified.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_env_utils.h"
#include "katra_lifecycle.h"
#include "katra_module.h"

/* Version info */
#define DAEMON_VERSION "1.0.0"

/* Usage message */
static void print_usage(const char* program_name) {
    /* GUIDELINE_APPROVED: CLI help output to stderr is standard practice */
    fprintf(stderr,
        "Katra Unified Daemon v%s\n"
        "\n"
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  -p, --port PORT      HTTP port (default: 9742)\n"
        "  -m, --mcp-port PORT  MCP JSON-RPC port for Claude Code (default: 3141)\n"
        "  -M, --no-mcp         Disable MCP listener\n"
        "  -b, --bind ADDRESS   Bind address (default: 127.0.0.1)\n"
        "  -n, --namespace NS   Default namespace (default: default)\n"
        "  -s, --socket PATH    Unix socket path (default: /tmp/katra.sock)\n"
        "  -S, --no-socket      Disable Unix socket\n"
        "  -h, --help           Show this help message\n"
        "  -v, --version        Show version\n"
        "\n"
        "Environment variables:\n"
        "  KATRA_UNIFIED_PORT   HTTP port\n"
        "  KATRA_MCP_PORT       MCP JSON-RPC port (0 to disable)\n"
        "  KATRA_UNIFIED_BIND   Bind address\n"
        "  KATRA_NAMESPACE      Default namespace\n"
        "  KATRA_SOCKET_PATH    Unix socket path (empty to disable)\n"
        "\n"
        "Protocols:\n"
        "  HTTP REST API        POST /operation, GET /health, GET /methods\n"
        "  MCP JSON-RPC         Claude Code protocol (initialize, tools/list, tools/call)\n"
        "  Unix socket          Same as HTTP (fast local path)\n"
        "\n"
        "HTTP Example:\n"
        "  curl -X POST http://localhost:9742/operation \\\n"
        "    -H 'Content-Type: application/json' \\\n"
        "    -d '{\"method\":\"recall\",\"params\":{\"topic\":\"Casey\"}}'\n"
        "\n"
        "MCP Example (Claude Code):\n"
        "  echo '{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\"}' | nc localhost 3141\n"
        "\n",
        DAEMON_VERSION, program_name);
}

/* Parse command line arguments */
static int parse_args(int argc, char* argv[], katra_daemon_config_t* config) {
    static struct option long_options[] = {
        {"port",      required_argument, 0, 'p'},
        {"mcp-port",  required_argument, 0, 'm'},
        {"no-mcp",    no_argument,       0, 'M'},
        {"bind",      required_argument, 0, 'b'},
        {"namespace", required_argument, 0, 'n'},
        {"socket",    required_argument, 0, 's'},
        {"no-socket", no_argument,       0, 'S'},
        {"help",      no_argument,       0, 'h'},
        {"version",   no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "p:m:Mb:n:s:Shv", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                config->http_port = (uint16_t)atoi(optarg);
                if (config->http_port == 0) {
                    /* GUIDELINE_APPROVED: CLI arg error before logging init */
                    fprintf(stderr, "Invalid port: %s\n", optarg);
                    return E_INPUT_INVALID;
                }
                break;

            case 'm':
                config->mcp_port = (uint16_t)atoi(optarg);
                break;

            case 'M':
                config->mcp_port = 0;  /* Disable MCP */
                break;

            case 'b':
                config->bind_address = optarg;
                break;

            case 'n':
                config->default_namespace = optarg;
                break;

            case 's':
                config->socket_path = optarg;
                config->enable_unix_socket = true;
                break;

            case 'S':
                config->enable_unix_socket = false;
                break;

            case 'h':
                print_usage(argv[0]);
                exit(EXIT_CODE_SUCCESS);

            case 'v':
                printf("katra-unified-daemon v%s\n", DAEMON_VERSION);
                exit(EXIT_CODE_SUCCESS);

            default:
                print_usage(argv[0]);
                return E_INPUT_INVALID;
        }
    }

    return KATRA_SUCCESS;
}

/* Load configuration from environment */
static void load_env_config(katra_daemon_config_t* config) {
    /* HTTP Port */
    int port = 0;
    if (katra_getenvint("KATRA_UNIFIED_PORT", &port) == KATRA_SUCCESS) {
        if (port > 0 && port <= MAX_TCP_PORT) {
            config->http_port = (uint16_t)port;
        }
    }

    /* MCP Port (0 disables) */
    int mcp_port = 0;
    if (katra_getenvint("KATRA_MCP_PORT", &mcp_port) == KATRA_SUCCESS) {
        if (mcp_port >= 0 && mcp_port <= MAX_TCP_PORT) {
            config->mcp_port = (uint16_t)mcp_port;
        }
    }

    /* Bind address */
    const char* bind = katra_getenv("KATRA_UNIFIED_BIND");
    if (bind) {
        config->bind_address = bind;
    }

    /* Namespace */
    const char* ns = katra_getenv("KATRA_NAMESPACE");
    if (ns) {
        config->default_namespace = ns;
    }

    /* Unix socket path (empty string disables) */
    const char* socket_path = katra_getenv("KATRA_SOCKET_PATH");
    if (socket_path) {
        if (strlen(socket_path) == 0) {
            config->enable_unix_socket = false;
        } else {
            config->socket_path = socket_path;
            config->enable_unix_socket = true;
        }
    }
}

int main(int argc, char* argv[]) {
    int result = KATRA_SUCCESS;

    /* Initialize logging */
    log_init(NULL);
    log_set_level(LOG_INFO);

    /* Initialize Katra core (for MCP tools to work) */
    result = katra_lifecycle_init();
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: Critical startup error before katra_report_error available */
        fprintf(stderr, "Failed to initialize Katra lifecycle\n");
        return EXIT_CODE_FAILURE;
    }

    /* Initialize module loader (non-fatal if fails) */
    result = katra_module_loader_init();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Module loader init failed (non-fatal): %d", result);
    } else {
        /* Discover available modules */
        int discovered = katra_module_loader_discover();
        if (discovered > 0) {
            LOG_INFO("Discovered %d loadable module(s)", discovered);
        }
    }

    /* Set default configuration */
    katra_daemon_config_t config = {
        .http_port = KATRA_UNIFIED_DEFAULT_PORT,
        .mcp_port = KATRA_UNIFIED_MCP_PORT,  /* MCP enabled by default */
        .bind_address = "127.0.0.1",
        .enable_unix_socket = true,  /* Unix socket enabled by default */
        .socket_path = KATRA_UNIFIED_SOCKET_PATH,
        .max_clients = DEFAULT_MAX_CLIENTS,
        .default_namespace = "default"
    };

    /* Load environment variables */
    load_env_config(&config);

    /* Parse command line arguments (overrides env) */
    result = parse_args(argc, argv, &config);
    if (result != KATRA_SUCCESS) {
        return EXIT_CODE_FAILURE;
    }

    LOG_INFO("Starting Katra Unified Daemon v%s", DAEMON_VERSION);
    LOG_INFO("Configuration: HTTP=%d, MCP=%d, bind=%s, namespace=%s",
             config.http_port, config.mcp_port, config.bind_address, config.default_namespace);
    if (config.enable_unix_socket) {
        LOG_INFO("Unix socket: %s", config.socket_path);
    }
    if (config.mcp_port > 0) {
        LOG_INFO("MCP JSON-RPC: port %d (for Claude Code)", config.mcp_port);
    }

    /* Start HTTP daemon (blocks until shutdown) */
    result = katra_http_daemon_start(&config);

    /* Cleanup */
    katra_module_loader_shutdown();
    katra_lifecycle_cleanup();
    log_cleanup();

    return (result == KATRA_SUCCESS) ? EXIT_CODE_SUCCESS : EXIT_CODE_FAILURE;
}
