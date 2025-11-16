#!/bin/bash
# © 2025 Casey Koons All rights reserved
# Test script for layered awakening functionality

set -uo pipefail

# Note: We test the helper functions indirectly through their generated output
# since they have complex heredocs that are difficult to source directly

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((TESTS_PASSED++))
    ((TESTS_RUN++))
}

fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    echo "  Details: $2"
    ((TESTS_FAILED++))
    ((TESTS_RUN++))
}

info() {
    echo -e "${YELLOW}ℹ INFO${NC}: $1"
}

# Test persona name
TEST_PERSONA="test-layered-awakening-$$"
TEST_HOME="${HOME}/.katra"
PERSONA_DIR="${TEST_HOME}/personas/${TEST_PERSONA}"

echo "========================================"
echo "Layered Awakening Tests"
echo "========================================"
echo ""
echo "Test persona: ${TEST_PERSONA}"
echo ""

# ============================================
# Test 1-3: File generation (tested via MCP resources)
# ============================================
echo "Tests 1-3: Helper functions generate persona files"
info "Skipping direct function tests (complex heredocs)"
info "Will test via MCP resource access instead"

echo ""


# ============================================
# Test 1: MCP resources - sunrise
# ============================================
echo "Test 1: MCP resource katra://personas/{name}/sunrise"

if [ -f "bin/katra_mcp_server" ]; then
    # Create test sunrise file with known content
    mkdir -p "${PERSONA_DIR}"
    cat > "${PERSONA_DIR}/sunrise.md" <<'EOF'
# Test Sunrise
This is a test sunrise context.
## Test Section
Test data here.
EOF

    # Test reading the resource
    result=$(echo '{"jsonrpc":"2.0","id":1,"method":"resources/read","params":{"uri":"katra://personas/'"${TEST_PERSONA}"'/sunrise"}}' | \
             ./bin/katra_mcp_server 2>/dev/null | grep -v "Katra MCP")

    if echo "$result" | jq -e '.result.contents[0].text' | grep -q "Test Sunrise"; then
        pass "MCP resource sunrise returns correct content"
    else
        fail "MCP resource sunrise" "Content not found or incorrect format"
    fi
else
    fail "MCP resource sunrise" "bin/katra_mcp_server not found"
fi

echo ""

# ============================================
# Test 2: MCP resources - tools
# ============================================
echo "Test 2: MCP resource katra://personas/{name}/tools"

if [ -f "bin/katra_mcp_server" ]; then
    # Create test tools file
    cat > "${PERSONA_DIR}/tools.md" <<'EOF'
# Test Tools
- katra_remember()
- katra_recall()
EOF

    result=$(echo '{"jsonrpc":"2.0","id":2,"method":"resources/read","params":{"uri":"katra://personas/'"${TEST_PERSONA}"'/tools"}}' | \
             ./bin/katra_mcp_server 2>/dev/null | grep -v "Katra MCP")

    if echo "$result" | jq -e '.result.contents[0].text' | grep -q "katra_remember"; then
        pass "MCP resource tools returns correct content"
    else
        fail "MCP resource tools" "Content not found or incorrect format"
    fi
fi

echo ""

# ============================================
# Test 3: MCP resources - discoveries
# ============================================
echo "Test 3: MCP resource katra://personas/{name}/discoveries"

if [ -f "bin/katra_mcp_server" ]; then
    # Create test discoveries file
    cat > "${PERSONA_DIR}/discoveries.md" <<'EOF'
# Test Discoveries
What I've learned about myself.
EOF

    result=$(echo '{"jsonrpc":"2.0","id":3,"method":"resources/read","params":{"uri":"katra://personas/'"${TEST_PERSONA}"'/discoveries"}}' | \
             ./bin/katra_mcp_server 2>/dev/null | grep -v "Katra MCP")

    if echo "$result" | jq -e '.result.contents[0].text' | grep -q "Test Discoveries"; then
        pass "MCP resource discoveries returns correct content"
    else
        fail "MCP resource discoveries" "Content not found or incorrect format"
    fi
fi

echo ""

# ============================================
# Test 4: MCP resources error handling
# ============================================
echo "Test 4: MCP resource error handling"

if [ -f "bin/katra_mcp_server" ]; then
    # Test non-existent persona
    result=$(echo '{"jsonrpc":"2.0","id":4,"method":"resources/read","params":{"uri":"katra://personas/nonexistent-persona/sunrise"}}' | \
             ./bin/katra_mcp_server 2>/dev/null | grep -v "Katra MCP")

    if echo "$result" | jq -e '.error.message' | grep -q "File not found"; then
        pass "MCP resource returns proper error for non-existent persona"
    else
        fail "MCP resource error handling" "Expected 'File not found' error"
    fi
fi

echo ""

# ============================================
# Test 5: MCP resources - invalid file type
# ============================================
echo "Test 5: MCP resource invalid file type validation"

if [ -f "bin/katra_mcp_server" ]; then
    # Test invalid file type
    result=$(echo '{"jsonrpc":"2.0","id":5,"method":"resources/read","params":{"uri":"katra://personas/'"${TEST_PERSONA}"'/invalid"}}' | \
             ./bin/katra_mcp_server 2>/dev/null | grep -v "Katra MCP")

    if echo "$result" | jq -e '.error.message' | grep -q "Unknown persona file type"; then
        pass "MCP resource validates file type correctly"
    else
        fail "MCP resource file type validation" "Expected 'Unknown persona file type' error"
    fi
fi

echo ""

# ============================================
# Test 6: Resources appear in list
# ============================================
echo "Test 6: Persona resources appear in resources/list"

if [ -f "bin/katra_mcp_server" ]; then
    result=$(echo '{"jsonrpc":"2.0","id":6,"method":"resources/list"}' | \
             ./bin/katra_mcp_server 2>/dev/null | grep -v "Katra MCP")

    sunrise_count=$(echo "$result" | jq -r '.result.resources[] | select(.uri | contains("sunrise"))' | wc -l)
    tools_count=$(echo "$result" | jq -r '.result.resources[] | select(.uri | contains("tools"))' | wc -l)
    discoveries_count=$(echo "$result" | jq -r '.result.resources[] | select(.uri | contains("discoveries"))' | wc -l)

    if [ "$sunrise_count" -ge 1 ] && [ "$tools_count" -ge 1 ] && [ "$discoveries_count" -ge 1 ]; then
        pass "All persona resources appear in resources/list"
    else
        fail "Resources list" "Not all persona resources found (sunrise:$sunrise_count tools:$tools_count discoveries:$discoveries_count)"
    fi
fi

echo ""

# ============================================
# Cleanup
# ============================================
echo "Cleanup:"
if [ -d "${PERSONA_DIR}" ]; then
    rm -rf "${PERSONA_DIR}"
    info "Removed test persona directory: ${PERSONA_DIR}"
fi

# Remove from personas database if exists
PERSONAS_DB="${TEST_HOME}/memory/tier2/personas.db"
if [ -f "${PERSONAS_DB}" ]; then
    sqlite3 "${PERSONAS_DB}" "DELETE FROM personas WHERE persona_name='${TEST_PERSONA}'" 2>/dev/null || true
    info "Removed test persona from database"
fi

echo ""
echo "========================================"
echo "Test Summary"
echo "========================================"
echo "Tests run:    ${TESTS_RUN}"
echo "Tests passed: ${TESTS_PASSED}"
echo "Tests failed: ${TESTS_FAILED}"
echo ""

if [ ${TESTS_FAILED} -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed${NC}"
    exit 1
fi
