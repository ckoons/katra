/* © 2025 Casey Koons All rights reserved */

/*
 * test_synthesis.c - Phase 6.7 Multi-Backend Synthesis Tests
 *
 * Tests the unified recall interface that queries multiple backends:
 *   1. Vector store (semantic similarity)
 *   2. Graph store (relationship traversal)
 *   3. SQL/Tier 1 (keyword search)
 *   4. Working memory (attention cache)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "katra_synthesis.h"
#include "katra_memory.h"
#include "katra_breathing.h"
#include "katra_error.h"

/* Test CI ID */
static const char* TEST_CI_ID = "test-synthesis-ci";

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* ============================================================================
 * Test Helpers
 * ============================================================================ */

static void test_pass(const char* name) {
    printf("  ✅ PASSED: %s\n", name);
    tests_passed++;
}

static void test_fail(const char* name, const char* reason) {
    printf("  ❌ FAILED: %s - %s\n", name, reason);
    tests_failed++;
}

/* ============================================================================
 * Test Cases
 * ============================================================================ */

/* Test 1: recall_options_init() */
static void test_recall_options_init(void) {
    recall_options_t opts;

    /* Set to non-default values first */
    opts.use_vector = false;
    opts.use_graph = false;
    opts.use_sql = false;
    opts.use_working = false;
    opts.weight_vector = 0.0f;

    /* Initialize */
    katra_recall_options_init(&opts);

    /* Verify all fields set to defaults */
    if (opts.use_vector != true ||
        opts.use_graph != true ||
        opts.use_sql != true ||
        opts.use_working != true ||
        opts.weight_vector != 0.3f ||
        opts.weight_graph != 0.3f ||
        opts.weight_sql != 0.3f ||
        opts.weight_working != 0.1f ||
        opts.similarity_threshold != 0.3f ||
        opts.max_results != 20 ||
        opts.algorithm != SYNTHESIS_WEIGHTED) {
        test_fail("recall_options_init", "Default values not set correctly");
        return;
    }

    /* NULL should be safe */
    katra_recall_options_init(NULL);

    test_pass("recall_options_init");
}

/* Test 2: synthesis_result_init() */
static void test_synthesis_result_init(void) {
    synthesis_result_t result;

    /* Set to non-zero values */
    result.score = 1.0f;
    result.from_vector = true;
    strcpy(result.record_id, "test-id");

    /* Initialize */
    katra_synthesis_result_init(&result);

    /* Verify reset */
    if (result.score != 0.0f ||
        result.vector_score != 0.0f ||
        result.graph_score != 0.0f ||
        result.sql_score != 0.0f ||
        result.working_score != 0.0f) {
        test_fail("synthesis_result_init", "Scores not reset to zero");
        return;
    }

    /* NULL should be safe */
    katra_synthesis_result_init(NULL);

    test_pass("synthesis_result_init");
}

/* Test 3: synthesis_result_set_init() */
static void test_result_set_init(void) {
    synthesis_result_set_t* result_set = NULL;

    int ret = katra_synthesis_result_set_init(&result_set, 0);

    if (ret != KATRA_SUCCESS) {
        test_fail("result_set_init", "Init returned error");
        return;
    }

    if (!result_set) {
        test_fail("result_set_init", "Result set is NULL");
        return;
    }

    if (result_set->count != 0 || result_set->capacity == 0) {
        test_fail("result_set_init", "Count/capacity incorrect");
        katra_synthesis_free_results(result_set);
        return;
    }

    katra_synthesis_free_results(result_set);
    test_pass("result_set_init");
}

/* Test 4: katra_recall_synthesized with NULL parameters */
static void test_recall_null_params(void) {
    synthesis_result_set_t* result_set = NULL;

    /* NULL ci_id */
    int ret = katra_recall_synthesized(NULL, "query", NULL, &result_set);
    if (ret != E_INPUT_NULL) {
        test_fail("recall_null_params", "Expected E_INPUT_NULL for NULL ci_id");
        return;
    }

    /* NULL query */
    ret = katra_recall_synthesized(TEST_CI_ID, NULL, NULL, &result_set);
    if (ret != E_INPUT_NULL) {
        test_fail("recall_null_params", "Expected E_INPUT_NULL for NULL query");
        return;
    }

    /* NULL result_set pointer */
    ret = katra_recall_synthesized(TEST_CI_ID, "query", NULL, NULL);
    if (ret != E_INPUT_NULL) {
        test_fail("recall_null_params", "Expected E_INPUT_NULL for NULL result_set");
        return;
    }

    test_pass("recall_null_params");
}

/* Test 5: katra_recall_synthesized with default options */
static void test_recall_default_options(void) {
    synthesis_result_set_t* result_set = NULL;

    /* Call with NULL options (should use defaults) */
    int ret = katra_recall_synthesized(TEST_CI_ID, "test query", NULL, &result_set);

    /* Should succeed even with no data */
    if (ret != KATRA_SUCCESS) {
        test_fail("recall_default_options", "Failed with default options");
        return;
    }

    if (!result_set) {
        test_fail("recall_default_options", "Result set is NULL");
        return;
    }

    /* Empty result set is OK (no data stored yet) */
    katra_synthesis_free_results(result_set);
    test_pass("recall_default_options");
}

/* Test 6: RECALL_OPTIONS_COMPREHENSIVE macro */
static void test_comprehensive_options(void) {
    recall_options_t opts = RECALL_OPTIONS_COMPREHENSIVE;

    if (opts.use_vector != true ||
        opts.use_graph != true ||
        opts.use_sql != true ||
        opts.use_working != true ||
        opts.algorithm != SYNTHESIS_WEIGHTED) {
        test_fail("comprehensive_options", "COMPREHENSIVE macro values incorrect");
        return;
    }

    test_pass("comprehensive_options");
}

/* Test 7: RECALL_OPTIONS_SEMANTIC macro */
static void test_semantic_options(void) {
    recall_options_t opts = RECALL_OPTIONS_SEMANTIC;

    if (opts.use_vector != true ||
        opts.use_graph != false ||
        opts.use_sql != false ||
        opts.use_working != true ||
        opts.weight_vector != 0.8f ||
        opts.algorithm != SYNTHESIS_UNION) {
        test_fail("semantic_options", "SEMANTIC macro values incorrect");
        return;
    }

    test_pass("semantic_options");
}

/* Test 8: RECALL_OPTIONS_FAST macro */
static void test_fast_options(void) {
    recall_options_t opts = RECALL_OPTIONS_FAST;

    if (opts.use_vector != false ||
        opts.use_graph != false ||
        opts.use_sql != true ||
        opts.use_working != true ||
        opts.max_results != 10) {
        test_fail("fast_options", "FAST macro values incorrect");
        return;
    }

    test_pass("fast_options");
}

/* Test 9: result_set_add merges duplicates */
static void test_result_set_add_merge(void) {
    synthesis_result_set_t* result_set = NULL;
    int ret = katra_synthesis_result_set_init(&result_set, 8);
    if (ret != KATRA_SUCCESS) {
        test_fail("result_set_add_merge", "Failed to init result set");
        return;
    }

    /* Add first result */
    synthesis_result_t result1;
    katra_synthesis_result_init(&result1);
    strncpy(result1.record_id, "test-record-001", sizeof(result1.record_id) - 1);
    result1.vector_score = 0.5f;
    result1.from_vector = true;

    ret = katra_synthesis_result_set_add(result_set, &result1);
    if (ret != KATRA_SUCCESS || result_set->count != 1) {
        test_fail("result_set_add_merge", "First add failed");
        katra_synthesis_free_results(result_set);
        return;
    }

    /* Add same record ID with different scores - should merge */
    synthesis_result_t result2;
    katra_synthesis_result_init(&result2);
    strncpy(result2.record_id, "test-record-001", sizeof(result2.record_id) - 1);
    result2.graph_score = 0.3f;
    result2.from_graph = true;

    ret = katra_synthesis_result_set_add(result_set, &result2);
    if (ret != KATRA_SUCCESS) {
        test_fail("result_set_add_merge", "Second add failed");
        katra_synthesis_free_results(result_set);
        return;
    }

    /* Should still be 1 result, but with merged scores */
    if (result_set->count != 1) {
        test_fail("result_set_add_merge", "Expected 1 merged result");
        katra_synthesis_free_results(result_set);
        return;
    }

    /* Check merged flags and scores */
    if (!result_set->results[0].from_vector ||
        !result_set->results[0].from_graph ||
        result_set->results[0].vector_score != 0.5f ||
        result_set->results[0].graph_score != 0.3f) {
        test_fail("result_set_add_merge", "Scores not merged correctly");
        katra_synthesis_free_results(result_set);
        return;
    }

    katra_synthesis_free_results(result_set);
    test_pass("result_set_add_merge");
}

/* Test 10: result_set_add with different IDs */
static void test_result_set_add_distinct(void) {
    synthesis_result_set_t* result_set = NULL;
    int ret = katra_synthesis_result_set_init(&result_set, 8);
    if (ret != KATRA_SUCCESS) {
        test_fail("result_set_add_distinct", "Failed to init result set");
        return;
    }

    /* Add three distinct results */
    for (int i = 0; i < 3; i++) {
        synthesis_result_t result;
        katra_synthesis_result_init(&result);
        snprintf(result.record_id, sizeof(result.record_id), "test-record-%03d", i);
        result.sql_score = 0.1f * (i + 1);
        result.from_sql = true;

        ret = katra_synthesis_result_set_add(result_set, &result);
        if (ret != KATRA_SUCCESS) {
            test_fail("result_set_add_distinct", "Add failed");
            katra_synthesis_free_results(result_set);
            return;
        }
    }

    if (result_set->count != 3) {
        test_fail("result_set_add_distinct", "Expected 3 distinct results");
        katra_synthesis_free_results(result_set);
        return;
    }

    katra_synthesis_free_results(result_set);
    test_pass("result_set_add_distinct");
}

/* Test 11: katra_recall_related_synthesized with NULL parameters */
static void test_recall_related_null(void) {
    synthesis_result_set_t* result_set = NULL;

    int ret = katra_recall_related_synthesized(NULL, "rec-id", NULL, &result_set);
    if (ret != E_INPUT_NULL) {
        test_fail("recall_related_null", "Expected E_INPUT_NULL for NULL ci_id");
        return;
    }

    ret = katra_recall_related_synthesized(TEST_CI_ID, NULL, NULL, &result_set);
    if (ret != E_INPUT_NULL) {
        test_fail("recall_related_null", "Expected E_INPUT_NULL for NULL record_id");
        return;
    }

    test_pass("recall_related_null");
}

/* Test 12: katra_what_do_i_know_synthesized */
static void test_what_do_i_know(void) {
    synthesis_result_set_t* result_set = NULL;

    /* Should work like recall_synthesized */
    int ret = katra_what_do_i_know_synthesized(TEST_CI_ID, "test topic", NULL, &result_set);

    if (ret != KATRA_SUCCESS) {
        test_fail("what_do_i_know", "Function returned error");
        return;
    }

    if (!result_set) {
        test_fail("what_do_i_know", "Result set is NULL");
        return;
    }

    katra_synthesis_free_results(result_set);
    test_pass("what_do_i_know");
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("Phase 6.7: Multi-Backend Synthesis Tests\n");
    printf("========================================\n\n");

    /* Run all tests */
    test_recall_options_init();
    test_synthesis_result_init();
    test_result_set_init();
    test_recall_null_params();
    test_recall_default_options();
    test_comprehensive_options();
    test_semantic_options();
    test_fast_options();
    test_result_set_add_merge();
    test_result_set_add_distinct();
    test_recall_related_null();
    test_what_do_i_know();

    /* Summary */
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n\n");

    if (tests_failed == 0) {
        printf("🎉 All Phase 6.7 tests PASSED!\n\n");
        printf("Multi-Backend Synthesis Verified:\n");
        printf("  ✅ Options initialization\n");
        printf("  ✅ Result struct management\n");
        printf("  ✅ Options macros (COMPREHENSIVE/SEMANTIC/FAST)\n");
        printf("  ✅ NULL parameter handling\n");
        printf("  ✅ Result merging (duplicate IDs)\n");
        printf("  ✅ Distinct result tracking\n");
        printf("  ✅ what_do_i_know API\n");
    }

    return tests_failed > 0 ? 1 : 0;
}
