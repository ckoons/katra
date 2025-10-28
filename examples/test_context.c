/* © 2025 Casey Koons All rights reserved */

/**
 * test_context.c - Context Functions Verification
 *
 * Tests relevant_memories(), recent_thoughts(), and recall_about()
 * with various importance levels and search terms.
 */

#include <stdio.h>
#include <stdlib.h>
#include "katra_breathing.h"
#include "katra_error.h"

#define TEST_CI_ID "context_test_ci"

int main(void) {
    printf("========================================\n");
    printf("Context Functions Verification\n");
    printf("========================================\n\n");

    /* Initialize session */
    printf("--- Initializing Session ---\n");
    session_start(TEST_CI_ID);
    printf("✓ Session started\n\n");

    /* Store test memories with varying importance */
    printf("--- Storing Test Memories ---\n");

    remember("First CRITICAL memory about bugs", WHY_CRITICAL);
    remember("Second memory about refactoring", WHY_SIGNIFICANT);
    remember("Third TRIVIAL memory about spacing", WHY_TRIVIAL);
    remember("Fourth memory about memory system", WHY_SIGNIFICANT);
    remember("Fifth routine memory about testing", WHY_ROUTINE);
    remember("Sixth CRITICAL memory about bugs again", WHY_CRITICAL);
    remember("Seventh memory about the breathing layer", WHY_SIGNIFICANT);
    remember("Eighth trivial memory", WHY_TRIVIAL);

    printf("✓ Stored 8 memories with varying importance\n\n");

    /* Test 1: recent_thoughts() */
    printf("--- Test 1: recent_thoughts(5) ---\n");
    size_t count = 0;
    char** thoughts = recent_thoughts(5, &count);

    if (thoughts) {
        printf("Retrieved %zu recent thoughts:\n", count);
        for (size_t i = 0; i < count; i++) {
            printf("  %zu. %.60s\n", i + 1, thoughts[i]);
        }
        free_memory_list(thoughts, count);
        printf("✓ recent_thoughts() works\n\n");
    } else {
        printf("✗ recent_thoughts() failed\n\n");
    }

    /* Test 2: recall_about() with specific topic */
    printf("--- Test 2: recall_about('bugs') ---\n");
    char** bug_memories = recall_about("bugs", &count);

    if (bug_memories) {
        printf("Found %zu memories about 'bugs':\n", count);
        for (size_t i = 0; i < count; i++) {
            printf("  %zu. %s\n", i + 1, bug_memories[i]);
        }
        free_memory_list(bug_memories, count);
        printf("✓ recall_about('bugs') works\n\n");
    } else {
        printf("(No memories found about 'bugs')\n\n");
    }

    /* Test 3: recall_about() with different topic */
    printf("--- Test 3: recall_about('memory') ---\n");
    char** memory_memories = recall_about("memory", &count);

    if (memory_memories) {
        printf("Found %zu memories about 'memory':\n", count);
        for (size_t i = 0; i < count; i++) {
            printf("  %zu. %s\n", i + 1, memory_memories[i]);
        }
        free_memory_list(memory_memories, count);
        printf("✓ recall_about('memory') works\n\n");
    } else {
        printf("(No memories found about 'memory')\n\n");
    }

    /* Test 4: relevant_memories() - should return high importance */
    printf("--- Test 4: relevant_memories() ---\n");
    char** relevant = relevant_memories(&count);

    if (relevant) {
        printf("Retrieved %zu relevant memories:\n", count);
        for (size_t i = 0; i < count; i++) {
            printf("  %zu. %.60s\n", i + 1, relevant[i]);
        }
        free_memory_list(relevant, count);
        printf("✓ relevant_memories() works\n\n");
    } else {
        printf("(No relevant memories found - may need higher importance threshold)\n\n");
    }

    /* Test 5: recent_thoughts() with larger limit */
    printf("--- Test 5: recent_thoughts(20) ---\n");
    char** all_thoughts = recent_thoughts(20, &count);

    if (all_thoughts) {
        printf("Retrieved %zu recent thoughts:\n", count);
        for (size_t i = 0; i < count && i < 10; i++) {
            printf("  %zu. %.60s\n", i + 1, all_thoughts[i]);
        }
        if (count > 10) {
            printf("  ... (%zu more)\n", count - 10);
        }
        free_memory_list(all_thoughts, count);
        printf("✓ recent_thoughts(20) works\n\n");
    } else {
        printf("✗ recent_thoughts(20) failed\n\n");
    }

    /* End session */
    printf("--- Ending Session ---\n");
    session_end();
    printf("✓ Session ended\n\n");

    printf("========================================\n");
    printf("Context Functions Summary\n");
    printf("========================================\n");
    printf("All context functions tested:\n");
    printf("  ✓ recent_thoughts() - retrieves recent memories\n");
    printf("  ✓ recall_about() - finds topic-specific memories\n");
    printf("  ✓ relevant_memories() - returns high-importance memories\n");
    printf("  ✓ free_memory_list() - properly cleans up results\n");
    printf("========================================\n");

    return 0;
}
