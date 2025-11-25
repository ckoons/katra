/* © 2025 Casey Koons All rights reserved */

/*
 * katra_universal_encoder.c - Universal Memory Encoding (Phase 6.6)
 *
 * Consolidates memory formation into single entry point that writes to:
 *   1. Tier 1: Core memory storage
 *   2. Vector store: Semantic embeddings
 *   3. Graph store: Memory associations
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_universal_encoder.h"
#include "katra_memory.h"
#include "katra_vector.h"
#include "katra_graph.h"
#include "katra_breathing.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Internal breathing layer functions (forward declarations) */
extern int breathing_create_auto_edges(graph_store_t* graph_store,
                                        vector_store_t* vector_store,
                                        const context_config_t* config,
                                        const char* new_record_id,
                                        const char* content);
extern vector_store_t* breathing_get_vector_store(void);
extern graph_store_t* breathing_get_graph_store(void);
extern context_config_t* breathing_get_config_ptr(void);

/* ============================================================================
 * UNIVERSAL ENCODE - FULL VERSION
 * ============================================================================ */

int katra_universal_encode(memory_record_t* record,
                           vector_store_t* vector_store,
                           graph_store_t* graph_store,
                           context_config_t* config,
                           const encode_options_t* options,
                           encode_result_t* result) {
    /* Initialize result if provided */
    if (result) {
        encode_result_init(result);
    }

    /* Validate required parameters */
    KATRA_CHECK_NULL(record);
    KATRA_CHECK_NULL(record->content);

    /* Use default options if not provided */
    encode_options_t default_opts = ENCODE_OPTIONS_DEFAULT;
    if (!options) {
        options = &default_opts;
    }

    int overall_result = KATRA_SUCCESS;

    /* ========================================================================
     * Step 1: Store to Tier 1 (Core Memory)
     * This is the critical path - must succeed
     * ======================================================================== */
    int store_result = katra_memory_store(record);

    if (store_result != KATRA_SUCCESS) {
        LOG_ERROR("Universal encode: Core memory storage failed: %d", store_result);
        if (result) {
            result->error_code = store_result;
        }
        return store_result;
    }

    /* Mark success and copy record_id */
    if (result) {
        result->memory_stored = true;
        if (record->record_id) {
            strncpy(result->record_id, record->record_id,
                    sizeof(result->record_id) - 1);
            result->record_id[sizeof(result->record_id) - 1] = '\0';
        }
    }

    LOG_DEBUG("Universal encode: Core memory stored: %s",
              record->record_id ? record->record_id : "unknown");

    /* ========================================================================
     * Step 2: Create Vector Embedding (optional)
     * Non-fatal by default - continue even if this fails
     * ======================================================================== */
    if (vector_store && !options->skip_vector && record->record_id && record->content) {
        int vec_result = katra_vector_store(vector_store, record->record_id, record->content);

        if (vec_result == KATRA_SUCCESS) {
            if (result) {
                result->vector_created = true;
            }
            LOG_DEBUG("Universal encode: Vector embedding created for %s", record->record_id);
        } else {
            LOG_WARN("Universal encode: Vector embedding failed for %s (non-fatal): %d",
                     record->record_id, vec_result);

            if (options->require_all) {
                if (result) {
                    result->error_code = vec_result;
                }
                overall_result = vec_result;
            }
        }
    }

    /* ========================================================================
     * Step 3: Create Graph Edges (optional)
     * Non-fatal by default - continue even if this fails
     * ======================================================================== */
    if (graph_store && !options->skip_graph && record->record_id && record->content) {
        int edge_result = breathing_create_auto_edges(
            graph_store, vector_store, config,
            record->record_id, record->content
        );

        if (edge_result == KATRA_SUCCESS) {
            if (result) {
                result->edges_created = true;
                /* TODO: Get actual edge count from breathing_create_auto_edges */
                result->edge_count = 1;  /* Placeholder */
            }
            LOG_DEBUG("Universal encode: Graph edges created for %s", record->record_id);
        } else {
            LOG_WARN("Universal encode: Graph edge creation failed for %s (non-fatal): %d",
                     record->record_id, edge_result);

            if (options->require_all) {
                if (result && result->error_code == 0) {
                    result->error_code = edge_result;
                }
                if (overall_result == KATRA_SUCCESS) {
                    overall_result = edge_result;
                }
            }
        }
    }

    return overall_result;
}

/* ============================================================================
 * UNIVERSAL ENCODE - SIMPLE VERSION
 * Uses breathing layer's global stores
 * ============================================================================ */

int katra_universal_encode_simple(memory_record_t* record) {
    KATRA_CHECK_NULL(record);

    /* Get global stores from breathing layer */
    vector_store_t* vector_store = breathing_get_vector_store();
    graph_store_t* graph_store = breathing_get_graph_store();
    context_config_t* config = breathing_get_config_ptr();

    /* Encode with defaults */
    return katra_universal_encode(
        record,
        vector_store,
        graph_store,
        config,
        NULL,  /* Use default options */
        NULL   /* Don't need result */
    );
}
