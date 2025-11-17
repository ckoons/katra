/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_graph.c - Automatic graph edge creation (Phase 6.2)
 *
 * Auto-creates graph edges during memory formation:
 * - SIMILAR edges: Based on vector similarity (when semantic search enabled)
 * - SEQUENTIAL edges: Based on temporal proximity (time-based)
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_breathing_internal.h"
#include "katra_graph.h"
#include "katra_vector.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/* Get most recent memory ID (for sequential edge detection) */
static char* get_most_recent_memory_id(void) {
    /* Use breathing layer's recent_thoughts() function to get last memory */
    size_t count = 0;
    char** recent_ids = recent_thoughts(1, &count);  /* Get just the most recent */

    if (!recent_ids || count == 0) {
        return NULL;
    }

    /* Save the first (most recent) ID */
    char* result = recent_ids[0];  /* Transfer ownership */
    recent_ids[0] = NULL;  /* Don't double-free */

    /* Free the array (but not the transferred string) */
    free_memory_list(recent_ids, count);

    return result;
}

/* ============================================================================
 * PUBLIC API - Automatic Edge Creation
 * ============================================================================ */

int breathing_create_auto_edges(graph_store_t* graph_store,
                                 vector_store_t* vector_store,
                                 const context_config_t* config,
                                 const char* new_record_id,
                                 const char* content) {
    if (!graph_store || !config || !new_record_id) {
        return E_INPUT_NULL;
    }

    int result = KATRA_SUCCESS;
    int edges_created = 0;

    LOG_DEBUG("Creating auto-edges for memory: %s", new_record_id);

    /* Step 1: Create SIMILAR edges using vector similarity */
    if (vector_store && config->use_semantic_search && content) {
        vector_match_t** matches = NULL;
        size_t match_count = 0;

        /* Search for similar memories */
        result = katra_vector_search(vector_store, content,
                                     config->graph_max_similar_edges,
                                     &matches, &match_count);

        if (result == KATRA_SUCCESS && matches) {
            /* Create SIMILAR edges for matches above threshold */
            for (size_t i = 0; i < match_count; i++) {
                if (matches[i]->similarity >= config->graph_similarity_threshold) {
                    /* Don't create self-edges */
                    if (strcmp(matches[i]->record_id, new_record_id) != 0) {
                        /* Create bidirectional SIMILAR edges */
                        result = katra_graph_add_edge(graph_store,
                                                     new_record_id,
                                                     matches[i]->record_id,
                                                     REL_SIMILAR,
                                                     "semantic similarity",
                                                     matches[i]->similarity);
                        if (result == KATRA_SUCCESS) {
                            edges_created++;

                            /* Add reverse edge */
                            katra_graph_add_edge(graph_store,
                                               matches[i]->record_id,
                                               new_record_id,
                                               REL_SIMILAR,
                                               "semantic similarity",
                                               matches[i]->similarity);
                            edges_created++;
                        }
                    }
                }
            }

            katra_vector_free_matches(matches, match_count);
            LOG_DEBUG("Created %d SIMILAR edges", edges_created);
        }
    }

    /* Step 2: Create SEQUENTIAL edge from previous memory */
    char* prev_id = get_most_recent_memory_id();
    if (prev_id && strcmp(prev_id, new_record_id) != 0) {
        result = katra_graph_add_edge(graph_store,
                                     prev_id,
                                     new_record_id,
                                     REL_SEQUENTIAL,
                                     "temporal sequence",
                                     1.0f);  /* Full strength */
        if (result == KATRA_SUCCESS) {
            edges_created++;
            LOG_DEBUG("Created SEQUENTIAL edge: %s -> %s", prev_id, new_record_id);
        }
        free(prev_id);
    }

    LOG_INFO("Auto-edge creation complete: %d total edges created for %s",
             edges_created, new_record_id);

    return KATRA_SUCCESS;  /* Non-fatal even if edge creation fails */
}

/* Note: breathing_get_graph_store() is implemented in katra_breathing.c */
