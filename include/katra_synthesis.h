/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_SYNTHESIS_H
#define KATRA_SYNTHESIS_H

/*
 * katra_synthesis.h - Multi-Backend Synthesis Layer (Phase 6.7)
 *
 * Combines results from multiple memory backends for unified recall:
 *   1. Vector Store: Semantic similarity search
 *   2. Graph Store: Relationship traversal
 *   3. SQL Store: Structured queries (Tier 1 memory)
 *   4. Working Memory: Current attention cache
 *
 * Synthesis creates emergent intelligence by combining different views
 * of memory to provide richer, more contextual recall.
 */

#include "katra_memory.h"
#include "katra_vector.h"
#include "katra_graph.h"
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * Types and Structures
 * ============================================================================ */

/**
 * Synthesis algorithm determines how results from multiple backends are combined
 */
typedef enum {
    SYNTHESIS_UNION,        /* Combine all results from all backends */
    SYNTHESIS_INTERSECTION, /* Only memories found by ALL backends */
    SYNTHESIS_WEIGHTED,     /* Score and rank by backend agreement + weights */
    SYNTHESIS_HIERARCHICAL  /* Cascade: Vector → Graph → SQL → Working */
} synthesis_algorithm_t;

/**
 * recall_options_t - Controls which backends to query and how to weight results
 */
typedef struct {
    /* Backend enable flags */
    bool use_vector;       /* Query semantic similarity */
    bool use_graph;        /* Query relationship traversal */
    bool use_sql;          /* Query structured Tier 1 storage */
    bool use_working;      /* Query current working memory */

    /* Backend weights (for SYNTHESIS_WEIGHTED algorithm) */
    float weight_vector;   /* Default: 0.3 */
    float weight_graph;    /* Default: 0.3 */
    float weight_sql;      /* Default: 0.3 */
    float weight_working;  /* Default: 0.1 */

    /* Search parameters */
    float similarity_threshold;  /* Min similarity for vector search (0.0-1.0) */
    int max_results;             /* Maximum results to return (0 = default) */

    /* Algorithm selection */
    synthesis_algorithm_t algorithm;  /* How to combine results */
} recall_options_t;

/**
 * synthesis_result_t - A single synthesized memory result
 */
typedef struct {
    char record_id[64];         /* Memory record ID */
    char* content;              /* Memory content (allocated) */
    float score;                /* Combined synthesis score (0.0-1.0) */

    /* Per-backend scores (for debugging/analysis) */
    float vector_score;         /* Similarity from vector search */
    float graph_score;          /* Relevance from graph traversal */
    float sql_score;            /* Match from SQL query */
    float working_score;        /* Attention from working memory */

    /* Source flags */
    bool from_vector;           /* Found in vector search */
    bool from_graph;            /* Found in graph traversal */
    bool from_sql;              /* Found in SQL query */
    bool from_working;          /* Found in working memory */

    /* Memory metadata */
    time_t timestamp;           /* When memory was created */
    float importance;           /* Original importance score */
} synthesis_result_t;

/**
 * synthesis_result_set_t - Collection of synthesized results
 */
typedef struct {
    synthesis_result_t* results;  /* Array of results */
    size_t count;                 /* Number of results */
    size_t capacity;              /* Allocated capacity */

    /* Statistics */
    int vector_matches;           /* Memories found via vector */
    int graph_matches;            /* Memories found via graph */
    int sql_matches;              /* Memories found via SQL */
    int working_matches;          /* Memories found in working memory */
} synthesis_result_set_t;

/* Default options for different use cases */
#define RECALL_OPTIONS_COMPREHENSIVE \
    { .use_vector = true, .use_graph = true, .use_sql = true, .use_working = true, \
      .weight_vector = 0.3f, .weight_graph = 0.3f, .weight_sql = 0.3f, .weight_working = 0.1f, \
      .similarity_threshold = 0.3f, .max_results = 20, .algorithm = SYNTHESIS_WEIGHTED }

#define RECALL_OPTIONS_SEMANTIC \
    { .use_vector = true, .use_graph = false, .use_sql = false, .use_working = true, \
      .weight_vector = 0.8f, .weight_graph = 0.0f, .weight_sql = 0.0f, .weight_working = 0.2f, \
      .similarity_threshold = 0.3f, .max_results = 20, .algorithm = SYNTHESIS_UNION }

#define RECALL_OPTIONS_RELATIONSHIPS \
    { .use_vector = false, .use_graph = true, .use_sql = true, .use_working = false, \
      .weight_vector = 0.0f, .weight_graph = 0.7f, .weight_sql = 0.3f, .weight_working = 0.0f, \
      .similarity_threshold = 0.3f, .max_results = 20, .algorithm = SYNTHESIS_HIERARCHICAL }

#define RECALL_OPTIONS_FAST \
    { .use_vector = false, .use_graph = false, .use_sql = true, .use_working = true, \
      .weight_vector = 0.0f, .weight_graph = 0.0f, .weight_sql = 0.5f, .weight_working = 0.5f, \
      .similarity_threshold = 0.3f, .max_results = 10, .algorithm = SYNTHESIS_UNION }

/* ============================================================================
 * Core Synthesis API
 * ============================================================================ */

/**
 * katra_recall_synthesized() - Multi-backend synthesized recall
 *
 * Queries multiple memory backends and synthesizes results according
 * to the specified algorithm and weights.
 *
 * Parameters:
 *   ci_id - CI identity to search
 *   query - Search query (text)
 *   options - Recall options (can be NULL for defaults)
 *   result_set - Output: allocated result set (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error code on failure
 *
 * Example:
 *   synthesis_result_set_t* results = NULL;
 *   recall_options_t opts = RECALL_OPTIONS_COMPREHENSIVE;
 *   int ret = katra_recall_synthesized(ci_id, "project meeting", &opts, &results);
 *   if (ret == KATRA_SUCCESS && results) {
 *       for (size_t i = 0; i < results->count; i++) {
 *           printf("Score %.2f: %s\n", results->results[i].score,
 *                  results->results[i].content);
 *       }
 *       katra_synthesis_free_results(results);
 *   }
 */
int katra_recall_synthesized(const char* ci_id,
                             const char* query,
                             const recall_options_t* options,
                             synthesis_result_set_t** result_set);

/**
 * katra_recall_related_synthesized() - Find memories related to a given memory
 *
 * Uses graph traversal and vector similarity to find related memories.
 *
 * Parameters:
 *   ci_id - CI identity
 *   record_id - Source memory to find relationships from
 *   options - Recall options
 *   result_set - Output: allocated result set
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_recall_related_synthesized(const char* ci_id,
                                     const char* record_id,
                                     const recall_options_t* options,
                                     synthesis_result_set_t** result_set);

/**
 * katra_what_do_i_know_synthesized() - Comprehensive topic exploration
 *
 * Combines all backend knowledge about a topic into a unified view.
 *
 * Parameters:
 *   ci_id - CI identity
 *   topic - Topic to explore
 *   options - Recall options
 *   result_set - Output: allocated result set
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_what_do_i_know_synthesized(const char* ci_id,
                                     const char* topic,
                                     const recall_options_t* options,
                                     synthesis_result_set_t** result_set);

/* ============================================================================
 * Result Management
 * ============================================================================ */

/**
 * katra_synthesis_free_results() - Free a synthesis result set
 *
 * Frees all memory associated with a result set including content strings.
 */
void katra_synthesis_free_results(synthesis_result_set_t* result_set);

/**
 * katra_synthesis_result_init() - Initialize a single result
 */
void katra_synthesis_result_init(synthesis_result_t* result);

/**
 * katra_synthesis_result_set_init() - Initialize a result set
 */
int katra_synthesis_result_set_init(synthesis_result_set_t** result_set,
                                    size_t initial_capacity);

/**
 * katra_synthesis_result_set_add() - Add a result to a result set
 *
 * If the record_id already exists, updates the scores instead of duplicating.
 */
int katra_synthesis_result_set_add(synthesis_result_set_t* result_set,
                                   const synthesis_result_t* result);

/* ============================================================================
 * Options Initialization
 * ============================================================================ */

/**
 * katra_recall_options_init() - Initialize options with defaults
 */
static inline void katra_recall_options_init(recall_options_t* opts) {
    if (opts) {
        opts->use_vector = true;
        opts->use_graph = true;
        opts->use_sql = true;
        opts->use_working = true;
        opts->weight_vector = 0.3f;
        opts->weight_graph = 0.3f;
        opts->weight_sql = 0.3f;
        opts->weight_working = 0.1f;
        opts->similarity_threshold = 0.3f;
        opts->max_results = 20;
        opts->algorithm = SYNTHESIS_WEIGHTED;
    }
}

#endif /* KATRA_SYNTHESIS_H */
