/* © 2025 Casey Koons All rights reserved */

/*
 * test_persona_paths.c - Tests for dual-location persona path utilities
 *
 * Tests the path functions that support shipped vs user persona locations:
 * - Shipped: {project_root}/personas/{name}/ (Git-tracked templates)
 * - User: ~/.katra/personas/{name}/ (User data, never in Git)
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Project includes */
#include "katra_path_utils.h"
#include "katra_error.h"
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

/* Test: Get home directory */
void test_get_home_dir(void) {
    printf("Testing: Get home directory ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_get_home_dir(buffer, sizeof(buffer));

    ASSERT(result == KATRA_SUCCESS, "katra_get_home_dir() failed");
    ASSERT(strlen(buffer) > 0, "Home directory is empty");
    ASSERT(buffer[0] == '/', "Home directory is not absolute path");
}

/* Test: Build path under ~/.katra/ */
void test_build_path(void) {
    printf("Testing: Build path under ~/.katra/ ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_build_path(buffer, sizeof(buffer), "memory", "tier1", NULL);

    ASSERT(result == KATRA_SUCCESS, "katra_build_path() failed");
    ASSERT(strstr(buffer, ".katra") != NULL, "Path doesn't contain .katra");
    ASSERT(strstr(buffer, "memory") != NULL, "Path doesn't contain 'memory'");
    ASSERT(strstr(buffer, "tier1") != NULL, "Path doesn't contain 'tier1'");
}

/* Test: Path join */
void test_path_join(void) {
    printf("Testing: Path join ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_path_join(buffer, sizeof(buffer), "/tmp/test", "file.txt");

    ASSERT(result == KATRA_SUCCESS, "katra_path_join() failed");
    ASSERT(strcmp(buffer, "/tmp/test/file.txt") == 0, "Joined path incorrect");
}

/* Test: Path join with trailing slash */
void test_path_join_trailing_slash(void) {
    printf("Testing: Path join with trailing slash ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_path_join(buffer, sizeof(buffer), "/tmp/test/", "file.txt");

    ASSERT(result == KATRA_SUCCESS, "katra_path_join() with trailing slash failed");
    ASSERT(strcmp(buffer, "/tmp/test/file.txt") == 0, "Joined path with trailing slash incorrect");
}

/* Test: Path join with extension */
void test_path_join_with_ext(void) {
    printf("Testing: Path join with extension ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_path_join_with_ext(buffer, sizeof(buffer),
                                           "/tmp/test", "file", "txt");

    ASSERT(result == KATRA_SUCCESS, "katra_path_join_with_ext() failed");
    ASSERT(strcmp(buffer, "/tmp/test/file.txt") == 0, "Joined path with extension incorrect");
}

/* Test: Get project root */
void test_get_project_root(void) {
    printf("Testing: Get project root ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_get_project_root(buffer, sizeof(buffer));

    ASSERT(result == KATRA_SUCCESS, "katra_get_project_root() failed");
    ASSERT(strlen(buffer) > 0, "Project root is empty");
    ASSERT(buffer[0] == '/', "Project root is not absolute path");

    /* Verify it looks like katra project */
    char makefile_path[KATRA_PATH_MAX];
    snprintf(makefile_path, sizeof(makefile_path), "%s/Makefile", buffer);
    struct stat st;
    ASSERT(stat(makefile_path, &st) == 0, "Project root doesn't contain Makefile");
}

/* Test: Get shipped persona directory */
void test_get_shipped_persona_dir(void) {
    printf("Testing: Get shipped persona directory ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_get_shipped_persona_dir(buffer, sizeof(buffer), "Assistant");

    ASSERT(result == KATRA_SUCCESS, "katra_get_shipped_persona_dir() failed");
    ASSERT(strstr(buffer, "personas") != NULL, "Path doesn't contain 'personas'");
    ASSERT(strstr(buffer, "Assistant") != NULL, "Path doesn't contain persona name");
    ASSERT(strstr(buffer, ".katra") == NULL, "Shipped path should not be under ~/.katra");
}

/* Test: Get user persona directory */
void test_get_user_persona_dir(void) {
    printf("Testing: Get user persona directory ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_get_user_persona_dir(buffer, sizeof(buffer), "MyCustomPersona");

    ASSERT(result == KATRA_SUCCESS, "katra_get_user_persona_dir() failed");
    ASSERT(strstr(buffer, ".katra") != NULL, "Path doesn't contain .katra");
    ASSERT(strstr(buffer, "personas") != NULL, "Path doesn't contain 'personas'");
    ASSERT(strstr(buffer, "MyCustomPersona") != NULL, "Path doesn't contain persona name");
}

/* Test: Build user persona path */
void test_build_user_persona_path(void) {
    printf("Testing: Build user persona path ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_build_user_persona_path(buffer, sizeof(buffer),
                                                 "TestPersona", "memory", "tier1", NULL);

    ASSERT(result == KATRA_SUCCESS, "katra_build_user_persona_path() failed");
    ASSERT(strstr(buffer, ".katra") != NULL, "Path doesn't contain .katra");
    ASSERT(strstr(buffer, "personas") != NULL, "Path doesn't contain 'personas'");
    ASSERT(strstr(buffer, "TestPersona") != NULL, "Path doesn't contain persona name");
    ASSERT(strstr(buffer, "memory") != NULL, "Path doesn't contain 'memory'");
    ASSERT(strstr(buffer, "tier1") != NULL, "Path doesn't contain 'tier1'");
}

/* Test: NULL parameter handling */
void test_null_parameters(void) {
    printf("Testing: NULL parameter handling ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result;

    /* NULL buffer */
    result = katra_get_home_dir(NULL, sizeof(buffer));
    ASSERT(result == E_INPUT_NULL, "NULL buffer should return E_INPUT_NULL");

    /* NULL persona name */
    result = katra_get_user_persona_dir(buffer, sizeof(buffer), NULL);
    ASSERT(result == E_INPUT_NULL, "NULL persona name should return E_INPUT_NULL");
}

/* Test: Buffer overflow protection */
void test_buffer_overflow(void) {
    printf("Testing: Buffer overflow protection ... ");
    tests_run++;

    char small_buffer[10];
    int result;

    /* Try to build a path that's too long */
    result = katra_build_path(small_buffer, sizeof(small_buffer),
                               "very_long_directory_name",
                               "another_long_name",
                               "yet_another_component", NULL);

    ASSERT(result == E_INPUT_TOO_LARGE, "Should detect buffer overflow");
}

/* Test: Deprecated get_persona_dir compatibility */
void test_deprecated_get_persona_dir(void) {
    printf("Testing: Deprecated get_persona_dir compatibility ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_get_persona_dir(buffer, sizeof(buffer), "TestPersona");

    ASSERT(result == KATRA_SUCCESS, "katra_get_persona_dir() failed");
    ASSERT(strstr(buffer, ".katra") != NULL, "Deprecated function should point to .katra directory");
    /* Note: Deprecated function supports both unified and scattered layouts via KATRA_PERSONA_LAYOUT env var */
    /* Default is scattered layout: ~/.katra/TestPersona/ (backward compatibility) */
}

/* Test: Deprecated build_persona_path compatibility */
void test_deprecated_build_persona_path(void) {
    printf("Testing: Deprecated build_persona_path compatibility ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    int result = katra_build_persona_path(buffer, sizeof(buffer),
                                           "TestPersona", "config", NULL);

    ASSERT(result == KATRA_SUCCESS, "katra_build_persona_path() failed");
    ASSERT(strstr(buffer, ".katra") != NULL, "Deprecated function should point to user location");
    ASSERT(strstr(buffer, "config") != NULL, "Path doesn't contain 'config'");
}

/* Test: Ensure directory creation */
void test_ensure_dir(void) {
    printf("Testing: Ensure directory creation ... ");
    tests_run++;

    char test_dir[KATRA_PATH_MAX];
    snprintf(test_dir, sizeof(test_dir), "/tmp/katra_test_dir_%d", getpid());

    /* Clean up if exists */
    rmdir(test_dir);

    /* Create directory */
    int result = katra_ensure_dir(test_dir);
    ASSERT(result == KATRA_SUCCESS, "katra_ensure_dir() failed");

    /* Verify it exists */
    struct stat st;
    ASSERT(stat(test_dir, &st) == 0, "Directory was not created");
    ASSERT(S_ISDIR(st.st_mode), "Path is not a directory");

    /* Test idempotency - calling again should succeed */
    result = katra_ensure_dir(test_dir);
    ASSERT(result == KATRA_SUCCESS, "katra_ensure_dir() should be idempotent");

    /* Cleanup */
    rmdir(test_dir);
}

/* Test: Build and ensure directory */
void test_build_and_ensure_dir(void) {
    printf("Testing: Build and ensure directory ... ");
    tests_run++;

    char buffer[KATRA_PATH_MAX];
    snprintf(buffer, sizeof(buffer), "/tmp/katra_test_%d", getpid());

    /* Clean up if exists */
    char test_path[KATRA_PATH_MAX];
    snprintf(test_path, sizeof(test_path), "%s/sub1/sub2", buffer);
    rmdir(test_path);
    snprintf(test_path, sizeof(test_path), "%s/sub1", buffer);
    rmdir(test_path);
    rmdir(buffer);

    /* Create nested directory structure */
    int result = katra_ensure_dir(test_path);
    ASSERT(result == KATRA_SUCCESS, "Failed to create nested directories");

    /* Verify it exists */
    struct stat st;
    ASSERT(stat(test_path, &st) == 0, "Nested directory was not created");
    ASSERT(S_ISDIR(st.st_mode), "Path is not a directory");

    /* Cleanup */
    rmdir(test_path);
    snprintf(test_path, sizeof(test_path), "%s/sub1", buffer);
    rmdir(test_path);
    rmdir(buffer);
}

/* Main */
int main(void) {
    printf("========================================\n");
    printf("Persona Path Utilities Test Suite\n");
    printf("========================================\n\n");

    /* Run tests */
    test_get_home_dir();
    test_build_path();
    test_path_join();
    test_path_join_trailing_slash();
    test_path_join_with_ext();
    test_get_project_root();
    test_get_shipped_persona_dir();
    test_get_user_persona_dir();
    test_build_user_persona_path();
    test_null_parameters();
    test_buffer_overflow();
    test_deprecated_get_persona_dir();
    test_deprecated_build_persona_path();
    test_ensure_dir();
    test_build_and_ensure_dir();

    /* Summary */
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("========================================\n");

    return (tests_failed == 0) ? 0 : 1;
}
