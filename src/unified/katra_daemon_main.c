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
#include "katra_mcp.h"
#include "katra_identity.h"
#include "katra_limits.h"
#include "katra_init.h"
#include "katra_memory.h"
#include "katra_hooks.h"
#include "katra_meeting.h"
#include "katra_vector.h"

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

/* Global state (defined in mcp_globals.c) */
extern char g_persona_name[];
extern char g_ci_id[];

/* Resolve persona identity for MCP subsystem */
static int resolve_daemon_persona(void) {
    int result = KATRA_SUCCESS;
    char old_ci_id[KATRA_CI_ID_SIZE];

    /* Priority 1: KATRA_PERSONA environment variable */
    const char* env_name = katra_getenv("KATRA_PERSONA");
    if (env_name && strlen(env_name) > 0) {
        strncpy(g_persona_name, env_name, KATRA_CI_ID_SIZE - 1);
        g_persona_name[KATRA_CI_ID_SIZE - 1] = '\0';
        strncpy(g_ci_id, g_persona_name, KATRA_CI_ID_SIZE - 1);
        g_ci_id[KATRA_CI_ID_SIZE - 1] = '\0';

        result = katra_lookup_persona(env_name, old_ci_id, sizeof(old_ci_id));
        if (result == KATRA_SUCCESS) {
            katra_update_persona_session(g_persona_name);
        } else {
            katra_register_persona(g_persona_name, g_ci_id);
        }

        /* GUIDELINE_APPROVED: startup diagnostic */
        fprintf(stderr, "Katra daemon using persona '%s'\n", g_persona_name);
        return KATRA_SUCCESS;
    }

    /* Priority 2: last_active from persona registry */
    char last_name[KATRA_CI_ID_SIZE];
    char registry_id[KATRA_CI_ID_SIZE];
    result = katra_get_last_active(last_name, sizeof(last_name),
                                   registry_id, sizeof(registry_id));
    if (result == KATRA_SUCCESS) {
        strncpy(g_persona_name, last_name, KATRA_CI_ID_SIZE - 1);
        g_persona_name[KATRA_CI_ID_SIZE - 1] = '\0';
        strncpy(g_ci_id, g_persona_name, KATRA_CI_ID_SIZE - 1);
        g_ci_id[KATRA_CI_ID_SIZE - 1] = '\0';

        katra_update_persona_session(g_persona_name);
        fprintf(stderr, "Katra daemon resuming persona '%s'\n", g_persona_name);
        return KATRA_SUCCESS;
    }

    /* Priority 3: Create anonymous persona */
    time_t now = time(NULL);
    snprintf(g_persona_name, KATRA_CI_ID_SIZE, "daemon_%ld", (long)now);
    strncpy(g_ci_id, g_persona_name, KATRA_CI_ID_SIZE - 1);
    g_ci_id[KATRA_CI_ID_SIZE - 1] = '\0';
    katra_register_persona(g_persona_name, g_ci_id);
    fprintf(stderr, "Katra daemon created anonymous persona '%s'\n", g_persona_name);

    return KATRA_SUCCESS;
}

int main(int argc, char* argv[]) {
    int result = KATRA_SUCCESS;

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

    /* Load .env files (needed for KATRA_PERSONA) */
    result = katra_loadenv();
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic */
        fprintf(stderr, "Warning: Failed to load .env configuration: %s\n",
                katra_error_message(result));
    }

    /* Load environment config variables */
    load_env_config(&config);

    /* Parse command line arguments (overrides env) */
    result = parse_args(argc, argv, &config);
    if (result != KATRA_SUCCESS) {
        return EXIT_CODE_FAILURE;
    }

    /* Initialize persona registry */
    result = katra_identity_init();
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize persona registry: %s\n",
                katra_error_message(result));
        return EXIT_CODE_FAILURE;
    }

    /* Resolve CI identity */
    result = resolve_daemon_persona();
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to resolve persona\n");
        return EXIT_CODE_FAILURE;
    }

    /* Initialize full MCP subsystem for Claude Code tool support */

    /* Step 1: Core init */
    result = katra_init();
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize Katra core: %s\n",
                katra_error_message(result));
        return EXIT_CODE_FAILURE;
    }

    /* Step 2: Logging */
    log_init(NULL);
    log_set_level(LOG_INFO);

    /* Step 3: Memory subsystem */
    result = katra_memory_init(g_ci_id);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize Katra memory: %s\n",
                katra_error_message(result));
        katra_exit();
        return EXIT_CODE_FAILURE;
    }

    /* Step 4: Lifecycle layer */
    result = katra_lifecycle_init();
    if (result != KATRA_SUCCESS && result != E_ALREADY_INITIALIZED) {
        fprintf(stderr, "Failed to initialize lifecycle: %s\n",
                katra_error_message(result));
        katra_memory_cleanup();
        katra_exit();
        return EXIT_CODE_FAILURE;
    }

    /* Step 5: Hook registry + Anthropic adapter */
    result = katra_hooks_init();
    if (result != KATRA_SUCCESS && result != E_ALREADY_INITIALIZED) {
        fprintf(stderr, "Failed to initialize hooks: %s\n",
                katra_error_message(result));
        katra_lifecycle_cleanup();
        katra_memory_cleanup();
        katra_exit();
        return EXIT_CODE_FAILURE;
    }

    {
        const katra_hook_adapter_t* anthropic_adapter = katra_hook_anthropic_adapter();
        result = katra_hooks_register(anthropic_adapter);
        if (result != KATRA_SUCCESS) {
            LOG_WARN("Failed to register Anthropic adapter (non-fatal): %d", result);
        }
    }

    /* Step 6: Meeting room (inter-CI chat) */
    result = meeting_room_init();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Failed to initialize meeting room (non-fatal): %d", result);
    }

    /* Step 7: Vector database for semantic search */
    {
        extern vector_store_t* g_vector_store;
        g_vector_store = katra_vector_init(g_ci_id, false);
        if (!g_vector_store) {
            LOG_WARN("Vector database init failed, semantic search disabled");
        }
    }

    /* Step 8: Start session */
    result = katra_session_start(g_ci_id);
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Failed to start session (non-fatal): %d", result);
    }

    /* Set session persona name */
    {
        mcp_session_t* session = mcp_get_session();
        strncpy(session->chosen_name, g_persona_name, sizeof(session->chosen_name) - 1);
        session->chosen_name[sizeof(session->chosen_name) - 1] = '\0';
        session->registered = true;
    }

    /* Initialize module loader (non-fatal if fails) */
    result = katra_module_loader_init();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Module loader init failed (non-fatal): %d", result);
    } else {
        int discovered = katra_module_loader_discover();
        if (discovered > 0) {
            LOG_INFO("Discovered %d loadable module(s)", discovered);
        }
    }

    LOG_INFO("Starting Katra Unified Daemon v%s", DAEMON_VERSION);
    LOG_INFO("Configuration: HTTP=%d, MCP=%d, bind=%s, namespace=%s, persona=%s",
             config.http_port, config.mcp_port, config.bind_address,
             config.default_namespace, g_persona_name);
    if (config.enable_unix_socket) {
        LOG_INFO("Unix socket: %s", config.socket_path);
    }
    if (config.mcp_port > 0) {
        LOG_INFO("MCP JSON-RPC: port %d (for Claude Code)", config.mcp_port);
    }

    /* Start HTTP daemon (blocks until shutdown) */
    result = katra_http_daemon_start(&config);

    /* Cleanup (reverse order of initialization) */
    katra_module_loader_shutdown();
    katra_session_end();
    {
        extern vector_store_t* g_vector_store;
        if (g_vector_store) { katra_vector_cleanup(g_vector_store); g_vector_store = NULL; }
    }
    if (g_ci_id[0]) { meeting_room_unregister_ci(g_ci_id); }
    meeting_room_cleanup();
    katra_hooks_cleanup();
    katra_lifecycle_cleanup();
    katra_memory_cleanup();
    katra_exit();
    log_cleanup();

    /* GUIDELINE_APPROVED: shutdown diagnostic */
    fprintf(stderr, "Katra Unified Daemon stopped\n");
    return (result == KATRA_SUCCESS) ? EXIT_CODE_SUCCESS : EXIT_CODE_FAILURE;
}
