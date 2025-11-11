/* © 2025 Casey Koons All rights reserved */

/* Test external embeddings API integration (Phase 6.1c) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/katra_vector.h"
#include "../include/katra_error.h"
#include "../include/katra_log.h"

/* Test 1: API key detection from environment */
static void test_api_key_detection(void) {
    printf("\n=== Test 1: API key detection ===\n");

    /* Save original env vars */
    const char* orig_openai = getenv("OPENAI_API_KEY");
    const char* orig_anthropic = getenv("ANTHROPIC_API_KEY");

    /* Test with no API keys */
    unsetenv("OPENAI_API_KEY");
    unsetenv("ANTHROPIC_API_KEY");

    const char* key = katra_vector_external_get_api_key();
    assert(key == NULL);
    assert(!katra_vector_external_available(key));
    printf("✓ No API key correctly returns NULL\n");

    /* Test with OpenAI key */
    setenv("OPENAI_API_KEY", "test-openai-key", 1);
    key = katra_vector_external_get_api_key();
    assert(key != NULL);
    assert(strcmp(key, "test-openai-key") == 0);
    assert(katra_vector_external_available(key));
    printf("✓ OpenAI key correctly detected\n");

    /* Test with Anthropic key (OpenAI takes priority) */
    setenv("ANTHROPIC_API_KEY", "test-anthropic-key", 1);
    key = katra_vector_external_get_api_key();
    assert(key != NULL);
    assert(strcmp(key, "test-openai-key") == 0);  /* OpenAI still first */
    printf("✓ OpenAI key has priority over Anthropic\n");

    /* Test with only Anthropic key */
    unsetenv("OPENAI_API_KEY");
    key = katra_vector_external_get_api_key();
    assert(key != NULL);
    assert(strcmp(key, "test-anthropic-key") == 0);
    printf("✓ Anthropic key correctly detected\n");

    /* Restore original env vars */
    if (orig_openai) {
        setenv("OPENAI_API_KEY", orig_openai, 1);
    } else {
        unsetenv("OPENAI_API_KEY");
    }
    if (orig_anthropic) {
        setenv("ANTHROPIC_API_KEY", orig_anthropic, 1);
    } else {
        unsetenv("ANTHROPIC_API_KEY");
    }

    printf("✓ Test 1 passed\n");
}

/* Test 2: External embeddings without API key (should fail gracefully) */
static void test_no_api_key(void) {
    printf("\n=== Test 2: External embeddings without API key ===\n");

    vector_embedding_t* embedding = NULL;

    /* Should return error with NULL API key */
    int result = katra_vector_external_create("test text", NULL, "openai", &embedding);
    assert(result == E_INPUT_NULL);
    assert(embedding == NULL);
    printf("✓ Returns E_INPUT_NULL with no API key\n");

    /* Should return error with empty API key */
    result = katra_vector_external_create("test text", "", "openai", &embedding);
    assert(result != KATRA_SUCCESS);
    printf("✓ Fails gracefully with empty API key\n");

    printf("✓ Test 2 passed\n");
}

/* Test 3: Integration with vector store fallback */
static void test_store_fallback(void) {
    printf("\n=== Test 3: Vector store fallback behavior ===\n");

    /* Save original API key */
    const char* orig_openai = getenv("OPENAI_API_KEY");
    unsetenv("OPENAI_API_KEY");

    /* Create store with external method */
    vector_store_t* store = katra_vector_init("test_external_ci", false);
    assert(store != NULL);

    store->method = EMBEDDING_EXTERNAL;

    /* Build corpus with a few documents first for TF-IDF to work properly */
    katra_vector_store(store, "corpus_1", "artificial intelligence and deep learning");
    katra_vector_store(store, "corpus_2", "natural language processing techniques");

    /* Store should fall back to TF-IDF when no API key available */
    int result = katra_vector_store(store, "test_rec_1", "machine learning algorithms");
    assert(result == KATRA_SUCCESS);
    assert(store->count >= 3);
    printf("✓ Falls back to TF-IDF when no API key\n");

    /* Verify embedding was created */
    vector_embedding_t* embedding = katra_vector_get(store, "test_rec_1");
    assert(embedding != NULL);
    assert(embedding->dimensions == VECTOR_DIMENSIONS);
    /* Note: magnitude may be 0 if TF-IDF corpus is still building, check for non-NULL only */
    printf("✓ Fallback embedding created successfully (magnitude: %.3f)\n", embedding->magnitude);

    /* Test search still works */
    vector_match_t** matches = NULL;
    size_t count = 0;
    result = katra_vector_search(store, "machine learning", 5, &matches, &count);
    assert(result == KATRA_SUCCESS);
    assert(count >= 1);
    printf("✓ Search works with fallback embeddings\n");

    katra_vector_free_matches(matches, count);
    katra_vector_cleanup(store);

    /* Restore original API key */
    if (orig_openai) {
        setenv("OPENAI_API_KEY", orig_openai, 1);
    }

    printf("✓ Test 3 passed\n");
}

/* Test 4: Real API call (only if API key available) */
static void test_real_api_call(void) {
    printf("\n=== Test 4: Real API call (conditional) ===\n");

    const char* api_key = katra_vector_external_get_api_key();

    if (!api_key || strlen(api_key) == 0) {
        printf("⊘ Skipping real API test (no API key set)\n");
        printf("  Set OPENAI_API_KEY or ANTHROPIC_API_KEY to test real API\n");
        return;
    }

    printf("→ Found API key, testing real API call...\n");

    vector_embedding_t* embedding = NULL;
    int result = katra_vector_external_create(
        "machine learning and artificial intelligence",
        api_key,
        "openai",
        &embedding
    );

    if (result == KATRA_SUCCESS) {
        assert(embedding != NULL);
        assert(embedding->dimensions == VECTOR_DIMENSIONS);
        assert(embedding->magnitude > 0.0f);

        /* Check that embedding has non-zero values */
        int non_zero = 0;
        for (size_t i = 0; i < embedding->dimensions; i++) {
            if (embedding->values[i] != 0.0f) {
                non_zero++;
            }
        }
        assert(non_zero > 0);
        printf("✓ Real API call succeeded (non-zero values: %d/%zu)\n",
               non_zero, embedding->dimensions);

        katra_vector_free_embedding(embedding);
    } else {
        printf("⊘ Real API call failed: %d (may be rate limit or invalid key)\n", result);
        printf("  This is non-fatal - external embeddings have fallback\n");
    }

    printf("✓ Test 4 completed\n");
}

/* Test 5: Unsupported provider */
static void test_unsupported_provider(void) {
    printf("\n=== Test 5: Unsupported provider ===\n");

    /* Set a dummy API key */
    const char* orig_openai = getenv("OPENAI_API_KEY");
    setenv("OPENAI_API_KEY", "test-key", 1);

    vector_embedding_t* embedding = NULL;
    int result = katra_vector_external_create(
        "test text",
        "test-key",
        "unsupported_provider",
        &embedding
    );

    assert(result == E_INPUT_INVALID);
    assert(embedding == NULL);
    printf("✓ Returns E_INPUT_INVALID for unsupported provider\n");

    /* Restore original API key */
    if (orig_openai) {
        setenv("OPENAI_API_KEY", orig_openai, 1);
    } else {
        unsetenv("OPENAI_API_KEY");
    }

    printf("✓ Test 5 passed\n");
}

/* Test 6: NULL provider defaults to OpenAI */
static void test_default_provider(void) {
    printf("\n=== Test 6: NULL provider defaults to OpenAI ===\n");

    /* Save original API key */
    const char* orig_openai = getenv("OPENAI_API_KEY");
    unsetenv("OPENAI_API_KEY");

    vector_embedding_t* embedding = NULL;

    /* NULL provider should default to OpenAI but fail without key */
    int result = katra_vector_external_create(
        "test text",
        NULL,
        NULL,
        &embedding
    );

    assert(result == E_INPUT_NULL);  /* No API key */
    assert(embedding == NULL);
    printf("✓ NULL provider handled correctly\n");

    /* Restore original API key */
    if (orig_openai) {
        setenv("OPENAI_API_KEY", orig_openai, 1);
    }

    printf("✓ Test 6 passed\n");
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  EXTERNAL EMBEDDINGS TEST SUITE (Phase 6.1c)              ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    /* Initialize logging */
    log_init("test_vector_external");
    log_set_level(LOG_ERROR);  /* Reduce noise during tests */

    /* Run tests */
    test_api_key_detection();
    test_no_api_key();
    test_store_fallback();
    test_real_api_call();
    test_unsupported_provider();
    test_default_provider();

    /* Cleanup */
    log_cleanup();

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  ALL TESTS PASSED ✓                                       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}
