/* © 2025 Casey Koons All rights reserved */

/* Turn Tracking and Metadata System Tests */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "katra_init.h"
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"

#define TEST_CI_ID "test_turn_tracking_ci"

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

/* Helper: Check result and print status */
static int check_result(const char* test_name, int result, int expected) {
    tests_run++;
    if (result == expected) {
        tests_passed++;
        printf("  ✓ %s\n", test_name);
        return 0;
    } else {
        printf("  ✗ %s (got %d, expected %d)\n", test_name, result, expected);
        return 1;
    }
}

/* Test: Turn state transitions */
static int test_turn_state_transitions(void) {
    printf("\nTesting turn state transitions...\n");
    int failures = 0;

    /* End any active turn from session_start, then check initial state */
    end_turn();
    turn_state_t state = get_turn_state();
    failures += check_result("Initial state is IDLE", state, TURN_STATE_IDLE);

    /* Begin turn */
    int result = begin_turn();
    failures += check_result("begin_turn() succeeds", result, KATRA_SUCCESS);

    state = get_turn_state();
    failures += check_result("State after begin is ACTIVE", state, TURN_STATE_ACTIVE);

    /* Get current turn ID */
    const char* turn_id = get_current_turn_id();
    if (turn_id && strlen(turn_id) > 0) {
        tests_run++;
        tests_passed++;
        printf("  ✓ get_current_turn_id() returns valid ID: %s\n", turn_id);
    } else {
        tests_run++;
        printf("  ✗ get_current_turn_id() returned NULL or empty\n");
        failures++;
    }

    /* End turn */
    result = end_turn();
    failures += check_result("end_turn() succeeds", result, KATRA_SUCCESS);

    state = get_turn_state();
    failures += check_result("State after end is IDLE", state, TURN_STATE_IDLE);

    /* Turn ID should be cleared */
    turn_id = get_current_turn_id();
    if (turn_id == NULL || strlen(turn_id) == 0) {
        tests_run++;
        tests_passed++;
        printf("  ✓ Turn ID cleared after end_turn()\n");
    } else {
        tests_run++;
        printf("  ✗ Turn ID should be cleared: %s\n", turn_id);
        failures++;
    }

    return failures;
}

/* Test: Memory tracking during turn */
static int test_memory_tracking_in_turn(void) {
    printf("\nTesting memory tracking during turn...\n");
    int failures = 0;

    /* Start a new turn */
    int result = begin_turn();
    failures += check_result("Begin new turn", result, KATRA_SUCCESS);

    /* Create some memories */
    result = remember("First thought in turn", WHY_INTERESTING);
    failures += check_result("remember() first thought", result, KATRA_SUCCESS);

    result = remember("Second thought in turn", WHY_SIGNIFICANT);
    failures += check_result("remember() second thought", result, KATRA_SUCCESS);

    result = learn("Some knowledge in turn");
    failures += check_result("learn() in turn", result, KATRA_SUCCESS);

    /* Get memories from this turn */
    size_t count = 0;
    char** memories = get_memories_this_turn(&count);

    if (memories && count >= 3) {
        tests_run++;
        tests_passed++;
        printf("  ✓ get_memories_this_turn() returned %zu memories\n", count);
    } else {
        tests_run++;
        printf("  ✗ Expected at least 3 memories, got %zu\n", count);
        failures++;
    }

    /* Verify memory IDs are not empty */
    if (memories && count > 0) {
        int valid_ids = 1;
        for (size_t i = 0; i < count; i++) {
            if (!memories[i] || strlen(memories[i]) == 0) {
                valid_ids = 0;
                break;
            }
        }
        failures += check_result("All memory IDs are valid", valid_ids, 1);
    }

    if (memories) {
        free_memory_list(memories, count);
    }

    /* End turn */
    result = end_turn();
    failures += check_result("End turn", result, KATRA_SUCCESS);

    /* After ending, no memories should be in current turn */
    memories = get_memories_this_turn(&count);
    failures += check_result("No memories after end_turn()", count, 0);

    if (memories) {
        free_memory_list(memories, count);
    }

    return failures;
}

/* Test: Session-level memory tracking */
static int test_session_memory_tracking(void) {
    printf("\nTesting session-level memory tracking...\n");
    int failures = 0;

    /* Get session memories before any turns */
    size_t initial_count = 0;
    char** initial_memories = get_memories_this_session(&initial_count);

    printf("  Initial session memory count: %zu\n", initial_count);

    if (initial_memories) {
        free_memory_list(initial_memories, initial_count);
    }

    /* First turn */
    int result = begin_turn();
    failures += check_result("Begin first turn", result, KATRA_SUCCESS);

    result = remember("Turn 1 memory", WHY_INTERESTING);
    failures += check_result("Create memory in turn 1", result, KATRA_SUCCESS);

    result = end_turn();
    failures += check_result("End first turn", result, KATRA_SUCCESS);

    /* Second turn */
    result = begin_turn();
    failures += check_result("Begin second turn", result, KATRA_SUCCESS);

    result = remember("Turn 2 memory", WHY_INTERESTING);
    failures += check_result("Create memory in turn 2", result, KATRA_SUCCESS);

    result = end_turn();
    failures += check_result("End second turn", result, KATRA_SUCCESS);

    /* Get all session memories */
    size_t session_count = 0;
    char** session_memories = get_memories_this_session(&session_count);

    /* Should have at least initial + 2 new memories */
    if (session_memories && session_count >= initial_count + 2) {
        tests_run++;
        tests_passed++;
        printf("  ✓ Session has %zu total memories (at least 2 new)\n", session_count);
    } else {
        tests_run++;
        printf("  ✗ Expected at least %zu memories, got %zu\n",
               initial_count + 2, session_count);
        failures++;
    }

    if (session_memories) {
        free_memory_list(session_memories, session_count);
    }

    return failures;
}

/* Test: Metadata update operations */
static int test_metadata_updates(void) {
    printf("\nTesting metadata update operations...\n");
    int failures = 0;

    /* Create a memory and get its ID */
    begin_turn();

    int result = remember("Test memory for metadata", WHY_INTERESTING);
    failures += check_result("Create test memory", result, KATRA_SUCCESS);

    size_t count = 0;
    char** memories = get_memories_this_turn(&count);

    if (!memories || count == 0) {
        printf("  ✗ Failed to get memory for testing metadata\n");
        end_turn();
        return failures + 1;
    }

    const char* memory_id = memories[0];
    printf("  Testing with memory ID: %s\n", memory_id);

    /* Test: Mark as personal */
    bool personal = true;
    result = update_memory_metadata(memory_id, &personal, NULL, NULL);
    failures += check_result("Mark memory as personal", result, KATRA_SUCCESS);

    /* Test: Set not_to_archive flag */
    bool not_to_archive = true;
    result = update_memory_metadata(memory_id, NULL, &not_to_archive, NULL);
    failures += check_result("Set not_to_archive flag", result, KATRA_SUCCESS);

    /* Test: Add to collection */
    result = update_memory_metadata(memory_id, NULL, NULL, "Testing/Metadata");
    failures += check_result("Add to collection", result, KATRA_SUCCESS);

    /* Test: Update multiple fields at once */
    personal = true;
    not_to_archive = true;
    result = update_memory_metadata(memory_id, &personal, &not_to_archive,
                                    "Testing/Complete");
    failures += check_result("Update multiple fields", result, KATRA_SUCCESS);

    /* Test: Helper function - add to personal collection */
    result = add_to_personal_collection(memory_id, "Personal/Important");
    failures += check_result("add_to_personal_collection()", result, KATRA_SUCCESS);

    /* Test: Helper function - remove from personal collection */
    result = remove_from_personal_collection(memory_id);
    failures += check_result("remove_from_personal_collection()", result, KATRA_SUCCESS);

    /* Test: Review memory */
    result = review_memory(memory_id);
    failures += check_result("review_memory()", result, KATRA_SUCCESS);

    /* Cleanup */
    free_memory_list(memories, count);
    end_turn();

    return failures;
}

/* Test: Error handling */
static int test_error_handling(void) {
    printf("\nTesting error handling...\n");
    int failures = 0;

    /* Test: Update non-existent memory */
    bool personal = true;
    int result = update_memory_metadata("nonexistent_id_12345", &personal, NULL, NULL);
    failures += check_result("Update non-existent memory fails",
                            result != KATRA_SUCCESS, 1);

    /* Test: Update with NULL memory_id */
    result = update_memory_metadata(NULL, &personal, NULL, NULL);
    failures += check_result("Update with NULL memory_id fails",
                            result != KATRA_SUCCESS, 1);

    /* Test: Update with no metadata fields */
    begin_turn();
    remember("Test memory", WHY_INTERESTING);
    size_t count = 0;
    char** memories = get_memories_this_turn(&count);

    if (memories && count > 0) {
        result = update_memory_metadata(memories[0], NULL, NULL, NULL);
        failures += check_result("Update with no fields fails",
                                result != KATRA_SUCCESS, 1);
        free_memory_list(memories, count);
    }

    end_turn();

    /* Test: Begin turn when already active */
    begin_turn();
    result = begin_turn();
    failures += check_result("begin_turn() when already active",
                            result, KATRA_SUCCESS);  /* Should be idempotent */
    end_turn();

    /* Test: End turn when not active */
    result = end_turn();
    failures += check_result("end_turn() when not active",
                            result, KATRA_SUCCESS);  /* Should be idempotent */

    return failures;
}

/* Test: Content revision */
static int test_content_revision(void) {
    printf("\nTesting content revision...\n");
    int failures = 0;

    begin_turn();

    int result = remember("Original content", WHY_INTERESTING);
    failures += check_result("Create memory for revision", result, KATRA_SUCCESS);

    size_t count = 0;
    char** memories = get_memories_this_turn(&count);

    if (memories && count > 0) {
        const char* memory_id = memories[0];

        /* Revise content */
        result = revise_memory_content(memory_id, "Revised content after reflection");
        failures += check_result("revise_memory_content()", result, KATRA_SUCCESS);

        /* TODO: Verify content was actually updated by querying the memory */

        free_memory_list(memories, count);
    } else {
        printf("  ✗ Failed to create memory for revision test\n");
        failures++;
    }

    end_turn();
    return failures;
}

int main(void) {
    int total_failures = 0;

    printf("========================================\n");
    printf("Turn Tracking and Metadata System Tests\n");
    printf("========================================\n");

    /* Initialize Katra */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        printf("FATAL: katra_init failed: %d\n", result);
        return 1;
    }

    result = katra_memory_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("FATAL: katra_memory_init failed: %d\n", result);
        katra_exit();
        return 1;
    }

    result = breathe_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("FATAL: breathe_init failed: %d\n", result);
        katra_memory_cleanup();
        katra_exit();
        return 1;
    }

    result = session_start(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("FATAL: session_start failed: %d\n", result);
        breathe_cleanup();
        katra_memory_cleanup();
        katra_exit();
        return 1;
    }

    /* Run all tests */
    total_failures += test_turn_state_transitions();
    total_failures += test_memory_tracking_in_turn();
    total_failures += test_session_memory_tracking();
    total_failures += test_metadata_updates();
    total_failures += test_error_handling();
    total_failures += test_content_revision();

    /* Cleanup */
    session_end();
    breathe_cleanup();
    katra_memory_cleanup();
    katra_exit();

    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", total_failures);
    printf("========================================\n");

    return total_failures > 0 ? 1 : 0;
}
