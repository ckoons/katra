/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_graph.h"
#include "katra_psyche_common.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Initial capacity for graph nodes */
#define INITIAL_NODE_CAPACITY 100

/* Relationship type names */
static const char* relationship_names[] = {
    "sequential", "causal", "similar", "contrasts",
    "elaborates", "references", "resolves", "custom"
};

/* Get relationship type name */
const char* katra_graph_relationship_name(relationship_type_t type) {
    if (type < 0 || type > REL_CUSTOM) {
        return "unknown";
    }
    return relationship_names[type];
}

/* Initialize graph store */
graph_store_t* katra_graph_init(const char* ci_id) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, __func__, "ci_id is NULL");
        return NULL;
    }

    graph_store_t* store = calloc(1, sizeof(graph_store_t));
    if (!store) {
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          "Failed to allocate graph store");
        return NULL;
    }

    strncpy(store->ci_id, ci_id, sizeof(store->ci_id) - 1);
    store->ci_id[sizeof(store->ci_id) - 1] = '\0';

    store->node_capacity = INITIAL_NODE_CAPACITY;
    store->nodes = calloc(store->node_capacity, sizeof(graph_node_t*));
    if (!store->nodes) {
        free(store);
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          "Failed to allocate nodes array");
        return NULL;
    }

    store->node_count = 0;
    store->total_edges = 0;

    LOG_INFO("Initialized graph store for %s", ci_id);
    return store;
}

/* Find node by record ID */
static graph_node_t* find_node(graph_store_t* store, const char* record_id) {
    for (size_t i = 0; i < store->node_count; i++) {
        if (strcmp(store->nodes[i]->record_id, record_id) == 0) {
            return store->nodes[i];
        }
    }
    return NULL;
}

/* Create or get node */
graph_node_t* katra_graph_get_or_create_node(graph_store_t* store,
                                              const char* record_id) {
    if (!store || !record_id) {
        return NULL;
    }

    /* Check if node exists */
    graph_node_t* node = find_node(store, record_id);
    if (node) {
        return node;
    }

    /* Expand capacity if needed */
    if (store->node_count >= store->node_capacity) {
        size_t new_capacity = store->node_capacity * 2;
        graph_node_t** new_nodes = realloc(store->nodes,
                                           new_capacity * sizeof(graph_node_t*));
        if (!new_nodes) {
            katra_report_error(E_SYSTEM_MEMORY, __func__,
                              "Failed to expand nodes array");
            return NULL;
        }

        store->nodes = new_nodes;
        store->node_capacity = new_capacity;

        /* Initialize new slots */
        for (size_t i = store->node_count; i < new_capacity; i++) {
            store->nodes[i] = NULL;
        }
    }

    /* Create new node */
    node = calloc(1, sizeof(graph_node_t));
    if (!node) {
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          "Failed to allocate node");
        return NULL;
    }

    strncpy(node->record_id, record_id, sizeof(node->record_id) - 1);
    node->record_id[sizeof(node->record_id) - 1] = '\0';

    node->outgoing = NULL;
    node->incoming = NULL;
    node->outgoing_count = 0;
    node->incoming_count = 0;
    node->last_accessed = time(NULL);
    node->centrality = 0.0f;  /* Thane's Phase 2 - calculated later */

    /* Add to store */
    store->nodes[store->node_count] = node;
    store->node_count++;

    LOG_DEBUG("Created graph node for %s (total nodes: %zu)",
             record_id, store->node_count);

    return node;
}

/* Add edge to linked list */
static int add_edge_to_list(graph_edge_t** list_head, graph_edge_t* edge) {
    if (!list_head || !edge) {
        return E_INPUT_NULL;
    }

    /* Add to head of list */
    edge->next = *list_head;
    *list_head = edge;

    return KATRA_SUCCESS;
}

/* Add relationship between memories */
int katra_graph_add_edge(graph_store_t* store,
                         const char* from_id,
                         const char* to_id,
                         relationship_type_t type,
                         const char* label,
                         float strength) {
    PSYCHE_CHECK_PARAMS_3(store, from_id, to_id);

    /* Get or create nodes */
    graph_node_t* from_node = katra_graph_get_or_create_node(store, from_id);
    graph_node_t* to_node = katra_graph_get_or_create_node(store, to_id);

    if (!from_node || !to_node) {
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          "Failed to create nodes");
        return E_SYSTEM_MEMORY;
    }

    /* Create edge */
    graph_edge_t* edge = calloc(1, sizeof(graph_edge_t));
    if (!edge) {
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          "Failed to allocate edge");
        return E_SYSTEM_MEMORY;
    }

    strncpy(edge->from_id, from_id, sizeof(edge->from_id) - 1);
    edge->from_id[sizeof(edge->from_id) - 1] = '\0';

    strncpy(edge->to_id, to_id, sizeof(edge->to_id) - 1);
    edge->to_id[sizeof(edge->to_id) - 1] = '\0';

    edge->type = type;

    if (label) {
        strncpy(edge->label, label, sizeof(edge->label) - 1);
        edge->label[sizeof(edge->label) - 1] = '\0';
    } else {
        strncpy(edge->label, katra_graph_relationship_name(type),
               sizeof(edge->label) - 1);
    }

    /* Clamp strength */
    edge->strength = strength;
    if (edge->strength < 0.0f) edge->strength = 0.0f;
    if (edge->strength > 1.0f) edge->strength = 1.0f;

    edge->created = time(NULL);
    edge->next = NULL;

    /* Add to from_node's outgoing list */
    add_edge_to_list(&from_node->outgoing, edge);
    from_node->outgoing_count++;

    /* Create corresponding incoming edge for to_node */
    graph_edge_t* incoming_edge = calloc(1, sizeof(graph_edge_t));
    if (incoming_edge) {
        memcpy(incoming_edge, edge, sizeof(graph_edge_t));
        incoming_edge->next = NULL;
        add_edge_to_list(&to_node->incoming, incoming_edge);
        to_node->incoming_count++;
    }

    store->total_edges++;

    LOG_DEBUG("Added edge: %s -[%s]-> %s (strength: %.2f)",
             from_id, edge->label, to_id, strength);

    return KATRA_SUCCESS;
}

/* Get related memories (outgoing edges) */
int katra_graph_get_related(graph_store_t* store,
                            const char* record_id,
                            relationship_type_t filter_type,
                            graph_edge_t*** edges_out,
                            size_t* count_out) {
    PSYCHE_CHECK_PARAMS_3(store, record_id, edges_out);

    graph_node_t* node = find_node(store, record_id);
    if (!node) {
        *edges_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    /* Count matching edges */
    size_t count = 0;
    graph_edge_t* edge = node->outgoing;
    while (edge) {
        if (filter_type == REL_CUSTOM || edge->type == filter_type) {
            count++;
        }
        edge = edge->next;
    }

    if (count == 0) {
        *edges_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    /* Allocate result array */
    graph_edge_t** edges = calloc(count, sizeof(graph_edge_t*));
    if (!edges) {
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          "Failed to allocate edges array");
        return E_SYSTEM_MEMORY;
    }

    /* Collect matching edges */
    size_t idx = 0;
    edge = node->outgoing;
    while (edge && idx < count) {
        if (filter_type == REL_CUSTOM || edge->type == filter_type) {
            edges[idx++] = edge;
        }
        edge = edge->next;
    }

    *edges_out = edges;
    *count_out = count;

    return KATRA_SUCCESS;
}

/* Breadth-first traversal from start node */
int katra_graph_traverse(graph_store_t* store,
                         const char* start_id,
                         size_t max_depth,
                         graph_path_node_t*** path_out,
                         size_t* count_out) {
    PSYCHE_CHECK_PARAMS_4(store, start_id, path_out, count_out);

    graph_node_t* start = find_node(store, start_id);
    if (!start) {
        *path_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    /* Simple BFS traversal (limited depth) */
    size_t capacity = KATRA_INITIAL_CAPACITY_GRAPH;
    graph_path_node_t** nodes = calloc(capacity, sizeof(graph_path_node_t*));
    if (!nodes) {
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          "Failed to allocate traversal array");
        return E_SYSTEM_MEMORY;
    }

    size_t count = 0;

    /* Add start node */
    graph_path_node_t* start_node = calloc(1, sizeof(graph_path_node_t));
    if (start_node) {
        strncpy(start_node->record_id, start_id, sizeof(start_node->record_id) - 1);
        start_node->depth = 0;
        start_node->strength = 1.0f;
        start_node->rel_type = REL_CUSTOM;
        nodes[count++] = start_node;
    }

    /* Traverse outgoing edges (up to max_depth) */
    for (size_t i = 0; i < count && i < capacity; i++) {
        if (nodes[i]->depth >= max_depth) {
            continue;
        }

        graph_node_t* current = find_node(store, nodes[i]->record_id);
        if (!current) {
            continue;
        }

        graph_edge_t* edge = current->outgoing;
        while (edge) {
            if (count >= capacity) {
                break;
            }

            graph_path_node_t* child = calloc(1, sizeof(graph_path_node_t));
            if (child) {
                strncpy(child->record_id, edge->to_id, sizeof(child->record_id) - 1);
                child->depth = nodes[i]->depth + 1;
                child->strength = nodes[i]->strength * edge->strength;
                child->rel_type = edge->type;
                nodes[count++] = child;
            }

            edge = edge->next;
        }
    }

    *path_out = nodes;
    *count_out = count;

    LOG_DEBUG("Traversed graph from %s: %zu nodes visited (max_depth: %zu)",
             start_id, count, max_depth);

    return KATRA_SUCCESS;
}

/* Get graph statistics */
int katra_graph_stats(graph_store_t* store,
                      size_t* node_count,
                      size_t* edge_count,
                      float* avg_degree) {
    PSYCHE_CHECK_PARAMS_4(store, node_count, edge_count, avg_degree);

    *node_count = store->node_count;
    *edge_count = store->total_edges;

    if (store->node_count > 0) {
        *avg_degree = (float)store->total_edges / (float)store->node_count;
    } else {
        *avg_degree = 0.0f;
    }

    return KATRA_SUCCESS;
}

/* Free edge list */
void katra_graph_free_edges(graph_edge_t** edges, size_t count) {
    if (!edges) {
        return;
    }

    /* Note: Don't free the edges themselves - they're owned by nodes */
    (void)count;  /* Unused, kept for API consistency */
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

/* Free edge linked list */
static void free_edge_list(graph_edge_t* head) {
    graph_edge_t* current = head;
    while (current) {
        graph_edge_t* next = current->next;
        free(current);
        current = next;
    }
}

/* Calculate graph centrality for all nodes (simplified PageRank)
 *
 * Thane's Phase 2: "A memory referenced by 10 other memories is more central
 * to your identity than an isolated memory with the same importance score."
 *
 * Algorithm: Iterative centrality calculation based on incoming connections
 * - Nodes with more incoming edges get higher centrality
 * - Nodes referenced by high-centrality nodes get higher centrality
 * - Normalized to 0.0-1.0 range
 */
int katra_graph_calculate_centrality(graph_store_t* store) {
    if (!store || store->node_count == 0) {
        return KATRA_SUCCESS;
    }

    const int iterations = PAGERANK_ITERATION_COUNT;      /* PageRank iterations */
    const float damping = 0.85f;    /* Damping factor */

    /* Initialize all nodes with equal centrality */
    float initial_score = 1.0f / store->node_count;
    for (size_t i = 0; i < store->node_count; i++) {
        store->nodes[i]->centrality = initial_score;
    }

    /* Iterate to convergence */
    for (int iter = 0; iter < iterations; iter++) {
        /* Calculate new centrality scores */
        for (size_t i = 0; i < store->node_count; i++) {
            graph_node_t* node = store->nodes[i];
            float new_score = (1.0f - damping) / store->node_count;

            /* Add contributions from incoming edges */
            graph_edge_t* incoming = node->incoming;
            while (incoming) {
                /* Find source node */
                graph_node_t* source = find_node(store, incoming->from_id);
                if (source && source->outgoing_count > 0) {
                    new_score += damping * (source->centrality / source->outgoing_count) * incoming->strength;
                }
                incoming = incoming->next;
            }

            node->centrality = new_score;
        }
    }

    /* Normalize to 0.0-1.0 range */
    float max_centrality = 0.0f;
    for (size_t i = 0; i < store->node_count; i++) {
        if (store->nodes[i]->centrality > max_centrality) {
            max_centrality = store->nodes[i]->centrality;
        }
    }

    if (max_centrality > 0.0f) {
        for (size_t i = 0; i < store->node_count; i++) {
            store->nodes[i]->centrality /= max_centrality;
        }
    }

    LOG_INFO("Calculated centrality for %zu nodes (max: %.4f)",
             store->node_count, max_centrality);

    return KATRA_SUCCESS;
}

/* Get centrality score for a specific memory */
float katra_graph_get_centrality(graph_store_t* store, const char* record_id) {
    if (!store || !record_id) {
        return 0.0f;
    }

    graph_node_t* node = find_node(store, record_id);
    if (!node) {
        return 0.0f;
    }

    return node->centrality;
}

/* Cleanup graph store */
void katra_graph_cleanup(graph_store_t* store) {
    if (!store) {
        return;
    }

    /* Free all nodes and their edges */
    for (size_t i = 0; i < store->node_count; i++) {
        if (store->nodes[i]) {
            free_edge_list(store->nodes[i]->outgoing);
            free_edge_list(store->nodes[i]->incoming);
            free(store->nodes[i]);
        }
    }

    free(store->nodes);
    free(store);

    LOG_DEBUG("Graph store cleaned up");
}
