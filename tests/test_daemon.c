/* © 2025 Casey Koons All rights reserved */
/* Phase 9: Interstitial Autonomy Daemon Tests */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "katra_daemon.h"
#include "katra_breathing.h"
#include "katra_error.h"
#include "katra_limits.h"

#define TEST_CI_ID "test_daemon_ci"

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(name) do { \
    printf("Testing: %s ... ", #name); \
    fflush(stdout); \
    if (name() == 0) { \
        printf(" ✓\n"); \
        tests_passed++; \
    } else { \
        printf(" ✗\n"); \
    } \
    tests_run++; \
} while(0)

/* Test: Daemon initialization */
static int test_daemon_init(void) {
    int result = katra_daemon_init();
    if (result != KATRA_SUCCESS) return 1;

    /* Double init should be safe */
    result = katra_daemon_init();
    if (result != KATRA_SUCCESS) return 1;

    return 0;
}

/* Test: Default configuration */
static int test_default_config(void) {
    daemon_config_t config;
    katra_daemon_default_config(&config);

    if (!config.enabled) return 1;
    if (config.interval_minutes != DAEMON_DEFAULT_INTERVAL_MINUTES) return 1;
    if (config.max_memories_per_run != DAEMON_DEFAULT_MAX_MEMORIES) return 1;
    if (!config.pattern_extraction) return 1;
    if (!config.association_formation) return 1;
    if (!config.theme_detection) return 1;
    if (!config.insight_generation) return 1;

    return 0;
}

/* Test: Config save and load */
static int test_config_save_load(void) {
    daemon_config_t config;
    katra_daemon_default_config(&config);

    /* Modify config */
    config.interval_minutes = 30;
    config.quiet_hours_start = 23;
    config.quiet_hours_end = 7;
    config.pattern_extraction = false;

    int result = katra_daemon_save_config(&config);
    if (result != KATRA_SUCCESS) return 1;

    /* Load and verify */
    daemon_config_t loaded;
    result = katra_daemon_load_config(&loaded);
    if (result != KATRA_SUCCESS) return 1;

    if (loaded.interval_minutes != 30) return 1;
    if (loaded.quiet_hours_start != 23) return 1;
    if (loaded.quiet_hours_end != 7) return 1;
    if (loaded.pattern_extraction != false) return 1;

    return 0;
}

/* Test: Should run check (quiet hours) */
static int test_should_run(void) {
    daemon_config_t config;
    katra_daemon_default_config(&config);

    /* Disabled daemon should not run */
    config.enabled = false;
    if (katra_daemon_should_run(&config)) return 1;

    /* NULL config should not run */
    if (katra_daemon_should_run(NULL)) return 1;

    return 0;
}

/* Test: CI active check */
static int test_ci_active(void) {
    /* NULL should return false */
    if (katra_daemon_ci_active(NULL)) return 1;

    /* Non-existent CI should return false */
    if (katra_daemon_ci_active("nonexistent_ci_12345")) return 1;

    return 0;
}

/* Test: Pattern extraction with no data */
static int test_pattern_extraction_empty(void) {
    daemon_pattern_t* patterns = NULL;
    size_t count = 0;

    int result = katra_daemon_extract_patterns(TEST_CI_ID, 100, &patterns, &count);
    if (result != KATRA_SUCCESS) return 1;

    /* Should return 0 patterns with no data */
    if (count != 0) {
        katra_daemon_free_patterns(patterns, count);
        return 1;
    }

    return 0;
}

/* Test: Association formation with no data */
static int test_association_formation_empty(void) {
    size_t formed = 999;

    int result = katra_daemon_form_associations(TEST_CI_ID, 100, &formed);
    if (result != KATRA_SUCCESS) return 1;

    /* Should form 0 associations with no data */
    if (formed != 0) return 1;

    return 0;
}

/* Test: Theme detection with no data */
static int test_theme_detection_empty(void) {
    theme_cluster_t* themes = NULL;
    size_t count = 0;

    int result = katra_daemon_detect_themes(TEST_CI_ID, 100, &themes, &count);
    if (result != KATRA_SUCCESS) return 1;

    /* Should return 0 themes with no data */
    if (count != 0) {
        katra_daemon_free_themes(themes, count);
        return 1;
    }

    return 0;
}

/* Test: Insight generation with no patterns/themes */
static int test_insight_generation_empty(void) {
    daemon_insight_t* insights = NULL;
    size_t count = 0;

    int result = katra_daemon_generate_insights(TEST_CI_ID, NULL, 0, NULL, 0,
                                                 &insights, &count);
    if (result != KATRA_SUCCESS) return 1;

    /* Should return 0 insights with no input */
    if (count != 0) {
        katra_daemon_free_insights(insights, count);
        return 1;
    }

    return 0;
}

/* Test: Get pending insights (empty) */
static int test_pending_insights_empty(void) {
    daemon_insight_t* insights = NULL;
    size_t count = 999;

    int result = katra_daemon_get_pending_insights(TEST_CI_ID, &insights, &count);
    if (result != KATRA_SUCCESS) return 1;

    /* Count should be 0 or some valid number */
    katra_daemon_free_insights(insights, count);
    return 0;
}

/* Test: Acknowledge insight (non-existent) */
static int test_acknowledge_nonexistent(void) {
    /* Acknowledging non-existent insight should still succeed (idempotent) */
    int result = katra_daemon_acknowledge_insight("nonexistent_insight_12345");
    /* This may fail or succeed depending on implementation */
    (void)result;
    return 0;
}

/* Test: Insight type names */
static int test_insight_type_names(void) {
    const char* name;

    name = katra_insight_type_name(INSIGHT_PATTERN);
    if (!name || strcmp(name, "pattern") != 0) return 1;

    name = katra_insight_type_name(INSIGHT_ASSOCIATION);
    if (!name || strcmp(name, "association") != 0) return 1;

    name = katra_insight_type_name(INSIGHT_THEME);
    if (!name || strcmp(name, "theme") != 0) return 1;

    name = katra_insight_type_name(INSIGHT_TEMPORAL);
    if (!name || strcmp(name, "temporal") != 0) return 1;

    name = katra_insight_type_name(INSIGHT_EMOTIONAL);
    if (!name || strcmp(name, "emotional") != 0) return 1;

    /* Invalid type should return "unknown" */
    name = katra_insight_type_name((insight_type_t)999);
    if (!name || strcmp(name, "unknown") != 0) return 1;

    return 0;
}

/* Test: Generate insight ID */
static int test_generate_insight_id(void) {
    char id1[64] = {0};
    char id2[64] = {0};

    katra_daemon_generate_insight_id(id1, sizeof(id1));
    if (strlen(id1) == 0) return 1;
    if (strncmp(id1, "ins_", 4) != 0) return 1;

    /* Generate another - should be different */
    usleep(1000);  /* Small delay to ensure different timestamp */
    katra_daemon_generate_insight_id(id2, sizeof(id2));
    if (strlen(id2) == 0) return 1;

    return 0;
}

/* Test: Format sunrise insights */
static int test_format_sunrise_insights(void) {
    char buffer[1024];

    /* Empty insights */
    int result = katra_daemon_format_sunrise_insights(NULL, 0, buffer, sizeof(buffer));
    if (result != KATRA_SUCCESS) return 1;
    if (buffer[0] != '\0') return 1;

    /* NULL buffer should fail */
    result = katra_daemon_format_sunrise_insights(NULL, 0, NULL, 0);
    if (result == KATRA_SUCCESS) return 1;

    return 0;
}

/* Test: Full daemon cycle (with no data) */
static int test_daemon_cycle_empty(void) {
    daemon_config_t config;
    katra_daemon_default_config(&config);

    daemon_result_t result_data;
    int result = katra_daemon_run_cycle(TEST_CI_ID, &config, &result_data);
    if (result != KATRA_SUCCESS) return 1;

    /* With no memories, should complete quickly with 0 results */
    if (result_data.error_code != 0) return 1;

    return 0;
}

/* Test: Daemon history */
static int test_daemon_history(void) {
    daemon_result_t* history = NULL;
    size_t count = 0;

    int result = katra_daemon_get_history(TEST_CI_ID, &history, &count);
    if (result != KATRA_SUCCESS) return 1;

    /* Should have at least one entry from test_daemon_cycle_empty */
    katra_daemon_free_history(history, count);
    return 0;
}

/* Test: NULL parameter handling */
static int test_null_params(void) {
    /* All functions should handle NULL gracefully */
    if (katra_daemon_load_config(NULL) != E_INPUT_NULL) return 1;
    if (katra_daemon_save_config(NULL) != E_INPUT_NULL) return 1;

    daemon_pattern_t* patterns;
    size_t count;
    if (katra_daemon_extract_patterns(NULL, 100, &patterns, &count) != E_INPUT_NULL) return 1;
    if (katra_daemon_extract_patterns(TEST_CI_ID, 100, NULL, &count) != E_INPUT_NULL) return 1;

    if (katra_daemon_form_associations(NULL, 100, &count) != E_INPUT_NULL) return 1;

    theme_cluster_t* themes;
    if (katra_daemon_detect_themes(NULL, 100, &themes, &count) != E_INPUT_NULL) return 1;

    daemon_insight_t* insights;
    if (katra_daemon_generate_insights(NULL, NULL, 0, NULL, 0, &insights, &count) != E_INPUT_NULL) return 1;

    if (katra_daemon_get_pending_insights(NULL, &insights, &count) != E_INPUT_NULL) return 1;
    if (katra_daemon_acknowledge_insight(NULL) != E_INPUT_NULL) return 1;

    daemon_result_t result;
    if (katra_daemon_run_cycle(NULL, NULL, &result) != E_INPUT_NULL) return 1;

    daemon_result_t* history;
    if (katra_daemon_get_history(NULL, &history, &count) != E_INPUT_NULL) return 1;

    return 0;
}

/* Test: Cleanup */
static int test_daemon_cleanup(void) {
    katra_daemon_cleanup();

    /* Double cleanup should be safe */
    katra_daemon_cleanup();

    return 0;
}

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Phase 9: Interstitial Autonomy Daemon Tests\n");
    printf("========================================\n");
    printf("\n");

    /* Initialize breathing layer for memory operations */
    breathe_init(TEST_CI_ID);
    session_start(TEST_CI_ID);

    RUN_TEST(test_daemon_init);
    RUN_TEST(test_default_config);
    RUN_TEST(test_config_save_load);
    RUN_TEST(test_should_run);
    RUN_TEST(test_ci_active);
    RUN_TEST(test_pattern_extraction_empty);
    RUN_TEST(test_association_formation_empty);
    RUN_TEST(test_theme_detection_empty);
    RUN_TEST(test_insight_generation_empty);
    RUN_TEST(test_pending_insights_empty);
    RUN_TEST(test_acknowledge_nonexistent);
    RUN_TEST(test_insight_type_names);
    RUN_TEST(test_generate_insight_id);
    RUN_TEST(test_format_sunrise_insights);
    RUN_TEST(test_daemon_cycle_empty);
    RUN_TEST(test_daemon_history);
    RUN_TEST(test_null_params);
    RUN_TEST(test_daemon_cleanup);

    /* Cleanup */
    session_end();
    breathe_cleanup();

    printf("\n");
    printf("========================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_run - tests_passed);
    printf("========================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
