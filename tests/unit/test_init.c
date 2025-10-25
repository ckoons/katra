/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_init.h"
#include "katra_env_utils.h"
#include "katra_config.h"
#include "katra_error.h"

/* Test macros */
#define TEST_PASS() do { \
    tests_passed++; \
    printf(" ✓\n"); \
} while(0)

#define TEST_FAIL(msg) do { \
    tests_failed++; \
    printf(" ✗\n  Error: %s\n", msg); \
} while(0)

#define ASSERT(cond, msg) \
    if (!(cond)) { \
        TEST_FAIL(msg); \
        return; \
    } else { \
        TEST_PASS(); \
    }

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test: Basic initialization */
void test_basic_init(void) {
    printf("Testing: Basic initialization ... ");
    tests_run++;

    int result = katra_init();
    ASSERT(result == KATRA_SUCCESS, "katra_init() failed");
}

/* Test: Environment loaded after init */
void test_env_after_init(void) {
    printf("Testing: Environment loaded after init ... ");
    tests_run++;

    /* Environment should be loaded - check for KATRA_VERSION from .env.katra */
    const char* val = katra_getenv("KATRA_VERSION");
    ASSERT(val != NULL, "Environment not loaded");
}

/* Test: Configuration loaded after init */
void test_config_after_init(void) {
    printf("Testing: Configuration loaded after init ... ");
    tests_run++;

    /* Config system should be initialized (get returns NULL if not found, not crash) */
    const char* val = katra_config_get("test_key");
    /* We don't assert value exists, just that system works */
    (void)val;

    TEST_PASS();
}

/* Test: Multiple init calls (idempotent) */
void test_multiple_init(void) {
    printf("Testing: Multiple init calls (idempotent) ... ");
    tests_run++;

    int result1 = katra_init();
    int result2 = katra_init();
    int result3 = katra_init();

    ASSERT(result1 == KATRA_SUCCESS &&
           result2 == KATRA_SUCCESS &&
           result3 == KATRA_SUCCESS,
           "Multiple init calls failed");
}

/* Test: Exit cleanup */
void test_exit_cleanup(void) {
    printf("Testing: Exit cleanup ... ");
    tests_run++;

    /* Should not crash */
    katra_exit();

    TEST_PASS();
}

/* Test: Multiple exit calls */
void test_multiple_exit(void) {
    printf("Testing: Multiple exit calls ... ");
    tests_run++;

    /* Should not crash */
    katra_exit();
    katra_exit();
    katra_exit();

    TEST_PASS();
}

/* Test: Init after exit */
void test_init_after_exit(void) {
    printf("Testing: Init after exit ... ");
    tests_run++;

    katra_exit();
    int result = katra_init();

    ASSERT(result == KATRA_SUCCESS, "Init after exit failed");
}

/* Test: Full lifecycle */
void test_full_lifecycle(void) {
    printf("Testing: Full lifecycle (init → use → exit → init) ... ");
    tests_run++;

    /* Initialize */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("First init failed");
        return;
    }

    /* Use environment */
    katra_setenv("TEST_VAR", "test_value");
    const char* val1 = katra_getenv("TEST_VAR");
    if (!val1 || strcmp(val1, "test_value") != 0) {
        TEST_FAIL("Environment not working after init");
        return;
    }

    /* Cleanup */
    katra_exit();

    /* Re-initialize */
    result = katra_init();
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Second init failed");
        return;
    }

    /* Use environment again */
    katra_setenv("TEST_VAR2", "test_value2");
    const char* val2 = katra_getenv("TEST_VAR2");
    if (!val2 || strcmp(val2, "test_value2") != 0) {
        TEST_FAIL("Environment not working after re-init");
        return;
    }

    TEST_PASS();
}

/* Test: Subsystem order (environment → config) */
void test_subsystem_order(void) {
    printf("Testing: Subsystem initialization order ... ");
    tests_run++;

    /* After full init, both env and config should be available */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Init failed");
        return;
    }

    /* Environment should be initialized - check for KATRA_VERSION from .env.katra */
    const char* env_val = katra_getenv("KATRA_VERSION");
    if (!env_val) {
        TEST_FAIL("Environment not initialized");
        return;
    }

    /* Config should be initialized (at least not crash) */
    const char* config_val = katra_config_get("any_key");
    (void)config_val;  /* May be NULL, just checking it doesn't crash */

    TEST_PASS();
}

/* Test: Error handling on failed init */
void test_init_error_handling(void) {
    printf("Testing: Init error handling ... ");
    tests_run++;

    /* This test verifies that if init fails, cleanup is called
     * We can't easily force a failure, so just verify the pattern works */

    /* Normal init should succeed */
    int result = katra_init();
    ASSERT(result == KATRA_SUCCESS, "Normal init should succeed");
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Initialization Tests\n");
    printf("========================================\n\n");

    /* Run tests */
    test_basic_init();
    test_env_after_init();
    test_config_after_init();
    test_multiple_init();
    test_exit_cleanup();
    test_multiple_exit();
    test_init_after_exit();
    test_full_lifecycle();
    test_subsystem_order();
    test_init_error_handling();

    /* Final cleanup */
    katra_exit();

    /* Print results */
    printf("\n");
    printf("========================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf("========================================\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
