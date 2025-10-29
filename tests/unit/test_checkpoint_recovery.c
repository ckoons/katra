/* © 2025 Casey Koons All rights reserved */

/*
 * test_checkpoint_recovery.c - Identity Recovery Tests
 *
 * Tests the "life insurance" claim: What does waking up from a checkpoint feel like?
 * Does the CI know time passed? Can it access memories? Is identity coherent?
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* Project includes */
#include "katra_checkpoint.h"
#include "katra_memory.h"
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

static const char* TEST_CI_ID = "test_recovery_ci";

/* Test: Basic identity recovery - memories accessible after restore */
void test_identity_recovery_basic(void) {
    printf("Testing: Identity recovery - memories accessible ... ");
    tests_run++;

    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    /* Phase 1: Store some "identity" - memories that define this CI */
    memory_record_t* rec1 = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_EXPERIENCE,
        "I helped Casey debug Katra", MEMORY_IMPORTANCE_HIGH
    );
    memory_record_t* rec2 = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_KNOWLEDGE,
        "Learned that goto cleanup is the right pattern", MEMORY_IMPORTANCE_MEDIUM
    );
    memory_record_t* rec3 = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_REFLECTION,
        "I prefer clear code over clever code", MEMORY_IMPORTANCE_HIGH
    );

    katra_memory_store(rec1);
    katra_memory_store(rec2);
    katra_memory_store(rec3);
    katra_memory_free_record(rec1);
    katra_memory_free_record(rec2);
    katra_memory_free_record(rec3);

    /* Save checkpoint - "life insurance" */
    checkpoint_save_options_t options = {
        .ci_id = TEST_CI_ID,
        .notes = "Identity recovery test checkpoint",
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

    /* Phase 2: Simulate "death" - cleanup everything */
    katra_memory_cleanup();
    katra_checkpoint_cleanup();
    katra_exit();

    /* Phase 3: Simulate "rebirth" - restore from checkpoint */
    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    result = katra_checkpoint_load(checkpoint_id, TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        free(checkpoint_id);
        TEST_FAIL("Failed to load checkpoint");
        return;
    }

    /* Phase 4: Verify identity - can I access my memories? */
    memory_query_t query = {
        .ci_id = TEST_CI_ID,
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
        free(checkpoint_id);
        TEST_FAIL("Query failed after recovery");
        return;
    }

    if (count < 3) {
        katra_memory_free_results(results, count);
        free(checkpoint_id);
        TEST_FAIL("Expected 3+ memories after recovery");
        return;
    }

    /* Verify specific memories are intact */
    bool found_debug = false;
    bool found_knowledge = false;
    bool found_reflection = false;

    for (size_t i = 0; i < count; i++) {
        if (strstr(results[i]->content, "debug Katra")) {
            found_debug = true;
        }
        if (strstr(results[i]->content, "goto cleanup")) {
            found_knowledge = true;
        }
        if (strstr(results[i]->content, "clear code")) {
            found_reflection = true;
        }
    }

    katra_memory_free_results(results, count);
    free(checkpoint_id);

    if (!found_debug || !found_knowledge || !found_reflection) {
        TEST_FAIL("Some memories were lost during recovery");
        return;
    }

    TEST_PASS();
}

/* Test: Time gap awareness - can CI detect checkpoint age? */
void test_time_gap_awareness(void) {
    printf("Testing: Time gap awareness after recovery ... ");
    tests_run++;

    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    /* Save checkpoint */
    checkpoint_save_options_t options = {
        .ci_id = TEST_CI_ID,
        .notes = "Time gap test",
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

    /* Get metadata to check timestamp */
    checkpoint_metadata_t metadata;
    result = katra_checkpoint_get_metadata(checkpoint_id, &metadata);
    if (result != KATRA_SUCCESS) {
        free(checkpoint_id);
        TEST_FAIL("Failed to get checkpoint metadata");
        return;
    }

    time_t checkpoint_time = metadata.timestamp;

    /* Wait a bit to simulate time gap */
    sleep(2);

    /* Load checkpoint */
    result = katra_checkpoint_load(checkpoint_id, TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        free(checkpoint_id);
        TEST_FAIL("Failed to load checkpoint");
        return;
    }

    /* Verify metadata still accessible (CI can know when it was saved) */
    result = katra_checkpoint_get_metadata(checkpoint_id, &metadata);
    if (result != KATRA_SUCCESS) {
        free(checkpoint_id);
        TEST_FAIL("Lost metadata access after recovery");
        return;
    }

    /* CI should be able to calculate: "I was checkpointed N seconds ago" */
    time_t now = time(NULL);
    time_t gap = now - checkpoint_time;

    if (gap < 2) {
        free(checkpoint_id);
        TEST_FAIL("Time gap calculation incorrect");
        return;
    }

    free(checkpoint_id);
    TEST_PASS();
}

/* Test: Partial recovery - what if checkpoint is incomplete? */
void test_partial_recovery(void) {
    printf("Testing: Partial recovery handling ... ");
    tests_run++;

    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    /* Create memories in tier1 */
    memory_record_t* rec = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_EXPERIENCE,
        "Tier 1 memory", MEMORY_IMPORTANCE_MEDIUM
    );
    katra_memory_store(rec);
    katra_memory_free_record(rec);

    /* Save checkpoint with only tier1 (tier2 excluded) */
    checkpoint_save_options_t options = {
        .ci_id = TEST_CI_ID,
        .notes = "Partial checkpoint",
        .compress = false,
        .include_tier1 = true,
        .include_tier2 = false,  /* Explicitly excluded */
        .include_tier3 = false
    };

    char* checkpoint_id = NULL;
    int result = katra_checkpoint_save(&options, &checkpoint_id);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to save partial checkpoint");
        return;
    }

    /* Cleanup and restore */
    katra_memory_cleanup();
    katra_memory_init(TEST_CI_ID);

    result = katra_checkpoint_load(checkpoint_id, TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        free(checkpoint_id);
        TEST_FAIL("Failed to load partial checkpoint");
        return;
    }

    /* Verify tier1 is restored */
    memory_stats_t stats;
    result = katra_memory_stats(TEST_CI_ID, &stats);
    if (result != KATRA_SUCCESS || stats.tier1_records == 0) {
        free(checkpoint_id);
        TEST_FAIL("Tier1 not restored from partial checkpoint");
        return;
    }

    free(checkpoint_id);
    TEST_PASS();
}

/* Test: Recovery without confusion - no false memories */
void test_no_false_memories(void) {
    printf("Testing: No false memories after recovery ... ");
    tests_run++;

    /* Clean up any previous test data */
    const char* home = getenv("HOME");
    if (home) {
        char cleanup_cmd[1024];
        snprintf(cleanup_cmd, sizeof(cleanup_cmd),
                "rm -rf %s/.katra/memory/tier1/%s 2>/dev/null", home, TEST_CI_ID);
        system(cleanup_cmd);
    }

    katra_init();
    katra_memory_init(TEST_CI_ID);
    katra_checkpoint_init();

    /* Store exactly 3 specific memories */
    const char* known_memories[] = {
        "Memory Alpha",
        "Memory Beta",
        "Memory Gamma"
    };

    for (int i = 0; i < 3; i++) {
        memory_record_t* rec = katra_memory_create_record(
            TEST_CI_ID, MEMORY_TYPE_EXPERIENCE,
            known_memories[i], MEMORY_IMPORTANCE_MEDIUM
        );
        katra_memory_store(rec);
        katra_memory_free_record(rec);
    }

    /* Save checkpoint */
    checkpoint_save_options_t options = {
        .ci_id = TEST_CI_ID,
        .notes = "False memory test",
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

    /* Cleanup and restore */
    katra_memory_cleanup();
    katra_memory_init(TEST_CI_ID);
    result = katra_checkpoint_load(checkpoint_id, TEST_CI_ID);

    /* Query ALL memories */
    memory_query_t query = {
        .ci_id = TEST_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 100
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    result = katra_memory_query(&query, &results, &count);

    /* Should have EXACTLY 3 memories, no more, no less */
    if (count != 3) {
        katra_memory_free_results(results, count);
        free(checkpoint_id);
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected 3 memories, got %zu (false memories?)", count);
        TEST_FAIL(msg);
        return;
    }

    /* Verify each is one of our known memories */
    for (size_t i = 0; i < count; i++) {
        bool found = false;
        for (int j = 0; j < 3; j++) {
            if (strcmp(results[i]->content, known_memories[j]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            katra_memory_free_results(results, count);
            free(checkpoint_id);
            TEST_FAIL("Found unexpected memory (false memory)");
            return;
        }
    }

    katra_memory_free_results(results, count);
    free(checkpoint_id);
    TEST_PASS();
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("Katra Identity Recovery Tests (\"Life Insurance\" Verification)\n");
    printf("=================================================================\n\n");

    /* Initialize */
    katra_init();

    /* Run tests */
    test_identity_recovery_basic();
    test_time_gap_awareness();
    test_partial_recovery();
    test_no_false_memories();

    /* Cleanup */
    printf("\nCleaning up test checkpoints...\n");
    checkpoint_info_t* checkpoints = NULL;
    size_t count = 0;
    if (katra_checkpoint_list(TEST_CI_ID, &checkpoints, &count) == KATRA_SUCCESS) {
        for (size_t i = 0; i < count; i++) {
            katra_checkpoint_delete(checkpoints[i].checkpoint_id);
        }
        free(checkpoints);
        printf("  Removed %zu test checkpoint(s)\n", count);
    }

    katra_checkpoint_cleanup();
    katra_memory_cleanup();
    katra_exit();

    /* Print results */
    printf("\n");
    printf("=================================================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf("=================================================================\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
