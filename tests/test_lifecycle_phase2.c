/* © 2025 Casey Koons All rights reserved */

/*
 * test_lifecycle_phase2.c - Phase 2 Autonomic Breathing Tests
 *
 * Tests the lifecycle layer implementation:
 * 1. Initialization and configuration
 * 2. Rate-limited breathing
 * 3. Session management with breathing
 * 4. Message awareness integration
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "katra_lifecycle.h"
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_init.h"
#include "katra_error.h"

/* Test result tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("\n--- Test: %s ---\n", name); \
    } while(0)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("❌ FAIL: %s\n", message); \
            tests_failed++; \
            return -1; \
        } else { \
            printf("✅ PASS: %s\n", message); \
            tests_passed++; \
        } \
    } while(0)

/* Test 1: Lifecycle initialization */
int test_lifecycle_init(void) {
    TEST("Lifecycle Initialization");

    int result = katra_lifecycle_init();
    ASSERT(result == KATRA_SUCCESS, "katra_lifecycle_init() succeeds");

    /* Second init should return E_ALREADY_INITIALIZED */
    result = katra_lifecycle_init();
    ASSERT(result == E_ALREADY_INITIALIZED, "Second init returns E_ALREADY_INITIALIZED");

    /* Check default breathing interval */
    int interval = katra_get_breath_interval();
    ASSERT(interval == 30, "Default breathing interval is 30 seconds");

    return 0;
}

/* Test 2: Breathing interval configuration */
int test_breath_interval_config(void) {
    TEST("Breathing Interval Configuration");

    /* Override breathing interval for testing */
    int result = katra_set_breath_interval(2);
    ASSERT(result == KATRA_SUCCESS, "katra_set_breath_interval(2) succeeds");

    int interval = katra_get_breath_interval();
    ASSERT(interval == 2, "Breathing interval updated to 2 seconds");

    /* Invalid interval should fail */
    result = katra_set_breath_interval(0);
    ASSERT(result == E_INVALID_PARAMS, "Invalid interval (0) returns E_INVALID_PARAMS");

    return 0;
}

/* Test 3: Session start with first breath */
int test_session_start_with_breath(void) {
    TEST("Session Start with First Breath");

    /* Initialize Katra core */
    int result = katra_init();
    ASSERT(result == KATRA_SUCCESS || result == E_ALREADY_INITIALIZED,
           "katra_init() succeeds");

    /* Initialize memory */
    result = katra_memory_init("test_ci_phase2");
    ASSERT(result == KATRA_SUCCESS, "katra_memory_init() succeeds");

    /* NOTE: Skipping meeting room init to avoid MCP dependencies in test */
    /* result = meeting_room_init(); */

    /* Start session - should trigger first breath */
    result = katra_session_start("test_ci_phase2");
    ASSERT(result == KATRA_SUCCESS, "katra_session_start() succeeds");

    printf("✅ Session started with autonomic breathing\n");

    return 0;
}

/* Test 4: Rate-limited breathing */
int test_rate_limited_breathing(void) {
    TEST("Rate-Limited Breathing");

    breath_context_t context1, context2, context3;

    /* First breath (should succeed immediately) */
    int result = katra_breath(&context1);
    ASSERT(result == KATRA_SUCCESS, "First katra_breath() succeeds");

    time_t time1 = context1.last_breath;
    printf("   First breath timestamp: %ld\n", (long)time1);

    /* Second breath immediately (should return cached) */
    result = katra_breath(&context2);
    ASSERT(result == KATRA_SUCCESS, "Second katra_breath() succeeds (cached)");
    ASSERT(context2.last_breath == time1, "Second breath returns cached timestamp");
    printf("   Second breath (cached): %ld\n", (long)context2.last_breath);

    /* Wait for breathing interval (2 seconds) */
    printf("   Waiting 2 seconds for breathing interval...\n");
    sleep(2);

    /* Third breath (should perform actual check) */
    result = katra_breath(&context3);
    ASSERT(result == KATRA_SUCCESS, "Third katra_breath() succeeds");
    ASSERT(context3.last_breath > time1, "Third breath has newer timestamp");
    printf("   Third breath (actual check): %ld\n", (long)context3.last_breath);

    return 0;
}

/* Test 5: Force breath (bypass rate limiting) */
int test_force_breath(void) {
    TEST("Force Breath (Bypass Rate Limiting)");

    breath_context_t context1, context2;

    /* Get current breath */
    int result = katra_breath(&context1);
    ASSERT(result == KATRA_SUCCESS, "katra_breath() succeeds");

    time_t time1 = context1.last_breath;
    printf("   Breath timestamp: %ld\n", (long)time1);

    /* Force immediate breath (should bypass rate limit) */
    result = katra_force_breath(&context2);
    ASSERT(result == KATRA_SUCCESS, "katra_force_breath() succeeds");
    ASSERT(context2.last_breath >= time1, "Forced breath has updated timestamp");
    printf("   Forced breath timestamp: %ld\n", (long)context2.last_breath);

    return 0;
}

/* Test 6: Session end with final breath */
int test_session_end_with_breath(void) {
    TEST("Session End with Final Breath");

    /* End session - should trigger final breath */
    int result = katra_session_end();
    ASSERT(result == KATRA_SUCCESS, "katra_session_end() succeeds");

    printf("✅ Session ended with final breath and cleanup\n");

    /* Cleanup */
    /* meeting_room_cleanup(); - skipped due to MCP dependencies */
    katra_lifecycle_cleanup();
    katra_memory_cleanup();
    katra_exit();

    return 0;
}

/* Main test runner */
int main(void) {
    printf("========================================\n");
    printf("Phase 2: Autonomic Breathing Tests\n");
    printf("========================================\n");

    /* Run tests */
    if (test_lifecycle_init() != 0) goto cleanup;
    if (test_breath_interval_config() != 0) goto cleanup;
    if (test_session_start_with_breath() != 0) goto cleanup;
    if (test_rate_limited_breathing() != 0) goto cleanup;
    if (test_force_breath() != 0) goto cleanup;
    if (test_session_end_with_breath() != 0) goto cleanup;

cleanup:
    /* Print summary */
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("\n🎉 All Phase 2 tests PASSED!\n");
        return 0;
    } else {
        printf("\n❌ Some tests FAILED\n");
        return 1;
    }
}
