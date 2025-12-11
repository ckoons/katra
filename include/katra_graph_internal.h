/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_GRAPH_INTERNAL_H
#define KATRA_GRAPH_INTERNAL_H

#include "katra_graph.h"

/* Graph constants */
#define GRAPH_INITIAL_CAPACITY 128
#define GRAPH_EDGE_INITIAL_CAPACITY 8
#define GRAPH_MAX_PATHS 10

/* Internal functions shared between katra_graph.c and katra_graph_query.c */

/* Find node by record_id - internal lookup */
graph_node_t* katra_graph_find_node(graph_store_t* store, const char* record_id);

/* Free a single node and its edges - internal cleanup */
void katra_graph_free_node(graph_node_t* node);

#endif /* KATRA_GRAPH_INTERNAL_H */
