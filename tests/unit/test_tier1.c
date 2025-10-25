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
        MEMORY_TYPE_INTERACTION,
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
            MEMORY_TYPE_INTERACTION,
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
        MEMORY_TYPE_INTERACTION,
        "Daily file test",
        MEMORY_IMPORTANCE_MEDIUM
    );

    if (!record) {
        TEST_FAIL("Failed to create record");
        return;
    }

    tier1_store(record);
    katra_memory_free_record(record);

    /* Check that today's file exists */
    const char* home = getenv("HOME");
    if (!home) {
        TEST_FAIL("HOME not set");
        return;
    }

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);

    char filepath[KATRA_PATH_MAX];
    snprintf(filepath, sizeof(filepath),
            "%s/.katra/memory/tier1/%04d-%02d-%02d.jsonl",
            home,
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
            MEMORY_TYPE_INTERACTION,
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
        MEMORY_TYPE_INTERACTION,
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
        MEMORY_TYPE_INTERACTION,
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
