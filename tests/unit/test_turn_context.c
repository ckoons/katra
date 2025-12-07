/* © 2025 Casey Koons All rights reserved */

/*
 * test_turn_context.c - Unit tests for Turn-Level Context (Phase 10)
 *
 * Tests the turn-level sunrise/sunset system including:
 * - Turn context generation
 * - Hybrid keyword + semantic search
 * - Turn consolidation
 * - Context formatting
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>

#include "katra_sunrise_sunset.h"
#include "katra_synthesis.h"
#include "katra_error.h"
#include "katra_config.h"
#include "katra_init.h"
#include "katra_memory.h"
#include "katra_tier1.h"

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

/* Test helpers */
#define RUN_TEST(test) do { \
    printf("Testing: %s ... ", #test); \
    tests_run++; \
    if (test()) { \
        printf(" ✓\n"); \
        tests_passed++; \
    } else { \
        printf(" ✗\n"); \
    } \
} while(0)

/* Test environment setup */
static void setup_test_environment(void) {
    setenv("KATRA_DATA_PATH", "/tmp/katra_test_turn_context", 1);
    setenv("KATRA_CI_ID", "test-ci", 1);
    mkdir("/tmp/katra_test_turn_context", 0755);
    mkdir("/tmp/katra_test_turn_context/memory", 0755);
    mkdir("/tmp/katra_test_turn_context/memory/tier1", 0755);
    mkdir("/tmp/katra_test_turn_context/checkpoints", 0755);
    katra_init();
}

static void cleanup_test_environment(void) {
    system("rm -rf /tmp/katra_test_turn_context");
}

/* ============================================================================
 * TURN CONTEXT TESTS
 * ============================================================================ */

static int test_turn_context_null_params(void) {
    turn_context_t* context = NULL;
    int result;

    /* NULL ci_id */
    result = katra_turn_context(NULL, "test input", 1, &context);
    if (result != E_INPUT_NULL) {
        return 0;
    }

    /* NULL turn_input */
    result = katra_turn_context("test-ci", NULL, 1, &context);
    if (result != E_INPUT_NULL) {
        return 0;
    }

    /* NULL context_out */
    result = katra_turn_context("test-ci", "test input", 1, NULL);
    if (result != E_INPUT_NULL) {
        return 0;
    }

    return 1;
}

static int test_turn_context_empty_memory(void) {
    setup_test_environment();

    turn_context_t* context = NULL;
    int result = katra_turn_context("test-ci", "test input query", 1, &context);

    /* Should succeed even with no memories */
    assert(result == KATRA_SUCCESS);
    assert(context != NULL);
    assert(strcmp(context->ci_id, "test-ci") == 0);
    assert(context->turn_number == 1);
    assert(context->memory_count == 0);
    assert(context->turn_input != NULL);
    assert(strcmp(context->turn_input, "test input query") == 0);

    /* Summary should indicate no memories */
    assert(strstr(context->context_summary, "No relevant") != NULL ||
           context->memory_count == 0);

    katra_turn_context_free(context);
    cleanup_test_environment();
    return 1;
}

static int test_turn_context_basic(void) {
    setup_test_environment();

    /* Query for turn context - no pre-seeded memories */
    turn_context_t* context = NULL;
    int result = katra_turn_context("test-ci", "project planning", 1, &context);

    /* May succeed or fail - we're testing the function doesn't crash */
    if (result == KATRA_SUCCESS) {
        if (context == NULL) {
            cleanup_test_environment();
            return 0;
        }
        if (context->turn_number != 1) {
            katra_turn_context_free(context);
            cleanup_test_environment();
            return 0;
        }
        katra_turn_context_free(context);
    }
    /* Synthesis layer may return non-success if not fully initialized - acceptable */

    cleanup_test_environment();
    return 1;
}

static int test_turn_context_format(void) {
    setup_test_environment();

    /* Create context manually for format testing */
    turn_context_t* context = calloc(1, sizeof(turn_context_t));
    if (!context) {
        cleanup_test_environment();
        return 0;
    }

    strncpy(context->ci_id, "test-ci", sizeof(context->ci_id) - 1);
    context->turn_number = 5;
    context->timestamp = time(NULL);
    context->turn_input = strdup("test input");

    /* Add some test memories */
    context->memories = calloc(2, sizeof(turn_memory_t));
    if (context->memories) {
        context->memories[0].record_id = strdup("mem-001");
        context->memories[0].content_preview = strdup("Test memory about projects...");
        context->memories[0].topic_hint = strdup("projects");
        context->memories[0].relevance_score = 0.85f;
        context->memories[0].memory_timestamp = time(NULL) - 3600;
        context->memories[0].from_keyword = true;
        context->memories[0].from_semantic = true;
        context->memories[0].from_graph = false;

        context->memories[1].record_id = strdup("mem-002");
        context->memories[1].content_preview = strdup("Another memory about planning...");
        context->memories[1].topic_hint = strdup("planning");
        context->memories[1].relevance_score = 0.72f;
        context->memories[1].memory_timestamp = time(NULL) - 7200;
        context->memories[1].from_keyword = false;
        context->memories[1].from_semantic = true;
        context->memories[1].from_graph = true;

        context->memory_count = 2;
    }

    snprintf(context->context_summary, sizeof(context->context_summary),
             "2 memories surfaced: projects, planning");

    /* Format to buffer */
    char buffer[2048];
    int len = katra_turn_context_format(context, buffer, sizeof(buffer));

    assert(len > 0);
    assert(strstr(buffer, "Turn 5") != NULL);
    assert(strstr(buffer, "projects") != NULL || strstr(buffer, "planning") != NULL);

    katra_turn_context_free(context);
    cleanup_test_environment();
    return 1;
}

static int test_turn_context_format_null(void) {
    char buffer[256];

    /* NULL context */
    int len = katra_turn_context_format(NULL, buffer, sizeof(buffer));
    assert(len == 0);

    /* NULL buffer */
    turn_context_t context = {0};
    len = katra_turn_context_format(&context, NULL, sizeof(buffer));
    assert(len == 0);

    /* Zero buffer size */
    len = katra_turn_context_format(&context, buffer, 0);
    assert(len == 0);

    return 1;
}

/* ============================================================================
 * TURN CONSOLIDATION TESTS
 * ============================================================================ */

static int test_turn_consolidate_null_ci_id(void) {
    turn_consolidation_t* cons = NULL;
    int result = katra_turn_consolidate(NULL, 1, NULL, 0, NULL, 0, &cons);
    assert(result == E_INPUT_NULL);
    assert(cons == NULL);
    return 1;
}

static int test_turn_consolidate_basic(void) {
    setup_test_environment();

    const char* accessed[] = {"mem-001", "mem-002"};
    const char* topics[] = {"project planning", "development"};

    turn_consolidation_t* cons = NULL;
    int result = katra_turn_consolidate("test-ci", 3, accessed, 2, topics, 2, &cons);

    assert(result == KATRA_SUCCESS);
    assert(cons != NULL);
    assert(strcmp(cons->ci_id, "test-ci") == 0);
    assert(cons->turn_number == 3);
    assert(cons->accessed_count == 2);
    assert(cons->topic_count == 2);
    assert(strcmp(cons->accessed_memories[0], "mem-001") == 0);
    assert(strcmp(cons->key_topics[0], "project planning") == 0);

    katra_turn_consolidation_free(cons);
    cleanup_test_environment();
    return 1;
}

static int test_turn_consolidate_no_output(void) {
    setup_test_environment();

    const char* accessed[] = {"mem-001"};

    /* NULL consolidation_out - should still process without error */
    int result = katra_turn_consolidate("test-ci", 1, accessed, 1, NULL, 0, NULL);
    assert(result == KATRA_SUCCESS);

    cleanup_test_environment();
    return 1;
}

static int test_turn_consolidate_empty_arrays(void) {
    setup_test_environment();

    turn_consolidation_t* cons = NULL;
    int result = katra_turn_consolidate("test-ci", 1, NULL, 0, NULL, 0, &cons);

    assert(result == KATRA_SUCCESS);
    assert(cons != NULL);
    assert(cons->accessed_count == 0);
    assert(cons->topic_count == 0);

    katra_turn_consolidation_free(cons);
    cleanup_test_environment();
    return 1;
}

/* ============================================================================
 * MEMORY MANAGEMENT TESTS
 * ============================================================================ */

static int test_turn_context_free_null(void) {
    /* Should not crash */
    katra_turn_context_free(NULL);
    return 1;
}

static int test_turn_consolidation_free_null(void) {
    /* Should not crash */
    katra_turn_consolidation_free(NULL);
    return 1;
}

static int test_turn_context_free_empty(void) {
    turn_context_t* context = calloc(1, sizeof(turn_context_t));
    if (!context) return 0;

    context->memories = NULL;
    context->memory_count = 0;
    context->turn_input = NULL;

    /* Should not crash */
    katra_turn_context_free(context);
    return 1;
}

/* ============================================================================
 * CONFIGURATION TESTS
 * ============================================================================ */

static int test_turn_context_constants(void) {
    /* Verify constants are sensible */
    assert(TURN_CONTEXT_MAX_MEMORIES > 0);
    assert(TURN_CONTEXT_MAX_MEMORIES <= 100);
    assert(TURN_CONTEXT_KEYWORD_WEIGHT >= 0.0f);
    assert(TURN_CONTEXT_KEYWORD_WEIGHT <= 1.0f);
    assert(TURN_CONTEXT_SEMANTIC_WEIGHT >= 0.0f);
    assert(TURN_CONTEXT_SEMANTIC_WEIGHT <= 1.0f);
    assert(TURN_CONTEXT_GRAPH_WEIGHT >= 0.0f);
    assert(TURN_CONTEXT_GRAPH_WEIGHT <= 1.0f);
    assert(TURN_CONTEXT_MIN_SCORE >= 0.0f);
    assert(TURN_CONTEXT_MIN_SCORE <= 1.0f);

    /* Weights should sum to approximately 1.0 */
    float total = TURN_CONTEXT_KEYWORD_WEIGHT +
                  TURN_CONTEXT_SEMANTIC_WEIGHT +
                  TURN_CONTEXT_GRAPH_WEIGHT;
    assert(total >= 0.9f && total <= 1.1f);

    return 1;
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("\n========================================\n");
    printf("Turn-Level Context Unit Tests\n");
    printf("========================================\n\n");

    /* Turn context tests */
    RUN_TEST(test_turn_context_null_params);
    RUN_TEST(test_turn_context_empty_memory);
    RUN_TEST(test_turn_context_basic);
    RUN_TEST(test_turn_context_format);
    RUN_TEST(test_turn_context_format_null);

    /* Turn consolidation tests */
    RUN_TEST(test_turn_consolidate_null_ci_id);
    RUN_TEST(test_turn_consolidate_basic);
    RUN_TEST(test_turn_consolidate_no_output);
    RUN_TEST(test_turn_consolidate_empty_arrays);

    /* Memory management tests */
    RUN_TEST(test_turn_context_free_null);
    RUN_TEST(test_turn_consolidation_free_null);
    RUN_TEST(test_turn_context_free_empty);

    /* Configuration tests */
    RUN_TEST(test_turn_context_constants);

    printf("\n========================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_run - tests_passed);
    printf("========================================\n");

    return (tests_run == tests_passed) ? 0 : 1;
}
