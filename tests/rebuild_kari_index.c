/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>

/* Project includes */
#include "katra_tier1_index.h"
#include "katra_memory.h"
#include "katra_error.h"

int main(void) {
    printf("Rebuilding FTS index for Kari...\n");

    /* Initialize memory system first */
    int result = katra_memory_init("Kari");
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize memory system: %d\n", result);
        return 1;
    }

    /* Rebuild index */
    result = tier1_index_rebuild("Kari");
    if (result < 0) {
        fprintf(stderr, "Failed to rebuild index: %d\n", result);
        return 1;
    }

    printf("Successfully indexed %d memories for Kari\n", result);

    /* Get stats */
    size_t memory_count = 0;
    size_t theme_count = 0;
    size_t connection_count = 0;
    result = tier1_index_stats("Kari", &memory_count, &theme_count, &connection_count);
    if (result == KATRA_SUCCESS) {
        printf("Index statistics:\n");
        printf("  Memories: %zu\n", memory_count);
        printf("  Themes: %zu\n", theme_count);
        printf("  Connections: %zu\n", connection_count);
    }

    tier1_index_cleanup();
    return 0;
}
