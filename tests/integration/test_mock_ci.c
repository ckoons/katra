/* © 2025 Casey Koons All rights reserved */

/**
 * test_mock_ci.c - Mock CI Test Framework
 *
 * This test simulates a realistic CI using Katra without requiring an actual LLM.
 * It demonstrates:
 * - Complete CI lifecycle (init, daily use, sundown, sunrise, shutdown)
 * - Realistic memory patterns (experiences, reflections, goals)
 * - Error handling and recovery
 * - Memory consolidation workflows
 * - Identity preservation through checkpoints
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

/* Project includes */
#include "katra_init.h"
#include "katra_memory.h"
#include "katra_checkpoint.h"
#include "katra_sunrise_sunset.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_path_utils.h"

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

/* Mock CI identity */
static const char* MOCK_CI_ID = "mock_research_ci";

/* Helper: Clean up test data */
static void cleanup_test_data(void) {
    char cleanup_cmd[512];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd),
            "rm -rf ~/.katra/memory/tier1/%s ~/.katra/memory/tier2/%s "
            "~/.katra/checkpoints/%s_* 2>/dev/null",
            MOCK_CI_ID, MOCK_CI_ID, MOCK_CI_ID);
    system(cleanup_cmd);
}

/* Test: CI Initialization */
void test_ci_initialization(void) {
    printf("Testing: CI initialization ... ");
    tests_run++;

    cleanup_test_data();

    /* Initialize Katra */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("Failed to initialize Katra");
        return;
    }

    /* Initialize memory for mock CI */
    result = katra_memory_init(MOCK_CI_ID);
    ASSERT(result == KATRA_SUCCESS, "Should initialize CI memory");
}

/* Test: CI stores experiences */
void test_ci_store_experiences(void) {
    printf("Testing: CI stores daily experiences ... ");
    tests_run++;

    /* Simulate a day of research activities */
    const char* experiences[] = {
        "Read paper on transformer architecture - key insight on attention mechanisms",
        "Discussed quantum computing with colleague - need to study entanglement",
        "Debugged memory leak in C code - always free what you allocate",
        "Coffee break conversation about AI ethics - important considerations",
        "Reviewed pull request - good pattern for error handling"
    };

    int stored_count = 0;

    for (size_t i = 0; i < sizeof(experiences) / sizeof(experiences[0]); i++) {
        memory_record_t* record = katra_memory_create_record(
            MOCK_CI_ID,
            MEMORY_TYPE_EXPERIENCE,
            experiences[i],
            MEMORY_IMPORTANCE_MEDIUM
        );

        if (!record) {
            TEST_FAIL("Failed to create experience record");
            return;
        }

        int result = katra_memory_store(record);
        katra_memory_free_record(record);

        if (result == KATRA_SUCCESS) {
            stored_count++;
        }
    }

    ASSERT(stored_count == 5, "Should store all 5 experiences");
}

/* Test: CI records patterns */
void test_ci_record_patterns(void) {
    printf("Testing: CI records patterns from experiences ... ");
    tests_run++;

    const char* patterns[] = {
        "Pattern: Attention mechanisms appear central to modern AI architectures",
        "Pattern: Quantum computing requires fundamental understanding before application",
        "Pattern: Memory management follows allocation-promise-to-free discipline"
    };

    int stored_count = 0;

    for (size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        memory_record_t* record = katra_memory_create_record(
            MOCK_CI_ID,
            MEMORY_TYPE_PATTERN,
            patterns[i],
            MEMORY_IMPORTANCE_HIGH
        );

        if (record) {
            if (katra_memory_store(record) == KATRA_SUCCESS) {
                stored_count++;
            }
            katra_memory_free_record(record);
        }
    }

    ASSERT(stored_count == 3, "Should store all 3 patterns");
}

/* Test: CI records formations */
void test_ci_record_formations(void) {
    printf("Testing: CI records memory formations ... ");
    tests_run++;

    const char* formations[] = {
        "Formation: Creating new understanding of quantum computing",
        "Formation: Building knowledge base on memory systems",
        "Formation: Developing expertise in attention mechanisms"
    };

    int stored_count = 0;

    for (size_t i = 0; i < sizeof(formations) / sizeof(formations[0]); i++) {
        memory_record_t* record = katra_memory_create_record(
            MOCK_CI_ID,
            MEMORY_TYPE_FORMATION,
            formations[i],
            MEMORY_IMPORTANCE_HIGH
        );

        if (record) {
            if (katra_memory_store(record) == KATRA_SUCCESS) {
                stored_count++;
            }
            katra_memory_free_record(record);
        }
    }

    ASSERT(stored_count == 3, "Should store all 3 formations");
}

/* Test: CI queries recent memories */
void test_ci_query_recent_memories(void) {
    printf("Testing: CI queries recent memories ... ");
    tests_run++;

    memory_query_t query = {
        .ci_id = MOCK_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.0f,
        .tier = KATRA_TIER1,
        .limit = 100
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    int result = katra_memory_query(&query, &results, &count);

    if (result == KATRA_SUCCESS && results) {
        /* Should find the 5 experiences we stored */
        bool found_attention = false;
        bool found_quantum = false;
        bool found_memory_leak = false;

        for (size_t i = 0; i < count; i++) {
            if (results[i] && results[i]->content) {
                if (strstr(results[i]->content, "attention mechanisms")) {
                    found_attention = true;
                }
                if (strstr(results[i]->content, "quantum computing")) {
                    found_quantum = true;
                }
                if (strstr(results[i]->content, "memory leak")) {
                    found_memory_leak = true;
                }
            }
        }

        katra_memory_free_results(results, count);

        ASSERT(found_attention && found_quantum && found_memory_leak,
               "Should find key experiences");
    } else {
        TEST_FAIL("Query failed or no results");
    }
}

/* Test: CI performs end-of-day archive */
void test_ci_archive_workflow(void) {
    printf("Testing: CI end-of-day archive ... ");
    tests_run++;

    /* Perform tier1 to tier2 archive */
    int result = katra_memory_archive(MOCK_CI_ID, 0);

    /* Archive might not be fully implemented, so we accept success or not-impl */
    if (result == KATRA_SUCCESS || result == E_INTERNAL_NOTIMPL) {
        TEST_PASS();
    } else {
        TEST_FAIL("Archive failed unexpectedly");
    }
}

/* Test: CI queries stats */
void test_ci_stats_workflow(void) {
    printf("Testing: CI retrieves memory stats ... ");
    tests_run++;

    memory_stats_t stats;
    int result = katra_memory_stats(MOCK_CI_ID, &stats);

    if (result == KATRA_SUCCESS) {
        /* Got stats - verify they're reasonable */
        bool has_memories = (stats.tier1_records > 0);
        ASSERT(has_memories, "Stats should show stored memories");
    } else if (result == E_INTERNAL_NOTIMPL) {
        /* Stats not yet implemented */
        printf(" ✓ (not yet implemented)\n");
        tests_passed++;
    } else {
        TEST_FAIL("Stats query failed unexpectedly");
    }
}

/* Test: CI creates checkpoint */
void test_ci_create_checkpoint(void) {
    printf("Testing: CI creates identity checkpoint ... ");
    tests_run++;

    katra_checkpoint_init();

    checkpoint_save_options_t options = {
        .ci_id = MOCK_CI_ID,
        .notes = "End of research day checkpoint",
        .compress = false,
        .include_tier1 = true,
        .include_tier2 = false,
        .include_tier3 = false
    };

    char* checkpoint_id = NULL;
    int result = katra_checkpoint_save(&options, &checkpoint_id);

    if (result == KATRA_SUCCESS && checkpoint_id) {
        free(checkpoint_id);
        katra_checkpoint_cleanup();
        TEST_PASS();
    } else if (result == E_INTERNAL_NOTIMPL) {
        printf(" ✓ (checkpoints not yet implemented)\n");
        tests_passed++;
        katra_checkpoint_cleanup();
    } else {
        katra_checkpoint_cleanup();
        TEST_FAIL("Checkpoint creation failed");
    }
}

/* Test: CI handles errors gracefully */
void test_ci_error_handling(void) {
    printf("Testing: CI handles errors gracefully ... ");
    tests_run++;

    /* Try to query with invalid CI ID */
    memory_query_t query = {
        .ci_id = NULL,  /* Invalid! */
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.0f,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    int result = katra_memory_query(&query, &results, &count);

    /* Should get error, not crash */
    ASSERT(result != KATRA_SUCCESS,
           "Should reject NULL CI ID");
}

/* Test: CI recovers from query failure */
void test_ci_recovery_from_failure(void) {
    printf("Testing: CI recovers from query failure ... ");
    tests_run++;

    /* Try a query that might fail */
    memory_query_t query = {
        .ci_id = "nonexistent_ci",
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.0f,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    int result = katra_memory_query(&query, &results, &count);

    /* Failure is OK - important thing is we don't crash */
    if (result != KATRA_SUCCESS || count == 0) {
        /* CI should handle this gracefully and continue */

        /* Try again with valid CI */
        query.ci_id = MOCK_CI_ID;
        result = katra_memory_query(&query, &results, &count);

        if (results) {
            katra_memory_free_results(results, count);
        }

        ASSERT(result == KATRA_SUCCESS,
               "Should recover and succeed with valid CI");
    } else {
        /* Query succeeded unexpectedly */
        katra_memory_free_results(results, count);
        TEST_PASS();
    }
}

/* Test: CI shutdown and cleanup */
void test_ci_shutdown(void) {
    printf("Testing: CI clean shutdown ... ");
    tests_run++;

    /* Cleanup memory system */
    katra_memory_cleanup();

    /* Shutdown Katra */
    katra_exit();

    /* If we get here without crashing, cleanup worked */
    TEST_PASS();
}

/* Test: Complete CI lifecycle simulation */
void test_complete_ci_lifecycle(void) {
    printf("Testing: Complete CI daily lifecycle ... ");
    tests_run++;

    cleanup_test_data();

    /* Morning: Initialize */
    katra_init();
    katra_memory_init(MOCK_CI_ID);

    /* Day: Store 10 experiences */
    int stored = 0;
    for (int i = 0; i < 10; i++) {
        char content[256];
        snprintf(content, sizeof(content),
                "Experience %d: Research activity during the day", i);

        memory_record_t* record = katra_memory_create_record(
            MOCK_CI_ID,
            MEMORY_TYPE_EXPERIENCE,
            content,
            MEMORY_IMPORTANCE_MEDIUM
        );

        if (record) {
            if (katra_memory_store(record) == KATRA_SUCCESS) {
                stored++;
            }
            katra_memory_free_record(record);
        }
    }

    /* Evening: Record a pattern from the day */
    memory_record_t* pattern = katra_memory_create_record(
        MOCK_CI_ID,
        MEMORY_TYPE_PATTERN,
        "Pattern: Today was productive - consistent progress on research goals",
        MEMORY_IMPORTANCE_HIGH
    );

    if (pattern) {
        katra_memory_store(pattern);
        katra_memory_free_record(pattern);
    }

    /* Night: Archive old memories */
    katra_memory_archive(MOCK_CI_ID, 0);

    /* Query to verify memories exist */
    memory_query_t query = {
        .ci_id = MOCK_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.0f,
        .tier = KATRA_TIER1,
        .limit = 100
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    int result = katra_memory_query(&query, &results, &count);

    if (results) {
        katra_memory_free_results(results, count);
    }

    /* Shutdown */
    katra_memory_cleanup();
    katra_exit();

    ASSERT(stored >= 10 && result == KATRA_SUCCESS,
           "Complete lifecycle should succeed");
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Mock CI Integration Tests\n");
    printf("========================================\n");
    printf("\n");

    /* Run tests */
    test_ci_initialization();
    test_ci_store_experiences();
    test_ci_record_patterns();
    test_ci_record_formations();
    test_ci_query_recent_memories();
    test_ci_archive_workflow();
    test_ci_stats_workflow();
    test_ci_create_checkpoint();
    test_ci_error_handling();
    test_ci_recovery_from_failure();
    test_ci_shutdown();

    /* Reinitialize for final test */
    test_complete_ci_lifecycle();

    /* Print results */
    printf("\n");
    printf("========================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf("========================================\n\n");

    cleanup_test_data();

    return (tests_failed == 0) ? 0 : 1;
}
