/* © 2025 Casey Koons All rights reserved */

/* Test HNSW indexing (Phase 6.1e) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "../include/katra_vector.h"
#include "../include/katra_error.h"
#include "../include/katra_log.h"

/* Test 1: Create and cleanup HNSW index */
static void test_hnsw_init_cleanup(void) {
    printf("\n=== Test 1: HNSW init and cleanup ===\n");

    hnsw_index_t* index = katra_vector_hnsw_init();
    assert(index != NULL);
    printf("✓ HNSW index initialized\n");

    /* Get stats for empty index */
    size_t nodes = 999;
    int max_layer = 999;
    size_t connections = 999;
    katra_vector_hnsw_stats(index, &nodes, &max_layer, &connections);
    assert(nodes == 0);
    assert(max_layer == 0);
    assert(connections == 0);
    printf("✓ Empty index has correct stats\n");

    katra_vector_hnsw_cleanup(index);
    printf("✓ Test 1 passed\n");
}

/* Test 2: Insert single embedding */
static void test_hnsw_single_insert(void) {
    printf("\n=== Test 2: Single embedding insert ===\n");

    hnsw_index_t* index = katra_vector_hnsw_init();
    assert(index != NULL);

    /* Create an embedding */
    vector_embedding_t* embedding = NULL;
    int result = katra_vector_create_embedding("test document", &embedding);
    assert(result == KATRA_SUCCESS);
    assert(embedding != NULL);

    /* Insert into index */
    result = katra_vector_hnsw_insert(index, 0, embedding);
    assert(result == KATRA_SUCCESS);
    printf("✓ Single embedding inserted\n");

    /* Check stats */
    size_t nodes = 0;
    katra_vector_hnsw_stats(index, &nodes, NULL, NULL);
    assert(nodes == 1);
    printf("✓ Index has 1 node\n");

    katra_vector_free_embedding(embedding);
    katra_vector_hnsw_cleanup(index);
    printf("✓ Test 2 passed\n");
}

/* Test 3: Insert multiple embeddings */
static void test_hnsw_multiple_inserts(void) {
    printf("\n=== Test 3: Multiple embedding inserts ===\n");

    hnsw_index_t* index = katra_vector_hnsw_init();
    assert(index != NULL);

    const char* docs[] = {
        "machine learning algorithms",
        "deep neural networks",
        "natural language processing",
        "computer vision systems",
        "artificial intelligence research"
    };
    size_t num_docs = sizeof(docs) / sizeof(docs[0]);

    vector_embedding_t* embeddings[5];

    /* Insert all embeddings */
    for (size_t i = 0; i < num_docs; i++) {
        int result = katra_vector_create_embedding(docs[i], &embeddings[i]);
        assert(result == KATRA_SUCCESS);

        result = katra_vector_hnsw_insert(index, i, embeddings[i]);
        assert(result == KATRA_SUCCESS);
    }
    printf("✓ Inserted %zu embeddings\n", num_docs);

    /* Check stats */
    size_t nodes = 0;
    int max_layer = 0;
    size_t connections = 0;
    katra_vector_hnsw_stats(index, &nodes, &max_layer, &connections);
    assert(nodes == num_docs);
    printf("✓ Index has %zu nodes, max layer %d, %zu connections\n",
           nodes, max_layer, connections);

    /* Cleanup */
    for (size_t i = 0; i < num_docs; i++) {
        katra_vector_free_embedding(embeddings[i]);
    }
    katra_vector_hnsw_cleanup(index);
    printf("✓ Test 3 passed\n");
}

/* Test 4: Search HNSW index */
static void test_hnsw_search(void) {
    printf("\n=== Test 4: HNSW search ===\n");

    hnsw_index_t* index = katra_vector_hnsw_init();
    assert(index != NULL);

    const char* docs[] = {
        "machine learning algorithms",
        "deep neural networks",
        "natural language processing",
        "computer vision systems",
        "artificial intelligence research"
    };
    size_t num_docs = sizeof(docs) / sizeof(docs[0]);

    vector_embedding_t* embeddings[5];

    /* Build index */
    for (size_t i = 0; i < num_docs; i++) {
        katra_vector_create_embedding(docs[i], &embeddings[i]);
        katra_vector_hnsw_insert(index, i, embeddings[i]);
    }
    printf("✓ Built index with %zu docs\n", num_docs);

    /* Create query embedding */
    vector_embedding_t* query = NULL;
    katra_vector_create_embedding("machine learning", &query);
    assert(query != NULL);

    /* Search */
    size_t* ids = NULL;
    float* distances = NULL;
    size_t count = 0;
    int result = katra_vector_hnsw_search(index, query, 3, &ids, &distances, &count);
    assert(result == KATRA_SUCCESS);
    assert(count > 0);
    assert(count <= 3);
    printf("✓ Search returned %zu results\n", count);

    /* Verify results */
    for (size_t i = 0; i < count; i++) {
        printf("  Result %zu: id=%zu, distance=%.3f\n", i, ids[i], distances[i]);
        assert(ids[i] < num_docs);
        assert(distances[i] >= 0.0f && distances[i] <= 2.0f);
    }
    printf("✓ All results valid\n");

    /* Cleanup */
    free(ids);
    free(distances);
    katra_vector_free_embedding(query);
    for (size_t i = 0; i < num_docs; i++) {
        katra_vector_free_embedding(embeddings[i]);
    }
    katra_vector_hnsw_cleanup(index);
    printf("✓ Test 4 passed\n");
}

/* Test 5: Build from vector store */
static void test_hnsw_build_from_store(void) {
    printf("\n=== Test 5: Build HNSW from vector store ===\n");

    /* Create vector store */
    vector_store_t* store = katra_vector_init("test_hnsw_ci", false);
    assert(store != NULL);

    /* Add documents */
    const char* docs[] = {
        "machine learning algorithms",
        "deep neural networks",
        "natural language processing",
        "computer vision systems",
        "artificial intelligence research"
    };
    size_t num_docs = sizeof(docs) / sizeof(docs[0]);

    for (size_t i = 0; i < num_docs; i++) {
        char rec_id[64];
        snprintf(rec_id, sizeof(rec_id), "doc_%zu", i);
        int result = katra_vector_store(store, rec_id, docs[i]);
        assert(result == KATRA_SUCCESS);
    }
    printf("✓ Created store with %zu docs\n", store->count);

    /* Build HNSW index from store */
    hnsw_index_t* index = NULL;
    int result = katra_vector_hnsw_build(store, &index);
    assert(result == KATRA_SUCCESS);
    assert(index != NULL);
    printf("✓ Built HNSW index from store\n");

    /* Verify index */
    size_t nodes = 0;
    katra_vector_hnsw_stats(index, &nodes, NULL, NULL);
    assert(nodes >= num_docs);  /* May have loaded persisted docs */
    printf("✓ Index has %zu nodes\n", nodes);

    /* Test search */
    vector_embedding_t* query = NULL;
    katra_vector_create_embedding("artificial intelligence", &query);
    assert(query != NULL);

    size_t* ids = NULL;
    float* distances = NULL;
    size_t count = 0;
    result = katra_vector_hnsw_search(index, query, 3, &ids, &distances, &count);
    assert(result == KATRA_SUCCESS);
    assert(count > 0);
    printf("✓ Search found %zu results\n", count);

    /* Cleanup */
    free(ids);
    free(distances);
    katra_vector_free_embedding(query);
    katra_vector_hnsw_cleanup(index);
    katra_vector_cleanup(store);
    printf("✓ Test 5 passed\n");
}

/* Test 6: Empty index search */
static void test_hnsw_empty_search(void) {
    printf("\n=== Test 6: Empty index search ===\n");

    hnsw_index_t* index = katra_vector_hnsw_init();
    assert(index != NULL);

    vector_embedding_t* query = NULL;
    katra_vector_create_embedding("test query", &query);

    size_t* ids = NULL;
    float* distances = NULL;
    size_t count = 0;
    int result = katra_vector_hnsw_search(index, query, 5, &ids, &distances, &count);
    assert(result == KATRA_SUCCESS);
    assert(count == 0);
    assert(ids == NULL);
    assert(distances == NULL);
    printf("✓ Empty index returns no results\n");

    katra_vector_free_embedding(query);
    katra_vector_hnsw_cleanup(index);
    printf("✓ Test 6 passed\n");
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  HNSW INDEXING TEST SUITE (Phase 6.1e)                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    /* Initialize logging */
    log_init("test_vector_hnsw");
    log_set_level(LOG_ERROR);  /* Reduce noise during tests */

    /* Run tests */
    test_hnsw_init_cleanup();
    test_hnsw_single_insert();
    test_hnsw_multiple_inserts();
    test_hnsw_search();
    test_hnsw_build_from_store();
    test_hnsw_empty_search();

    /* Cleanup */
    log_cleanup();

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  ALL TESTS PASSED ✓                                       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
