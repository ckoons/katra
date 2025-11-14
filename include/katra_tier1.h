/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_TIER1_H
#define KATRA_TIER1_H

#include <stdio.h>
#include "katra_memory.h"

/* Tier 1: Raw Recordings
 *
 * Short-term memory storage (days to weeks).
 * Every interaction captured verbatim.
 *
 * Storage format: JSONL (one JSON object per line)
 * Organization: One file per day: ~/.katra/memory/tier1/YYYY-MM-DD.jsonl
 *
 * Retention: Configurable (default 14 days)
 * Archive: Old recordings moved to Tier 2 (sleep digests)
 */

/* Tier 1 configuration */
#define TIER1_RETENTION_DAYS     14      /* Default retention period */
#define TIER1_MAX_FILE_SIZE_MB   100     /* Max size per daily file */
#define TIER1_BUFFER_SIZE        4096    /* Write buffer size */

/* Initialize Tier 1 storage
 *
 * Creates directory structure for raw recordings.
 *
 * Parameters:
 *   ci_id - CI identifier
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if directory creation fails
 */
int tier1_init(const char* ci_id);

/* Store raw recording
 *
 * Appends memory record to today's JSONL file.
 * Creates new file if needed.
 *
 * Parameters:
 *   record - Memory record to store
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if record is NULL
 *   E_SYSTEM_FILE if write fails
 *   E_MEMORY_TIER_FULL if daily file exceeds size limit
 */
int tier1_store(const memory_record_t* record);

/* Query Tier 1 recordings
 *
 * Searches raw recordings based on query parameters.
 * Scans daily files in reverse chronological order.
 *
 * Parameters:
 *   query - Query parameters
 *   results - Array to receive results
 *   count - Number of results returned
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if query or results is NULL
 *   E_SYSTEM_MEMORY if allocation fails
 */
int tier1_query(const memory_query_t* query,
                memory_record_t*** results,
                size_t* count);

/* Archive old Tier 1 recordings
 *
 * Moves recordings older than max_age_days to archive.
 * Marks records as archived (ready for Tier 2 processing).
 *
 * Parameters:
 *   ci_id - CI identifier
 *   max_age_days - Age threshold for archival
 *
 * Returns:
 *   Number of records archived, or negative error code
 */
int tier1_archive(const char* ci_id, int max_age_days);

/* Get Tier 1 statistics
 *
 * Returns stats about raw recordings storage.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   total_records - Number of records stored
 *   bytes_used - Bytes used
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int tier1_stats(const char* ci_id, size_t* total_records, size_t* bytes_used);

/* Get Tier 1 directory path
 *
 * Helper function for archive module and index rebuild.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   buffer - Output buffer for path
 *   size - Buffer size
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int tier1_get_dir(const char* ci_id, char* buffer, size_t size);

/* Collect JSONL files from Tier 1 directory
 *
 * Helper function for archive module and index rebuild.
 *
 * Parameters:
 *   tier1_dir - Tier 1 directory path
 *   filenames - Output array of filenames (caller must free)
 *   count - Number of files found
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int tier1_collect_jsonl_files(const char* tier1_dir, char*** filenames, size_t* count);

/* Flush Tier 1 storage to disk
 *
 * Forces all pending writes to disk for crash safety.
 * Should be called periodically (e.g., every 6 hours).
 *
 * Parameters:
 *   ci_id - CI identifier
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id is NULL
 *   E_SYSTEM_FILE if directory access fails
 */
int tier1_flush(const char* ci_id);

/* Cleanup Tier 1 storage
 *
 * Flushes pending writes and releases resources.
 */
void tier1_cleanup(void);

/* JSON parsing/serialization helpers (internal) */
void katra_tier1_json_unescape(const char* src, char* dst, size_t dst_size);
int katra_tier1_parse_json_record(const char* line, memory_record_t** record);
int katra_tier1_write_json_record(FILE* fp, const memory_record_t* record);

#endif /* KATRA_TIER1_H */
