/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_BREATHING_SEARCH_H
#define KATRA_BREATHING_SEARCH_H

#include <stddef.h>
#include "katra_memory.h"

/**
 * katra_breathing_search.h - Hybrid search API (Phase 6.1f)
 *
 * Combines keyword matching with semantic similarity search
 * for improved recall relevance.
 */

/**
 * hybrid_search() - Search using keyword + semantic similarity
 *
 * Combines keyword matching with vector similarity search.
 * Results are sorted by relevance (keyword matches first, then
 * semantic matches by similarity score).
 *
 * Parameters:
 *   topic - Search query
 *   all_results - All memory records to search
 *   all_count - Number of records
 *   match_count_out - Number of matches returned
 *
 * Returns:
 *   Array of matching memory content strings (caller must free)
 *   NULL if no matches or error
 */
char** hybrid_search(const char* topic,
                    memory_record_t** all_results,
                    size_t all_count,
                    size_t* match_count_out);

/**
 * keyword_search_only() - Search using keyword matching only
 *
 * Fallback when semantic search is disabled. Simple case-insensitive
 * substring matching.
 *
 * Parameters:
 *   topic - Search query
 *   all_results - All memory records to search
 *   all_count - Number of records
 *   match_count_out - Number of matches returned
 *
 * Returns:
 *   Array of matching memory content strings (caller must free)
 *   NULL if no matches or error
 */
char** keyword_search_only(const char* topic,
                           memory_record_t** all_results,
                           size_t all_count,
                           size_t* match_count_out);

#endif /* KATRA_BREATHING_SEARCH_H */
