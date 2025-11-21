#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Automated test suite for katra-cli
#
# Usage: ./tests/test_katra_cli.sh

set -euo pipefail

# Determine katra root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KATRA_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Test configuration
CLI="$KATRA_ROOT/bin/katra-cli"
MCP_SERVER="$KATRA_ROOT/bin/katra_mcp_server"
TEST_PORT="${KATRA_MCP_PORT:-3141}"  # Use existing port or default to 3141
TEST_LOG="/tmp/katra_cli_test_$$.log"
STATE_DIR="$HOME/.katra"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Cleanup handler
cleanup() {
    echo ""
    echo "Cleanup..."

    # Stop MCP server only if we started it
    if [ -n "${MCP_PID:-}" ]; then
        kill $MCP_PID 2>/dev/null || true
        echo -e "  ${GREEN}✓${NC} Stopped test MCP server"
    else
        echo -e "  ${BLUE}→${NC} Left existing MCP server running"
    fi

    # Clean state
    rm -f "$STATE_DIR/.human_"*
    echo -e "  ${GREEN}✓${NC} Cleaned state"

    # Remove log
    rm -f "$TEST_LOG"
}

trap cleanup EXIT

# Test helper functions
run_test() {
    local test_name="$1"
    local test_cmd="$2"
    local expected_pattern="$3"

    TESTS_RUN=$((TESTS_RUN + 1))

    # Run command and capture output (strip ANSI color codes for matching)
    local output
    local output_clean
    if output=$(eval "$test_cmd" 2>&1); then
        # Strip ANSI color codes for pattern matching
        output_clean=$(echo "$output" | sed 's/\x1b\[[0-9;]*m//g')

        # Check if output matches expected pattern
        if echo "$output_clean" | grep -q "$expected_pattern"; then
            echo -e "  ${GREEN}✓${NC} Test $TESTS_RUN: $test_name"
            TESTS_PASSED=$((TESTS_PASSED + 1))
            return 0
        else
            echo -e "  ${RED}✗${NC} Test $TESTS_RUN: $test_name (output mismatch)"
            echo "    Expected pattern: $expected_pattern"
            echo "    Got: $output_clean"
            TESTS_FAILED=$((TESTS_FAILED + 1))
            return 1
        fi
    else
        echo -e "  ${RED}✗${NC} Test $TESTS_RUN: $test_name (command failed)"
        echo "    Output: $output"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

# Main test execution
main() {
    echo "========================================"
    echo "Katra CLI Test Suite"
    echo "========================================"
    echo ""

    # Check prerequisites
    if [ ! -x "$CLI" ]; then
        echo -e "${RED}Error: katra-cli not found at $CLI${NC}"
        echo "Run 'make' to build"
        exit 1
    fi

    if [ ! -x "$MCP_SERVER" ]; then
        echo -e "${RED}Error: MCP server not found at $MCP_SERVER${NC}"
        echo "Run 'make' to build"
        exit 1
    fi

    if ! command -v jq &> /dev/null; then
        echo -e "${RED}Error: jq not installed${NC}"
        echo "Install with: brew install jq"
        exit 1
    fi

    # Setup
    echo "Setup..."

    # Clean state
    rm -f "$STATE_DIR/.human_"*
    echo -e "  ${GREEN}✓${NC} Cleaned state directory"

    # Check if server is already running on the port
    if lsof -Pi :$TEST_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
        echo -e "  ${BLUE}→${NC} Using existing TCP MCP server on port $TEST_PORT"
        MCP_PID=""  # Don't manage existing server
    else
        # Start TCP MCP server
        cd "$KATRA_ROOT"
        "$MCP_SERVER" --tcp --port $TEST_PORT > "$TEST_LOG" 2>&1 &
        MCP_PID=$!

        # Wait for server to be ready
        local count=0
        while [ $count -lt 10 ]; do
            if lsof -Pi :$TEST_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
                break
            fi
            sleep 0.5
            count=$((count + 1))
        done

        # Check if server started successfully
        if ! lsof -Pi :$TEST_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
            echo -e "${RED}Error: MCP server failed to start on port $TEST_PORT${NC}"
            if [ -f "$TEST_LOG" ]; then
                echo "Log output:"
                cat "$TEST_LOG"
            fi
            exit 1
        fi

        echo -e "  ${GREEN}✓${NC} Started TCP MCP server (PID: $MCP_PID)"
    fi

    # Set environment for tests
    export KATRA_MCP_PORT=$TEST_PORT
    export KATRA_MCP_HOST=localhost

    echo ""
    echo "Running tests..."

    # Test 1: Help command
    run_test "Help command" \
        "$CLI help" \
        "katra-cli - Human CLI"

    # Test 2: Register identity
    run_test "Register identity" \
        "$CLI register TestUser human" \
        "Registered as TestUser"

    # Test 3: Whoami query
    run_test "Whoami query" \
        "$CLI whoami" \
        "Name: TestUser"

    # Test 4: Who command
    run_test "Who command" \
        "$CLI who" \
        "TestUser"

    # Test 5: Say command
    run_test "Say command" \
        "$CLI say 'Test message from automated suite'" \
        "Message broadcast"

    # Test 6: Hear command (no messages expected)
    run_test "Hear command (no messages)" \
        "$CLI hear" \
        "No new messages"

    # Test 7: Status command
    run_test "Status command" \
        "$CLI status" \
        "SESSION:"

    # Test 8: Recent memories
    run_test "Recent memories (default)" \
        "$CLI recent" \
        "recent memories"

    # Test 9: Recent with limit
    run_test "Recent with --limit" \
        "$CLI recent --limit 10" \
        "recent memories"

    # Test 10: Recall search
    run_test "Recall search" \
        "$CLI recall session" \
        "memories"

    # Test 11: Team status
    run_test "Team status" \
        "$CLI team-status" \
        "Katra Team Status"

    # Test 12: Sessions list
    run_test "Sessions list" \
        "$CLI sessions" \
        "TCP mode"

    # Test 13: Multi-word message without quotes
    run_test "Multi-word message (no quotes)" \
        "$CLI say Testing multi word message" \
        "Message broadcast"

    # Results
    echo ""
    echo "========================================"
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}Results: $TESTS_PASSED/$TESTS_RUN tests passed (100%)${NC}"
        echo "========================================"
        exit 0
    else
        echo -e "${RED}Results: $TESTS_PASSED/$TESTS_RUN tests passed${NC}"
        echo -e "${RED}         $TESTS_FAILED tests failed${NC}"
        echo "========================================"
        exit 1
    fi
}

# Run tests
main
