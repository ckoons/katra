/* © 2025 Casey Koons All rights reserved */

/* team_ui_mcp.c - MCP/TCP client for the team coordinator UI.
 *                 One short-lived connection per call, newline-framed
 *                 JSON-RPC to the katra daemon. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "team_ui.h"
#include "katra_error.h"

/* Open a blocking TCP connection to the daemon.
 * Returns a valid fd (>= 0) or -1 on failure (error already reported). */
static int team_ui_connect(const char* host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        katra_report_error(E_SYSTEM_IO, "team_ui_connect",
                           "socket() failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        katra_report_error(E_INPUT_INVALID, "team_ui_connect",
                           "invalid host address: %s", host);
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        katra_report_error(E_SYSTEM_IO, "team_ui_connect",
                           "connect to %s:%d failed: %s", host, port, strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

/* Write all bytes to fd. Returns KATRA_SUCCESS or E_SYSTEM_IO. */
static int team_ui_send_all(int fd, const char* buf, size_t len) {
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, buf + off, len - off);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            katra_report_error(E_SYSTEM_IO, "team_ui_send_all",
                               "write failed: %s", strerror(errno));
            return E_SYSTEM_IO;
        }
        off += (size_t)n;
    }
    return KATRA_SUCCESS;
}

/* Read one newline-framed response into buf (NUL-terminated).
 * Returns KATRA_SUCCESS, E_SYSTEM_IO, or E_IO_EOF if nothing was read. */
static int team_ui_read_line(int fd, char* buf, size_t bufsize) {
    size_t off = 0;
    while (off < bufsize - 1) {
        ssize_t n = read(fd, buf + off, bufsize - 1 - off);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            katra_report_error(E_SYSTEM_IO, "team_ui_read_line",
                               "read failed: %s", strerror(errno));
            return E_SYSTEM_IO;
        }
        if (n == 0) {
            break;  /* EOF */
        }
        off += (size_t)n;
        buf[off] = '\0';
        if (memchr(buf, '\n', off) != NULL) {
            break;
        }
    }
    buf[off] = '\0';
    if (off == 0) {
        katra_report_error(E_IO_EOF, "team_ui_read_line",
                           "no response from daemon");
        return E_IO_EOF;
    }
    return KATRA_SUCCESS;
}

int team_ui_mcp_call(const char* host, int port, const char* tool,
                     json_t* arguments, char** text_out, bool* is_error_out) {
    int result = KATRA_SUCCESS;
    int fd = -1;
    char* request = NULL;
    char* response = NULL;
    json_t* req = NULL;
    json_t* params = NULL;
    json_t* root = NULL;
    json_t* err = NULL;
    json_t* res = NULL;
    json_t* text_node = NULL;
    json_error_t jerr;

    if (!host || !tool || !text_out) {
        if (arguments) {
            json_decref(arguments);
        }
        return E_INPUT_NULL;
    }
    *text_out = NULL;
    if (is_error_out) {
        *is_error_out = false;
    }

    /* Build the JSON-RPC request. arguments (if any) is stolen below. */
    req = json_object();
    params = json_object();
    if (!req || !params) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    json_object_set_new(req, MCP_FIELD_JSONRPC, json_string(MCP_JSONRPC_VERSION));
    json_object_set_new(req, MCP_FIELD_ID, json_integer(TEAM_UI_RPC_ID));
    json_object_set_new(req, MCP_FIELD_METHOD, json_string(MCP_METHOD_TOOLS_CALL));
    json_object_set_new(params, MCP_FIELD_NAME, json_string(tool));
    json_object_set_new(params, MCP_FIELD_ARGUMENTS,
                        arguments ? arguments : json_object());
    json_object_set_new(req, MCP_FIELD_PARAMS, params);
    params = NULL;      /* now owned by req */
    arguments = NULL;   /* now owned by req */

    request = json_dumps(req, JSON_COMPACT);
    if (!request) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    response = malloc(MCP_MAX_LINE + 1);
    if (!response) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    fd = team_ui_connect(host, port);
    if (fd < 0) {
        result = E_SYSTEM_IO;
        goto cleanup;
    }

    result = team_ui_send_all(fd, request, strlen(request));
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }
    result = team_ui_send_all(fd, "\n", 1);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    result = team_ui_read_line(fd, response, MCP_MAX_LINE + 1);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    root = json_loads(response, 0, &jerr);
    if (!root) {
        katra_report_error(E_INPUT_FORMAT, "team_ui_mcp_call",
                           "invalid JSON response: %s", jerr.text);
        result = E_INPUT_FORMAT;
        goto cleanup;
    }

    /* Protocol-level error: error.message */
    err = json_object_get(root, MCP_FIELD_ERROR);
    if (json_is_object(err)) {
        const char* msg = json_string_value(json_object_get(err, MCP_FIELD_MESSAGE));
        *text_out = strdup(msg ? msg : "");
        if (is_error_out) {
            *is_error_out = true;
        }
        result = (*text_out) ? KATRA_SUCCESS : E_SYSTEM_MEMORY;
        goto cleanup;
    }

    /* Tool result: result.content[0].text, result.isError */
    res = json_object_get(root, MCP_FIELD_RESULT);
    text_node = json_object_get(
        json_array_get(json_object_get(res, MCP_FIELD_CONTENT), 0),
        MCP_FIELD_TEXT);
    if (is_error_out) {
        *is_error_out = json_is_true(json_object_get(res, MCP_FIELD_IS_ERROR));
    }

    *text_out = strdup(json_string_value(text_node) ? json_string_value(text_node) : "");
    if (!*text_out) {
        result = E_SYSTEM_MEMORY;
    }

cleanup:
    if (fd >= 0) {
        close(fd);
    }
    if (arguments) {
        json_decref(arguments);
    }
    if (params) {
        json_decref(params);
    }
    if (req) {
        json_decref(req);
    }
    if (root) {
        json_decref(root);
    }
    free(request);
    free(response);
    return result;
}
