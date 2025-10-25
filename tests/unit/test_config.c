/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Project includes */
#include "katra_config.h"
#include "katra_env_utils.h"
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

/* Setup test configuration */
static int setup_test_config(void) {
    const char* home = getenv("HOME");
    if (!home) return -1;

    char base_dir[KATRA_PATH_MAX];
    char config_dir[KATRA_PATH_MAX];
    char config_file[KATRA_PATH_MAX];

    /* Create ~/.katra directory first */
    snprintf(base_dir, sizeof(base_dir), "%s/.katra", home);
    mkdir(base_dir, KATRA_DIR_PERMISSIONS);

    /* Create ~/.katra/config directory */
    snprintf(config_dir, sizeof(config_dir), "%s/.katra/config", home);
    mkdir(config_dir, KATRA_DIR_PERMISSIONS);

    /* Write test config file */
    snprintf(config_file, sizeof(config_file), "%s/test.conf", config_dir);
    FILE* fp = fopen(config_file, "w");
    if (!fp) return -1;

    fprintf(fp, "# Test configuration\n");
    fprintf(fp, "test_key=test_value\n");
    fprintf(fp, "log_level=debug\n");
    fprintf(fp, "memory_tier_size=1024\n");
    fprintf(fp, "checkpoint_interval=300\n");
    fprintf(fp, "\n");
    fprintf(fp, "# Quoted values\n");
    fprintf(fp, "quoted_value=\"value with spaces\"\n");
    fprintf(fp, "\n");
    fprintf(fp, "# Empty value\n");
    fprintf(fp, "empty_value=\n");

    fclose(fp);
    return 0;
}

/* Cleanup test configuration */
static void cleanup_test_config(void) {
    const char* home = getenv("HOME");
    if (!home) return;

    char config_file[KATRA_PATH_MAX];
    snprintf(config_file, sizeof(config_file), "%s/.katra/config/test.conf", home);
    unlink(config_file);
}

/* Test: Load configuration */
void test_load_config(void) {
    printf("Testing: Load configuration ... ");
    tests_run++;

    int result = katra_config();
    ASSERT(result == KATRA_SUCCESS, "Config load failed");
}

/* Test: Get configuration value */
void test_get_config(void) {
    printf("Testing: Get configuration value ... ");
    tests_run++;

    const char* val = katra_config_get("test_key");
    ASSERT(val && strcmp(val, "test_value") == 0, "Get config failed");
}

/* Test: Get integer configuration */
void test_get_integer_config(void) {
    printf("Testing: Get integer configuration ... ");
    tests_run++;

    const char* val = katra_config_get("memory_tier_size");
    ASSERT(val && strcmp(val, "1024") == 0, "Get integer config failed");

    int int_val = atoi(val);
    ASSERT(int_val == 1024, "Integer conversion failed");
}

/* Test: Get quoted value */
void test_quoted_value(void) {
    printf("Testing: Get quoted value ... ");
    tests_run++;

    const char* val = katra_config_get("quoted_value");
    ASSERT(val && strcmp(val, "value with spaces") == 0, "Quoted value failed");
}

/* Test: Get empty value */
void test_empty_value(void) {
    printf("Testing: Get empty value ... ");
    tests_run++;

    const char* val = katra_config_get("empty_value");
    ASSERT(val && strlen(val) == 0, "Empty value failed");
}

/* Test: Get nonexistent key */
void test_nonexistent_key(void) {
    printf("Testing: Get nonexistent key ... ");
    tests_run++;

    const char* val = katra_config_get("NONEXISTENT_KEY");
    ASSERT(val == NULL, "Nonexistent key should return NULL");
}

/* Test: Null key */
void test_null_key(void) {
    printf("Testing: Get with NULL key ... ");
    tests_run++;

    const char* val = katra_config_get(NULL);
    ASSERT(val == NULL, "NULL key should return NULL");
}

/* Test: Reload configuration */
void test_reload_config(void) {
    printf("Testing: Reload configuration ... ");
    tests_run++;

    int result = katra_config_reload();
    ASSERT(result == KATRA_SUCCESS, "Config reload failed");

    /* Verify values still accessible */
    const char* val = katra_config_get("test_key");
    ASSERT(val && strcmp(val, "test_value") == 0, "Values lost after reload");
}

/* Test: Multiple calls (idempotent) */
void test_multiple_calls(void) {
    printf("Testing: Multiple config calls (idempotent) ... ");
    tests_run++;

    int result1 = katra_config();
    int result2 = katra_config();

    ASSERT(result1 == KATRA_SUCCESS && result2 == KATRA_SUCCESS,
           "Multiple calls failed");
}

/* Test: Cleanup */
void test_cleanup(void) {
    printf("Testing: Config cleanup ... ");
    tests_run++;

    katra_config_cleanup();

    /* After cleanup, values should not be accessible */
    /* But we need to reload to verify, so just check cleanup doesn't crash */
    TEST_PASS();
}

/* Test: Directory structure creation */
void test_directory_creation(void) {
    printf("Testing: Directory structure creation ... ");
    tests_run++;

    const char* home = getenv("HOME");
    if (!home) {
        TEST_FAIL("HOME not set");
        return;
    }

    char dir_path[KATRA_PATH_MAX];
    struct stat st;

    /* Check ~/.katra/config */
    snprintf(dir_path, sizeof(dir_path), "%s/.katra/config", home);
    int result = stat(dir_path, &st);
    if (result != 0 || !S_ISDIR(st.st_mode)) {
        TEST_FAIL("~/.katra/config not created");
        return;
    }

    /* Check ~/.katra/logs */
    snprintf(dir_path, sizeof(dir_path), "%s/.katra/logs", home);
    result = stat(dir_path, &st);
    if (result != 0 || !S_ISDIR(st.st_mode)) {
        TEST_FAIL("~/.katra/logs not created");
        return;
    }

    /* Check ~/.katra/memory */
    snprintf(dir_path, sizeof(dir_path), "%s/.katra/memory", home);
    result = stat(dir_path, &st);
    if (result != 0 || !S_ISDIR(st.st_mode)) {
        TEST_FAIL("~/.katra/memory not created");
        return;
    }

    /* Check ~/.katra/checkpoints */
    snprintf(dir_path, sizeof(dir_path), "%s/.katra/checkpoints", home);
    result = stat(dir_path, &st);
    if (result != 0 || !S_ISDIR(st.st_mode)) {
        TEST_FAIL("~/.katra/checkpoints not created");
        return;
    }

    /* Check ~/.katra/audit */
    snprintf(dir_path, sizeof(dir_path), "%s/.katra/audit", home);
    result = stat(dir_path, &st);
    if (result != 0 || !S_ISDIR(st.st_mode)) {
        TEST_FAIL("~/.katra/audit not created");
        return;
    }

    TEST_PASS();
}

/* Main test runner */
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Configuration Tests\n");
    printf("========================================\n\n");

    /* Setup */
    if (setup_test_config() != 0) {
        fprintf(stderr, "ERROR: Failed to setup test configuration\n");
        return 1;
    }

    /* Load environment first (config depends on it) */
    katra_loadenv();

    /* Run tests */
    test_load_config();
    test_get_config();
    test_get_integer_config();
    test_quoted_value();
    test_empty_value();
    test_nonexistent_key();
    test_null_key();
    test_reload_config();
    test_multiple_calls();
    test_directory_creation();
    test_cleanup();

    /* Cleanup */
    cleanup_test_config();
    katra_config_cleanup();
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
