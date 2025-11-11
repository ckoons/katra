/* © 2025 Casey Koons All rights reserved */

/* Tests for TF-IDF embeddings (Phase 6.1b) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "katra_vector.h"
#include "katra_error.h"

#define TEST_CI_ID "test_tfidf"

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

/* Test 1: Basic TF-IDF embedding creation */
static void test_tfidf_basic(void) {
    TEST("test_tfidf_basic");

    /* Build a small corpus first (need multiple docs for IDF to work) */
    const char* corpus[] = {
        "machine learning algorithms for data analysis",
        "deep neural networks process information",
        "natural language understanding requires context"
    };

    /* Update IDF stats with corpus */
    for (int i = 0; i < 3; i++) {
        int result = katra_vector_tfidf_update_stats(corpus[i]);
        assert(result == KATRA_SUCCESS);
    }

    /* Now create embedding for first document */
    vector_embedding_t* embedding = NULL;
    int result = katra_vector_tfidf_create(corpus[0], &embedding);
    assert(result == KATRA_SUCCESS);
    assert(embedding != NULL);
    assert(embedding->dimensions == VECTOR_DIMENSIONS);
    assert(embedding->magnitude > 0.0f);

    /* Cleanup */
    katra_vector_free_embedding(embedding);

    PASS();
}

/* Test 2: TF-IDF with multiple documents */
static void test_tfidf_multiple_docs(void) {
    TEST("test_tfidf_multiple_docs");

    const char* docs[] = {
        "machine learning is a subset of artificial intelligence",
        "deep learning uses neural networks with multiple layers",
        "natural language processing enables machines to understand text"
    };

    /* Update IDF stats with all documents */
    for (int i = 0; i < 3; i++) {
        int result = katra_vector_tfidf_update_stats(docs[i]);
        assert(result == KATRA_SUCCESS);
    }

    /* Get stats */
    size_t vocab_size, total_docs;
    int result = katra_vector_tfidf_get_stats(&vocab_size, &total_docs);
    assert(result == KATRA_SUCCESS);
    assert(vocab_size > 0);
    assert(total_docs >= 3);

    /* Create embeddings for each */
    vector_embedding_t* embeddings[3];
    for (int i = 0; i < 3; i++) {
        result = katra_vector_tfidf_create(docs[i], &embeddings[i]);
        assert(result == KATRA_SUCCESS);
        assert(embeddings[i] != NULL);
    }

    /* Cleanup */
    for (int i = 0; i < 3; i++) {
        katra_vector_free_embedding(embeddings[i]);
    }

    PASS();
}

/* Test 3: TF-IDF similarity detection */
static void test_tfidf_similarity(void) {
    TEST("test_tfidf_similarity");

    const char* doc1 = "machine learning algorithms for classification";
    const char* doc2 = "machine learning models for prediction";
    const char* doc3 = "cooking recipes for italian food";

    /* Update IDF stats */
    katra_vector_tfidf_update_stats(doc1);
    katra_vector_tfidf_update_stats(doc2);
    katra_vector_tfidf_update_stats(doc3);

    /* Create embeddings */
    vector_embedding_t* emb1 = NULL;
    vector_embedding_t* emb2 = NULL;
    vector_embedding_t* emb3 = NULL;

    katra_vector_tfidf_create(doc1, &emb1);
    katra_vector_tfidf_create(doc2, &emb2);
    katra_vector_tfidf_create(doc3, &emb3);

    assert(emb1 != NULL && emb2 != NULL && emb3 != NULL);

    /* Calculate similarities */
    float sim_1_2 = katra_vector_cosine_similarity(emb1, emb2);
    float sim_1_3 = katra_vector_cosine_similarity(emb1, emb3);
    float sim_2_3 = katra_vector_cosine_similarity(emb2, emb3);

    /* doc1 and doc2 should be more similar (both about machine learning) */
    /* than doc1 and doc3 (different topics) */
    assert(sim_1_2 > sim_1_3);
    assert(sim_1_2 > sim_2_3);

    /* Cleanup */
    katra_vector_free_embedding(emb1);
    katra_vector_free_embedding(emb2);
    katra_vector_free_embedding(emb3);

    PASS();
}

/* Test 4: TF-IDF with store integration */
static void test_tfidf_with_store(void) {
    TEST("test_tfidf_with_store");

    /* Create vector store with TF-IDF method */
    vector_store_t* store = katra_vector_init(TEST_CI_ID, false);
    assert(store != NULL);
    assert(store->method == EMBEDDING_TFIDF);  /* Should default to TF-IDF */

    /* Add some documents */
    const char* texts[] = {
        "artificial intelligence and machine learning",
        "deep neural networks and backpropagation",
        "natural language processing applications"
    };

    for (int i = 0; i < 3; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "doc_%d", i);

        int result = katra_vector_store(store, record_id, texts[i]);
        assert(result == KATRA_SUCCESS);
    }

    assert(store->count >= 3);  /* May have loaded persisted vectors */

    /* Search for similar documents */
    const char* query = "machine learning neural networks";
    vector_match_t** matches = NULL;
    size_t match_count = 0;

    int result = katra_vector_search(store, query, 3, &matches, &match_count);
    assert(result == KATRA_SUCCESS);
    assert(match_count > 0);
    assert(matches != NULL);

    /* Verify we got similarity scores */
    for (size_t i = 0; i < match_count; i++) {
        assert(matches[i]->similarity >= 0.0f && matches[i]->similarity <= 1.0f);
    }

    /* Cleanup */
    katra_vector_free_matches(matches, match_count);
    katra_vector_cleanup(store);

    PASS();
}

/* Test 5: TF-IDF downweights common words */
static void test_tfidf_common_words(void) {
    TEST("test_tfidf_common_words");

    /* Documents with common words */
    const char* doc1 = "the quick brown fox jumps over the lazy dog";
    const char* doc2 = "the fast red cat runs under the sleepy bird";
    const char* doc3 = "the slow green turtle walks beside the active fish";

    /* Update IDF stats - "the" should become very common */
    katra_vector_tfidf_update_stats(doc1);
    katra_vector_tfidf_update_stats(doc2);
    katra_vector_tfidf_update_stats(doc3);

    /* Create embeddings */
    vector_embedding_t* emb1 = NULL;
    katra_vector_tfidf_create(doc1, &emb1);

    assert(emb1 != NULL);
    assert(emb1->magnitude > 0.0f);

    /* The embedding should not be dominated by "the" (which appears 3 times) */
    /* but should capture the distinctive words like "fox", "jumps", etc. */
    /* We can't easily test this directly, but we verify the embedding is valid */

    katra_vector_free_embedding(emb1);

    PASS();
}

int main(void) {
    printf("\n");
    printf("=================================\n");
    printf("TF-IDF Embeddings Tests\n");
    printf("=================================\n\n");

    test_tfidf_basic();
    test_tfidf_multiple_docs();
    test_tfidf_similarity();
    test_tfidf_with_store();
    test_tfidf_common_words();

    /* Cleanup global TF-IDF stats */
    katra_vector_tfidf_cleanup();

    printf("\n");
    printf("=================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("=================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
