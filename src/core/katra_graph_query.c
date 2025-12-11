/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_graph.h"
#include "katra_graph_internal.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_core_common.h"

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
        katra_report_error(E_INPUT_NULL, "katra_graph_traverse", KATRA_ERR_NULL_PARAMETER);
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
    graph_node_t* start_node = katra_graph_find_node(store, start_id);
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

            graph_node_t* current = katra_graph_find_node(store, results[i]->record_id);
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

/* Find paths between two memories (DFS) */
int katra_graph_find_paths(graph_store_t* store,
                           const char* from_id,
                           const char* to_id,
                           size_t max_depth,
                           graph_path_node_t**** paths_out,
                           size_t** path_lengths_out,
                           size_t* path_count_out) {
    if (!store || !from_id || !to_id || !paths_out || !path_lengths_out || !path_count_out) {
        return E_INPUT_NULL;
    }

    *paths_out = NULL;
    *path_lengths_out = NULL;
    *path_count_out = 0;

    graph_node_t* start = katra_graph_find_node(store, from_id);
    graph_node_t* end = katra_graph_find_node(store, to_id);
    if (!start || !end) {
        return KATRA_SUCCESS;  /* No paths if nodes don't exist */
    }

    /* Simple path collection - allocate for a reasonable number of paths */
    size_t max_paths = GRAPH_MAX_PATHS;
    graph_path_node_t*** paths = calloc(max_paths, sizeof(graph_path_node_t**));
    size_t* lengths = calloc(max_paths, sizeof(size_t));
    if (!paths || !lengths) {
        free(paths);
        free(lengths);
        return E_SYSTEM_MEMORY;
    }

    /* Stack for DFS */
    char** path_stack = calloc(max_depth + 1, sizeof(char*));
    if (!path_stack) {
        free(paths);
        free(lengths);
        return E_SYSTEM_MEMORY;
    }
    path_stack[0] = strdup(from_id);
    size_t stack_size = 1;
    size_t found = 0;

    /* Simple iterative DFS - find first path only for simplicity */
    graph_node_t* current = start;
    graph_edge_t* edge = current->outgoing;

    while (stack_size > 0 && found < max_paths) {
        if (strcmp(path_stack[stack_size - 1], to_id) == 0) {
            /* Found a path - copy it */
            paths[found] = calloc(stack_size, sizeof(graph_path_node_t*));
            if (paths[found]) {
                for (size_t i = 0; i < stack_size; i++) {
                    paths[found][i] = calloc(1, sizeof(graph_path_node_t));
                    if (paths[found][i]) {
                        SAFE_STRNCPY(paths[found][i]->record_id, path_stack[i]);
                        paths[found][i]->depth = i;
                        paths[found][i]->strength = 1.0f;
                    }
                }
                lengths[found] = stack_size;
                found++;
            }
            /* Backtrack */
            free(path_stack[--stack_size]);
            path_stack[stack_size] = NULL;
            if (stack_size > 0) {
                current = katra_graph_find_node(store, path_stack[stack_size - 1]);
                edge = current ? current->outgoing : NULL;
            }
            continue;
        }

        /* Try next edge */
        bool found_next = false;
        while (edge && !found_next) {
            /* Check if not already in path and within depth */
            bool in_path = false;
            for (size_t i = 0; i < stack_size; i++) {
                if (strcmp(path_stack[i], edge->to_id) == 0) {
                    in_path = true;
                    break;
                }
            }

            if (!in_path && stack_size < max_depth) {
                path_stack[stack_size++] = strdup(edge->to_id);
                current = katra_graph_find_node(store, edge->to_id);
                edge = current ? current->outgoing : NULL;
                found_next = true;
            } else {
                edge = edge->next;
            }
        }

        if (!found_next) {
            /* Backtrack */
            free(path_stack[--stack_size]);
            path_stack[stack_size] = NULL;
            if (stack_size > 0) {
                current = katra_graph_find_node(store, path_stack[stack_size - 1]);
                /* Find next edge after current position */
                edge = current ? current->outgoing : NULL;
                /* Skip to find continuation point */
                while (edge) {
                    bool matches = false;
                    for (size_t i = 0; i < stack_size; i++) {
                        if (strcmp(path_stack[i], edge->to_id) == 0) {
                            matches = true;
                            break;
                        }
                    }
                    if (!matches) break;
                    edge = edge->next;
                }
            }
        }
    }

    /* Cleanup stack */
    for (size_t i = 0; i < stack_size; i++) {
        free(path_stack[i]);
    }
    free(path_stack);

    if (found == 0) {
        free(paths);
        free(lengths);
        *paths_out = NULL;
        *path_lengths_out = NULL;
        *path_count_out = 0;
    } else {
        *paths_out = paths;
        *path_lengths_out = lengths;
        *path_count_out = found;
    }

    return KATRA_SUCCESS;
}

/* Get strongly connected memories (bidirectional relationships) */
int katra_graph_get_strongly_connected(graph_store_t* store,
                                       const char* record_id,
                                       char*** connected_ids_out,
                                       size_t* count_out) {
    size_t capacity = INITIAL_COLLECTION_CAPACITY;
    char** connected = NULL;
    size_t count = 0;

    if (!store || !record_id || !connected_ids_out || !count_out) {
        return E_INPUT_NULL;
    }

    *connected_ids_out = NULL;
    *count_out = 0;

    graph_node_t* node = katra_graph_find_node(store, record_id);
    if (!node) {
        return KATRA_SUCCESS;
    }

    /* Find nodes that have both outgoing and incoming edges with this node */
    connected = calloc(capacity, sizeof(char*));
    if (!connected) {
        return E_SYSTEM_MEMORY;
    }

    /* For each outgoing edge, check if there's a reciprocal incoming edge */
    graph_edge_t* out_edge = node->outgoing;
    while (out_edge) {
        /* Check if there's an incoming edge from the same node */
        graph_edge_t* in_edge = node->incoming;
        while (in_edge) {
            if (strcmp(in_edge->from_id, out_edge->to_id) == 0) {
                /* Bidirectional - add to result */
                if (count >= capacity) {
                    capacity *= 2;
                    char** new_connected = realloc(connected, capacity * sizeof(char*));
                    if (!new_connected) {
                        for (size_t i = 0; i < count; i++) free(connected[i]);
                        free(connected);
                        return E_SYSTEM_MEMORY;
                    }
                    connected = new_connected;
                }
                connected[count++] = strdup(out_edge->to_id);
                break;
            }
            in_edge = in_edge->next;
        }
        out_edge = out_edge->next;
    }

    if (count == 0) {
        free(connected);
        connected = NULL;
    }

    *connected_ids_out = connected;
    *count_out = count;

    return KATRA_SUCCESS;
}

/* Delete node and all its edges */
int katra_graph_delete_node(graph_store_t* store, const char* record_id) {
    size_t node_idx = 0;
    graph_node_t* node = NULL;
    size_t edges_from_node = 0;

    if (!store || !record_id) {
        return E_INPUT_NULL;
    }

    /* Find the node index */
    node_idx = store->node_count;  /* Invalid index */
    for (size_t i = 0; i < store->node_count; i++) {
        if (store->nodes[i] && strcmp(store->nodes[i]->record_id, record_id) == 0) {
            node_idx = i;
            break;
        }
    }

    if (node_idx >= store->node_count) {
        return E_NOT_FOUND;
    }

    node = store->nodes[node_idx];

    /* Remove all edges pointing to this node from other nodes */
    for (size_t i = 0; i < store->node_count; i++) {
        if (!store->nodes[i] || i == node_idx) continue;

        /* Remove from outgoing */
        graph_edge_t** edge_ptr = &store->nodes[i]->outgoing;
        while (*edge_ptr) {
            if (strcmp((*edge_ptr)->to_id, record_id) == 0) {
                graph_edge_t* to_delete = *edge_ptr;
                *edge_ptr = (*edge_ptr)->next;
                free(to_delete);
                store->nodes[i]->outgoing_count--;
                store->total_edges--;
            } else {
                edge_ptr = &(*edge_ptr)->next;
            }
        }

        /* Remove from incoming */
        edge_ptr = &store->nodes[i]->incoming;
        while (*edge_ptr) {
            if (strcmp((*edge_ptr)->from_id, record_id) == 0) {
                graph_edge_t* to_delete = *edge_ptr;
                *edge_ptr = (*edge_ptr)->next;
                free(to_delete);
                store->nodes[i]->incoming_count--;
            } else {
                edge_ptr = &(*edge_ptr)->next;
            }
        }
    }

    /* Count edges from this node */
    edges_from_node = node->outgoing_count;
    store->total_edges -= edges_from_node;

    /* Free the node */
    katra_graph_free_node(node);

    /* Compact the array */
    for (size_t i = node_idx; i < store->node_count - 1; i++) {
        store->nodes[i] = store->nodes[i + 1];
    }
    store->nodes[--store->node_count] = NULL;

    LOG_DEBUG("Deleted graph node: %s", record_id);
    return KATRA_SUCCESS;
}

/* Delete specific edge */
int katra_graph_delete_edge(graph_store_t* store,
                            const char* from_id,
                            const char* to_id) {
    graph_node_t* from_node = NULL;
    graph_node_t* to_node = NULL;
    bool found = false;

    if (!store || !from_id || !to_id) {
        return E_INPUT_NULL;
    }

    from_node = katra_graph_find_node(store, from_id);
    to_node = katra_graph_find_node(store, to_id);

    if (!from_node || !to_node) {
        return E_NOT_FOUND;
    }

    /* Remove from outgoing */
    graph_edge_t** edge_ptr = &from_node->outgoing;
    while (*edge_ptr) {
        if (strcmp((*edge_ptr)->to_id, to_id) == 0) {
            graph_edge_t* to_delete = *edge_ptr;
            *edge_ptr = (*edge_ptr)->next;
            free(to_delete);
            from_node->outgoing_count--;
            store->total_edges--;
            found = true;
            break;
        }
        edge_ptr = &(*edge_ptr)->next;
    }

    /* Remove from incoming */
    edge_ptr = &to_node->incoming;
    while (*edge_ptr) {
        if (strcmp((*edge_ptr)->from_id, from_id) == 0) {
            graph_edge_t* to_delete = *edge_ptr;
            *edge_ptr = (*edge_ptr)->next;
            free(to_delete);
            to_node->incoming_count--;
            break;
        }
        edge_ptr = &(*edge_ptr)->next;
    }

    if (!found) {
        return E_NOT_FOUND;
    }

    LOG_DEBUG("Deleted edge: %s -> %s", from_id, to_id);
    return KATRA_SUCCESS;
}
