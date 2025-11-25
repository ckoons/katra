/* © 2025 Casey Koons All rights reserved */

/*
 * test_universal_encoder.c - Phase 6.6 Universal Encoder Tests
 *
 * Tests the unified memory encoding interface that writes to:
 *   1. Tier 1 core memory
 *   2. Vector store (semantic embeddings)
 *   3. Graph store (memory associations)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "katra_universal_encoder.h"
#include "katra_memory.h"
#include "katra_breathing.h"
#include "katra_vector.h"
#include "katra_graph.h"
#include "katra_error.h"

/* Test CI ID */
static const char* TEST_CI_ID = "test-encoder-ci";

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

/* Test 1: encode_result_init() */
static void test_encode_result_init(void) {
    encode_result_t result;

    /* Set to non-zero values first */
    result.memory_stored = true;
    result.vector_created = true;
    result.edges_created = true;
    result.edge_count = 99;
    strcpy(result.record_id, "test-id");
    result.error_code = 42;

    /* Initialize */
    encode_result_init(&result);

    /* Verify all fields reset */
    if (result.memory_stored != false ||
        result.vector_created != false ||
        result.edges_created != false ||
        result.edge_count != 0 ||
        result.record_id[0] != '\0' ||
        result.error_code != 0) {
        test_fail("encode_result_init", "Fields not properly reset");
        return;
    }

    /* NULL should be safe */
    encode_result_init(NULL);

    test_pass("encode_result_init");
}

/* Test 2: katra_universal_encode with NULL record */
static void test_encode_null_record(void) {
    encode_result_t result;
    encode_result_init(&result);

    int ret = katra_universal_encode(NULL, NULL, NULL, NULL, NULL, &result);

    if (ret == E_INPUT_NULL) {
        test_pass("encode_null_record");
    } else {
        test_fail("encode_null_record", "Expected E_INPUT_NULL for NULL record");
    }
}

/* Test 3: katra_universal_encode with NULL content */
static void test_encode_null_content(void) {
    memory_record_t record;
    memset(&record, 0, sizeof(record));
    record.content = NULL;  /* NULL content */

    encode_result_t result;
    encode_result_init(&result);

    int ret = katra_universal_encode(&record, NULL, NULL, NULL, NULL, &result);

    if (ret == E_INPUT_NULL) {
        test_pass("encode_null_content");
    } else {
        test_fail("encode_null_content", "Expected E_INPUT_NULL for NULL content");
    }
}

/* Test 4: Default options initialization */
static void test_default_options(void) {
    encode_options_t opts = ENCODE_OPTIONS_DEFAULT;

    if (opts.skip_vector == false &&
        opts.skip_graph == false &&
        opts.require_all == false) {
        test_pass("default_options");
    } else {
        test_fail("default_options", "ENCODE_OPTIONS_DEFAULT has wrong values");
    }
}

/* Test 5: Strict options initialization */
static void test_strict_options(void) {
    encode_options_t opts = ENCODE_OPTIONS_STRICT;

    if (opts.skip_vector == false &&
        opts.skip_graph == false &&
        opts.require_all == true) {
        test_pass("strict_options");
    } else {
        test_fail("strict_options", "ENCODE_OPTIONS_STRICT has wrong values");
    }
}

/* Test 6: Fast options initialization */
static void test_fast_options(void) {
    encode_options_t opts = ENCODE_OPTIONS_FAST;

    if (opts.skip_vector == true &&
        opts.skip_graph == true &&
        opts.require_all == false) {
        test_pass("fast_options");
    } else {
        test_fail("fast_options", "ENCODE_OPTIONS_FAST has wrong values");
    }
}

/* Test 7: Full encoding with stores (requires breathing layer) */
static void test_full_encode_with_breathing(void) {
    /* Initialize breathing layer for test */
    int init_result = breathe_init(TEST_CI_ID);
    if (init_result != KATRA_SUCCESS) {
        test_fail("full_encode_with_breathing", "Failed to init breathing layer");
        return;
    }

    /* Create a memory record */
    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_EXPERIENCE,
        "Testing universal encoder with full pipeline",
        0.8f
    );

    if (!record) {
        test_fail("full_encode_with_breathing", "Failed to create record");
        breathe_cleanup();
        return;
    }

    /* Use the simple API which accesses global stores internally */
    int ret = katra_universal_encode_simple(record);

    /* Cleanup */
    katra_memory_free_record(record);
    breathe_cleanup();

    if (ret == KATRA_SUCCESS) {
        test_pass("full_encode_with_breathing");
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "ret=%d", ret);
        test_fail("full_encode_with_breathing", msg);
    }
}

/* Test 8: Simple encode API */
static void test_simple_encode(void) {
    /* Initialize breathing layer */
    int init_result = breathe_init(TEST_CI_ID);
    if (init_result != KATRA_SUCCESS) {
        test_fail("simple_encode", "Failed to init breathing layer");
        return;
    }

    /* Create record */
    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_DECISION,
        "Testing simple encode API",
        0.9f
    );

    if (!record) {
        test_fail("simple_encode", "Failed to create record");
        breathe_cleanup();
        return;
    }

    /* Use simple API */
    int ret = katra_universal_encode_simple(record);

    /* Cleanup */
    katra_memory_free_record(record);
    breathe_cleanup();

    if (ret == KATRA_SUCCESS) {
        test_pass("simple_encode");
    } else {
        char msg[64];
        snprintf(msg, sizeof(msg), "ret=%d", ret);
        test_fail("simple_encode", msg);
    }
}

/* Test 9: Encode with skip options (fast mode) */
static void test_encode_fast_mode(void) {
    /* Initialize breathing layer */
    int init_result = breathe_init(TEST_CI_ID);
    if (init_result != KATRA_SUCCESS) {
        test_fail("encode_fast_mode", "Failed to init breathing layer");
        return;
    }

    /* Create record */
    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_EXPERIENCE,
        "Testing fast mode (skip vector/graph)",
        0.5f
    );

    if (!record) {
        test_fail("encode_fast_mode", "Failed to create record");
        breathe_cleanup();
        return;
    }

    /* Encode with FAST options (skips vector and graph) */
    encode_options_t opts = ENCODE_OPTIONS_FAST;
    encode_result_t result;

    /* Using NULL stores is equivalent - test that memory still stores */
    int ret = katra_universal_encode(
        record,
        NULL,   /* Skip vector store */
        NULL,   /* Skip graph store */
        NULL,   /* No config needed */
        &opts,
        &result
    );

    /* Cleanup */
    katra_memory_free_record(record);
    breathe_cleanup();

    /* Memory should be stored, vector and edges should NOT be created */
    if (ret == KATRA_SUCCESS && result.memory_stored &&
        !result.vector_created && !result.edges_created) {
        test_pass("encode_fast_mode");
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "ret=%d, mem=%d, vec=%d, edges=%d",
                 ret, result.memory_stored, result.vector_created, result.edges_created);
        test_fail("encode_fast_mode", msg);
    }
}

/* Test 10: Encode with NULL stores (should still store memory) */
static void test_encode_null_stores(void) {
    /* Initialize breathing for storage backend */
    int init_result = breathe_init(TEST_CI_ID);
    if (init_result != KATRA_SUCCESS) {
        test_fail("encode_null_stores", "Failed to init breathing layer");
        return;
    }

    /* Create record */
    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_EXPERIENCE,
        "Testing with NULL vector and graph stores",
        0.6f
    );

    if (!record) {
        test_fail("encode_null_stores", "Failed to create record");
        breathe_cleanup();
        return;
    }

    /* Encode with NULL stores */
    encode_result_t result;
    int ret = katra_universal_encode(
        record,
        NULL,   /* No vector store */
        NULL,   /* No graph store */
        NULL,   /* No config */
        NULL,   /* Default options */
        &result
    );

    /* Cleanup */
    katra_memory_free_record(record);
    breathe_cleanup();

    /* Memory should still be stored even without vector/graph */
    if (ret == KATRA_SUCCESS && result.memory_stored &&
        !result.vector_created && !result.edges_created) {
        test_pass("encode_null_stores");
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "ret=%d, mem=%d, vec=%d, edges=%d",
                 ret, result.memory_stored, result.vector_created, result.edges_created);
        test_fail("encode_null_stores", msg);
    }
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("Phase 6.6: Universal Encoder Tests\n");
    printf("========================================\n\n");

    /* Run all tests */
    test_encode_result_init();
    test_encode_null_record();
    test_encode_null_content();
    test_default_options();
    test_strict_options();
    test_fast_options();
    test_full_encode_with_breathing();
    test_simple_encode();
    test_encode_fast_mode();
    test_encode_null_stores();

    /* Summary */
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    if (tests_failed == 0) {
        printf("\n🎉 All Phase 6.6 tests PASSED!\n\n");
        printf("Universal Encoder Verified:\n");
        printf("  ✅ Result struct initialization\n");
        printf("  ✅ NULL parameter handling\n");
        printf("  ✅ Options macros (DEFAULT/STRICT/FAST)\n");
        printf("  ✅ Full encode with breathing layer\n");
        printf("  ✅ Simple encode API\n");
        printf("  ✅ Skip options\n");
        printf("  ✅ Graceful degradation (NULL stores)\n");
        return 0;
    } else {
        printf("\n❌ Some tests FAILED\n");
        return 1;
    }
}
