/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_MM_INDEX_H
#define KATRA_MM_INDEX_H

/**
 * @file katra_mm_index.h
 * @brief SQLite persistence layer for metamemory nodes
 *
 * Provides database operations for storing, loading, and querying
 * metamemory nodes. Each project has its own database.
 */

#include <stdbool.h>
#include <stddef.h>

#include "katra_metamemory.h"

/* ============================================================================
 * Types
 * ============================================================================ */

/**
 * Index statistics.
 */
typedef struct {
    size_t total_nodes;
    size_t concept_count;
    size_t component_count;
    size_t function_count;
    size_t struct_count;
    size_t link_count;
    size_t file_count;
} mm_index_stats_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

/**
 * Initialize index for a project.
 *
 * Opens (or creates) the SQLite database for the project.
 * Must be called before any other index operations.
 *
 * Parameters:
 *   project_id - Project identifier
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if database cannot be opened
 */
int mm_index_init(const char* project_id);

/**
 * Close the index database.
 */
void mm_index_close(void);

/**
 * Check if index is initialized.
 */
bool mm_index_is_initialized(void);

/* ============================================================================
 * Node Operations
 * ============================================================================ */

/**
 * Store a node in the index.
 *
 * Stores the node and all its relationships (links, tasks, params, fields).
 * If a node with the same ID exists, it will be replaced.
 *
 * Parameters:
 *   node - Node to store
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int mm_index_store_node(const metamemory_node_t* node);

/**
 * Load a node from the index.
 *
 * Loads the node and all its relationships.
 *
 * Parameters:
 *   node_id - Node ID to load
 *   out - Output pointer (caller must free with metamemory_free_node)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_NOT_FOUND if node doesn't exist
 */
int mm_index_load_node(const char* node_id, metamemory_node_t** out);

/**
 * Delete a node from the index.
 *
 * Parameters:
 *   node_id - Node ID to delete
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int mm_index_delete_node(const char* node_id);

/**
 * Delete all nodes from a file.
 *
 * Used during refresh to clean up before re-scanning.
 *
 * Parameters:
 *   file_path - File path to delete nodes for
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int mm_index_delete_by_file(const char* file_path);

/* ============================================================================
 * Search Operations
 * ============================================================================ */

/**
 * Search for concepts matching a query.
 *
 * Searches concept names and purposes.
 *
 * Parameters:
 *   query - Search query
 *   results - Output array (caller must free)
 *   count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int mm_index_search_concepts(const char* query,
                             metamemory_node_t*** results,
                             size_t* count);

/**
 * Search for code elements matching a query.
 *
 * Searches function names, signatures, struct names.
 *
 * Parameters:
 *   query - Search query
 *   types - Type filter (NULL = functions and structs)
 *   type_count - Number of types
 *   results - Output array (caller must free)
 *   count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int mm_index_search_code(const char* query,
                         const metamemory_type_t* types,
                         size_t type_count,
                         metamemory_node_t*** results,
                         size_t* count);

/**
 * Get linked nodes.
 *
 * Parameters:
 *   node_id - Source node ID
 *   link_type - Type of link (NULL = all links)
 *   target_ids - Output array of target IDs (caller must free)
 *   count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int mm_index_get_links(const char* node_id,
                       const char* link_type,
                       char*** target_ids,
                       size_t* count);

/* ============================================================================
 * File Hash Operations
 * ============================================================================ */

/**
 * Store file hash for change detection.
 */
int mm_index_store_file_hash(const char* file_path, const char* hash);

/**
 * Get stored file hash.
 *
 * Returns:
 *   KATRA_SUCCESS if hash found
 *   E_NOT_FOUND if file not indexed
 */
int mm_index_get_file_hash(const char* file_path, char* hash, size_t hash_size);

/* ============================================================================
 * Statistics
 * ============================================================================ */

/**
 * Get index statistics.
 */
int mm_index_get_stats(mm_index_stats_t* stats);

#endif /* KATRA_MM_INDEX_H */
