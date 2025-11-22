#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Test suite for katra command forwarding and integration
#
# Tests:
# 1. Command forwarding to katra-cli (say, hear, who, etc.)
# 2. Persona management forwarding (archive, rename, remove, restore)
# 3. Help text completeness

set -euo pipefail

# Determine katra root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KATRA_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Test configuration
KATRA="$KATRA_ROOT/scripts/katra"
TEST_PORT="${KATRA_MCP_PORT:-3141}"
TEST_LOG="/tmp/katra_commands_test_$$.log"

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
    rm -f ~/.katra/.human_*
    echo -e "  ${GREEN}✓${NC} Cleaned state"

    # Remove log
    rm -f "$TEST_LOG"
}

trap cleanup EXIT

# Test helper function
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
    echo "Katra Command Integration Test Suite"
    echo "========================================"
    echo ""

    # Check prerequisites
    if [ ! -x "$KATRA" ]; then
        echo -e "${RED}Error: katra script not found at $KATRA${NC}"
        exit 1
    fi

    # Setup
    echo "Setup..."

    # Clean state
    rm -f ~/.katra/.human_*
    echo -e "  ${GREEN}✓${NC} Cleaned state directory"

    # Check if server is already running on the port
    if lsof -Pi :$TEST_PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
        echo -e "  ${BLUE}→${NC} Using existing TCP MCP server on port $TEST_PORT"
        MCP_PID=""  # Don't manage existing server
    else
        # Start TCP MCP server
        cd "$KATRA_ROOT"
        "$KATRA_ROOT/bin/katra_mcp_server" --tcp --port $TEST_PORT > "$TEST_LOG" 2>&1 &
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

    echo ""
    echo "Running tests..."

    # ===================================================================
    # Test Group 1: Help and Unknown Commands
    # ===================================================================

    run_test "Help command" \
        "$KATRA help" \
        "Katra - Start Claude Code"

    run_test "Help shows persona management" \
        "$KATRA help" \
        "Persona Management:"

    run_test "Help shows human commands" \
        "$KATRA help" \
        "Human Commands"

    run_test "Unknown command returns error" \
        "$KATRA nonexistent-command 2>&1 || true" \
        "Unknown command:"

    # ===================================================================
    # Test Group 2: Human Participation Command Forwarding
    # ===================================================================

    run_test "Register command forwarding" \
        "$KATRA register TestUser human" \
        "Registered as TestUser"

    run_test "Whoami command forwarding" \
        "$KATRA whoami" \
        "Name: TestUser"

    run_test "Who command forwarding" \
        "$KATRA who" \
        "TestUser"

    run_test "Say command forwarding" \
        "$KATRA say 'Test message from katra'" \
        "Message broadcast"

    run_test "Hear command forwarding (no messages)" \
        "$KATRA hear" \
        "No new messages"

    run_test "Status command forwarding" \
        "$KATRA status" \
        "SESSION:"

    run_test "Recent command forwarding" \
        "$KATRA recent" \
        "recent memories"

    run_test "Recent with --limit forwarding" \
        "$KATRA recent --limit 5" \
        "recent memories"

    run_test "Recall command forwarding" \
        "$KATRA recall session" \
        "memories"

    run_test "Sessions command forwarding" \
        "$KATRA sessions" \
        "TCP mode"

    # ===================================================================
    # Test Group 3: Persona Management Command Forwarding
    # ===================================================================

    # Note: These tests just verify forwarding works, not full functionality
    # The underlying scripts have their own test suites

    run_test "Archive command exists and forwards" \
        "$KATRA archive --help 2>&1 || true" \
        "archive"

    run_test "Rename command exists and forwards" \
        "$KATRA rename --help 2>&1 || true" \
        "rename"

    run_test "Remove command exists and forwards" \
        "$KATRA remove --help 2>&1 || true" \
        "remove"

    run_test "Restore command exists and forwards" \
        "$KATRA restore --help 2>&1 || true" \
        "restore"

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
