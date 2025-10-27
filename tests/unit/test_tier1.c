/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

/* Project includes */
#include "katra_tier1.h"
#include "katra_memory.h"
#include "katra_init.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

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
static const char* TEST_CI_ID = "test_ci_tier1";

/* Test: Tier 1 initialization */
void test_tier1_init(void) {
    printf("Testing: Tier 1 initialization ... ");
    tests_run++;

    int result = tier1_init(TEST_CI_ID);
    ASSERT(result == KATRA_SUCCESS, "tier1_init() failed");
}

/* Test: Tier 1 directory creation */
void test_tier1_directory_created(void) {
    printf("Testing: Tier 1 directory created ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    const char* home = getenv("HOME");
    if (!home) {
        TEST_FAIL("HOME not set");
        return;
    }

    char tier1_dir[KATRA_PATH_MAX];
    snprintf(tier1_dir, sizeof(tier1_dir), "%s/.katra/memory/tier1", home);

    struct stat st;
    if (stat(tier1_dir, &st) != 0) {
        TEST_FAIL("Tier 1 directory not created");
        return;
    }

    if (!S_ISDIR(st.st_mode)) {
        TEST_FAIL("Tier 1 path exists but is not a directory");
        return;
    }

    TEST_PASS();
}

/* Test: Store single record */
void test_tier1_store_single(void) {
    printf("Testing: Store single record to Tier 1 ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_EXPERIENCE,
        "Tier 1 storage test",
        MEMORY_IMPORTANCE_MEDIUM
    );

    if (!record) {
        TEST_FAIL("Failed to create record");
        return;
    }

    int result = tier1_store(record);
    katra_memory_free_record(record);

    ASSERT(result == KATRA_SUCCESS, "tier1_store() failed");
}

/* Test: Store multiple records */
void test_tier1_store_multiple(void) {
    printf("Testing: Store multiple records to Tier 1 ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    for (int i = 0; i < 5; i++) {
        char content[128];
        snprintf(content, sizeof(content), "Multi-store test record %d", i);

        memory_record_t* record = katra_memory_create_record(
            TEST_CI_ID,
            MEMORY_TYPE_EXPERIENCE,
            content,
            MEMORY_IMPORTANCE_LOW
        );

        if (!record) {
            TEST_FAIL("Failed to create record");
            return;
        }

        int result = tier1_store(record);
        katra_memory_free_record(record);

        if (result != KATRA_SUCCESS) {
            TEST_FAIL("Failed to store record");
            return;
        }
    }

    TEST_PASS();
}

/* Test: Daily file created */
void test_tier1_daily_file_created(void) {
    printf("Testing: Daily file created ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_EXPERIENCE,
        "Daily file test",
        MEMORY_IMPORTANCE_MEDIUM
    );

    if (!record) {
        TEST_FAIL("Failed to create record");
        return;
    }

    tier1_store(record);
    katra_memory_free_record(record);

    /* Check that today's file exists in CI-specific directory */
    const char* home = getenv("HOME");
    if (!home) {
        TEST_FAIL("HOME not set");
        return;
    }

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);

    char filepath[KATRA_PATH_MAX];
    snprintf(filepath, sizeof(filepath),
            "%s/.katra/memory/tier1/%s/%04d-%02d-%02d.jsonl",
            home,
            TEST_CI_ID,
            tm_info->tm_year + 1900,
            tm_info->tm_mon + 1,
            tm_info->tm_mday);

    struct stat st;
    if (stat(filepath, &st) != 0) {
        TEST_FAIL("Daily file not created");
        return;
    }

    if (st.st_size == 0) {
        TEST_FAIL("Daily file is empty");
        return;
    }

    TEST_PASS();
}

/* Test: Tier 1 statistics */
void test_tier1_stats(void) {
    printf("Testing: Tier 1 statistics ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    /* Store some records */
    for (int i = 0; i < 3; i++) {
        char content[128];
        snprintf(content, sizeof(content), "Stats test record %d", i);

        memory_record_t* record = katra_memory_create_record(
            TEST_CI_ID,
            MEMORY_TYPE_EXPERIENCE,
            content,
            MEMORY_IMPORTANCE_MEDIUM
        );

        if (record) {
            tier1_store(record);
            katra_memory_free_record(record);
        }
    }

    size_t total_records = 0;
    size_t bytes_used = 0;

    int result = tier1_stats(TEST_CI_ID, &total_records, &bytes_used);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("tier1_stats() failed");
        return;
    }

    if (total_records < 3) {
        TEST_FAIL("Expected at least 3 records");
        return;
    }

    if (bytes_used == 0) {
        TEST_FAIL("Expected non-zero bytes used");
        return;
    }

    TEST_PASS();
}

/* Test: JSON escaping in content */
void test_tier1_json_escaping(void) {
    printf("Testing: JSON escaping in content ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    /* Content with special characters that need escaping */
    const char* special_content = "Test with \"quotes\" and \n newlines \t tabs \\ backslashes";

    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_EXPERIENCE,
        special_content,
        MEMORY_IMPORTANCE_MEDIUM
    );

    if (!record) {
        TEST_FAIL("Failed to create record");
        return;
    }

    int result = tier1_store(record);
    katra_memory_free_record(record);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to store record with special characters");
        return;
    }

    /* If we got here, JSON was written successfully */
    TEST_PASS();
}

/* Test: Store with response and context */
void test_tier1_store_full_record(void) {
    printf("Testing: Store full record with response and context ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_EXPERIENCE,
        "User question",
        MEMORY_IMPORTANCE_HIGH
    );

    if (!record) {
        TEST_FAIL("Failed to create record");
        return;
    }

    /* Add response and context */
    record->response = strdup("CI response text");
    record->context = strdup("{\"session\":\"test123\"}");
    record->session_id = strdup("session_001");
    record->component = strdup("test_component");

    int result = tier1_store(record);
    katra_memory_free_record(record);

    ASSERT(result == KATRA_SUCCESS, "Failed to store full record");
}

/* Test: Store NULL record */
void test_tier1_store_null(void) {
    printf("Testing: Store NULL record ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    int result = tier1_store(NULL);
    ASSERT(result == E_INPUT_NULL, "Should fail with NULL record");
}

/* Test: Cleanup */
void test_tier1_cleanup(void) {
    printf("Testing: Tier 1 cleanup ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    /* Should not crash */
    tier1_cleanup();

    TEST_PASS();
}

/* Test: Query with no results */
void test_tier1_query_empty(void) {
    printf("Testing: Query with no results ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    memory_query_t query = {
        .ci_id = "nonexistent_ci",
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 0
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    int result = tier1_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("tier1_query() failed");
        return;
    }

    if (count != 0) {
        TEST_FAIL("Expected 0 results for nonexistent CI");
        katra_memory_free_results(results, count);
        return;
    }

    TEST_PASS();
}

/* Test: Query with results */
void test_tier1_query_with_results(void) {
    printf("Testing: Query with results ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    /* Store some test records */
    for (int i = 0; i < 5; i++) {
        char content[128];
        snprintf(content, sizeof(content), "Query test record %d", i);

        memory_record_t* record = katra_memory_create_record(
            TEST_CI_ID,
            MEMORY_TYPE_EXPERIENCE,
            content,
            MEMORY_IMPORTANCE_MEDIUM
        );

        if (record) {
            tier1_store(record);
            katra_memory_free_record(record);
        }
    }

    /* Query all records */
    memory_query_t query = {
        .ci_id = TEST_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 0
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    int result = tier1_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("tier1_query() failed");
        return;
    }

    if (count < 5) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected at least 5 results, got %zu", count);
        TEST_FAIL(msg);
        katra_memory_free_results(results, count);
        return;
    }

    /* Verify we got records back */
    if (!results || !results[0] || !results[0]->content) {
        TEST_FAIL("Results missing expected data");
        katra_memory_free_results(results, count);
        return;
    }

    katra_memory_free_results(results, count);
    TEST_PASS();
}

/* Test: Query with limit */
void test_tier1_query_with_limit(void) {
    printf("Testing: Query with limit ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    /* Store records */
    for (int i = 0; i < 10; i++) {
        char content[128];
        snprintf(content, sizeof(content), "Limit test record %d", i);

        memory_record_t* record = katra_memory_create_record(
            TEST_CI_ID,
            MEMORY_TYPE_EXPERIENCE,
            content,
            MEMORY_IMPORTANCE_LOW
        );

        if (record) {
            tier1_store(record);
            katra_memory_free_record(record);
        }
    }

    /* Query with limit of 3 */
    memory_query_t query = {
        .ci_id = TEST_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 3
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    int result = tier1_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("tier1_query() failed");
        return;
    }

    if (count > 3) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected max 3 results with limit, got %zu", count);
        TEST_FAIL(msg);
        katra_memory_free_results(results, count);
        return;
    }

    katra_memory_free_results(results, count);
    TEST_PASS();
}

/* Test: Query with importance filter */
void test_tier1_query_importance_filter(void) {
    printf("Testing: Query with importance filter ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    /* Store records with different importance */
    memory_record_t* low = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_EXPERIENCE, "Low importance", MEMORY_IMPORTANCE_LOW);
    memory_record_t* high = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_EXPERIENCE, "High importance", MEMORY_IMPORTANCE_HIGH);

    if (low) {
        tier1_store(low);
        katra_memory_free_record(low);
    }
    if (high) {
        tier1_store(high);
        katra_memory_free_record(high);
    }

    /* Query for high importance only */
    memory_query_t query = {
        .ci_id = TEST_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = MEMORY_IMPORTANCE_HIGH,
        .tier = KATRA_TIER1,
        .limit = 0
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    int result = tier1_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("tier1_query() failed");
        return;
    }

    /* Verify all results have high importance */
    bool all_high = true;
    for (size_t i = 0; i < count; i++) {
        if (results[i]->importance < MEMORY_IMPORTANCE_HIGH) {
            all_high = false;
            break;
        }
    }

    if (!all_high) {
        TEST_FAIL("Found low importance records in high importance query");
        katra_memory_free_results(results, count);
        return;
    }

    katra_memory_free_results(results, count);
    TEST_PASS();
}

/* Test: Archive function (counts only, since Tier 2 not implemented) */
void test_tier1_archive(void) {
    printf("Testing: Archive function (counting) ... ");
    tests_run++;

    tier1_init(TEST_CI_ID);

    /* Archive with 7 days - should return count */
    int count = tier1_archive(TEST_CI_ID, 7);
    if (count < 0) {
        TEST_FAIL("tier1_archive() failed");
        return;
    }

    /* Count >= 0 is success (may be 0 if no old records) */
    TEST_PASS();
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Tier 1 Storage Tests\n");
    printf("========================================\n\n");

    /* Initialize katra first */
    katra_init();

    /* Run tests */
    test_tier1_init();
    test_tier1_directory_created();
    test_tier1_store_single();
    test_tier1_store_multiple();
    test_tier1_daily_file_created();
    test_tier1_stats();
    test_tier1_json_escaping();
    test_tier1_store_full_record();
    test_tier1_store_null();
    test_tier1_query_empty();
    test_tier1_query_with_results();
    test_tier1_query_with_limit();
    test_tier1_query_importance_filter();
    test_tier1_archive();
    test_tier1_cleanup();

    /* Cleanup */
    tier1_cleanup();
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
