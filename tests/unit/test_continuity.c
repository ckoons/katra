/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Project includes */
#include "katra_continuity.h"
#include "katra_tier1.h"
#include "katra_tier2.h"
#include "katra_init.h"
#include "katra_error.h"
#include "katra_log.h"

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

/* Test CI ID - use timestamp to make it unique per test run */
static char TEST_CI_ID[64];

/* Test: Get daily stats with no memories */
void test_get_daily_stats_empty(void) {
    printf("Testing: Get daily stats (no memories) ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    daily_stats_t stats;
    int result = katra_get_daily_stats(TEST_CI_ID, &stats);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("katra_get_daily_stats() failed");
        tier1_cleanup();
        return;
    }

    if (stats.interaction_count != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected 0 interactions, got %d",
                stats.interaction_count);
        TEST_FAIL(msg);
        tier1_cleanup();
        return;
    }

    tier1_cleanup();
    TEST_PASS();
}

/* Test: Get daily stats with NULL parameters */
void test_get_daily_stats_null(void) {
    printf("Testing: Get daily stats with NULL ... ");
    tests_run++;

    daily_stats_t stats;

    /* NULL ci_id */
    int result = katra_get_daily_stats(NULL, &stats);
    if (result != E_INPUT_NULL) {
        TEST_FAIL("Should fail with NULL ci_id");
        return;
    }

    /* NULL stats */
    result = katra_get_daily_stats(TEST_CI_ID, NULL);
    if (result != E_INPUT_NULL) {
        TEST_FAIL("Should fail with NULL stats");
        return;
    }

    TEST_PASS();
}

/* Test: Get daily stats with memories */
void test_get_daily_stats_with_memories(void) {
    printf("Testing: Get daily stats with memories ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    /* Create some test memories for today */
    memory_record_t* rec1 = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_INTERACTION,
        "How do I implement sunrise/sunset?",
        MEMORY_IMPORTANCE_HIGH
    );

    memory_record_t* rec2 = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_INTERACTION,
        "What is the difference between Tier 1 and Tier 2?",
        MEMORY_IMPORTANCE_MEDIUM
    );

    if (!rec1 || !rec2) {
        TEST_FAIL("Failed to create test memories");
        katra_memory_free_record(rec1);
        katra_memory_free_record(rec2);
        tier1_cleanup();
        return;
    }

    tier1_store(rec1);
    tier1_store(rec2);

    katra_memory_free_record(rec1);
    katra_memory_free_record(rec2);

    /* Get stats */
    daily_stats_t stats;
    int result = katra_get_daily_stats(TEST_CI_ID, &stats);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("katra_get_daily_stats() failed");
        tier1_cleanup();
        return;
    }

    if (stats.interaction_count != 2) {
        TEST_FAIL("Expected 2 interactions");
        tier1_cleanup();
        return;
    }

    if (stats.questions_asked != 2) {
        TEST_FAIL("Expected 2 questions (count '?' marks)");
        tier1_cleanup();
        return;
    }

    tier1_cleanup();
    TEST_PASS();
}

/* Test: Sundown with NULL ci_id */
void test_sundown_null(void) {
    printf("Testing: Sundown with NULL ci_id ... ");
    tests_run++;

    int result = katra_sundown_basic(NULL, NULL);
    ASSERT(result == E_INPUT_NULL, "Should fail with NULL ci_id");
}

/* Test: Sundown with no memories (first day) */
void test_sundown_first_day(void) {
    printf("Testing: Sundown first day (no memories) ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);
    tier2_init(TEST_CI_ID);

    int result = katra_sundown_basic(TEST_CI_ID, NULL);

    tier1_cleanup();
    tier2_cleanup();

    ASSERT(result == KATRA_SUCCESS,
           "Sundown should succeed even with no memories");
}

/* Test: Sundown with custom summary */
void test_sundown_custom_summary(void) {
    printf("Testing: Sundown with custom summary ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);
    tier2_init(TEST_CI_ID);

    const char* custom_summary = "Today was productive! Completed 5 tasks.";
    int result = katra_sundown_basic(TEST_CI_ID, custom_summary);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Sundown with custom summary failed");
        tier1_cleanup();
        tier2_cleanup();
        return;
    }

    /* Verify digest was created by querying all digests for this CI */
    digest_query_t query = {
        .ci_id = TEST_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .period_type = (period_type_t)-1,
        .theme = NULL,
        .keyword = NULL,
        .digest_type = DIGEST_TYPE_INTERACTION,
        .limit = 10
    };

    digest_record_t** results = NULL;
    size_t count = 0;
    result = tier2_query(&query, &results, &count);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("tier2_query() failed");
        tier1_cleanup();
        tier2_cleanup();
        return;
    }

    if (count == 0) {
        TEST_FAIL("No digests found after sundown");
        tier1_cleanup();
        tier2_cleanup();
        return;
    }

    /* Find digest with our custom summary */
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (results[i]->summary &&
            strstr(results[i]->summary, "productive") != NULL) {
            found = true;
            break;
        }
    }

    if (!found) {
        /* Debug: print what we found */
        char msg[256];
        snprintf(msg, sizeof(msg),
                "Custom summary not found. Got %zu digests. First summary: %s",
                count, (count > 0 && results[0]->summary) ? results[0]->summary : "(null)");
        katra_digest_free_results(results, count);
        tier1_cleanup();
        tier2_cleanup();
        TEST_FAIL(msg);
        return;
    }

    katra_digest_free_results(results, count);
    tier1_cleanup();
    tier2_cleanup();
    TEST_PASS();
}

/* Test: Sunrise with NULL parameters */
void test_sunrise_null(void) {
    printf("Testing: Sunrise with NULL parameters ... ");
    tests_run++;

    digest_record_t* digest = NULL;

    /* NULL ci_id */
    int result = katra_sunrise_basic(NULL, &digest);
    if (result != E_INPUT_NULL) {
        TEST_FAIL("Should fail with NULL ci_id");
        return;
    }

    /* NULL digest */
    result = katra_sunrise_basic(TEST_CI_ID, NULL);
    if (result != E_INPUT_NULL) {
        TEST_FAIL("Should fail with NULL digest");
        return;
    }

    TEST_PASS();
}

/* Test: Sunrise first day (no previous day) */
void test_sunrise_first_day(void) {
    printf("Testing: Sunrise first day (no history) ... ");
    tests_run++;

    tier2_init(TEST_CI_ID);

    digest_record_t* digest = NULL;
    int result = katra_sunrise_basic(TEST_CI_ID, &digest);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Sunrise should succeed even with no history");
        tier2_cleanup();
        return;
    }

    if (digest != NULL) {
        TEST_FAIL("Should have NULL digest on first day");
        katra_digest_free(digest);
        tier2_cleanup();
        return;
    }

    tier2_cleanup();
    TEST_PASS();
}

/* Test: Sundown then sunrise workflow */
void test_sundown_sunrise_workflow(void) {
    printf("Testing: Sundown → Sunrise workflow ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);
    tier2_init(TEST_CI_ID);

    /* Create memories for today */
    memory_record_t* rec = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_INTERACTION,
        "Test interaction for continuity",
        MEMORY_IMPORTANCE_MEDIUM
    );

    if (!rec) {
        TEST_FAIL("Failed to create test memory");
        tier1_cleanup();
        tier2_cleanup();
        return;
    }

    tier1_store(rec);
    katra_memory_free_record(rec);

    /* Run sundown */
    int result = katra_sundown_basic(TEST_CI_ID, "End of day test");
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Sundown failed");
        tier1_cleanup();
        tier2_cleanup();
        return;
    }

    /* Run sunrise */
    digest_record_t* digest = NULL;
    result = katra_sunrise_basic(TEST_CI_ID, &digest);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Sunrise failed");
        tier1_cleanup();
        tier2_cleanup();
        return;
    }

    /* Note: digest might be NULL if query doesn't find today's summary
     * (since we're querying for YESTERDAY's summary) */
    if (digest) {
        katra_digest_free(digest);
    }

    tier1_cleanup();
    tier2_cleanup();
    TEST_PASS();
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Continuity (Sunrise/Sunset) Tests\n");
    printf("========================================\n\n");

    /* Create unique CI ID for this test run */
    snprintf(TEST_CI_ID, sizeof(TEST_CI_ID), "test_ci_cont_%ld", time(NULL));

    /* Initialize katra first */
    katra_init();

    /* Run tests */
    test_get_daily_stats_null();
    test_get_daily_stats_empty();
    test_get_daily_stats_with_memories();
    test_sundown_null();
    test_sundown_first_day();
    test_sundown_custom_summary();
    test_sunrise_null();
    test_sunrise_first_day();
    test_sundown_sunrise_workflow();

    /* Cleanup */
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
