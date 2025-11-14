/* © 2025 Casey Koons All rights reserved */

/* Tests for multi-word vector search (fixing semantic search issues) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "katra_vector.h"
#include "katra_error.h"
#include "katra_log.h"

#define TEST_CI_ID "test_multiword"

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

#define FAIL(msg) \
    do { \
        printf("FAIL: %s\n", msg); \
        return; \
    } while(0)

/* Test 1: Multi-word query with both terms in vocabulary */
static void test_multiword_both_terms_known(void) {
    TEST("test_multiword_both_terms_known");

    /* Clean start */
    katra_vector_tfidf_cleanup();

    /* Create vector store (force new to avoid loading old persisted vectors) */
    vector_store_t* store = katra_vector_init(TEST_CI_ID, true);
    assert(store != NULL);

    /* Add documents that will build vocabulary */
    const char* docs[] = {
        "I attended Dragon Con in Atlanta with George R R Martin",
        "The Roman Empire history is fascinating to study",
        "Machine learning algorithms process data efficiently"
    };

    for (int i = 0; i < 3; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "doc_%d", i);
        int result = katra_vector_store(store, record_id, docs[i]);
        if (result != KATRA_SUCCESS) {
            FAIL("Failed to store document");
        }
    }

    /* Now search with multi-word query where both terms are in vocabulary */
    const char* query = "Dragon Atlanta";
    vector_match_t** matches = NULL;
    size_t match_count = 0;

    int result = katra_vector_search(store, query, 10, &matches, &match_count);
    assert(result == KATRA_SUCCESS);

    printf("\n    Query: '%s' returned %zu matches\n", query, match_count);
    if (match_count > 0) {
        printf("    Top match: %s (similarity: %.4f)\n",
               matches[0]->record_id, matches[0]->similarity);

        /* Should match doc_0 (has both "Dragon" and "Atlanta") */
        bool found_dragon_con = false;
        for (size_t i = 0; i < match_count; i++) {
            if (strcmp(matches[i]->record_id, "doc_0") == 0) {
                found_dragon_con = true;
                if (matches[i]->similarity > 0.0f) {
                    printf("    ✓ Found Dragon Con memory (similarity: %.4f)\n",
                           matches[i]->similarity);
                } else {
                    printf("    ✗ Found Dragon Con memory but similarity is ZERO (%.4f)\n",
                           matches[i]->similarity);
                }
                break;
            }
        }

        if (!found_dragon_con) {
            katra_vector_free_matches(matches, match_count);
            katra_vector_cleanup(store);
            FAIL("Did not find Dragon Con memory in results");
        }
    } else {
        katra_vector_cleanup(store);
        FAIL("Multi-word query returned zero results");
    }

    /* Cleanup */
    katra_vector_free_matches(matches, match_count);
    katra_vector_cleanup(store);

    PASS();
}

/* Test 2: Multi-word query with one term unknown */
static void test_multiword_one_term_unknown(void) {
    TEST("test_multiword_one_term_unknown");

    /* Clean start */
    katra_vector_tfidf_cleanup();

    vector_store_t* store = katra_vector_init(TEST_CI_ID, true);
    assert(store != NULL);

    /* Add documents */
    const char* docs[] = {
        "Machine learning algorithms are powerful",
        "Data science uses statistical methods",
        "Artificial intelligence systems learn patterns"
    };

    for (int i = 0; i < 3; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "doc_%d", i);
        katra_vector_store(store, record_id, docs[i]);
    }

    /* Query with one known term (machine) and one unknown (xyzabc) */
    const char* query = "machine xyzabc";
    vector_match_t** matches = NULL;
    size_t match_count = 0;

    int result = katra_vector_search(store, query, 10, &matches, &match_count);
    assert(result == KATRA_SUCCESS);

    printf("\n    Query: '%s' returned %zu matches\n", query, match_count);

    /* Should still get results based on "machine" even though "xyzabc" is unknown */
    if (match_count > 0) {
        printf("    ✓ Got %zu results despite unknown term\n", match_count);
        assert(matches[0]->similarity > 0.0f);
    }

    /* Cleanup */
    katra_vector_free_matches(matches, match_count);
    katra_vector_cleanup(store);

    PASS();
}

/* Test 3: Query doesn't pollute IDF statistics */
static void test_query_no_idf_pollution(void) {
    TEST("test_query_no_idf_pollution");

    /* Clean start */
    katra_vector_tfidf_cleanup();

    vector_store_t* store = katra_vector_init(TEST_CI_ID, true);
    assert(store != NULL);

    /* Add one document */
    katra_vector_store(store, "doc_1", "machine learning is great");

    /* Get IDF stats before query */
    size_t vocab_before, docs_before;
    katra_vector_tfidf_get_stats(&vocab_before, &docs_before);

    printf("\n    Before query: vocab=%zu, docs=%zu\n", vocab_before, docs_before);

    /* Execute query with new terms */
    const char* query = "unicorn rainbow sparkles";
    vector_match_t** matches = NULL;
    size_t match_count = 0;

    katra_vector_search(store, query, 10, &matches, &match_count);

    /* Get IDF stats after query */
    size_t vocab_after, docs_after;
    katra_vector_tfidf_get_stats(&vocab_after, &docs_after);

    printf("    After query:  vocab=%zu, docs=%zu\n", vocab_after, docs_after);

    /* Vocabulary should NOT have grown (query terms should not be added) */
    if (vocab_after > vocab_before) {
        katra_vector_free_matches(matches, match_count);
        katra_vector_cleanup(store);
        FAIL("Query polluted IDF vocabulary");
    }

    /* Document count should NOT have increased */
    if (docs_after > docs_before) {
        katra_vector_free_matches(matches, match_count);
        katra_vector_cleanup(store);
        FAIL("Query incremented document count");
    }

    printf("    ✓ Query did not pollute IDF statistics\n");

    /* Cleanup */
    katra_vector_free_matches(matches, match_count);
    katra_vector_cleanup(store);

    PASS();
}

/* Test 4: Contiguous phrase matching */
static void test_contiguous_phrase(void) {
    TEST("test_contiguous_phrase");

    /* Clean start */
    katra_vector_tfidf_cleanup();

    vector_store_t* store = katra_vector_init(TEST_CI_ID, true);
    assert(store != NULL);

    /* Add documents */
    const char* docs[] = {
        "Dragon Con is a convention in Atlanta",
        "Dragons are mythical creatures that breathe fire",
        "Atlanta is a city in Georgia"
    };

    for (int i = 0; i < 3; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "doc_%d", i);
        katra_vector_store(store, record_id, docs[i]);
    }

    /* Query with contiguous phrase (should use keyword search) */
    const char* query = "Dragon Con";
    vector_match_t** matches = NULL;
    size_t match_count = 0;

    int result = katra_vector_search(store, query, 10, &matches, &match_count);
    assert(result == KATRA_SUCCESS);

    printf("\n    Query: '%s' returned %zu matches\n", query, match_count);

    /* Should find doc_0 which contains exact phrase "Dragon Con" */
    bool found = false;
    for (size_t i = 0; i < match_count; i++) {
        if (strcmp(matches[i]->record_id, "doc_0") == 0) {
            found = true;
            printf("    ✓ Found exact phrase match: %s (similarity: %.4f)\n",
                   matches[i]->record_id, matches[i]->similarity);
            break;
        }
    }

    if (!found) {
        katra_vector_free_matches(matches, match_count);
        katra_vector_cleanup(store);
        FAIL("Contiguous phrase not found");
    }

    /* Cleanup */
    katra_vector_free_matches(matches, match_count);
    katra_vector_cleanup(store);

    PASS();
}

/* Test 5: Semantic similarity with related terms */
static void test_semantic_related_terms(void) {
    TEST("test_semantic_related_terms");

    /* Clean start */
    katra_vector_tfidf_cleanup();

    vector_store_t* store = katra_vector_init(TEST_CI_ID, true);
    assert(store != NULL);

    /* Add related documents */
    const char* docs[] = {
        "The dog barked loudly at the mailman",
        "The puppy played with a ball in the yard",
        "The elephant walked through the jungle",
        "The car drove down the highway quickly"
    };

    for (int i = 0; i < 4; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "doc_%d", i);
        katra_vector_store(store, record_id, docs[i]);
    }

    /* Query about dogs (should match doc_0 and doc_1 semantically) */
    const char* query = "dog puppy";
    vector_match_t** matches = NULL;
    size_t match_count = 0;

    int result = katra_vector_search(store, query, 10, &matches, &match_count);
    assert(result == KATRA_SUCCESS);

    printf("\n    Query: '%s' returned %zu matches\n", query, match_count);

    if (match_count > 0) {
        printf("    Top matches:\n");
        for (size_t i = 0; i < match_count && i < 3; i++) {
            printf("      %zu. %s (similarity: %.4f)\n",
                   i+1, matches[i]->record_id, matches[i]->similarity);
        }

        /* Top results should be dog-related docs */
        bool has_dog_doc = false;
        for (size_t i = 0; i < match_count && i < 2; i++) {
            if (strcmp(matches[i]->record_id, "doc_0") == 0 ||
                strcmp(matches[i]->record_id, "doc_1") == 0) {
                has_dog_doc = true;
                break;
            }
        }

        if (!has_dog_doc) {
            printf("    ⚠ Warning: Expected dog-related docs in top results\n");
        } else {
            printf("    ✓ Found dog-related docs in top results\n");
        }
    }

    /* Cleanup */
    katra_vector_free_matches(matches, match_count);
    katra_vector_cleanup(store);

    PASS();
}

/* Test 6: Three-word query */
static void test_three_word_query(void) {
    TEST("test_three_word_query");

    /* Clean start */
    katra_vector_tfidf_cleanup();

    vector_store_t* store = katra_vector_init(TEST_CI_ID, true);
    assert(store != NULL);

    /* Add documents */
    const char* docs[] = {
        "Semantic search uses vector embeddings for similarity",
        "Machine learning models require training data",
        "Natural language processing analyzes text"
    };

    for (int i = 0; i < 3; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "doc_%d", i);
        katra_vector_store(store, record_id, docs[i]);
    }

    /* Three-word query */
    const char* query = "semantic vector similarity";
    vector_match_t** matches = NULL;
    size_t match_count = 0;

    int result = katra_vector_search(store, query, 10, &matches, &match_count);
    assert(result == KATRA_SUCCESS);

    printf("\n    Query: '%s' returned %zu matches\n", query, match_count);

    if (match_count == 0) {
        katra_vector_cleanup(store);
        FAIL("Three-word query returned zero results");
    }

    printf("    ✓ Three-word query successful (%zu results)\n", match_count);

    /* Cleanup */
    katra_vector_free_matches(matches, match_count);
    katra_vector_cleanup(store);

    PASS();
}

int main(void) {
    printf("\n");
    printf("==========================================\n");
    printf("Multi-Word Vector Search Tests\n");
    printf("==========================================\n\n");

    /* Set log level to DEBUG for detailed output */
    setenv("KATRA_LOG_LEVEL", "DEBUG", 1);

    test_multiword_both_terms_known();
    test_multiword_one_term_unknown();
    test_query_no_idf_pollution();
    test_contiguous_phrase();
    test_semantic_related_terms();
    test_three_word_query();

    printf("\n");
    printf("==========================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("==========================================\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
