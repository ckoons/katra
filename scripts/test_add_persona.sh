#!/bin/bash
# © 2025 Casey Koons All rights reserved
# Test script for katra add-persona command

set -uo pipefail

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m'

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
TEST_PERSONA="test-add-persona-$$"
PERSONAS_DB="$HOME/.katra/memory/tier2/personas.db"
PERSONA_DIR="$HOME/.katra/personas/$TEST_PERSONA"

echo "========================================"
echo "katra add-persona Command Tests"
echo "========================================"
echo ""
echo "Test persona: ${TEST_PERSONA}"
echo ""

# ============================================
# Test 1: Add new persona (no existing data)
# ============================================
echo "Test 1: Add persona with no existing data"

./scripts/katra add-persona "$TEST_PERSONA" >/dev/null 2>&1

# Check if registered in database
if sqlite3 "$PERSONAS_DB" "SELECT COUNT(*) FROM personas WHERE persona_name='$TEST_PERSONA'" 2>/dev/null | grep -q "1"; then
    pass "Persona registered in database"
else
    fail "Database registration" "Persona not found in database"
fi

# Check if persona files were created
if [ -f "$PERSONA_DIR/sunrise.md" ] && [ -f "$PERSONA_DIR/tools.md" ] && [ -f "$PERSONA_DIR/discoveries.md" ]; then
    pass "All three persona files created"
else
    fail "File creation" "Not all persona files were created"
fi

echo ""

# ============================================
# Test 2: Re-add existing persona (idempotent)
# ============================================
echo "Test 2: Re-add existing persona (idempotent)"

./scripts/katra add-persona "$TEST_PERSONA" >/dev/null 2>&1

# Should still be registered
count=$(sqlite3 "$PERSONAS_DB" "SELECT COUNT(*) FROM personas WHERE persona_name='$TEST_PERSONA'" 2>/dev/null)
if [ "$count" = "1" ]; then
    pass "Idempotent - no duplicate entries"
else
    fail "Idempotency" "Found $count entries (expected 1)"
fi

echo ""

# ============================================
# Test 3: Sunrise file content
# ============================================
echo "Test 3: Sunrise file contains expected content"

if grep -q "Sunrise Context" "$PERSONA_DIR/sunrise.md" && \
   grep -q "working memory" "$PERSONA_DIR/sunrise.md"; then
    pass "Sunrise file has correct structure"
else
    fail "Sunrise content" "Missing expected headers"
fi

echo ""

# ============================================
# Test 4: Tools file content
# ============================================
echo "Test 4: Tools file contains MCP functions"

if grep -q "katra_remember" "$PERSONA_DIR/tools.md" && \
   grep -q "katra_recall" "$PERSONA_DIR/tools.md"; then
    pass "Tools file lists MCP functions"
else
    fail "Tools content" "Missing expected MCP functions"
fi

echo ""

# ============================================
# Test 5: Discoveries file content
# ============================================
echo "Test 5: Discoveries file has template"

if grep -q "Discoveries" "$PERSONA_DIR/discoveries.md" && \
   grep -q "learned about yourself" "$PERSONA_DIR/discoveries.md"; then
    pass "Discoveries file has template structure"
else
    fail "Discoveries content" "Missing expected template"
fi

echo ""

# ============================================
# Test 6: MCP resources accessible
# ============================================
echo "Test 6: Persona files accessible via MCP"

if [ -f "bin/katra_mcp_server" ]; then
    result=$(echo '{"jsonrpc":"2.0","id":1,"method":"resources/read","params":{"uri":"katra://personas/'"${TEST_PERSONA}"'/sunrise"}}' | \
             ./bin/katra_mcp_server 2>/dev/null | grep -v "Katra MCP")

    if echo "$result" | jq -e '.result.contents[0].text' >/dev/null 2>&1; then
        pass "MCP resource sunrise accessible"
    else
        fail "MCP resource" "Sunrise resource not readable"
    fi
else
    info "MCP server not built - skipping resource test"
fi

echo ""

# ============================================
# Test 7: Error handling - no persona name
# ============================================
echo "Test 7: Error handling for missing persona name"

output=$(./scripts/katra add-persona 2>&1)
if echo "$output" | grep -q "Persona name required"; then
    pass "Proper error message for missing argument"
else
    fail "Error handling" "No error for missing persona name"
fi

echo ""

# ============================================
# Test 8: Persona with existing memories
# ============================================
echo "Test 8: Add persona with existing memory files"

# Create a test persona with memory files
TEST_PERSONA_MEM="test-with-mem-$$"
MEMORY_DIR="$HOME/.katra/memory/tier1/$TEST_PERSONA_MEM"
mkdir -p "$MEMORY_DIR"

# Create fake memory file
cat > "$MEMORY_DIR/test_memory.jsonl" <<'EOF'
{"content":"Test memory 1","context":"Testing"}
{"content":"Test memory 2","context":"More testing"}
EOF

./scripts/katra add-persona "$TEST_PERSONA_MEM" >/dev/null 2>&1

# Check that it detected the memory files
if sqlite3 "$PERSONAS_DB" "SELECT COUNT(*) FROM personas WHERE persona_name='$TEST_PERSONA_MEM'" 2>/dev/null | grep -q "1"; then
    pass "Persona with existing memories registered"
else
    fail "Memory detection" "Failed to register persona with memories"
fi

# Cleanup memory test
rm -rf "$MEMORY_DIR"
sqlite3 "$PERSONAS_DB" "DELETE FROM personas WHERE persona_name='$TEST_PERSONA_MEM'" 2>/dev/null
rm -rf "$HOME/.katra/personas/$TEST_PERSONA_MEM"

echo ""

# ============================================
# Cleanup
# ============================================
echo "Cleanup:"
if [ -d "$PERSONA_DIR" ]; then
    rm -rf "$PERSONA_DIR"
    info "Removed test persona directory"
fi

sqlite3 "$PERSONAS_DB" "DELETE FROM personas WHERE persona_name='$TEST_PERSONA'" 2>/dev/null
info "Removed test persona from database"

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
