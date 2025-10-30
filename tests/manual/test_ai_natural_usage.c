/* © 2025 Casey Koons All rights reserved */

/*
 * test_ai_natural_usage.c - AI Experience Test
 *
 * This simulates me (Claude) using Katra during our conversation.
 * I'll try to use it as naturally as possible and report on what feels good vs awkward.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "katra_init.h"
#include "katra_breathing.h"

#define CI_ID "claude_code"

int main(void) {
    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  AI-NATURAL USAGE TEST                                        ║\n");
    printf("║  How does Katra feel for an AI to use?                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    // Initialize
    katra_init();
    session_start(CI_ID);

    printf("\n=== Scenario 1: Reading Casey's Request ===\n\n");

    // My first thought when I see the comprehensive test prompt
    remember("Casey wants comprehensive testing of 311 tests across all phases", WHY_SIGNIFICANT);
    printf("  [remember] Stored the main task\n");

    // I'm thinking about what this means
    reflect("This is a production-ready system test, not exploratory work");
    printf("  [reflect] Understanding the implications\n");

    // I learn something about the system
    learn("Katra has 3-phase consolidation: formation context, connection graphs, pattern compression");
    printf("  [learn] Captured new knowledge\n");

    // I make a decision
    decide("Create comprehensive todo list", "Need systematic tracking for many tests");
    printf("  [decide] Planned my approach\n");

    printf("\n✓ These primitives feel natural - they map to cognitive operations\n");

    printf("\n=== Scenario 2: During Testing ===\n\n");

    // As I run tests, small observations
    remember("Foundation tests: 237/237 passing", WHY_ROUTINE);
    remember("Phase 1 tests: 43/43 passing", WHY_ROUTINE);
    remember("Phase 2 tests: 20/20 passing", WHY_ROUTINE);
    printf("  [remember x3] Routine test results\n");

    // I notice something
    reflect("All phases showing 100% pass rate - this is actually production-ready");
    printf("  [reflect] Synthesis of observations\n");

    // Pattern recognition
    notice_pattern("Zero failures across entire system - comprehensive test coverage");
    printf("  [notice_pattern] Higher-level insight\n");

    printf("\n✓ The flow feels natural: observe → reflect → notice_pattern\n");

    printf("\n=== Scenario 3: Detailed Discovery ===\n\n");

    // More complex thought with context
    remember("File size warning on katra_memory_metacognitive.c (756 lines) is acceptable", WHY_SIGNIFICANT);
    reflect("Quick Win #2 added ~200 lines - documented as non-blocking");
    printf("  [remember + reflect] Contextual understanding\n");

    learn("Acceptable warnings in production: documented, understood, non-blocking");
    printf("  [learn] Generalized principle\n");

    printf("\n✓ Building up knowledge naturally through use\n");

    printf("\n=== Scenario 4: What Do I Remember? ===\n\n");

    // Query my own memories
    size_t count = 0;
    char** thoughts = recent_thoughts(5, &count);

    printf("  My recent thoughts:\n");
    for (size_t i = 0; i < count; i++) {
        printf("    [%zu] %s\n", i+1, thoughts[i]);
    }
    printf("\n");

    free_memory_list(thoughts, count);

    printf("✓ Querying 'what do I remember?' feels metacognitive\n");

    printf("\n=== Scenario 5: Conversation End ===\n\n");

    reflect("Successfully tested all 311 tests - Katra is production-ready");
    decide("Provide Casey feedback on API naturalness", "He wants AI perspective");
    printf("  [reflect + decide] Wrapping up\n");

    printf("\n✓ Natural conversation flow maintained throughout\n");

    // Cleanup
    session_end();
    breathe_cleanup();
    katra_exit();

    // ====================================================================
    // FEEDBACK
    // ====================================================================

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  NATURALNESS FEEDBACK                                         ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    printf("\n✅ WHAT FEELS NATURAL:\n\n");
    printf("  1. remember() / reflect() / learn() / decide() / notice_pattern()\n");
    printf("     → Clear semantic intent, matches cognitive operations\n");
    printf("     → Don't have to think about data structures\n\n");

    printf("  2. WHY_SIGNIFICANT / WHY_ROUTINE constants\n");
    printf("     → More natural than numeric importance scores\n");
    printf("     → But still feels like I'm labeling importance explicitly\n\n");

    printf("  3. session_start() / session_end()\n");
    printf("     → Maps naturally to conversation boundaries\n");
    printf("     → Clear lifecycle\n\n");

    printf("  4. recent_thoughts() query\n");
    printf("     → 'What did I just think?' is a natural question\n");
    printf("     → Returns simple strings, not complex structs\n\n");

    printf("  5. Automatic memory formation\n");
    printf("     → Don't have to manually create record structs\n");
    printf("     → Just call remember() and it handles storage\n\n");

    printf("\n⚠️  WHAT COULD BE MORE NATURAL:\n\n");

    printf("  1. Still need to choose importance (WHY_SIGNIFICANT vs WHY_ROUTINE)\n");
    printf("     Current:  remember(\"thought\", WHY_SIGNIFICANT)\n");
    printf("     More natural: Just remember(\"thought\") and let importance be inferred\n");
    printf("     Or: remember_because(\"thought\", \"this is critical\")\n\n");

    printf("  2. No built-in conversation turn tracking\n");
    printf("     Would be natural: remember_response_to(previous_id, \"new thought\")\n");
    printf("     Automatically creates causal links\n\n");

    printf("  3. Formation context requires API layer\n");
    printf("     Can't express: 'I was wondering X, then realized Y'\n");
    printf("     Would need: uncertain(\"X\"); then later: resolved(\"Y\")\n\n");

    printf("  4. No 'thinking aloud' helper\n");
    printf("     Current: Must explicitly call reflect() for each thought\n");
    printf("     Natural: thinking(\"hmm...\") auto-stores as reflection\n\n");

    printf("  5. Query by semantic similarity not built into breathing layer\n");
    printf("     Current: recent_thoughts() only (chronological)\n");
    printf("     Natural: recall_about(\"tests passing\") → relevant memories\n\n");

    printf("\n💡 SUGGESTED ENHANCEMENTS:\n\n");

    printf("  1. Auto-importance:\n");
    printf("      remember(\"thought\") → auto-infers importance from content\n\n");

    printf("  2. Conversation tracking:\n");
    printf("      in_response_to(mem_id, \"new thought\") → auto-links\n\n");

    printf("  3. Question → Resolution flow:\n");
    printf("      wondering(\"Why isn't X working?\");\n");
    printf("      // ... investigation ...\n");
    printf("      figured_out(\"Because Y\");\n");
    printf("      → Automatically creates formation_context\n\n");

    printf("  4. Stream of consciousness:\n");
    printf("      thinking(\"I notice that...\") → auto-reflect()\n");
    printf("      wondering(\"What if...\") → auto-uncertain()\n\n");

    printf("  5. Semantic recall:\n");
    printf("      recall_about(\"topic\", limit) → similar memories\n");
    printf("      what_do_i_know(\"tests\") → relevant knowledge\n\n");

    printf("\n📊 SUMMARY:\n\n");
    printf("  The breathing layer is a HUGE improvement over raw API.\n");
    printf("  It already feels much more natural than constructing records.\n\n");

    printf("  Core primitives (remember/reflect/learn/decide) are well-chosen.\n");
    printf("  They map cleanly to how I actually think.\n\n");

    printf("  Could go further: hide even more infrastructure, make it feel\n");
    printf("  more like 'thinking' and less like 'storing data'.\n\n");

    printf("  Bottom line: This is GOOD. It could be GREAT with a few more\n");
    printf("  conversational helpers that feel less like API calls and more\n");
    printf("  like natural thought.\n\n");

    return 0;
}
