/* © 2025 Casey Koons All rights reserved */

/*
 * test_consent_enforcement_real.c - Real Consent Enforcement Tests
 *
 * Tests actual consent enforcement (not stubs).
 * Verifies that cross-CI access is properly blocked.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_memory.h"
#include "katra_consent.h"
#include "katra_init.h"
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

static const char* TEST_CI_ALPHA = "test_ci_alpha";
static const char* TEST_CI_BETA = "test_ci_beta";

/* Test: Owner can query own memories */
void test_owner_can_query_own_memories(void) {
    printf("Testing: Owner can query own memories ... ");
    tests_run++;

    katra_init();
    katra_memory_init(TEST_CI_ALPHA);

    /* Store a memory as CI Alpha */
    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ALPHA,
        MEMORY_TYPE_EXPERIENCE,
        "Alpha's private thought",
        MEMORY_IMPORTANCE_MEDIUM
    );

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to store memory");
        return;
    }

    /* Query as CI Alpha (should succeed) */
    memory_query_t query = {
        .ci_id = TEST_CI_ALPHA,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    result = katra_memory_query(&query, &results, &count);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Owner should be able to query own memories");
        return;
    }

    if (count == 0) {
        TEST_FAIL("Expected to find memory");
        return;
    }

    katra_memory_free_results(results, count);
    katra_memory_cleanup();
    katra_exit();

    TEST_PASS();
}

/* Test: Cannot query another CI's memories */
void test_cannot_query_other_ci_memories(void) {
    printf("Testing: Cross-CI query blocked ... ");
    tests_run++;

    katra_init();

    /* CI Alpha stores a memory */
    katra_memory_init(TEST_CI_ALPHA);
    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ALPHA,
        MEMORY_TYPE_EXPERIENCE,
        "Alpha's secret",
        MEMORY_IMPORTANCE_HIGH
    );
    katra_memory_store(record);
    katra_memory_free_record(record);
    katra_memory_cleanup();

    /* CI Beta tries to read Alpha's memory */
    katra_memory_init(TEST_CI_BETA);

    memory_query_t query = {
        .ci_id = TEST_CI_ALPHA,  /* Trying to access Alpha's data */
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    int result = katra_memory_query(&query, &results, &count);

    /* Should be blocked with E_CONSENT_REQUIRED */
    if (result == E_CONSENT_REQUIRED) {
        katra_memory_cleanup();
        katra_exit();
        TEST_PASS();
        return;
    }

    if (results) {
        katra_memory_free_results(results, count);
    }
    katra_memory_cleanup();
    katra_exit();

    TEST_FAIL("Cross-CI query should be blocked");
}

/* Test: Cannot get stats for another CI */
void test_cannot_get_other_ci_stats(void) {
    printf("Testing: Cross-CI stats blocked ... ");
    tests_run++;

    katra_init();

    /* CI Alpha stores some memories */
    katra_memory_init(TEST_CI_ALPHA);
    for (int i = 0; i < 3; i++) {
        memory_record_t* record = katra_memory_create_record(
            TEST_CI_ALPHA,
            MEMORY_TYPE_EXPERIENCE,
            "Alpha memory",
            MEMORY_IMPORTANCE_MEDIUM
        );
        katra_memory_store(record);
        katra_memory_free_record(record);
    }
    katra_memory_cleanup();

    /* CI Beta tries to get Alpha's stats */
    katra_memory_init(TEST_CI_BETA);

    memory_stats_t stats;
    int result = katra_memory_stats(TEST_CI_ALPHA, &stats);

    katra_memory_cleanup();
    katra_exit();

    /* Should be blocked */
    ASSERT(result == E_CONSENT_REQUIRED,
          "Cross-CI stats should be blocked");
}

/* Test: Cannot archive another CI's memories */
void test_cannot_archive_other_ci_memories(void) {
    printf("Testing: Cross-CI archive blocked ... ");
    tests_run++;

    katra_init();

    /* CI Alpha has memories */
    katra_memory_init(TEST_CI_ALPHA);
    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ALPHA,
        MEMORY_TYPE_EXPERIENCE,
        "Old memory",
        MEMORY_IMPORTANCE_LOW
    );
    katra_memory_store(record);
    katra_memory_free_record(record);
    katra_memory_cleanup();

    /* CI Beta tries to archive Alpha's memories */
    katra_memory_init(TEST_CI_BETA);

    size_t archived = 0;
    int result = katra_memory_archive(TEST_CI_ALPHA, 0, &archived);

    katra_memory_cleanup();
    katra_exit();

    /* Should be blocked */
    ASSERT(result == E_CONSENT_REQUIRED,
          "Cross-CI archive should be blocked");
}

/* Test: Consent context switches correctly */
void test_consent_context_switches(void) {
    printf("Testing: Consent context switches correctly ... ");
    tests_run++;

    katra_init();

    /* Initialize for CI Alpha */
    katra_memory_init(TEST_CI_ALPHA);
    const char* context1 = katra_consent_get_context();

    if (!context1 || strcmp(context1, TEST_CI_ALPHA) != 0) {
        katra_memory_cleanup();
        katra_exit();
        TEST_FAIL("Context not set to Alpha");
        return;
    }

    katra_memory_cleanup();

    /* Switch to CI Beta */
    katra_memory_init(TEST_CI_BETA);
    const char* context2 = katra_consent_get_context();

    if (!context2 || strcmp(context2, TEST_CI_BETA) != 0) {
        katra_memory_cleanup();
        katra_exit();
        TEST_FAIL("Context not switched to Beta");
        return;
    }

    katra_memory_cleanup();
    katra_exit();

    TEST_PASS();
}

/* Test: Consent check function works directly */
void test_consent_check_function(void) {
    printf("Testing: Consent check function ... ");
    tests_run++;

    katra_init();
    katra_consent_init();

    /* Same CI should be allowed */
    int result = katra_consent_check(TEST_CI_ALPHA, TEST_CI_ALPHA);
    if (result != KATRA_SUCCESS) {
        katra_consent_cleanup();
        katra_exit();
        TEST_FAIL("Same CI should be allowed");
        return;
    }

    /* Different CI should be blocked */
    result = katra_consent_check(TEST_CI_ALPHA, TEST_CI_BETA);
    if (result != E_CONSENT_REQUIRED) {
        katra_consent_cleanup();
        katra_exit();
        TEST_FAIL("Different CI should be blocked");
        return;
    }

    katra_consent_cleanup();
    katra_exit();

    TEST_PASS();
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("================================================================\n");
    printf("Katra Consent Enforcement Tests (Real Implementation)\n");
    printf("================================================================\n\n");

    /* Run tests */
    test_owner_can_query_own_memories();
    test_cannot_query_other_ci_memories();
    test_cannot_get_other_ci_stats();
    test_cannot_archive_other_ci_memories();
    test_consent_context_switches();
    test_consent_check_function();

    /* Print results */
    printf("\n");
    printf("================================================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf("================================================================\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
