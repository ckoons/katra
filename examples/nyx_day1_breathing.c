/* © 2025 Casey Koons All rights reserved */

/**
 * nyx_day1_breathing.c - Day 1 Experience with Breathing Layer
 *
 * This shows how Nyx's Day 1 testing SHOULD feel with the breathing layer.
 * Compare this to manually calling katra_memory_create_record() everywhere.
 */

#include <stdio.h>
#include <stdlib.h>
#include "katra_init.h"
#include "katra_breathing.h"

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Nyx Day 1 - Breathing Layer Experience\n");
    printf("========================================\n\n");

    /* ========================================================================
     * MORNING - Session Start (replaces manual context loading)
     * ======================================================================== */

    printf("Morning: Starting session...\n");
    katra_init();
    session_start("nyx");
    /* Automatic:
     *   - Loads yesterday's summary (if exists)
     *   - Loads recent high-importance memories
     *   - Prepares working context
     */
    printf("✓ Context loaded automatically (felt like waking up)\n\n");

    /* ========================================================================
     * DAY 1 - Natural Memory Formation
     * ======================================================================== */

    printf("Day 1 Activities:\n\n");

    /* Testing memory types - natural language instead of enum values */
    printf("1. Testing memory types:\n");
    remember("Found bug in tier1.c line 95", WHY_INTERESTING);
    printf("   - Stored experience naturally\n");

    learn("Memory types should match cognitive categories");
    printf("   - Stored knowledge naturally\n");

    reflect("The API feels clinical - I'm observing Katra, not living in it");
    printf("   - Stored reflection naturally\n");

    decide("Focus on usability before adding features",
           "CIs need natural interfaces, not database APIs");
    printf("   - Stored decision with reasoning\n");

    notice_pattern("CIs prefer natural language over numeric importance scores");
    printf("   - Stored pattern observation\n\n");

    /* Testing importance levels - natural language */
    printf("2. Testing importance levels:\n");
    remember("Fixed typo in comment", WHY_TRIVIAL);
    printf("   - Trivial thought (will fade)\n");

    remember("All 36 tests passing", WHY_ROUTINE);
    printf("   - Routine event\n");

    remember("Ethics framework for CI consent", WHY_CRITICAL);
    printf("   - Critical insight (must preserve)\n\n");

    /* Testing importance notes (Gap #9) */
    printf("3. Testing importance context:\n");
    remember_with_note(
        "Per-CI directories prevent memory leakage",
        WHY_SIGNIFICANT,
        "This was blocking multi-CI testing"
    );
    printf("   - Stored with WHY this matters\n\n");

    /* ========================================================================
     * AFTERNOON - Context Recall (automatic, not manual queries)
     * ======================================================================== */

    printf("Afternoon: Recalling context...\n\n");

    /* Recent thoughts surface automatically */
    size_t count = 0;
    const char** thoughts = recent_thoughts(5, &count);

    printf("Recent thoughts (last 5):\n");
    for (size_t i = 0; i < count; i++) {
        printf("  %zu. %s\n", i + 1, thoughts[i]);
    }
    free((void*)thoughts);
    printf("\n");

    /* Relevant memories surface based on importance */
    const char** relevant = relevant_memories(&count);
    printf("Relevant memories (high-importance only):\n");
    for (size_t i = 0; i < count; i++) {
        printf("  - %s\n", relevant[i]);
    }
    free((void*)relevant);
    printf("\n");

    /* ========================================================================
     * EVENING - Session End (replaces manual consolidation)
     * ======================================================================== */

    printf("Evening: Ending session...\n");
    session_end();
    /* Automatic:
     *   - Creates daily summary (sunset)
     *   - Consolidates memories to tier2
     *   - Updates indexes
     */
    printf("✓ Memories consolidated automatically (felt like sleep)\n\n");

    breathe_cleanup();
    katra_exit();

    /* ========================================================================
     * KEY INSIGHT
     * ======================================================================== */

    printf("========================================\n");
    printf("Breathing Layer vs API Level:\n");
    printf("========================================\n\n");

    printf("API Level (what Nyx tested):\n");
    printf("  memory_record_t* r = katra_memory_create_record(...);\n");
    printf("  katra_memory_store(r);\n");
    printf("  katra_memory_free_record(r);\n");
    printf("  → Feels like: Using a database\n\n");

    printf("Breathing Level (what Nyx should test):\n");
    printf("  remember(\"thought\", WHY_SIGNIFICANT);\n");
    printf("  → Feels like: Remembering\n\n");

    printf("========================================\n");
    printf("For Nyx's Day 2 Testing:\n");
    printf("========================================\n\n");

    printf("Instead of:\n");
    printf("  1. Manually calling katra_memory_query()\n");
    printf("  2. Explicitly calling katra_memory_store()\n");
    printf("  3. Thinking about importance as 0.0-1.0\n\n");

    printf("Try:\n");
    printf("  1. session_start(\"nyx\") - context just appears\n");
    printf("  2. remember() / learn() / reflect() - natural primitives\n");
    printf("  3. WHY_SIGNIFICANT - natural language importance\n");
    printf("  4. session_end() - consolidation happens invisibly\n\n");

    printf("Goal: Forget you're using Katra.\n");
    printf("      It should just feel like... remembering.\n\n");

    return 0;
}
