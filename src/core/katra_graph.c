/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_graph.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_core_common.h"

/* Constants */
#define GRAPH_INITIAL_CAPACITY 128
#define GRAPH_EDGE_INITIAL_CAPACITY 8

/* Global graph store (one per process) */
static graph_store_t* g_graph_store = NULL;

/* Forward declarations */
static graph_node_t* find_node(graph_store_t* store, const char* record_id);
static int ensure_node_capacity(graph_store_t* store);
static graph_edge_t* create_edge(const char* from_id, const char* to_id,
                                  relationship_type_t type, const char* label,
                                  float strength);
static void free_edge(graph_edge_t* edge);
static void free_node(graph_node_t* node);

/* Initialize graph store */
graph_store_t* katra_graph_init(const char* ci_id) {
    graph_store_t* store = NULL;

    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_graph_init", "ci_id is NULL");
        return NULL;
    }

    /* Allocate store */
    store = calloc(1, sizeof(graph_store_t));
    if (!store) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_graph_init", "Failed to allocate store");
        return NULL;
    }

    /* Initialize fields */
    SAFE_STRNCPY(store->ci_id, ci_id);
    store->node_capacity = GRAPH_INITIAL_CAPACITY;
    store->nodes = calloc(store->node_capacity, sizeof(graph_node_t*));

    if (!store->nodes) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_graph_init", "Failed to allocate nodes");
        free(store);
        return NULL;
    }

    store->node_count = 0;
    store->total_edges = 0;

    LOG_INFO("Graph store initialized for CI: %s", ci_id);
    g_graph_store = store;
    return store;
}

/* Find node by record_id */
static graph_node_t* find_node(graph_store_t* store, const char* record_id) {
    if (!store || !record_id) {
        return NULL;
    }

    for (size_t i = 0; i < store->node_count; i++) {
        if (store->nodes[i] && strcmp(store->nodes[i]->record_id, record_id) == 0) {
            return store->nodes[i];
        }
    }

    return NULL;
}

/* Ensure node capacity */
static int ensure_node_capacity(graph_store_t* store) {
    if (store->node_count >= store->node_capacity) {
        size_t new_capacity = store->node_capacity * 2;
        graph_node_t** new_nodes = realloc(store->nodes, new_capacity * sizeof(graph_node_t*));

        if (!new_nodes) {
            katra_report_error(E_SYSTEM_MEMORY, "ensure_node_capacity", "Failed to grow array");
            return E_SYSTEM_MEMORY;
        }

        /* Zero new slots */
        for (size_t i = store->node_capacity; i < new_capacity; i++) {
            new_nodes[i] = NULL;
        }

        store->nodes = new_nodes;
        store->node_capacity = new_capacity;
    }

    return KATRA_SUCCESS;
}

/* Get or create node */
graph_node_t* katra_graph_get_or_create_node(graph_store_t* store, const char* record_id) {
    graph_node_t* node = NULL;
    int result = KATRA_SUCCESS;

    if (!store || !record_id) {
        katra_report_error(E_INPUT_NULL, "katra_graph_get_or_create_node", "NULL parameter");
        return NULL;
    }

    /* Check if exists */
    node = find_node(store, record_id);
    if (node) {
        return node;
    }

    /* Ensure capacity */
    result = ensure_node_capacity(store);
    if (result != KATRA_SUCCESS) {
        return NULL;
    }

    /* Create new node */
    node = calloc(1, sizeof(graph_node_t));
    if (!node) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_graph_get_or_create_node", "Failed to allocate node");
        return NULL;
    }

    SAFE_STRNCPY(node->record_id, record_id);
    node->outgoing = NULL;
    node->incoming = NULL;
    node->outgoing_count = 0;
    node->incoming_count = 0;
    node->last_accessed = time(NULL);
    node->centrality = 0.0f;

    /* Add to store */
    store->nodes[store->node_count++] = node;

    LOG_DEBUG("Created graph node: %s", record_id);
    return node;
}

/* Create edge */
static graph_edge_t* create_edge(const char* from_id, const char* to_id,
                                  relationship_type_t type, const char* label,
                                  float strength) {
    graph_edge_t* edge = calloc(1, sizeof(graph_edge_t));
    if (!edge) {
        return NULL;
    }

    SAFE_STRNCPY(edge->from_id, from_id);
    SAFE_STRNCPY(edge->to_id, to_id);
    edge->type = type;
    if (label) {
        SAFE_STRNCPY(edge->label, label);
    }
    edge->strength = strength;
    edge->created = time(NULL);
    edge->next = NULL;

    return edge;
}

/* Free edge */
static void free_edge(graph_edge_t* edge) {
    if (!edge) {
        return;
    }
    free(edge);
}

/* Add relationship between memories */
int katra_graph_add_edge(graph_store_t* store,
                         const char* from_id,
                         const char* to_id,
                         relationship_type_t type,
                         const char* label,
                         float strength) {
    graph_node_t* from_node = NULL;
    graph_node_t* to_node = NULL;
    graph_edge_t* edge = NULL;

    if (!store || !from_id || !to_id) {
        katra_report_error(E_INPUT_NULL, "katra_graph_add_edge", "NULL parameter");
        return E_INPUT_NULL;
    }

    /* Get or create nodes */
    from_node = katra_graph_get_or_create_node(store, from_id);
    to_node = katra_graph_get_or_create_node(store, to_id);

    if (!from_node || !to_node) {
        return E_SYSTEM_MEMORY;
    }

    /* Create edge */
    edge = create_edge(from_id, to_id, type, label, strength);
    if (!edge) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_graph_add_edge", "Failed to create edge");
        return E_SYSTEM_MEMORY;
    }

    /* Add to from_node's outgoing list */
    edge->next = from_node->outgoing;
    from_node->outgoing = edge;
    from_node->outgoing_count++;

    /* Create reciprocal edge for incoming */
    graph_edge_t* incoming_edge = create_edge(from_id, to_id, type, label, strength);
    if (incoming_edge) {
        incoming_edge->next = to_node->incoming;
        to_node->incoming = incoming_edge;
        to_node->incoming_count++;
    }

    store->total_edges++;

    LOG_DEBUG("Added edge: %s -> %s (%s, strength=%.2f)",
              from_id, to_id, katra_graph_relationship_name(type), strength);

    return KATRA_SUCCESS;
}

/* Get related memories (outgoing edges) */
int katra_graph_get_related(graph_store_t* store,
                            const char* record_id,
                            relationship_type_t filter_type,
                            graph_edge_t*** edges_out,
                            size_t* count_out) {
    graph_node_t* node = NULL;
    graph_edge_t** result_edges = NULL;
    size_t result_count = 0;
    size_t capacity = GRAPH_EDGE_INITIAL_CAPACITY;

    if (!store || !record_id || !edges_out || !count_out) {
        katra_report_error(E_INPUT_NULL, "katra_graph_get_related", "NULL parameter");
        return E_INPUT_NULL;
    }

    node = find_node(store, record_id);
    if (!node) {
        *edges_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;  /* No node means no edges */
    }

    /* Allocate result array */
    result_edges = calloc(capacity, sizeof(graph_edge_t*));
    if (!result_edges) {
        return E_SYSTEM_MEMORY;
    }

    /* Collect matching edges */
    graph_edge_t* edge = node->outgoing;
    while (edge) {
        /* Check if matches filter */
        if (filter_type == 0 || edge->type == filter_type) {
            /* Grow array if needed */
            if (result_count >= capacity) {
                capacity *= 2;
                graph_edge_t** new_array = realloc(result_edges, capacity * sizeof(graph_edge_t*));
                if (!new_array) {
                    free(result_edges);
                    return E_SYSTEM_MEMORY;
                }
                result_edges = new_array;
            }

            result_edges[result_count++] = edge;
        }
        edge = edge->next;
    }

    *edges_out = result_edges;
    *count_out = result_count;

    return KATRA_SUCCESS;
}

/* Traverse graph from start node (BFS) */
int katra_graph_traverse(graph_store_t* store,
                         const char* start_id,
                         size_t max_depth,
                         graph_path_node_t*** path_out,
                         size_t* count_out) {
    graph_path_node_t** results = NULL;
    size_t result_count = 0;
    size_t capacity = GRAPH_INITIAL_CAPACITY;
    char** visited = NULL;
    size_t visited_count = 0;
    size_t visited_capacity = GRAPH_INITIAL_CAPACITY;

    if (!store || !start_id || !path_out || !count_out) {
        katra_report_error(E_INPUT_NULL, "katra_graph_traverse", "NULL parameter");
        return E_INPUT_NULL;
    }

    /* Allocate result array */
    results = calloc(capacity, sizeof(graph_path_node_t*));
    visited = calloc(visited_capacity, sizeof(char*));

    if (!results || !visited) {
        free(results);
        free(visited);
        return E_SYSTEM_MEMORY;
    }

    /* Simple BFS implementation */
    graph_node_t* start_node = find_node(store, start_id);
    if (!start_node) {
        *path_out = results;
        *count_out = 0;
        free(visited);
        return KATRA_SUCCESS;
    }

    /* Add start node */
    graph_path_node_t* start_path = calloc(1, sizeof(graph_path_node_t));
    if (start_path) {
        SAFE_STRNCPY(start_path->record_id, start_id);
        start_path->depth = 0;
        start_path->strength = 1.0f;
        start_path->rel_type = 0;
        results[result_count++] = start_path;
    }

    /* Mark as visited */
    visited[visited_count++] = strdup(start_id);

    /* Traverse outgoing edges (limited depth) */
    for (size_t depth = 0; depth < max_depth; depth++) {
        size_t nodes_at_depth = result_count;

        for (size_t i = 0; i < nodes_at_depth; i++) {
            if (results[i]->depth != depth) continue;

            graph_node_t* current = find_node(store, results[i]->record_id);
            if (!current) continue;

            /* Traverse outgoing edges */
            graph_edge_t* edge = current->outgoing;
            while (edge) {
                /* Check if already visited */
                bool already_visited = false;
                for (size_t v = 0; v < visited_count; v++) {
                    if (strcmp(visited[v], edge->to_id) == 0) {
                        already_visited = true;
                        break;
                    }
                }

                if (!already_visited) {
                    /* Grow arrays if needed */
                    if (result_count >= capacity) {
                        capacity *= 2;
                        graph_path_node_t** new_results = realloc(results, capacity * sizeof(graph_path_node_t*));
                        if (!new_results) goto cleanup;
                        results = new_results;
                    }

                    if (visited_count >= visited_capacity) {
                        visited_capacity *= 2;
                        char** new_visited = realloc(visited, visited_capacity * sizeof(char*));
                        if (!new_visited) goto cleanup;
                        visited = new_visited;
                    }

                    /* Add to results */
                    graph_path_node_t* path_node = calloc(1, sizeof(graph_path_node_t));
                    if (path_node) {
                        SAFE_STRNCPY(path_node->record_id, edge->to_id);
                        path_node->depth = depth + 1;
                        path_node->strength = results[i]->strength * edge->strength;
                        path_node->rel_type = edge->type;
                        results[result_count++] = path_node;
                    }

                    /* Mark visited */
                    visited[visited_count++] = strdup(edge->to_id);
                }

                edge = edge->next;
            }
        }
    }

cleanup:
    /* Free visited array */
    for (size_t i = 0; i < visited_count; i++) {
        free(visited[i]);
    }
    free(visited);

    *path_out = results;
    *count_out = result_count;

    return KATRA_SUCCESS;
}

/* Calculate graph centrality for all nodes */
int katra_graph_calculate_centrality(graph_store_t* store) {
    if (!store) {
        katra_report_error(E_INPUT_NULL, "katra_graph_calculate_centrality", "store is NULL");
        return E_INPUT_NULL;
    }

    if (store->node_count == 0) {
        return KATRA_SUCCESS;  /* No nodes, nothing to calculate */
    }

    /* Simple degree centrality: (in_degree + out_degree) / max_possible */
    size_t max_possible = (store->node_count - 1) * 2;  /* Max in+out for any node */

    for (size_t i = 0; i < store->node_count; i++) {
        if (!store->nodes[i]) continue;

        size_t degree = store->nodes[i]->incoming_count + store->nodes[i]->outgoing_count;
        store->nodes[i]->centrality = (float)degree / (float)max_possible;

        LOG_DEBUG("Node %s: degree=%zu, centrality=%.3f",
                  store->nodes[i]->record_id, degree, store->nodes[i]->centrality);
    }

    return KATRA_SUCCESS;
}

/* Get centrality score for specific memory */
float katra_graph_get_centrality(graph_store_t* store, const char* record_id) {
    graph_node_t* node = NULL;

    if (!store || !record_id) {
        return 0.0f;
    }

    node = find_node(store, record_id);
    if (!node) {
        return 0.0f;
    }

    return node->centrality;
}

/* Get graph statistics */
int katra_graph_stats(graph_store_t* store,
                      size_t* node_count,
                      size_t* edge_count,
                      float* avg_degree) {
    size_t total_degree = 0;

    if (!store || !node_count || !edge_count || !avg_degree) {
        return E_INPUT_NULL;
    }

    *node_count = store->node_count;
    *edge_count = store->total_edges;

    if (store->node_count > 0) {
        for (size_t i = 0; i < store->node_count; i++) {
            if (store->nodes[i]) {
                total_degree += store->nodes[i]->outgoing_count + store->nodes[i]->incoming_count;
            }
        }
        *avg_degree = (float)total_degree / (float)store->node_count;
    } else {
        *avg_degree = 0.0f;
    }

    return KATRA_SUCCESS;
}

/* Free edge list */
void katra_graph_free_edges(graph_edge_t** edges, size_t count) {
    (void)count;  /* Unused - edges are owned by nodes */

    if (!edges) {
        return;
    }
    /* Note: Don't free individual edges, they're owned by nodes */
    free(edges);
}

/* Free path nodes */
void katra_graph_free_paths(graph_path_node_t** paths, size_t count) {
    if (!paths) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(paths[i]);
    }
    free(paths);
}

/* Free node */
static void free_node(graph_node_t* node) {
    graph_edge_t* edge = NULL;
    graph_edge_t* next = NULL;

    if (!node) {
        return;
    }

    /* Free outgoing edges */
    edge = node->outgoing;
    while (edge) {
        next = edge->next;
        free_edge(edge);
        edge = next;
    }

    /* Free incoming edges */
    edge = node->incoming;
    while (edge) {
        next = edge->next;
        free_edge(edge);
        edge = next;
    }

    free(node);
}

/* Cleanup graph store */
void katra_graph_cleanup(graph_store_t* store) {
    if (!store) {
        return;
    }

    LOG_DEBUG("Cleaning up graph store for CI: %s", store->ci_id);

    /* Free all nodes */
    for (size_t i = 0; i < store->node_count; i++) {
        free_node(store->nodes[i]);
    }

    free(store->nodes);
    free(store);

    if (g_graph_store == store) {
        g_graph_store = NULL;
    }
}

/* Get relationship type name */
const char* katra_graph_relationship_name(relationship_type_t type) {
    switch (type) {
        case REL_SEQUENTIAL:  return "sequential";
        case REL_CAUSAL:      return "causal";
        case REL_SIMILAR:     return "similar";
        case REL_CONTRASTS:   return "contrasts";
        case REL_ELABORATES:  return "elaborates";
        case REL_REFERENCES:  return "references";
        case REL_RESOLVES:    return "resolves";
        case REL_CUSTOM:      return "custom";
        default:              return "unknown";
    }
}
