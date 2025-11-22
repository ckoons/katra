/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_working_memory.c - Working Memory Budget Management
 *
 * Phase 2: Implements automatic working memory hygiene
 * - Tracks session-scoped memory count
 * - Archives oldest at soft limit (convert to permanent)
 * - Deletes oldest at hard limit
 * - Provides stats visibility to CI
 *
 * Design:
 * - Time-based (older memories fade naturally)
 * - Hybrid archival (soft=archive, hard=delete)
 * - Enabled by default, configurable
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_breathing_internal.h"
#include "katra_memory.h"
#include "katra_tier1_index.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* No external symbols needed - use accessor functions */

/* ============================================================================
 * WORKING MEMORY STATISTICS
 * ============================================================================ */

/**
 * working_memory_get_count() - Get count of active session-scoped memories
 *
 * Parameters:
 *   ci_id - CI identity string
 *   count - Output: number of session-scoped memories
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id or count is NULL
 *   E_SYSTEM_FILE on database error
 */
int working_memory_get_count(const char* ci_id, size_t* count) {
    KATRA_CHECK_NULL(ci_id);
    KATRA_CHECK_NULL(count);

    *count = 0;

    /* Get database handle */
    sqlite3* db = tier1_index_get_db();
    if (!db) {
        katra_report_error(E_SYSTEM_FILE, "working_memory_get_count",
                          "Failed to get database handle");
        return E_SYSTEM_FILE;
    }

    /* Count session-scoped memories */
    const char* count_sql = "SELECT COUNT(*) FROM memories WHERE ci_id = ? AND session_scoped = 1";
    sqlite3_stmt* stmt = NULL;

    int result = KATRA_SUCCESS;

    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL) != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "working_memory_get_count",
                          "Failed to prepare COUNT statement");
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *count = (size_t)sqlite3_column_int64(stmt, 0);
        LOG_DEBUG("Session-scoped memory count for %s: %zu", ci_id, *count);
    } else {
        katra_report_error(E_SYSTEM_FILE, "working_memory_get_count",
                          "Failed to execute COUNT query");
        result = E_SYSTEM_FILE;
    }

    sqlite3_finalize(stmt);
    return result;
}

/* ============================================================================
 * WORKING MEMORY ARCHIVAL
 * ============================================================================ */

/**
 * working_memory_archive_oldest() - Archive oldest session-scoped memories
 *
 * Hybrid strategy:
 * - At soft limit: Convert to permanent (remove "session" tag, set session_scoped=false)
 * - At hard limit: Delete entirely
 *
 * Parameters:
 *   ci_id - CI identity string
 *   count_to_process - How many oldest memories to process
 *   at_hard_limit - true=delete, false=archive (convert to permanent)
 *   processed_count - Output: number of memories processed
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id is NULL
 *   E_SYSTEM_FILE on database error
 */
int working_memory_archive_oldest(const char* ci_id, size_t count_to_process,
                                   bool at_hard_limit, size_t* processed_count) {
    KATRA_CHECK_NULL(ci_id);

    if (processed_count) {
        *processed_count = 0;
    }

    /* Get database handle */
    sqlite3* db = tier1_index_get_db();
    if (!db) {
        katra_report_error(E_SYSTEM_FILE, "working_memory_archive_oldest",
                          "Failed to get database handle");
        return E_SYSTEM_FILE;
    }

    int result = KATRA_SUCCESS;
    size_t count = 0;

    if (at_hard_limit) {
        /* Hard limit: Delete oldest memories entirely */
        const char* delete_sql =
            "DELETE FROM memories "
            "WHERE record_id IN ("
            "  SELECT record_id FROM memories "
            "  WHERE ci_id = ? AND session_scoped = 1 "
            "  ORDER BY timestamp ASC "
            "  LIMIT ?"
            ")";

        sqlite3_stmt* stmt = NULL;
        if (sqlite3_prepare_v2(db, delete_sql, -1, &stmt, NULL) != SQLITE_OK) {
            katra_report_error(E_SYSTEM_FILE, "working_memory_archive_oldest",
                              "Failed to prepare DELETE statement");
            return E_SYSTEM_FILE;
        }

        sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 2, (sqlite3_int64)count_to_process);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            count = (size_t)sqlite3_changes(db);
            LOG_INFO("Deleted %zu oldest session-scoped memories (hard limit)", count);
        } else {
            katra_report_error(E_SYSTEM_FILE, "working_memory_archive_oldest",
                              "Failed to delete oldest session memories");
            result = E_SYSTEM_FILE;
        }

        sqlite3_finalize(stmt);

    } else {
        /* Soft limit: Convert to permanent (remove session scope) */
        const char* archive_sql =
            "UPDATE memories "
            "SET session_scoped = 0 "
            "WHERE record_id IN ("
            "  SELECT record_id FROM memories "
            "  WHERE ci_id = ? AND session_scoped = 1 "
            "  ORDER BY timestamp ASC "
            "  LIMIT ?"
            ")";

        sqlite3_stmt* stmt = NULL;
        if (sqlite3_prepare_v2(db, archive_sql, -1, &stmt, NULL) != SQLITE_OK) {
            katra_report_error(E_SYSTEM_FILE, "working_memory_archive_oldest",
                              "Failed to prepare UPDATE statement");
            return E_SYSTEM_FILE;
        }

        sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 2, (sqlite3_int64)count_to_process);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            count = (size_t)sqlite3_changes(db);
            LOG_INFO("Archived %zu oldest session-scoped memories (soft limit)", count);
        } else {
            katra_report_error(E_SYSTEM_FILE, "working_memory_archive_oldest",
                              "Failed to archive oldest session memories");
            result = E_SYSTEM_FILE;
        }

        sqlite3_finalize(stmt);
    }

    if (processed_count) {
        *processed_count = count;
    }

    return result;
}

/* ============================================================================
 * WORKING MEMORY BUDGET ENFORCEMENT
 * ============================================================================ */

/**
 * working_memory_check_budget() - Check and enforce working memory budget
 *
 * Called during periodic maintenance (every ~30s).
 *
 * Strategy:
 * - If count >= hard_limit: Delete oldest (batch_size) memories
 * - Else if count >= soft_limit: Archive oldest (batch_size) memories
 * - Else: No action
 *
 * Parameters:
 *   ci_id - CI identity string
 *   processed_count - Output: number of memories processed (optional)
 *
 * Returns:
 *   KATRA_SUCCESS on success (even if no action taken)
 *   E_INPUT_NULL if ci_id is NULL
 *   E_SYSTEM_FILE on database error
 */
int working_memory_check_budget(const char* ci_id, size_t* processed_count) {
    KATRA_CHECK_NULL(ci_id);

    if (processed_count) {
        *processed_count = 0;
    }

    /* Get config pointer */
    context_config_t* config = breathing_get_config_ptr();
    if (!config) {
        LOG_DEBUG("Working memory budget: config not available");
        return KATRA_SUCCESS;
    }

    /* Check if enabled */
    if (!config->working_memory_enabled) {
        LOG_DEBUG("Working memory budget disabled - skipping check");
        return KATRA_SUCCESS;
    }

    /* Get current count */
    size_t current_count = 0;
    int result = working_memory_get_count(ci_id, &current_count);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check limits */
    size_t hard_limit = config->working_memory_hard_limit;
    size_t soft_limit = config->working_memory_soft_limit;
    size_t batch_size = config->working_memory_batch_size;

    if (current_count >= hard_limit) {
        /* Hard limit: Delete oldest */
        LOG_WARN("Working memory at hard limit (%zu/%zu) - deleting oldest %zu",
                current_count, hard_limit, batch_size);

        size_t deleted = 0;
        result = working_memory_archive_oldest(ci_id, batch_size, true, &deleted);

        if (result == KATRA_SUCCESS && processed_count) {
            *processed_count = deleted;
        }

    } else if (current_count >= soft_limit) {
        /* Soft limit: Archive oldest */
        LOG_INFO("Working memory at soft limit (%zu/%zu) - archiving oldest %zu",
                current_count, soft_limit, batch_size);

        size_t archived = 0;
        result = working_memory_archive_oldest(ci_id, batch_size, false, &archived);

        if (result == KATRA_SUCCESS && processed_count) {
            *processed_count = archived;
        }

    } else {
        /* Within budget - no action */
        LOG_DEBUG("Working memory within budget (%zu/%zu/%zu)",
                 current_count, soft_limit, hard_limit);
    }

    return result;
}
