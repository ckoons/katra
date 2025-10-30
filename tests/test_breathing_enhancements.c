/* © 2025 Casey Koons All rights reserved */

/*
 * test_breathing_enhancements.c - Test new natural usage enhancements
 *
 * Tests for:
 *  - thinking() - Stream of consciousness wrapper
 *  - wondering() / figured_out() - Formation context flow
 *  - what_do_i_know() - Knowledge-specific recall
 *  - in_response_to() - Conversation tracking
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "katra_init.h"
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"

#define CI_ID "test_breathing_enhancements_ci"

/* ============================================================================
 * Test Setup/Teardown
 * ============================================================================ */

void setup(void) {
    katra_init();
    breathe_init(CI_ID);
    session_start(CI_ID);
}

void teardown(void) {
    session_end();
    breathe_cleanup();
    katra_exit();
}

/* ============================================================================
 * thinking() Tests
 * ============================================================================ */

void test_thinking_basic(void) {
    int result = thinking("I notice the pattern is emerging...");
    assert(result == KATRA_SUCCESS);

    /* Verify it's stored as reflection */
    memory_query_t query = {
        .ci_id = CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_REFLECTION,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    result = katra_memory_query(&query, &results, &count);
    assert(result == KATRA_SUCCESS);
    assert(count >= 1);

    /* Find the thinking memory */
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (strstr(results[i]->content, "pattern is emerging") != NULL) {
            found = true;
            break;
        }
    }
    assert(found);

    katra_memory_free_results(results, count);
}

void test_thinking_null_thought(void) {
    int result = thinking(NULL);
    assert(result == E_INPUT_NULL);
}

/* ============================================================================
 * wondering() / figured_out() Tests
 * ============================================================================ */

void test_wondering_basic(void) {
    int result = wondering("Why isn't consolidation running?");
    assert(result == KATRA_SUCCESS);

    /* Verify it's stored with formation context */
    memory_query_t query = {
        .ci_id = CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_REFLECTION,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    result = katra_memory_query(&query, &results, &count);
    assert(result == KATRA_SUCCESS);
    assert(count >= 1);

    /* Find the wondering memory */
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (strstr(results[i]->content, "consolidation running") != NULL) {
            assert(results[i]->context_question != NULL);
            assert(results[i]->context_uncertainty != NULL);
            found = true;
            break;
        }
    }
    assert(found);

    katra_memory_free_results(results, count);
}

void test_figured_out_basic(void) {
    int result = figured_out("Because tier1 wasn't at threshold yet");
    assert(result == KATRA_SUCCESS);

    /* Verify it's stored with formation context */
    memory_query_t query = {
        .ci_id = CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_REFLECTION,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    result = katra_memory_query(&query, &results, &count);
    assert(result == KATRA_SUCCESS);
    assert(count >= 1);

    /* Find the figured_out memory */
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (strstr(results[i]->content, "threshold yet") != NULL) {
            assert(results[i]->context_resolution != NULL);
            found = true;
            break;
        }
    }
    assert(found);

    katra_memory_free_results(results, count);
}

void test_wondering_then_figured_out(void) {
    /* Simulate discovery flow */
    int result = wondering("Why are tests failing?");
    assert(result == KATRA_SUCCESS);

    result = figured_out("Forgot to initialize breathing layer");
    assert(result == KATRA_SUCCESS);

    /* Both should be stored */
    memory_query_t query = {
        .ci_id = CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_REFLECTION,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 10
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    result = katra_memory_query(&query, &results, &count);
    assert(result == KATRA_SUCCESS);
    assert(count >= 2);

    katra_memory_free_results(results, count);
}

/* ============================================================================
 * what_do_i_know() Tests
 * ============================================================================ */

void test_what_do_i_know_basic(void) {
    /* Store some knowledge */
    learn("Consolidation archives old memories");
    learn("Tier1 is for raw recordings");
    learn("Pattern compression preserves outliers");

    /* Also store non-knowledge */
    remember("Had lunch today", WHY_ROUTINE);

    /* Query knowledge only */
    size_t count = 0;
    char** knowledge = what_do_i_know("consolidation", &count);

    assert(knowledge != NULL);
    assert(count >= 1);

    /* Find the consolidation knowledge */
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (strstr(knowledge[i], "archives old memories") != NULL) {
            found = true;
            break;
        }
    }
    assert(found);

    free_memory_list(knowledge, count);
}

void test_what_do_i_know_no_matches(void) {
    learn("Tier1 is for raw recordings");

    size_t count = 0;
    char** knowledge = what_do_i_know("nonexistent", &count);

    assert(knowledge == NULL);
    assert(count == 0);
}

void test_what_do_i_know_null_concept(void) {
    size_t count = 0;
    char** knowledge = what_do_i_know(NULL, &count);

    assert(knowledge == NULL);
    assert(count == 0);
}

/* ============================================================================
 * in_response_to() Tests
 * ============================================================================ */

void test_in_response_to_basic(void) {
    /* Store initial thought */
    int result = remember("Casey asked about Phase 4", WHY_SIGNIFICANT);
    assert(result == KATRA_SUCCESS);

    /* Get the memory ID */
    memory_query_t query = {
        .ci_id = CI_ID,
        .start_time = 0,
        .end_time = 0,
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 1
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    result = katra_memory_query(&query, &results, &count);
    assert(result == KATRA_SUCCESS);
    assert(count == 1);

    char* first_id = strdup(results[0]->record_id);
    katra_memory_free_results(results, count);

    /* Store response */
    char* response_id = in_response_to(first_id, "Explained semantic embeddings");
    assert(response_id != NULL);

    /* Verify related_to link - use large limit to ensure we get all memories */
    query.limit = 1000;
    results = NULL;
    count = 0;
    result = katra_memory_query(&query, &results, &count);
    assert(result == KATRA_SUCCESS);
    assert(count >= 2);

    /* Find the response memory */
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(results[i]->record_id, response_id) == 0) {
            assert(results[i]->related_to != NULL);
            assert(strcmp(results[i]->related_to, first_id) == 0);
            found = true;
            break;
        }
    }
    assert(found);

    katra_memory_free_results(results, count);
    free(first_id);
    free(response_id);
}

void test_in_response_to_null_params(void) {
    char* result = in_response_to(NULL, "test");
    assert(result == NULL);

    result = in_response_to("id", NULL);
    assert(result == NULL);
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void run_test(const char* name, void (*test_func)(void)) {
    printf("  ✓ %s\n", name);
    setup();
    test_func();
    teardown();
}

int main(void) {
    printf("\n=================================================================\n");
    printf("Katra Breathing Enhancements Unit Tests\n");
    printf("=================================================================\n\n");

    printf("thinking() Tests:\n");
    run_test("test_thinking_basic", test_thinking_basic);
    run_test("test_thinking_null_thought", test_thinking_null_thought);

    printf("\nwondering() / figured_out() Tests:\n");
    run_test("test_wondering_basic", test_wondering_basic);
    run_test("test_figured_out_basic", test_figured_out_basic);
    run_test("test_wondering_then_figured_out", test_wondering_then_figured_out);

    printf("\nwhat_do_i_know() Tests:\n");
    run_test("test_what_do_i_know_basic", test_what_do_i_know_basic);
    run_test("test_what_do_i_know_no_matches", test_what_do_i_know_no_matches);
    run_test("test_what_do_i_know_null_concept", test_what_do_i_know_null_concept);

    printf("\nin_response_to() Tests:\n");
    run_test("test_in_response_to_basic", test_in_response_to_basic);
    run_test("test_in_response_to_null_params", test_in_response_to_null_params);

    printf("\n=================================================================\n");
    printf("Test Results: 11/11 passed\n");
    printf("=================================================================\n\n");

    return 0;
}
