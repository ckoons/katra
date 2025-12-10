/* © 2025 Casey Koons All rights reserved */

/*
 * Katra HTTP Daemon
 *
 * HTTP server for the unified Katra interface. Accepts POST /operation
 * requests with shared_state JSON, dispatches to method handlers.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

/* Project includes */
#include "katra_unified.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Shutdown flag */
static volatile sig_atomic_t g_http_shutdown = 0;

/* Signal handler */
static void http_signal_handler(int signum) {
    if (signum == SIGTERM || signum == SIGINT) {
        LOG_INFO("HTTP daemon received shutdown signal");
        g_http_shutdown = 1;
    }
}

/* HTTP response templates */
static const char* HTTP_RESPONSE_TEMPLATE =
    "HTTP/1.1 %d %s\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %zu\r\n"
    "Connection: close\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
    "Access-Control-Allow-Headers: Content-Type\r\n"
    "\r\n"
    "%s";

static const char* HTTP_CORS_RESPONSE =
    "HTTP/1.1 204 No Content\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
    "Access-Control-Allow-Headers: Content-Type\r\n"
    "Access-Control-Max-Age: 86400\r\n"
    "Connection: close\r\n"
    "\r\n";

/* Status code to reason phrase */
static const char* http_status_phrase(int code) {
    switch (code) {
        case HTTP_OK: return "OK";
        case HTTP_BAD_REQUEST: return "Bad Request";
        case HTTP_NOT_FOUND: return "Not Found";
        case HTTP_METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HTTP_INTERNAL_ERROR: return "Internal Server Error";
        default: return "Unknown";
    }
}

/* Send HTTP response */
int katra_http_send_response(int client_fd, int status_code, const char* body) {
    if (client_fd < 0 || !body) {
        return E_INPUT_INVALID;
    }

    char response[KATRA_UNIFIED_MAX_RESPONSE + KATRA_HTTP_HEADER_SIZE];
    size_t body_len = strlen(body);

    int response_len = snprintf(response, sizeof(response), HTTP_RESPONSE_TEMPLATE,
                                 status_code, http_status_phrase(status_code),
                                 body_len, body);

    if (response_len < 0 || (size_t)response_len >= sizeof(response)) {
        LOG_ERROR("Response buffer overflow");
        return E_BUFFER_OVERFLOW;
    }

    ssize_t sent = write(client_fd, response, (size_t)response_len);
    if (sent < 0) {
        LOG_ERROR("Failed to send response: %s", strerror(errno));
        return E_SYSTEM_IO;
    }

    return KATRA_SUCCESS;
}

/* Parse HTTP request - extract method, path, and body */
static int parse_http_request(const char* request, size_t request_len,
                               char* method, size_t method_size,
                               char* path, size_t path_size,
                               const char** body, size_t* body_len) {
    if (!request || !method || !path || !body || !body_len) {
        return E_INPUT_NULL;
    }

    /* Parse request line: METHOD PATH HTTP/1.x */
    const char* line_end = strstr(request, "\r\n");
    if (!line_end) {
        return E_INPUT_INVALID;
    }

    /* Extract method */
    const char* space1 = strchr(request, ' ');
    if (!space1 || space1 > line_end) {
        return E_INPUT_INVALID;
    }

    size_t method_len = (size_t)(space1 - request);
    if (method_len >= method_size) {
        return E_BUFFER_OVERFLOW;
    }
    strncpy(method, request, method_len);
    method[method_len] = '\0';

    /* Extract path */
    const char* path_start = space1 + 1;
    const char* space2 = strchr(path_start, ' ');
    if (!space2 || space2 > line_end) {
        return E_INPUT_INVALID;
    }

    size_t path_len = (size_t)(space2 - path_start);
    if (path_len >= path_size) {
        return E_BUFFER_OVERFLOW;
    }
    strncpy(path, path_start, path_len);
    path[path_len] = '\0';

    /* Find body (after double CRLF) */
    const char* body_start = strstr(request, "\r\n\r\n");
    if (body_start) {
        body_start += 4;  /* Skip \r\n\r\n */
        *body = body_start;
        *body_len = request_len - (size_t)(body_start - request);
    } else {
        *body = NULL;
        *body_len = 0;
    }

    return KATRA_SUCCESS;
}

/* Handle GET /health */
static int handle_health_check(int client_fd) {
    const char* health_json = "{\"status\":\"healthy\",\"service\":\"katra-unified\",\"version\":\"1.0\"}";
    return katra_http_send_response(client_fd, HTTP_OK, health_json);
}

/* Handle GET /methods */
static int handle_list_methods(int client_fd) {
    json_t* methods = katra_list_methods();
    if (!methods) {
        return katra_http_send_response(client_fd, HTTP_INTERNAL_ERROR,
                                         "{\"error\":\"Failed to list methods\"}");
    }

    json_t* response = json_object();
    json_object_set_new(response, "methods", methods);
    json_object_set_new(response, "count", json_integer(json_array_size(methods)));

    char* json_str = json_dumps(response, JSON_COMPACT);
    json_decref(response);

    if (!json_str) {
        return katra_http_send_response(client_fd, HTTP_INTERNAL_ERROR,
                                         "{\"error\":\"Failed to serialize methods\"}");
    }

    int result = katra_http_send_response(client_fd, HTTP_OK, json_str);
    free(json_str);
    return result;
}

/* Handle POST /operation */
static int handle_operation(int client_fd, const char* body, size_t body_len) {
    if (!body || body_len == 0) {
        return katra_http_send_response(client_fd, HTTP_BAD_REQUEST,
                                         "{\"error\":{\"code\":\"E_PARSE\",\"message\":\"Empty request body\"}}");
    }

    /* Parse request JSON */
    json_t* request = NULL;
    int result = katra_unified_parse_request(body, &request);
    if (result != KATRA_SUCCESS) {
        return katra_http_send_response(client_fd, HTTP_BAD_REQUEST,
                                         "{\"error\":{\"code\":\"E_PARSE\",\"message\":\"Invalid JSON\"}}");
    }

    /* Dispatch to handler */
    json_t* response = katra_unified_dispatch(request);
    json_decref(request);

    if (!response) {
        return katra_http_send_response(client_fd, HTTP_INTERNAL_ERROR,
                                         "{\"error\":{\"code\":\"E_INTERNAL\",\"message\":\"Dispatch failed\"}}");
    }

    /* Serialize response */
    char* json_str = json_dumps(response, JSON_COMPACT);
    json_decref(response);

    if (!json_str) {
        return katra_http_send_response(client_fd, HTTP_INTERNAL_ERROR,
                                         "{\"error\":{\"code\":\"E_INTERNAL\",\"message\":\"Serialization failed\"}}");
    }

    /* Send response */
    result = katra_http_send_response(client_fd, HTTP_OK, json_str);
    free(json_str);
    return result;
}

/* Handle single HTTP request */
int katra_http_handle_request(int client_fd, const char* request_body, size_t body_len) {
    char method[16] = {0};
    char path[256] = {0};
    const char* body = NULL;
    size_t content_len = 0;

    /* Parse HTTP request */
    int result = parse_http_request(request_body, body_len, method, sizeof(method),
                                     path, sizeof(path), &body, &content_len);
    if (result != KATRA_SUCCESS) {
        return katra_http_send_response(client_fd, HTTP_BAD_REQUEST,
                                         "{\"error\":{\"code\":\"E_PARSE\",\"message\":\"Invalid HTTP request\"}}");
    }

    LOG_DEBUG("HTTP %s %s", method, path);

    /* Handle CORS preflight */
    if (strcmp(method, "OPTIONS") == 0) {
        ssize_t sent = write(client_fd, HTTP_CORS_RESPONSE, strlen(HTTP_CORS_RESPONSE));
        return (sent > 0) ? KATRA_SUCCESS : E_SYSTEM_IO;
    }

    /* Route to handler */
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/health") == 0) {
            return handle_health_check(client_fd);
        } else if (strcmp(path, "/methods") == 0) {
            return handle_list_methods(client_fd);
        } else {
            return katra_http_send_response(client_fd, HTTP_NOT_FOUND,
                                             "{\"error\":{\"code\":\"E_NOT_FOUND\",\"message\":\"Endpoint not found\"}}");
        }
    } else if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/operation") == 0) {
            return handle_operation(client_fd, body, content_len);
        } else {
            return katra_http_send_response(client_fd, HTTP_NOT_FOUND,
                                             "{\"error\":{\"code\":\"E_NOT_FOUND\",\"message\":\"Endpoint not found\"}}");
        }
    } else {
        return katra_http_send_response(client_fd, HTTP_METHOD_NOT_ALLOWED,
                                         "{\"error\":{\"code\":\"E_METHOD\",\"message\":\"Method not allowed\"}}");
    }
}

/* Client handler thread */
static void* client_thread(void* arg) {
    int client_fd = (int)(intptr_t)arg;
    char buffer[KATRA_UNIFIED_MAX_REQUEST];
    ssize_t bytes_read;

    /* Read request */
    bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        return NULL;
    }
    buffer[bytes_read] = '\0';

    /* Handle request */
    katra_http_handle_request(client_fd, buffer, (size_t)bytes_read);

    close(client_fd);
    return NULL;
}

/* Start HTTP daemon (blocks until shutdown) */
int katra_http_daemon_start(const katra_daemon_config_t* config) {
    int server_fd = -1;
    int result = KATRA_SUCCESS;
    struct sockaddr_in server_addr;
    int opt = 1;

    KATRA_CHECK_NULL(config);

    /* Initialize dispatcher */
    result = katra_unified_init(config);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Setup signal handlers */
    signal(SIGTERM, http_signal_handler);
    signal(SIGINT, http_signal_handler);
    signal(SIGPIPE, SIG_IGN);

    /* Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        katra_report_error(E_SYSTEM_IO, "katra_http_daemon_start",
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
    server_addr.sin_port = htons(config->http_port);

    if (inet_pton(AF_INET, config->bind_address, &server_addr.sin_addr) <= 0) {
        close(server_fd);
        katra_report_error(E_INPUT_INVALID, "katra_http_daemon_start",
                          "Invalid bind address");
        return E_INPUT_INVALID;
    }

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd);
        katra_report_error(E_SYSTEM_IO, "katra_http_daemon_start",
                          "Failed to bind socket");
        return E_SYSTEM_IO;
    }

    /* Listen */
    if (listen(server_fd, config->max_clients) < 0) {
        close(server_fd);
        katra_report_error(E_SYSTEM_IO, "katra_http_daemon_start",
                          "Failed to listen on socket");
        return E_SYSTEM_IO;
    }

    LOG_INFO("Katra HTTP daemon listening on %s:%d",
             config->bind_address, config->http_port);

    /* Create Unix socket for local fast path (optional) */
    int unix_fd = -1;
    if (config->enable_unix_socket && config->socket_path) {
        unix_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (unix_fd < 0) {
            LOG_WARN("Failed to create Unix socket: %s", strerror(errno));
        } else {
            /* Remove old socket if exists */
            unlink(config->socket_path);

            struct sockaddr_un unix_addr;
            memset(&unix_addr, 0, sizeof(unix_addr));
            unix_addr.sun_family = AF_UNIX;
            strncpy(unix_addr.sun_path, config->socket_path,
                    sizeof(unix_addr.sun_path) - 1);

            if (bind(unix_fd, (struct sockaddr*)&unix_addr, sizeof(unix_addr)) < 0) {
                LOG_WARN("Failed to bind Unix socket: %s", strerror(errno));
                close(unix_fd);
                unix_fd = -1;
            } else if (listen(unix_fd, config->max_clients) < 0) {
                LOG_WARN("Failed to listen on Unix socket: %s", strerror(errno));
                close(unix_fd);
                unix_fd = -1;
            } else {
                /* Set socket permissions (world readable/writable) */
                chmod(config->socket_path, 0666);
                LOG_INFO("Katra Unix socket listening on %s", config->socket_path);
            }
        }
    }

    /* Accept loop */
    int max_fd = server_fd;
    if (unix_fd > max_fd) max_fd = unix_fd;

    while (!g_http_shutdown) {
        fd_set read_fds;
        struct timeval timeout;

        /* Use select for timeout to check shutdown flag */
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        if (unix_fd >= 0) {
            FD_SET(unix_fd, &read_fds);
        }
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("select() failed: %s", strerror(errno));
            break;
        }

        if (ready == 0) {
            continue;  /* Timeout, check shutdown flag */
        }

        /* Accept connection on HTTP socket */
        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd >= 0) {
                /* Spawn thread to handle client */
                pthread_t thread;
                if (pthread_create(&thread, NULL, client_thread, (void*)(intptr_t)client_fd) != 0) {
                    LOG_ERROR("Failed to create client thread");
                    close(client_fd);
                } else {
                    pthread_detach(thread);
                }
            } else if (errno != EINTR) {
                LOG_ERROR("accept() failed on HTTP socket: %s", strerror(errno));
            }
        }

        /* Accept connection on Unix socket */
        if (unix_fd >= 0 && FD_ISSET(unix_fd, &read_fds)) {
            struct sockaddr_un client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(unix_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd >= 0) {
                /* Spawn thread to handle client */
                pthread_t thread;
                if (pthread_create(&thread, NULL, client_thread, (void*)(intptr_t)client_fd) != 0) {
                    LOG_ERROR("Failed to create client thread");
                    close(client_fd);
                } else {
                    pthread_detach(thread);
                }
            } else if (errno != EINTR) {
                LOG_ERROR("accept() failed on Unix socket: %s", strerror(errno));
            }
        }
    }

    /* Cleanup */
    LOG_INFO("HTTP daemon shutting down...");
    close(server_fd);
    if (unix_fd >= 0) {
        close(unix_fd);
        if (config->socket_path) {
            unlink(config->socket_path);
        }
    }
    katra_unified_shutdown();

    return result;
}
