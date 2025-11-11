/* © 2025 Casey Koons All rights reserved */

/*
 * benchmark_vector.c - Vector search performance benchmarks
 *
 * Tests performance of vector operations at scale:
 * - Embedding creation (1K, 10K, 100K)
 * - Vector storage and retrieval
 * - HNSW index building and search
 * - Cosine similarity calculations
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

/* Project includes */
#include "katra_vector.h"
#include "katra_error.h"
#include "katra_log.h"

/* Benchmark configuration */
#define BENCH_SMALL_SIZE 1000
#define BENCH_MEDIUM_SIZE 10000
#define BENCH_LARGE_SIZE 100000
#define BENCH_SEARCH_QUERIES 100

/* Timing utilities */
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

/* Test data generation */
static char* generate_test_text(int index) {
    static char buffer[256];
    snprintf(buffer, sizeof(buffer),
            "This is test document %d with some unique content about topic_%d",
            index, index % 100);
    return buffer;
}

/* Benchmark: Embedding creation */
static void bench_embedding_creation(size_t count) {
    printf("\nBenchmark: Creating %zu embeddings...\n", count);

    double start = get_time_ms();

    for (size_t i = 0; i < count; i++) {
        vector_embedding_t* embedding = NULL;
        char* text = generate_test_text((int)i);

        int result = katra_vector_create_embedding(text, &embedding);
        if (result != KATRA_SUCCESS || !embedding) {
            printf("  FAILED at embedding %zu\n", i);
            return;
        }

        katra_vector_free_embedding(embedding);

        if ((i + 1) % 1000 == 0) {
            printf("  Progress: %zu / %zu\n", i + 1, count);
        }
    }

    double elapsed = get_time_ms() - start;
    printf("  Completed: %zu embeddings in %.2f ms\n", count, elapsed);
    printf("  Average: %.3f ms per embedding\n", elapsed / count);
    printf("  Rate: %.0f embeddings/second\n", (count * 1000.0) / elapsed);
}

/* Benchmark: Vector storage and retrieval */
static void bench_vector_storage(size_t count) {
    printf("\nBenchmark: Storing and retrieving %zu vectors...\n", count);

    vector_store_t* store = katra_vector_init("bench_ci", false);
    if (!store) {
        printf("  FAILED: Could not initialize vector store\n");
        return;
    }

    /* Storage phase */
    double start = get_time_ms();

    for (size_t i = 0; i < count; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "record_%zu", i);

        char* text = generate_test_text((int)i);
        int result = katra_vector_store(store, record_id, text);

        if (result != KATRA_SUCCESS) {
            printf("  FAILED at store %zu\n", i);
            katra_vector_cleanup(store);
            return;
        }

        if ((i + 1) % 1000 == 0) {
            printf("  Stored: %zu / %zu\n", i + 1, count);
        }
    }

    double store_time = get_time_ms() - start;
    printf("  Storage: %zu vectors in %.2f ms (%.0f vectors/sec)\n",
           count, store_time, (count * 1000.0) / store_time);

    /* Retrieval phase */
    start = get_time_ms();

    for (size_t i = 0; i < count; i += 10) {  /* Sample every 10th */
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "record_%zu", i);

        vector_embedding_t* embedding = katra_vector_get(store, record_id);
        if (!embedding) {
            printf("  FAILED to retrieve record %zu\n", i);
        }
    }

    double retrieve_time = get_time_ms() - start;
    size_t retrieve_count = count / 10;
    printf("  Retrieval: %zu vectors in %.2f ms (%.0f vectors/sec)\n",
           retrieve_count, retrieve_time,
           (retrieve_count * 1000.0) / retrieve_time);

    katra_vector_cleanup(store);
}

/* Benchmark: Vector search */
static void bench_vector_search(size_t vector_count) {
    printf("\nBenchmark: Searching among %zu vectors...\n", vector_count);

    vector_store_t* store = katra_vector_init("bench_ci", false);
    if (!store) {
        printf("  FAILED: Could not initialize vector store\n");
        return;
    }

    /* Populate store */
    printf("  Populating vector store...\n");
    for (size_t i = 0; i < vector_count; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "record_%zu", i);

        char* text = generate_test_text((int)i);
        katra_vector_store(store, record_id, text);

        if ((i + 1) % 1000 == 0) {
            printf("    Stored: %zu / %zu\n", i + 1, vector_count);
        }
    }

    /* Run search queries */
    printf("  Running %d search queries...\n", BENCH_SEARCH_QUERIES);

    double total_search_time = 0.0;

    for (int q = 0; q < BENCH_SEARCH_QUERIES; q++) {
        char query_text[256];
        snprintf(query_text, sizeof(query_text),
                "Query about topic_%d", rand() % 100);

        double start = get_time_ms();

        vector_match_t** matches = NULL;
        size_t match_count = 0;
        int result = katra_vector_search(store, query_text, 10,
                                         &matches, &match_count);

        double elapsed = get_time_ms() - start;
        total_search_time += elapsed;

        if (result != KATRA_SUCCESS) {
            printf("    FAILED query %d\n", q);
        }

        katra_vector_free_matches(matches, match_count);
    }

    printf("  Search: %d queries in %.2f ms\n",
           BENCH_SEARCH_QUERIES, total_search_time);
    printf("  Average: %.3f ms per query\n",
           total_search_time / BENCH_SEARCH_QUERIES);
    printf("  Rate: %.0f queries/second\n",
           (BENCH_SEARCH_QUERIES * 1000.0) / total_search_time);

    katra_vector_cleanup(store);
}

/* Benchmark: HNSW index building */
static void bench_hnsw_index(size_t vector_count) {
    printf("\nBenchmark: Building HNSW index with %zu vectors...\n",
           vector_count);

    vector_store_t* store = katra_vector_init("bench_ci", false);
    if (!store) {
        printf("  FAILED: Could not initialize vector store\n");
        return;
    }

    /* Populate store */
    printf("  Populating vector store...\n");
    for (size_t i = 0; i < vector_count; i++) {
        char record_id[64];
        snprintf(record_id, sizeof(record_id), "record_%zu", i);

        char* text = generate_test_text((int)i);
        katra_vector_store(store, record_id, text);

        if ((i + 1) % 1000 == 0) {
            printf("    Stored: %zu / %zu\n", i + 1, vector_count);
        }
    }

    /* Build HNSW index */
    printf("  Building HNSW index...\n");
    double start = get_time_ms();

    hnsw_index_t* index = NULL;
    int result = katra_vector_hnsw_build(store, &index);

    double build_time = get_time_ms() - start;

    if (result != KATRA_SUCCESS || !index) {
        printf("  FAILED: Could not build HNSW index\n");
        katra_vector_cleanup(store);
        return;
    }

    printf("  Index built in %.2f ms\n", build_time);

    /* Get index statistics */
    size_t nodes;
    int max_layer;
    size_t total_connections;
    katra_vector_hnsw_stats(index, &nodes, &max_layer, &total_connections);

    printf("  Index stats:\n");
    printf("    Nodes: %zu\n", nodes);
    printf("    Max layer: %d\n", max_layer);
    printf("    Total connections: %zu\n", total_connections);
    printf("    Avg connections/node: %.2f\n",
           (float)total_connections / nodes);

    /* Benchmark HNSW search */
    printf("  Benchmarking HNSW search (%d queries)...\n",
           BENCH_SEARCH_QUERIES);

    double total_search_time = 0.0;

    for (int q = 0; q < BENCH_SEARCH_QUERIES; q++) {
        vector_embedding_t* query = NULL;
        char query_text[256];
        snprintf(query_text, sizeof(query_text),
                "Query about topic_%d", rand() % 100);

        katra_vector_create_embedding(query_text, &query);

        double search_start = get_time_ms();

        size_t* ids = NULL;
        float* distances = NULL;
        size_t count = 0;

        katra_vector_hnsw_search(index, query, 10, &ids, &distances, &count);

        double search_elapsed = get_time_ms() - search_start;
        total_search_time += search_elapsed;

        free(ids);
        free(distances);
        katra_vector_free_embedding(query);
    }

    printf("  HNSW search: %d queries in %.2f ms\n",
           BENCH_SEARCH_QUERIES, total_search_time);
    printf("  Average: %.3f ms per query\n",
           total_search_time / BENCH_SEARCH_QUERIES);
    printf("  Rate: %.0f queries/second\n",
           (BENCH_SEARCH_QUERIES * 1000.0) / total_search_time);

    katra_vector_hnsw_cleanup(index);
    katra_vector_cleanup(store);
}

/* Main benchmark driver */
int main(void) {
    printf("========================================\n");
    printf("Katra Vector Performance Benchmarks\n");
    printf("========================================\n\n");

    srand((unsigned int)time(NULL));

    /* Embedding creation benchmarks */
    bench_embedding_creation(BENCH_SMALL_SIZE);

    /* Storage benchmarks */
    bench_vector_storage(BENCH_SMALL_SIZE);

    /* Search benchmarks */
    bench_vector_search(BENCH_SMALL_SIZE);

    /* HNSW benchmarks */
    bench_hnsw_index(BENCH_SMALL_SIZE);

    printf("\n========================================\n");
    printf("Medium-scale benchmarks (10K vectors)\n");
    printf("========================================\n");

    bench_embedding_creation(BENCH_MEDIUM_SIZE);
    bench_vector_storage(BENCH_MEDIUM_SIZE);
    bench_vector_search(BENCH_MEDIUM_SIZE);
    bench_hnsw_index(BENCH_MEDIUM_SIZE);

    printf("\n========================================\n");
    printf("Benchmarks Complete!\n");
    printf("========================================\n");

    return 0;
}
