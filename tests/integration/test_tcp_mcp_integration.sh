#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# TCP MCP Server Integration Test Suite
#
# Tests all aspects of the TCP MCP server including:
# - Server start/stop/restart
# - Multiple concurrent clients
# - Health check endpoint
# - Environment variable configuration
# - Graceful shutdown with active clients

# Don't exit on error - we want to report all test failures
set +e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test configuration
TEST_PORT=3199
TEST_BIND="127.0.0.1"
PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
SERVER_BIN="$PROJECT_DIR/bin/katra_mcp_server"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Cleanup function
cleanup() {
    echo ""
    echo "Cleaning up..."
    # Kill any servers on test port
    pkill -9 -f "katra_mcp_server.*--port.*$TEST_PORT" 2>/dev/null || true
    pkill -9 -f "katra_mcp_server" 2>/dev/null || true
    sleep 2
}

# Trap cleanup on exit
trap cleanup EXIT

# Utility functions
pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((TESTS_PASSED++))
    ((TESTS_RUN++))
}

fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    if [ -n "$2" ]; then
        echo "  Details: $2"
    fi
    ((TESTS_FAILED++))
    ((TESTS_RUN++))
}

info() {
    echo -e "${YELLOW}ℹ${NC} $1"
}

# Wait for server to start
wait_for_server() {
    local port=$1
    local max_wait=${2:-5}
    local waited=0

    while [ $waited -lt $max_wait ]; do
        if curl -s "http://localhost:$port/health" >/dev/null 2>&1; then
            return 0
        fi
        sleep 1
        ((waited++))
    done

    return 1
}

# Check if server is running on port
is_server_running() {
    local port=$1
    lsof -i ":$port" >/dev/null 2>&1
}

#===============================================================================
# TEST SUITE
#===============================================================================

echo "========================================"
echo "TCP MCP Server Integration Test Suite"
echo "========================================"
echo ""
echo "Project: $PROJECT_DIR"
echo "Binary: $SERVER_BIN"
echo "Test port: $TEST_PORT"
echo ""

# Verify server binary exists
if [ ! -f "$SERVER_BIN" ]; then
    echo -e "${RED}Error: Server binary not found: $SERVER_BIN${NC}"
    echo "Run 'make' to build the server first"
    exit 1
fi

#===============================================================================
# Test 1: Server Start/Stop
#===============================================================================

info "Test 1: Server start/stop"

# Start server (redirect output to avoid clutter)
$SERVER_BIN --tcp --port $TEST_PORT >/dev/null 2>&1 &
SERVER_PID=$!
sleep 3

if is_server_running $TEST_PORT; then
    pass "Server started successfully on port $TEST_PORT"
else
    fail "Server failed to start" "PID $SERVER_PID not listening on port $TEST_PORT"
fi

# Stop server
kill $SERVER_PID 2>/dev/null
sleep 1

if ! is_server_running $TEST_PORT; then
    pass "Server stopped successfully"
else
    fail "Server did not stop" "Still listening on port $TEST_PORT"
    pkill -9 -f "katra_mcp_server.*$TEST_PORT"
fi

#===============================================================================
# Test 2: Health Check Endpoint
#===============================================================================

info "Test 2: Health check endpoint"

# Start server
$SERVER_BIN --tcp --port $TEST_PORT >/dev/null 2>&1 &
SERVER_PID=$!

if wait_for_server $TEST_PORT; then
    HEALTH_RESPONSE=$(curl -s "http://localhost:$TEST_PORT/health")

    if echo "$HEALTH_RESPONSE" | grep -q '"status":"healthy"'; then
        pass "Health check endpoint returns healthy status"
    else
        fail "Health check endpoint returned unexpected response" "$HEALTH_RESPONSE"
    fi

    if echo "$HEALTH_RESPONSE" | grep -q '"ok":true'; then
        pass "Health check endpoint includes ok:true"
    else
        fail "Health check endpoint missing ok:true" "$HEALTH_RESPONSE"
    fi
else
    fail "Server did not start for health check test"
fi

kill $SERVER_PID 2>/dev/null
sleep 1

#===============================================================================
# Test 3: Environment Variable Configuration
#===============================================================================

info "Test 3: Environment variable configuration"

# Start server with environment variables
KATRA_MCP_TCP_MODE=true \
KATRA_MCP_TCP_PORT=$TEST_PORT \
KATRA_MCP_TCP_BIND=$TEST_BIND \
$SERVER_BIN >/dev/null 2>&1 &
SERVER_PID=$!

if wait_for_server $TEST_PORT; then
    pass "Server respects KATRA_MCP_TCP_MODE environment variable"
    pass "Server respects KATRA_MCP_TCP_PORT environment variable"
else
    fail "Server did not start with environment variables"
fi

kill $SERVER_PID 2>/dev/null
sleep 1

#===============================================================================
# Test 4: Command-Line Override
#===============================================================================

info "Test 4: Command-line arguments override environment"

# Set conflicting environment variable
export KATRA_MCP_TCP_PORT=9999

# Start with command-line port (should override env)
$SERVER_BIN --tcp --port $TEST_PORT >/dev/null 2>&1 &
SERVER_PID=$!
sleep 1

if is_server_running $TEST_PORT; then
    pass "Command-line --port overrides KATRA_MCP_TCP_PORT"
else
    fail "Command-line port override failed" "Server not on port $TEST_PORT"
fi

# Verify server is NOT on env port
if ! is_server_running 9999; then
    pass "Server correctly ignored environment port"
else
    fail "Server incorrectly used environment port"
    pkill -9 -f "katra_mcp_server.*9999"
fi

kill $SERVER_PID 2>/dev/null
unset KATRA_MCP_TCP_PORT
sleep 1

#===============================================================================
# Test 5: Multiple Concurrent Clients
#===============================================================================

info "Test 5: Multiple concurrent clients"

# Start server
$SERVER_BIN --tcp --port $TEST_PORT >/dev/null 2>&1 &
SERVER_PID=$!

if wait_for_server $TEST_PORT; then
    # Spawn multiple health check requests concurrently
    for i in {1..10}; do
        curl -s "http://localhost:$TEST_PORT/health" >/dev/null &
    done

    # Wait for all requests to complete
    wait

    if is_server_running $TEST_PORT; then
        pass "Server handles multiple concurrent clients"
    else
        fail "Server crashed with concurrent clients"
    fi
else
    fail "Server did not start for concurrent client test"
fi

kill $SERVER_PID 2>/dev/null
sleep 1

#===============================================================================
# Test 6: Graceful Shutdown with Active Clients
#===============================================================================

info "Test 6: Graceful shutdown with active clients"

# Start server
$SERVER_BIN --tcp --port $TEST_PORT >/dev/null 2>&1 &
SERVER_PID=$!

if wait_for_server $TEST_PORT; then
    # Start a long-running client connection
    (sleep 2 && curl -s "http://localhost:$TEST_PORT/health" >/dev/null) &
    CLIENT_PID=$!

    # Send SIGTERM to server while client is connected
    kill -TERM $SERVER_PID 2>/dev/null
    sleep 2

    if ! is_server_running $TEST_PORT; then
        pass "Server shuts down gracefully with SIGTERM"
    else
        fail "Server did not respond to SIGTERM"
        kill -9 $SERVER_PID 2>/dev/null
    fi

    # Clean up client
    kill $CLIENT_PID 2>/dev/null || true
else
    fail "Server did not start for graceful shutdown test"
fi

#===============================================================================
# Test 7: Port Already in Use
#===============================================================================

info "Test 7: Port already in use handling"

# Start first server
$SERVER_BIN --tcp --port $TEST_PORT >/dev/null 2>&1 &
SERVER1_PID=$!

if wait_for_server $TEST_PORT; then
    # Try to start second server on same port
    $SERVER_BIN --tcp --port $TEST_PORT >/dev/null 2>&1 &
    SERVER2_PID=$!
    sleep 2

    # Check if second server failed to start
    if ! kill -0 $SERVER2_PID 2>/dev/null; then
        pass "Server correctly fails when port is in use"
    else
        fail "Server incorrectly started on occupied port"
        kill $SERVER2_PID 2>/dev/null
    fi

    kill $SERVER1_PID 2>/dev/null
else
    fail "First server did not start for port-in-use test"
fi

sleep 1

#===============================================================================
# Test 8: Server Restart
#===============================================================================

info "Test 8: Server restart"

# Start server
$SERVER_BIN --tcp --port $TEST_PORT >/dev/null 2>&1 &
SERVER1_PID=$!

if wait_for_server $TEST_PORT; then
    # Stop server
    kill $SERVER1_PID
    sleep 2

    # Restart server (SO_REUSEADDR should allow immediate restart)
    $SERVER_BIN --tcp --port $TEST_PORT >/dev/null 2>&1 &
    SERVER2_PID=$!

    if wait_for_server $TEST_PORT 3; then
        pass "Server can restart immediately (SO_REUSEADDR working)"
    else
        fail "Server failed to restart" "SO_REUSEADDR may not be set"
    fi

    kill $SERVER2_PID 2>/dev/null
else
    fail "Server did not start for restart test"
fi

#===============================================================================
# SUMMARY
#===============================================================================

echo ""
echo "========================================"
echo "Test Summary"
echo "========================================"
echo "Total tests: $TESTS_RUN"
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
echo -e "${RED}Failed: $TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed${NC}"
    exit 1
fi
