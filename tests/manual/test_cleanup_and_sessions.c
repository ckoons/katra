/* © 2025 Casey Koons All rights reserved */

/* Test formalized cleanup order and cross-session continuity */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_error.h"

#define TEST_CI_ID "test_cleanup_sessions_ci"

int main(void) {
    printf("============================================\n");
    printf("Cleanup Order & Session Continuity Test\n");
    printf("============================================\n\n");

    /* ========== Test 1: Formalized Cleanup Order ========== */
    printf("TEST 1: Formalized Cleanup Order\n");
    printf("=====================================\n\n");

    /* Initialize */
    printf("1. Initializing breathing layer...\n");
    int result = breathe_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("   ERROR: Failed to initialize: %d\n", result);
        return 1;
    }
    printf("   ✓ Initialized\n\n");

    /* Store some memories */
    printf("2. Storing memories before cleanup...\n");
    for (int i = 0; i < 10; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Pre-cleanup memory %d", i);
        result = remember(msg, WHY_INTERESTING);
        if (result != KATRA_SUCCESS) {
            printf("   ERROR: Failed to store memory: %d\n", result);
        }
    }
    printf("   ✓ Stored 10 memories\n\n");

    /* Verify we can store before cleanup */
    printf("3. Verifying normal operation...\n");
    result = remember("Should succeed before cleanup", WHY_INTERESTING);
    if (result == KATRA_SUCCESS) {
        printf("   ✓ Memory storage working (result: %d)\n\n", result);
    } else {
        printf("   ✗ UNEXPECTED: Memory storage failed: %d\n\n", result);
    }

    /* Call cleanup - this should trigger the 5-step process */
    printf("4. Calling breathe_cleanup() (watch logs for 5 steps)...\n");
    breathe_cleanup();
    printf("   ✓ Cleanup completed\n\n");

    /* Try to use after cleanup - should fail */
    printf("5. Testing operations after cleanup (should fail)...\n");
    result = remember("Should fail after cleanup", WHY_INTERESTING);
    if (result == E_INVALID_STATE) {
        printf("   ✓ Correctly rejected with E_INVALID_STATE (%d)\n\n", result);
    } else {
        printf("   ✗ UNEXPECTED: Got result %d, expected %d\n\n",
               result, E_INVALID_STATE);
    }

    /* Test double cleanup - should be safe */
    printf("6. Testing double cleanup (should be safe)...\n");
    breathe_cleanup();
    printf("   ✓ No crash on double cleanup\n\n");

    /* Test reinitialize after cleanup */
    printf("7. Re-initializing after cleanup...\n");
    result = breathe_init(TEST_CI_ID);
    if (result == KATRA_SUCCESS) {
        printf("   ✓ Re-initialization successful\n\n");

        /* Verify we can store again */
        result = remember("After reinit", WHY_INTERESTING);
        if (result == KATRA_SUCCESS) {
            printf("   ✓ Memory storage works after reinit\n\n");
        } else {
            printf("   ✗ Memory storage failed after reinit: %d\n\n", result);
        }

        breathe_cleanup();
    } else {
        printf("   ✗ Re-initialization failed: %d\n\n", result);
    }

    /* ========== Test 2: Cross-Session Continuity ========== */
    printf("\nTEST 2: Cross-Session Continuity\n");
    printf("=====================================\n\n");

    /* Session 1 */
    printf("1. Starting Session 1...\n");
    result = session_start(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("   ERROR: Failed to start session 1: %d\n", result);
        return 1;
    }

    /* Store memories in session 1 */
    printf("   Storing memories in Session 1:\n");
    remember("Session 1 memory A - important discovery", WHY_SIGNIFICANT);
    remember("Session 1 memory B - interesting pattern", WHY_INTERESTING);
    remember("Session 1 memory C - routine observation", WHY_ROUTINE);
    printf("   ✓ Stored 3 memories in Session 1\n");

    /* Get session ID for reference */
    enhanced_stats_t* stats = get_enhanced_statistics();
    if (stats) {
        printf("   Session start time: %ld\n", (long)stats->session_start_time);
        free(stats);
    }

    session_end();
    breathe_cleanup();
    printf("   ✓ Session 1 ended\n\n");

    /* Small delay to ensure different timestamps */
    sleep(1);

    /* Session 2 */
    printf("2. Starting Session 2...\n");
    result = session_start(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("   ERROR: Failed to start session 2: %d\n", result);
        return 1;
    }

    /* Store different memories in session 2 */
    printf("   Storing memories in Session 2:\n");
    remember("Session 2 memory X - new finding", WHY_SIGNIFICANT);
    remember("Session 2 memory Y - follow-up", WHY_INTERESTING);
    printf("   ✓ Stored 2 memories in Session 2\n\n");

    /* Recall previous session - should get Session 1 memories only */
    printf("3. Recalling previous session (should be Session 1)...\n");
    size_t count = 0;
    char** prev = recall_previous_session(TEST_CI_ID, 50, &count);

    if (prev) {
        printf("   Retrieved %zu memories from previous session:\n", count);

        int found_session_1 = 0;
        int found_session_2 = 0;

        for (size_t i = 0; i < count; i++) {
            printf("   [%zu] %s\n", i + 1, prev[i]);

            /* Check which session these memories are from */
            if (strstr(prev[i], "Session 1")) {
                found_session_1++;
            }
            if (strstr(prev[i], "Session 2")) {
                found_session_2++;
            }
        }

        printf("\n   Validation:\n");
        printf("   - Session 1 memories: %d (expected: 3)\n", found_session_1);
        printf("   - Session 2 memories: %d (expected: 0)\n", found_session_2);

        if (found_session_1 == 3 && found_session_2 == 0) {
            printf("   ✓ Cross-session recall working correctly!\n\n");
        } else {
            printf("   ✗ UNEXPECTED: Session filtering not working correctly\n\n");
        }

        free_memory_list(prev, count);
    } else {
        printf("   ✗ Failed to recall previous session\n");
        printf("   (Count: %zu)\n\n", count);
    }

    /* Test with NULL parameters */
    printf("4. Testing NULL parameter handling...\n");
    prev = recall_previous_session(NULL, 50, &count);
    if (prev == NULL && count == 0) {
        printf("   ✓ NULL ci_id handled correctly\n");
    }

    prev = recall_previous_session(TEST_CI_ID, 50, NULL);
    if (prev == NULL) {
        printf("   ✓ NULL count handled correctly\n");
    }
    printf("\n");

    session_end();
    breathe_cleanup();
    printf("   ✓ Session 2 ended\n\n");

    /* Session 3 - test with no previous session */
    printf("5. Starting Session 3 (previous should be Session 2)...\n");
    result = session_start(TEST_CI_ID);
    if (result == KATRA_SUCCESS) {
        prev = recall_previous_session(TEST_CI_ID, 50, &count);
        if (prev) {
            printf("   Retrieved %zu memories from previous session:\n", count);
            /* Should get Session 2 memories */
            for (size_t i = 0; i < count; i++) {
                printf("   [%zu] %s\n", i + 1, prev[i]);
            }
            free_memory_list(prev, count);
            printf("   ✓ Retrieved Session 2 memories\n");
        }
        session_end();
        breathe_cleanup();
    }
    printf("\n");

    printf("============================================\n");
    printf("All tests completed successfully!\n");
    printf("============================================\n");

    return 0;
}
