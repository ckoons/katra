/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_BREATHING_INTERNAL_H
#define KATRA_BREATHING_INTERNAL_H

#include <stdbool.h>
#include <time.h>
#include "katra_breathing.h"

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

#endif /* KATRA_BREATHING_INTERNAL_H */
