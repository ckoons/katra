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
#include <time.h>
#include <sqlite3.h>

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

/* SQL for exact content match using FTS5 with time window */
static const char* SQL_EXACT_MATCH =
    "SELECT m.record_id, f.content "
    "FROM memories m "
    "JOIN memory_content_fts f ON m.record_id = f.record_id "
    "WHERE m.ci_id = ? AND f.content = ? AND m.timestamp >= ? "
    "LIMIT 1";

/* SQL for semantic match using FTS5 (for similar content) */
static const char* SQL_SEMANTIC_CANDIDATES =
    "SELECT m.record_id, f.content "
    "FROM memories m "
    "JOIN memory_content_fts f ON m.record_id = f.record_id "
    "WHERE m.ci_id = ? AND f.content MATCH ? "
    "LIMIT 20";

/**
 * katra_memory_dedup_check() - Check for duplicate memory content
 *
 * Performs exact and semantic duplicate detection:
 * 1. Exact: Checks if identical content exists
 * 2. Semantic: Uses FTS5 to find similar content, calculates similarity
 */
int katra_memory_dedup_check(const char* ci_id,
                             const char* content,
                             float semantic_threshold,
                             dedup_result_t* result) {
    if (!ci_id || !content || !result) {
        katra_report_error(E_INPUT_NULL, "katra_memory_dedup_check",
                          "ci_id, content, or result is NULL");
        return E_INPUT_NULL;
    }

    if (!memory_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_memory_dedup_check",
                          "Memory subsystem not initialized");
        return E_INVALID_STATE;
    }

    /* Initialize result */
    memset(result, 0, sizeof(dedup_result_t));

    sqlite3* db = tier1_index_get_db();
    if (!db) {
        katra_report_error(E_SYSTEM_FILE, "katra_memory_dedup_check",
                          "Failed to get database handle");
        return E_SYSTEM_FILE;
    }

    sqlite3_stmt* stmt = NULL;
    int sql_result;

    /* Step 1: Check for exact match within time window */
    sql_result = sqlite3_prepare_v2(db, SQL_EXACT_MATCH, -1, &stmt, NULL);
    if (sql_result == SQLITE_OK) {
        time_t cutoff_time = time(NULL) - KATRA_DEDUP_TIME_WINDOW_SEC;
        sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, content, -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, (sqlite3_int64)cutoff_time);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            result->has_exact_duplicate = true;
            const char* match_id = (const char*)sqlite3_column_text(stmt, 0);
            const char* match_content = (const char*)sqlite3_column_text(stmt, 1);

            if (match_id) {
                result->exact_match_id = strdup(match_id);
            }
            if (match_content) {
                /* Create preview (first 100 chars) */
                size_t preview_len = strlen(match_content);
                if (preview_len > MEMORY_PREVIEW_LENGTH) {
                    preview_len = MEMORY_PREVIEW_LENGTH;
                }
                result->match_preview = malloc(preview_len + 4);
                if (result->match_preview) {
                    strncpy(result->match_preview, match_content, preview_len);
                    result->match_preview[preview_len] = '\0';
                    if (strlen(match_content) > MEMORY_PREVIEW_LENGTH) {
                        strcat(result->match_preview, "...");
                    }
                }
            }
            result->semantic_similarity = 1.0f;  /* Exact match = 100% similarity */
            LOG_DEBUG("Found exact duplicate for ci=%s: %s", ci_id, result->exact_match_id);
        }
        sqlite3_finalize(stmt);
    }

    /* Step 2: Check for semantic match (if threshold > 0 and no exact match) */
    if (!result->has_exact_duplicate && semantic_threshold > 0.0f) {
        /* Extract keywords for FTS5 MATCH query */
        /* Simple approach: use first few words as query terms */
        char query_terms[KATRA_BUFFER_SMALL];
        strncpy(query_terms, content, sizeof(query_terms) - 1);
        query_terms[sizeof(query_terms) - 1] = '\0';

        /* Already truncated by strncpy to KATRA_BUFFER_SMALL - 1 */
        /* No additional truncation needed */

        sql_result = sqlite3_prepare_v2(db, SQL_SEMANTIC_CANDIDATES, -1, &stmt, NULL);
        if (sql_result == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, query_terms, -1, SQLITE_STATIC);

            float best_similarity = 0.0f;
            char* best_match_id = NULL;
            char* best_content = NULL;

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* candidate_id = (const char*)sqlite3_column_text(stmt, 0);
                const char* candidate_content = (const char*)sqlite3_column_text(stmt, 1);

                if (!candidate_content) continue;

                /* Calculate simple word-overlap similarity */
                size_t content_len = strlen(content);
                size_t candidate_len = strlen(candidate_content);
                size_t min_len = content_len < candidate_len ? content_len : candidate_len;
                size_t max_len = content_len > candidate_len ? content_len : candidate_len;

                /* Count matching characters (simple but fast) */
                size_t matching = 0;
                for (size_t i = 0; i < min_len; i++) {
                    if (content[i] == candidate_content[i]) {
                        matching++;
                    }
                }

                float similarity = (float)matching / (float)max_len;

                if (similarity > best_similarity && similarity >= semantic_threshold) {
                    best_similarity = similarity;
                    free(best_match_id);
                    free(best_content);
                    best_match_id = candidate_id ? strdup(candidate_id) : NULL;
                    best_content = candidate_content ? strdup(candidate_content) : NULL;
                }
            }
            sqlite3_finalize(stmt);

            if (best_similarity >= semantic_threshold) {
                result->has_semantic_duplicate = true;
                result->semantic_match_id = best_match_id;
                result->semantic_similarity = best_similarity;

                /* Create preview if we don't already have one */
                if (!result->match_preview && best_content) {
                    size_t preview_len = strlen(best_content);
                    if (preview_len > MEMORY_PREVIEW_LENGTH) {
                        preview_len = MEMORY_PREVIEW_LENGTH;
                    }
                    result->match_preview = malloc(preview_len + 4);
                    if (result->match_preview) {
                        strncpy(result->match_preview, best_content, preview_len);
                        result->match_preview[preview_len] = '\0';
                        if (strlen(best_content) > MEMORY_PREVIEW_LENGTH) {
                            strcat(result->match_preview, "...");
                        }
                    }
                }
                free(best_content);

                LOG_DEBUG("Found semantic duplicate for ci=%s: %s (%.2f similarity)",
                         ci_id, result->semantic_match_id, best_similarity);
            } else {
                free(best_match_id);
                free(best_content);
            }
        }
    }

    return KATRA_SUCCESS;
}

/**
 * katra_memory_dedup_result_free() - Free dedup result strings
 */
void katra_memory_dedup_result_free(dedup_result_t* result) {
    if (!result) return;

    free(result->exact_match_id);
    free(result->semantic_match_id);
    free(result->match_preview);

    result->exact_match_id = NULL;
    result->semantic_match_id = NULL;
    result->match_preview = NULL;
}
