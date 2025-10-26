/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_TIER2_INDEX_H
#define KATRA_TIER2_INDEX_H

#include "katra_tier2.h"
#include <sqlite3.h>

/* Tier 2 Index: SQLite-based fast lookup
 *
 * Purpose:
 *   - Fast queries by CI ID, time, type, themes, keywords
 *   - O(log n) instead of O(n) file scans
 *   - Maintains pointers to JSONL file locations
 *
 * Schema:
 *   - digests: Main table with digest metadata
 *   - themes: Theme extraction for searching
 *   - keywords: Keyword extraction for searching
 *
 * Location: ~/.katra/memory/tier2/index/digests.db
 */

/* Index location structure */
typedef struct {
    char file_path[512];    /* JSONL file path */
    long offset;            /* Byte offset in file */
} index_location_t;

/* Initialize Tier 2 index database
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
int tier2_index_init(const char* ci_id);

/* Add digest to index
 *
 * Inserts digest metadata into SQLite index.
 * Called automatically by tier2_store_digest().
 *
 * Parameters:
 *   digest - Digest record to index
 *   file_path - JSONL file path where digest is stored
 *   offset - Byte offset in file
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if digest is NULL
 *   E_SYSTEM_FILE if database write fails
 */
int tier2_index_add(const digest_record_t* digest,
                    const char* file_path,
                    long offset);

/* Query index for matching digest IDs
 *
 * Fast lookup using SQLite indexes.
 * Returns digest IDs and file locations.
 *
 * Parameters:
 *   query - Query parameters
 *   digest_ids - Array of digest IDs (caller must free)
 *   locations - Array of file locations (caller must free)
 *   count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_SYSTEM_FILE if database read fails
 */
int tier2_index_query(const digest_query_t* query,
                      char*** digest_ids,
                      index_location_t** locations,
                      size_t* count);

/* Load specific digests by ID
 *
 * Reads digests from JSONL files using index locations.
 *
 * Parameters:
 *   locations - Array of file locations
 *   count - Number of locations
 *   results - Array of digest records (caller must free)
 *   result_count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_SYSTEM_FILE if file read fails
 */
int tier2_load_by_locations(const index_location_t* locations,
                             size_t count,
                             digest_record_t*** results,
                             size_t* result_count);

/* Rebuild index from JSONL files
 *
 * Scans all digest files and rebuilds index.
 * Used for recovery or initial index creation.
 *
 * Parameters:
 *   ci_id - CI identifier
 *
 * Returns:
 *   Number of digests indexed, or negative error code
 */
int tier2_index_rebuild(const char* ci_id);

/* Check if index exists
 *
 * Returns:
 *   true if index database exists
 *   false otherwise
 */
bool tier2_index_exists(const char* ci_id);

/* Cleanup index resources
 *
 * Closes database connections.
 */
void tier2_index_cleanup(void);

/* Get index statistics
 *
 * Parameters:
 *   ci_id - CI identifier
 *   digest_count - Number of indexed digests
 *   theme_count - Number of unique themes
 *   keyword_count - Number of unique keywords
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int tier2_index_stats(const char* ci_id,
                      size_t* digest_count,
                      size_t* theme_count,
                      size_t* keyword_count);

#endif /* KATRA_TIER2_INDEX_H */
