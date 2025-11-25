/* © 2025 Casey Koons All rights reserved */

/*
 * katra_synthesis.c - Multi-Backend Synthesis Layer (Phase 6.7)
 *
 * Combines results from multiple memory backends for unified recall.
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_synthesis.h"
#include "katra_memory.h"
#include "katra_vector.h"
#include "katra_graph.h"
#include "katra_breathing.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Forward declarations for breathing layer accessors */
extern vector_store_t* breathing_get_vector_store(void);
extern graph_store_t* breathing_get_graph_store(void);

/* ============================================================================
 * Internal Constants
 * ============================================================================ */

#define SYNTHESIS_INITIAL_CAPACITY 32
#define SYNTHESIS_MAX_RESULTS_DEFAULT 20

/* ============================================================================
 * Result Management
 * ============================================================================ */

void katra_synthesis_result_init(synthesis_result_t* result) {
    if (!result) return;

    memset(result, 0, sizeof(synthesis_result_t));
    result->score = 0.0f;
    result->vector_score = 0.0f;
    result->graph_score = 0.0f;
    result->sql_score = 0.0f;
    result->working_score = 0.0f;
}

int katra_synthesis_result_set_init(synthesis_result_set_t** result_set,
                                    size_t initial_capacity) {
    KATRA_CHECK_NULL(result_set);

    *result_set = calloc(1, sizeof(synthesis_result_set_t));
    if (!*result_set) {
        return E_SYSTEM_MEMORY;
    }

    if (initial_capacity == 0) {
        initial_capacity = SYNTHESIS_INITIAL_CAPACITY;
    }

    (*result_set)->results = calloc(initial_capacity, sizeof(synthesis_result_t));
    if (!(*result_set)->results) {
        free(*result_set);
        *result_set = NULL;
        return E_SYSTEM_MEMORY;
    }

    (*result_set)->count = 0;
    (*result_set)->capacity = initial_capacity;
    (*result_set)->vector_matches = 0;
    (*result_set)->graph_matches = 0;
    (*result_set)->sql_matches = 0;
    (*result_set)->working_matches = 0;

    return KATRA_SUCCESS;
}

void katra_synthesis_free_results(synthesis_result_set_t* result_set) {
    if (!result_set) return;

    if (result_set->results) {
        for (size_t i = 0; i < result_set->count; i++) {
            free(result_set->results[i].content);
        }
        free(result_set->results);
    }

    free(result_set);
}

/**
 * Find existing result by record_id, returns index or -1 if not found
 */
static int find_result_by_id(synthesis_result_set_t* result_set,
                             const char* record_id) {
    if (!result_set || !record_id) return -1;

    for (size_t i = 0; i < result_set->count; i++) {
        if (strcmp(result_set->results[i].record_id, record_id) == 0) {
            return (int)i;
        }
    }
    return -1;
}

int katra_synthesis_result_set_add(synthesis_result_set_t* result_set,
                                   const synthesis_result_t* result) {
    KATRA_CHECK_NULL(result_set);
    KATRA_CHECK_NULL(result);

    /* Check if already exists - merge scores instead of duplicating */
    int existing_idx = find_result_by_id(result_set, result->record_id);
    if (existing_idx >= 0) {
        synthesis_result_t* existing = &result_set->results[existing_idx];

        /* Merge scores - keep highest */
        if (result->vector_score > existing->vector_score) {
            existing->vector_score = result->vector_score;
            existing->from_vector = true;
        }
        if (result->graph_score > existing->graph_score) {
            existing->graph_score = result->graph_score;
            existing->from_graph = true;
        }
        if (result->sql_score > existing->sql_score) {
            existing->sql_score = result->sql_score;
            existing->from_sql = true;
        }
        if (result->working_score > existing->working_score) {
            existing->working_score = result->working_score;
            existing->from_working = true;
        }

        /* Recalculate combined score */
        existing->score = existing->vector_score + existing->graph_score +
                          existing->sql_score + existing->working_score;

        return KATRA_SUCCESS;
    }

    /* Need to add new result - grow array if needed */
    if (result_set->count >= result_set->capacity) {
        size_t new_capacity = result_set->capacity * 2;
        synthesis_result_t* new_results = realloc(result_set->results,
            new_capacity * sizeof(synthesis_result_t));
        if (!new_results) {
            return E_SYSTEM_MEMORY;
        }
        result_set->results = new_results;
        result_set->capacity = new_capacity;
    }

    /* Copy result */
    synthesis_result_t* dest = &result_set->results[result_set->count];
    memcpy(dest, result, sizeof(synthesis_result_t));

    /* Duplicate content string */
    if (result->content) {
        dest->content = strdup(result->content);
        if (!dest->content) {
            return E_SYSTEM_MEMORY;
        }
    }

    result_set->count++;
    return KATRA_SUCCESS;
}

/* ============================================================================
 * Backend Query Functions
 * ============================================================================ */

/**
 * Query vector store for semantic similarity
 */
static int query_vector_backend(const char* ci_id,
                                const char* query,
                                const recall_options_t* opts,
                                synthesis_result_set_t* result_set) {
    (void)ci_id;  /* Used for filtering in future */

    vector_store_t* vector_store = breathing_get_vector_store();
    if (!vector_store) {
        LOG_DEBUG("Vector store not available for synthesis");
        return KATRA_SUCCESS;  /* Non-fatal - continue without vector */
    }

    /* Search for similar memories */
    size_t max_k = opts->max_results > 0 ? (size_t)opts->max_results : SYNTHESIS_MAX_RESULTS_DEFAULT;
    vector_match_t** vec_matches = NULL;
    size_t vec_count = 0;

    int ret = katra_vector_search(vector_store, query, max_k,
                                  &vec_matches, &vec_count);
    if (ret != KATRA_SUCCESS || vec_count == 0 || !vec_matches) {
        LOG_DEBUG("Vector search returned no results");
        return KATRA_SUCCESS;
    }

    /* Convert vector results to synthesis results */
    for (size_t i = 0; i < vec_count; i++) {
        /* Apply threshold filtering */
        if (vec_matches[i]->similarity < opts->similarity_threshold) {
            continue;
        }

        synthesis_result_t synth_result;
        katra_synthesis_result_init(&synth_result);

        strncpy(synth_result.record_id, vec_matches[i]->record_id,
                sizeof(synth_result.record_id) - 1);
        synth_result.vector_score = vec_matches[i]->similarity * opts->weight_vector;
        synth_result.score = synth_result.vector_score;
        synth_result.from_vector = true;

        /* Content not loaded yet - will be done during final synthesis */
        katra_synthesis_result_set_add(result_set, &synth_result);
        result_set->vector_matches++;
    }

    katra_vector_free_matches(vec_matches, vec_count);
    return KATRA_SUCCESS;
}

/**
 * Query graph store for relationships
 */
static int query_graph_backend(const char* ci_id,
                               const char* query,
                               const recall_options_t* opts,
                               synthesis_result_set_t* result_set) {
    (void)ci_id;
    (void)query;  /* Graph traversal starts from existing results */

    graph_store_t* graph_store = breathing_get_graph_store();
    if (!graph_store) {
        LOG_DEBUG("Graph store not available for synthesis");
        return KATRA_SUCCESS;
    }

    /* For each existing result, find related memories via graph */
    size_t original_count = result_set->count;
    for (size_t i = 0; i < original_count; i++) {
        const char* record_id = result_set->results[i].record_id;

        /* Get related edges from graph */
        graph_edge_t** edges = NULL;
        size_t edge_count = 0;

        /* Use REL_SIMILAR as a reasonable default for synthesis */
        int ret = katra_graph_get_related(graph_store, record_id,
                                          REL_SIMILAR, &edges, &edge_count);
        if (ret != KATRA_SUCCESS || edge_count == 0 || !edges) {
            continue;
        }

        /* Add related nodes as results */
        for (size_t j = 0; j < edge_count && j < 5; j++) {  /* Limit per source */
            synthesis_result_t synth_result;
            katra_synthesis_result_init(&synth_result);

            strncpy(synth_result.record_id, edges[j]->to_id,
                    sizeof(synth_result.record_id) - 1);
            synth_result.graph_score = edges[j]->strength * opts->weight_graph;
            synth_result.score = synth_result.graph_score;
            synth_result.from_graph = true;

            katra_synthesis_result_set_add(result_set, &synth_result);
            result_set->graph_matches++;
        }

        /* Free edges */
        for (size_t j = 0; j < edge_count; j++) {
            free(edges[j]);
        }
        free(edges);
    }

    return KATRA_SUCCESS;
}

/**
 * Query SQL (Tier 1) store for keyword matches
 */
static int query_sql_backend(const char* ci_id,
                             const char* query,
                             const recall_options_t* opts,
                             synthesis_result_set_t* result_set) {
    /* Use existing recall_about functionality */
    size_t count = 0;
    char** memories = recall_about(ci_id, query, &count);

    if (!memories || count == 0) {
        LOG_DEBUG("SQL recall returned no results");
        return KATRA_SUCCESS;
    }

    /* Convert string memories to synthesis results */
    size_t max_results = opts->max_results > 0 ? (size_t)opts->max_results : SYNTHESIS_MAX_RESULTS_DEFAULT;
    for (size_t i = 0; i < count && i < max_results; i++) {
        synthesis_result_t synth_result;
        katra_synthesis_result_init(&synth_result);

        /* Generate a simple ID for SQL results (hash of content) */
        snprintf(synth_result.record_id, sizeof(synth_result.record_id),
                 "sql_%zu_%lu", i, (unsigned long)time(NULL));

        synth_result.sql_score = opts->weight_sql;
        synth_result.score = synth_result.sql_score;
        synth_result.from_sql = true;
        synth_result.content = strdup(memories[i]);
        synth_result.timestamp = time(NULL);  /* Current time as fallback */

        katra_synthesis_result_set_add(result_set, &synth_result);
        free(synth_result.content);  /* Was duplicated in add */
        result_set->sql_matches++;

        free(memories[i]);
    }

    /* Free remaining if any */
    for (size_t i = max_results; i < count; i++) {
        free(memories[i]);
    }
    free(memories);

    return KATRA_SUCCESS;
}

/**
 * Query working memory for current attention
 */
static int query_working_backend(const char* ci_id,
                                 const char* query,
                                 const recall_options_t* opts,
                                 synthesis_result_set_t* result_set) {
    (void)ci_id;
    (void)query;
    (void)opts;
    (void)result_set;

    /* Working memory integration - uses breathing layer */
    /* TODO: Implement working memory query when API is available */
    LOG_DEBUG("Working memory synthesis not yet implemented");
    return KATRA_SUCCESS;
}

/* ============================================================================
 * Synthesis Algorithms
 * ============================================================================ */

/**
 * Sort results by score (descending)
 */
static int compare_by_score(const void* a, const void* b) {
    const synthesis_result_t* ra = (const synthesis_result_t*)a;
    const synthesis_result_t* rb = (const synthesis_result_t*)b;

    if (rb->score > ra->score) return 1;
    if (rb->score < ra->score) return -1;
    return 0;
}

/**
 * Apply synthesis algorithm to combine/filter results
 */
static int apply_synthesis_algorithm(synthesis_result_set_t* result_set,
                                     const recall_options_t* opts) {
    if (!result_set || result_set->count == 0) {
        return KATRA_SUCCESS;
    }

    switch (opts->algorithm) {
        case SYNTHESIS_UNION:
            /* Keep all results, sort by score */
            qsort(result_set->results, result_set->count,
                  sizeof(synthesis_result_t), compare_by_score);
            break;

        case SYNTHESIS_INTERSECTION: {
            /* Keep only results found by ALL enabled backends */
            size_t write_idx = 0;
            for (size_t i = 0; i < result_set->count; i++) {
                synthesis_result_t* r = &result_set->results[i];
                bool keep = true;

                if (opts->use_vector && !r->from_vector) keep = false;
                if (opts->use_graph && !r->from_graph) keep = false;
                if (opts->use_sql && !r->from_sql) keep = false;
                if (opts->use_working && !r->from_working) keep = false;

                if (keep) {
                    if (write_idx != i) {
                        /* Move to keep position, free old content if moving */
                        if (write_idx < i) {
                            free(result_set->results[write_idx].content);
                        }
                        memcpy(&result_set->results[write_idx], r, sizeof(synthesis_result_t));
                    }
                    write_idx++;
                } else {
                    free(r->content);
                }
            }
            result_set->count = write_idx;
            qsort(result_set->results, result_set->count,
                  sizeof(synthesis_result_t), compare_by_score);
            break;
        }

        case SYNTHESIS_WEIGHTED:
            /* Recalculate scores using weights, sort by combined score */
            for (size_t i = 0; i < result_set->count; i++) {
                synthesis_result_t* r = &result_set->results[i];
                r->score = r->vector_score + r->graph_score +
                           r->sql_score + r->working_score;
            }
            qsort(result_set->results, result_set->count,
                  sizeof(synthesis_result_t), compare_by_score);
            break;

        case SYNTHESIS_HIERARCHICAL:
            /* Already processed in order (vector → graph → SQL → working) */
            /* Just sort by final score */
            qsort(result_set->results, result_set->count,
                  sizeof(synthesis_result_t), compare_by_score);
            break;
    }

    /* Trim to max_results */
    if (opts->max_results > 0 && result_set->count > (size_t)opts->max_results) {
        /* Free excess content */
        for (size_t i = opts->max_results; i < result_set->count; i++) {
            free(result_set->results[i].content);
        }
        result_set->count = opts->max_results;
    }

    return KATRA_SUCCESS;
}

/* ============================================================================
 * Core Synthesis API
 * ============================================================================ */

int katra_recall_synthesized(const char* ci_id,
                             const char* query,
                             const recall_options_t* options,
                             synthesis_result_set_t** result_set) {
    KATRA_CHECK_NULL(ci_id);
    KATRA_CHECK_NULL(query);
    KATRA_CHECK_NULL(result_set);

    /* Use default options if not provided */
    recall_options_t default_opts;
    if (!options) {
        katra_recall_options_init(&default_opts);
        options = &default_opts;
    }

    /* Initialize result set */
    int ret = katra_synthesis_result_set_init(result_set, SYNTHESIS_INITIAL_CAPACITY);
    if (ret != KATRA_SUCCESS) {
        return ret;
    }

    LOG_DEBUG("Synthesis recall: query='%s', backends=[vec=%d,graph=%d,sql=%d,work=%d]",
              query, options->use_vector, options->use_graph,
              options->use_sql, options->use_working);

    /* Query each enabled backend */
    if (options->use_vector) {
        query_vector_backend(ci_id, query, options, *result_set);
    }

    if (options->use_graph) {
        query_graph_backend(ci_id, query, options, *result_set);
    }

    if (options->use_sql) {
        query_sql_backend(ci_id, query, options, *result_set);
    }

    if (options->use_working) {
        query_working_backend(ci_id, query, options, *result_set);
    }

    /* Apply synthesis algorithm */
    apply_synthesis_algorithm(*result_set, options);

    LOG_DEBUG("Synthesis complete: %zu results (vec=%d, graph=%d, sql=%d, work=%d)",
              (*result_set)->count,
              (*result_set)->vector_matches, (*result_set)->graph_matches,
              (*result_set)->sql_matches, (*result_set)->working_matches);

    return KATRA_SUCCESS;
}

int katra_recall_related_synthesized(const char* ci_id,
                                     const char* record_id,
                                     const recall_options_t* options,
                                     synthesis_result_set_t** result_set) {
    KATRA_CHECK_NULL(ci_id);
    KATRA_CHECK_NULL(record_id);
    KATRA_CHECK_NULL(result_set);

    /* Use record_id as query - the graph backend will find related items */
    return katra_recall_synthesized(ci_id, record_id, options, result_set);
}

int katra_what_do_i_know_synthesized(const char* ci_id,
                                     const char* topic,
                                     const recall_options_t* options,
                                     synthesis_result_set_t** result_set) {
    /* This is essentially the same as recall_synthesized */
    return katra_recall_synthesized(ci_id, topic, options, result_set);
}
