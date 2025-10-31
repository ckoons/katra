/* © 2025 Casey Koons All rights reserved */

/* Test katra_get_session_info() API */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "katra_init.h"
#include "katra_breathing.h"
#include "katra_error.h"

#define TEST_CI_ID "test_session_info_ci"

static int test_session_info_before_init(void) {
    printf("TEST: Session info before init...\n");

    katra_session_info_t info;
    int result = katra_get_session_info(&info);

    if (result != E_INVALID_STATE) {
        printf("  FAIL: Expected E_INVALID_STATE before init, got %d\n", result);
        return 1;
    }

    printf("  PASS: Correctly returns E_INVALID_STATE before init\n");
    return 0;
}

static int test_session_info_after_start(void) {
    printf("TEST: Session info after session_start...\n");

    /* Start session */
    int result = session_start(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("  FAIL: session_start failed: %d\n", result);
        return 1;
    }

    /* Get session info */
    katra_session_info_t info;
    result = katra_get_session_info(&info);
    if (result != KATRA_SUCCESS) {
        printf("  FAIL: katra_get_session_info failed: %d\n", result);
        return 1;
    }

    /* Verify fields */
    if (strcmp(info.ci_id, TEST_CI_ID) != 0) {
        printf("  FAIL: ci_id mismatch: expected '%s', got '%s'\n",
               TEST_CI_ID, info.ci_id);
        return 1;
    }

    if (strlen(info.session_id) == 0) {
        printf("  FAIL: session_id is empty\n");
        return 1;
    }

    if (!info.is_active) {
        printf("  FAIL: is_active should be true\n");
        return 1;
    }

    if (info.start_time == 0) {
        printf("  FAIL: start_time is zero\n");
        return 1;
    }

    time_t now = time(NULL);
    if (info.start_time > now) {
        printf("  FAIL: start_time is in the future\n");
        return 1;
    }

    printf("  PASS: Session info correctly populated\n");
    printf("    ci_id: %s\n", info.ci_id);
    printf("    session_id: %s\n", info.session_id);
    printf("    start_time: %ld\n", (long)info.start_time);
    printf("    is_active: %s\n", info.is_active ? "true" : "false");

    return 0;
}

static int test_session_info_metrics(void) {
    printf("TEST: Session info tracks metrics...\n");

    /* Add some memories */
    int result = remember("Test memory 1", WHY_INTERESTING);
    if (result != KATRA_SUCCESS) {
        printf("  FAIL: remember failed: %d\n", result);
        return 1;
    }

    result = learn("Test knowledge");
    if (result != KATRA_SUCCESS) {
        printf("  FAIL: learn failed: %d\n", result);
        return 1;
    }

    /* Perform some queries */
    size_t count = 0;
    char** thoughts = recent_thoughts(10, &count);
    free_memory_list(thoughts, count);

    char** memories = recall_about("test", &count);
    free_memory_list(memories, count);

    /* Get session info */
    katra_session_info_t info;
    result = katra_get_session_info(&info);
    if (result != KATRA_SUCCESS) {
        printf("  FAIL: katra_get_session_info failed: %d\n", result);
        return 1;
    }

    /* Verify metrics */
    if (info.memories_added < 2) {
        printf("  FAIL: Expected at least 2 memories, got %zu\n", info.memories_added);
        return 1;
    }

    if (info.queries_processed < 2) {
        printf("  FAIL: Expected at least 2 queries, got %zu\n", info.queries_processed);
        return 1;
    }

    if (info.last_activity == 0) {
        printf("  FAIL: last_activity should be set\n");
        return 1;
    }

    printf("  PASS: Metrics correctly tracked\n");
    printf("    memories_added: %zu\n", info.memories_added);
    printf("    queries_processed: %zu\n", info.queries_processed);
    printf("    last_activity: %ld\n", (long)info.last_activity);

    return 0;
}

static int test_session_info_null_check(void) {
    printf("TEST: Session info null check...\n");

    int result = katra_get_session_info(NULL);
    if (result != E_INPUT_NULL) {
        printf("  FAIL: Expected E_INPUT_NULL, got %d\n", result);
        return 1;
    }

    printf("  PASS: Correctly rejects NULL pointer\n");
    return 0;
}

int main(void) {
    int failures = 0;

    printf("=== Session Info API Tests ===\n\n");

    /* Initialize Katra */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        printf("FATAL: katra_init failed: %d\n", result);
        return 1;
    }

    /* Test 1: Before initialization */
    failures += test_session_info_before_init();

    /* Test 2: After session start */
    failures += test_session_info_after_start();

    /* Test 3: Metrics tracking */
    failures += test_session_info_metrics();

    /* Test 4: Null check */
    failures += test_session_info_null_check();

    /* Cleanup */
    session_end();
    breathe_cleanup();
    katra_exit();

    printf("\n=== Test Summary ===\n");
    printf("Total tests: 4\n");
    printf("Failures: %d\n", failures);

    return failures > 0 ? 1 : 0;
}
