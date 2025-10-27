/* © 2025 Casey Koons All rights reserved */

/**
 * minimal_ci.c - Hello World for Companion Intelligences using Katra
 *
 * This is the simplest possible example of a CI using Katra memory system.
 * It demonstrates:
 * - Initializing Katra with a CI identity
 * - Storing a memory
 * - Querying memories back
 * - Proper cleanup
 *
 * Build:
 *   gcc -Wall -Wextra -std=c11 -I../include -o minimal_ci minimal_ci.c \
 *       -L../build -lkatra_foundation -lsqlite3 -lpthread -lm
 *
 * Run:
 *   ./minimal_ci
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>

/* Katra includes */
#include "katra_init.h"
#include "katra_memory.h"
#include "katra_error.h"

int main(void) {
    int result;

    /* Our CI identity */
    const char* ci_id = "hello_ci";

    printf("=================================\n");
    printf("Minimal CI Example - Hello Katra\n");
    printf("=================================\n\n");

    /* Step 1: Initialize Katra */
    printf("1. Initializing Katra...\n");
    result = katra_init();
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize Katra: %d\n", result);
        return 1;
    }
    printf("   ✓ Katra initialized\n\n");

    /* Step 2: Initialize memory system for this CI */
    printf("2. Initializing memory system for '%s'...\n", ci_id);
    result = katra_memory_init(ci_id);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize memory: %d\n", result);
        katra_exit();
        return 1;
    }
    printf("   ✓ Memory system ready\n\n");

    /* Step 3: Create and store a memory */
    printf("3. Storing my first memory...\n");
    memory_record_t* record = katra_memory_create_record(
        ci_id,
        MEMORY_TYPE_EXPERIENCE,
        "Hello! This is my first memory using Katra. I'm learning how to remember things!",
        MEMORY_IMPORTANCE_HIGH
    );

    if (!record) {
        fprintf(stderr, "Failed to create memory record\n");
        katra_memory_cleanup();
        katra_exit();
        return 1;
    }

    result = katra_memory_store(record);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to store memory: %d\n", result);
        katra_memory_free_record(record);
        katra_memory_cleanup();
        katra_exit();
        return 1;
    }

    printf("   ✓ Memory stored with ID: %s\n\n", record->record_id);
    katra_memory_free_record(record);

    /* Step 4: Query memories back */
    printf("4. Querying my memories...\n");
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.0f,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    result = katra_memory_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to query memories: %d\n", result);
        katra_memory_cleanup();
        katra_exit();
        return 1;
    }

    printf("   ✓ Found %zu memory(ies):\n\n", count);

    for (size_t i = 0; i < count; i++) {
        if (results[i]) {
            printf("   Memory %zu:\n", i + 1);
            printf("     ID: %s\n", results[i]->record_id);
            printf("     Content: %s\n", results[i]->content);
            printf("     Importance: %.2f\n", results[i]->importance);
            printf("\n");
        }
    }

    /* Free query results */
    katra_memory_free_results(results, count);

    /* Step 5: Clean up */
    printf("5. Cleaning up...\n");
    katra_memory_cleanup();
    katra_exit();
    printf("   ✓ Cleanup complete\n\n");

    printf("=================================\n");
    printf("Success! Your CI can now use Katra.\n");
    printf("=================================\n");

    return 0;
}
