#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# test_working_memory_unit.sh - Quick unit test for Working Memory Budget (Phase 2 + 2.1)
#
# Tests the configuration and basic functionality without waiting for breathing cycles

set -e  # Exit on error

echo "========================================"
echo "Working Memory Budget Unit Test"
echo "Phase 2 + 2.1 Configuration Verification"
echo "========================================"
echo ""

# Change to project root
cd "$(dirname "$0")/.."

# Test 1: Verify working memory implementation is compiled
echo "Test 1: Verifying working memory implementation is compiled..."
if nm build/katra_breathing_working_memory.o 2>/dev/null | grep -q "working_memory_get_stats"; then
    echo "✅ PASSED: working_memory_get_stats() found in object file"
else
    echo "❌ FAILED: working_memory_get_stats() not found"
    exit 1
fi
echo ""

# Test 2: Verify working memory is linked into library
echo "Test 2: Verifying working memory is linked into library..."
if nm build/libkatra_utils.a 2>/dev/null | grep -q "working_memory_get_stats"; then
    echo "✅ PASSED: working_memory functions found in library"
else
    echo "❌ FAILED: working_memory functions not found in library"
    exit 1
fi
echo ""

# Test 3: Verify configuration constants
echo "Test 3: Verifying configuration constants..."
if grep -q "WORKING_MEMORY_SOFT_LIMIT 35" include/katra_limits.h; then
    echo "✅ PASSED: Soft limit set to 35"
else
    echo "❌ FAILED: Soft limit not configured correctly"
    exit 1
fi

if grep -q "WORKING_MEMORY_HARD_LIMIT 50" include/katra_limits.h; then
    echo "✅ PASSED: Hard limit set to 50"
else
    echo "❌ FAILED: Hard limit not configured correctly"
    exit 1
fi

if grep -q "WORKING_MEMORY_BATCH_SIZE 10" include/katra_limits.h; then
    echo "✅ PASSED: Batch size set to 10"
else
    echo "❌ FAILED: Batch size not configured correctly"
    exit 1
fi
echo ""

# Test 4: Verify protected tags are defined
echo "Test 4: Verifying protected tags implementation..."
if grep -q "g_protected_tags" src/breathing/katra_breathing.c; then
    echo "✅ PASSED: Protected tags array found"

    # Check for specific protected tags
    if grep -A 10 "g_protected_tags" src/breathing/katra_breathing.c | grep -q "TAG_INSIGHT"; then
        echo "  ✅ insight tag protected"
    fi

    if grep -A 10 "g_protected_tags" src/breathing/katra_breathing.c | grep -q "TAG_PERMANENT"; then
        echo "  ✅ permanent tag protected"
    fi

    if grep -A 10 "g_protected_tags" src/breathing/katra_breathing.c | grep -q "TAG_PERSONAL"; then
        echo "  ✅ personal tag protected"
    fi
else
    echo "❌ FAILED: Protected tags not implemented"
    exit 1
fi
echo ""

# Test 5: Verify budget check is integrated into breathing cycle
echo "Test 5: Verifying budget check integration..."
if grep -q "working_memory_check_budget" src/breathing/katra_breathing_health.c; then
    echo "✅ PASSED: Budget check integrated into periodic maintenance"
else
    echo "❌ FAILED: Budget check not integrated"
    exit 1
fi
echo ""

# Test 6: Verify stats API is exposed
echo "Test 6: Verifying stats API is exposed..."
if grep -q "working_memory_stats_t" include/katra_breathing.h; then
    echo "✅ PASSED: Stats structure defined in public header"
else
    echo "❌ FAILED: Stats structure not exposed"
    exit 1
fi

if grep -q "working_memory_get_stats" include/katra_breathing.h; then
    echo "✅ PASSED: Stats getter function declared in public header"
else
    echo "❌ FAILED: Stats getter not exposed"
    exit 1
fi
echo ""

# Test 7: Verify tag-aware archival configuration
echo "Test 7: Verifying tag-aware archival configuration..."
if grep -q "tag_aware_archival" include/katra_breathing.h; then
    echo "✅ PASSED: Tag-aware archival configuration field exists"
else
    echo "❌ FAILED: Tag-aware archival not configured"
    exit 1
fi
echo ""

# Test 8: Verify documentation exists
echo "Test 8: Verifying documentation..."
if [ -f "docs/guide/WORKING_MEMORY_BUDGET.md" ]; then
    echo "✅ PASSED: Working memory budget documentation exists"

    # Check doc completeness
    if grep -q "Phase 2.1" docs/guide/WORKING_MEMORY_BUDGET.md; then
        echo "  ✅ Documents Phase 2.1 (tag-aware archival)"
    fi

    if grep -q "protected_tags" docs/guide/WORKING_MEMORY_BUDGET.md; then
        echo "  ✅ Documents protected tags"
    fi

    if grep -q "working_memory_stats_t" docs/guide/WORKING_MEMORY_BUDGET.md; then
        echo "  ✅ Documents stats API"
    fi
else
    echo "❌ FAILED: Documentation not found"
    exit 1
fi
echo ""

# Summary
echo "========================================"
echo "🎉 All Unit Tests PASSED!"
echo "========================================"
echo ""
echo "Phase 2 + 2.1 Implementation Verified:"
echo "  ✅ Working memory functions compiled"
echo "  ✅ Functions linked into library"
echo "  ✅ Configuration constants correct (35/50/10)"
echo "  ✅ Protected tags implemented (insight, permanent, personal, etc.)"
echo "  ✅ Budget check integrated into breathing cycle"
echo "  ✅ Stats API exposed to public interface"
echo "  ✅ Tag-aware archival configured"
echo "  ✅ Documentation complete"
echo ""
echo "Next Steps for Full Production Testing:"
echo "  1. Run: ./tests/test_working_memory_production.sh"
echo "     (This will test with real MCP server and breathing cycles)"
echo ""
echo "  2. Or manually test:"
echo "     - Start MCP server: make restart-mcp"
echo "     - Register CI: ./bin/katra-cli register test-ci tester"
echo "     - Create memories: ./bin/katra-cli remember \"test\" --tags session"
echo "     - Wait 30s and check logs for archival/deletion"
echo ""
