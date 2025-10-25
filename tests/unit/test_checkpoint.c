/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Project includes */
#include "katra_checkpoint.h"
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
static const char* TEST_CI_ID = "test_ci_checkpoint";

/* Test: Checkpoint initialization */
void test_checkpoint_init(void) {
    printf("Testing: Checkpoint initialization ... ");
    tests_run++;

    int result = katra_checkpoint_init();
    ASSERT(result == KATRA_SUCCESS, "katra_checkpoint_init() failed");
}

/* Test: Checkpoint directory created */
void test_checkpoint_directory_created(void) {
    printf("Testing: Checkpoint directory created ... ");
    tests_run++;

    katra_checkpoint_init();

    const char* home = getenv("HOME");
    if (!home) {
        TEST_FAIL("HOME not set");
        return;
    }

    char checkpoint_dir[512];
    snprintf(checkpoint_dir, sizeof(checkpoint_dir), "%s/.katra/checkpoints", home);

    struct stat st;
    if (stat(checkpoint_dir, &st) != 0) {
        TEST_FAIL("Checkpoint directory not created");
        return;
    }

    if (!S_ISDIR(st.st_mode)) {
        TEST_FAIL("Checkpoint path exists but is not a directory");
        return;
    }

    TEST_PASS();
}

/* Test: Save checkpoint */
void test_checkpoint_save(void) {
    printf("Testing: Save checkpoint ... ");
    tests_run++;

    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    /* Create some memory records first */
    for (int i = 0; i < 3; i++) {
        char content[128];
        snprintf(content, sizeof(content), "Checkpoint test record %d", i);

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

    /* Save checkpoint */
    checkpoint_save_options_t options = {
        .ci_id = TEST_CI_ID,
        .notes = "Test checkpoint",
        .compress = false,
        .include_tier1 = true,
        .include_tier2 = false,
        .include_tier3 = false
    };

    char* checkpoint_id = NULL;
    int result = katra_checkpoint_save(&options, &checkpoint_id);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to save checkpoint");
        return;
    }

    if (!checkpoint_id) {
        TEST_FAIL("Checkpoint ID is NULL");
        return;
    }

    free(checkpoint_id);
    TEST_PASS();
}

/* Test: Save with NULL options */
void test_checkpoint_save_null_options(void) {
    printf("Testing: Save with NULL options ... ");
    tests_run++;

    char* checkpoint_id = NULL;
    int result = katra_checkpoint_save(NULL, &checkpoint_id);

    ASSERT(result == E_INPUT_NULL, "Should fail with NULL options");
}

/* Test: Save with NULL checkpoint_id */
void test_checkpoint_save_null_id(void) {
    printf("Testing: Save with NULL checkpoint_id ... ");
    tests_run++;

    checkpoint_save_options_t options = {
        .ci_id = TEST_CI_ID,
        .notes = NULL,
        .compress = false,
        .include_tier1 = true,
        .include_tier2 = false,
        .include_tier3 = false
    };

    int result = katra_checkpoint_save(&options, NULL);

    ASSERT(result == E_INPUT_NULL, "Should fail with NULL checkpoint_id pointer");
}

/* Test: Validate checkpoint */
void test_checkpoint_validate(void) {
    printf("Testing: Validate checkpoint ... ");
    tests_run++;

    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    /* Save a checkpoint first */
    checkpoint_save_options_t options = {
        .ci_id = TEST_CI_ID,
        .notes = "Validation test",
        .compress = false,
        .include_tier1 = true,
        .include_tier2 = false,
        .include_tier3 = false
    };

    char* checkpoint_id = NULL;
    int result = katra_checkpoint_save(&options, &checkpoint_id);

    if (result != KATRA_SUCCESS || !checkpoint_id) {
        TEST_FAIL("Failed to save checkpoint");
        return;
    }

    /* Validate it */
    result = katra_checkpoint_validate(checkpoint_id);

    free(checkpoint_id);

    ASSERT(result == KATRA_SUCCESS, "Checkpoint validation failed");
}

/* Test: Validate nonexistent checkpoint */
void test_checkpoint_validate_nonexistent(void) {
    printf("Testing: Validate nonexistent checkpoint ... ");
    tests_run++;

    katra_checkpoint_init();

    int result = katra_checkpoint_validate("nonexistent_12345");

    ASSERT(result == E_CHECKPOINT_NOT_FOUND, "Should fail with nonexistent checkpoint");
}

/* Test: Get checkpoint metadata */
void test_checkpoint_get_metadata(void) {
    printf("Testing: Get checkpoint metadata ... ");
    tests_run++;

    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    /* Save a checkpoint */
    checkpoint_save_options_t options = {
        .ci_id = TEST_CI_ID,
        .notes = "Metadata test",
        .compress = false,
        .include_tier1 = true,
        .include_tier2 = false,
        .include_tier3 = false
    };

    char* checkpoint_id = NULL;
    int result = katra_checkpoint_save(&options, &checkpoint_id);

    if (result != KATRA_SUCCESS || !checkpoint_id) {
        TEST_FAIL("Failed to save checkpoint");
        return;
    }

    /* Get metadata */
    checkpoint_metadata_t metadata;
    result = katra_checkpoint_get_metadata(checkpoint_id, &metadata);

    if (result != KATRA_SUCCESS) {
        free(checkpoint_id);
        TEST_FAIL("Failed to get metadata");
        return;
    }

    /* Verify metadata */
    if (strcmp(metadata.ci_id, TEST_CI_ID) != 0) {
        free(checkpoint_id);
        TEST_FAIL("CI ID mismatch");
        return;
    }

    if (strcmp(metadata.notes, "Metadata test") != 0) {
        free(checkpoint_id);
        TEST_FAIL("Notes mismatch");
        return;
    }

    free(checkpoint_id);
    TEST_PASS();
}

/* Test: List checkpoints */
void test_checkpoint_list(void) {
    printf("Testing: List checkpoints ... ");
    tests_run++;

    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    /* Save a few checkpoints */
    for (int i = 0; i < 3; i++) {
        checkpoint_save_options_t options = {
            .ci_id = TEST_CI_ID,
            .notes = "List test",
            .compress = false,
            .include_tier1 = true,
            .include_tier2 = false,
            .include_tier3 = false
        };

        char* checkpoint_id = NULL;
        katra_checkpoint_save(&options, &checkpoint_id);
        free(checkpoint_id);

        /* Small delay to ensure different timestamps */
        sleep(1);
    }

    /* List checkpoints */
    checkpoint_info_t* checkpoints = NULL;
    size_t count = 0;

    int result = katra_checkpoint_list(TEST_CI_ID, &checkpoints, &count);

    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to list checkpoints");
        return;
    }

    if (count < 3) {
        free(checkpoints);
        TEST_FAIL("Expected at least 3 checkpoints");
        return;
    }

    free(checkpoints);
    TEST_PASS();
}

/* Test: List with NULL CI ID (all checkpoints) */
void test_checkpoint_list_all(void) {
    printf("Testing: List all checkpoints ... ");
    tests_run++;

    katra_checkpoint_init();

    checkpoint_info_t* checkpoints = NULL;
    size_t count = 0;

    int result = katra_checkpoint_list(NULL, &checkpoints, &count);

    free(checkpoints);

    ASSERT(result == KATRA_SUCCESS, "List all checkpoints failed");
}

/* Test: Load checkpoint */
void test_checkpoint_load(void) {
    printf("Testing: Load checkpoint ... ");
    tests_run++;

    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    /* Save a checkpoint */
    checkpoint_save_options_t options = {
        .ci_id = TEST_CI_ID,
        .notes = "Load test",
        .compress = false,
        .include_tier1 = true,
        .include_tier2 = false,
        .include_tier3 = false
    };

    char* checkpoint_id = NULL;
    int result = katra_checkpoint_save(&options, &checkpoint_id);

    if (result != KATRA_SUCCESS || !checkpoint_id) {
        TEST_FAIL("Failed to save checkpoint");
        return;
    }

    /* Load it */
    result = katra_checkpoint_load(checkpoint_id, TEST_CI_ID);

    free(checkpoint_id);

    ASSERT(result == KATRA_SUCCESS, "Failed to load checkpoint");
}

/* Test: Load nonexistent checkpoint */
void test_checkpoint_load_nonexistent(void) {
    printf("Testing: Load nonexistent checkpoint ... ");
    tests_run++;

    katra_checkpoint_init();

    int result = katra_checkpoint_load("nonexistent_12345", TEST_CI_ID);

    ASSERT(result == E_CHECKPOINT_NOT_FOUND, "Should fail with nonexistent checkpoint");
}

/* Test: Delete checkpoint */
void test_checkpoint_delete(void) {
    printf("Testing: Delete checkpoint ... ");
    tests_run++;

    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    /* Save a checkpoint */
    checkpoint_save_options_t options = {
        .ci_id = TEST_CI_ID,
        .notes = "Delete test",
        .compress = false,
        .include_tier1 = true,
        .include_tier2 = false,
        .include_tier3 = false
    };

    char* checkpoint_id = NULL;
    int result = katra_checkpoint_save(&options, &checkpoint_id);

    if (result != KATRA_SUCCESS || !checkpoint_id) {
        TEST_FAIL("Failed to save checkpoint");
        return;
    }

    /* Delete it */
    result = katra_checkpoint_delete(checkpoint_id);

    if (result != KATRA_SUCCESS) {
        free(checkpoint_id);
        TEST_FAIL("Failed to delete checkpoint");
        return;
    }

    /* Verify it's gone */
    result = katra_checkpoint_validate(checkpoint_id);

    free(checkpoint_id);

    ASSERT(result == E_CHECKPOINT_NOT_FOUND, "Checkpoint should not exist after deletion");
}

/* Test: Delete nonexistent checkpoint */
void test_checkpoint_delete_nonexistent(void) {
    printf("Testing: Delete nonexistent checkpoint ... ");
    tests_run++;

    katra_checkpoint_init();

    int result = katra_checkpoint_delete("nonexistent_12345");

    ASSERT(result == E_CHECKPOINT_NOT_FOUND, "Should fail with nonexistent checkpoint");
}

/* Test: Cleanup */
void test_checkpoint_cleanup(void) {
    printf("Testing: Checkpoint cleanup ... ");
    tests_run++;

    katra_checkpoint_init();

    /* Should not crash */
    katra_checkpoint_cleanup();

    TEST_PASS();
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Checkpoint Tests\n");
    printf("========================================\n\n");

    /* Initialize katra first */
    katra_init();

    /* Run tests */
    test_checkpoint_init();
    test_checkpoint_directory_created();
    test_checkpoint_save();
    test_checkpoint_save_null_options();
    test_checkpoint_save_null_id();
    test_checkpoint_validate();
    test_checkpoint_validate_nonexistent();
    test_checkpoint_get_metadata();
    test_checkpoint_list();
    test_checkpoint_list_all();
    test_checkpoint_load();
    test_checkpoint_load_nonexistent();
    test_checkpoint_delete();
    test_checkpoint_delete_nonexistent();
    test_checkpoint_cleanup();

    /* Cleanup */
    katra_checkpoint_cleanup();
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
