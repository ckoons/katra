/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Project includes */
#include "katra_sunrise_sunset.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_init.h"

/* Test counters */
static int tests_run = 0;
static int tests_passed = 0;

/* Test helper macros */
#define TEST(name) \
    printf("Testing: " name " ... "); \
    tests_run++;

#define PASS() \
    printf(" ✓\n"); \
    tests_passed++;

#define FAIL(msg) \
    printf(" ✗\n"); \
    printf("  FAILED: %s\n", msg); \
    return;

/* Test: Extract topic clusters */
static void test_extract_topics(void) {
    TEST("Extract topic clusters from memories");

    /* Initialize system */
    katra_init();

    vector_store_t* vectors = katra_vector_init("test_ci", false);
    if (!vectors) {
        FAIL("Failed to initialize vector store");
    }

    /* Add some test vectors */
    katra_vector_store(vectors, "mem1", "machine learning and AI");
    katra_vector_store(vectors, "mem2", "deep neural networks");
    katra_vector_store(vectors, "mem3", "cooking pasta recipes");
    katra_vector_store(vectors, "mem4", "artificial intelligence");

    /* Extract topics (will be empty without actual memory records) */
    topic_cluster_t** clusters = NULL;
    size_t count = 0;
    int result = katra_extract_topics("test_ci", vectors, &clusters, &count);

    if (result != KATRA_SUCCESS) {
        FAIL("Failed to extract topics");
    }

    /* For test CI without memories, count will be 0 - this is expected */
    /* The function works correctly, just no data to cluster */

    katra_topics_free(clusters, count);
    katra_vector_cleanup(vectors);
    PASS();
}

/* Test: Trace conversation threads */
static void test_trace_threads(void) {
    TEST("Trace conversation threads");

    graph_store_t* graph = katra_graph_init("test_ci");
    if (!graph) {
        FAIL("Failed to initialize graph");
    }

    /* Build a conversation thread */
    katra_graph_add_edge(graph, "mem1", "mem2", REL_SEQUENTIAL, NULL, 0.8f);
    katra_graph_add_edge(graph, "mem2", "mem3", REL_SEQUENTIAL, NULL, 0.9f);
    katra_graph_add_edge(graph, "mem3", "mem4", REL_SEQUENTIAL, NULL, 0.7f);

    /* Trace threads */
    conversation_thread_t** threads = NULL;
    size_t count = 0;
    int result = katra_trace_threads("test_ci", graph, &threads, &count);

    if (result != KATRA_SUCCESS) {
        FAIL("Failed to trace threads");
    }

    katra_threads_free(threads, count);
    katra_graph_cleanup(graph);
    PASS();
}

/* Test: Build emotional arc */
static void test_emotional_arc(void) {
    TEST("Build emotional arc");

    emotional_tag_t* arc = NULL;
    size_t count = 0;
    int result = katra_build_emotional_arc("test_ci", &arc, &count);

    if (result != KATRA_SUCCESS) {
        FAIL("Failed to build emotional arc");
    }

    /* Arc should be empty for non-existent memories */
    if (count != 0) {
        free(arc);
        FAIL("Expected empty arc for test CI");
    }

    PASS();
}

/* Test: Detect insights */
static void test_detect_insights(void) {
    TEST("Detect insights from patterns");

    /* Create mock topics and threads */
    topic_cluster_t* topic1 = calloc(1, sizeof(topic_cluster_t));
    topic_cluster_t* topic2 = calloc(1, sizeof(topic_cluster_t));
    topic_cluster_t* topics[] = {topic1, topic2};

    conversation_thread_t* thread1 = calloc(1, sizeof(conversation_thread_t));
    conversation_thread_t* threads[] = {thread1};

    daily_insight_t** insights = NULL;
    size_t count = 0;
    int result = katra_detect_insights("test_ci", topics, 2,
                                       threads, 1,
                                       &insights, &count);

    if (result != KATRA_SUCCESS) {
        FAIL("Failed to detect insights");
    }

    /* Should have detected some insights */
    if (count == 0) {
        FAIL("Expected at least one insight");
    }

    katra_insights_free(insights, count);
    free(topic1);
    free(topic2);
    free(thread1);
    PASS();
}

/* Test: Enhanced sundown */
static void test_sundown(void) {
    TEST("Enhanced sundown protocol");

    vector_store_t* vectors = katra_vector_init("test_ci", false);
    graph_store_t* graph = katra_graph_init("test_ci");

    if (!vectors || !graph) {
        FAIL("Failed to initialize stores");
    }

    sundown_context_t* context = NULL;
    int result = katra_sundown("test_ci", vectors, graph, &context);

    if (result != KATRA_SUCCESS) {
        FAIL("Sundown failed");
    }

    if (!context) {
        FAIL("Context is NULL");
    }

    if (strcmp(context->ci_id, "test_ci") != 0) {
        FAIL("Wrong CI ID in context");
    }

    katra_sundown_free(context);
    katra_vector_cleanup(vectors);
    katra_graph_cleanup(graph);
    PASS();
}

/* Test: Enhanced sunrise */
static void test_sunrise(void) {
    TEST("Enhanced sunrise protocol");

    vector_store_t* vectors = katra_vector_init("test_ci", false);
    graph_store_t* graph = katra_graph_init("test_ci");

    if (!vectors || !graph) {
        FAIL("Failed to initialize stores");
    }

    sunrise_context_t* context = NULL;
    int result = katra_sunrise("test_ci", vectors, graph, &context);

    if (result != KATRA_SUCCESS) {
        FAIL("Sunrise failed");
    }

    if (!context) {
        FAIL("Context is NULL");
    }

    if (strcmp(context->ci_id, "test_ci") != 0) {
        FAIL("Wrong CI ID in context");
    }

    /* Baseline mood should be neutral */
    if (strcmp(context->baseline_mood.emotion, EMOTION_NEUTRAL) != 0) {
        FAIL("Expected neutral baseline mood");
    }

    katra_sunrise_free(context);
    katra_vector_cleanup(vectors);
    katra_graph_cleanup(graph);
    PASS();
}

/* Test: Sundown → Sunrise workflow */
static void test_full_workflow(void) {
    TEST("Full sundown → sunrise workflow");

    vector_store_t* vectors = katra_vector_init("test_ci", false);
    graph_store_t* graph = katra_graph_init("test_ci");

    if (!vectors || !graph) {
        FAIL("Failed to initialize stores");
    }

    /* Add some test data */
    katra_vector_store(vectors, "mem1", "test memory 1");
    katra_vector_store(vectors, "mem2", "test memory 2");
    katra_graph_add_edge(graph, "mem1", "mem2", REL_SEQUENTIAL, NULL, 0.8f);

    /* Run sundown */
    sundown_context_t* sundown_ctx = NULL;
    int result = katra_sundown("test_ci", vectors, graph, &sundown_ctx);

    if (result != KATRA_SUCCESS) {
        FAIL("Sundown failed");
    }

    /* Run sunrise */
    sunrise_context_t* sunrise_ctx = NULL;
    result = katra_sunrise("test_ci", vectors, graph, &sunrise_ctx);

    if (result != KATRA_SUCCESS) {
        FAIL("Sunrise failed");
    }

    katra_sundown_free(sundown_ctx);
    katra_sunrise_free(sunrise_ctx);
    katra_vector_cleanup(vectors);
    katra_graph_cleanup(graph);
    PASS();
}

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Sunrise/Sunset Tests\n");
    printf("========================================\n");
    printf("\n");

    /* Run tests */
    test_extract_topics();
    test_trace_threads();
    test_emotional_arc();
    test_detect_insights();
    test_sundown();
    test_sunrise();
    test_full_workflow();

    /* Print results */
    printf("\n");
    printf("========================================\n");
    printf("Test Results:\n");
    printf("  Tests run:    %d\n", tests_run);
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_run - tests_passed);
    printf("========================================\n");
    printf("\n");

    return (tests_run == tests_passed) ? 0 : 1;
}
