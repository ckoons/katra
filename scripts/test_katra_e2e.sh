#!/bin/bash
# © 2025 Casey Koons All rights reserved
# End-to-end test for katra launch workflow

set -euo pipefail

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m'

TEST_PERSONA="e2e-test-$(date +%s)"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Katra Launch Workflow E2E Test${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Clean up any previous test persona
cleanup_test_persona() {
    local persona="$1"
    local db_path="$HOME/.katra/memory/tier2/personas.db"
    if [ -f "$db_path" ]; then
        sqlite3 "$db_path" "DELETE FROM personas WHERE persona_name='$persona'" 2>/dev/null || true
    fi
}

# Test 1: New Persona Launch
echo -e "${YELLOW}Test 1: New Persona Launch${NC}"
echo "Cleaning up any previous test data..."
cleanup_test_persona "$TEST_PERSONA"

echo "Testing: katra start --persona $TEST_PERSONA"
echo ""

# Create a modified version of katra that shows what it would do instead of executing
# We'll check if the persona is registered and prompt is generated
"$SCRIPT_DIR/katra" start --persona "$TEST_PERSONA" 2>&1 | head -20 &
KATRA_PID=$!

# Give it 2 seconds to start
sleep 2

# Kill the background process (we just wanted to see the startup messages)
kill $KATRA_PID 2>/dev/null || true
wait $KATRA_PID 2>/dev/null || true

echo ""
echo "Checking if persona was registered..."
if "$SCRIPT_DIR/../scripts/test_launch_helpers.sh" 2>&1 | grep -q "persona_exists.*$TEST_PERSONA"; then
    echo -e "${GREEN}✓ Persona registered in database${NC}"
else
    # Check directly
    if sqlite3 "$HOME/.katra/memory/tier2/personas.db" \
        "SELECT COUNT(*) FROM personas WHERE persona_name='$TEST_PERSONA'" 2>/dev/null | grep -q "1"; then
        echo -e "${GREEN}✓ Persona registered in database${NC}"
    else
        echo -e "${RED}✗ Persona NOT registered${NC}"
    fi
fi

echo ""

# Test 2: Verify Prompt Generation for New Persona
echo -e "${YELLOW}Test 2: Prompt Generation for New Persona${NC}"

# Source the helper functions
eval "$("$SCRIPT_DIR/test_launch_helpers.sh" 2>&1 | grep -A 200 "^persona_exists" | head -150)"

# Generate the prompt
if command -v generate_new_persona_prompt >/dev/null 2>&1; then
    PROMPT=$(generate_new_persona_prompt "$TEST_PERSONA")
    if [[ "$PROMPT" == *"Good morning, $TEST_PERSONA"* ]]; then
        echo -e "${GREEN}✓ New persona prompt generated correctly${NC}"
        echo "  First line: $(echo "$PROMPT" | head -1)"
    else
        echo -e "${RED}✗ New persona prompt incorrect${NC}"
    fi
else
    echo -e "${YELLOW}⚠ Cannot test prompt generation (function not sourced)${NC}"
fi

echo ""

# Test 3: Returning Persona Workflow
echo -e "${YELLOW}Test 3: Returning Persona Workflow${NC}"
echo "Persona $TEST_PERSONA should now exist in database"

# Check if it exists
if sqlite3 "$HOME/.katra/memory/tier2/personas.db" \
    "SELECT COUNT(*) FROM personas WHERE persona_name='$TEST_PERSONA'" 2>/dev/null | grep -q "1"; then
    echo -e "${GREEN}✓ Persona exists - ready for reclamation test${NC}"

    # Test reclamation prompt generation
    if command -v generate_reclamation_prompt >/dev/null 2>&1; then
        RECLAIM_PROMPT=$(generate_reclamation_prompt "$TEST_PERSONA")
        if [[ "$RECLAIM_PROMPT" == *"Welcome back, $TEST_PERSONA"* ]]; then
            echo -e "${GREEN}✓ Reclamation prompt generated correctly${NC}"
            echo "  First line: $(echo "$RECLAIM_PROMPT" | head -1)"
        else
            echo -e "${RED}✗ Reclamation prompt incorrect${NC}"
        fi
    fi
else
    echo -e "${RED}✗ Persona not found in database${NC}"
fi

echo ""

# Test 4: Dependency Checks
echo -e "${YELLOW}Test 4: Dependency Checks${NC}"
for cmd in sqlite3 jq claude; do
    if command -v $cmd >/dev/null 2>&1; then
        echo -e "${GREEN}✓ $cmd found${NC}"
    else
        echo -e "${RED}✗ $cmd NOT found${NC}"
    fi
done

echo ""

# Summary
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Test Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo "Test persona: $TEST_PERSONA"
echo ""
echo "To clean up:"
echo "  sqlite3 ~/.katra/memory/tier2/personas.db \"DELETE FROM personas WHERE persona_name='$TEST_PERSONA'\""
echo ""
echo -e "${GREEN}All tests completed${NC}"
