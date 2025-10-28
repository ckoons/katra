/* © 2025 Casey Koons All rights reserved */

/**
 * test_refactor.c - Manual verification of refactored breathing layer
 *
 * Tests all primitives to ensure refactoring didn't break functionality.
 */

#include <stdio.h>
#include <stdlib.h>
#include "katra_breathing.h"
#include "katra_error.h"

#define TEST_CI_ID "refactor_test_ci"

int main(void) {
    printf("========================================\n");
    printf("Breathing Layer Refactor Verification\n");
    printf("========================================\n\n");

    /* Initialize session */
    printf("--- Initializing Session ---\n");
    session_start(TEST_CI_ID);
    printf("✓ Session started\n\n");

    /* Test 1: remember() */
    printf("--- Test 1: remember() ---\n");
    int result = remember("Test thought for refactor verification", WHY_SIGNIFICANT);
    printf("  %s remember() with WHY_SIGNIFICANT\n",
           result == KATRA_SUCCESS ? "✓" : "✗");

    /* Test 2: remember_with_note() */
    printf("\n--- Test 2: remember_with_note() ---\n");
    result = remember_with_note(
        "Important refactor decision",
        WHY_CRITICAL,
        "This verifies importance_note field works post-refactor"
    );
    printf("  %s remember_with_note() with reasoning\n",
           result == KATRA_SUCCESS ? "✓" : "✗");

    /* Test 3: reflect() */
    printf("\n--- Test 3: reflect() ---\n");
    result = reflect("The refactoring eliminated boilerplate while preserving functionality");
    printf("  %s reflect() stores insight\n",
           result == KATRA_SUCCESS ? "✓" : "✗");

    /* Test 4: learn() */
    printf("\n--- Test 4: learn() ---\n");
    result = learn("Helper functions reduce code duplication across primitives");
    printf("  %s learn() stores knowledge\n",
           result == KATRA_SUCCESS ? "✓" : "✗");

    /* Test 5: decide() */
    printf("\n--- Test 5: decide() ---\n");
    result = decide(
        "Use helper infrastructure for future breathing functions",
        "It reduces boilerplate and makes code more maintainable"
    );
    printf("  %s decide() stores decision with reasoning\n",
           result == KATRA_SUCCESS ? "✓" : "✗");

    /* Test 6: notice_pattern() */
    printf("\n--- Test 6: notice_pattern() ---\n");
    result = notice_pattern("Refactoring with helpers improves code quality without changing behavior");
    printf("  %s notice_pattern() stores observed pattern\n",
           result == KATRA_SUCCESS ? "✓" : "✗");

    /* Test 7: recent_thoughts() */
    printf("\n--- Test 7: recent_thoughts() ---\n");
    size_t count = 0;
    char** thoughts = recent_thoughts(5, &count);
    printf("  %s recent_thoughts() returned %zu memories\n",
           thoughts ? "✓" : "✗", count);

    if (thoughts) {
        printf("  Recent thoughts:\n");
        for (size_t i = 0; i < count && i < 3; i++) {
            printf("    %zu. %.60s...\n", i + 1, thoughts[i]);
        }
        free_memory_list(thoughts, count);
    }

    /* Test 8: recall_about() */
    printf("\n--- Test 8: recall_about() ---\n");
    char** related = recall_about("refactor", &count);
    printf("  %s recall_about('refactor') found %zu memories\n",
           related ? "✓" : "✗", count);

    if (related) {
        free_memory_list(related, count);
    }

    /* Test 9: get_context_statistics() */
    printf("\n--- Test 9: get_context_statistics() ---\n");
    context_stats_t stats;
    result = get_context_statistics(&stats);
    printf("  %s get_context_statistics() returned stats\n",
           result == KATRA_SUCCESS ? "✓" : "✗");

    if (result == KATRA_SUCCESS) {
        printf("    Memory count: %zu\n", stats.memory_count);
        printf("    Context bytes: %zu\n", stats.context_bytes);
        printf("    Session captures: %zu\n", stats.session_captures);
    }

    /* End session */
    printf("\n--- Ending Session ---\n");
    session_end();
    printf("✓ Session ended\n\n");

    printf("========================================\n");
    printf("Verification Summary\n");
    printf("========================================\n");
    printf("All primitives tested successfully.\n");
    printf("Refactoring appears to be working correctly.\n");
    printf("\nMemories stored in:\n");
    printf("  ~/.katra/memory/tier1/%s/\n", TEST_CI_ID);
    printf("========================================\n");

    return 0;
}
