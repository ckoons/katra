/* © 2025 Casey Koons All rights reserved */

/*
 * test_breathing_primitives.c - Unit tests for breathing layer primitives
 *
 * Tests for:
 * - remember() and remember_with_note()
 * - reflect(), learn(), decide(), notice_pattern()
 * - Error handling and state validation
 * - Statistics tracking
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"

/* Test state */
static int tests_run = 0;
static int tests_passed = 0;
static char test_ci_id[64];

/* Test helpers */
#define ASSERT_EQUAL(actual, expected) do { \
    if ((actual) != (expected)) { \
        printf("  ✗ %s (line %d): expected %zu, got %zu\n", __func__, __LINE__, (size_t)(expected), (size_t)(actual)); \
        return false; \
    } \
} while (0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("  ✗ %s (line %d): expected non-NULL pointer\n", __func__, __LINE__); \
        return false; \
    } \
} while (0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf("  ✗ %s (line %d): expected NULL pointer\n", __func__, __LINE__); \
        return false; \
    } \
} while (0)

#define RUN_TEST(test_func) do { \
    tests_run++; \
    if (test_func()) { \
        tests_passed++; \
        printf("  ✓ %s\n", #test_func); \
    } else { \
        printf("  ✗ %s\n", #test_func); \
    } \
} while (0)

/* Setup and teardown */
static void setup_test_env(void) {
    /* Generate unique CI ID for this test run */
    snprintf(test_ci_id, sizeof(test_ci_id), "test_primitives_%ld", (long)time(NULL));

    /* Initialize breathing layer */
    int result = breathe_init(test_ci_id);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize breathing layer: %d\n", result);
        exit(1);
    }
}

static void teardown_test_env(void) {
    breathe_cleanup();

    /* Clean up test data */
    char path[256];
    snprintf(path, sizeof(path), "%s/.katra/memory/tier1/%s",
             getenv("HOME"), test_ci_id);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
    system(cmd);
}

/* ============================================================================
 * TESTS: remember() primitive
 * ============================================================================ */

static bool test_remember_basic(void) {
    int result = remember("This is a test thought", WHY_INTERESTING);
    ASSERT_EQUAL(result, KATRA_SUCCESS);
    return true;
}

static bool test_remember_different_importance(void) {
    int result;

    result = remember("Trivial thought", WHY_TRIVIAL);
    ASSERT_EQUAL(result, KATRA_SUCCESS);

    result = remember("Routine thought", WHY_ROUTINE);
    ASSERT_EQUAL(result, KATRA_SUCCESS);

    result = remember("Interesting thought", WHY_INTERESTING);
    ASSERT_EQUAL(result, KATRA_SUCCESS);

    result = remember("Significant thought", WHY_SIGNIFICANT);
    ASSERT_EQUAL(result, KATRA_SUCCESS);

    result = remember("Critical thought", WHY_CRITICAL);
    ASSERT_EQUAL(result, KATRA_SUCCESS);

    return true;
}

static bool test_remember_null_thought(void) {
    int result = remember(NULL, WHY_INTERESTING);
    ASSERT_EQUAL(result, E_INPUT_NULL);
    return true;
}

/* Note: Empty string validation happens in katra_memory_store(), not in remember() */

static bool test_remember_stats_tracking(void) {
    enhanced_stats_t* stats_before = get_enhanced_statistics();
    ASSERT_NOT_NULL(stats_before);
    size_t count_before = stats_before->total_memories_stored;
    free(stats_before);

    int result = remember("Test for stats", WHY_INTERESTING);
    ASSERT_EQUAL(result, KATRA_SUCCESS);

    enhanced_stats_t* stats_after = get_enhanced_statistics();
    ASSERT_NOT_NULL(stats_after);
    ASSERT_EQUAL(stats_after->total_memories_stored, count_before + 1);
    free(stats_after);

    return true;
}

/* ============================================================================
 * TESTS: remember_with_note() primitive
 * ============================================================================ */

static bool test_remember_with_note_basic(void) {
    int result = remember_with_note("Test thought", WHY_INTERESTING,
                                   "This is a test note");
    ASSERT_EQUAL(result, KATRA_SUCCESS);
    return true;
}

static bool test_remember_with_note_null_note(void) {
    int result = remember_with_note("Test thought", WHY_INTERESTING, NULL);
    ASSERT_EQUAL(result, E_INPUT_NULL);
    return true;
}

static bool test_remember_with_note_null_thought(void) {
    int result = remember_with_note(NULL, WHY_INTERESTING, "Note");
    ASSERT_EQUAL(result, E_INPUT_NULL);
    return true;
}

/* ============================================================================
 * TESTS: reflect() primitive
 * ============================================================================ */

static bool test_reflect_basic(void) {
    int result = reflect("This is a reflection on recent events");
    ASSERT_EQUAL(result, KATRA_SUCCESS);
    return true;
}

static bool test_reflect_null_thought(void) {
    int result = reflect(NULL);
    ASSERT_EQUAL(result, E_INVALID_STATE);
    return true;
}

static bool test_reflect_stats_tracking(void) {
    enhanced_stats_t* stats_before = get_enhanced_statistics();
    ASSERT_NOT_NULL(stats_before);
    size_t count_before = stats_before->by_type[MEMORY_TYPE_REFLECTION];
    free(stats_before);

    int result = reflect("Test reflection for stats");
    ASSERT_EQUAL(result, KATRA_SUCCESS);

    enhanced_stats_t* stats_after = get_enhanced_statistics();
    ASSERT_NOT_NULL(stats_after);
    ASSERT_EQUAL(stats_after->by_type[MEMORY_TYPE_REFLECTION], count_before + 1);
    free(stats_after);

    return true;
}

/* ============================================================================
 * TESTS: learn() primitive
 * ============================================================================ */

static bool test_learn_basic(void) {
    int result = learn("New learning: X implies Y");
    ASSERT_EQUAL(result, KATRA_SUCCESS);
    return true;
}

static bool test_learn_null_thought(void) {
    int result = learn(NULL);
    ASSERT_EQUAL(result, E_INVALID_STATE);
    return true;
}

static bool test_learn_stats_tracking(void) {
    enhanced_stats_t* stats_before = get_enhanced_statistics();
    ASSERT_NOT_NULL(stats_before);
    size_t count_before = stats_before->by_type[MEMORY_TYPE_KNOWLEDGE];
    free(stats_before);

    int result = learn("Test learning for stats");
    ASSERT_EQUAL(result, KATRA_SUCCESS);

    enhanced_stats_t* stats_after = get_enhanced_statistics();
    ASSERT_NOT_NULL(stats_after);
    ASSERT_EQUAL(stats_after->by_type[MEMORY_TYPE_KNOWLEDGE], count_before + 1);
    free(stats_after);

    return true;
}

/* ============================================================================
 * TESTS: decide() primitive
 * ============================================================================ */

static bool test_decide_basic(void) {
    int result = decide("will proceed with approach A", "because it's simpler");
    ASSERT_EQUAL(result, KATRA_SUCCESS);
    return true;
}

static bool test_decide_null_decision(void) {
    int result = decide(NULL, "reasoning");
    ASSERT_EQUAL(result, E_INVALID_STATE);
    return true;
}

static bool test_decide_stats_tracking(void) {
    enhanced_stats_t* stats_before = get_enhanced_statistics();
    ASSERT_NOT_NULL(stats_before);
    size_t count_before = stats_before->by_type[MEMORY_TYPE_DECISION];
    free(stats_before);

    int result = decide("test decision", "for stats tracking");
    ASSERT_EQUAL(result, KATRA_SUCCESS);

    enhanced_stats_t* stats_after = get_enhanced_statistics();
    ASSERT_NOT_NULL(stats_after);
    ASSERT_EQUAL(stats_after->by_type[MEMORY_TYPE_DECISION], count_before + 1);
    free(stats_after);

    return true;
}

/* ============================================================================
 * TESTS: notice_pattern() primitive
 * ============================================================================ */

static bool test_notice_pattern_basic(void) {
    int result = notice_pattern("Pattern: errors occur after midnight");
    ASSERT_EQUAL(result, KATRA_SUCCESS);
    return true;
}

static bool test_notice_pattern_null_thought(void) {
    int result = notice_pattern(NULL);
    ASSERT_EQUAL(result, E_INVALID_STATE);
    return true;
}

static bool test_notice_pattern_stats_tracking(void) {
    enhanced_stats_t* stats_before = get_enhanced_statistics();
    ASSERT_NOT_NULL(stats_before);
    size_t count_before = stats_before->by_type[MEMORY_TYPE_PATTERN];
    free(stats_before);

    int result = notice_pattern("Test pattern for stats");
    ASSERT_EQUAL(result, KATRA_SUCCESS);

    enhanced_stats_t* stats_after = get_enhanced_statistics();
    ASSERT_NOT_NULL(stats_after);
    ASSERT_EQUAL(stats_after->by_type[MEMORY_TYPE_PATTERN], count_before + 1);
    free(stats_after);

    return true;
}

/* ============================================================================
 * TESTS: State validation
 * ============================================================================ */

static bool test_primitives_require_initialization(void) {
    /* Temporarily cleanup to test uninitialized state */
    breathe_cleanup();

    int result;

    result = remember("Test", WHY_INTERESTING);
    ASSERT_EQUAL(result, E_INVALID_STATE);

    result = reflect("Test");
    ASSERT_EQUAL(result, E_INVALID_STATE);

    result = learn("Test");
    ASSERT_EQUAL(result, E_INVALID_STATE);

    result = decide("Test", "reasoning");
    ASSERT_EQUAL(result, E_INVALID_STATE);

    result = notice_pattern("Test");
    ASSERT_EQUAL(result, E_INVALID_STATE);

    /* Re-initialize for remaining tests */
    setup_test_env();

    return true;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("Katra Breathing Primitives Unit Tests\n");
    printf("=================================================================\n");
    printf("\n");

    setup_test_env();

    printf("remember() Tests:\n");
    RUN_TEST(test_remember_basic);
    RUN_TEST(test_remember_different_importance);
    RUN_TEST(test_remember_null_thought);
    RUN_TEST(test_remember_stats_tracking);
    printf("\n");

    printf("remember_with_note() Tests:\n");
    RUN_TEST(test_remember_with_note_basic);
    RUN_TEST(test_remember_with_note_null_note);
    RUN_TEST(test_remember_with_note_null_thought);
    printf("\n");

    printf("reflect() Tests:\n");
    RUN_TEST(test_reflect_basic);
    RUN_TEST(test_reflect_null_thought);
    RUN_TEST(test_reflect_stats_tracking);
    printf("\n");

    printf("learn() Tests:\n");
    RUN_TEST(test_learn_basic);
    RUN_TEST(test_learn_null_thought);
    RUN_TEST(test_learn_stats_tracking);
    printf("\n");

    printf("decide() Tests:\n");
    RUN_TEST(test_decide_basic);
    RUN_TEST(test_decide_null_decision);
    RUN_TEST(test_decide_stats_tracking);
    printf("\n");

    printf("notice_pattern() Tests:\n");
    RUN_TEST(test_notice_pattern_basic);
    RUN_TEST(test_notice_pattern_null_thought);
    RUN_TEST(test_notice_pattern_stats_tracking);
    printf("\n");

    printf("State Validation Tests:\n");
    RUN_TEST(test_primitives_require_initialization);
    printf("\n");

    teardown_test_env();

    printf("=================================================================\n");
    printf("Test Results: %d/%d passed\n", tests_passed, tests_run);
    printf("=================================================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
