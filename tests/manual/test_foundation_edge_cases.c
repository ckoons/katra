/* © 2025 Casey Koons All rights reserved */

/* Manual test for foundation layer edge cases */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_error.h"
#include "katra_path_utils.h"
#include "katra_json_utils.h"
#include "katra_limits.h"

#define TEST_PASS "\033[32m✓\033[0m"
#define TEST_FAIL "\033[31m✗\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

void test_assert(int condition, const char* test_name) {
    if (condition) {
        printf("%s %s\n", TEST_PASS, test_name);
        tests_passed++;
    } else {
        printf("%s %s\n", TEST_FAIL, test_name);
        tests_failed++;
    }
}

/* Test error handling edge cases */
void test_error_handling(void) {
    printf("\n=== Error Handling Edge Cases ===\n");

    /* Test error string for success */
    const char* success_str = katra_error_string(KATRA_SUCCESS);
    test_assert(strcmp(success_str, "Success") == 0, "Error string for SUCCESS");

    /* Test error name for success */
    const char* success_name = katra_error_name(KATRA_SUCCESS);
    test_assert(strcmp(success_name, "SUCCESS") == 0, "Error name for SUCCESS");

    /* Test error string for various error codes */
    const char* mem_err = katra_error_string(E_SYSTEM_MEMORY);
    test_assert(strstr(mem_err, "Out of memory") != NULL, "System memory error string");
    test_assert(strstr(mem_err, "SYSTEM:1001") != NULL, "Error code in string");

    /* Test error name extraction */
    const char* null_err_name = katra_error_name(E_INPUT_NULL);
    test_assert(strcmp(null_err_name, "E_INPUT_NULL") == 0, "Input NULL error name");

    /* Test error suggestion */
    const char* suggestion = katra_error_suggestion(E_SYSTEM_MEMORY);
    test_assert(suggestion != NULL && strlen(suggestion) > 0, "Error suggestion provided");

    /* Test unknown error code */
    const char* unknown = katra_error_name(99999);
    test_assert(strcmp(unknown, "E_UNKNOWN") == 0, "Unknown error code handling");

    /* Test error formatting */
    char buffer[512];
    int written = katra_error_format(buffer, sizeof(buffer), E_INPUT_NULL);
    test_assert(written > 0, "Error format writes data");
    test_assert(strstr(buffer, "E_INPUT_NULL") != NULL, "Error format includes name");
    test_assert(strstr(buffer, "Null pointer provided") != NULL, "Error format includes message");

    /* Test error format with NULL buffer */
    int result = katra_error_format(NULL, 100, E_INPUT_NULL);
    test_assert(result == -1, "Error format rejects NULL buffer");

    /* Test error format with zero size */
    result = katra_error_format(buffer, 0, E_INPUT_NULL);
    test_assert(result == -1, "Error format rejects zero size");

    /* Test error type extraction macros */
    int type = KATRA_ERROR_TYPE(E_SYSTEM_MEMORY);
    int num = KATRA_ERROR_NUM(E_SYSTEM_MEMORY);
    test_assert(type == ERR_SYSTEM, "Error type extraction");
    test_assert(num == 1001, "Error number extraction");
}

/* Test path utilities edge cases */
void test_path_utilities(void) {
    printf("\n=== Path Utilities Edge Cases ===\n");

    char buffer[KATRA_PATH_MAX];

    /* Test NULL buffer */
    int result = katra_get_home_dir(NULL, 100);
    test_assert(result == E_INPUT_NULL, "Home dir rejects NULL buffer");

    /* Test zero size */
    result = katra_get_home_dir(buffer, 0);
    test_assert(result == E_INPUT_NULL, "Home dir rejects zero size");

    /* Test normal home dir */
    result = katra_get_home_dir(buffer, sizeof(buffer));
    test_assert(result == KATRA_SUCCESS, "Get home dir succeeds");
    test_assert(strlen(buffer) > 0, "Home dir is non-empty");

    /* Test build path with NULL */
    result = katra_build_path(NULL, 100, "test", NULL);
    test_assert(result == E_INPUT_NULL, "Build path rejects NULL buffer");

    /* Test build path with zero size */
    result = katra_build_path(buffer, 0, "test", NULL);
    test_assert(result == E_INPUT_NULL, "Build path rejects zero size");

    /* Test build path normal */
    result = katra_build_path(buffer, sizeof(buffer), "memory", "tier1", NULL);
    test_assert(result == KATRA_SUCCESS, "Build path succeeds");
    test_assert(strstr(buffer, ".katra/memory/tier1") != NULL, "Path contains components");

    /* Test build path with too small buffer */
    char small_buffer[10];
    result = katra_build_path(small_buffer, sizeof(small_buffer),
                              "very", "long", "path", "that", "wont", "fit", NULL);
    test_assert(result == E_INPUT_TOO_LARGE, "Build path detects overflow");

    /* Test path join with NULL */
    result = katra_path_join(buffer, sizeof(buffer), NULL, "file.txt");
    test_assert(result == E_INPUT_NULL, "Path join rejects NULL dir");

    result = katra_path_join(buffer, sizeof(buffer), "/tmp", NULL);
    test_assert(result == E_INPUT_NULL, "Path join rejects NULL filename");

    /* Test path join normal */
    result = katra_path_join(buffer, sizeof(buffer), "/tmp", "test.txt");
    test_assert(result == KATRA_SUCCESS, "Path join succeeds");
    test_assert(strcmp(buffer, "/tmp/test.txt") == 0, "Path join correct");

    /* Test path join with trailing slash */
    result = katra_path_join(buffer, sizeof(buffer), "/tmp/", "test.txt");
    test_assert(result == KATRA_SUCCESS, "Path join with trailing slash");
    test_assert(strcmp(buffer, "/tmp/test.txt") == 0, "No double slash");

    /* Test path join with extension */
    result = katra_path_join_with_ext(buffer, sizeof(buffer), "/tmp", "test", "txt");
    test_assert(result == KATRA_SUCCESS, "Path join with ext succeeds");
    test_assert(strcmp(buffer, "/tmp/test.txt") == 0, "Extension added correctly");
}

/* Test JSON utilities edge cases */
void test_json_utilities(void) {
    printf("\n=== JSON Utilities Edge Cases ===\n");

    const char* valid_json = "{\"name\":\"test\",\"value\":42,\"score\":3.14,\"flag\":true}";
    const char* malformed_json = "{\"incomplete\":";

    char buffer[256];
    int int_val;
    long long_val;
    size_t size_val;
    float float_val;
    bool bool_val;

    /* Test NULL parameters */
    int result = katra_json_get_string(NULL, "key", buffer, sizeof(buffer));
    test_assert(result == E_INPUT_NULL, "JSON get string rejects NULL json");

    result = katra_json_get_string(valid_json, NULL, buffer, sizeof(buffer));
    test_assert(result == E_INPUT_NULL, "JSON get string rejects NULL key");

    result = katra_json_get_string(valid_json, "name", NULL, sizeof(buffer));
    test_assert(result == E_INPUT_NULL, "JSON get string rejects NULL value buffer");

    result = katra_json_get_string(valid_json, "name", buffer, 0);
    test_assert(result == E_INPUT_NULL, "JSON get string rejects zero size");

    /* Test extracting string */
    result = katra_json_get_string(valid_json, "name", buffer, sizeof(buffer));
    test_assert(result == KATRA_SUCCESS, "JSON extract string succeeds");
    test_assert(strcmp(buffer, "test") == 0, "JSON string value correct");

    /* Test extracting non-existent key */
    result = katra_json_get_string(valid_json, "nonexistent", buffer, sizeof(buffer));
    test_assert(result == E_NOT_FOUND, "JSON reports missing key");

    /* Test extracting integer */
    result = katra_json_get_int(valid_json, "value", &int_val);
    test_assert(result == KATRA_SUCCESS, "JSON extract int succeeds");
    test_assert(int_val == 42, "JSON int value correct");

    /* Test NULL parameter for int */
    result = katra_json_get_int(valid_json, "value", NULL);
    test_assert(result == E_INPUT_NULL, "JSON get int rejects NULL value ptr");

    /* Test extracting long */
    result = katra_json_get_long(valid_json, "value", &long_val);
    test_assert(result == KATRA_SUCCESS, "JSON extract long succeeds");
    test_assert(long_val == 42L, "JSON long value correct");

    /* Test extracting size_t */
    result = katra_json_get_size(valid_json, "value", &size_val);
    test_assert(result == KATRA_SUCCESS, "JSON extract size succeeds");
    test_assert(size_val == 42, "JSON size value correct");

    /* Test extracting float */
    result = katra_json_get_float(valid_json, "score", &float_val);
    test_assert(result == KATRA_SUCCESS, "JSON extract float succeeds");
    test_assert(float_val > 3.13 && float_val < 3.15, "JSON float value correct");

    /* Test extracting boolean */
    result = katra_json_get_bool(valid_json, "flag", &bool_val);
    test_assert(result == KATRA_SUCCESS, "JSON extract bool succeeds");
    test_assert(bool_val == true, "JSON bool value correct");

    /* Test JSON escaping */
    char escaped[256];
    const char* test_string = "Line 1\nLine 2\tTabbed\"Quote\\Backslash";
    katra_json_escape(test_string, escaped, sizeof(escaped));
    test_assert(strstr(escaped, "\\n") != NULL, "Newline escaped");
    test_assert(strstr(escaped, "\\t") != NULL, "Tab escaped");
    test_assert(strstr(escaped, "\\\"") != NULL, "Quote escaped");
    test_assert(strstr(escaped, "\\\\") != NULL, "Backslash escaped");

    /* Test JSON escape with NULL */
    katra_json_escape(NULL, escaped, sizeof(escaped));
    test_assert(escaped[0] == '\0', "NULL string escapes to empty");

    /* Test JSON escape with NULL destination */
    katra_json_escape(test_string, NULL, 100);
    /* Should not crash */
    test_assert(1, "NULL destination doesn't crash");

    /* Test malformed JSON */
    result = katra_json_get_string(malformed_json, "incomplete", buffer, sizeof(buffer));
    test_assert(result == E_NOT_FOUND, "Malformed JSON handled gracefully");
}

/* Test special character handling */
void test_special_characters(void) {
    printf("\n=== Special Character Handling ===\n");

    /* Test JSON with special characters */
    const char* json_with_special = "{\"message\":\"Test\\nNewline\\tTab\\rReturn\"}";
    char buffer[256];

    /* Note: The JSON in memory is already escaped, so we're testing extraction */
    int result = katra_json_get_string(json_with_special, "message", buffer, sizeof(buffer));
    test_assert(result == KATRA_SUCCESS, "Extract escaped JSON succeeds");
    /* The extracted value will have escaped sequences as literals */

    /* Test escaping various characters */
    const char* special = "Test\nNew\tTab\rReturn\"Quote\\Slash";
    char escaped[256];
    katra_json_escape(special, escaped, sizeof(escaped));

    test_assert(strstr(escaped, "\\n") != NULL, "Newline escaping");
    test_assert(strstr(escaped, "\\t") != NULL, "Tab escaping");
    test_assert(strstr(escaped, "\\r") != NULL, "Return escaping");
    test_assert(strstr(escaped, "\\\"") != NULL, "Quote escaping");
    test_assert(strstr(escaped, "\\\\") != NULL, "Backslash escaping");

    /* Test path with spaces (shouldn't break anything) */
    char path_buffer[KATRA_PATH_MAX];
    result = katra_path_join(path_buffer, sizeof(path_buffer),
                             "/tmp/test space", "file name.txt");
    test_assert(result == KATRA_SUCCESS, "Path with spaces handled");
    test_assert(strstr(path_buffer, "test space/file name.txt") != NULL,
                "Spaces preserved in path");
}

int main(void) {
    printf("=========================================\n");
    printf("Foundation Layer Edge Case Tests\n");
    printf("=========================================\n");

    test_error_handling();
    test_path_utilities();
    test_json_utilities();
    test_special_characters();

    printf("\n=========================================\n");
    printf("Test Results:\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("=========================================\n");

    return tests_failed == 0 ? 0 : 1;
}
