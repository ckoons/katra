/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_UNIVERSAL_ENCODER_H
#define KATRA_UNIVERSAL_ENCODER_H

/*
 * katra_universal_encoder.h - Universal Memory Encoding (Phase 6.6)
 *
 * Problem: Memory formation currently requires multiple separate calls:
 *   1. katra_memory_store() - Store to Tier 1 (JSONL/SQLite)
 *   2. vector_store_add() - Create embedding for semantic search
 *   3. breathing_create_auto_edges() - Create graph associations
 *
 * These are scattered across breathing layer and MCP tools, leading to:
 *   - Inconsistent encoding (some memories lack vectors/edges)
 *   - Duplication of logic
 *   - Harder to maintain
 *
 * Solution: Universal encoder that writes to ALL backends in one call.
 *
 * Design Philosophy:
 *   - Single entry point for memory formation
 *   - Non-fatal degradation (if vector store unavailable, still stores memory)
 *   - Atomic semantics where possible (all-or-nothing for core storage)
 *   - Minimal API surface (one function does everything)
 */

#include "katra_memory.h"
#include "katra_vector.h"
#include "katra_graph.h"
#include "katra_breathing.h"  /* For context_config_t */
#include <stdbool.h>

/* Encoding result - tracks what was successfully encoded */
typedef struct {
    bool memory_stored;      /* Tier 1 storage succeeded */
    bool vector_created;     /* Semantic embedding created */
    bool edges_created;      /* Graph edges created */
    int edge_count;          /* Number of edges created */
    char record_id[64];      /* Memory record ID if stored */
    int error_code;          /* First error encountered (0 = success) */
} encode_result_t;

/* Encoding options - control what backends to write to */
typedef struct {
    bool skip_vector;        /* Don't create embedding */
    bool skip_graph;         /* Don't create graph edges */
    bool require_all;        /* Fail if ANY backend fails (default: false = best effort) */
} encode_options_t;

/* Default options (encode to all backends, best effort) */
#define ENCODE_OPTIONS_DEFAULT { .skip_vector = false, .skip_graph = false, .require_all = false }

/* Encode to all backends, fail if any fails */
#define ENCODE_OPTIONS_STRICT { .skip_vector = false, .skip_graph = false, .require_all = true }

/* Encode to memory only (skip expensive operations) */
#define ENCODE_OPTIONS_FAST { .skip_vector = true, .skip_graph = true, .require_all = false }

/**
 * katra_universal_encode() - Encode memory to all backends
 *
 * Single entry point for memory formation. Stores to:
 *   1. Tier 1: Core memory storage (JSONL + SQLite index)
 *   2. Vector store: Semantic embedding for similarity search
 *   3. Graph store: SIMILAR and SEQUENTIAL edges
 *
 * Parameters:
 *   record - Memory record to encode (must have ci_id and content set)
 *   vector_store - Vector store (can be NULL to skip vectorization)
 *   graph_store - Graph store (can be NULL to skip graph edges)
 *   config - Context configuration for thresholds (can be NULL for defaults)
 *   options - Encoding options (can be NULL for defaults)
 *   result - Output: encoding result (can be NULL if not needed)
 *
 * Returns:
 *   KATRA_SUCCESS if core memory stored (regardless of vector/graph status)
 *   Error code if core storage fails
 *
 * Notes:
 *   - By default, vector/graph failures don't cause overall failure
 *   - Use ENCODE_OPTIONS_STRICT to require all backends
 *   - Result struct shows exactly what succeeded/failed
 */
int katra_universal_encode(memory_record_t* record,
                           vector_store_t* vector_store,
                           graph_store_t* graph_store,
                           context_config_t* config,
                           const encode_options_t* options,
                           encode_result_t* result);

/**
 * katra_universal_encode_simple() - Simplified encoding with global stores
 *
 * Uses breathing layer's global vector and graph stores.
 * Equivalent to:
 *   katra_universal_encode(record, breathing_get_vector_store(),
 *                          breathing_get_graph_store(),
 *                          breathing_get_config_ptr(), NULL, NULL)
 *
 * Parameters:
 *   record - Memory record to encode
 *
 * Returns:
 *   KATRA_SUCCESS if core memory stored
 *   Error code if core storage fails
 */
int katra_universal_encode_simple(memory_record_t* record);

/**
 * Helper: Initialize encode_result_t to known state
 */
static inline void encode_result_init(encode_result_t* result) {
    if (result) {
        result->memory_stored = false;
        result->vector_created = false;
        result->edges_created = false;
        result->edge_count = 0;
        result->record_id[0] = '\0';
        result->error_code = 0;
    }
}

#endif /* KATRA_UNIVERSAL_ENCODER_H */
