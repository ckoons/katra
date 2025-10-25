/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Project includes */
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

/* Test CI ID */
static const char* TEST_CI_ID = "test_ci_tier2";

/* Test: Tier 2 initialization */
void test_tier2_init(void) {
    printf("Testing: Tier 2 initialization ... ");
    tests_run++;

    int result = tier2_init(TEST_CI_ID);
    ASSERT(result == KATRA_SUCCESS, "tier2_init() failed");
}

/* Test: Tier 2 directories created */
void test_tier2_directories_created(void) {
    printf("Testing: Tier 2 directories created ... ");
    tests_run++;

    tier2_init(TEST_CI_ID);

    const char* home = getenv("HOME");
    if (!home) {
        TEST_FAIL("HOME not set");
        return;
    }

    char tier2_dir[512];
    snprintf(tier2_dir, sizeof(tier2_dir), "%s/.katra/memory/tier2", home);

    struct stat st;
    if (stat(tier2_dir, &st) != 0) {
        TEST_FAIL("Tier 2 directory not created");
        return;
    }

    if (!S_ISDIR(st.st_mode)) {
        TEST_FAIL("Tier 2 path exists but is not a directory");
        return;
    }

    /* Check weekly subdirectory */
    char weekly_dir[512];
    snprintf(weekly_dir, sizeof(weekly_dir), "%s/weekly", tier2_dir);
    if (stat(weekly_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        TEST_FAIL("Weekly subdirectory not created");
        return;
    }

    /* Check monthly subdirectory */
    char monthly_dir[512];
    snprintf(monthly_dir, sizeof(monthly_dir), "%s/monthly", tier2_dir);
    if (stat(monthly_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        TEST_FAIL("Monthly subdirectory not created");
        return;
    }

    /* Check index subdirectory */
    char index_dir[512];
    snprintf(index_dir, sizeof(index_dir), "%s/index", tier2_dir);
    if (stat(index_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        TEST_FAIL("Index subdirectory not created");
        return;
    }

    TEST_PASS();
}

/* Test: Create digest record */
void test_create_digest(void) {
    printf("Testing: Create digest record ... ");
    tests_run++;

    digest_record_t* digest = katra_digest_create(
        TEST_CI_ID,
        PERIOD_TYPE_WEEKLY,
        "2025-W01",
        DIGEST_TYPE_INTERACTION
    );

    if (!digest) {
        TEST_FAIL("Failed to create digest");
        return;
    }

    if (!digest->digest_id || !digest->ci_id || !digest->period_id) {
        TEST_FAIL("Missing required fields");
        katra_digest_free(digest);
        return;
    }

    if (digest->period_type != PERIOD_TYPE_WEEKLY) {
        TEST_FAIL("Wrong period type");
        katra_digest_free(digest);
        return;
    }

    if (digest->digest_type != DIGEST_TYPE_INTERACTION) {
        TEST_FAIL("Wrong digest type");
        katra_digest_free(digest);
        return;
    }

    katra_digest_free(digest);
    TEST_PASS();
}

/* Test: Free digest record */
void test_free_digest(void) {
    printf("Testing: Free digest record ... ");
    tests_run++;

    digest_record_t* digest = katra_digest_create(
        TEST_CI_ID,
        PERIOD_TYPE_MONTHLY,
        "2025-01",
        DIGEST_TYPE_LEARNING
    );

    if (!digest) {
        TEST_FAIL("Failed to create digest");
        return;
    }

    /* Should not crash */
    katra_digest_free(digest);
    TEST_PASS();
}

/* Test: Store digest */
void test_store_digest(void) {
    printf("Testing: Store digest ... ");
    tests_run++;

    tier2_init(TEST_CI_ID);

    digest_record_t* digest = katra_digest_create(
        TEST_CI_ID,
        PERIOD_TYPE_WEEKLY,
        "2025-W01",
        DIGEST_TYPE_INTERACTION
    );

    if (!digest) {
        TEST_FAIL("Failed to create digest");
        return;
    }

    /* Add some content to the digest */
    digest->summary = strdup("Test summary for week 1");
    digest->questions_asked = 5;
    digest->source_record_count = 10;

    int result = tier2_store_digest(digest);
    katra_digest_free(digest);

    ASSERT(result == KATRA_SUCCESS, "tier2_store_digest() failed");
}

/* Test: Store digest with NULL */
void test_store_digest_null(void) {
    printf("Testing: Store digest with NULL ... ");
    tests_run++;

    tier2_init(TEST_CI_ID);

    int result = tier2_store_digest(NULL);
    ASSERT(result == E_INPUT_NULL, "Should fail with NULL digest");
}

/* Test: Tier 2 statistics */
void test_tier2_stats(void) {
    printf("Testing: Tier 2 statistics ... ");
    tests_run++;

    tier2_init(TEST_CI_ID);

    size_t total_digests = 0;
    size_t bytes_used = 0;

    int result = tier2_stats(TEST_CI_ID, &total_digests, &bytes_used);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("tier2_stats() failed");
        return;
    }

    /* Should be 0 since nothing stored yet */
    if (total_digests != 0) {
        TEST_FAIL("Expected 0 digests");
        return;
    }

    if (bytes_used != 0) {
        TEST_FAIL("Expected 0 bytes");
        return;
    }

    TEST_PASS();
}

/* Test: Query with CI ID that has no digests */
void test_query_empty(void) {
    printf("Testing: Query with nonexistent CI ID ... ");
    tests_run++;

    tier2_init(TEST_CI_ID);

    /* Query for a CI ID that doesn't have any stored digests */
    digest_query_t query = {
        .ci_id = "nonexistent_ci_id",
        .start_time = 0,
        .end_time = 0,
        .period_type = (period_type_t)-1,
        .theme = NULL,
        .keyword = NULL,
        .digest_type = (digest_type_t)-1,
        .limit = 0
    };

    digest_record_t** results = NULL;
    size_t count = 0;

    int result = tier2_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("tier2_query() failed");
        return;
    }

    if (count != 0) {
        TEST_FAIL("Expected 0 results for nonexistent CI ID");
        katra_digest_free_results(results, count);
        return;
    }

    TEST_PASS();
}

/* Test: Query with NULL parameters */
void test_query_null(void) {
    printf("Testing: Query with NULL parameters ... ");
    tests_run++;

    tier2_init(TEST_CI_ID);

    digest_query_t query = {
        .ci_id = TEST_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .period_type = (period_type_t)-1,
        .theme = NULL,
        .keyword = NULL,
        .digest_type = (digest_type_t)-1,
        .limit = 0
    };

    digest_record_t** results = NULL;
    size_t count = 0;

    /* NULL query */
    int result = tier2_query(NULL, &results, &count);
    if (result != E_INPUT_NULL) {
        TEST_FAIL("Should fail with NULL query");
        return;
    }

    /* NULL results */
    result = tier2_query(&query, NULL, &count);
    if (result != E_INPUT_NULL) {
        TEST_FAIL("Should fail with NULL results");
        return;
    }

    /* NULL count */
    result = tier2_query(&query, &results, NULL);
    if (result != E_INPUT_NULL) {
        TEST_FAIL("Should fail with NULL count");
        return;
    }

    TEST_PASS();
}

/* Test: Query after store */
void test_query_after_store(void) {
    printf("Testing: Query after store ... ");
    tests_run++;

    tier2_init(TEST_CI_ID);

    /* Store a digest with unique period_id */
    digest_record_t* digest = katra_digest_create(
        TEST_CI_ID,
        PERIOD_TYPE_WEEKLY,
        "2025-W42",  /* Unique week to avoid conflicts */
        DIGEST_TYPE_LEARNING
    );

    if (!digest) {
        TEST_FAIL("Failed to create digest");
        return;
    }

    digest->summary = strdup("Test learning summary");
    digest->questions_asked = 3;
    digest->source_record_count = 5;

    int result = tier2_store_digest(digest);
    katra_digest_free(digest);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to store digest");
        return;
    }

    /* Query for the digest */
    digest_query_t query = {
        .ci_id = TEST_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .period_type = PERIOD_TYPE_WEEKLY,
        .theme = NULL,
        .keyword = NULL,
        .digest_type = DIGEST_TYPE_LEARNING,
        .limit = 10
    };

    digest_record_t** results = NULL;
    size_t count = 0;

    result = tier2_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("tier2_query() failed");
        return;
    }

    /* Should find at least the digest we just stored */
    if (count == 0) {
        TEST_FAIL("Expected at least 1 result after storing digest");
        katra_digest_free_results(results, count);
        return;
    }

    /* Verify at least one result matches our stored digest */
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (results[i]->period_type == PERIOD_TYPE_WEEKLY &&
            results[i]->digest_type == DIGEST_TYPE_LEARNING &&
            strcmp(results[i]->period_id, "2025-W42") == 0) {
            found = true;
            break;
        }
    }

    katra_digest_free_results(results, count);

    if (!found) {
        TEST_FAIL("Could not find the stored digest in query results");
        return;
    }

    TEST_PASS();
}

/* Test: Tier 2 cleanup */
void test_tier2_cleanup(void) {
    printf("Testing: Tier 2 cleanup ... ");
    tests_run++;

    tier2_init(TEST_CI_ID);

    /* Should not crash */
    tier2_cleanup();

    TEST_PASS();
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Tier 2 Storage Tests\n");
    printf("========================================\n\n");

    /* Initialize katra first */
    katra_init();

    /* Run tests */
    test_tier2_init();
    test_tier2_directories_created();
    test_create_digest();
    test_free_digest();
    test_store_digest();
    test_store_digest_null();
    test_query_empty();
    test_query_null();
    test_query_after_store();
    test_tier2_stats();
    test_tier2_cleanup();

    /* Cleanup */
    tier2_cleanup();
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
