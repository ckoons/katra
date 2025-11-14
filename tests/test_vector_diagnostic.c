/* © 2025 Casey Koons All rights reserved */

/* Diagnostic test to investigate zero similarity issue */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "katra_vector.h"
#include "katra_error.h"

int main(void) {
    printf("\n=== Vector Diagnostic Test ===\n\n");

    /* Clean start */
    katra_vector_tfidf_cleanup();

    /* Remove old persisted vectors */
    system("rm -f ~/.katra/memory/tier2/vectors/vectors.db");

    /* Create vector store */
    vector_store_t* store = katra_vector_init("test_diagnostic", true);
    if (!store) {
        printf("FAIL: Could not create vector store\n");
        return 1;
    }

    /* Add a document */
    const char* doc = "Dragon Con Atlanta GRRM";
    printf("Storing document: '%s'\n", doc);
    int result = katra_vector_store(store, "doc_0", doc);
    if (result != KATRA_SUCCESS) {
        printf("FAIL: Could not store vector\n");
        return 1;
    }

    /* Get the stored embedding */
    vector_embedding_t* stored_emb = katra_vector_get(store, "doc_0");
    if (!stored_emb) {
        printf("FAIL: Could not retrieve stored embedding\n");
        return 1;
    }

    printf("\nStored embedding:\n");
    printf("  record_id: %s\n", stored_emb->record_id);
    printf("  dimensions: %zu\n", stored_emb->dimensions);
    printf("  magnitude: %.6f\n", stored_emb->magnitude);

    /* Show first 10 non-zero values */
    int non_zero_count = 0;
    printf("  First 10 non-zero values:\n");
    for (size_t i = 0; i < stored_emb->dimensions && non_zero_count < 10; i++) {
        if (stored_emb->values[i] != 0.0f) {
            printf("    [%zu] = %.6f\n", i, stored_emb->values[i]);
            non_zero_count++;
        }
    }
    printf("  Total non-zero dimensions: %d\n", non_zero_count);

    /* Now create a query embedding */
    const char* query = "Dragon Atlanta";
    printf("\nCreating query embedding: '%s'\n", query);

    vector_match_t** matches = NULL;
    size_t match_count = 0;
    result = katra_vector_search(store, query, 10, &matches, &match_count);

    if (result != KATRA_SUCCESS) {
        printf("FAIL: Search failed\n");
        return 1;
    }

    printf("\nSearch returned %zu matches:\n", match_count);
    for (size_t i = 0; i < match_count; i++) {
        printf("  [%zu] %s: similarity=%.6f\n",
               i, matches[i]->record_id, matches[i]->similarity);

        /* Calculate dot product manually */
        if (matches[i]->embedding) {
            float dot = 0.0f;
            int shared_nonzero = 0;

            /* We need the query embedding - let's create it directly */
            vector_embedding_t* query_emb = NULL;
            katra_vector_tfidf_create(query, &query_emb);

            if (query_emb) {
                printf("    Query embedding: mag=%.6f\n", query_emb->magnitude);

                /* Count non-zero in query */
                int query_nonzero = 0;
                for (size_t j = 0; j < query_emb->dimensions; j++) {
                    if (query_emb->values[j] != 0.0f) {
                        query_nonzero++;

                        /* Check if doc also has non-zero at this position */
                        if (matches[i]->embedding->values[j] != 0.0f) {
                            shared_nonzero++;
                            dot += query_emb->values[j] * matches[i]->embedding->values[j];
                        }
                    }
                }

                printf("    Query non-zero dims: %d\n", query_nonzero);
                printf("    Doc non-zero dims: %d\n", non_zero_count);
                printf("    Shared non-zero dims: %d\n", shared_nonzero);
                printf("    Manual dot product: %.6f\n", dot);
                printf("    Manual similarity: %.6f\n",
                       dot / (query_emb->magnitude * matches[i]->embedding->magnitude));

                katra_vector_free_embedding(query_emb);
            }
        }
    }

    /* Cleanup */
    katra_vector_free_matches(matches, match_count);
    katra_vector_cleanup(store);

    printf("\n=== Test Complete ===\n");
    return 0;
}
