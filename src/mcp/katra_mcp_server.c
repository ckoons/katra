/* © 2025 Casey Koons All rights reserved */

/* Katra MCP Server - Main entry point and lifecycle management */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_mcp_tcp.h"
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
#include "katra_env_utils.h"

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

/* Global reload flag (SIGUSR1) */
volatile sig_atomic_t g_reload_requested = 0;

/* Binary modification time for hot reload detection */
static time_t g_binary_mtime = 0;

/* Signal handler for shutdown (SIGTERM, SIGINT) */
void mcp_signal_handler(int signum) {
    if (signum == SIGUSR1) {
        /* Hot reload request */
        g_reload_requested = 1;
        /* GUIDELINE_APPROVED: async-signal-safe message */
        ssize_t result = write(STDERR_FILENO, "Hot reload requested...\n",
                              strlen("Hot reload requested...\n"));
        (void)result;
    } else {
        /* Shutdown request */
        g_shutdown_requested = 1;
        /* GUIDELINE_APPROVED: async-signal-safe shutdown message */
        ssize_t result = write(STDERR_FILENO, MCP_MSG_SHUTDOWN, strlen(MCP_MSG_SHUTDOWN));
        (void)result;
    }
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

/* Migrate persona from old PID-based ci_id to name-based ci_id if needed */
static int migrate_persona_if_needed(const char* persona_name, const char* new_ci_id,
                                     const char* old_ci_id) {
    if (strcmp(old_ci_id, new_ci_id) != 0) {
        /* Old PID-based ci_id detected - update to name-based */
        /* GUIDELINE_APPROVED: startup diagnostic message */
        fprintf(stderr, "Migrating persona '%s' from old ci_id '%s' to name-based '%s'\n",
                persona_name, old_ci_id, new_ci_id);

        /* Update persona registry with new ci_id */
        return katra_register_persona(persona_name, new_ci_id);
    }

    /* Already using name-based ci_id */
    /* GUIDELINE_APPROVED: startup diagnostic message */
    fprintf(stderr, "Katra MCP Server resuming persona '%s' with CI identity: %s\n",
            persona_name, new_ci_id);
    return KATRA_SUCCESS;
}

/* Handle existing persona (found in registry) */
static int handle_existing_persona(const char* persona_name, const char* ci_id,
                                   const char* old_ci_id) {
    int result = migrate_persona_if_needed(persona_name, ci_id, old_ci_id);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Update session count */
    return katra_update_persona_session(persona_name);
}

/* Handle new persona (not in registry) */
static int handle_new_persona(const char* persona_name, const char* ci_id) {
    int result = katra_register_persona(persona_name, ci_id);
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to register persona: %s\n",
                katra_error_message(result));
        return result;
    }

    /* GUIDELINE_APPROVED: startup diagnostic message */
    fprintf(stderr, "Katra MCP Server created new persona '%s' with CI identity: %s\n",
            persona_name, ci_id);
    return KATRA_SUCCESS;
}

/* Resolve persona from environment variable */
static int resolve_persona_from_env(char* persona_name_out, char* ci_id_out,
                                    size_t buffer_size) {
    const char* env_name = katra_getenv("KATRA_PERSONA");
    if (!env_name || strlen(env_name) == 0) {
        return E_INPUT_NULL;  /* Signal: try next priority */
    }

    /* Set persona name from environment */
    strncpy(persona_name_out, env_name, buffer_size - 1);
    persona_name_out[buffer_size - 1] = '\0';

    /* ALWAYS use persona name as ci_id (identity preservation fix) */
    strncpy(ci_id_out, persona_name_out, buffer_size - 1);
    ci_id_out[buffer_size - 1] = '\0';

    /* Look up in persona registry to check if exists */
    char old_ci_id[KATRA_CI_ID_SIZE];
    int result = katra_lookup_persona(env_name, old_ci_id, sizeof(old_ci_id));

    if (result == KATRA_SUCCESS) {
        /* Found existing persona */
        return handle_existing_persona(persona_name_out, ci_id_out, old_ci_id);
    }

    /* Not found - create new persona */
    return handle_new_persona(persona_name_out, ci_id_out);
}

/* Resolve persona from last active in registry */
static int resolve_persona_from_last_active(char* persona_name_out, char* ci_id_out,
                                            size_t buffer_size) {
    char last_active_name[KATRA_CI_ID_SIZE];
    char registry_ci_id[KATRA_CI_ID_SIZE];

    int result = katra_get_last_active(last_active_name, sizeof(last_active_name),
                                      registry_ci_id, sizeof(registry_ci_id));
    if (result != KATRA_SUCCESS) {
        return result;  /* Signal: try next priority */
    }

    /* Set persona name */
    strncpy(persona_name_out, last_active_name, buffer_size - 1);
    persona_name_out[buffer_size - 1] = '\0';

    /* ALWAYS use persona name as ci_id (identity preservation fix) */
    strncpy(ci_id_out, persona_name_out, buffer_size - 1);
    ci_id_out[buffer_size - 1] = '\0';

    /* Handle migration if needed */
    return handle_existing_persona(persona_name_out, ci_id_out, registry_ci_id);
}

/* Create anonymous persona (fallback when no persona found) */
static int create_anonymous_persona(char* persona_name_out, char* ci_id_out,
                                    size_t buffer_size) {
    /* Create timestamp-based anonymous name */
    time_t now = time(NULL);
    snprintf(persona_name_out, buffer_size, "anonymous_%ld", (long)now);

    /* Use persona name as ci_id for consistency */
    strncpy(ci_id_out, persona_name_out, buffer_size - 1);
    ci_id_out[buffer_size - 1] = '\0';

    /* Register anonymous persona */
    return handle_new_persona(persona_name_out, ci_id_out);
}

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
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
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

        /* Step 4.6: Auto-regenerate vectors if needed (Phase 6.1f - Async) */
        /* Note: Semantic search is enabled by default, so check if vectors need building */
        if (g_vector_store->count < MIN_VECTOR_COUNT_THRESHOLD) {
            /* Vector count is very low - start async regeneration */
            LOG_INFO("Starting async vector regeneration (current count: %zu)", g_vector_store->count);
            /* GUIDELINE_APPROVED: startup diagnostic for user feedback */
            fprintf(stderr, "Warming up semantic search (vectors building in background)...\n");

            int regen_result = regenerate_vectors_async();
            if (regen_result != KATRA_SUCCESS) {
                LOG_WARN("Failed to start async vector regeneration: %d", regen_result);
                /* Non-fatal: server will work with keyword search fallback */
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

    /* Unregister from meeting room before cleanup (prevents duplicate registrations) */
    if (g_ci_id[0]) {
        meeting_room_unregister_ci(g_ci_id);
    }

    meeting_room_cleanup();
    katra_hooks_cleanup();  /* Hook registry cleanup */
    katra_lifecycle_cleanup();  /* Lifecycle layer cleanup */
    katra_memory_cleanup();
    katra_exit();

    LOG_INFO("MCP server cleanup complete");
}

/* Check if binary has been updated */
/* Note: Currently unused - reserved for future auto-reload detection */
static bool mcp_server_binary_updated(void) __attribute__((unused));
static bool mcp_server_binary_updated(void) {
    struct stat st;
    char binary_path[KATRA_PATH_MAX];

#ifdef __APPLE__
    /* macOS: use _NSGetExecutablePath */
    uint32_t size = sizeof(binary_path);
    if (_NSGetExecutablePath(binary_path, &size) != 0) {
        return false;
    }
#else
    /* Linux: use /proc/self/exe */
    if (readlink("/proc/self/exe", binary_path, sizeof(binary_path)) < 0) {
        return false;
    }
#endif

    if (stat(binary_path, &st) == 0) {
        if (g_binary_mtime == 0) {
            /* First time - store mtime */
            g_binary_mtime = st.st_mtime;
            return false;
        }
        return st.st_mtime > g_binary_mtime;
    }
    return false;
}

/* Perform hot reload via exec() */
static void mcp_server_hot_reload(int argc, char* argv[], char* envp[]) {
    (void)argc;  /* Unused - kept for future extensibility */

    LOG_INFO("Hot reload: executing new binary");
    /* GUIDELINE_APPROVED: user notification of hot reload */
    fprintf(stderr, "Reloading MCP server with updated binary...\n");

    /* exec() replaces current process with new binary */
    execve(argv[0], argv, envp);

    /* If we get here, exec() failed */
    LOG_ERROR("Hot reload failed: execve() error");
    katra_report_error(E_SYSTEM_PROCESS, "mcp_server_hot_reload",
                      "Failed to execute new binary");
}

/* Main loop - read JSON-RPC requests from stdin */
void mcp_main_loop(void) {
    char buffer[MCP_MAX_LINE];

    LOG_INFO("MCP server main loop started");

    while (!g_shutdown_requested && !g_reload_requested && fgets(buffer, sizeof(buffer), stdin)) {
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
    } else if (g_reload_requested) {
        LOG_INFO("MCP server main loop exiting (reload requested)");
    } else {
        LOG_INFO("MCP server main loop exiting (stdin closed)");
    }
}

/* Main entry point */
int main(int argc, char* argv[], char* envp[]) {
    int exit_code = EXIT_SUCCESS;
    bool tcp_mode = false;
    uint16_t tcp_port = KATRA_MCP_DEFAULT_PORT;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tcp") == 0) {
            tcp_mode = true;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            tcp_port = (uint16_t)atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("Options:\n");
            printf("  --tcp         Run in TCP multi-tenant mode\n");
            printf("  --port PORT   TCP port (default: %d)\n", KATRA_MCP_DEFAULT_PORT);
            printf("  --help        Show this help message\n");
            return EXIT_SUCCESS;
        }
    }

    /* Setup signal handlers */
    signal(SIGTERM, mcp_signal_handler);
    signal(SIGINT, mcp_signal_handler);
    signal(SIGUSR1, mcp_signal_handler);  /* Hot reload */
    signal(SIGPIPE, SIG_IGN);

    /* Load environment from .env files (required for KATRA_PERSONA) */
    int result = katra_loadenv();
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Warning: Failed to load .env configuration: %s\n",
                katra_error_message(result));
        /* Continue anyway - can still use system environment */
    }

    /* Read TCP mode from environment if not set via command line */
    if (!tcp_mode) {
        const char* tcp_mode_env = katra_getenv("KATRA_MCP_TCP_MODE");
        if (tcp_mode_env && (strcmp(tcp_mode_env, "true") == 0 || strcmp(tcp_mode_env, "1") == 0)) {
            tcp_mode = true;
        }
    }

    /* Read TCP port from environment if not set via command line */
    if (tcp_mode && tcp_port == KATRA_MCP_DEFAULT_PORT) {
        int env_port = 0;
        if (katra_getenvint("KATRA_MCP_TCP_PORT", &env_port) == KATRA_SUCCESS) {
            if (env_port > 0 && env_port <= 65535) {
                tcp_port = (uint16_t)env_port;
            } else {
                /* GUIDELINE_APPROVED: startup diagnostic */
                fprintf(stderr, "Warning: Invalid KATRA_MCP_TCP_PORT=%d, using default %d\n",
                        env_port, KATRA_MCP_DEFAULT_PORT);
            }
        }
    }

    /* Initialize persona registry */
    result = katra_identity_init();
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Failed to initialize persona registry: %s\n",
                katra_error_message(result));
        return EXIT_FAILURE;
    }

    /* Determine CI identity using persona system (priority cascade) */
    /* Priority 1: KATRA_PERSONA environment variable */
    result = resolve_persona_from_env(g_persona_name, g_ci_id, sizeof(g_persona_name));

    if (result != KATRA_SUCCESS) {
        /* Priority 2: last_active from persona registry */
        result = resolve_persona_from_last_active(g_persona_name, g_ci_id, sizeof(g_persona_name));

        if (result != KATRA_SUCCESS) {
            /* Priority 3: Create anonymous persona */
            result = create_anonymous_persona(g_persona_name, g_ci_id, sizeof(g_persona_name));
            if (result != KATRA_SUCCESS) {
                return EXIT_FAILURE;
            }
        }
    }

    /* Update session name from resolved persona */
    strncpy(g_session.chosen_name, g_persona_name, sizeof(g_session.chosen_name) - 1);
    g_session.chosen_name[sizeof(g_session.chosen_name) - 1] = '\0';

    /* Auto-register when KATRA_PERSONA is set (prevents "Katra" default name) */
    g_session.registered = true;

    /* Initialize server with determined ci_id */
    result = mcp_server_init(g_ci_id);
    if (result != KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: startup diagnostic before logging initialized */
        fprintf(stderr, "Server initialization failed: %s\n",
                katra_error_message(result));
        return EXIT_FAILURE;
    }

    /* Run in TCP mode or stdio mode */
    if (tcp_mode) {
        /* TCP multi-tenant mode */
        mcp_tcp_config_t config;
        config.port = tcp_port;
        config.bind_address = "127.0.0.1";
        config.max_clients = KATRA_MCP_MAX_CLIENTS;
        config.enable_health_check = true;

        LOG_INFO("Starting TCP MCP server on port %d", tcp_port);
        result = mcp_tcp_server_start(&config);
        if (result != KATRA_SUCCESS) {
            /* GUIDELINE_APPROVED: main() error output before exit */
            fprintf(stderr, "TCP server failed: %s\n",
                    katra_error_message(result));
            exit_code = EXIT_FAILURE;
        }
    } else {
        /* stdio mode (backward compatibility) */
        mcp_main_loop();
    }

    /* Cleanup */
    mcp_server_cleanup();

    /* Check for hot reload request */
    if (g_reload_requested) {
        /* Perform hot reload via exec() - does not return */
        mcp_server_hot_reload(argc, argv, envp);
        /* If we get here, exec() failed - fall through to normal exit */
    }

    /* GUIDELINE_APPROVED: shutdown diagnostic message */
    fprintf(stderr, "Katra MCP Server stopped\n");
    return exit_code;
}
