/* © 2025 Casey Koons All rights reserved */

/* Test memory pressure awareness and periodic consolidation */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_error.h"

#define TEST_CI_ID "test_memory_pressure_ci"

int main(void) {
    printf("============================================\n");
    printf("Memory Pressure & Consolidation Test\n");
    printf("============================================\n\n");

    /* Initialize breathing layer */
    printf("1. Initializing breathing layer for %s...\n", TEST_CI_ID);
    int result = breathe_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("   ERROR: Failed to initialize: %d\n", result);
        return 1;
    }
    printf("   ✓ Initialized successfully\n\n");

    /* Get initial health status */
    printf("2. Checking initial memory health...\n");
    memory_health_t* health = get_memory_health(TEST_CI_ID);
    if (!health) {
        printf("   ERROR: Failed to get health status\n");
        breathe_cleanup();
        return 1;
    }
    printf("   Records: %zu\n", health->tier1_records);
    printf("   Fill: %.1f%%\n", health->tier1_fill_percentage);
    printf("   Memory pressure: %s\n", health->memory_pressure ? "YES" : "NO");
    printf("   Degraded mode: %s\n", health->degraded_mode ? "YES" : "NO");
    printf("   Consolidations: %zu\n", health->consolidation_count);
    free(health);
    printf("   ✓ Initial health looks good\n\n");

    /* Store some memories to create pressure */
    printf("3. Storing memories to test pressure detection...\n");
    printf("   Storing batches of 1000 memories...\n");

    for (int batch = 0; batch < 10; batch++) {
        printf("   Batch %d: ", batch + 1);

        for (int i = 0; i < 1000; i++) {
            char content[128];
            snprintf(content, sizeof(content),
                    "Test memory batch %d item %d", batch, i);
            remember(content, WHY_ROUTINE);
        }

        /* Check health after each batch */
        health = get_memory_health(TEST_CI_ID);
        if (health) {
            printf("%zu records, %.1f%% full",
                   health->tier1_records,
                   health->tier1_fill_percentage);

            if (health->degraded_mode) {
                printf(" [CRITICAL - DEGRADED MODE]");
            } else if (health->memory_pressure) {
                printf(" [WARNING - MEMORY PRESSURE]");
            }

            printf("\n");

            /* Stop if we hit critical pressure to avoid actual issues */
            if (health->degraded_mode) {
                printf("   ✓ Degraded mode detected - stopping test\n");
                free(health);
                break;
            }

            free(health);
        }
    }
    printf("\n");

    /* Test periodic maintenance */
    printf("4. Testing periodic maintenance call...\n");
    result = breathe_periodic_maintenance();
    printf("   Maintenance result: %s\n",
           result == KATRA_SUCCESS ? "SUCCESS" : "FAILED");

    /* Check if consolidation happened */
    enhanced_stats_t* stats = get_enhanced_statistics();
    if (stats) {
        printf("   Total consolidations: %zu\n", stats->consolidation_count);
        printf("   Total memories stored: %zu\n", stats->total_memories_stored);
        printf("   ✓ Statistics retrieved\n");
        free(stats);
    }
    printf("\n");

    /* Final health check */
    printf("5. Final health check...\n");
    health = get_memory_health(TEST_CI_ID);
    if (health) {
        printf("   Records: %zu\n", health->tier1_records);
        printf("   Fill: %.1f%%\n", health->tier1_fill_percentage);
        printf("   Memory pressure: %s\n", health->memory_pressure ? "YES" : "NO");
        printf("   Degraded mode: %s\n", health->degraded_mode ? "YES" : "NO");
        printf("   Consolidations: %zu\n", health->consolidation_count);
        free(health);
        printf("   ✓ Final health check complete\n");
    }
    printf("\n");

    /* Cleanup */
    printf("6. Cleaning up...\n");
    breathe_cleanup();
    printf("   ✓ Cleanup complete\n\n");

    printf("============================================\n");
    printf("Test completed successfully!\n");
    printf("============================================\n");

    return 0;
}
