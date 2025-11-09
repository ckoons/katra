/* © 2025 Casey Koons All rights reserved */

/*
 * test_breathing_simple.c - Simple Phase 2 Breathing Test
 *
 * Tests core breathing functions without full session dependencies
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "katra_lifecycle.h"
#include "katra_error.h"

int main(void) {
    printf("========================================\n");
    printf("Phase 2: Autonomic Breathing Test\n");
    printf("========================================\n\n");

    /* Test 1: Initialize lifecycle layer */
    printf("Test 1: Initializing lifecycle layer...\n");
    int result = katra_lifecycle_init();
    if (result != KATRA_SUCCESS) {
        printf("❌ FAILED: katra_lifecycle_init() returned %d\n", result);
        return 1;
    }
    printf("✅ PASSED: Lifecycle layer initialized\n\n");

    /* Test 2: Check default breathing interval */
    printf("Test 2: Checking default breathing interval...\n");
    int interval = katra_get_breath_interval();
    printf("   Default interval: %d seconds\n", interval);
    if (interval != 30) {
        printf("❌ FAILED: Expected 30, got %d\n", interval);
        return 1;
    }
    printf("✅ PASSED: Default breathing interval is 30 seconds\n\n");

    /* Test 3: Override breathing interval for testing */
    printf("Test 3: Setting breathing interval to 2 seconds for testing...\n");
    result = katra_set_breath_interval(2);
    if (result != KATRA_SUCCESS) {
        printf("❌ FAILED: katra_set_breath_interval() returned %d\n", result);
        return 1;
    }
    interval = katra_get_breath_interval();
    if (interval != 2) {
        printf("❌ FAILED: Expected 2, got %d\n", interval);
        return 1;
    }
    printf("✅ PASSED: Breathing interval updated to 2 seconds\n\n");

    /* Test 4: Test invalid interval */
    printf("Test 4: Testing invalid breathing interval (0)...\n");
    result = katra_set_breath_interval(0);
    if (result != E_INVALID_PARAMS) {
        printf("❌ FAILED: Expected E_INVALID_PARAMS, got %d\n", result);
        return 1;
    }
    printf("✅ PASSED: Invalid interval correctly rejected\n\n");

    /* Test 5: Cleanup */
    printf("Test 5: Cleaning up lifecycle layer...\n");
    katra_lifecycle_cleanup();
    printf("✅ PASSED: Lifecycle layer cleaned up\n\n");

    /* Summary */
    printf("========================================\n");
    printf("🎉 All Phase 2 tests PASSED!\n");
    printf("========================================\n");
    printf("\nPhase 2 Implementation Verified:\n");
    printf("  ✅ Lifecycle initialization\n");
    printf("  ✅ Default breathing interval (30s)\n");
    printf("  ✅ Configurable breathing interval\n");
    printf("  ✅ Input validation\n");
    printf("  ✅ Cleanup\n");
    printf("\nNote: Full session integration tested via MCP server\n");

    return 0;
}
