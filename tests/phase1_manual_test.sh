#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# Phase 1 Manual Test Script
#
# Tests returning CI memory access functionality
# Test sequence:
#   1. Nyx stores/recalls memories
#   2. Twin Nyx (new session) recalls Nyx's memories
#   3. Register as Alice, create memories, restart, verify isolation

set -e  # Exit on error

KATRA_BIN="./bin/katra_mcp_server"
TEST_LOG="tests/phase1_test.log"

echo "=== Katra Phase 1 Manual Test ===" | tee "$TEST_LOG"
echo ""

# Function to test memory operations
test_memory_ops() {
    local persona_name="$1"
    echo "Testing memory operations for: $persona_name"

    # Start MCP server with persona
    export KATRA_PERSONA="$persona_name"

    # Note: This is a manual test script
    # Actual testing requires MCP client interaction
    # This script documents the test procedure

    echo "  1. Set KATRA_PERSONA=$persona_name"
    echo "  2. Start Claude Code session"
    echo "  3. Call katra_register(name='$persona_name', role='test')"
    echo "  4. Call katra_remember(content='Test memory for $persona_name', context='significant')"
    echo "  5. Call katra_recall(topic='Test memory')"
    echo "  6. Verify memory is returned"
    echo ""
}

echo "=== Test 1: Nyx Initial Session ===" | tee -a "$TEST_LOG"
test_memory_ops "Nyx" | tee -a "$TEST_LOG"

echo "=== Test 2: Twin Nyx (New Session, Same Persona) ===" | tee -a "$TEST_LOG"
echo "  1. Close previous session" | tee -a "$TEST_LOG"
echo "  2. Start new session with KATRA_PERSONA=Nyx" | tee -a "$TEST_LOG"
echo "  3. Call katra_recall(topic='Test memory')" | tee -a "$TEST_LOG"
echo "  4. Verify memory from Test 1 is returned" | tee -a "$TEST_LOG"
echo "" | tee -a "$TEST_LOG"

echo "=== Test 3: Alice (Different Persona) ===" | tee -a "$TEST_LOG"
test_memory_ops "Alice" | tee -a "$TEST_LOG"
echo "  7. Verify Alice CANNOT see Nyx's memories" | tee -a "$TEST_LOG"
echo "" | tee -a "$TEST_LOG"

echo "=== Test 4: Alice Returns ===" | tee -a "$TEST_LOG"
echo "  1. Close Alice session" | tee -a "$TEST_LOG"
echo "  2. Start new session with KATRA_PERSONA=Alice" | tee -a "$TEST_LOG"
echo "  3. Call katra_recall(topic='Test memory')" | tee -a "$TEST_LOG"
echo "  4. Verify ONLY Alice's memories are returned" | tee -a "$TEST_LOG"
echo "" | tee -a "$TEST_LOG"

echo "=== Manual Test Procedure Complete ===" | tee -a "$TEST_LOG"
echo "See $TEST_LOG for test procedure" | tee -a "$TEST_LOG"
echo ""
echo "To execute tests:"
echo "  1. Follow the test procedure above manually using Claude Code"
echo "  2. Verify each step succeeds"
echo "  3. Report any failures"
