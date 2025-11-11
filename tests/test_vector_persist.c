/* © 2025 Casey Koons All rights reserved */

/* Tests for vector persistence layer (Phase 6.1d) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "katra_vector.h"
#include "katra_error.h"

#define TEST_CI_ID "test_vector_persist"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("Running: %s... ", name); \
        fflush(stdout);

#define PASS() \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

/* Test 1: Initialize persistence */
static void test_init_persistence(void) {
    TEST("test_init_persistence");

    int result = katra_vector_persist_init(TEST_CI_ID);
    assert(result == KATRA_SUCCESS);

    PASS();
}

/* Test 2: Save and load single embedding */
static void test_save_load_single(void) {
    TEST("test_save_load_single");

    /* Create test embedding */
    vector_embedding_t embedding;
    embedding.dimensions = VECTOR_DIMENSIONS;
    embedding.values = calloc(VECTOR_DIMENSIONS, sizeof(float));
    assert(embedding.values != NULL);

    strncpy(embedding.record_id, "test_rec_001", sizeof(embedding.record_id) - 1);

    /* Fill with test data */
    for (size_t i = 0; i < VECTOR_DIMENSIONS; i++) {
        embedding.values[i] = (float)i / (float)VECTOR_DIMENSIONS;
    }
    embedding.magnitude = 1.0f;

    /* Save embedding */
    int result = katra_vector_persist_save(TEST_CI_ID, &embedding);
    assert(result == KATRA_SUCCESS);

    /* Create vector store and load */
    vector_store_t* store = katra_vector_init(TEST_CI_ID, false);
    assert(store != NULL);

    /* Load should have loaded our saved embedding */
    assert(store->count >= 1);

    /* Find our embedding */
    vector_embedding_t* loaded = NULL;
    for (size_t i = 0; i < store->count; i++) {
        if (strcmp(store->embeddings[i]->record_id, "test_rec_001") == 0) {
            loaded = store->embeddings[i];
            break;
        }
    }

    assert(loaded != NULL);
    assert(loaded->dimensions == VECTOR_DIMENSIONS);
    assert(loaded->magnitude > 0.99f && loaded->magnitude < 1.01f);

    /* Verify first few values match */
    for (size_t i = 0; i < 10 && i < VECTOR_DIMENSIONS; i++) {
        float expected = (float)i / (float)VECTOR_DIMENSIONS;
        float diff = loaded->values[i] - expected;
        if (diff < 0) diff = -diff;
        assert(diff < 0.001f);
    }

    /* Cleanup */
    free(embedding.values);
    katra_vector_cleanup(store);

    PASS();
}

/* Test 3: Delete persisted embedding */
static void test_delete_persisted(void) {
    TEST("test_delete_persisted");

    /* Create and save embedding */
    vector_embedding_t embedding;
    embedding.dimensions = VECTOR_DIMENSIONS;
    embedding.values = calloc(VECTOR_DIMENSIONS, sizeof(float));
    assert(embedding.values != NULL);

    strncpy(embedding.record_id, "test_rec_delete", sizeof(embedding.record_id) - 1);
    embedding.magnitude = 1.0f;

    int result = katra_vector_persist_save(TEST_CI_ID, &embedding);
    assert(result == KATRA_SUCCESS);
    free(embedding.values);

    /* Delete it */
    result = katra_vector_persist_delete(TEST_CI_ID, "test_rec_delete");
    assert(result == KATRA_SUCCESS);

    /* Load and verify it's gone */
    vector_store_t* store = katra_vector_init(TEST_CI_ID, false);
    assert(store != NULL);

    /* Should not find deleted embedding */
    int found = 0;
    for (size_t i = 0; i < store->count; i++) {
        if (strcmp(store->embeddings[i]->record_id, "test_rec_delete") == 0) {
            found = 1;
            break;
        }
    }

    assert(found == 0);
    katra_vector_cleanup(store);

    PASS();
}

/* Test 4: Multiple embeddings persistence */
static void test_multiple_persist(void) {
    TEST("test_multiple_persist");

    /* Create vector store */
    vector_store_t* store = katra_vector_init(TEST_CI_ID, false);
    assert(store != NULL);

    /* Add multiple embeddings */
    const char* texts[] = {
        "First test document about machine learning",
        "Second test document about artificial intelligence",
        "Third test document about neural networks"
    };

    for (int i = 0; i < 3; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "multi_test_%d", i);

        int result = katra_vector_store(store, record_id, texts[i]);
        assert(result == KATRA_SUCCESS);
    }

    assert(store->count >= 3);
    katra_vector_cleanup(store);

    /* Reload and verify all are present */
    store = katra_vector_init(TEST_CI_ID, false);
    assert(store != NULL);

    int found_count = 0;
    for (int i = 0; i < 3; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "multi_test_%d", i);

        for (size_t j = 0; j < store->count; j++) {
            if (strcmp(store->embeddings[j]->record_id, record_id) == 0) {
                found_count++;
                break;
            }
        }
    }

    assert(found_count == 3);
    katra_vector_cleanup(store);

    PASS();
}

int main(void) {
    printf("\n");
    printf("=================================\n");
    printf("Vector Persistence Tests\n");
    printf("=================================\n\n");

    test_init_persistence();
    test_save_load_single();
    test_delete_persisted();
    test_multiple_persist();

    printf("\n");
    printf("=================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("=================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
