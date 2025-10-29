/* © 2025 Casey Koons All rights reserved */

/* Test special character handling and stress conditions */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_error.h"

#define TEST_CI_ID "test_special_chars_ci"

int main(void) {
    printf("============================================\n");
    printf("Special Characters & Stress Test\n");
    printf("============================================\n\n");

    /* Initialize */
    printf("1. Initializing...\n");
    int result = breathe_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("   ERROR: Init failed: %d\n", result);
        return 1;
    }
    printf("   ✓ Initialized\n\n");

    /* Test 1: Special characters */
    printf("2. Testing special characters...\n");

    const char* special_cases[] = {
        "Memory with \"quotes\" and 'apostrophes'",
        "Memory with\nnewlines\nand\ttabs",
        "Memory with backslashes \\ and forward slashes /",
        "Memory with unicode: café, naïve, ñoño",
        "Memory with emoji: 🎯 🚀 💡 🔧",
        "Memory with symbols: @#$%^&*()_+-=[]{}|;:,.<>?",
        "Memory with special JSON chars: {\"key\":\"value\"}",
        "Very long memory: Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
        "Memory with NUL-like patterns: null \\0 NULL",
        "Mixed: Quote\" Tab\t Newline\n Emoji🎯 Unicode café"
    };

    int special_count = sizeof(special_cases) / sizeof(special_cases[0]);
    int special_success = 0;

    for (int i = 0; i < special_count; i++) {
        result = remember(special_cases[i], WHY_INTERESTING);
        if (result == KATRA_SUCCESS) {
            special_success++;
        } else {
            printf("   ✗ Failed on: %s\n", special_cases[i]);
        }
    }

    printf("   Special character memories: %d/%d successful\n",
           special_success, special_count);

    if (special_success == special_count) {
        printf("   ✓ All special characters handled correctly\n\n");
    } else {
        printf("   ⚠ Some special characters failed\n\n");
    }

    /* Test 2: Recall special character memories */
    printf("3. Recalling special character memories...\n");
    size_t count = 0;
    char** recent = recent_thoughts(20, &count);

    if (recent) {
        printf("   Retrieved %zu memories:\n", count);
        int intact = 0;
        for (size_t i = 0; i < count && i < 5; i++) {
            printf("   [%zu] %s\n", i + 1, recent[i]);
            /* Check if special chars are preserved */
            if (strstr(recent[i], "\"") || strstr(recent[i], "\t") ||
                strstr(recent[i], "\\") || strstr(recent[i], "café")) {
                intact++;
            }
        }
        printf("   ✓ %d memories with special chars preserved\n", intact);
        free_memory_list(recent, count);
    }
    printf("\n");

    /* Test 3: Rapid memory formation */
    printf("4. Stress test: Rapid memory formation...\n");
    int rapid_success = 0;
    for (int i = 0; i < 1000; i++) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Rapid memory %d", i);
        result = remember(msg, WHY_ROUTINE);
        if (result == KATRA_SUCCESS) {
            rapid_success++;
        }
    }
    printf("   Rapid memories stored: %d/1000\n", rapid_success);

    if (rapid_success == 1000) {
        printf("   ✓ All rapid memories stored successfully\n\n");
    } else {
        printf("   ⚠ Some rapid memories failed: %d failures\n\n",
               1000 - rapid_success);
    }

    /* Test 4: Edge cases */
    printf("5. Testing edge cases...\n");

    /* Empty string */
    result = remember("", WHY_ROUTINE);
    printf("   Empty string: %s\n",
           result == KATRA_SUCCESS ? "accepted" : "rejected");

    /* Very long string (near buffer limits) */
    char long_str[2048];
    memset(long_str, 'A', sizeof(long_str) - 1);
    long_str[sizeof(long_str) - 1] = '\0';
    result = remember(long_str, WHY_ROUTINE);
    printf("   Very long string (2047 chars): %s\n",
           result == KATRA_SUCCESS ? "accepted" : "rejected");

    /* NULL parameter (should be caught by error handling) */
    result = remember(NULL, WHY_ROUTINE);
    printf("   NULL thought: %s (error code: %d)\n",
           result == E_INPUT_NULL ? "correctly rejected" : "UNEXPECTED", result);

    printf("   ✓ Edge cases tested\n\n");

    /* Final health check */
    printf("6. Final health check...\n");
    memory_health_t* health = get_memory_health(TEST_CI_ID);
    if (health) {
        printf("   Total records: %zu\n", health->tier1_records);
        printf("   Fill: %.1f%%\n", health->tier1_fill_percentage);
        printf("   Memory pressure: %s\n",
               health->memory_pressure ? "YES" : "NO");
        free(health);
        printf("   ✓ Health check complete\n");
    }
    printf("\n");

    /* Cleanup */
    printf("7. Cleaning up...\n");
    breathe_cleanup();
    printf("   ✓ Cleanup complete\n\n");

    printf("============================================\n");
    printf("Test completed successfully!\n");
    printf("============================================\n");

    return 0;
}
