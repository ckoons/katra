/* © 2025 Casey Koons All rights reserved */

/**
 * test_string_to_importance.c - Debug test for semantic parsing
 *
 * Directly tests string_to_importance() function
 */

#include <stdio.h>
#include "katra_breathing.h"

int main(void) {
    printf("Testing string_to_importance() directly:\n\n");

    const char* tests[] = {
        "critical",
        "not important",
        "very important",
        "interesting",
        "routine",
        "significant",
        "essential",
        "trivial",
        NULL
    };

    for (int i = 0; tests[i] != NULL; i++) {
        float importance = string_to_importance(tests[i]);
        printf("  '%s' → %.2f\n", tests[i], importance);
    }

    return 0;
}
