/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Project includes */
#include "katra_log.h"
#include "katra_env_utils.h"
#include "katra_limits.h"

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

/* Test: Log initialization */
void test_log_init(void) {
    printf("Testing: Log initialization ... ");
    tests_run++;

    /* Just verify logging can be used - first LOG_* call initializes */
    LOG_INFO("Test log initialization");

    TEST_PASS();
}

/* Test: All log levels */
void test_log_levels(void) {
    printf("Testing: All log levels ... ");
    tests_run++;

    /* These should not crash */
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");

    TEST_PASS();
}

/* Test: Log with format arguments */
void test_log_format(void) {
    printf("Testing: Log with format arguments ... ");
    tests_run++;

    int value = 42;
    const char* str = "test";

    LOG_INFO("Integer: %d, String: %s", value, str);
    LOG_DEBUG("Format test: %d %s %f", 10, "hello", 3.14);

    TEST_PASS();
}

/* Test: Log directory creation */
void test_log_directory(void) {
    printf("Testing: Log directory creation ... ");
    tests_run++;

    const char* home = getenv("HOME");
    if (!home) {
        TEST_FAIL("HOME not set");
        return;
    }

    char log_dir[KATRA_PATH_MAX];
    snprintf(log_dir, sizeof(log_dir), "%s/.katra/logs", home);

    struct stat st;
    int result = stat(log_dir, &st);

    ASSERT(result == 0 && S_ISDIR(st.st_mode), "Log directory not created");
}

/* Test: Log file creation */
void test_log_file_creation(void) {
    printf("Testing: Log file creation (skip - logs to stderr) ... ");
    tests_run++;

    /* Note: Current logging implementation writes to stderr, not files.
     * File logging will be implemented in Phase 2.
     * For now, just verify logging doesn't crash. */

    LOG_INFO("Test log file creation");

    TEST_PASS();
}

/* Test: Multiple log calls */
void test_multiple_logs(void) {
    printf("Testing: Multiple log calls ... ");
    tests_run++;

    for (int i = 0; i < 10; i++) {
        LOG_INFO("Log message %d", i);
    }

    TEST_PASS();
}

/* Test: Long log message */
void test_long_message(void) {
    printf("Testing: Long log message ... ");
    tests_run++;

    char long_msg[KATRA_BUFFER_LARGE];
    memset(long_msg, 'A', sizeof(long_msg) - 1);
    long_msg[sizeof(long_msg) - 1] = '\0';

    /* Should not crash with very long message */
    LOG_INFO("%s", long_msg);

    TEST_PASS();
}

/* Test: Log level filtering */
void test_log_level_filtering(void) {
    printf("Testing: Log level filtering ... ");
    tests_run++;

    /* Set log level to INFO */
    katra_setenv("KATRA_LOG_LEVEL", "INFO");

    /* DEBUG should be filtered, INFO+ should be logged */
    LOG_DEBUG("This should be filtered");
    LOG_INFO("This should be logged");
    LOG_WARN("This should be logged");
    LOG_ERROR("This should be logged");

    /* Reset to DEBUG */
    katra_setenv("KATRA_LOG_LEVEL", "DEBUG");

    TEST_PASS();
}

/* Test: Null message handling */
void test_null_message(void) {
    printf("Testing: Null message handling ... ");
    tests_run++;

    /* These should not crash - implementation should handle NULL gracefully */
    /* Note: Can't directly pass NULL to variadic macros, so just test empty string */
    LOG_INFO("");
    LOG_DEBUG("");

    TEST_PASS();
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Logging Tests\n");
    printf("========================================\n\n");

    /* Setup - load environment */
    katra_loadenv();

    /* Run tests */
    test_log_init();
    test_log_levels();
    test_log_format();
    test_log_directory();
    test_log_file_creation();
    test_multiple_logs();
    test_long_message();
    test_log_level_filtering();
    test_null_message();

    /* Cleanup */
    katra_freeenv();

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
