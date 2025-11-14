/* © 2025 Casey Koons All rights reserved */

/* Test to examine production vectors and query results */

#include <stdio.h>
#include <stdlib.h>
#include "katra_vector.h"
#include "katra_error.h"

int main(void) {
    printf("\n=== Production Vector Analysis ===\n\n");

    /* Load existing vector store */
    vector_store_t* store = katra_vector_init("Casey", false);
    if (!store) {
        printf("FAIL: Could not load vector store\n");
        return 1;
    }

    printf("Loaded %zu vectors\n\n", store->count);

    /* Test query: "zebra vocabulary" */
    const char* query1 = "zebra vocabulary";
    vector_match_t** matches1 = NULL;
    size_t count1 = 0;

    katra_vector_search(store, query1, 10, &matches1, &count1);
    printf("Query: '%s'\n", query1);
    printf("Results: %zu matches\n", count1);
    if (count1 > 0) {
        for (size_t i = 0; i < count1 && i < 5; i++) {
            printf("  [%zu] %s: similarity=%.4f\n",
                   i, matches1[i]->record_id, matches1[i]->similarity);
        }
    }
    printf("\n");
    katra_vector_free_matches(matches1, count1);

    /* Test query: "unique interference" */
    const char* query2 = "unique interference";
    vector_match_t** matches2 = NULL;
    size_t match_count_2 = 0;

    katra_vector_search(store, query2, 10, &matches2, &match_count_2);
    printf("Query: '%s'\n", query2);
    printf("Results: %zu matches\n", match_count_2);
    if (match_count_2 > 0) {
        for (size_t i = 0; i < match_count_2 && i < 5; i++) {
            printf("  [%zu] %s: similarity=%.4f\n",
                   i, matches2[i]->record_id, matches2[i]->similarity);
        }
    }
    printf("\n");
    katra_vector_free_matches(matches2, match_count_2);

    /* Cleanup */
    katra_vector_cleanup(store);

    printf("=== Test Complete ===\n");
    return 0;
}
