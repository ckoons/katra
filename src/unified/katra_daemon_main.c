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

/* Version info */
#define DAEMON_VERSION "1.0.0"

/* Usage message */
static void print_usage(const char* program_name) {
    fprintf(stderr,
        "Katra Unified Daemon v%s\n"
        "\n"
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  -p, --port PORT      HTTP port (default: 9742)\n"
        "  -b, --bind ADDRESS   Bind address (default: 127.0.0.1)\n"
        "  -n, --namespace NS   Default namespace (default: default)\n"
        "  -s, --socket PATH    Unix socket path (default: /tmp/katra.sock)\n"
        "  -S, --no-socket      Disable Unix socket\n"
        "  -h, --help           Show this help message\n"
        "  -v, --version        Show version\n"
        "\n"
        "Environment variables:\n"
        "  KATRA_UNIFIED_PORT   HTTP port\n"
        "  KATRA_UNIFIED_BIND   Bind address\n"
        "  KATRA_NAMESPACE      Default namespace\n"
        "  KATRA_SOCKET_PATH    Unix socket path (empty to disable)\n"
        "\n"
        "Endpoints:\n"
        "  POST /operation      Execute unified operation\n"
        "  GET  /health         Health check\n"
        "  GET  /methods        List available methods\n"
        "\n"
        "Example:\n"
        "  curl -X POST http://localhost:9742/operation \\\n"
        "    -H 'Content-Type: application/json' \\\n"
        "    -d '{\"method\":\"recall\",\"params\":{\"topic\":\"Casey\"}}'\n"
        "\n"
        "Unix socket example:\n"
        "  curl --unix-socket /tmp/katra.sock -X POST http://localhost/operation \\\n"
        "    -H 'Content-Type: application/json' \\\n"
        "    -d '{\"method\":\"status\"}'\n"
        "\n",
        DAEMON_VERSION, program_name);
}

/* Parse command line arguments */
static int parse_args(int argc, char* argv[], katra_daemon_config_t* config) {
    static struct option long_options[] = {
        {"port",      required_argument, 0, 'p'},
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

    while ((opt = getopt_long(argc, argv, "p:b:n:s:Shv", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                config->http_port = (uint16_t)atoi(optarg);
                if (config->http_port == 0) {
                    fprintf(stderr, "Invalid port: %s\n", optarg);
                    return E_INPUT_INVALID;
                }
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
    /* Port */
    int port = 0;
    if (katra_getenvint("KATRA_UNIFIED_PORT", &port) == KATRA_SUCCESS) {
        if (port > 0 && port <= MAX_TCP_PORT) {
            config->http_port = (uint16_t)port;
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
        fprintf(stderr, "Failed to initialize Katra lifecycle\n");
        return EXIT_CODE_FAILURE;
    }

    /* Set default configuration */
    katra_daemon_config_t config = {
        .http_port = KATRA_UNIFIED_DEFAULT_PORT,
        .bind_address = "127.0.0.1",
        .enable_unix_socket = true,  /* Unix socket enabled by default */
        .socket_path = KATRA_UNIFIED_SOCKET_PATH,
        .max_clients = 32,
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
    LOG_INFO("Configuration: port=%d, bind=%s, namespace=%s",
             config.http_port, config.bind_address, config.default_namespace);
    if (config.enable_unix_socket) {
        LOG_INFO("Unix socket: %s", config.socket_path);
    }

    /* Start HTTP daemon (blocks until shutdown) */
    result = katra_http_daemon_start(&config);

    /* Cleanup */
    katra_lifecycle_cleanup();
    log_cleanup();

    return (result == KATRA_SUCCESS) ? EXIT_CODE_SUCCESS : EXIT_CODE_FAILURE;
}
