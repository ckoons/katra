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

/* Active client connections */
static mcp_tcp_client_t* g_clients[KATRA_MCP_MAX_CLIENTS];
static pthread_mutex_t g_clients_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t g_tcp_shutdown = 0;

/* Signal handler for graceful shutdown */
static void tcp_signal_handler(int signum) {
    if (signum == SIGTERM || signum == SIGINT) {
        LOG_INFO("TCP server received shutdown signal");
        g_tcp_shutdown = 1;
    }
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

/* Load configuration from file */
int mcp_tcp_load_config(mcp_tcp_config_t* config, const char* config_file) {
    KATRA_CHECK_NULL(config);
    KATRA_CHECK_NULL(config_file);

    /* For now, use defaults - later parse config_file */
    config->port = KATRA_MCP_DEFAULT_PORT;
    config->bind_address = "127.0.0.1";
    config->max_clients = KATRA_MCP_MAX_CLIENTS;
    config->enable_health_check = true;

    LOG_INFO("TCP config: port=%d, max_clients=%d",
             config->port, config->max_clients);

    return KATRA_SUCCESS;
}

/* Start TCP server (blocks until shutdown) */
int mcp_tcp_server_start(const mcp_tcp_config_t* config) {
    int server_fd = -1;
    int result = KATRA_SUCCESS;
    struct sockaddr_in server_addr;

    KATRA_CHECK_NULL(config);

    /* Initialize client tracking */
    memset(g_clients, 0, sizeof(g_clients));

    /* Setup signal handlers */
    signal(SIGTERM, tcp_signal_handler);
    signal(SIGINT, tcp_signal_handler);
    signal(SIGPIPE, SIG_IGN);  /* Ignore broken pipe */

    /* Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        katra_report_error(E_SYSTEM_IO, "mcp_tcp_server_start",
                          "Failed to create socket");
        return E_SYSTEM_IO;
    }

    /* Set socket options */
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_WARN("Failed to set SO_REUSEADDR: %s", strerror(errno));
    }

    /* Bind to address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->port);

    if (inet_pton(AF_INET, config->bind_address, &server_addr.sin_addr) <= 0) {
        katra_report_error(E_INPUT_INVALID, "mcp_tcp_server_start",
                          "Invalid bind address");
        close(server_fd);
        return E_INPUT_INVALID;
    }

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        katra_report_error(E_SYSTEM_IO, "mcp_tcp_server_start",
                          "Failed to bind socket");
        close(server_fd);
        return E_SYSTEM_IO;
    }

    /* Listen for connections */
    if (listen(server_fd, config->max_clients) < 0) {
        katra_report_error(E_SYSTEM_IO, "mcp_tcp_server_start",
                          "Failed to listen on socket");
        close(server_fd);
        return E_SYSTEM_IO;
    }

    LOG_INFO("TCP MCP server listening on %s:%d",
             config->bind_address, config->port);

    /* Accept loop */
    while (!g_tcp_shutdown) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        /* Use select for timeout to check shutdown flag */
        fd_set read_fds;
        struct timeval timeout;

        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ready = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;  /* Interrupted by signal */
            }
            LOG_ERROR("select() failed: %s", strerror(errno));
            break;
        }

        if (ready == 0) {
            continue;  /* Timeout, check shutdown flag */
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

        /* Check for available client slot */
        int slot = find_free_client_slot();
        if (slot < 0) {
            LOG_WARN("Max clients reached, rejecting connection");
            close(client_fd);
            continue;
        }

        /* Create client structure */
        mcp_tcp_client_t* client = calloc(1, sizeof(mcp_tcp_client_t));
        if (!client) {
            LOG_ERROR("Failed to allocate client structure");
            close(client_fd);
            continue;
        }

        client->socket_fd = client_fd;
        client->registered = false;
        client->connected_at = time(NULL);

        /* Add to tracking array */
        if (add_client(client) < 0) {
            LOG_ERROR("Failed to add client to tracking array");
            free(client);
            close(client_fd);
            continue;
        }

        /* Spawn thread to handle client */
        pthread_t thread;
        if (pthread_create(&thread, NULL, client_thread, (void*)(intptr_t)slot) != 0) {
            LOG_ERROR("Failed to create client thread");
            remove_client(slot);
            continue;
        }

        pthread_detach(thread);  /* Allow thread to clean up automatically */

        LOG_INFO("Accepted client connection from %s (slot %d)",
                 inet_ntoa(client_addr.sin_addr), slot);
    }

    /* Cleanup */
    LOG_INFO("TCP server shutting down...");

    /* Close all client connections */
    pthread_mutex_lock(&g_clients_lock);
    for (int i = 0; i < KATRA_MCP_MAX_CLIENTS; i++) {
        if (g_clients[i]) {
            close(g_clients[i]->socket_fd);
            free(g_clients[i]);
            g_clients[i] = NULL;
        }
    }
    pthread_mutex_unlock(&g_clients_lock);

    close(server_fd);
    LOG_INFO("TCP server stopped");

    return result;
}
