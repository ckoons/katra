/* © 2025 Casey Koons All rights reserved */

/**
 * level3_demo.c - Level 3 Integration Demonstration
 *
 * Shows how runtime hooks make memory invisible.
 * Simulates a CI session with automatic memory formation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "katra_init.h"
#include "katra_breathing.h"
#include "katra_error.h"

/* Simulated CI response generation */
const char* simulate_ci_thinking(int turn) {
    static const char* responses[] = {
        /* Turn 1 - significant learning */
        "I've been exploring the codebase and learned something important: "
        "the breathing layer makes memory feel natural instead of clinical. "
        "This is a significant improvement over the raw API.",

        /* Turn 2 - routine activity */
        "Let me check the test results. Looks like all tests are passing.",

        /* Turn 3 - pattern recognition */
        "I'm noticing a pattern here: CIs prefer natural language constructs "
        "over numeric values. This applies to importance levels, memory types, "
        "and probably other areas too.",

        /* Turn 4 - decision */
        "I've decided to use the Level 3 integration API for the Tekton project. "
        "The automatic capture will make memory formation invisible.",

        /* Turn 5 - critical insight */
        "This is critical: Memory should be a side-effect of thinking, "
        "not an explicit action. That's the key insight from Nyx and Theron's testing."
    };

    if (turn >= 0 && turn < 5) {
        return responses[turn];
    }
    return "Generic response with no significance markers.";
}

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Level 3: Invisible Memory Demo\n");
    printf("========================================\n\n");

    /* ========================================================================
     * MORNING - Session Start with Automatic Context
     * ======================================================================== */

    printf("=== Session Start ===\n\n");

    katra_init();
    session_start("demo_ci");

    /* Get working context automatically */
    char* context = get_working_context();
    if (context) {
        printf("Working context loaded automatically:\n");
        printf("---\n%s---\n\n", context);
        free(context);
    } else {
        printf("(No previous context - this is first session)\n\n");
    }

    /* ========================================================================
     * ACTIVE SESSION - Invisible Memory Formation
     * ======================================================================== */

    printf("=== CI Activity (5 turns) ===\n\n");

    for (int turn = 0; turn < 5; turn++) {
        printf("Turn %d:\n", turn + 1);

        /* CI generates response naturally */
        const char* response = simulate_ci_thinking(turn);
        printf("  CI: %s\n", response);

        /* INVISIBLE: Memory formation happens automatically */
        auto_capture_from_response(response);
        /* CI never called remember() - memory formed as side-effect */

        printf("\n");
    }

    /* ========================================================================
     * CHECK STATISTICS - How much was captured?
     * ======================================================================== */

    printf("=== Session Statistics ===\n\n");

    context_stats_t stats;
    if (get_context_statistics(&stats) == KATRA_SUCCESS) {
        printf("Working memory stats:\n");
        printf("  Total memories: %zu\n", stats.memory_count);
        printf("  Context size: %zu bytes\n", stats.context_bytes);
        printf("  Auto-captured this session: %zu\n", stats.session_captures);

        if (stats.last_memory_time > 0) {
            char time_str[64];
            struct tm* tm_info = localtime(&stats.last_memory_time);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            printf("  Most recent memory: %s\n", time_str);
        }
    }

    printf("\n");

    /* ========================================================================
     * EVENING - Session End with Automatic Consolidation
     * ======================================================================== */

    printf("=== Session End ===\n\n");

    session_end();
    /* Automatic: creates daily summary, consolidates, updates indexes */

    printf("Session ended. Memories consolidated automatically.\n\n");

    breathe_cleanup();
    katra_exit();

    /* ========================================================================
     * KEY DEMONSTRATION
     * ======================================================================== */

    printf("========================================\n");
    printf("Level 3 Invisibility:\n");
    printf("========================================\n\n");

    printf("What the CI experienced:\n");
    printf("  1. Session started → context just appeared\n");
    printf("  2. Generated 5 responses naturally\n");
    printf("  3. Session ended → consolidation happened\n\n");

    printf("What the CI NEVER did:\n");
    printf("  ✗ Call remember() explicitly\n");
    printf("  ✗ Query for context manually\n");
    printf("  ✗ Invoke consolidation functions\n\n");

    printf("Memory formation was:\n");
    printf("  ✓ Automatic (no explicit calls)\n");
    printf("  ✓ Unconscious (side-effect of thinking)\n");
    printf("  ✓ Invisible (like breathing)\n\n");

    printf("This is Level 3: Memory that feels natural.\n\n");

    return 0;
}
