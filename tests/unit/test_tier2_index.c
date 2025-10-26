/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_error.h"
#include "katra_tier2.h"
#include "katra_tier2_index.h"
#include "katra_init.h"

#define TEST_CI_ID "test_ci_index"

/* Test macros */
#define TEST_PASS() do { \
    printf(" ✓\n"); \
    tests_passed++; \
} while (0)

#define TEST_FAIL(msg) do { \
    printf(" ✗\nFailed: %s\n", msg); \
    tests_failed++; \
} while (0)

#define TEST_RUN(test) do { \
    printf("Testing: %s ... ", #test); \
    test(); \
} while (0)

#define TEST_SUITE_START(name) do { \
    printf("\n========================================\n"); \
    printf("%s\n", name); \
    printf("========================================\n\n"); \
} while (0)

#define TEST_SUITE_END() do { \
    printf("\n========================================\n"); \
    printf("Test Results:\n"); \
    printf("  Tests run:    %d\n", tests_passed + tests_failed); \
    printf("  Tests passed: %d\n", tests_passed); \
    printf("  Tests failed: %d\n", tests_failed); \
    printf("========================================\n\n"); \
} while (0)

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test: Index initialization */
void test_index_init(void) {
    int result = tier2_index_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to initialize Tier 2 index");
        return;
    }

    /* Check that index exists */
    if (!tier2_index_exists(TEST_CI_ID)) {
        TEST_FAIL("Index does not exist after initialization");
        return;
    }

    tier2_index_cleanup();
    TEST_PASS();
}

/* Test: Index store and query */
void test_index_store_and_query(void) {
    int result;

    /* Initialize systems */
    result = tier2_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to initialize Tier 2");
        return;
    }

    /* Create and store a test digest */
    digest_record_t* digest = katra_digest_create(TEST_CI_ID,
                                                   PERIOD_TYPE_WEEKLY,
                                                   "2025-W43",
                                                   DIGEST_TYPE_LEARNING);
    if (!digest) {
        TEST_FAIL("Failed to create digest");
        return;
    }

    digest->summary = strdup("Test digest for index testing");

    result = tier2_store_digest(digest);
    if (result != KATRA_SUCCESS) {
        katra_digest_free(digest);
        TEST_FAIL("Failed to store digest");
        return;
    }

    /* Query using the index */
    digest_query_t query = {
        .ci_id = TEST_CI_ID,
        .period_type = PERIOD_TYPE_WEEKLY,
        .digest_type = DIGEST_TYPE_LEARNING,
        .limit = 10
    };

    digest_record_t** results = NULL;
    size_t count = 0;

    result = tier2_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        katra_digest_free(digest);
        TEST_FAIL("Failed to query digests");
        return;
    }

    if (count == 0) {
        katra_digest_free(digest);
        katra_digest_free_results(results, count);
        TEST_FAIL("No results found after storing digest");
        return;
    }

    katra_digest_free(digest);
    katra_digest_free_results(results, count);
    tier2_cleanup();
    TEST_PASS();
}

/* Test: Index rebuild */
void test_index_rebuild(void) {
    int result;

    /* Initialize and store some digests */
    result = tier2_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to initialize Tier 2");
        return;
    }

    /* Store a few digests */
    for (int i = 0; i < 3; i++) {
        char period_id[32];
        snprintf(period_id, sizeof(period_id), "2025-W%d", 40 + i);

        digest_record_t* digest = katra_digest_create(TEST_CI_ID,
                                                       PERIOD_TYPE_WEEKLY,
                                                       period_id,
                                                       DIGEST_TYPE_PROJECT);
        if (digest) {
            digest->summary = strdup("Test digest");
            tier2_store_digest(digest);
            katra_digest_free(digest);
        }
    }

    /* Rebuild the index */
    int indexed = tier2_index_rebuild(TEST_CI_ID);
    if (indexed < 0) {
        tier2_cleanup();
        TEST_FAIL("Index rebuild failed");
        return;
    }

    if (indexed < 3) {
        tier2_cleanup();
        TEST_FAIL("Index rebuild did not index all digests");
        return;
    }

    tier2_cleanup();
    TEST_PASS();
}

/* Test: Index statistics */
void test_index_stats(void) {
    int result;
    size_t digest_count = 0;
    size_t theme_count = 0;
    size_t keyword_count = 0;

    /* Initialize */
    result = tier2_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to initialize Tier 2");
        return;
    }

    /* Store a digest */
    digest_record_t* digest = katra_digest_create(TEST_CI_ID,
                                                   PERIOD_TYPE_WEEKLY,
                                                   "2025-W44",
                                                   DIGEST_TYPE_MIXED);
    if (digest) {
        digest->summary = strdup("Test digest for stats");
        tier2_store_digest(digest);
        katra_digest_free(digest);
    }

    /* Get statistics */
    result = tier2_index_stats(TEST_CI_ID, &digest_count, &theme_count, &keyword_count);
    if (result != KATRA_SUCCESS) {
        tier2_cleanup();
        TEST_FAIL("Failed to get index statistics");
        return;
    }

    if (digest_count == 0) {
        tier2_cleanup();
        TEST_FAIL("Index stats shows 0 digests after storing");
        return;
    }

    tier2_cleanup();
    TEST_PASS();
}

/* Test: Index query with time range */
void test_index_query_time_range(void) {
    int result;

    /* Initialize */
    result = tier2_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to initialize Tier 2");
        return;
    }

    /* Store digests with different timestamps */
    time_t now = time(NULL);
    time_t yesterday = now - (24 * 60 * 60);
    time_t week_ago = now - (7 * 24 * 60 * 60);

    for (int i = 0; i < 3; i++) {
        char period_id[32];
        snprintf(period_id, sizeof(period_id), "2025-W%d", 45 + i);

        digest_record_t* digest = katra_digest_create(TEST_CI_ID,
                                                       PERIOD_TYPE_WEEKLY,
                                                       period_id,
                                                       DIGEST_TYPE_INTERACTION);
        if (digest) {
            digest->timestamp = (i == 0) ? week_ago : ((i == 1) ? yesterday : now);
            digest->summary = strdup("Test digest");
            tier2_store_digest(digest);
            katra_digest_free(digest);
        }
    }

    /* Query for recent digests only */
    digest_query_t query = {
        .ci_id = TEST_CI_ID,
        .start_time = yesterday - 3600,  /* Start from just before yesterday */
        .digest_type = DIGEST_TYPE_INTERACTION,
        .limit = 10
    };

    digest_record_t** results = NULL;
    size_t count = 0;

    result = tier2_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        tier2_cleanup();
        TEST_FAIL("Failed to query with time range");
        return;
    }

    /* Should get 2 results (yesterday and today, not week_ago) */
    if (count < 2) {
        katra_digest_free_results(results, count);
        tier2_cleanup();
        TEST_FAIL("Time range query returned too few results");
        return;
    }

    katra_digest_free_results(results, count);
    tier2_cleanup();
    TEST_PASS();
}

/* Main test runner */
int main(void) {
    int result;

    TEST_SUITE_START("Katra Tier 2 Index Tests");

    /* Initialize Katra */
    result = katra_init();
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize Katra: %d\n", result);
        return 1;
    }

    /* Run tests */
    TEST_RUN(test_index_init);
    TEST_RUN(test_index_store_and_query);
    TEST_RUN(test_index_rebuild);
    TEST_RUN(test_index_stats);
    TEST_RUN(test_index_query_time_range);

    /* Cleanup */
    katra_exit();

    TEST_SUITE_END();
    return 0;
}
