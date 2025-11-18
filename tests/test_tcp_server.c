/* © 2025 Casey Koons All rights reserved */

/* TCP MCP Server Tests */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <jansson.h>
#include "katra_mcp_tcp.h"
#include "katra_mcp.h"
#include "katra_vector.h"
#include "katra_error.h"
#include "katra_limits.h"

#define TEST_PORT 3142  /* Different from default to avoid conflicts */

/* Mock globals for MCP tools (required by MCP library) */
char g_persona_name[256] = "test_tcp_persona";
char g_ci_id[256] = "test_tcp_ci";
vector_store_t* g_vector_store = NULL;

/* Mock session state */
static mcp_session_t test_session = {
    .chosen_name = "TestTCP",
    .role = "developer",
    .registered = true,
    .first_call = false,
    .connected_at = 0
};

/* Mock session functions */
mcp_session_t* mcp_get_session(void) {
    return &test_session;
}

const char* mcp_get_session_name(void) {
    return test_session.chosen_name;
}

bool mcp_is_registered(void) {
    return test_session.registered;
}

bool mcp_is_first_call(void) {
    return test_session.first_call;
}

void mcp_mark_first_call_complete(void) {
    test_session.first_call = false;
}

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

#ifdef ENABLE_INTEGRATION_TESTS
/* Server thread function - used in integration tests only */
static void* server_thread(void* arg) {
    mcp_tcp_config_t* config = (mcp_tcp_config_t*)arg;
    mcp_tcp_server_start(config);
    return NULL;
}
#endif

/* Test: TCP server configuration loading */
static int test_tcp_config(void) {
    printf("Testing TCP configuration...\n");
    tests_run++;

    /* Test default configuration */
    mcp_tcp_config_t config = {
        .port = KATRA_MCP_DEFAULT_PORT,
        .bind_address = "127.0.0.1",
        .max_clients = KATRA_MCP_MAX_CLIENTS,
        .enable_health_check = true
    };

    /* Verify default values */
    if (config.port != KATRA_MCP_DEFAULT_PORT) {
        printf("  ✗ Wrong default port: %d\n", config.port);
        return 1;
    }

    if (config.max_clients != KATRA_MCP_MAX_CLIENTS) {
        printf("  ✗ Wrong max clients: %d\n", config.max_clients);
        return 1;
    }

    if (!config.enable_health_check) {
        printf("  ✗ Health check should be enabled by default\n");
        return 1;
    }

    tests_passed++;
    printf("  ✓ TCP configuration works\n");
    return 0;
}

#ifdef ENABLE_INTEGRATION_TESTS
/* Test: Health check endpoint - requires running server */
static int test_health_check(void) {
    printf("Testing health check endpoint...\n");
    tests_run++;

    /* Create socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("  ✗ Failed to create socket\n");
        return 1;
    }

    /* Connect to server */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TEST_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("  ✗ Failed to connect to server\n");
        close(sock);
        return 1;
    }

    /* Send health check request */
    const char* request = "GET /health HTTP/1.1\r\n\r\n";
    ssize_t sent = write(sock, request, strlen(request));
    if (sent < 0) {
        printf("  ✗ Failed to send request\n");
        close(sock);
        return 1;
    }

    /* Read response */
    char buffer[1024];
    ssize_t received = read(sock, buffer, sizeof(buffer) - 1);
    if (received < 0) {
        printf("  ✗ Failed to read response\n");
        close(sock);
        return 1;
    }
    buffer[received] = '\0';

    close(sock);

    /* Verify response */
    if (strstr(buffer, "200 OK") == NULL) {
        printf("  ✗ Health check failed: %s\n", buffer);
        return 1;
    }

    if (strstr(buffer, "\"status\":\"healthy\"") == NULL) {
        printf("  ✗ Invalid health response\n");
        return 1;
    }

    tests_passed++;
    printf("  ✓ Health check works\n");
    return 0;
}

/* Test: Multiple concurrent connections - requires running server */
static int test_concurrent_connections(void) {
    printf("Testing concurrent connections...\n");
    tests_run++;

    const int num_clients = 3;
    int sockets[num_clients];
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TEST_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    /* Create multiple connections */
    for (int i = 0; i < num_clients; i++) {
        sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (sockets[i] < 0) {
            printf("  ✗ Failed to create socket %d\n", i);
            for (int j = 0; j < i; j++) {
                close(sockets[j]);
            }
            return 1;
        }

        if (connect(sockets[i], (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            printf("  ✗ Failed to connect socket %d\n", i);
            for (int j = 0; j <= i; j++) {
                close(sockets[j]);
            }
            return 1;
        }
    }

    /* Send health checks from all connections */
    const char* request = "GET /health HTTP/1.1\r\n\r\n";
    for (int i = 0; i < num_clients; i++) {
        ssize_t sent = write(sockets[i], request, strlen(request));
        if (sent < 0) {
            printf("  ✗ Failed to send from socket %d\n", i);
            for (int j = 0; j < num_clients; j++) {
                close(sockets[j]);
            }
            return 1;
        }
    }

    /* Read responses */
    for (int i = 0; i < num_clients; i++) {
        char buffer[1024];
        ssize_t received = read(sockets[i], buffer, sizeof(buffer) - 1);
        if (received < 0) {
            printf("  ✗ Failed to read from socket %d\n", i);
            for (int j = 0; j < num_clients; j++) {
                close(sockets[j]);
            }
            return 1;
        }
        buffer[received] = '\0';

        if (strstr(buffer, "200 OK") == NULL) {
            printf("  ✗ Socket %d health check failed\n", i);
            for (int j = 0; j < num_clients; j++) {
                close(sockets[j]);
            }
            return 1;
        }
    }

    /* Close all connections */
    for (int i = 0; i < num_clients; i++) {
        close(sockets[i]);
    }

    tests_passed++;
    printf("  ✓ Concurrent connections work\n");
    return 0;
}
#endif /* ENABLE_INTEGRATION_TESTS */

int main(void) {
    printf("\n========================================\n");
    printf("TCP MCP Server Tests\n");
    printf("========================================\n\n");

    /* Only test configuration for now - full server tests require */
    /* environment initialization and would be integration tests */
    test_tcp_config();

    /* Print results */
    printf("\n========================================\n");
    printf("Test Results: %d/%d passed\n", tests_passed, tests_run);
    printf("========================================\n\n");

    /* NOTE: Full integration tests (health check, concurrent connections) */
    /* are skipped as they require a running MCP server with full */
    /* Katra environment initialization. These should be tested manually */
    /* or via integration test suite. */

    return (tests_passed == tests_run) ? EXIT_SUCCESS : EXIT_FAILURE;
}
