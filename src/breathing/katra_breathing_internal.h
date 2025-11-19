/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_BREATHING_INTERNAL_H
#define KATRA_BREATHING_INTERNAL_H

#include <stdbool.h>
#include <time.h>
#include "katra_breathing.h"
#include "katra_vector.h"  /* Phase 6.1f: for vector_store_t */
#include "katra_graph.h"   /* Phase 6.2: for graph_store_t */

/**
 * katra_breathing_internal.h - Internal state accessors for breathing layer
 *
 * This header provides access to shared global state across breathing layer
 * compilation units. NOT for external use.
 */

/* Global state access functions */

/**
 * Check if breathing layer is initialized
 */
bool breathing_get_initialized(void);

/**
 * Get CI ID from global context
 * Returns: CI ID string or NULL if not initialized
 */
const char* breathing_get_ci_id(void);

/**
 * Get session ID from global context
 * Returns: Session ID string or NULL if not set
 */
const char* breathing_get_session_id(void);

/**
 * Get pointer to global context configuration
 * Returns: Pointer to config structure
 */
context_config_t* breathing_get_config_ptr(void);

/**
 * Get pointer to global enhanced statistics
 * Returns: Pointer to stats structure
 */
enhanced_stats_t* breathing_get_stats_ptr(void);

/**
 * Track memory stored in stats
 */
void breathing_track_memory_stored(memory_type_t type, why_remember_t importance);

/**
 * Track semantic remember in stats
 */
void breathing_track_semantic_remember(why_remember_t importance);

/**
 * Track query in stats
 */
void breathing_track_relevant_query(void);
void breathing_track_recent_query(void);
void breathing_track_topic_query(size_t match_count);

/**
 * Track context load in stats
 */
void breathing_track_context_load(size_t memory_count);

/**
 * Track memory in current turn (for reflection API)
 */
int track_memory_in_turn(const char* record_id);

/**
 * Cleanup turn tracking (called during breathe_cleanup)
 */
void cleanup_turn_tracking(void);

/**
 * Get vector store for semantic search (Phase 6.1f)
 * Returns: Pointer to vector store or NULL if not initialized
 */
vector_store_t* breathing_get_vector_store(void);

/**
 * Initialize vector store if not already initialized
 * Returns: KATRA_SUCCESS or error code
 */
int breathing_init_vector_store(void);

/**
 * Get graph store for memory associations (Phase 6.2)
 * Returns: Pointer to graph store or NULL if not initialized
 */
graph_store_t* breathing_get_graph_store(void);

/**
 * Set graph store pointer (Phase 6.2)
 * Used for dynamic enable/disable of auto-edges
 */
void breathing_set_graph_store(graph_store_t* store);

/**
 * Create automatic graph edges after memory formation (Phase 6.2)
 * Creates SIMILAR edges (vector similarity) and SEQUENTIAL edges (temporal proximity)
 *
 * Parameters:
 *   graph_store - Graph store to add edges to
 *   vector_store - Vector store for similarity search (can be NULL)
 *   config - Context configuration with thresholds
 *   new_record_id - ID of newly created memory
 *   content - Memory content for similarity matching
 *
 * Returns: KATRA_SUCCESS (non-fatal even if edge creation fails)
 */
int breathing_create_auto_edges(graph_store_t* graph_store,
                                 vector_store_t* vector_store,
                                 const context_config_t* config,
                                 const char* new_record_id,
                                 const char* content);

#endif /* KATRA_BREATHING_INTERNAL_H */
