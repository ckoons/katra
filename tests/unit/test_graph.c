/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Project includes */
#include "katra_graph.h"
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

/* Test: Initialize graph store */
static void test_graph_init(void) {
    TEST("Graph store initialization");

    graph_store_t* store = katra_graph_init("test_ci");
    if (!store) {
        FAIL("Failed to initialize graph store");
    }

    if (strcmp(store->ci_id, "test_ci") != 0) {
        FAIL("CI ID not set correctly");
    }

    if (store->node_count != 0) {
        FAIL("Initial node count should be 0");
    }

    if (store->total_edges != 0) {
        FAIL("Initial edge count should be 0");
    }

    katra_graph_cleanup(store);
    PASS();
}

/* Test: Create nodes */
static void test_create_node(void) {
    TEST("Create graph nodes");

    graph_store_t* store = katra_graph_init("test_ci");
    if (!store) {
        FAIL("Failed to initialize store");
    }

    graph_node_t* node1 = katra_graph_get_or_create_node(store, "mem1");
    graph_node_t* node2 = katra_graph_get_or_create_node(store, "mem2");

    if (!node1 || !node2) {
        FAIL("Failed to create nodes");
    }

    if (store->node_count != 2) {
        FAIL("Node count should be 2");
    }

    /* Get existing node */
    graph_node_t* node1_again = katra_graph_get_or_create_node(store, "mem1");
    if (node1_again != node1) {
        FAIL("Should return existing node");
    }

    if (store->node_count != 2) {
        FAIL("Node count should still be 2");
    }

    katra_graph_cleanup(store);
    PASS();
}

/* Test: Add edges */
static void test_add_edges(void) {
    TEST("Add edges between nodes");

    graph_store_t* store = katra_graph_init("test_ci");
    if (!store) {
        FAIL("Failed to initialize store");
    }

    /* Add edges */
    int result1 = katra_graph_add_edge(store, "mem1", "mem2",
                                       REL_SEQUENTIAL, NULL, 0.8f);
    int result2 = katra_graph_add_edge(store, "mem2", "mem3",
                                       REL_CAUSAL, "causes", 0.9f);

    if (result1 != KATRA_SUCCESS || result2 != KATRA_SUCCESS) {
        FAIL("Failed to add edges");
    }

    if (store->total_edges != 2) {
        FAIL("Should have 2 edges");
    }

    if (store->node_count != 3) {
        FAIL("Should have 3 nodes");
    }

    katra_graph_cleanup(store);
    PASS();
}

/* Test: Get related memories */
static void test_get_related(void) {
    TEST("Get related memories");

    graph_store_t* store = katra_graph_init("test_ci");
    if (!store) {
        FAIL("Failed to initialize store");
    }

    /* Build graph:  mem1 -> mem2 -> mem3
     *                       -> mem4
     */
    katra_graph_add_edge(store, "mem1", "mem2", REL_SEQUENTIAL, NULL, 0.8f);
    katra_graph_add_edge(store, "mem2", "mem3", REL_SIMILAR, NULL, 0.7f);
    katra_graph_add_edge(store, "mem2", "mem4", REL_ELABORATES, NULL, 0.9f);

    /* Get outgoing edges from mem2 (all types) */
    graph_edge_t** edges = NULL;
    size_t count = 0;
    int result = katra_graph_get_related(store, "mem2", 0,
                                          &edges, &count);

    if (result != KATRA_SUCCESS) {
        FAIL("Failed to get related memories");
    }

    if (count != 2) {
        FAIL("Should have 2 outgoing edges from mem2");
    }

    katra_graph_free_edges(edges, count);
    katra_graph_cleanup(store);
    PASS();
}

/* Test: Filter by relationship type */
static void test_filter_by_type(void) {
    TEST("Filter edges by relationship type");

    graph_store_t* store = katra_graph_init("test_ci");
    if (!store) {
        FAIL("Failed to initialize store");
    }

    /* Add different edge types */
    katra_graph_add_edge(store, "mem1", "mem2", REL_SEQUENTIAL, NULL, 0.8f);
    katra_graph_add_edge(store, "mem1", "mem3", REL_SIMILAR, NULL, 0.7f);
    katra_graph_add_edge(store, "mem1", "mem4", REL_SIMILAR, NULL, 0.9f);

    /* Filter for similar relationships only */
    graph_edge_t** edges = NULL;
    size_t count = 0;
    int result = katra_graph_get_related(store, "mem1", REL_SIMILAR,
                                          &edges, &count);

    if (result != KATRA_SUCCESS) {
        FAIL("Failed to filter edges");
    }

    if (count != 2) {
        FAIL("Should have 2 similar edges");
    }

    katra_graph_free_edges(edges, count);
    katra_graph_cleanup(store);
    PASS();
}

/* Test: Graph traversal */
static void test_traversal(void) {
    TEST("Graph traversal (BFS)");

    graph_store_t* store = katra_graph_init("test_ci");
    if (!store) {
        FAIL("Failed to initialize store");
    }

    /* Build graph: mem1 -> mem2 -> mem3
     *                       -> mem4 -> mem5
     */
    katra_graph_add_edge(store, "mem1", "mem2", REL_SEQUENTIAL, NULL, 0.8f);
    katra_graph_add_edge(store, "mem2", "mem3", REL_SEQUENTIAL, NULL, 0.7f);
    katra_graph_add_edge(store, "mem2", "mem4", REL_SEQUENTIAL, NULL, 0.9f);
    katra_graph_add_edge(store, "mem4", "mem5", REL_SEQUENTIAL, NULL, 0.6f);

    /* Traverse from mem1 */
    graph_path_node_t** nodes = NULL;
    size_t count = 0;
    int result = katra_graph_traverse(store, "mem1", 3, &nodes, &count);

    if (result != KATRA_SUCCESS) {
        FAIL("Traversal failed");
    }

    if (count < 4) {  /* Should visit at least mem1, mem2, mem3, mem4 */
        FAIL("Should visit multiple nodes");
    }

    /* First node should be start node */
    if (strcmp(nodes[0]->record_id, "mem1") != 0) {
        FAIL("First node should be start node");
    }

    if (nodes[0]->depth != 0) {
        FAIL("Start node should have depth 0");
    }

    katra_graph_free_paths(nodes, count);
    katra_graph_cleanup(store);
    PASS();
}

/* Test: Graph statistics */
static void test_graph_stats(void) {
    TEST("Graph statistics");

    graph_store_t* store = katra_graph_init("test_ci");
    if (!store) {
        FAIL("Failed to initialize store");
    }

    katra_graph_add_edge(store, "mem1", "mem2", REL_SEQUENTIAL, NULL, 0.8f);
    katra_graph_add_edge(store, "mem2", "mem3", REL_SEQUENTIAL, NULL, 0.7f);
    katra_graph_add_edge(store, "mem3", "mem1", REL_REFERENCES, NULL, 0.9f);

    size_t node_count = 0;
    size_t edge_count = 0;
    float avg_degree = 0.0f;

    int result = katra_graph_stats(store, &node_count, &edge_count, &avg_degree);

    if (result != KATRA_SUCCESS) {
        FAIL("Failed to get stats");
    }

    if (node_count != 3) {
        FAIL("Should have 3 nodes");
    }

    if (edge_count != 3) {
        FAIL("Should have 3 edges");
    }

    if (avg_degree <= 0.0f) {
        FAIL("Average degree should be > 0");
    }

    katra_graph_cleanup(store);
    PASS();
}

/* Test: Expand node capacity */
static void test_expand_nodes(void) {
    TEST("Expand node capacity");

    graph_store_t* store = katra_graph_init("test_ci");
    if (!store) {
        FAIL("Failed to initialize store");
    }

    /* Create many nodes to trigger expansion */
    for (int i = 0; i < 150; i++) {
        char id[64];
        snprintf(id, sizeof(id), "mem%d", i);

        graph_node_t* node = katra_graph_get_or_create_node(store, id);
        if (!node) {
            FAIL("Failed to create node during expansion");
        }
    }

    if (store->node_count != 150) {
        FAIL("Should have 150 nodes");
    }

    if (store->node_capacity < 150) {
        FAIL("Capacity should have expanded");
    }

    katra_graph_cleanup(store);
    PASS();
}

/* Test: Relationship types */
static void test_relationship_types(void) {
    TEST("Relationship type names");

    const char* name = katra_graph_relationship_name(REL_SEQUENTIAL);
    if (strcmp(name, "sequential") != 0) {
        FAIL("Wrong name for sequential");
    }

    name = katra_graph_relationship_name(REL_CAUSAL);
    if (strcmp(name, "causal") != 0) {
        FAIL("Wrong name for causal");
    }

    name = katra_graph_relationship_name(REL_SIMILAR);
    if (strcmp(name, "similar") != 0) {
        FAIL("Wrong name for similar");
    }

    PASS();
}

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Katra Graph Database Tests\n");
    printf("========================================\n");
    printf("\n");

    /* Run tests */
    test_graph_init();
    test_create_node();
    test_add_edges();
    test_get_related();
    test_filter_by_type();
    test_traversal();
    test_graph_stats();
    test_expand_nodes();
    test_relationship_types();

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
