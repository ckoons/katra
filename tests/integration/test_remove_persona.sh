#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Integration test for remove-persona.sh script
# Tests persona removal with memory cleanup

set -e

# Test configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
REMOVE_SCRIPT="$PROJECT_ROOT/scripts/remove-persona.sh"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Logging
log_test() {
    echo -e "${YELLOW}TEST:${NC} $1"
    TESTS_RUN=$((TESTS_RUN + 1))
}

log_pass() {
    echo -e "${GREEN}PASS:${NC} $1"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

log_fail() {
    echo -e "${RED}FAIL:${NC} $1"
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

# Setup test environment
setup() {
    # Use temporary KATRA_HOME for testing
    export KATRA_HOME="/tmp/test_remove_persona_$$"
    mkdir -p "$KATRA_HOME"
    mkdir -p "$KATRA_HOME/personas"
    mkdir -p "$KATRA_HOME/memory/tier1"
    mkdir -p "$KATRA_HOME/memory/tier2"
    mkdir -p "$KATRA_HOME/memory/tier3"

    # Create test personas.json
    cat > "$KATRA_HOME/personas.json" <<'EOF'
{
  "version": "1.0",
  "personas": {
    "TestPersona": {
      "name": "TestPersona",
      "ci_id": "test-001",
      "created_at": "2025-01-01T00:00:00Z"
    },
    "TestPersona2": {
      "name": "TestPersona2",
      "ci_id": "test-002",
      "created_at": "2025-01-01T00:00:00Z"
    }
  }
}
EOF

    # Create test persona directory and data
    mkdir -p "$KATRA_HOME/personas/TestPersona/config"
    echo '{"name":"TestPersona"}' > "$KATRA_HOME/personas/TestPersona/config/persona.json"

    # Create test memories
    mkdir -p "$KATRA_HOME/memory/tier1/test-001"
    mkdir -p "$KATRA_HOME/memory/tier2/test-001"
    echo "test memory" > "$KATRA_HOME/memory/tier1/test-001/memory.txt"
    echo "test memory" > "$KATRA_HOME/memory/tier2/test-001/memory.txt"
}

# Cleanup test environment
cleanup() {
    if [ -d "$KATRA_HOME" ]; then
        rm -rf "$KATRA_HOME"
    fi
}

# Test: Script exists and is executable
test_script_exists() {
    log_test "Script exists and is executable"

    if [ -x "$REMOVE_SCRIPT" ]; then
        log_pass "Script found and executable"
    else
        log_fail "Script not found or not executable: $REMOVE_SCRIPT"
    fi
}

# Test: Help output works
test_help_output() {
    log_test "Help output works"

    if "$REMOVE_SCRIPT" --help | grep -q "Remove a persona"; then
        log_pass "Help output displays correctly"
    else
        log_fail "Help output is missing or incorrect"
    fi
}

# Test: Dry run mode
test_dry_run() {
    log_test "Dry run mode (no changes)"

    setup

    # Run dry run
    "$REMOVE_SCRIPT" --dry-run --persona TestPersona

    # Verify nothing was deleted
    if [ -d "$KATRA_HOME/personas/TestPersona" ] && \
       [ -d "$KATRA_HOME/memory/tier1/test-001" ] && \
       [ -f "$KATRA_HOME/personas.json" ] && \
       jq -e '.personas.TestPersona' "$KATRA_HOME/personas.json" >/dev/null 2>&1; then
        log_pass "Dry run made no changes"
    else
        log_fail "Dry run modified data"
    fi

    cleanup
}

# Test: Remove persona with force flag
test_remove_with_force() {
    log_test "Remove persona with --force flag"

    setup

    # Remove persona
    "$REMOVE_SCRIPT" --force --persona TestPersona

    # Verify persona was removed
    if [ ! -d "$KATRA_HOME/personas/TestPersona" ] && \
       [ ! -d "$KATRA_HOME/memory/tier1/test-001" ] && \
       [ ! -d "$KATRA_HOME/memory/tier2/test-001" ] && \
       ! jq -e '.personas.TestPersona' "$KATRA_HOME/personas.json" >/dev/null 2>&1; then
        log_pass "Persona and memories removed"
    else
        log_fail "Persona or memories still exist"
    fi

    # Verify other persona was NOT removed
    if jq -e '.personas.TestPersona2' "$KATRA_HOME/personas.json" >/dev/null 2>&1; then
        log_pass "Other personas preserved"
    else
        log_fail "Other personas were incorrectly removed"
    fi

    cleanup
}

# Test: Audit log entry created
test_audit_log() {
    log_test "Audit log entry created"

    setup

    # Remove persona
    "$REMOVE_SCRIPT" --force --persona TestPersona

    # Check audit log
    if [ -f "$KATRA_HOME/audit.jsonl" ] && \
       grep -q "persona_removed" "$KATRA_HOME/audit.jsonl" && \
       grep -q "TestPersona" "$KATRA_HOME/audit.jsonl"; then
        log_pass "Audit log entry created"
    else
        log_fail "Audit log entry missing or incorrect"
    fi

    cleanup
}

# Test: Error handling - nonexistent persona
test_nonexistent_persona() {
    log_test "Error handling for nonexistent persona"

    setup

    # Try to remove nonexistent persona
    if ! "$REMOVE_SCRIPT" --force --persona NonExistent 2>&1 | grep -q "Persona not found"; then
        log_fail "Should report persona not found"
    else
        log_pass "Correctly reports nonexistent persona"
    fi

    cleanup
}

# Test: Persona without memory directories
test_no_memories() {
    log_test "Persona without memory directories"

    setup

    # Create persona without memories
    cat > "$KATRA_HOME/personas.json" <<'EOF'
{
  "version": "1.0",
  "personas": {
    "NoMemories": {
      "name": "NoMemories",
      "ci_id": "no-mem-001",
      "created_at": "2025-01-01T00:00:00Z"
    }
  }
}
EOF

    # Remove persona (should not fail)
    if "$REMOVE_SCRIPT" --force --persona NoMemories; then
        log_pass "Handles persona with no memories"
    else
        log_fail "Failed to handle persona with no memories"
    fi

    cleanup
}

# Main test execution
main() {
    echo "========================================"
    echo "Integration Test: remove-persona.sh"
    echo "========================================"
    echo ""

    # Run tests
    test_script_exists
    test_help_output
    test_dry_run
    test_remove_with_force
    test_audit_log
    test_nonexistent_persona
    test_no_memories

    # Report results
    echo ""
    echo "========================================"
    echo "Test Results"
    echo "========================================"
    echo "Tests run:    $TESTS_RUN"
    echo "Tests passed: $TESTS_PASSED"
    echo "Tests failed: $TESTS_FAILED"
    echo "========================================"

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}ALL TESTS PASSED${NC}"
        exit 0
    else
        echo -e "${RED}SOME TESTS FAILED${NC}"
        exit 1
    fi
}

# Run tests
main
