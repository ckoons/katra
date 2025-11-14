/* © 2025 Casey Koons All rights reserved */

/* CLI tool to regenerate all semantic search vectors */

#include <stdio.h>
#include <stdlib.h>
#include "katra_breathing.h"
#include "katra_error.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ci_id>\n", argv[0]);
        fprintf(stderr, "Example: %s Casey\n", argv[0]);
        return 1;
    }

    const char* ci_id = argv[1];

    printf("\n=== Regenerating Vectors for %s ===\n\n", ci_id);

    /* Initialize breathing layer */
    int result = session_start(ci_id);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "ERROR: Could not start session: %d\n", result);
        return 1;
    }

    /* Regenerate all vectors */
    int count = regenerate_vectors();
    if (count < 0) {
        fprintf(stderr, "\nERROR: Vector regeneration failed with error code: %d\n\n", count);
        session_end();
        return 1;
    }

    printf("\n✓ Successfully regenerated %d vectors\n\n", count);

    /* Cleanup */
    session_end();

    return 0;
}
