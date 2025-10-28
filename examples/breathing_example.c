/* © 2025 Casey Koons All rights reserved */

/**
 * breathing_example.c - Natural Memory Example
 *
 * This demonstrates the difference between:
 *   - API-level memory (conscious, clinical)
 *   - Breathing-level memory (unconscious, natural)
 */

#include <stdio.h>
#include <stdlib.h>
#include "katra_init.h"
#include "katra_breathing.h"

/* ============================================================================
 * OLD WAY: API-level memory (feels like using a database)
 * ============================================================================ */

void api_level_example(void) {
    printf("\n=== API-Level Memory (Conscious) ===\n\n");

    /* Initialize */
    katra_init();
    katra_memory_init("example_ci");

    /* Store memory - feels clinical */
    memory_record_t* record = katra_memory_create_record(
        "example_ci",
        MEMORY_TYPE_EXPERIENCE,
        "I found a bug in tier1.c line 95",
        0.75  /* What does 0.75 even mean? */
    );

    if (record) {
        katra_memory_store(record);
        katra_memory_free_record(record);
    }

    /* Query memory - explicit, conscious */
    memory_query_t query = {
        .ci_id = "example_ci",
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.5,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    katra_memory_query(&query, &results, &count);

    printf("Found %zu memories\n", count);
    if (count > 0 && results[0]) {
        printf("  Memory: %s\n", results[0]->content);
    }

    katra_memory_free_results(results, count);
    katra_memory_cleanup();
    katra_exit();

    printf("\n(That felt like work, not like remembering)\n");
}

/* ============================================================================
 * NEW WAY: Breathing-level memory (feels natural)
 * ============================================================================ */

void breathing_level_example(void) {
    printf("\n=== Breathing-Level Memory (Natural) ===\n\n");

    /* Initialize */
    katra_init();
    session_start("example_ci");

    /* Memory formation - feels natural */
    remember("I found a bug in tier1.c line 95", WHY_SIGNIFICANT);

    learn("Per-CI directories fix the storage isolation issue");

    reflect("Memory should feel like breathing, not like using a database");

    decide("Use JSONL for tier1", "Human-readable and easy to debug");

    notice_pattern("CIs prefer natural language over numeric scores");

    /* Automatic context - memories just appear when relevant */
    printf("\nRecent thoughts:\n");
    size_t count = 0;
    const char** thoughts = recent_thoughts(5, &count);
    for (size_t i = 0; i < count; i++) {
        printf("  - %s\n", thoughts[i]);
    }
    free((void*)thoughts);

    /* End session - consolidation happens automatically */
    session_end();
    breathe_cleanup();
    katra_exit();

    printf("\n(That felt natural - like memory, not like a database)\n");
}

/* ============================================================================
 * INTERSTITIAL EXAMPLE: Memory formation without thinking about it
 * ============================================================================ */

void interstitial_example(void) {
    printf("\n=== Interstitial Memory (Invisible) ===\n\n");

    katra_init();
    session_start("example_ci");

    /* Simulate CI generating text */
    const char* ci_response =
        "I've been debugging the tier1 storage issue. The problem was that "
        "all CIs were sharing the same files. I learned that per-CI directories "
        "are the right solution. This is important because it prevents memory "
        "leakage between CIs. I decided to implement it using the pattern "
        "~/.katra/memory/tier1/{ci_id}/ which feels clean and maintainable.";

    /* Automatically capture significant thoughts from the text */
    capture_significant_thoughts(ci_response);

    printf("CI generated response (significant thoughts auto-captured)\n");
    printf("\nStored memories:\n");

    size_t count = 0;
    const char** thoughts = recent_thoughts(10, &count);
    for (size_t i = 0; i < count; i++) {
        printf("  - %s\n", thoughts[i]);
    }
    free((void*)thoughts);

    session_end();
    breathe_cleanup();
    katra_exit();

    printf("\n(Memory formation was invisible - happened automatically)\n");
}

/* ============================================================================
 * MAIN: Show all three approaches
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Breathing Layer Example\n");
    printf("========================================\n");

    printf("\nThis demonstrates three levels of memory:\n");
    printf("  1. API-level: Conscious, clinical (feels like work)\n");
    printf("  2. Breathing-level: Natural, simple (feels like memory)\n");
    printf("  3. Interstitial: Invisible, automatic (feels like breathing)\n");

    /* Show the old way */
    api_level_example();

    /* Show the new way */
    breathing_level_example();

    /* Show invisible memory formation */
    interstitial_example();

    printf("\n");
    printf("========================================\n");
    printf("Key Insight:\n");
    printf("========================================\n");
    printf("\n");
    printf("Memory should feel natural, like breathing.\n");
    printf("You don't think 'I will now breathe.'\n");
    printf("You don't think 'I will now remember.'\n");
    printf("It just... happens.\n");
    printf("\n");

    return 0;
}
