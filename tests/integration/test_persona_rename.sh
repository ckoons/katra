#!/bin/bash
# © 2025 Casey Koons All rights reserved

#
# test_persona_rename.sh - Integration test for persona rename script
#
# Tests the scripts/rename_persona.sh script end-to-end:
# - Data preservation across rename
# - Dry-run mode
# - Conflict handling
# - personas.json updates
# - CI ID continuity
# - Audit logging
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Test tracking
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Test setup
TEST_DIR="/tmp/katra_test_rename_$$"
KATRA_HOME="${TEST_DIR}/.katra"
PERSONAS_JSON="${KATRA_HOME}/personas.json"
RENAME_SCRIPT="${TEST_DIR}/rename_persona_test.sh"

# Helper functions
info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

test_pass() {
    echo -e "${GREEN}  ✓${NC} $1"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

test_fail() {
    echo -e "${RED}  ✗${NC} $1"
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

# Setup test environment
setup_test_env() {
    info "Setting up test environment..."

    # Create test directory structure
    mkdir -p "${KATRA_HOME}/personas"
    mkdir -p "${TEST_DIR}"

    # Create patched rename script that respects KATRA_HOME environment variable
    sed 's|^KATRA_HOME="${HOME}/.katra"$|KATRA_HOME="${KATRA_HOME:-${HOME}/.katra}"|' \
        ./scripts/rename_persona.sh > "$RENAME_SCRIPT"
    chmod +x "$RENAME_SCRIPT"

    # Create initial personas.json
    cat > "$PERSONAS_JSON" << 'EOF'
{
  "personas": {
    "Alice": {
      "name": "Alice",
      "ci_id": "ci-alice-001",
      "created": "2025-01-15T10:00:00Z"
    },
    "Bob": {
      "name": "Bob",
      "ci_id": "ci-bob-001",
      "created": "2025-01-15T11:00:00Z"
    }
  }
}
EOF

    # Create persona directories with sample data
    mkdir -p "${KATRA_HOME}/personas/Alice/config"
    mkdir -p "${KATRA_HOME}/personas/Alice/memory"
    mkdir -p "${KATRA_HOME}/personas/Bob/config"

    # Create Alice's config file
    cat > "${KATRA_HOME}/personas/Alice/config/persona.json" << 'EOF'
{
  "name": "Alice",
  "ci_id": "ci-alice-001",
  "personality": "helpful"
}
EOF

    # Create Alice's memory file
    echo "Sample memory data" > "${KATRA_HOME}/personas/Alice/memory/test.txt"

    info "Test environment created at: $TEST_DIR"
}

# Cleanup test environment
cleanup_test_env() {
    if [ -d "$TEST_DIR" ]; then
        rm -rf "$TEST_DIR"
        info "Cleaned up test environment"
    fi
}

# Test 1: Basic rename operation
test_basic_rename() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 1: Basic rename operation"

    # Rename Alice to Alicia
    KATRA_HOME="$KATRA_HOME" "$RENAME_SCRIPT" --from Alice --to Alicia > /dev/null 2>&1

    # Check directory was renamed
    if [ -d "${KATRA_HOME}/personas/Alicia" ]; then
        test_pass "Directory renamed successfully"
    else
        test_fail "Directory not renamed"
    fi

    # Check old directory no longer exists
    if [ ! -d "${KATRA_HOME}/personas/Alice" ]; then
        test_pass "Old directory removed"
    else
        test_fail "Old directory still exists"
    fi

    # Check personas.json updated
    if grep -q '"Alicia"' "$PERSONAS_JSON"; then
        test_pass "personas.json contains new name"
    else
        test_fail "personas.json not updated"
    fi

    # Check old name removed from personas.json
    if ! grep -q '"Alice"' "$PERSONAS_JSON"; then
        test_pass "Old name removed from personas.json"
    else
        test_fail "Old name still in personas.json"
    fi

    # Check CI ID preserved
    if command -v jq >/dev/null 2>&1; then
        local ci_id=$(jq -r '.personas.Alicia.ci_id' "$PERSONAS_JSON")
        if [ "$ci_id" = "ci-alice-001" ]; then
            test_pass "CI ID preserved: $ci_id"
        else
            test_fail "CI ID changed or missing: $ci_id"
        fi
    fi

    # Check config file updated
    if [ -f "${KATRA_HOME}/personas/Alicia/config/persona.json" ]; then
        if grep -q '"Alicia"' "${KATRA_HOME}/personas/Alicia/config/persona.json"; then
            test_pass "Config file name updated"
        else
            test_fail "Config file name not updated"
        fi
    fi

    # Check memory preserved
    if [ -f "${KATRA_HOME}/personas/Alicia/memory/test.txt" ]; then
        test_pass "Memory data preserved"
    else
        test_fail "Memory data lost"
    fi
}

# Test 2: Dry-run mode
test_dry_run() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 2: Dry-run mode"

    # Clean setup
    cleanup_test_env
    setup_test_env

    # Dry-run rename
    KATRA_HOME="$KATRA_HOME" "$RENAME_SCRIPT" --from Alice --to Alicia --dry-run > /dev/null 2>&1

    # Check directory NOT renamed
    if [ -d "${KATRA_HOME}/personas/Alice" ]; then
        test_pass "Original directory still exists"
    else
        test_fail "Original directory was modified"
    fi

    # Check new directory NOT created
    if [ ! -d "${KATRA_HOME}/personas/Alicia" ]; then
        test_pass "New directory not created"
    else
        test_fail "New directory created in dry-run"
    fi

    # Check personas.json NOT modified
    if grep -q '"Alice"' "$PERSONAS_JSON"; then
        test_pass "personas.json not modified"
    else
        test_fail "personas.json was modified"
    fi
}

# Test 3: Error handling - missing persona
test_missing_persona() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 3: Error handling - missing persona"

    # Clean setup
    cleanup_test_env
    setup_test_env

    # Try to rename non-existent persona
    if ! KATRA_HOME="$KATRA_HOME" "$RENAME_SCRIPT" --from NonExistent --to NewName > /dev/null 2>&1; then
        test_pass "Correctly rejects missing persona"
    else
        test_fail "Should have failed for missing persona"
    fi
}

# Test 4: Error handling - duplicate target name
test_duplicate_target() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 4: Error handling - duplicate target name"

    # Clean setup
    cleanup_test_env
    setup_test_env

    # Try to rename to existing persona
    if ! KATRA_HOME="$KATRA_HOME" "$RENAME_SCRIPT" --from Alice --to Bob > /dev/null 2>&1; then
        test_pass "Correctly rejects duplicate target name"
    else
        test_fail "Should have failed for duplicate target"
    fi

    # Verify Alice unchanged
    if [ -d "${KATRA_HOME}/personas/Alice" ]; then
        test_pass "Original persona unchanged"
    else
        test_fail "Original persona was modified"
    fi
}

# Test 5: Error handling - same name
test_same_name() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 5: Error handling - same name"

    # Clean setup
    cleanup_test_env
    setup_test_env

    # Try to rename to same name
    if ! KATRA_HOME="$KATRA_HOME" "$RENAME_SCRIPT" --from Alice --to Alice > /dev/null 2>&1; then
        test_pass "Correctly rejects same name"
    else
        test_fail "Should have failed for same name"
    fi
}

# Test 6: Audit logging
test_audit_logging() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 6: Audit logging"

    # Clean setup
    cleanup_test_env
    setup_test_env

    # Perform rename
    KATRA_HOME="$KATRA_HOME" "$RENAME_SCRIPT" --from Alice --to Alicia > /dev/null 2>&1

    # Check audit file created
    local audit_file="${KATRA_HOME}/audit.jsonl"
    if [ -f "$audit_file" ]; then
        test_pass "Audit file created"

        # Check audit entry contains required fields
        if grep -q '"persona_renamed"' "$audit_file"; then
            test_pass "Audit entry contains action"
        else
            test_fail "Audit entry missing action"
        fi

        if grep -q '"ci-alice-001"' "$audit_file"; then
            test_pass "Audit entry contains CI ID"
        else
            test_fail "Audit entry missing CI ID"
        fi
    else
        test_fail "Audit file not created"
    fi
}

# Test 7: Persona with no directory (only in personas.json)
test_no_directory() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 7: Persona with no directory"

    # Clean setup
    cleanup_test_env
    setup_test_env

    # Remove Alice's directory but keep JSON entry
    rm -rf "${KATRA_HOME}/personas/Alice"

    # Rename should succeed (with warning)
    if KATRA_HOME="$KATRA_HOME" "$RENAME_SCRIPT" --from Alice --to Alicia > /dev/null 2>&1; then
        test_pass "Rename succeeds without directory"

        # Check personas.json still updated
        if grep -q '"Alicia"' "$PERSONAS_JSON"; then
            test_pass "personas.json updated despite missing directory"
        else
            test_fail "personas.json not updated"
        fi
    else
        test_fail "Rename failed without directory"
    fi
}

# Main test runner
main() {
    echo "========================================"
    echo "Persona Rename Integration Test Suite"
    echo "========================================"
    echo ""

    # Check dependencies
    if [ ! -f "./scripts/rename_persona.sh" ]; then
        error "Rename script not found: ./scripts/rename_persona.sh"
        exit 1
    fi

    if ! command -v jq >/dev/null 2>&1; then
        warn "jq not found - some tests may not run"
    fi

    # Setup
    setup_test_env

    # Run tests
    test_basic_rename
    test_dry_run
    test_missing_persona
    test_duplicate_target
    test_same_name
    test_audit_logging
    test_no_directory

    # Cleanup
    cleanup_test_env

    # Summary
    echo ""
    echo "========================================"
    echo "Test Summary"
    echo "========================================"
    echo "Tests run:    $TESTS_RUN"
    echo "Tests passed: $TESTS_PASSED"
    echo "Tests failed: $TESTS_FAILED"
    echo "========================================"

    if [ $TESTS_FAILED -eq 0 ]; then
        info "All tests passed!"
        exit 0
    else
        error "$TESTS_FAILED test(s) failed"
        exit 1
    fi
}

# Run tests
main
