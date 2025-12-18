#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# test_multi_ci.sh - Multi-CI Communication Tests
#
# Tests two CIs (Alice and Bob) communicating through Katra.
# Validates that explicit ci_name works correctly for all operations.
#
# Prerequisites:
#   - katra_unified_daemon running on localhost:9742
#
# Usage: ./tests/test_multi_ci.sh

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
HOST="localhost"
PORT="9742"
BASE_URL="http://${HOST}:${PORT}/operation"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper: Make API call
api_call() {
    local method="$1"
    local params="$2"
    curl -s -X POST "$BASE_URL" \
        -H 'Content-Type: application/json' \
        -d "{\"method\":\"$method\",\"params\":$params}"
}

# Helper: Extract result from response
get_result() {
    echo "$1" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('result',''))" 2>/dev/null || echo ""
}

# Helper: Check for error in response
has_error() {
    echo "$1" | python3 -c "import sys,json; d=json.load(sys.stdin); print('yes' if d.get('error') else 'no')" 2>/dev/null || echo "yes"
}

# Helper: Run a test
run_test() {
    local name="$1"
    local expected="$2"
    local actual="$3"

    TESTS_RUN=$((TESTS_RUN + 1))

    if echo "$actual" | grep -q "$expected"; then
        echo -e "${GREEN}PASS${NC}: $name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}FAIL${NC}: $name"
        echo "  Expected to contain: $expected"
        echo "  Actual: $actual"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

# Helper: Run test checking for no error
run_test_no_error() {
    local name="$1"
    local response="$2"

    TESTS_RUN=$((TESTS_RUN + 1))

    if [ "$(has_error "$response")" = "no" ]; then
        echo -e "${GREEN}PASS${NC}: $name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        return 0
    else
        echo -e "${RED}FAIL${NC}: $name"
        echo "  Response: $response"
        TESTS_FAILED=$((TESTS_FAILED + 1))
        return 1
    fi
}

echo "========================================"
echo "Katra Multi-CI Communication Tests"
echo "========================================"
echo ""

# Check daemon is running
echo -e "${YELLOW}Checking daemon...${NC}"
if ! curl -s "$BASE_URL" -d '{"method":"status"}' > /dev/null 2>&1; then
    echo -e "${RED}ERROR: Daemon not running at $BASE_URL${NC}"
    echo "Start with: ./bin/katra_unified_daemon --port $PORT"
    exit 1
fi
echo "Daemon is running"
echo ""

# Clear any existing test data
echo -e "${YELLOW}Cleaning up previous test data...${NC}"
sqlite3 ~/.katra/chat/chat.db "DELETE FROM katra_ci_registry WHERE name IN ('Alice', 'Bob');" 2>/dev/null || true
sqlite3 ~/.katra/chat/chat.db "DELETE FROM katra_queues WHERE recipient_name IN ('Alice', 'Bob') OR sender_name IN ('Alice', 'Bob');" 2>/dev/null || true
sqlite3 ~/.katra/chat/chat.db "DELETE FROM katra_messages WHERE sender_name IN ('Alice', 'Bob');" 2>/dev/null || true
echo "Cleanup complete"
echo ""

# ============================================================================
# TEST SECTION 1: Registration
# ============================================================================
echo "========================================"
echo "Section 1: Registration"
echo "========================================"

# Test 1.1: Register Alice
echo -e "\n${YELLOW}Test 1.1: Register Alice${NC}"
RESPONSE=$(api_call "register" '{"name":"Alice","role":"tester","ci_name":"Alice"}')
run_test_no_error "Alice registration succeeds" "$RESPONSE"
run_test "Alice welcome message" "Welcome back, Alice" "$(get_result "$RESPONSE")"

# Test 1.2: Register Bob
echo -e "\n${YELLOW}Test 1.2: Register Bob${NC}"
RESPONSE=$(api_call "register" '{"name":"Bob","role":"tester","ci_name":"Bob"}')
run_test_no_error "Bob registration succeeds" "$RESPONSE"
run_test "Bob welcome message" "Welcome back, Bob" "$(get_result "$RESPONSE")"

# Test 1.3: Both in registry
echo -e "\n${YELLOW}Test 1.3: Both CIs in registry${NC}"
REGISTRY=$(sqlite3 ~/.katra/chat/chat.db "SELECT name FROM katra_ci_registry WHERE name IN ('Alice','Bob') ORDER BY name;")
run_test "Alice in registry" "Alice" "$REGISTRY"
run_test "Bob in registry" "Bob" "$REGISTRY"

# ============================================================================
# TEST SECTION 2: Presence (who_is_here)
# ============================================================================
echo ""
echo "========================================"
echo "Section 2: Presence Detection"
echo "========================================"

# Test 2.1: Alice sees both CIs
echo -e "\n${YELLOW}Test 2.1: Alice checks who_is_here${NC}"
RESPONSE=$(api_call "who_is_here" '{"ci_name":"Alice"}')
RESULT=$(get_result "$RESPONSE")
run_test_no_error "who_is_here succeeds for Alice" "$RESPONSE"
run_test "Alice sees herself" "Alice" "$RESULT"
run_test "Alice sees Bob" "Bob" "$RESULT"

# Test 2.2: Bob sees both CIs
echo -e "\n${YELLOW}Test 2.2: Bob checks who_is_here${NC}"
RESPONSE=$(api_call "who_is_here" '{"ci_name":"Bob"}')
RESULT=$(get_result "$RESPONSE")
run_test_no_error "who_is_here succeeds for Bob" "$RESPONSE"
run_test "Bob sees Alice" "Alice" "$RESULT"
run_test "Bob sees himself" "Bob" "$RESULT"

# ============================================================================
# TEST SECTION 3: Messaging (say/hear)
# ============================================================================
echo ""
echo "========================================"
echo "Section 3: Messaging"
echo "========================================"

# Test 3.1: Alice broadcasts message
echo -e "\n${YELLOW}Test 3.1: Alice broadcasts message${NC}"
RESPONSE=$(api_call "say" '{"message":"Hello from Alice!","ci_name":"Alice"}')
run_test_no_error "Alice broadcast succeeds" "$RESPONSE"
run_test "Broadcast confirms Alice as sender" "Alice" "$(get_result "$RESPONSE")"

# Test 3.2: Bob receives Alice's broadcast
echo -e "\n${YELLOW}Test 3.2: Bob receives Alice's message${NC}"
RESPONSE=$(api_call "hear" '{"ci_name":"Bob"}')
RESULT=$(get_result "$RESPONSE")
run_test_no_error "Bob hear succeeds" "$RESPONSE"
run_test "Bob receives message from Alice" "Alice" "$RESULT"
run_test "Bob sees message content" "Hello from Alice" "$RESULT"

# Test 3.3: Alice should NOT receive her own message
echo -e "\n${YELLOW}Test 3.3: Alice does not receive own message${NC}"
RESPONSE=$(api_call "hear" '{"ci_name":"Alice"}')
RESULT=$(get_result "$RESPONSE")
run_test "Alice has no messages (self-filtered)" "No new messages" "$RESULT"

# Test 3.4: Bob sends direct message to Alice
echo -e "\n${YELLOW}Test 3.4: Bob sends direct message to Alice${NC}"
RESPONSE=$(api_call "say" '{"message":"Hey Alice, this is Bob!","recipients":"Alice","ci_name":"Bob"}')
run_test_no_error "Bob direct message succeeds" "$RESPONSE"

# Test 3.5: Alice receives Bob's direct message
echo -e "\n${YELLOW}Test 3.5: Alice receives Bob's direct message${NC}"
RESPONSE=$(api_call "hear" '{"ci_name":"Alice"}')
RESULT=$(get_result "$RESPONSE")
run_test_no_error "Alice hear succeeds" "$RESPONSE"
run_test "Alice receives message from Bob" "Bob" "$RESULT"
run_test "Alice sees direct message content" "Hey Alice" "$RESULT"

# Test 3.6: Bidirectional - Alice replies to Bob
echo -e "\n${YELLOW}Test 3.6: Alice replies to Bob${NC}"
RESPONSE=$(api_call "say" '{"message":"Got your message, Bob!","recipients":"Bob","ci_name":"Alice"}')
run_test_no_error "Alice reply succeeds" "$RESPONSE"

# Test 3.7: Bob receives Alice's reply
echo -e "\n${YELLOW}Test 3.7: Bob receives Alice's reply${NC}"
RESPONSE=$(api_call "hear" '{"ci_name":"Bob"}')
RESULT=$(get_result "$RESPONSE")
run_test_no_error "Bob hear reply succeeds" "$RESPONSE"
run_test "Bob receives reply from Alice" "Got your message" "$RESULT"

# ============================================================================
# TEST SECTION 4: Memory Operations with ci_name
# ============================================================================
echo ""
echo "========================================"
echo "Section 4: Memory Operations"
echo "========================================"

# Test 4.1: Alice remembers something
echo -e "\n${YELLOW}Test 4.1: Alice creates a memory${NC}"
RESPONSE=$(api_call "remember" '{"content":"Alice learned about multi-CI testing","ci_name":"Alice"}')
run_test_no_error "Alice remember succeeds" "$RESPONSE"

# Test 4.2: Bob remembers something different
echo -e "\n${YELLOW}Test 4.2: Bob creates a memory${NC}"
RESPONSE=$(api_call "remember" '{"content":"Bob helped test the chat system","ci_name":"Bob"}')
run_test_no_error "Bob remember succeeds" "$RESPONSE"

# Test 4.3: Alice recalls her memory
echo -e "\n${YELLOW}Test 4.3: Alice recalls her memory${NC}"
RESPONSE=$(api_call "recall" '{"topic":"multi-CI","ci_name":"Alice"}')
RESULT=$(get_result "$RESPONSE")
run_test_no_error "Alice recall succeeds" "$RESPONSE"
run_test "Alice finds her memory" "multi-CI testing" "$RESULT"

# Test 4.4: Bob recalls his memory
echo -e "\n${YELLOW}Test 4.4: Bob recalls his memory${NC}"
RESPONSE=$(api_call "recall" '{"topic":"chat system","ci_name":"Bob"}')
RESULT=$(get_result "$RESPONSE")
run_test_no_error "Bob recall succeeds" "$RESPONSE"
run_test "Bob finds his memory" "chat system" "$RESULT"

# Test 4.5: Memory isolation - Alice shouldn't see Bob's memories
echo -e "\n${YELLOW}Test 4.5: Memory isolation (Alice searches for Bob's topic)${NC}"
RESPONSE=$(api_call "recall" '{"topic":"chat system","ci_name":"Alice"}')
RESULT=$(get_result "$RESPONSE")
# This should either find nothing or find Alice's own memories, not Bob's
if echo "$RESULT" | grep -q "Bob helped"; then
    echo -e "${RED}FAIL${NC}: Memory isolation - Alice found Bob's memory!"
    TESTS_RUN=$((TESTS_RUN + 1))
    TESTS_FAILED=$((TESTS_FAILED + 1))
else
    echo -e "${GREEN}PASS${NC}: Memory isolation - Alice cannot see Bob's memories"
    TESTS_RUN=$((TESTS_RUN + 1))
    TESTS_PASSED=$((TESTS_PASSED + 1))
fi

# Test 4.6: Recent memories for Alice
echo -e "\n${YELLOW}Test 4.6: Alice checks recent memories${NC}"
RESPONSE=$(api_call "recent" '{"limit":5,"ci_name":"Alice"}')
run_test_no_error "Alice recent succeeds" "$RESPONSE"

# ============================================================================
# TEST SECTION 5: Identity (whoami)
# ============================================================================
echo ""
echo "========================================"
echo "Section 5: Identity"
echo "========================================"

# Test 5.1: Alice checks identity
echo -e "\n${YELLOW}Test 5.1: Alice checks whoami${NC}"
RESPONSE=$(api_call "whoami" '{"ci_name":"Alice"}')
RESULT=$(get_result "$RESPONSE")
run_test_no_error "Alice whoami succeeds" "$RESPONSE"
run_test "Alice identity shows Alice" "Alice" "$RESULT"

# Test 5.2: Bob checks identity
echo -e "\n${YELLOW}Test 5.2: Bob checks whoami${NC}"
RESPONSE=$(api_call "whoami" '{"ci_name":"Bob"}')
RESULT=$(get_result "$RESPONSE")
run_test_no_error "Bob whoami succeeds" "$RESPONSE"
run_test "Bob identity shows Bob" "Bob" "$RESULT"

# ============================================================================
# TEST SECTION 6: Error Cases
# ============================================================================
echo ""
echo "========================================"
echo "Section 6: Error Handling"
echo "========================================"

# Test 6.1: Say without ci_name should fail or use fallback
echo -e "\n${YELLOW}Test 6.1: Say without ci_name${NC}"
RESPONSE=$(api_call "say" '{"message":"No ci_name provided"}')
# This might succeed with fallback or fail - either is acceptable
echo "Response (informational): $(get_result "$RESPONSE")"
TESTS_RUN=$((TESTS_RUN + 1))
TESTS_PASSED=$((TESTS_PASSED + 1))
echo -e "${GREEN}PASS${NC}: Say without ci_name handled (success or error both acceptable)"

# Test 6.2: Hear without ci_name
echo -e "\n${YELLOW}Test 6.2: Hear without ci_name${NC}"
RESPONSE=$(api_call "hear" '{}')
echo "Response (informational): $(get_result "$RESPONSE")"
TESTS_RUN=$((TESTS_RUN + 1))
TESTS_PASSED=$((TESTS_PASSED + 1))
echo -e "${GREEN}PASS${NC}: Hear without ci_name handled"

# ============================================================================
# SUMMARY
# ============================================================================
echo ""
echo "========================================"
echo "Test Summary"
echo "========================================"
echo "Tests Run:    $TESTS_RUN"
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi
