/* © 2025 Casey Koons All rights reserved */

/*
 * test_breathing_phase2.c - Tests for Phase 2 breathing layer improvements
 *
 * Tests three main features:
 * - Semantic reason parsing (string-based importance)
 * - Context configuration (tunable limits)
 * - Enhanced statistics tracking
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_limits.h"

/* Test CI ID */
#define TEST_CI_ID "test_phase2_ci"

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Cleanup before each test */
static void setup_test(void) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf ~/.katra/memory/tier1/%s", TEST_CI_ID);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "rm -rf ~/.katra/memory/tier2/%s", TEST_CI_ID);
    system(cmd);
    /* Clean index database to prevent dedup from finding old test memories */
    system("rm -f ~/.katra/memory/tier1/index/memories.db");
}

/* Test result reporting */
static void report_test(const char* test_name, int passed) {
    tests_run++;
    if (passed) {
        tests_passed++;
        printf("  ✓ %s\n", test_name);
    } else {
        tests_failed++;
        printf("  ✗ %s\n", test_name);
    }
}

/* ============================================================================
 * SEMANTIC REASON PARSING TESTS
 * ============================================================================ */

static void test_semantic_trivial_parsing(void) {
    const char* trivial_phrases[] = {
        "trivial",
        "fleeting thought",
        "not important",
        "unimportant detail",
        "forget this",
        NULL
    };

    int passed = 1;
    for (int i = 0; trivial_phrases[i] != NULL; i++) {
        float importance = string_to_importance(trivial_phrases[i]);
        if (importance != MEMORY_IMPORTANCE_TRIVIAL) {
            printf("    Failed: '%s' -> %.2f (expected %.2f)\n",
                   trivial_phrases[i], importance, MEMORY_IMPORTANCE_TRIVIAL);
            passed = 0;
        }
    }

    report_test("Semantic parsing: trivial phrases", passed);
}

static void test_semantic_routine_parsing(void) {
    const char* routine_phrases[] = {
        "routine",
        "normal activity",
        "everyday task",
        "regular occurrence",
        "usual thing",
        NULL
    };

    int passed = 1;
    for (int i = 0; routine_phrases[i] != NULL; i++) {
        float importance = string_to_importance(routine_phrases[i]);
        if (importance != MEMORY_IMPORTANCE_LOW) {
            printf("    Failed: '%s' -> %.2f (expected %.2f)\n",
                   routine_phrases[i], importance, MEMORY_IMPORTANCE_LOW);
            passed = 0;
        }
    }

    report_test("Semantic parsing: routine phrases", passed);
}

static void test_semantic_critical_parsing(void) {
    const char* critical_phrases[] = {
        "critical",
        "crucial decision",
        "life-changing event",
        "must remember this",
        "never forget",
        "extremely important",
        NULL
    };

    int passed = 1;
    for (int i = 0; critical_phrases[i] != NULL; i++) {
        float importance = string_to_importance(critical_phrases[i]);
        if (importance != MEMORY_IMPORTANCE_CRITICAL) {
            printf("    Failed: '%s' -> %.2f (expected %.2f)\n",
                   critical_phrases[i], importance, MEMORY_IMPORTANCE_CRITICAL);
            passed = 0;
        }
    }

    report_test("Semantic parsing: critical phrases", passed);
}

static void test_semantic_significant_parsing(void) {
    const char* significant_phrases[] = {
        "significant",
        "important insight",
        "very noteworthy",
        "this matters",
        "key finding",
        "essential information",
        NULL
    };

    int passed = 1;
    for (int i = 0; significant_phrases[i] != NULL; i++) {
        float importance = string_to_importance(significant_phrases[i]);
        if (importance != MEMORY_IMPORTANCE_HIGH) {
            printf("    Failed: '%s' -> %.2f (expected %.2f)\n",
                   significant_phrases[i], importance, MEMORY_IMPORTANCE_HIGH);
            passed = 0;
        }
    }

    report_test("Semantic parsing: significant phrases", passed);
}

static void test_semantic_interesting_parsing(void) {
    const char* interesting_phrases[] = {
        "interesting",
        "worth remembering",
        "notable",
        "noteworthy",
        NULL
    };

    int passed = 1;
    for (int i = 0; interesting_phrases[i] != NULL; i++) {
        float importance = string_to_importance(interesting_phrases[i]);
        if (importance != MEMORY_IMPORTANCE_MEDIUM) {
            printf("    Failed: '%s' -> %.2f (expected %.2f)\n",
                   interesting_phrases[i], importance, MEMORY_IMPORTANCE_MEDIUM);
            passed = 0;
        }
    }

    report_test("Semantic parsing: interesting phrases", passed);
}

static void test_semantic_case_insensitive(void) {
    float imp1 = string_to_importance("CRITICAL");
    float imp2 = string_to_importance("critical");
    float imp3 = string_to_importance("CrItIcAl");

    int passed = (imp1 == MEMORY_IMPORTANCE_CRITICAL &&
                  imp2 == MEMORY_IMPORTANCE_CRITICAL &&
                  imp3 == MEMORY_IMPORTANCE_CRITICAL);

    report_test("Semantic parsing: case insensitive", passed);
}

static void test_semantic_default(void) {
    const char* unrecognized[] = {
        NULL,
        "",
        "xyzabc",
        "random text",
        NULL
    };

    int passed = 1;
    for (int i = 0; unrecognized[i] != NULL; i++) {
        float importance = string_to_importance(unrecognized[i]);
        if (importance != MEMORY_IMPORTANCE_MEDIUM) {
            printf("    Failed: '%s' -> %.2f (expected default %.2f)\n",
                   unrecognized[i] ? unrecognized[i] : "NULL",
                   importance, MEMORY_IMPORTANCE_MEDIUM);
            passed = 0;
        }
    }

    /* NULL should also default to MEDIUM */
    if (string_to_importance(NULL) != MEMORY_IMPORTANCE_MEDIUM) {
        passed = 0;
    }

    report_test("Semantic parsing: default to MEDIUM", passed);
}

static void test_remember_semantic(void) {
    setup_test();

    int result = session_start(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        report_test("remember_semantic() basic usage", 0);
        return;
    }

    /* Store with semantic reason */
    result = remember_semantic(TEST_CI_ID, "Test semantic memory", "very important");

    /* Verify it stored successfully */
    int passed = (result == KATRA_SUCCESS);

    /* Check stats tracked semantic remember */
    enhanced_stats_t* stats = get_enhanced_statistics();
    if (stats) {
        passed = passed && (stats->semantic_remember_count == 1);
        passed = passed && (stats->total_memories_stored == 1);
        free(stats);
    } else {
        passed = 0;
    }

    session_end();
    breathe_cleanup();

    report_test("remember_semantic() basic usage", passed);
}

static void test_remember_with_semantic_note(void) {
    setup_test();

    int result = session_start(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        report_test("remember_with_semantic_note() usage", 0);
        return;
    }

    /* Store with semantic reason + note */
    result = remember_with_semantic_note(
        "Important discovery",
        "critical",
        "This changes everything"
    );

    /* Verify it stored successfully */
    int passed = (result == KATRA_SUCCESS);

    /* Check stats */
    enhanced_stats_t* stats = get_enhanced_statistics();
    if (stats) {
        passed = passed && (stats->semantic_remember_count == 1);
        free(stats);
    } else {
        passed = 0;
    }

    session_end();
    breathe_cleanup();

    report_test("remember_with_semantic_note() usage", passed);
}

/* ============================================================================
 * CONTEXT CONFIGURATION TESTS
 * ============================================================================ */

static void test_context_config_defaults(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Get default config */
    context_config_t* config = get_context_config();

    int passed = (config != NULL &&
                  config->max_relevant_memories == 10 &&
                  config->max_recent_thoughts == 20 &&
                  config->max_topic_recall == 100 &&
                  config->min_importance_relevant == MEMORY_IMPORTANCE_HIGH &&
                  config->max_context_age_days == 7);

    free(config);
    session_end();
    breathe_cleanup();

    report_test("Context config: default values", passed);
}

static void test_context_config_set(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Set custom config */
    context_config_t custom = {
        .max_relevant_memories = 20,
        .max_recent_thoughts = 50,
        .max_topic_recall = 200,
        .min_importance_relevant = MEMORY_IMPORTANCE_MEDIUM,
        .max_context_age_days = 14
    };

    int result = set_context_config(&custom);

    /* Verify it was set */
    context_config_t* retrieved = get_context_config();

    int passed = (result == KATRA_SUCCESS &&
                  retrieved != NULL &&
                  retrieved->max_relevant_memories == 20 &&
                  retrieved->max_recent_thoughts == 50 &&
                  retrieved->max_topic_recall == 200 &&
                  retrieved->min_importance_relevant == MEMORY_IMPORTANCE_MEDIUM &&
                  retrieved->max_context_age_days == 14);

    free(retrieved);
    session_end();
    breathe_cleanup();

    report_test("Context config: set custom values", passed);
}

static void test_context_config_validation(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Try to set invalid config (too large) */
    context_config_t invalid = {
        .max_relevant_memories = 2000,  /* Over 1000 limit */
        .max_recent_thoughts = 20,
        .max_topic_recall = 100,
        .min_importance_relevant = MEMORY_IMPORTANCE_HIGH,
        .max_context_age_days = 7
    };

    int result = set_context_config(&invalid);

    /* Should reject invalid config */
    int passed = (result == E_INVALID_PARAMS);

    /* Try invalid importance */
    context_config_t invalid2 = {
        .max_relevant_memories = 10,
        .max_recent_thoughts = 20,
        .max_topic_recall = 100,
        .min_importance_relevant = 1.5,  /* Invalid (>1.0) */
        .max_context_age_days = 7
    };

    result = set_context_config(&invalid2);
    passed = passed && (result == E_INVALID_PARAMS);

    session_end();
    breathe_cleanup();

    report_test("Context config: validation rejects invalid", passed);
}

static void test_context_config_reset(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Set custom config */
    context_config_t custom = {
        .max_relevant_memories = 50,
        .max_recent_thoughts = 100,
        .max_topic_recall = 500,
        .min_importance_relevant = MEMORY_IMPORTANCE_LOW,
        .max_context_age_days = 30
    };

    set_context_config(&custom);

    /* Reset to defaults */
    int result = set_context_config(NULL);

    /* Verify defaults restored */
    context_config_t* config = get_context_config();

    int passed = (result == KATRA_SUCCESS &&
                  config != NULL &&
                  config->max_relevant_memories == 10 &&
                  config->max_recent_thoughts == 20 &&
                  config->max_topic_recall == 100);

    free(config);
    session_end();
    breathe_cleanup();

    report_test("Context config: reset to defaults", passed);
}

/* ============================================================================
 * ENHANCED STATISTICS TESTS
 * ============================================================================ */

static void test_stats_memory_formation(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Store various types of memories */
    remember(TEST_CI_ID, "Experience", WHY_ROUTINE);
    learn(TEST_CI_ID, "Knowledge");
    reflect(TEST_CI_ID, "Reflection");
    decide(TEST_CI_ID, "Decision", "Because");
    notice_pattern(TEST_CI_ID, "Pattern");
    remember_semantic(TEST_CI_ID, "Semantic", "important");

    /* Get stats */
    enhanced_stats_t* stats = get_enhanced_statistics();

    int passed = (stats != NULL &&
                  stats->total_memories_stored == 6 &&
                  stats->by_type[MEMORY_TYPE_EXPERIENCE] == 2 &&  /* remember + remember_semantic */
                  stats->by_type[MEMORY_TYPE_KNOWLEDGE] == 1 &&
                  stats->by_type[MEMORY_TYPE_REFLECTION] == 1 &&
                  stats->by_type[MEMORY_TYPE_DECISION] == 1 &&
                  stats->by_type[MEMORY_TYPE_PATTERN] == 1 &&
                  stats->semantic_remember_count == 1);

    free(stats);
    session_end();
    breathe_cleanup();

    report_test("Enhanced stats: memory formation tracking", passed);
}

static void test_stats_importance_distribution(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Store memories with different importance levels */
    remember(TEST_CI_ID, "Trivial", WHY_TRIVIAL);
    remember(TEST_CI_ID, "Routine", WHY_ROUTINE);
    remember(TEST_CI_ID, "Interesting", WHY_INTERESTING);
    remember(TEST_CI_ID, "Significant", WHY_SIGNIFICANT);
    remember(TEST_CI_ID, "Critical", WHY_CRITICAL);

    /* Get stats */
    enhanced_stats_t* stats = get_enhanced_statistics();

    int passed = (stats != NULL &&
                  stats->by_importance[WHY_TRIVIAL] == 1 &&
                  stats->by_importance[WHY_ROUTINE] == 1 &&
                  stats->by_importance[WHY_INTERESTING] == 1 &&
                  stats->by_importance[WHY_SIGNIFICANT] == 1 &&
                  stats->by_importance[WHY_CRITICAL] == 1);

    free(stats);
    session_end();
    breathe_cleanup();

    report_test("Enhanced stats: importance distribution", passed);
}

static void test_stats_query_tracking(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Store some memories */
    remember(TEST_CI_ID, "Memory 1", WHY_SIGNIFICANT);
    remember(TEST_CI_ID, "Memory 2 about bugs", WHY_SIGNIFICANT);
    remember(TEST_CI_ID, "Memory 3 about bugs", WHY_SIGNIFICANT);

    /* Perform queries */
    size_t count = 0;

    char** relevant = relevant_memories(&count);
    free_memory_list(relevant, count);

    char** recent = recent_thoughts(TEST_CI_ID, 10, &count);
    free_memory_list(recent, count);

    char** about = recall_about(TEST_CI_ID, "bugs", &count);
    size_t match_count = count;
    free_memory_list(about, count);

    /* Get stats */
    enhanced_stats_t* stats = get_enhanced_statistics();

    int passed = (stats != NULL &&
                  stats->relevant_queries == 1 &&
                  stats->recent_queries == 1 &&
                  stats->topic_queries == 1 &&
                  stats->topic_matches == match_count);

    free(stats);
    session_end();
    breathe_cleanup();

    report_test("Enhanced stats: query tracking", passed);
}

static void test_stats_session_timing(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Get initial stats */
    enhanced_stats_t* stats1 = get_enhanced_statistics();
    time_t start_time = stats1->session_start_time;
    free(stats1);

    /* Do some work */
    sleep(1);
    remember(TEST_CI_ID, "Test", WHY_ROUTINE);

    /* Get stats again */
    enhanced_stats_t* stats2 = get_enhanced_statistics();

    int passed = (stats2 != NULL &&
                  stats2->session_start_time == start_time &&
                  stats2->last_activity_time > start_time &&
                  stats2->session_duration_seconds >= 1);

    free(stats2);
    session_end();
    breathe_cleanup();

    report_test("Enhanced stats: session timing", passed);
}

static void test_stats_reset_on_session_start(void) {
    setup_test();

    /* First session */
    session_start(TEST_CI_ID);
    remember(TEST_CI_ID, "Memory 1", WHY_ROUTINE);
    remember(TEST_CI_ID, "Memory 2", WHY_ROUTINE);
    session_end();

    /* Second session - stats should reset */
    session_start(TEST_CI_ID);
    remember(TEST_CI_ID, "Memory 3", WHY_ROUTINE);

    enhanced_stats_t* stats = get_enhanced_statistics();

    /* Should only count memory from current session */
    int passed = (stats != NULL &&
                  stats->total_memories_stored == 1);

    free(stats);
    session_end();
    breathe_cleanup();

    report_test("Enhanced stats: reset on session start", passed);
}

static void test_stats_context_loading(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Store significant memories */
    remember(TEST_CI_ID, "Important 1", WHY_SIGNIFICANT);
    remember(TEST_CI_ID, "Important 2", WHY_SIGNIFICANT);
    remember(TEST_CI_ID, "Important 3", WHY_SIGNIFICANT);

    /* Trigger context load */
    int result = load_context();

    /* Get stats */
    enhanced_stats_t* stats = get_enhanced_statistics();

    int passed = (result == KATRA_SUCCESS &&
                  stats != NULL &&
                  stats->context_loads >= 1);  /* At least 1 from load_context() */

    free(stats);
    session_end();
    breathe_cleanup();

    report_test("Enhanced stats: context loading", passed);
}

/* ============================================================================
 * INTEGRATION TESTS
 * ============================================================================ */

static void test_integration_semantic_with_config(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Set custom config */
    context_config_t custom = {
        .max_relevant_memories = 5,
        .max_recent_thoughts = 10,
        .max_topic_recall = 50,
        .min_importance_relevant = MEMORY_IMPORTANCE_MEDIUM,
        .max_context_age_days = 3
    };
    set_context_config(&custom);

    /* Store memories with semantic reasons */
    remember_semantic(TEST_CI_ID, "Discovery 1", "very important");
    remember_semantic(TEST_CI_ID, "Discovery 2", "critical");
    remember_semantic(TEST_CI_ID, "Discovery 3", "interesting");

    /* Query with configured limits */
    size_t count = 0;
    char** relevant = relevant_memories(&count);

    /* Should respect max_relevant_memories limit and importance threshold */
    int passed = (count <= 5);

    free_memory_list(relevant, count);
    session_end();
    breathe_cleanup();

    report_test("Integration: semantic + config", passed);
}

static void test_integration_stats_comprehensive(void) {
    setup_test();
    session_start(TEST_CI_ID);

    /* Perform variety of operations */
    remember(TEST_CI_ID, "Exp", WHY_ROUTINE);
    remember_semantic(TEST_CI_ID, "Sem", "important");
    remember_with_note(TEST_CI_ID, "Note", WHY_SIGNIFICANT, "reason");
    learn(TEST_CI_ID, "Learn");
    reflect(TEST_CI_ID, "Reflect");
    decide(TEST_CI_ID, "Decide", "why");
    notice_pattern(TEST_CI_ID, "Pattern");

    size_t count = 0;
    char** thoughts = recent_thoughts(TEST_CI_ID, 5, &count);
    free_memory_list(thoughts, count);

    char** relevant = relevant_memories(&count);
    free_memory_list(relevant, count);

    /* Get comprehensive stats */
    enhanced_stats_t* stats = get_enhanced_statistics();

    int passed = (stats != NULL &&
                  stats->total_memories_stored == 7 &&
                  stats->semantic_remember_count == 1 &&
                  stats->recent_queries == 1 &&
                  stats->relevant_queries == 1 &&
                  stats->last_activity_time > 0);

    free(stats);
    session_end();
    breathe_cleanup();

    report_test("Integration: comprehensive stats", passed);
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("Katra Phase 2 Breathing Layer Tests\n");
    printf("=================================================================\n");
    printf("\n");

    /* Semantic Reason Parsing Tests */
    printf("Semantic Reason Parsing:\n");
    test_semantic_trivial_parsing();
    test_semantic_routine_parsing();
    test_semantic_critical_parsing();
    test_semantic_significant_parsing();
    test_semantic_interesting_parsing();
    test_semantic_case_insensitive();
    test_semantic_default();
    test_remember_semantic();
    test_remember_with_semantic_note();

    /* Context Configuration Tests */
    printf("\nContext Configuration:\n");
    test_context_config_defaults();
    test_context_config_set();
    test_context_config_validation();
    test_context_config_reset();

    /* Enhanced Statistics Tests */
    printf("\nEnhanced Statistics:\n");
    test_stats_memory_formation();
    test_stats_importance_distribution();
    test_stats_query_tracking();
    test_stats_session_timing();
    test_stats_reset_on_session_start();
    test_stats_context_loading();

    /* Integration Tests */
    printf("\nIntegration Tests:\n");
    test_integration_semantic_with_config();
    test_integration_stats_comprehensive();

    /* Summary */
    printf("\n");
    printf("=================================================================\n");
    printf("Test Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(" (%d FAILED)", tests_failed);
    }
    printf("\n");
    printf("=================================================================\n");
    printf("\n");

    return (tests_failed == 0) ? 0 : 1;
}
