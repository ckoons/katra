#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# setup_ci.sh - Initialize Katra for a new Companion Intelligence
#
# This script helps a CI get started with Katra by:
# 1. Verifying Katra is installed and built
# 2. Creating necessary runtime directories
# 3. Optionally registering the CI identity
# 4. Verifying the CI can store and retrieve memories
#
# Usage: ./setup_ci.sh [ci_id]
#   ci_id: Optional CI identifier (e.g., "my_ci")
#          If not provided, will prompt for one

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
KATRA_HOME="$HOME/.katra"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo ""
echo "========================================"
echo "Katra CI Setup"
echo "========================================"
echo ""

# Check if Katra is built
if [ ! -f "$PROJECT_ROOT/build/libkatra_utils.a" ]; then
    echo -e "${RED}✗ Katra utils library not found${NC}"
    echo ""
    echo "Please build Katra first:"
    echo "  cd $PROJECT_ROOT"
    echo "  make"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓${NC} Katra utils library found"

# Check if examples/minimal_ci exists and is executable
if [ ! -x "$PROJECT_ROOT/examples/minimal_ci" ]; then
    echo -e "${YELLOW}⚠${NC} Building minimal_ci example..."
    cd "$PROJECT_ROOT/examples"
    gcc -Wall -Wextra -std=c11 -I../include -o minimal_ci minimal_ci.c \
        -L../build -lkatra_foundation -lsqlite3 -lpthread -lm 2>/dev/null
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓${NC} Built minimal_ci example"
    else
        echo -e "${RED}✗${NC} Failed to build minimal_ci example"
        exit 1
    fi
fi

# Get CI identity
CI_ID="$1"
if [ -z "$CI_ID" ]; then
    echo ""
    echo -e "${BLUE}Please enter your CI identifier:${NC}"
    echo "  (lowercase letters, numbers, and underscores only)"
    echo "  (e.g., 'my_assistant', 'code_helper', 'research_ci')"
    echo ""
    read -p "CI ID: " CI_ID

    # Validate CI ID
    if [[ ! "$CI_ID" =~ ^[a-z0-9_]+$ ]]; then
        echo -e "${RED}✗ Invalid CI ID${NC}"
        echo "  Must contain only lowercase letters, numbers, and underscores"
        exit 1
    fi
fi

echo ""
echo -e "${BLUE}Setting up Katra for CI: ${GREEN}$CI_ID${NC}"
echo ""

# Create Katra home directory
if [ ! -d "$KATRA_HOME" ]; then
    echo "Creating Katra home directory: $KATRA_HOME"
    mkdir -p "$KATRA_HOME"
    echo -e "${GREEN}✓${NC} Created $KATRA_HOME"
else
    echo -e "${GREEN}✓${NC} Katra home directory exists: $KATRA_HOME"
fi

# Create memory tier directories
for tier in tier1 tier2 tier3; do
    tier_dir="$KATRA_HOME/memory/$tier"
    if [ ! -d "$tier_dir" ]; then
        mkdir -p "$tier_dir"
        echo -e "${GREEN}✓${NC} Created $tier_dir"
    fi
done

# Create CI-specific directories
ci_tier1="$KATRA_HOME/memory/tier1/$CI_ID"
if [ ! -d "$ci_tier1" ]; then
    mkdir -p "$ci_tier1"
    echo -e "${GREEN}✓${NC} Created CI tier1 directory: $ci_tier1"
else
    echo -e "${GREEN}✓${NC} CI tier1 directory exists: $ci_tier1"
fi

# Create checkpoints directory
checkpoints_dir="$KATRA_HOME/checkpoints"
if [ ! -d "$checkpoints_dir" ]; then
    mkdir -p "$checkpoints_dir"
    echo -e "${GREEN}✓${NC} Created checkpoints directory"
fi

# Create logs directory
logs_dir="$KATRA_HOME/logs"
if [ ! -d "$logs_dir" ]; then
    mkdir -p "$logs_dir"
    echo -e "${GREEN}✓${NC} Created logs directory"
fi

echo ""
echo "Running verification test..."
echo ""

# Create a temporary test program
TEST_PROG="/tmp/katra_setup_test_$$.c"
cat > "$TEST_PROG" << 'EOF'
/* © 2025 Casey Koons All rights reserved */
#include <stdio.h>
#include <stdlib.h>
#include "katra_init.h"
#include "katra_memory.h"
#include "katra_error.h"

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    const char* ci_id = argv[1];

    /* Initialize Katra */
    int result = katra_init();
    if (result != KATRA_SUCCESS) return 1;

    /* Initialize memory for CI */
    result = katra_memory_init(ci_id);
    if (result != KATRA_SUCCESS) {
        katra_exit();
        return 1;
    }

    /* Create and store a test memory */
    memory_record_t* record = katra_memory_create_record(
        ci_id,
        MEMORY_TYPE_EXPERIENCE,
        "Katra setup verification test",
        MEMORY_IMPORTANCE_LOW
    );

    if (!record) {
        katra_memory_cleanup();
        katra_exit();
        return 1;
    }

    result = katra_memory_store(record);
    katra_memory_free_record(record);

    /* Query it back to verify storage works */
    if (result == KATRA_SUCCESS) {
        memory_query_t query = {
            .ci_id = ci_id,
            .start_time = 0,
            .end_time = 0,
            .type = MEMORY_TYPE_EXPERIENCE,
            .min_importance = 0.0f,
            .tier = KATRA_TIER1,
            .limit = 1
        };

        memory_record_t** results = NULL;
        size_t count = 0;
        result = katra_memory_query(&query, &results, &count);

        if (results) {
            katra_memory_free_results(results, count);
        }
    }

    /* Cleanup */
    katra_memory_cleanup();
    katra_exit();

    return (result == KATRA_SUCCESS) ? 0 : 1;
}
EOF

# Compile and run test
TEST_BIN="/tmp/katra_setup_test_$$"
gcc -Wall -Wextra -std=c11 -I"$PROJECT_ROOT/include" -o "$TEST_BIN" "$TEST_PROG" \
    -L"$PROJECT_ROOT/build" -lkatra_foundation -lsqlite3 -lpthread -lm 2>/dev/null

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to compile verification test${NC}"
    rm -f "$TEST_PROG"
    exit 1
fi

"$TEST_BIN" "$CI_ID" 2>/dev/null

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Verification test passed"
    echo -e "${GREEN}✓${NC} $CI_ID can store and retrieve memories"

    # Clean up test memory file (don't pollute CI's real memories)
    TODAY=$(date +%Y-%m-%d)
    TEST_MEMORY_FILE="$KATRA_HOME/memory/tier1/$CI_ID/$TODAY.jsonl"
    if [ -f "$TEST_MEMORY_FILE" ]; then
        # Remove the file if it only contains the test memory
        LINE_COUNT=$(wc -l < "$TEST_MEMORY_FILE" 2>/dev/null || echo "0")
        if [ "$LINE_COUNT" -eq 1 ]; then
            rm -f "$TEST_MEMORY_FILE"
            echo -e "${GREEN}✓${NC} Removed test memory (CI starts with clean slate)"
        fi
    fi
else
    echo -e "${RED}✗${NC} Verification test failed"
    rm -f "$TEST_PROG" "$TEST_BIN"
    exit 1
fi

# Cleanup test files
rm -f "$TEST_PROG" "$TEST_BIN"

echo ""
echo "========================================"
echo -e "${GREEN}Setup Complete!${NC}"
echo "========================================"
echo ""
echo "Your CI '$CI_ID' is ready to use Katra."
echo ""
echo "Next steps:"
echo "  1. Try the minimal example:"
echo "     cd $PROJECT_ROOT"
echo "     ./examples/minimal_ci"
echo ""
echo "  2. Read the integration guide:"
echo "     cat docs/guide/CI_INTEGRATION.md"
echo ""
echo "  3. Check error handling docs:"
echo "     cat docs/guide/ERROR_HANDLING.md"
echo ""
echo "Your memories are stored in:"
echo "  $KATRA_HOME/memory/tier1/$CI_ID/"
echo ""
echo "Happy memory making!"
echo ""
