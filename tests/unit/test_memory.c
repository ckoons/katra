/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Project includes */
#include "katra_memory.h"
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
static const char* TEST_CI_ID = "test_ci_001";

/* Test: Basic memory initialization */
void test_memory_init(void) {
    printf("Testing: Memory initialization ... ");
    tests_run++;

    int result = katra_memory_init(TEST_CI_ID);
    ASSERT(result == KATRA_SUCCESS, "katra_memory_init() failed");
}

/* Test: Memory init with NULL CI ID */
void test_memory_init_null(void) {
    printf("Testing: Memory init with NULL CI ID ... ");
    tests_run++;

    int result = katra_memory_init(NULL);
    ASSERT(result == E_INPUT_NULL, "Should fail with NULL CI ID");
}

/* Test: Multiple init calls (idempotent) */
void test_memory_init_multiple(void) {
    printf("Testing: Multiple memory init calls ... ");
    tests_run++;

    int result1 = katra_memory_init(TEST_CI_ID);
    int result2 = katra_memory_init(TEST_CI_ID);

    ASSERT(result1 == KATRA_SUCCESS && result2 == KATRA_SUCCESS,
           "Multiple init calls failed");
}

/* Test: Create memory record */
void test_memory_create_record(void) {
    printf("Testing: Create memory record ... ");
    tests_run++;

    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_INTERACTION,
        "Test interaction content",
        MEMORY_IMPORTANCE_MEDIUM
    );

    if (!record) {
        TEST_FAIL("Failed to create record");
        return;
    }

    /* Verify record fields */
    if (!record->record_id || !record->ci_id || !record->content) {
        katra_memory_free_record(record);
        TEST_FAIL("Record missing required fields");
        return;
    }

    if (strcmp(record->ci_id, TEST_CI_ID) != 0) {
        katra_memory_free_record(record);
        TEST_FAIL("CI ID mismatch");
        return;
    }

    if (strcmp(record->content, "Test interaction content") != 0) {
        katra_memory_free_record(record);
        TEST_FAIL("Content mismatch");
        return;
    }

    if (record->importance != MEMORY_IMPORTANCE_MEDIUM) {
        katra_memory_free_record(record);
        TEST_FAIL("Importance mismatch");
        return;
    }

    if (record->type != MEMORY_TYPE_INTERACTION) {
        katra_memory_free_record(record);
        TEST_FAIL("Type mismatch");
        return;
    }

    if (record->tier != KATRA_TIER1) {
        katra_memory_free_record(record);
        TEST_FAIL("Tier should default to TIER1");
        return;
    }

    if (record->archived) {
        katra_memory_free_record(record);
        TEST_FAIL("Archived should default to false");
        return;
    }

    katra_memory_free_record(record);
    TEST_PASS();
}

/* Test: Store memory record */
void test_memory_store(void) {
    printf("Testing: Store memory record ... ");
    tests_run++;

    katra_memory_init(TEST_CI_ID);

    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_INTERACTION,
        "Store test content",
        MEMORY_IMPORTANCE_HIGH
    );

    if (!record) {
        TEST_FAIL("Failed to create record");
        return;
    }

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    ASSERT(result == KATRA_SUCCESS, "Failed to store record");
}

/* Test: Store with NULL record */
void test_memory_store_null(void) {
    printf("Testing: Store with NULL record ... ");
    tests_run++;

    katra_memory_init(TEST_CI_ID);

    int result = katra_memory_store(NULL);
    ASSERT(result == E_INPUT_NULL, "Should fail with NULL record");
}

/* Test: Store with invalid importance */
void test_memory_store_invalid_importance(void) {
    printf("Testing: Store with invalid importance ... ");
    tests_run++;

    katra_memory_init(TEST_CI_ID);

    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_INTERACTION,
        "Invalid importance test",
        1.5  /* Invalid - must be 0.0-1.0 */
    );

    if (!record) {
        TEST_FAIL("Failed to create record");
        return;
    }

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    ASSERT(result == E_INPUT_RANGE, "Should fail with invalid importance");
}

/* Test: Get memory statistics */
void test_memory_stats(void) {
    printf("Testing: Get memory statistics ... ");
    tests_run++;

    katra_memory_init(TEST_CI_ID);

    /* Store a few records */
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
            katra_memory_store(record);
            katra_memory_free_record(record);
        }
    }

    memory_stats_t stats;
    memset(&stats, 0, sizeof(stats));

    int result = katra_memory_stats(TEST_CI_ID, &stats);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to get stats");
        return;
    }

    /* Should have at least 3 records (we just stored them) */
    if (stats.total_records < 3) {
        TEST_FAIL("Expected at least 3 records");
        return;
    }

    if (stats.tier1_records < 3) {
        TEST_FAIL("Expected at least 3 tier1 records");
        return;
    }

    if (stats.bytes_used == 0) {
        TEST_FAIL("Expected non-zero bytes used");
        return;
    }

    TEST_PASS();
}

/* Test: Memory cleanup */
void test_memory_cleanup(void) {
    printf("Testing: Memory cleanup ... ");
    tests_run++;

    katra_memory_init(TEST_CI_ID);

    /* Should not crash */
    katra_memory_cleanup();

    TEST_PASS();
}

/* Test: Multiple cleanup calls */
void test_memory_cleanup_multiple(void) {
    printf("Testing: Multiple cleanup calls ... ");
    tests_run++;

    katra_memory_init(TEST_CI_ID);

    /* Should not crash */
    katra_memory_cleanup();
    katra_memory_cleanup();
    katra_memory_cleanup();

    TEST_PASS();
}

/* Test: Full lifecycle */
void test_memory_lifecycle(void) {
    printf("Testing: Full memory lifecycle ... ");
    tests_run++;

    /* Initialize */
    int result = katra_memory_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Init failed");
        return;
    }

    /* Create and store record */
    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_EXPERIENCE,
        "Lifecycle test content",
        MEMORY_IMPORTANCE_LOW
    );

    if (!record) {
        TEST_FAIL("Failed to create record");
        return;
    }

    result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Store failed");
        return;
    }

    /* Get stats */
    memory_stats_t stats;
    result = katra_memory_stats(TEST_CI_ID, &stats);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Stats failed");
        return;
    }

    /* Cleanup */
    katra_memory_cleanup();

    /* Re-initialize */
    result = katra_memory_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Re-init failed");
        return;
    }

    TEST_PASS();
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Memory Tests\n");
    printf("========================================\n\n");

    /* Initialize katra first */
    katra_init();

    /* Run tests */
    test_memory_init();
    test_memory_init_null();
    test_memory_init_multiple();
    test_memory_create_record();
    test_memory_store();
    test_memory_store_null();
    test_memory_store_invalid_importance();
    test_memory_stats();
    test_memory_cleanup();
    test_memory_cleanup_multiple();
    test_memory_lifecycle();

    /* Cleanup */
    katra_memory_cleanup();
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
