/* © 2025 Casey Koons All rights reserved */

/* Test: Context Persistence - Session continuity through cognitive snapshots */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "katra_breathing.h"
#include "katra_breathing_context_persist.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_init.h"

#define TEST_CI_ID "test_context_persist"

/* Test counter */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        tests_failed++; \
        return; \
    } \
    tests_passed++; \
} while (0)

/* Cleanup test environment */
static void cleanup_test_env(void) {
    breathe_cleanup();
    katra_memory_cleanup();
}

/* Test 1: Initialize context persistence */
static void test_init(void) {
    printf("Test 1: Initialize context persistence... ");

    int result = session_start(TEST_CI_ID);
    TEST_ASSERT(result == KATRA_SUCCESS, "Session start failed");

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 2: Update current focus */
static void test_update_focus(void) {
    printf("Test 2: Update current focus... ");

    session_start(TEST_CI_ID);

    int result = update_current_focus("Testing context persistence");
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to update focus");

    const char* focus = get_current_focus_snapshot(TEST_CI_ID);
    TEST_ASSERT(focus != NULL, "Focus is NULL");
    TEST_ASSERT(strcmp(focus, "Testing context persistence") == 0, "Focus mismatch");

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 3: Add pending questions */
static void test_pending_questions(void) {
    printf("Test 3: Add pending questions... ");

    session_start(TEST_CI_ID);

    int result = add_pending_question("How does context restoration work?");
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to add question 1");

    result = add_pending_question("What gets stored in snapshots?");
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to add question 2");

    size_t count = 0;
    char** questions = get_pending_questions_snapshot(TEST_CI_ID, &count);
    TEST_ASSERT(questions != NULL, "Questions is NULL");
    TEST_ASSERT(count == 2, "Question count mismatch");

    free_memory_list(questions, count);

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 4: Mark files modified */
static void test_file_modifications(void) {
    printf("Test 4: Mark files modified... ");

    session_start(TEST_CI_ID);

    int result = mark_file_modified("test.c", "created");
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to mark file 1");

    result = mark_file_modified("test.h", "edited");
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to mark file 2");

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 5: Record accomplishments */
static void test_accomplishments(void) {
    printf("Test 5: Record accomplishments... ");

    session_start(TEST_CI_ID);

    int result = record_accomplishment("Implemented context persistence");
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to record accomplishment 1");

    result = record_accomplishment("Added MCP integration");
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to record accomplishment 2");

    char* summary = get_project_state_summary_snapshot(TEST_CI_ID);
    TEST_ASSERT(summary != NULL, "Summary is NULL");

    free(summary);

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 6: Update communication style */
static void test_communication_style(void) {
    printf("Test 6: Update communication style... ");

    session_start(TEST_CI_ID);

    int result = update_communication_style("Direct technical collaboration");
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to update style");

    result = update_user_preferences("Prefers goto cleanup, no magic numbers");
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to update preferences");

    char* context = get_relationship_context_snapshot(TEST_CI_ID);
    TEST_ASSERT(context != NULL, "Relationship context is NULL");

    free(context);

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 7: Update thinking patterns */
static void test_thinking_patterns(void) {
    printf("Test 7: Update thinking patterns... ");

    session_start(TEST_CI_ID);

    int result = update_thinking_patterns("Systematic, verify with tests, extract at 3rd usage");
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to update thinking patterns");

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 8: Capture context snapshot */
static void test_capture_snapshot(void) {
    printf("Test 8: Capture context snapshot... ");

    session_start(TEST_CI_ID);

    /* Set up context */
    update_current_focus("Testing snapshot capture");
    add_pending_question("Does snapshot capture work?");
    record_accomplishment("Created test suite");

    /* Capture snapshot */
    int result = capture_context_snapshot(TEST_CI_ID, NULL);
    TEST_ASSERT(result == KATRA_SUCCESS, "Failed to capture snapshot");

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 9: Restore context as latent space */
static void test_restore_latent_space(void) {
    printf("Test 9: Restore context as latent space... ");

    session_start(TEST_CI_ID);

    /* Set up and capture context */
    update_current_focus("Testing latent space restoration");
    record_accomplishment("Completed snapshot implementation");
    update_user_preferences("Prefers markdown formatting");
    capture_context_snapshot(TEST_CI_ID, NULL);

    /* Restore as latent space */
    char* latent_space = restore_context_as_latent_space(TEST_CI_ID);
    TEST_ASSERT(latent_space != NULL, "Latent space is NULL");
    TEST_ASSERT(strlen(latent_space) > 0, "Latent space is empty");

    /* Verify it contains expected sections */
    TEST_ASSERT(strstr(latent_space, "Current Focus") != NULL, "Missing focus section");
    TEST_ASSERT(strstr(latent_space, "Testing latent space restoration") != NULL,
                "Missing focus content");

    free(latent_space);

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 10: Session end auto-snapshot */
static void test_session_end_snapshot(void) {
    printf("Test 10: Session end auto-snapshot... ");

    session_start(TEST_CI_ID);

    /* Set up context */
    update_current_focus("Testing auto-snapshot on session end");
    record_accomplishment("Implemented session_end integration");

    /* End session (should auto-capture) */
    int result = session_end();
    TEST_ASSERT(result == KATRA_SUCCESS, "Session end failed");

    /* Start new session and verify snapshot exists */
    session_start(TEST_CI_ID);
    char* latent_space = restore_context_as_latent_space(TEST_CI_ID);
    TEST_ASSERT(latent_space != NULL, "Auto-snapshot not found");

    free(latent_space);

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 11: Cross-session continuity */
static void test_cross_session_continuity(void) {
    printf("Test 11: Cross-session continuity... ");

    /* Session 1: Create context */
    session_start(TEST_CI_ID);
    update_current_focus("Session 1 focus");
    update_thinking_patterns("Pattern from session 1");
    session_end();

    /* Session 2: Verify context restored */
    session_start(TEST_CI_ID);
    char* latent_space = restore_context_as_latent_space(TEST_CI_ID);
    TEST_ASSERT(latent_space != NULL, "Context not restored across sessions");
    TEST_ASSERT(strstr(latent_space, "Session 1 focus") != NULL,
                "Focus not restored");
    TEST_ASSERT(strstr(latent_space, "Pattern from session 1") != NULL,
                "Thinking patterns not restored");

    free(latent_space);
    session_end();

    printf("PASS\n");
    cleanup_test_env();
}

/* Test 12: Empty snapshot handling */
static void test_empty_snapshot(void) {
    printf("Test 12: Empty snapshot handling... ");

    session_start(TEST_CI_ID);

    /* Try to restore before any snapshot exists */
    char* latent_space = restore_context_as_latent_space("nonexistent_ci");
    TEST_ASSERT(latent_space == NULL, "Should return NULL for nonexistent CI");

    printf("PASS\n");
    cleanup_test_env();
}

/* Main test runner */
int main(void) {
    printf("\n========================================\n");
    printf("Context Persistence Tests\n");
    printf("========================================\n\n");

    /* Initialize logging */
    log_init(NULL);
    log_set_level(LOG_ERROR);  /* Reduce noise */

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

    /* Run tests */
    test_init();
    test_update_focus();
    test_pending_questions();
    test_file_modifications();
    test_accomplishments();
    test_communication_style();
    test_thinking_patterns();
    test_capture_snapshot();
    test_restore_latent_space();
    test_session_end_snapshot();
    test_cross_session_continuity();
    test_empty_snapshot();

    /* Final cleanup */
    katra_memory_cleanup();
    katra_exit();
    log_cleanup();

    /* Print results */
    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", tests_passed, tests_failed);
    printf("========================================\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
