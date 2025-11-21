/* © 2025 Casey Koons All rights reserved */

/*
 * katra_memory_ops.c - Memory deletion operations
 *
 * Provides deletion operations for the memory subsystem.
 * Split from katra_memory.c to stay under 600 line budget.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_tier1_index.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* External symbols from katra_memory.c */
extern bool memory_initialized;

/**
 * katra_memory_delete_session_scoped() - Delete all session-scoped memories
 *
 * Deletes all memory records marked with session_scoped=true flag.
 * Called during session_end() to clear working memory.
 *
 * Implementation:
 * 1. Query tier1 index for session_scoped=1 records
 * 2. Delete index entries
 * 3. Mark JSONL records as deleted (set archived=true)
 *
 * Note: Physical JSONL file compaction happens during consolidation.
 */
int katra_memory_delete_session_scoped(const char* ci_id, size_t* deleted_count) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_memory_delete_session_scoped",
                          "ci_id is NULL");
        return E_INPUT_NULL;
    }

    if (!memory_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_memory_delete_session_scoped",
                          "Memory subsystem not initialized");
        return E_INVALID_STATE;
    }

    /* Get database handle */
    sqlite3* db = tier1_index_get_db();
    if (!db) {
        katra_report_error(E_SYSTEM_FILE, "katra_memory_delete_session_scoped",
                          "Failed to get database handle");
        return E_SYSTEM_FILE;
    }

    int result = KATRA_SUCCESS;
    size_t count = 0;

    /* Delete from index (session_scoped=1) */
    const char* delete_sql = "DELETE FROM memories WHERE ci_id = ? AND session_scoped = 1";
    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(db, delete_sql, -1, &stmt, NULL) != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "katra_memory_delete_session_scoped",
                          "Failed to prepare DELETE statement");
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_DONE) {
        count = (size_t)sqlite3_changes(db);
        LOG_DEBUG("Deleted %zu session-scoped memory index entries", count);
        result = KATRA_SUCCESS;
    } else {
        katra_report_error(E_SYSTEM_FILE, "katra_memory_delete_session_scoped",
                          "Failed to delete session-scoped memories");
        result = E_SYSTEM_FILE;
    }

    sqlite3_finalize(stmt);

    /* Return count if requested */
    if (deleted_count) {
        *deleted_count = count;
    }

    /* Note: Physical JSONL records remain until consolidation */
    /* They are effectively deleted from index, so won't appear in queries */

    return result;
}
