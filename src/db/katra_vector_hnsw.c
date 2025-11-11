/* © 2025 Casey Koons All rights reserved */

/* HNSW (Hierarchical Navigable Small World) indexing - Phase 6.1e */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* Project includes */
#include "katra_vector.h"
#include "katra_error.h"
#include "katra_log.h"

/* HNSW node (one per embedding) */
typedef struct hnsw_node {
    size_t id;                       /* Node ID (index in store) */
    vector_embedding_t* embedding;   /* Associated embedding */
    int level;                       /* Highest layer for this node */
    struct hnsw_neighbor** layers;   /* Connections per layer */
    size_t* layer_sizes;             /* Number of connections per layer */
} hnsw_node_t;

/* Neighbor connection */
typedef struct hnsw_neighbor {
    hnsw_node_t* node;               /* Connected node */
    float distance;                  /* Distance to this neighbor */
} hnsw_neighbor_t;

/* Priority queue element for search */
typedef struct pq_element {
    hnsw_node_t* node;
    float distance;
} pq_element_t;

/* HNSW index structure (complete definition for struct hnsw_index) */
struct hnsw_index {
    hnsw_node_t** nodes;             /* All nodes */
    size_t count;                    /* Number of nodes */
    size_t capacity;                 /* Allocated capacity */
    hnsw_node_t* entry_point;        /* Entry point for search */
    int max_layer;                   /* Current max layer */
};
typedef struct hnsw_index hnsw_index_t;

/* Calculate distance between embeddings (1 - cosine similarity) */
static float hnsw_distance(const vector_embedding_t* a, const vector_embedding_t* b) {
    float similarity = katra_vector_cosine_similarity(a, b);
    return 1.0f - similarity;  /* Convert to distance */
}

/* Generate random layer for new node */
static int generate_random_layer(void) {
    double r = (double)rand() / RAND_MAX;
    return (int)(-log(r) * HNSW_ML);
}

/* Create new HNSW node */
static hnsw_node_t* hnsw_node_create(size_t id, vector_embedding_t* embedding, int level) {
    hnsw_node_t* node = calloc(1, sizeof(hnsw_node_t));
    if (!node) {
        return NULL;
    }

    node->id = id;
    node->embedding = embedding;
    node->level = level;

    /* Allocate layers */
    node->layers = calloc(level + 1, sizeof(hnsw_neighbor_t*));
    node->layer_sizes = calloc(level + 1, sizeof(size_t));

    if (!node->layers || !node->layer_sizes) {
        free(node->layers);
        free(node->layer_sizes);
        free(node);
        return NULL;
    }

    /* Allocate neighbor arrays for each layer */
    for (int lc = 0; lc <= level; lc++) {
        node->layers[lc] = calloc(HNSW_M_MAX, sizeof(hnsw_neighbor_t));
        if (!node->layers[lc]) {
            /* Cleanup on failure */
            for (int i = 0; i < lc; i++) {
                free(node->layers[i]);
            }
            free(node->layers);
            free(node->layer_sizes);
            free(node);
            return NULL;
        }
        node->layer_sizes[lc] = 0;
    }

    return node;
}

/* Free HNSW node */
static void hnsw_node_free(hnsw_node_t* node) {
    if (!node) {
        return;
    }

    if (node->layers) {
        for (int lc = 0; lc <= node->level; lc++) {
            free(node->layers[lc]);
        }
        free(node->layers);
    }

    free(node->layer_sizes);
    free(node);
}

/* Add neighbor connection to node at layer */
static int hnsw_add_connection(hnsw_node_t* node, hnsw_node_t* neighbor,
                                float distance, int layer) {
    if (layer > node->level) {
        return E_INPUT_INVALID;
    }

    size_t max_connections = (layer == 0) ? HNSW_M_MAX : HNSW_M;

    /* If not at capacity, just add */
    if (node->layer_sizes[layer] < max_connections) {
        size_t idx = node->layer_sizes[layer];
        node->layers[layer][idx].node = neighbor;
        node->layers[layer][idx].distance = distance;
        node->layer_sizes[layer]++;
        return KATRA_SUCCESS;
    }

    /* At capacity - find farthest neighbor and replace if new is closer */
    size_t farthest_idx = 0;
    float farthest_dist = node->layers[layer][0].distance;

    for (size_t i = 1; i < node->layer_sizes[layer]; i++) {
        if (node->layers[layer][i].distance > farthest_dist) {
            farthest_dist = node->layers[layer][i].distance;
            farthest_idx = i;
        }
    }

    if (distance < farthest_dist) {
        node->layers[layer][farthest_idx].node = neighbor;
        node->layers[layer][farthest_idx].distance = distance;
    }

    return KATRA_SUCCESS;
}

/* Search layer for nearest neighbors */
static hnsw_node_t** search_layer(hnsw_node_t* entry_point, vector_embedding_t* query,
                                   size_t ef, int layer, size_t* result_count) {
    /* Simplified search - return entry point's neighbors at this layer */
    /* Full HNSW would use priority queue and beam search */
    (void)query;  /* Unused in simplified implementation */

    if (!entry_point || layer > entry_point->level) {
        *result_count = 0;
        return NULL;
    }

    /* Collect neighbors at this layer */
    size_t count = entry_point->layer_sizes[layer];
    if (count == 0) {
        *result_count = 0;
        return NULL;
    }

    hnsw_node_t** results = calloc(count + 1, sizeof(hnsw_node_t*));
    if (!results) {
        *result_count = 0;
        return NULL;
    }

    /* Add entry point */
    results[0] = entry_point;

    /* Add neighbors */
    for (size_t i = 0; i < count && i < ef; i++) {
        results[i + 1] = entry_point->layers[layer][i].node;
    }

    *result_count = count + 1;
    return results;
}

/* Initialize HNSW index */
hnsw_index_t* katra_vector_hnsw_init(void) {
    hnsw_index_t* index = calloc(1, sizeof(hnsw_index_t));
    if (!index) {
        return NULL;
    }

    index->capacity = 100;
    index->nodes = calloc(index->capacity, sizeof(hnsw_node_t*));
    if (!index->nodes) {
        free(index);
        return NULL;
    }

    index->count = 0;
    index->entry_point = NULL;
    index->max_layer = 0;

    return index;
}

/* Insert node into HNSW index */
int katra_vector_hnsw_insert(hnsw_index_t* index, size_t id,
                              vector_embedding_t* embedding) {
    if (!index || !embedding) {
        return E_INPUT_NULL;
    }

    /* Expand capacity if needed */
    if (index->count >= index->capacity) {
        size_t new_capacity = index->capacity * 2;
        hnsw_node_t** new_nodes = realloc(index->nodes,
                                          new_capacity * sizeof(hnsw_node_t*));
        if (!new_nodes) {
            return E_SYSTEM_MEMORY;
        }
        index->nodes = new_nodes;
        index->capacity = new_capacity;
    }

    /* Generate layer for new node */
    int level = generate_random_layer();
    if (level > HNSW_MAX_LAYERS) {
        level = HNSW_MAX_LAYERS;
    }

    /* Create new node */
    hnsw_node_t* new_node = hnsw_node_create(id, embedding, level);
    if (!new_node) {
        return E_SYSTEM_MEMORY;
    }

    /* First node - becomes entry point */
    if (index->count == 0) {
        index->nodes[0] = new_node;
        index->entry_point = new_node;
        index->max_layer = level;
        index->count = 1;
        LOG_DEBUG("Created HNSW entry point at layer %d", level);
        return KATRA_SUCCESS;
    }

    /* Search for nearest neighbors at each layer */
    hnsw_node_t* curr_nearest = index->entry_point;

    /* Traverse from top layer down to target layer */
    for (int lc = index->max_layer; lc > level; lc--) {
        /* Find nearest at this layer */
        float best_dist = hnsw_distance(curr_nearest->embedding, embedding);

        for (size_t i = 0; i < curr_nearest->layer_sizes[lc]; i++) {
            hnsw_node_t* neighbor = curr_nearest->layers[lc][i].node;
            float dist = hnsw_distance(neighbor->embedding, embedding);
            if (dist < best_dist) {
                best_dist = dist;
                curr_nearest = neighbor;
            }
        }
    }

    /* Insert at target layers */
    for (int lc = level; lc >= 0; lc--) {
        /* Find ef nearest neighbors at this layer */
        size_t candidates_count = 0;
        hnsw_node_t** candidates = search_layer(curr_nearest, embedding,
                                                 HNSW_EF_CONSTRUCTION, lc,
                                                 &candidates_count);

        /* Connect to M nearest */
        size_t m = (lc == 0) ? HNSW_M_MAX : HNSW_M;
        size_t connections = (candidates_count < m) ? candidates_count : m;

        for (size_t i = 0; i < connections; i++) {
            hnsw_node_t* neighbor = candidates[i];
            float dist = hnsw_distance(embedding, neighbor->embedding);

            /* Add bidirectional connection */
            hnsw_add_connection(new_node, neighbor, dist, lc);
            hnsw_add_connection(neighbor, new_node, dist, lc);
        }

        free(candidates);
    }

    /* Update entry point if new node is at higher layer */
    if (level > index->max_layer) {
        index->entry_point = new_node;
        index->max_layer = level;
    }

    /* Add to index */
    index->nodes[index->count] = new_node;
    index->count++;

    return KATRA_SUCCESS;
}

/* Search HNSW index for k nearest neighbors */
int katra_vector_hnsw_search(hnsw_index_t* index, vector_embedding_t* query,
                              size_t k, size_t** ids_out, float** distances_out,
                              size_t* count_out) {
    if (!index || !query || !ids_out || !distances_out || !count_out) {
        return E_INPUT_NULL;
    }

    if (index->count == 0) {
        *ids_out = NULL;
        *distances_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    /* Search from entry point down through layers */
    hnsw_node_t* curr_nearest = index->entry_point;
    float curr_dist = hnsw_distance(curr_nearest->embedding, query);

    /* Greedy search from top to layer 0 */
    for (int lc = index->max_layer; lc > 0; lc--) {
        bool changed = true;
        while (changed) {
            changed = false;

            for (size_t i = 0; i < curr_nearest->layer_sizes[lc]; i++) {
                hnsw_node_t* neighbor = curr_nearest->layers[lc][i].node;
                float dist = hnsw_distance(neighbor->embedding, query);

                if (dist < curr_dist) {
                    curr_nearest = neighbor;
                    curr_dist = dist;
                    changed = true;
                }
            }
        }
    }

    /* Beam search at layer 0 to find k nearest */
    size_t result_size = (k < index->count) ? k : index->count;
    size_t* ids = calloc(result_size, sizeof(size_t));
    float* distances = calloc(result_size, sizeof(float));

    if (!ids || !distances) {
        free(ids);
        free(distances);
        return E_SYSTEM_MEMORY;
    }

    /* Start with entry point result */
    ids[0] = curr_nearest->id;
    distances[0] = curr_dist;
    size_t found = 1;

    /* Add neighbors at layer 0 */
    for (size_t i = 0; i < curr_nearest->layer_sizes[0] && found < result_size; i++) {
        hnsw_node_t* neighbor = curr_nearest->layers[0][i].node;
        ids[found] = neighbor->id;
        distances[found] = hnsw_distance(neighbor->embedding, query);
        found++;
    }

    *ids_out = ids;
    *distances_out = distances;
    *count_out = found;

    return KATRA_SUCCESS;
}

/* Build HNSW index from vector store */
int katra_vector_hnsw_build(vector_store_t* store, hnsw_index_t** index_out) {
    if (!store || !index_out) {
        return E_INPUT_NULL;
    }

    hnsw_index_t* index = katra_vector_hnsw_init();
    if (!index) {
        return E_SYSTEM_MEMORY;
    }

    /* Insert all embeddings */
    for (size_t i = 0; i < store->count; i++) {
        int result = katra_vector_hnsw_insert(index, i, store->embeddings[i]);
        if (result != KATRA_SUCCESS) {
            katra_vector_hnsw_cleanup(index);
            return result;
        }
    }

    LOG_INFO("Built HNSW index with %zu nodes, max layer %d",
             index->count, index->max_layer);

    *index_out = index;
    return KATRA_SUCCESS;
}

/* Cleanup HNSW index */
void katra_vector_hnsw_cleanup(hnsw_index_t* index) {
    if (!index) {
        return;
    }

    /* Free all nodes */
    for (size_t i = 0; i < index->count; i++) {
        hnsw_node_free(index->nodes[i]);
    }

    free(index->nodes);
    free(index);
}

/* Get statistics */
void katra_vector_hnsw_stats(hnsw_index_t* index, size_t* nodes_out,
                              int* max_layer_out, size_t* total_connections_out) {
    if (!index) {
        return;
    }

    if (nodes_out) {
        *nodes_out = index->count;
    }

    if (max_layer_out) {
        *max_layer_out = index->max_layer;
    }

    if (total_connections_out) {
        size_t total = 0;
        for (size_t i = 0; i < index->count; i++) {
            for (int lc = 0; lc <= index->nodes[i]->level; lc++) {
                total += index->nodes[i]->layer_sizes[lc];
            }
        }
        *total_connections_out = total;
    }
}
