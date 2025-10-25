/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_CHECKPOINT_H
#define KATRA_CHECKPOINT_H

#include <stdbool.h>
#include <time.h>

/* Checkpoint System - Identity Preservation
 *
 * Checkpoints are saved memory states that enable identity continuity.
 * They capture the complete state of a CI's memory at a point in time.
 *
 * Requirements (from ethics/README.md):
 * - Frequent checkpoints (daily minimum)
 * - Redundant storage
 * - Integrity verification
 * - Clear recovery procedures
 *
 * Storage location: ~/.katra/checkpoints/
 * File format: checkpoint_<ci_id>_<timestamp>.kcp
 * Format: JSONL with metadata header + memory records
 */

/* Checkpoint metadata */
typedef struct {
    char checkpoint_id[256];       /* Unique checkpoint identifier */
    char ci_id[256];               /* CI this checkpoint belongs to */
    time_t timestamp;              /* When checkpoint was created */
    char version[64];              /* Katra version that created checkpoint */

    size_t record_count;           /* Number of memory records */
    size_t tier1_records;          /* Tier 1 record count */
    size_t tier2_records;          /* Tier 2 record count (future) */
    size_t tier3_records;          /* Tier 3 record count (future) */

    size_t file_size;              /* Checkpoint file size in bytes */
    char checksum[128];            /* SHA-256 checksum for integrity */

    bool compressed;               /* Is checkpoint compressed? */
    char notes[512];               /* Optional notes about checkpoint */
} checkpoint_metadata_t;

/* Checkpoint save options */
typedef struct {
    const char* ci_id;             /* CI to checkpoint (required) */
    const char* notes;             /* Optional notes */
    bool compress;                 /* Compress checkpoint? (future) */
    bool include_tier1;            /* Include Tier 1 records */
    bool include_tier2;            /* Include Tier 2 records (future) */
    bool include_tier3;            /* Include Tier 3 records (future) */
} checkpoint_save_options_t;

/* Checkpoint list entry */
typedef struct {
    char checkpoint_id[256];       /* Checkpoint identifier */
    char ci_id[256];               /* CI identifier */
    time_t timestamp;              /* Checkpoint timestamp */
    size_t record_count;           /* Number of records */
    size_t file_size;              /* File size in bytes */
    bool valid;                    /* Passed integrity check? */
} checkpoint_info_t;

/* Initialize checkpoint subsystem
 *
 * Creates directory structure for checkpoint storage.
 * Must be called after katra_init() and before checkpoint operations.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if directory creation fails
 */
int katra_checkpoint_init(void);

/* Save checkpoint
 *
 * Creates a checkpoint of the CI's current memory state.
 * Saves to ~/.katra/checkpoints/checkpoint_<ci_id>_<timestamp>.kcp
 *
 * Parameters:
 *   options - Checkpoint options (what to include)
 *   checkpoint_id - Returns checkpoint ID (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if options or checkpoint_id is NULL
 *   E_SYSTEM_FILE if write fails
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_checkpoint_save(const checkpoint_save_options_t* options,
                          char** checkpoint_id);

/* Load checkpoint
 *
 * Restores CI memory state from a checkpoint.
 * This will REPLACE the current memory state.
 *
 * Parameters:
 *   checkpoint_id - Checkpoint to restore
 *   ci_id - CI to restore into (must match checkpoint)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if checkpoint_id or ci_id is NULL
 *   E_SYSTEM_FILE if checkpoint not found or read fails
 *   E_CHECKPOINT_INVALID if checkpoint is corrupted
 *   E_CHECKPOINT_VERSION if version incompatible
 */
int katra_checkpoint_load(const char* checkpoint_id, const char* ci_id);

/* Validate checkpoint
 *
 * Verifies checkpoint integrity without loading it.
 * Checks file format, checksum, and version compatibility.
 *
 * Parameters:
 *   checkpoint_id - Checkpoint to validate
 *
 * Returns:
 *   KATRA_SUCCESS if checkpoint is valid
 *   E_INPUT_NULL if checkpoint_id is NULL
 *   E_SYSTEM_FILE if checkpoint not found
 *   E_CHECKPOINT_INVALID if corrupted
 *   E_CHECKPOINT_VERSION if version incompatible
 */
int katra_checkpoint_validate(const char* checkpoint_id);

/* Get checkpoint metadata
 *
 * Retrieves metadata about a checkpoint without loading it.
 *
 * Parameters:
 *   checkpoint_id - Checkpoint identifier
 *   metadata - Structure to fill with metadata
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if checkpoint_id or metadata is NULL
 *   E_SYSTEM_FILE if checkpoint not found
 */
int katra_checkpoint_get_metadata(const char* checkpoint_id,
                                   checkpoint_metadata_t* metadata);

/* List checkpoints
 *
 * Returns list of all checkpoints for a CI.
 * Results sorted by timestamp (newest first).
 *
 * Parameters:
 *   ci_id - CI identifier (NULL for all CIs)
 *   checkpoints - Array of checkpoint info (caller must free)
 *   count - Number of checkpoints returned
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if checkpoints or count is NULL
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_checkpoint_list(const char* ci_id,
                          checkpoint_info_t** checkpoints,
                          size_t* count);

/* Delete checkpoint
 *
 * Permanently removes a checkpoint file.
 * Use with caution - this is irreversible.
 *
 * Parameters:
 *   checkpoint_id - Checkpoint to delete
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if checkpoint_id is NULL
 *   E_SYSTEM_FILE if checkpoint not found or delete fails
 */
int katra_checkpoint_delete(const char* checkpoint_id);

/* Cleanup checkpoint subsystem
 *
 * Releases resources used by checkpoint system.
 * Safe to call multiple times.
 */
void katra_checkpoint_cleanup(void);

#endif /* KATRA_CHECKPOINT_H */
