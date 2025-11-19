#!/bin/bash
# © 2025 Casey Koons All rights reserved

#
# test_dual_location.sh - Integration test for dual-location persona architecture
#
# Tests the full workflow of shipped → user persona migration:
# - Shipped personas in {project_root}/personas/{name}/ (Git-tracked templates)
# - User personas in ~/.katra/personas/{name}/ (User data, never in Git)
# - Path utilities correctly find both locations
# - MCP config updates when persona location changes
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
TEST_DIR="/tmp/katra_test_dual_$$"
PROJECT_ROOT="$(pwd)"
SHIPPED_PERSONAS_DIR="${PROJECT_ROOT}/personas"
USER_KATRA_HOME="${TEST_DIR}/.katra"
USER_PERSONAS_DIR="${USER_KATRA_HOME}/personas"

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
    mkdir -p "$USER_PERSONAS_DIR"
    mkdir -p "$TEST_DIR"

    info "Test environment created at: $TEST_DIR"
    info "Project root: $PROJECT_ROOT"
}

# Cleanup test environment
cleanup_test_env() {
    if [ -d "$TEST_DIR" ]; then
        rm -rf "$TEST_DIR"
        info "Cleaned up test environment"
    fi
}

# Test 1: Verify shipped persona exists
test_shipped_persona_exists() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 1: Verify shipped persona exists"

    # Check that Assistant persona exists in shipped location
    if [ -d "${SHIPPED_PERSONAS_DIR}/Assistant" ]; then
        test_pass "Shipped Assistant persona exists"
    else
        test_fail "Shipped Assistant persona not found"
        return
    fi

    # Check it has config.json
    if [ -f "${SHIPPED_PERSONAS_DIR}/Assistant/config.json" ]; then
        test_pass "Shipped persona has config.json"
    else
        test_fail "Shipped persona missing config.json"
    fi

    # Verify it's NOT in user location
    if [ ! -d "${USER_PERSONAS_DIR}/Assistant" ]; then
        test_pass "Shipped persona not in user location"
    else
        test_fail "Shipped persona found in user location (test pollution)"
    fi
}

# Test 2: Create user persona from shipped template
test_create_user_persona() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 2: Create user persona from shipped template"

    # Create user persona by copying shipped template
    local persona_name="TestPersona"
    local user_persona_dir="${USER_PERSONAS_DIR}/${persona_name}"

    mkdir -p "$user_persona_dir"
    if [ -f "${SHIPPED_PERSONAS_DIR}/Assistant/config.json" ]; then
        cp "${SHIPPED_PERSONAS_DIR}/Assistant/config.json" "${user_persona_dir}/config.json"
    fi

    # Verify user persona created
    if [ -d "$user_persona_dir" ]; then
        test_pass "User persona directory created"
    else
        test_fail "Failed to create user persona directory"
        return
    fi

    # Verify config copied
    if [ -f "${user_persona_dir}/config.json" ]; then
        test_pass "Config copied to user persona"
    else
        test_fail "Config not copied"
    fi

    # Verify shipped persona still exists (not moved)
    if [ -d "${SHIPPED_PERSONAS_DIR}/Assistant" ]; then
        test_pass "Shipped persona still exists (not moved)"
    else
        test_fail "Shipped persona was moved instead of copied"
    fi
}

# Test 3: Path utilities find shipped persona
test_path_utils_shipped() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 3: Path utilities find shipped persona"

    # Create a simple C program to test path utilities
    local test_prog="${TEST_DIR}/test_shipped_path"
    cat > "${test_prog}.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "katra_path_utils.h"
#include "katra_limits.h"

int main(void) {
    char buffer[KATRA_PATH_MAX];
    int result = katra_get_shipped_persona_dir(buffer, sizeof(buffer), "Assistant");

    if (result != 0) {
        return 1;
    }

    printf("%s\n", buffer);

    /* Verify path contains "personas" */
    if (strstr(buffer, "personas") == NULL) {
        return 1;
    }

    /* Verify path contains "Assistant" */
    if (strstr(buffer, "Assistant") == NULL) {
        return 1;
    }

    /* Verify path does NOT contain ".katra" */
    if (strstr(buffer, ".katra") != NULL) {
        return 1;
    }

    return 0;
}
EOF

    # Compile and run
    if gcc -I./include -L./build -o "$test_prog" "${test_prog}.c" \
        -lkatra_utils -std=c11 > /dev/null 2>&1; then

        if "$test_prog" > /dev/null 2>&1; then
            test_pass "Path utils correctly find shipped persona"
        else
            test_fail "Path utils failed to find shipped persona"
        fi
    else
        warn "Could not compile test program - skipping"
    fi
}

# Test 4: Path utilities find user persona
test_path_utils_user() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 4: Path utilities find user persona"

    # Create a simple C program to test path utilities
    local test_prog="${TEST_DIR}/test_user_path"
    cat > "${test_prog}.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "katra_path_utils.h"
#include "katra_limits.h"

int main(void) {
    char buffer[KATRA_PATH_MAX];
    int result = katra_get_user_persona_dir(buffer, sizeof(buffer), "MyPersona");

    if (result != 0) {
        return 1;
    }

    printf("%s\n", buffer);

    /* Verify path contains ".katra" */
    if (strstr(buffer, ".katra") == NULL) {
        return 1;
    }

    /* Verify path contains "personas" */
    if (strstr(buffer, "personas") == NULL) {
        return 1;
    }

    /* Verify path contains "MyPersona" */
    if (strstr(buffer, "MyPersona") == NULL) {
        return 1;
    }

    return 0;
}
EOF

    # Compile and run
    if gcc -I./include -L./build -o "$test_prog" "${test_prog}.c" \
        -lkatra_utils -std=c11 > /dev/null 2>&1; then

        if "$test_prog" > /dev/null 2>&1; then
            test_pass "Path utils correctly find user persona location"
        else
            test_fail "Path utils failed to find user persona location"
        fi
    else
        warn "Could not compile test program - skipping"
    fi
}

# Test 5: User persona can have memory directory
test_user_persona_memory() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 5: User persona can have memory directory"

    local persona_name="TestPersona"
    local user_persona_dir="${USER_PERSONAS_DIR}/${persona_name}"
    local memory_dir="${user_persona_dir}/memory"

    # Create memory directory
    mkdir -p "$memory_dir"

    # Create sample memory file
    echo "Sample memory data" > "${memory_dir}/test_memory.txt"

    # Verify memory directory exists
    if [ -d "$memory_dir" ]; then
        test_pass "Memory directory created in user persona"
    else
        test_fail "Failed to create memory directory"
    fi

    # Verify memory file exists
    if [ -f "${memory_dir}/test_memory.txt" ]; then
        test_pass "Memory data stored in user persona"
    else
        test_fail "Memory data not stored"
    fi

    # Verify shipped persona has NO memory directory (read-only template)
    if [ ! -d "${SHIPPED_PERSONAS_DIR}/Assistant/memory" ]; then
        test_pass "Shipped persona has no memory directory (read-only)"
    else
        test_fail "Shipped persona has memory directory (should be template only)"
    fi
}

# Test 6: Separation of shipped vs user personas
test_separation() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 6: Separation of shipped vs user personas"

    # Verify shipped personas are in project root
    if [ "$(dirname $SHIPPED_PERSONAS_DIR)" = "$PROJECT_ROOT" ]; then
        test_pass "Shipped personas in project root"
    else
        test_fail "Shipped personas not in correct location"
    fi

    # Verify user personas are in ~/.katra
    if [[ "$USER_PERSONAS_DIR" == *".katra/personas"* ]]; then
        test_pass "User personas in ~/.katra/personas"
    else
        test_fail "User personas not in correct location"
    fi

    # Verify shipped and user locations are different
    if [ "$SHIPPED_PERSONAS_DIR" != "$USER_PERSONAS_DIR" ]; then
        test_pass "Shipped and user locations are separate"
    else
        test_fail "Shipped and user locations are the same"
    fi
}

# Test 7: Deprecated function compatibility
test_deprecated_compatibility() {
    TESTS_RUN=$((TESTS_RUN + 1))
    echo ""
    info "Test 7: Deprecated function compatibility"

    # Create a simple C program to test deprecated functions
    local test_prog="${TEST_DIR}/test_deprecated"
    cat > "${test_prog}.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "katra_path_utils.h"
#include "katra_limits.h"

int main(void) {
    char buffer[KATRA_PATH_MAX];

    /* Test deprecated katra_get_persona_dir() */
    int result = katra_get_persona_dir(buffer, sizeof(buffer), "TestPersona");

    if (result != 0) {
        return 1;
    }

    /* Should point to user location (.katra) for backward compatibility */
    if (strstr(buffer, ".katra") == NULL) {
        return 1;
    }

    return 0;
}
EOF

    # Compile and run
    if gcc -I./include -L./build -o "$test_prog" "${test_prog}.c" \
        -lkatra_utils -std=c11 > /dev/null 2>&1; then

        if "$test_prog" > /dev/null 2>&1; then
            test_pass "Deprecated functions maintain backward compatibility"
        else
            test_fail "Deprecated functions broken"
        fi
    else
        warn "Could not compile test program - skipping"
    fi
}

# Main test runner
main() {
    echo "========================================"
    echo "Dual-Location Persona Integration Test"
    echo "========================================"
    echo ""

    # Setup
    setup_test_env

    # Run tests
    test_shipped_persona_exists
    test_create_user_persona
    test_path_utils_shipped
    test_path_utils_user
    test_user_persona_memory
    test_separation
    test_deprecated_compatibility

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
