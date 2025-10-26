/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_GRAPH_H
#define KATRA_GRAPH_H

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

/* Graph database for memory associations and relationships */

/* Relationship types */
typedef enum {
    REL_SEQUENTIAL,         /* A followed by B */
    REL_CAUSAL,            /* A caused B */
    REL_SIMILAR,           /* A is similar to B */
    REL_CONTRASTS,         /* A contrasts with B */
    REL_ELABORATES,        /* A elaborates on B */
    REL_REFERENCES,        /* A references B */
    REL_RESOLVES,          /* A resolves B (question->answer) */
    REL_CUSTOM             /* Custom relationship */
} relationship_type_t;

/* Graph edge (relationship between memories) */
typedef struct graph_edge {
    char from_id[256];          /* Source memory ID */
    char to_id[256];            /* Target memory ID */
    relationship_type_t type;   /* Relationship type */
    char label[128];            /* Human-readable label */
    float strength;             /* Relationship strength (0.0-1.0) */
    time_t created;             /* When association created */
    struct graph_edge* next;    /* Next edge (linked list) */
} graph_edge_t;

/* Graph node (memory with associations) */
typedef struct graph_node {
    char record_id[256];        /* Memory record ID */
    graph_edge_t* outgoing;     /* Edges from this node */
    graph_edge_t* incoming;     /* Edges to this node */
    size_t outgoing_count;      /* Number of outgoing edges */
    size_t incoming_count;      /* Number of incoming edges */
    time_t last_accessed;       /* Access tracking */
} graph_node_t;

/* Graph store context */
typedef struct {
    char ci_id[256];            /* CI identifier */
    graph_node_t** nodes;       /* Node array */
    size_t node_count;          /* Number of nodes */
    size_t node_capacity;       /* Allocated capacity */
    size_t total_edges;         /* Total edges in graph */
} graph_store_t;

/* Traversal result */
typedef struct {
    char record_id[256];        /* Node record ID */
    size_t depth;               /* Distance from start */
    float strength;             /* Path strength */
    relationship_type_t rel_type;  /* Relationship type */
} graph_path_node_t;

/* Initialize graph store */
graph_store_t* katra_graph_init(const char* ci_id);

/* Create or get node */
graph_node_t* katra_graph_get_or_create_node(graph_store_t* store,
                                              const char* record_id);

/* Add relationship between memories */
int katra_graph_add_edge(graph_store_t* store,
                         const char* from_id,
                         const char* to_id,
                         relationship_type_t type,
                         const char* label,
                         float strength);

/* Get related memories (outgoing edges) */
int katra_graph_get_related(graph_store_t* store,
                            const char* record_id,
                            relationship_type_t filter_type,
                            graph_edge_t*** edges_out,
                            size_t* count_out);

/* Traverse graph from start node */
int katra_graph_traverse(graph_store_t* store,
                         const char* start_id,
                         size_t max_depth,
                         graph_path_node_t*** path_out,
                         size_t* count_out);

/* Find paths between two memories */
int katra_graph_find_paths(graph_store_t* store,
                           const char* from_id,
                           const char* to_id,
                           size_t max_depth,
                           graph_path_node_t**** paths_out,
                           size_t** path_lengths_out,
                           size_t* path_count_out);

/* Get strongly connected memories (bidirectional relationships) */
int katra_graph_get_strongly_connected(graph_store_t* store,
                                       const char* record_id,
                                       char*** connected_ids_out,
                                       size_t* count_out);

/* Delete node and all its edges */
int katra_graph_delete_node(graph_store_t* store, const char* record_id);

/* Delete specific edge */
int katra_graph_delete_edge(graph_store_t* store,
                            const char* from_id,
                            const char* to_id);

/* Get graph statistics */
int katra_graph_stats(graph_store_t* store,
                      size_t* node_count,
                      size_t* edge_count,
                      float* avg_degree);

/* Free edge list */
void katra_graph_free_edges(graph_edge_t** edges, size_t count);

/* Free path nodes */
void katra_graph_free_paths(graph_path_node_t** paths, size_t count);

/* Cleanup graph store */
void katra_graph_cleanup(graph_store_t* store);

/* Helper: Get relationship type name */
const char* katra_graph_relationship_name(relationship_type_t type);

#endif /* KATRA_GRAPH_H */
