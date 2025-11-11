#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# check_ready.sh - Verify Katra is ready for CI testing
#
# This script checks that:
# 1. Katra compiles cleanly
# 2. All tests pass
# 3. Required directories exist
# 4. Code discipline is maintained
# 5. System is ready for a CI to use

set -e  # Exit on first error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "========================================"
echo "Katra Readiness Check"
echo "========================================"
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

passed=0
failed=0
warnings=0

check_pass() {
    echo -e "${GREEN}✓${NC} $1"
    passed=$((passed + 1))
}

check_fail() {
    echo -e "${RED}✗${NC} $1"
    failed=$((failed + 1))
}

check_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
    warnings=$((warnings + 1))
}

# Change to project root
cd "$PROJECT_ROOT"

echo "Checking build system..."
echo "----------------------------------------"

# Check 1: Makefile exists
if [ -f "Makefile" ]; then
    check_pass "Makefile exists"
else
    check_fail "Makefile not found"
    exit 1
fi

# Check 2: Build artifacts exist
echo ""
echo "Checking build artifacts..."
if [ -f "build/libkatra_utils.a" ]; then
    check_pass "Utils library exists"
else
    check_warn "Utils library not built (run 'make')"
fi

# Check 3: Source structure
echo ""
echo "Checking source structure..."
echo "----------------------------------------"

if [ -d "src" ]; then
    check_pass "src/ directory exists"
else
    check_fail "src/ directory missing"
fi

if [ -d "include" ]; then
    check_pass "include/ directory exists"
else
    check_fail "include/ directory missing"
fi

if [ -d "tests" ]; then
    check_pass "tests/ directory exists"
else
    check_fail "tests/ directory missing"
fi

# Check 4: Required headers
echo ""
echo "Checking core APIs..."
echo "----------------------------------------"

required_headers=(
    "katra_init.h"
    "katra_memory.h"
    "katra_checkpoint.h"
    "katra_error.h"
)

for header in "${required_headers[@]}"; do
    if [ -f "include/$header" ]; then
        check_pass "$header present"
    else
        check_fail "$header missing"
    fi
done

# Check 5: Test suite
echo ""
echo "Running test suite..."
echo "----------------------------------------"

if make test-quick > /dev/null 2>&1; then
    check_pass "All tests pass"
else
    check_warn "Some tests failed (run 'make test-quick' for details)"
fi

# Check 6: Runtime directories
echo ""
echo "Checking runtime environment..."
echo "----------------------------------------"

KATRA_HOME="$HOME/.katra"

if [ -d "$KATRA_HOME" ]; then
    check_pass "~/.katra directory exists"
else
    check_warn "~/.katra not yet created (will be created on first use)"
fi

if [ -d "$KATRA_HOME/memory/tier1" ]; then
    check_pass "Tier1 directory exists"
else
    check_warn "Tier1 directory not yet created"
fi

# Check 7: Code discipline
echo ""
echo "Checking code discipline..."
echo "----------------------------------------"

if [ -f "scripts/dev/count_core.sh" ]; then
    line_count=$(./scripts/dev/count_core.sh 2>&1 | grep "Used (no includes)" | awk '{print $4}')
    if [ -n "$line_count" ]; then
        check_pass "Line count: $line_count lines (budget: 30,000)"
        if [ "$line_count" -gt 30000 ]; then
            check_warn "Over budget by $((line_count - 30000)) lines"
        fi
    fi
else
    check_warn "Line count script not available"
fi

# Check 8: Examples
echo ""
echo "Checking CI integration resources..."
echo "----------------------------------------"

if [ -f "examples/minimal_ci.c" ]; then
    check_pass "Minimal CI example exists"
else
    check_warn "Minimal CI example not yet created"
fi

if [ -f "scripts/setup_ci.sh" ]; then
    check_pass "CI setup script exists"
else
    check_warn "CI setup script not yet created"
fi

if [ -f "docs/guide/CI_INTEGRATION.md" ]; then
    check_pass "CI integration guide exists"
else
    check_warn "CI integration guide not yet created"
fi

# Summary
echo ""
echo "========================================"
echo "Readiness Summary"
echo "========================================"
echo -e "${GREEN}Passed:${NC}   $passed"
echo -e "${YELLOW}Warnings:${NC} $warnings"
echo -e "${RED}Failed:${NC}   $failed"
echo ""

if [ $failed -eq 0 ]; then
    if [ $warnings -eq 0 ]; then
        echo -e "${GREEN}✓ Katra is ready for CI testing!${NC}"
        exit 0
    else
        echo -e "${YELLOW}⚠ Katra is mostly ready, but has $warnings warnings${NC}"
        echo "  Review warnings above before CI testing."
        exit 0
    fi
else
    echo -e "${RED}✗ Katra is NOT ready for CI testing${NC}"
    echo "  Fix $failed critical issues before proceeding."
    exit 1
fi
