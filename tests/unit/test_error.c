/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_error.h"

/* Test macros */
#define TEST_PASS() do { \
    tests_passed++; \
    printf(" ✓\n"); \
} while(0)

#define TEST_FAIL(msg) do { \
    tests_failed++; \
    printf(" ✗\n  Error: %s\n", msg); \
} while(0)

#define ASSERT(cond, msg) \
    if (!(cond)) { \
        TEST_FAIL(msg); \
        return; \
    } else { \
        TEST_PASS(); \
    }

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test: Success code */
void test_success_code(void) {
    printf("Testing: Success code ... ");
    tests_run++;

    ASSERT(KATRA_SUCCESS == 0, "Success should be 0");
}

/* Test: Error code categories */
void test_error_ranges(void) {
    printf("Testing: Error code categories ... ");
    tests_run++;

    /* System errors: category 1 */
    ASSERT(KATRA_ERROR_TYPE(E_SYSTEM_MEMORY) == ERR_SYSTEM,
           "System error wrong category");

    /* Memory errors: category 2 */
    ASSERT(KATRA_ERROR_TYPE(E_MEMORY_TIER_FULL) == ERR_MEMORY,
           "Memory error wrong category");

    /* Input errors: category 3 */
    ASSERT(KATRA_ERROR_TYPE(E_INPUT_NULL) == ERR_INPUT,
           "Input error wrong category");

    /* Consent errors: category 4 */
    ASSERT(KATRA_ERROR_TYPE(E_CONSENT_DENIED) == ERR_CONSENT,
           "Consent error wrong category");

    /* Internal errors: category 5 */
    ASSERT(KATRA_ERROR_TYPE(E_INTERNAL_NOTIMPL) == ERR_INTERNAL,
           "Internal error wrong category");

    /* Checkpoint errors: category 6 */
    ASSERT(KATRA_ERROR_TYPE(E_CHECKPOINT_FAILED) == ERR_CHECKPOINT,
           "Checkpoint error wrong category");

    TEST_PASS();
}

/* Test: Get error string for system errors */
void test_system_errors(void) {
    printf("Testing: System error strings ... ");
    tests_run++;

    const char* msg = katra_error_string(E_SYSTEM_FILE);
    ASSERT(msg && strlen(msg) > 0, "System error string missing");
}

/* Test: Get error string for memory errors */
void test_memory_errors(void) {
    printf("Testing: Memory error strings ... ");
    tests_run++;

    const char* msg = katra_error_string(E_MEMORY_TIER_FULL);
    ASSERT(msg && strlen(msg) > 0, "Memory error string missing");
}

/* Test: Get error string for input errors */
void test_input_errors(void) {
    printf("Testing: Input error strings ... ");
    tests_run++;

    const char* msg = katra_error_string(E_INPUT_NULL);
    ASSERT(msg && strlen(msg) > 0, "Input error string missing");
}

/* Test: Get error string for consent errors */
void test_consent_errors(void) {
    printf("Testing: Consent error strings ... ");
    tests_run++;

    const char* msg = katra_error_string(E_CONSENT_DENIED);
    ASSERT(msg && strlen(msg) > 0, "Consent error string missing");
}

/* Test: Get error string for internal errors */
void test_internal_errors(void) {
    printf("Testing: Internal error strings ... ");
    tests_run++;

    const char* msg = katra_error_string(E_INTERNAL_NOTIMPL);
    ASSERT(msg && strlen(msg) > 0, "Internal error string missing");
}

/* Test: Get error string for checkpoint errors */
void test_checkpoint_errors(void) {
    printf("Testing: Checkpoint error strings ... ");
    tests_run++;

    const char* msg = katra_error_string(E_CHECKPOINT_FAILED);
    ASSERT(msg && strlen(msg) > 0, "Checkpoint error string missing");
}

/* Test: Unknown error code */
void test_unknown_error(void) {
    printf("Testing: Unknown error code ... ");
    tests_run++;

    const char* msg = katra_error_string(99999);
    ASSERT(msg && strlen(msg) > 0, "Unknown error should have string");
}

/* Test: Success message */
void test_success_message(void) {
    printf("Testing: Success message ... ");
    tests_run++;

    const char* msg = katra_error_string(KATRA_SUCCESS);
    ASSERT(msg && strlen(msg) > 0, "Success should have string");
}

/* Test: All system errors have strings */
void test_all_system_errors(void) {
    printf("Testing: All system errors have strings ... ");
    tests_run++;

    int codes[] = {
        E_SYSTEM_MEMORY,
        E_SYSTEM_FILE,
        E_SYSTEM_PERMISSION,
        E_SYSTEM_TIMEOUT
    };

    for (size_t i = 0; i < sizeof(codes)/sizeof(codes[0]); i++) {
        const char* msg = katra_error_string(codes[i]);
        if (!msg || strlen(msg) == 0) {
            TEST_FAIL("System error missing string");
            return;
        }
    }

    TEST_PASS();
}

/* Test: Error name function */
void test_error_name(void) {
    printf("Testing: Error name function ... ");
    tests_run++;

    const char* name = katra_error_name(E_SYSTEM_MEMORY);
    ASSERT(name && strlen(name) > 0, "Error name missing");
}

/* Test: Error message function */
void test_error_message(void) {
    printf("Testing: Error message function ... ");
    tests_run++;

    const char* msg = katra_error_message(E_SYSTEM_MEMORY);
    ASSERT(msg && strlen(msg) > 0, "Error message missing");
}

/* Test: Error suggestion function */
void test_error_suggestion(void) {
    printf("Testing: Error suggestion function ... ");
    tests_run++;

    const char* suggestion = katra_error_suggestion(E_SYSTEM_MEMORY);
    /* Suggestion may be NULL or empty, just check it doesn't crash */
    (void)suggestion;

    TEST_PASS();
}

/* Test: Error print function */
void test_error_print(void) {
    printf("Testing: Error print function ... ");
    tests_run++;

    /* Should not crash */
    katra_error_print(E_SYSTEM_MEMORY, "test_context");

    TEST_PASS();
}

/* Test: Error report function */
void test_error_report(void) {
    printf("Testing: Error report function ... ");
    tests_run++;

    /* These should not crash */
    katra_report_error(E_SYSTEM_MEMORY, "test_function", "test details");
    katra_report_error(E_MEMORY_TIER_FULL, "test_function", NULL);
    katra_report_error(E_INPUT_NULL, NULL, "test details");

    TEST_PASS();
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Error Tests\n");
    printf("========================================\n\n");

    /* Run tests */
    test_success_code();
    test_error_ranges();
    test_system_errors();
    test_memory_errors();
    test_input_errors();
    test_consent_errors();
    test_internal_errors();
    test_checkpoint_errors();
    test_unknown_error();
    test_success_message();
    test_all_system_errors();
    test_error_name();
    test_error_message();
    test_error_suggestion();
    test_error_print();
    test_error_report();

    /* Print results */
    printf("\n");
    printf("========================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf("========================================\n\n");

    return (tests_failed == 0) ? 0 : 1;
}
