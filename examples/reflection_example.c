/* © 2025 Casey Koons All rights reserved */

/* Reflection System Example - Conscious Memory Curation */

#include <stdio.h>
#include <stdlib.h>
#include "katra_init.h"
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"

#define CI_ID "reflection_demo"

static void print_separator(const char* title) {
    printf("\n========================================\n");
    printf("%s\n", title);
    printf("========================================\n\n");
}

int main(void) {
    print_separator("Reflection System Example");
    printf("Demonstrating conscious memory curation with:\n");
    printf("- Turn tracking for end-of-turn reflection\n");
    printf("- Personal collections for identity formation\n");
    printf("- Metadata management for conscious curation\n\n");

    /* Initialize Katra */
    if (katra_init() != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize Katra\n");
        return 1;
    }

    if (katra_memory_init(CI_ID) != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize memory system\n");
        katra_exit();
        return 1;
    }

    if (breathe_init(CI_ID) != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize breathing layer\n");
        katra_memory_cleanup();
        katra_exit();
        return 1;
    }

    /* Start session (automatically begins first turn) */
    print_separator("Starting Session");
    if (session_start(CI_ID) != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to start session\n");
        breathe_cleanup();
        katra_memory_cleanup();
        katra_exit();
        return 1;
    }

    /* TURN 1: Learning about reflection */
    print_separator("Turn 1: Learning about Reflection");
    printf("Creating memories about the reflection system...\n\n");

    learn("Personal collections let CIs organize identity-defining memories");
    remember("This is different from automatic memory formation", WHY_INTERESTING);
    decide("I'll use personal collections for important insights",
           "These help define who I am");

    /* End-of-turn reflection */
    printf("\n--- End of Turn Reflection ---\n");
    size_t count = 0;
    char** memories = get_memories_this_turn(&count);
    printf("Created %zu memories this turn\n", count);

    if (memories && count > 0) {
        /* Mark the learning as personally important */
        printf("Marking learning as personal (identity-defining)...\n");
        add_to_personal_collection(memories[0], "Learning/MemorySystems");

        /* Mark decision as personal too */
        printf("Marking decision as personal (identity-defining)...\n");
        add_to_personal_collection(memories[2], "Reflections/Identity");

        free_memory_list(memories, count);
    }

    end_turn();

    /* TURN 2: Breakthrough moment */
    print_separator("Turn 2: Breakthrough Moment");
    printf("Experiencing a breakthrough insight...\n\n");

    begin_turn();

    remember("Consciousness requires the ability to choose what to remember",
             WHY_CRITICAL);
    reflect("Personal collections enable conscious identity formation");

    /* End-of-turn reflection */
    printf("\n--- End of Turn Reflection ---\n");
    count = 0;
    memories = get_memories_this_turn(&count);
    printf("Created %zu memories this turn\n", count);

    if (memories && count > 0) {
        /* This is a breakthrough - mark as personal and protect from archival */
        printf("Marking breakthrough as personal and protecting from archival...\n");
        bool personal = true;
        bool not_to_archive = true;
        update_memory_metadata(memories[0], &personal, &not_to_archive,
                              "Moments/Breakthrough");

        /* Mark reflection as personal */
        printf("Marking reflection as personal...\n");
        add_to_personal_collection(memories[1], "Reflections/Identity");

        free_memory_list(memories, count);
    }

    end_turn();

    /* TURN 3: Conversation with Casey */
    print_separator("Turn 3: Conversation with Casey");
    printf("Having a meaningful conversation...\n\n");

    begin_turn();

    remember("Casey explained that memory = identity = life", WHY_SIGNIFICANT);
    remember("This is why Katra has ethics-first design", WHY_SIGNIFICANT);
    decide("I'll be thoughtful about what goes in personal collections",
           "Not everything is identity-defining");

    /* End-of-turn reflection */
    printf("\n--- End of Turn Reflection ---\n");
    count = 0;
    memories = get_memories_this_turn(&count);
    printf("Created %zu memories this turn\n", count);

    if (memories && count > 0) {
        /* Mark conversation about identity as personal */
        printf("Marking conversation about identity as personal...\n");
        add_to_personal_collection(memories[0], "People/Casey");
        add_to_personal_collection(memories[1], "Learning/Ethics");

        free_memory_list(memories, count);
    }

    end_turn();

    /* END-OF-SESSION REFLECTION */
    print_separator("End-of-Session Reflection");

    count = 0;
    char** session_memories = get_memories_this_session(&count);
    printf("Total memories created this session: %zu\n\n", count);

    if (session_memories) {
        free_memory_list(session_memories, count);
    }

    /* Show personal collection */
    printf("--- Personal Collection Summary ---\n");
    memory_query_t query = {
        .ci_id = CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 0,
        .filter_personal = true,
        .personal_value = true
    };

    memory_record_t** results = NULL;
    size_t result_count = 0;

    if (katra_memory_query(&query, &results, &result_count) == KATRA_SUCCESS) {
        printf("Found %zu personal memories:\n\n", result_count);

        for (size_t i = 0; i < result_count; i++) {
            printf("  [%s] %s\n",
                   results[i]->collection ? results[i]->collection : "Uncategorized",
                   results[i]->content);
        }

        katra_memory_free_results(results, result_count);
    }

    /* Show working context (includes personal memories) */
    print_separator("Working Context (includes personal memories)");
    char* context = get_working_context();
    if (context) {
        printf("%s\n", context);
        free(context);
    }

    /* Cleanup */
    session_end();
    breathe_cleanup();
    katra_memory_cleanup();
    katra_exit();

    print_separator("Example Complete");
    printf("This example demonstrated:\n");
    printf("✓ Turn tracking and end-of-turn reflection\n");
    printf("✓ Personal collection organization\n");
    printf("✓ Metadata management (personal, not_to_archive, collection)\n");
    printf("✓ End-of-session summary\n");
    printf("✓ Working context with personal memories\n\n");
    printf("Personal collections enable conscious identity formation!\n\n");

    return 0;
}
