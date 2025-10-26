/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Project includes */
#include "katra_vector.h"
#include "katra_error.h"
#include "katra_log.h"

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

/* Test: Initialize vector store */
static void test_vector_init(void) {
    TEST("Vector store initialization");

    vector_store_t* store = katra_vector_init("test_ci", false);
    if (!store) {
        FAIL("Failed to initialize vector store");
    }

    if (strcmp(store->ci_id, "test_ci") != 0) {
        FAIL("CI ID not set correctly");
    }

    if (store->count != 0) {
        FAIL("Initial count should be 0");
    }

    katra_vector_cleanup(store);
    PASS();
}

/* Test: Create embedding */
static void test_create_embedding(void) {
    TEST("Create embedding from text");

    vector_embedding_t* embedding = NULL;
    int result = katra_vector_create_embedding("hello world", &embedding);

    if (result != KATRA_SUCCESS) {
        FAIL("Failed to create embedding");
    }

    if (!embedding) {
        FAIL("Embedding is NULL");
    }

    if (embedding->dimensions != VECTOR_DIMENSIONS) {
        FAIL("Wrong number of dimensions");
    }

    if (embedding->magnitude == 0.0f) {
        FAIL("Magnitude should not be zero");
    }

    katra_vector_free_embedding(embedding);
    PASS();
}

/* Test: Store and retrieve embeddings */
static void test_store_retrieve(void) {
    TEST("Store and retrieve embeddings");

    vector_store_t* store = katra_vector_init("test_ci", false);
    if (!store) {
        FAIL("Failed to initialize store");
    }

    /* Store embeddings */
    int result1 = katra_vector_store(store, "rec1", "The quick brown fox");
    int result2 = katra_vector_store(store, "rec2", "The lazy dog");

    if (result1 != KATRA_SUCCESS || result2 != KATRA_SUCCESS) {
        FAIL("Failed to store embeddings");
    }

    if (store->count != 2) {
        FAIL("Store count should be 2");
    }

    /* Retrieve embedding */
    vector_embedding_t* emb = katra_vector_get(store, "rec1");
    if (!emb) {
        FAIL("Failed to retrieve embedding");
    }

    if (strcmp(emb->record_id, "rec1") != 0) {
        FAIL("Wrong record ID");
    }

    katra_vector_cleanup(store);
    PASS();
}

/* Test: Cosine similarity */
static void test_cosine_similarity(void) {
    TEST("Cosine similarity calculation");

    vector_embedding_t* emb1 = NULL;
    vector_embedding_t* emb2 = NULL;
    vector_embedding_t* emb3 = NULL;

    katra_vector_create_embedding("hello world", &emb1);
    katra_vector_create_embedding("hello world", &emb2);  /* Same text */
    katra_vector_create_embedding("goodbye mars", &emb3);  /* Different text */

    if (!emb1 || !emb2 || !emb3) {
        FAIL("Failed to create embeddings");
    }

    /* Same text should have high similarity */
    float sim1 = katra_vector_cosine_similarity(emb1, emb2);
    if (sim1 < 0.99f) {  /* Should be ~1.0 */
        FAIL("Same text should have similarity near 1.0");
    }

    /* Different text should have lower similarity */
    float sim2 = katra_vector_cosine_similarity(emb1, emb3);
    if (sim2 >= sim1) {
        FAIL("Different text should have lower similarity");
    }

    katra_vector_free_embedding(emb1);
    katra_vector_free_embedding(emb2);
    katra_vector_free_embedding(emb3);
    PASS();
}

/* Test: Vector search */
static void test_vector_search(void) {
    TEST("Vector similarity search");

    vector_store_t* store = katra_vector_init("test_ci", false);
    if (!store) {
        FAIL("Failed to initialize store");
    }

    /* Store test data */
    katra_vector_store(store, "rec1", "machine learning algorithms");
    katra_vector_store(store, "rec2", "deep neural networks");
    katra_vector_store(store, "rec3", "cooking pasta recipes");
    katra_vector_store(store, "rec4", "artificial intelligence");

    /* Search for similar to ML topics */
    vector_match_t** matches = NULL;
    size_t count = 0;
    int result = katra_vector_search(store, "AI and machine learning", 3,
                                      &matches, &count);

    if (result != KATRA_SUCCESS) {
        FAIL("Search failed");
    }

    if (count != 3) {
        FAIL("Expected 3 matches");
    }

    /* Results should be ordered by similarity */
    if (matches[0]->similarity < matches[1]->similarity) {
        FAIL("Results not sorted by similarity");
    }

    katra_vector_free_matches(matches, count);
    katra_vector_cleanup(store);
    PASS();
}

/* Test: Delete embedding */
static void test_delete_embedding(void) {
    TEST("Delete embedding");

    vector_store_t* store = katra_vector_init("test_ci", false);
    if (!store) {
        FAIL("Failed to initialize store");
    }

    katra_vector_store(store, "rec1", "test1");
    katra_vector_store(store, "rec2", "test2");
    katra_vector_store(store, "rec3", "test3");

    if (store->count != 3) {
        FAIL("Should have 3 embeddings");
    }

    /* Delete middle embedding */
    int result = katra_vector_delete(store, "rec2");
    if (result != KATRA_SUCCESS) {
        FAIL("Delete failed");
    }

    if (store->count != 2) {
        FAIL("Count should be 2 after delete");
    }

    /* Should not be able to retrieve deleted embedding */
    vector_embedding_t* emb = katra_vector_get(store, "rec2");
    if (emb != NULL) {
        FAIL("Deleted embedding still retrievable");
    }

    katra_vector_cleanup(store);
    PASS();
}

/* Test: Expand capacity */
static void test_expand_capacity(void) {
    TEST("Expand vector store capacity");

    vector_store_t* store = katra_vector_init("test_ci", false);
    if (!store) {
        FAIL("Failed to initialize store");
    }

    /* Store many embeddings to trigger expansion */
    for (int i = 0; i < 150; i++) {
        char id[64];
        char text[128];
        snprintf(id, sizeof(id), "rec%d", i);
        snprintf(text, sizeof(text), "test document number %d", i);

        int result = katra_vector_store(store, id, text);
        if (result != KATRA_SUCCESS) {
            FAIL("Failed to store embedding during expansion");
        }
    }

    if (store->count != 150) {
        FAIL("Should have 150 embeddings");
    }

    if (store->capacity < 150) {
        FAIL("Capacity should have expanded");
    }

    katra_vector_cleanup(store);
    PASS();
}

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Vector Database Tests\n");
    printf("========================================\n");
    printf("\n");

    /* Run tests */
    test_vector_init();
    test_create_embedding();
    test_store_retrieve();
    test_cosine_similarity();
    test_vector_search();
    test_delete_embedding();
    test_expand_capacity();

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
