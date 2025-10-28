/* © 2025 Casey Koons All rights reserved */

/**
 * test_semantic.c - Test semantic importance parsing
 *
 * Verifies that semantic phrases correctly map to importance levels.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "katra_breathing.h"
#include "katra_error.h"
#include "katra_memory.h"

#define TEST_CI_ID "semantic_test_ci"

void test_semantic(const char* thought, const char* semantic_why, const char* expected_level) {
    int result = remember_semantic(thought, semantic_why);

    /* Query back all memories to find the one we just stored */
    memory_query_t query = {
        .ci_id = TEST_CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.0f,
        .tier = KATRA_TIER1,
        .limit = 100  /* Get all to search for our memory */
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    katra_memory_query(&query, &results, &count);

    /* Find the memory with matching content */
    float found_importance = -1.0f;
    for (size_t i = 0; i < count; i++) {
        if (results[i] && results[i]->content &&
            strcmp(results[i]->content, thought) == 0) {
            found_importance = results[i]->importance;
            break;
        }
    }

    if (found_importance >= 0.0f) {
        printf("  Thought: '%s'\n", thought);
        printf("  Semantic: '%s'\n", semantic_why);
        printf("    → Expected: %s\n", expected_level);
        printf("    → Got importance: %.2f\n", found_importance);
        printf("    %s\n\n", result == KATRA_SUCCESS ? "✓" : "✗");
    } else {
        printf("  ERROR: Could not find memory for: '%s'\n\n", thought);
    }

    if (results) {
        katra_memory_free_results(results, count);
    }
}

int main(void) {
    printf("========================================\n");
    printf("Semantic Importance Parsing Test\n");
    printf("========================================\n\n");

    session_start(TEST_CI_ID);

    printf("--- Testing Semantic Phrases ---\n\n");

    test_semantic("Information for the system", "critical", "CRITICAL (1.0)");
    test_semantic("Just a routine update", "not important", "TRIVIAL (0.25)");
    test_semantic("Need to remember this", "very important", "HIGH (0.75)");
    test_semantic("Fact about the codebase", "interesting", "MEDIUM (0.50)");
    test_semantic("Daily update", "routine", "LOW (0.25)");
    test_semantic("Breakthrough in testing", "significant", "HIGH (0.75)");
    test_semantic("Insight about memory", "essential", "HIGH (0.75)");
    test_semantic("Observation", "trivial", "TRIVIAL (0.25)");

    session_end();

    printf("========================================\n");
    printf("Semantic parsing test complete\n");
    printf("========================================\n");

    return 0;
}
