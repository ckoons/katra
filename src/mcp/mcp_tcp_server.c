/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

/* Project includes */
#include "katra_mcp_tcp.h"
#include "katra_mcp.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_strings.h"
#include "katra_env_utils.h"

/* Active client connections */
static mcp_tcp_client_t* g_clients[KATRA_MCP_MAX_CLIENTS];
static pthread_mutex_t g_clients_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t g_tcp_shutdown = 0;

/* Forward declarations */
static int find_free_client_slot(void);
static int add_client(mcp_tcp_client_t* client);
static void remove_client(int slot);
static void* client_thread(void* arg);

/* Signal handler for graceful shutdown */
static void tcp_signal_handler(int signum) {
    if (signum == SIGTERM || signum == SIGINT) {
        LOG_INFO("TCP server received shutdown signal");
        g_tcp_shutdown = 1;
    }
}

/* Helper: Setup server socket (bind and listen) */
static int tcp_server_socket_setup(const mcp_tcp_config_t* config, int* server_fd_out) {
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;

    /* Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        katra_report_error(E_SYSTEM_IO, "tcp_server_socket_setup",
                          "Failed to create socket");
        return E_SYSTEM_IO;
    }

    /* Set socket options */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_WARN("Failed to set SO_REUSEADDR: %s", strerror(errno));
    }

    /* Bind to address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->port);

    if (inet_pton(AF_INET, config->bind_address, &server_addr.sin_addr) <= 0) {
        close(server_fd);
        katra_report_error(E_INPUT_INVALID, "tcp_server_socket_setup",
                          "Invalid bind address");
        return E_INPUT_INVALID;
    }

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd);
        katra_report_error(E_SYSTEM_IO, "tcp_server_socket_setup",
                          "Failed to bind socket");
        return E_SYSTEM_IO;
    }

    /* Listen for connections */
    if (listen(server_fd, config->max_clients) < 0) {
        close(server_fd);
        katra_report_error(E_SYSTEM_IO, "tcp_server_socket_setup",
                          "Failed to listen on socket");
        return E_SYSTEM_IO;
    }

    LOG_INFO("TCP MCP server listening on %s:%d",
             config->bind_address, config->port);

    *server_fd_out = server_fd;
    return KATRA_SUCCESS;
}

/* Helper: Handle incoming client connection */
static int tcp_server_handle_client(int client_fd, const struct sockaddr_in* client_addr) {
    int slot;
    mcp_tcp_client_t* client = NULL;
    pthread_t thread;

    /* Check for available client slot */
    slot = find_free_client_slot();
    if (slot < 0) {
        LOG_WARN("Max clients reached, rejecting connection");
        close(client_fd);
        return E_RESOURCE_LIMIT;
    }

    /* Create client structure */
    client = calloc(1, sizeof(mcp_tcp_client_t));
    if (!client) {
        LOG_ERROR("Failed to allocate client structure");
        close(client_fd);
        return E_SYSTEM_MEMORY;
    }

    client->socket_fd = client_fd;
    client->registered = false;
    client->connected_at = time(NULL);

    /* Add to tracking array */
    if (add_client(client) < 0) {
        LOG_ERROR("Failed to add client to tracking array");
        free(client);
        close(client_fd);
        return E_INTERNAL_LOGIC;
    }

    /* Spawn thread to handle client */
    if (pthread_create(&thread, NULL, client_thread, (void*)(intptr_t)slot) != 0) {
        LOG_ERROR("Failed to create client thread");
        remove_client(slot);
        return E_SYSTEM_PROCESS;
    }

    pthread_detach(thread);

    LOG_INFO("Accepted client connection from %s (slot %d)",
             inet_ntoa(client_addr->sin_addr), slot);

    return KATRA_SUCCESS;
}

/* Helper: Cleanup all TCP server resources */
static void tcp_server_cleanup(int server_fd) {
    LOG_INFO("TCP server shutting down...");

    /* Close all client connections */
    (void)pthread_mutex_lock(&g_clients_lock);
    for (int i = 0; i < KATRA_MCP_MAX_CLIENTS; i++) {
        if (g_clients[i]) {
            close(g_clients[i]->socket_fd);
            free(g_clients[i]);
            g_clients[i] = NULL;
        }
    }
    pthread_mutex_unlock(&g_clients_lock);

    if (server_fd >= 0) {
        close(server_fd);
    }
    LOG_INFO("TCP server stopped");
}

/* Find free client slot */
static int find_free_client_slot(void) {
    int result = -1;
    pthread_mutex_lock(&g_clients_lock);

    for (int i = 0; i < KATRA_MCP_MAX_CLIENTS; i++) {
        if (g_clients[i] == NULL) {
            result = i;
            break;
        }
    }

    pthread_mutex_unlock(&g_clients_lock);
    return result;
}

/* Add client to tracking array */
static int add_client(mcp_tcp_client_t* client) {
    int result = -1;
    pthread_mutex_lock(&g_clients_lock);

    for (int i = 0; i < KATRA_MCP_MAX_CLIENTS; i++) {
        if (g_clients[i] == NULL) {
            g_clients[i] = client;
            result = i;
            break;
        }
    }

    pthread_mutex_unlock(&g_clients_lock);
    return result;
}

/* Remove client from tracking array */
static void remove_client(int slot) {
    pthread_mutex_lock(&g_clients_lock);

    if (slot >= 0 && slot < KATRA_MCP_MAX_CLIENTS) {
        if (g_clients[slot]) {
            close(g_clients[slot]->socket_fd);
            free(g_clients[slot]);
            g_clients[slot] = NULL;
        }
    }

    pthread_mutex_unlock(&g_clients_lock);
}

/* Health check endpoint handler */
int mcp_tcp_handle_health_check(int client_fd) {
    const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "{\"status\":\"healthy\",\"ok\":true}";

    ssize_t sent = write(client_fd, response, strlen(response));
    if (sent < 0) {
        return E_SYSTEM_IO;
    }

    return KATRA_SUCCESS;
}

/* Handle single client connection (runs in thread) */
void mcp_tcp_handle_client(mcp_tcp_client_t* client) {
    char buffer[MCP_MAX_LINE];
    ssize_t bytes_read = 0;

    if (!client) {
        return;
    }

    LOG_INFO("Handling client connection on fd %d", client->socket_fd);

    /* Read and process requests in a loop */
    while (!g_tcp_shutdown) {
        bytes_read = read(client->socket_fd, buffer, sizeof(buffer) - 1);

        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                LOG_WARN("Client read error: %s", strerror(errno));
            }
            break;  /* Client disconnected or error */
        }

        buffer[bytes_read] = '\0';

        /* Check for health check request */
        if (strncmp(buffer, "GET /health", 11) == 0) {
            mcp_tcp_handle_health_check(client->socket_fd);
            break;  /* Close after health check */
        }

        /* Strip newline */
        buffer[strcspn(buffer, MCP_CHAR_NEWLINE)] = '\0';

        if (strlen(buffer) == 0) {
            continue;
        }

        LOG_DEBUG("Client request: %.100s...", buffer);

        /* Parse JSON-RPC request */
        json_t* request = mcp_parse_request(buffer);
        if (!request) {
            json_t* error_response = mcp_error_response(NULL, MCP_ERROR_PARSE,
                                                       MCP_ERR_PARSE_ERROR,
                                                       MCP_ERR_INVALID_REQUEST);
            if (error_response) {
                char* response_str = json_dumps(error_response, JSON_COMPACT);
                if (response_str) {
                    dprintf(client->socket_fd, "%s\n", response_str);
                    free(response_str);
                }
                json_decref(error_response);
            }
            continue;
        }

        /* Dispatch request to handler */
        json_t* response = mcp_dispatch_request(request);
        json_decref(request);

        if (response) {
            char* response_str = json_dumps(response, JSON_COMPACT);
            if (response_str) {
                dprintf(client->socket_fd, "%s\n", response_str);
                free(response_str);
            }
            json_decref(response);
        }
    }

    LOG_INFO("Client connection closed on fd %d", client->socket_fd);
}

/* Client thread entry point */
static void* client_thread(void* arg) {
    int slot = (int)(intptr_t)arg;
    mcp_tcp_client_t* client = NULL;

    pthread_mutex_lock(&g_clients_lock);
    client = g_clients[slot];
    pthread_mutex_unlock(&g_clients_lock);

    if (client) {
        mcp_tcp_handle_client(client);
        remove_client(slot);
    }

    return NULL;
}

/* Load configuration from Katra environment */
int mcp_tcp_load_config(mcp_tcp_config_t* config, const char* config_file) {
    int result = KATRA_SUCCESS;
    KATRA_CHECK_NULL(config);

    /* Note: config_file parameter kept for API compatibility but ignored */
    /* Configuration now comes from .env files via katra_getenv() */
    (void)config_file;  /* Unused */

    /* Load port from environment */
    config->port = KATRA_MCP_DEFAULT_PORT;
    int env_port = 0;
    if (katra_getenvint("KATRA_MCP_TCP_PORT", &env_port) == KATRA_SUCCESS) {
        if (env_port > 0 && env_port <= 65535) {
            config->port = (uint16_t)env_port;
        }
    }

    /* Load bind address from environment */
    const char* bind_addr = katra_getenv("KATRA_MCP_TCP_BIND");
    config->bind_address = bind_addr ? bind_addr : "127.0.0.1";

    /* Load max clients from environment */
    config->max_clients = KATRA_MCP_MAX_CLIENTS;
    int max_clients = 0;
    if (katra_getenvint("KATRA_MCP_TCP_MAX_CLIENTS", &max_clients) == KATRA_SUCCESS) {
        if (max_clients > 0 && max_clients <= KATRA_MCP_MAX_CLIENTS) {
            config->max_clients = max_clients;
        }
    }

    /* Load health check setting from environment */
    config->enable_health_check = true;
    const char* health_check = katra_getenv("KATRA_MCP_TCP_HEALTH_CHECK");
    if (health_check) {
        if (strcmp(health_check, "false") == 0 || strcmp(health_check, "0") == 0) {
            config->enable_health_check = false;
        }
    }

    LOG_INFO("TCP config: port=%d, bind=%s, max_clients=%d, health_check=%s",
             config->port, config->bind_address, config->max_clients,
             config->enable_health_check ? "enabled" : "disabled");

    return result;
}

/* Start TCP server (blocks until shutdown) */
int mcp_tcp_server_start(const mcp_tcp_config_t* config) {
    int server_fd = -1;
    int result = KATRA_SUCCESS;

    KATRA_CHECK_NULL(config);

    /* Initialize client tracking */
    memset(g_clients, 0, sizeof(g_clients));

    /* Setup signal handlers */
    signal(SIGTERM, tcp_signal_handler);
    signal(SIGINT, tcp_signal_handler);
    signal(SIGPIPE, SIG_IGN);

    /* Setup server socket */
    result = tcp_server_socket_setup(config, &server_fd);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Accept loop */
    while (!g_tcp_shutdown) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        fd_set read_fds;
        struct timeval timeout;

        /* Use select for timeout to check shutdown flag */
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ready = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("select() failed: %s", strerror(errno));
            break;
        }

        if (ready == 0) {
            continue;
        }

        /* Accept connection */
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("accept() failed: %s", strerror(errno));
            continue;
        }

        /* Handle client in separate thread */
        tcp_server_handle_client(client_fd, &client_addr);
    }

    /* Cleanup resources */
    tcp_server_cleanup(server_fd);

    return result;
}
