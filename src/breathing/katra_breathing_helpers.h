/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_BREATHING_HELPERS_H
#define KATRA_BREATHING_HELPERS_H

/*
 * Internal helper functions for breathing layer
 * Reduces boilerplate and improves maintainability
 */

#include "katra_breathing.h"
#include "katra_memory.h"

/* ============================================================================
 * MEMORY FORMATION HELPERS
 * ============================================================================ */

/*
 * Store typed memory with automatic session attachment and stats tracking
 * Returns: KATRA_SUCCESS or error code
 */
int breathing_store_typed_memory(memory_type_t type,
                                 const char* content,
                                 float importance,
                                 const char* importance_note,
                                 why_remember_t why_enum,
                                 const char* func_name);

/*
 * Attach current session ID to memory record
 * Handles strdup and error checking
 */
void breathing_attach_session(memory_record_t* record);

/* ============================================================================
 * QUERY RESULT HELPERS
 * ============================================================================ */

/*
 * Copy content strings from memory query results to owned array
 * Caller must free returned array with free_memory_list()
 * Returns: Array of strings or NULL on error
 */
char** breathing_copy_memory_contents(memory_record_t** results,
                                     size_t result_count,
                                     size_t* out_count);

/* ============================================================================
 * SEMANTIC PARSING HELPERS
 * ============================================================================ */

/*
 * Check if semantic string contains any phrase from list
 * Returns: true if match found, false otherwise
 */
bool breathing_contains_any_phrase(const char* semantic, const char** phrases);

#endif /* KATRA_BREATHING_HELPERS_H */
