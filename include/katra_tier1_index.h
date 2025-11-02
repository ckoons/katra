/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_TIER1_INDEX_H
#define KATRA_TIER1_INDEX_H

#include "katra_memory.h"
#include <sqlite3.h>
#include <stdbool.h>

/* Tier 1 Index: SQLite-based fast lookup for memories
 *
 * Purpose:
 *   - Fast queries by CI ID, time, type, importance, content
 *   - O(log n) instead of O(n) file scans
 *   - Maintains pointers to JSONL file locations
 *   - Supports convergence detection (find related memories)
 *   - Enables interstitial conversation analyzer
 *
 * Schema:
 *   - memories: Main table with memory metadata
 *   - memory_content_fts: Full-text search on content
 *   - memory_connections: Graph relationship cache
 *   - memory_themes: Theme extraction for automatic capture
 *
 * Location: ~/.katra/memory/tier1/index/memories.db
 */

/* Index location structure */
typedef struct {
    char file_path[512];    /* JSONL file path */
    long offset;            /* Byte offset in file */
} memory_location_t;

/* Get database handle
 *
 * Returns:
 *   SQLite database handle or NULL if not initialized
 */
sqlite3* tier1_index_get_db(void);

/* Initialize Tier 1 index database
 *
 * Creates SQLite database and schema if not exists.
 *
 * Parameters:
 *   ci_id - CI identifier
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if database creation fails
 */
int tier1_index_init(const char* ci_id);

/* Add memory to index
 *
 * Inserts memory metadata into SQLite index.
 * Called automatically by tier1_store().
 *
 * Parameters:
 *   record - Memory record to index
 *   file_path - JSONL file path where memory is stored
 *   offset - Byte offset in file
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if record is NULL
 *   E_SYSTEM_FILE if database write fails
 */
int tier1_index_add(const memory_record_t* record,
                    const char* file_path,
                    long offset);

/* Query index for matching memory IDs
 *
 * Fast lookup using SQLite indexes and FTS.
 * Returns memory IDs and file locations.
 *
 * Parameters:
 *   query - Query parameters
 *   record_ids - Array of record IDs (caller must free)
 *   locations - Array of file locations (caller must free)
 *   count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_SYSTEM_FILE if database read fails
 */
int tier1_index_query(const memory_query_t* query,
                      char*** record_ids,
                      memory_location_t** locations,
                      size_t* count);

/* Find similar memories (for convergence detection)
 *
 * Searches for memories with similar content/themes.
 * Used by interstitial conversation analyzer.
 *
 * Parameters:
 *   content - Content to search for
 *   importance_threshold - Minimum importance (0.0-1.0)
 *   time_window_hours - Look back this many hours (0 = unlimited)
 *   record_ids - Array of record IDs (caller must free)
 *   locations - Array of file locations (caller must free)
 *   count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int tier1_index_find_similar(const char* content,
                             float importance_threshold,
                             int time_window_hours,
                             char*** record_ids,
                             memory_location_t** locations,
                             size_t* count);

/* Load specific memories by ID
 *
 * Reads memories from JSONL files using index locations.
 *
 * Parameters:
 *   locations - Array of file locations
 *   count - Number of locations
 *   results - Array of memory records (caller must free)
 *   result_count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_SYSTEM_FILE if file read fails
 */
int tier1_load_by_locations(const memory_location_t* locations,
                            size_t count,
                            memory_record_t*** results,
                            size_t* result_count);

/* Update memory metadata in index
 *
 * Updates importance, access count, centrality without full reindex.
 *
 * Parameters:
 *   record_id - Memory record ID
 *   importance - New importance score
 *   access_count - New access count
 *   centrality - New graph centrality
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int tier1_index_update_metadata(const char* record_id,
                                float importance,
                                size_t access_count,
                                float centrality);

/* Mark memory as archived
 *
 * Updates index to reflect archival status.
 *
 * Parameters:
 *   record_id - Memory record ID
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int tier1_index_mark_archived(const char* record_id);

/* Rebuild index from JSONL files
 *
 * Scans all memory files and rebuilds index.
 * Used for recovery or initial index creation.
 *
 * Parameters:
 *   ci_id - CI identifier
 *
 * Returns:
 *   Number of memories indexed, or negative error code
 */
int tier1_index_rebuild(const char* ci_id);

/* Check if index exists
 *
 * Returns:
 *   true if index database exists
 *   false otherwise
 */
bool tier1_index_exists(const char* ci_id);

/* Cleanup index resources
 *
 * Closes database connections.
 */
void tier1_index_cleanup(void);

/* Get index statistics
 *
 * Parameters:
 *   ci_id - CI identifier
 *   memory_count - Number of indexed memories
 *   theme_count - Number of unique themes
 *   connection_count - Number of graph connections
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int tier1_index_stats(const char* ci_id,
                      size_t* memory_count,
                      size_t* theme_count,
                      size_t* connection_count);

#endif /* KATRA_TIER1_INDEX_H */
