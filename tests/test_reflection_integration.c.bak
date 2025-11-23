/* © 2025 Casey Koons All rights reserved */

/* End-to-End Reflection Integration Test */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "katra_init.h"
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"

#define TEST_CI_ID "test_reflection_ci"

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

/* Scenario: Complete session with conscious reflection workflow */
static int test_complete_reflection_workflow(void) {
    printf("\n=== Complete Reflection Workflow ===\n");
    int failures = 0;

    /* TURN 1: Initial exploration */
    printf("\n--- Turn 1: Initial Exploration ---\n");
    int result = begin_turn();
    failures += check_result("Turn 1: begin_turn()", result, KATRA_SUCCESS);

    result = remember("Exploring the memory reflection system", WHY_INTERESTING);
    failures += check_result("Turn 1: remember exploration", result, KATRA_SUCCESS);

    result = learn("Personal collections preserve identity-defining memories");
    failures += check_result("Turn 1: learn about personal collections", result, KATRA_SUCCESS);

    /* End-of-turn reflection: Review what was created */
    size_t turn1_count = 0;
    char** turn1_memories = get_memories_this_turn(&turn1_count);
    if (turn1_count == 2) {
        tests_run++;
        tests_passed++;
        printf("  ✓ Turn 1: Created 2 memories for review\n");
    } else {
        tests_run++;
        printf("  ✗ Turn 1: Expected 2 memories, got %zu\n", turn1_count);
        failures++;
    }

    /* CI decides the learning is personally important */
    if (turn1_memories && turn1_count >= 2) {
        result = add_to_personal_collection(turn1_memories[1], "Learning/MemorySystems");
        failures += check_result("Turn 1: Add learning to personal collection",
                                result, KATRA_SUCCESS);
    }

    if (turn1_memories) {
        free_memory_list(turn1_memories, turn1_count);
    }

    result = end_turn();
    failures += check_result("Turn 1: end_turn()", result, KATRA_SUCCESS);

    /* TURN 2: Working with personal memories */
    printf("\n--- Turn 2: Working with Personal Memories ---\n");
    result = begin_turn();
    failures += check_result("Turn 2: begin_turn()", result, KATRA_SUCCESS);

    result = remember("This is a breakthrough moment", WHY_CRITICAL);
    failures += check_result("Turn 2: remember breakthrough", result, KATRA_SUCCESS);

    result = reflect("Personal collections let CIs consciously shape their identity");
    failures += check_result("Turn 2: reflect on identity", result, KATRA_SUCCESS);

    /* End-of-turn reflection */
    size_t turn2_count = 0;
    char** turn2_memories = get_memories_this_turn(&turn2_count);

    if (turn2_memories && turn2_count >= 2) {
        /* Mark breakthrough as personal */
        result = add_to_personal_collection(turn2_memories[0], "Moments/Breakthrough");
        failures += check_result("Turn 2: Add breakthrough to personal collection",
                                result, KATRA_SUCCESS);

        /* Mark reflection as personal and not to archive */
        bool personal = true;
        bool not_to_archive = true;
        result = update_memory_metadata(turn2_memories[1], &personal, &not_to_archive,
                                       "Reflections/Identity");
        failures += check_result("Turn 2: Update reflection metadata",
                                result, KATRA_SUCCESS);

        /* Review both memories */
        for (size_t i = 0; i < turn2_count; i++) {
            result = review_memory(turn2_memories[i]);
            if (result == KATRA_SUCCESS) {
                tests_run++;
                tests_passed++;
                printf("  ✓ Turn 2: Reviewed memory %zu\n", i + 1);
            } else {
                tests_run++;
                printf("  ✗ Turn 2: Failed to review memory %zu\n", i + 1);
                failures++;
            }
        }
    }

    if (turn2_memories) {
        free_memory_list(turn2_memories, turn2_count);
    }

    result = end_turn();
    failures += check_result("Turn 2: end_turn()", result, KATRA_SUCCESS);

    /* TURN 3: Content revision based on deeper understanding */
    printf("\n--- Turn 3: Content Revision ---\n");
    result = begin_turn();
    failures += check_result("Turn 3: begin_turn()", result, KATRA_SUCCESS);

    result = remember("Initial understanding of metadata", WHY_INTERESTING);
    failures += check_result("Turn 3: remember initial understanding", result, KATRA_SUCCESS);

    /* Get memory for revision */
    size_t turn3_count = 0;
    char** turn3_memories = get_memories_this_turn(&turn3_count);

    if (turn3_memories && turn3_count > 0) {
        /* Revise content after deeper thought */
        result = revise_memory_content(turn3_memories[0],
            "Deeper understanding: Metadata enables conscious curation of identity");
        failures += check_result("Turn 3: Revise content", result, KATRA_SUCCESS);
    }

    if (turn3_memories) {
        free_memory_list(turn3_memories, turn3_count);
    }

    result = end_turn();
    failures += check_result("Turn 3: end_turn()", result, KATRA_SUCCESS);

    /* END-OF-SESSION REFLECTION */
    printf("\n--- End-of-Session Reflection ---\n");
    size_t session_count = 0;
    char** session_memories = get_memories_this_session(&session_count);

    /* Should have at least 5 memories from 3 turns */
    if (session_count >= 5) {
        tests_run++;
        tests_passed++;
        printf("  ✓ Session: Created %zu memories total\n", session_count);
    } else {
        tests_run++;
        printf("  ✗ Session: Expected at least 5 memories, got %zu\n", session_count);
        failures++;
    }

    if (session_memories) {
        free_memory_list(session_memories, session_count);
    }

    /* VERIFY: Personal collection memories are preserved */
    printf("\n--- Verification: Personal Collection ---\n");

    memory_query_t personal_query = {
        .ci_id = TEST_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 0,
        .filter_personal = true,
        .personal_value = true
    };

    memory_record_t** personal_results = NULL;
    size_t personal_count = 0;

    result = katra_memory_query(&personal_query, &personal_results, &personal_count);
    if (result == KATRA_SUCCESS && personal_count >= 3) {
        tests_run++;
        tests_passed++;
        printf("  ✓ Found %zu personal memories\n", personal_count);

        /* Verify collections are set */
        int has_learning = 0;
        int has_moments = 0;
        int has_reflections = 0;

        for (size_t i = 0; i < personal_count; i++) {
            if (personal_results[i]->collection) {
                if (strstr(personal_results[i]->collection, "Learning")) has_learning = 1;
                if (strstr(personal_results[i]->collection, "Moments")) has_moments = 1;
                if (strstr(personal_results[i]->collection, "Reflections")) has_reflections = 1;
            }
        }

        failures += check_result("Has Learning collection", has_learning, 1);
        failures += check_result("Has Moments collection", has_moments, 1);
        failures += check_result("Has Reflections collection", has_reflections, 1);

        katra_memory_free_results(personal_results, personal_count);
    } else {
        tests_run++;
        printf("  ✗ Expected at least 3 personal memories, got %zu\n", personal_count);
        failures++;
    }

    /* VERIFY: Working context includes personal memories */
    printf("\n--- Verification: Working Context ---\n");
    char* context = get_working_context();
    if (context) {
        int has_personal_section = strstr(context, "Personal Collection") != NULL;
        failures += check_result("Context includes Personal Collection section",
                                has_personal_section, 1);

        int has_learning_in_context = strstr(context, "Learning") != NULL;
        failures += check_result("Context includes Learning collection",
                                has_learning_in_context, 1);

        free(context);
    } else {
        tests_run++;
        printf("  ✗ Failed to get working context\n");
        failures++;
    }

    return failures;
}

int main(void) {
    int total_failures = 0;

    printf("========================================\n");
    printf("Reflection Integration Test\n");
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

    /* Run integration test */
    total_failures += test_complete_reflection_workflow();

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
