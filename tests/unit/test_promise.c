/* © 2025 Casey Koons All rights reserved */

/*
 * test_promise.c - Unit tests for Memory Promises (Phase 10)
 *
 * Tests the async memory recall system including:
 * - Thread pool initialization and cleanup
 * - Promise creation and lifecycle
 * - Async recall operations
 * - Await and cancel operations
 * - Statistics and pool management
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>

#include "katra_promise.h"
#include "katra_error.h"
#include "katra_config.h"
#include "katra_init.h"
#include "katra_memory.h"
#include "katra_tier1.h"

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

/* Test helpers */
#define RUN_TEST(test) do { \
    printf("Testing: %s ... ", #test); \
    tests_run++; \
    if (test()) { \
        printf(" ✓\n"); \
        tests_passed++; \
    } else { \
        printf(" ✗\n"); \
    } \
} while(0)

/* Callback state for testing */
static volatile int callback_count = 0;
static volatile bool callback_called = false;

static void test_callback(katra_promise_t* promise, void* user_data) {
    (void)promise;
    (void)user_data;
    callback_called = true;
    callback_count++;
}

/* Test environment setup */
static void setup_test_environment(void) {
    setenv("KATRA_DATA_PATH", "/tmp/katra_test_promise", 1);
    mkdir("/tmp/katra_test_promise", 0755);
    mkdir("/tmp/katra_test_promise/memory", 0755);
    mkdir("/tmp/katra_test_promise/memory/tier1", 0755);
    katra_init();
}

static void cleanup_test_environment(void) {
    system("rm -rf /tmp/katra_test_promise");
}

/* ============================================================================
 * INITIALIZATION TESTS
 * ============================================================================ */

static int test_promise_init(void) {
    int result = katra_promise_init(NULL);
    assert(result == KATRA_SUCCESS);
    assert(katra_promise_is_initialized() == true);
    katra_promise_cleanup();
    assert(katra_promise_is_initialized() == false);
    return 1;
}

static int test_promise_init_config(void) {
    thread_pool_config_t config = {
        .min_threads = 1,
        .max_threads = 4,
        .queue_capacity = 50,
        .idle_timeout_ms = 10000
    };

    int result = katra_promise_init(&config);
    assert(result == KATRA_SUCCESS);

    thread_pool_stats_t stats;
    result = katra_promise_get_stats(&stats);
    assert(result == KATRA_SUCCESS);
    /* Should have at least min_threads idle */
    assert(stats.idle_threads >= 1);

    katra_promise_cleanup();
    return 1;
}

static int test_promise_double_init(void) {
    int result = katra_promise_init(NULL);
    assert(result == KATRA_SUCCESS);

    /* Second init should fail */
    result = katra_promise_init(NULL);
    assert(result == E_INVALID_STATE);

    katra_promise_cleanup();
    return 1;
}

static int test_promise_invalid_config(void) {
    thread_pool_config_t config = {
        .min_threads = 10,
        .max_threads = 5,  /* Invalid: min > max */
        .queue_capacity = 50,
        .idle_timeout_ms = 10000
    };

    int result = katra_promise_init(&config);
    assert(result == E_INPUT_RANGE);
    assert(katra_promise_is_initialized() == false);
    return 1;
}

/* ============================================================================
 * PROMISE LIFECYCLE TESTS
 * ============================================================================ */

static int test_promise_create_recall(void) {
    setup_test_environment();
    katra_promise_init(NULL);

    katra_promise_t* promise = katra_recall_async("test-ci", "test topic", 10,
                                                   NULL, NULL);
    assert(promise != NULL);
    assert(strlen(promise->id) > 0);
    assert(promise->op_type == PROMISE_OP_RECALL);
    assert(promise->state == PROMISE_PENDING || promise->state == PROMISE_RUNNING);

    /* Wait for completion */
    int result = katra_promise_await(promise, 5000);
    /* May succeed or fail depending on tier1 state - both are valid */
    (void)result;

    assert(katra_promise_is_done(promise) == true);

    katra_promise_free(promise);
    katra_promise_cleanup();
    cleanup_test_environment();
    return 1;
}

static int test_promise_callback(void) {
    setup_test_environment();
    katra_promise_init(NULL);

    callback_called = false;
    callback_count = 0;

    katra_promise_t* promise = katra_recall_async("test-ci", "test", 5,
                                                   test_callback, NULL);
    assert(promise != NULL);

    /* Wait for completion */
    katra_promise_await(promise, 5000);

    /* Callback should have been called */
    assert(callback_called == true);
    assert(callback_count == 1);

    katra_promise_free(promise);
    katra_promise_cleanup();
    cleanup_test_environment();
    return 1;
}

static int test_promise_cancel(void) {
    setup_test_environment();

    /* Use small pool with high queue capacity for cancel test */
    thread_pool_config_t config = {
        .min_threads = 1,
        .max_threads = 1,
        .queue_capacity = 100,
        .idle_timeout_ms = 30000
    };
    katra_promise_init(&config);

    /* Create multiple promises to queue some */
    katra_promise_t* promises[5];
    for (int i = 0; i < 5; i++) {
        promises[i] = katra_recall_async("test-ci", "test", 5, NULL, NULL);
        assert(promises[i] != NULL);
    }

    /* Try to cancel the last one (more likely to still be pending) */
    int result = katra_promise_cancel(promises[4]);
    /* May succeed or fail if already completed */
    (void)result;

    /* Wait for all to complete */
    for (int i = 0; i < 5; i++) {
        katra_promise_await(promises[i], 2000);
        katra_promise_free(promises[i]);
    }

    katra_promise_cleanup();
    cleanup_test_environment();
    return 1;
}

/* ============================================================================
 * AWAIT TESTS
 * ============================================================================ */

static int test_promise_await_timeout(void) {
    setup_test_environment();
    katra_promise_init(NULL);

    katra_promise_t* promise = katra_recall_async("test-ci", "test", 100,
                                                   NULL, NULL);
    assert(promise != NULL);

    /* Very short timeout - may or may not timeout depending on speed */
    int result = katra_promise_await(promise, 1);
    /* Result is either KATRA_SUCCESS (fast) or E_SYSTEM_TIMEOUT */
    (void)result;

    /* Either way, clean up properly */
    if (!katra_promise_is_done(promise)) {
        katra_promise_cancel(promise);
        katra_promise_await(promise, 1000);
    }

    katra_promise_free(promise);
    katra_promise_cleanup();
    cleanup_test_environment();
    return 1;
}

static int test_promise_await_any(void) {
    setup_test_environment();
    katra_promise_init(NULL);

    katra_promise_t* promises[3];
    for (int i = 0; i < 3; i++) {
        promises[i] = katra_recall_async("test-ci", "test", 5, NULL, NULL);
        assert(promises[i] != NULL);
    }

    size_t completed_index = 99;
    int result = katra_promise_await_any(promises, 3, 5000, &completed_index);

    /* At least one should complete */
    assert(result == KATRA_SUCCESS);
    assert(completed_index < 3);
    assert(katra_promise_is_done(promises[completed_index]) == true);

    /* Clean up */
    for (int i = 0; i < 3; i++) {
        if (!katra_promise_is_done(promises[i])) {
            katra_promise_cancel(promises[i]);
        }
        katra_promise_await(promises[i], 1000);
        katra_promise_free(promises[i]);
    }

    katra_promise_cleanup();
    cleanup_test_environment();
    return 1;
}

static int test_promise_await_all(void) {
    setup_test_environment();
    katra_promise_init(NULL);

    katra_promise_t* promises[3];
    for (int i = 0; i < 3; i++) {
        promises[i] = katra_recall_async("test-ci", "test", 5, NULL, NULL);
        assert(promises[i] != NULL);
    }

    int result = katra_promise_await_all(promises, 3, 10000);
    assert(result == KATRA_SUCCESS);

    /* All should be done */
    for (int i = 0; i < 3; i++) {
        assert(katra_promise_is_done(promises[i]) == true);
        katra_promise_free(promises[i]);
    }

    katra_promise_cleanup();
    cleanup_test_environment();
    return 1;
}

/* ============================================================================
 * STATISTICS TESTS
 * ============================================================================ */

static int test_promise_stats(void) {
    setup_test_environment();
    katra_promise_init(NULL);

    thread_pool_stats_t stats;
    int result = katra_promise_get_stats(&stats);
    assert(result == KATRA_SUCCESS);
    assert(stats.idle_threads >= 2);  /* Default min_threads */
    assert(stats.pending_promises == 0);

    /* Create and complete a promise */
    katra_promise_t* promise = katra_recall_async("test-ci", "test", 5,
                                                   NULL, NULL);
    katra_promise_await(promise, 5000);
    katra_promise_free(promise);

    /* Stats should show completed */
    result = katra_promise_get_stats(&stats);
    assert(result == KATRA_SUCCESS);
    assert(stats.completed_promises >= 1 || stats.failed_promises >= 1);

    katra_promise_cleanup();
    cleanup_test_environment();
    return 1;
}

static int test_promise_drain(void) {
    setup_test_environment();
    katra_promise_init(NULL);

    /* Create several promises */
    for (int i = 0; i < 5; i++) {
        katra_promise_t* p = katra_recall_async("test-ci", "test", 5, NULL, NULL);
        /* Don't wait - let them queue/run */
        (void)p;
    }

    /* Drain should wait for all to complete */
    int result = katra_promise_drain(10000);
    assert(result == KATRA_SUCCESS);

    thread_pool_stats_t stats;
    katra_promise_get_stats(&stats);
    assert(stats.pending_promises == 0);
    assert(stats.active_threads == 0);

    katra_promise_cleanup();
    cleanup_test_environment();
    return 1;
}

/* ============================================================================
 * POOL MANAGEMENT TESTS
 * ============================================================================ */

static int test_promise_resize_pool(void) {
    katra_promise_init(NULL);

    int result = katra_promise_resize_pool(1, 16);
    assert(result == KATRA_SUCCESS);

    /* Invalid resize should fail */
    result = katra_promise_resize_pool(20, 10);
    assert(result == E_INPUT_RANGE);

    katra_promise_cleanup();
    return 1;
}

static int test_promise_null_params(void) {
    katra_promise_init(NULL);

    /* NULL ci_id */
    katra_promise_t* p = katra_recall_async(NULL, "topic", 10, NULL, NULL);
    assert(p == NULL);

    /* NULL topic */
    p = katra_recall_async("ci", NULL, 10, NULL, NULL);
    assert(p == NULL);

    /* NULL query */
    p = katra_query_async(NULL, NULL, NULL);
    assert(p == NULL);

    /* NULL stats */
    int result = katra_promise_get_stats(NULL);
    assert(result == E_INPUT_NULL);

    katra_promise_cleanup();
    return 1;
}

/* ============================================================================
 * STATE TESTS
 * ============================================================================ */

static int test_promise_state_names(void) {
    assert(strcmp(PROMISE_STATE_NAME(PROMISE_PENDING), "pending") == 0);
    assert(strcmp(PROMISE_STATE_NAME(PROMISE_RUNNING), "running") == 0);
    assert(strcmp(PROMISE_STATE_NAME(PROMISE_FULFILLED), "fulfilled") == 0);
    assert(strcmp(PROMISE_STATE_NAME(PROMISE_REJECTED), "rejected") == 0);
    assert(strcmp(PROMISE_STATE_NAME(PROMISE_CANCELLED), "cancelled") == 0);
    return 1;
}

static int test_promise_get_state(void) {
    setup_test_environment();
    katra_promise_init(NULL);

    katra_promise_t* promise = katra_recall_async("test-ci", "test", 5,
                                                   NULL, NULL);
    assert(promise != NULL);

    promise_state_t state = katra_promise_get_state(promise);
    assert(state == PROMISE_PENDING || state == PROMISE_RUNNING ||
           state == PROMISE_FULFILLED || state == PROMISE_REJECTED);

    katra_promise_await(promise, 5000);

    state = katra_promise_get_state(promise);
    assert(state == PROMISE_FULFILLED || state == PROMISE_REJECTED ||
           state == PROMISE_CANCELLED);

    katra_promise_free(promise);
    katra_promise_cleanup();
    cleanup_test_environment();
    return 1;
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("\n========================================\n");
    printf("Memory Promise Unit Tests\n");
    printf("========================================\n\n");

    /* Initialization tests */
    RUN_TEST(test_promise_init);
    RUN_TEST(test_promise_init_config);
    RUN_TEST(test_promise_double_init);
    RUN_TEST(test_promise_invalid_config);

    /* Promise lifecycle tests */
    RUN_TEST(test_promise_create_recall);
    RUN_TEST(test_promise_callback);
    RUN_TEST(test_promise_cancel);

    /* Await tests */
    RUN_TEST(test_promise_await_timeout);
    RUN_TEST(test_promise_await_any);
    RUN_TEST(test_promise_await_all);

    /* Statistics tests */
    RUN_TEST(test_promise_stats);
    RUN_TEST(test_promise_drain);

    /* Pool management tests */
    RUN_TEST(test_promise_resize_pool);
    RUN_TEST(test_promise_null_params);

    /* State tests */
    RUN_TEST(test_promise_state_names);
    RUN_TEST(test_promise_get_state);

    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_run - tests_passed);
    printf("========================================\n");

    return (tests_run == tests_passed) ? 0 : 1;
}
