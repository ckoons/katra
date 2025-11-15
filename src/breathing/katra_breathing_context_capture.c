/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_context_capture.c - Context snapshot capture and latent space
 *
 * Part of context persistence split. Contains snapshot capture and
 * latent space generation for session continuity.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_breathing_context_persist.h"
#include "katra_breathing_context_persist_internal.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* ============================================================================
 * SQL STATEMENTS
 * ============================================================================ */

/* GUIDELINE_APPROVED: SQL DML statements */
static const char* SQL_INSERT_SNAPSHOT =
    "INSERT OR REPLACE INTO context_snapshots ("
    "  snapshot_id, ci_id, session_id, snapshot_time,"
    "  current_focus, active_reasoning, communication_style,"
    "  user_preferences, recent_accomplishments, active_goals,"
    "  thinking_patterns, learned_lessons"
    ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

static const char* SQL_INSERT_QUESTION =
    "INSERT INTO pending_questions (snapshot_id, question_text) VALUES (?, ?);";

static const char* SQL_INSERT_FILE =
    "INSERT INTO modified_files (snapshot_id, file_path, modification_type) VALUES (?, ?, ?);";

static const char* SQL_GET_LATEST_SNAPSHOT =
    "SELECT * FROM context_snapshots WHERE ci_id = ? "
    "ORDER BY snapshot_time DESC LIMIT 1;";

static const char* SQL_GET_QUESTIONS =
    "SELECT question_text FROM pending_questions WHERE snapshot_id = ?;";
/* GUIDELINE_APPROVED_END */

/* ============================================================================
 * SNAPSHOT CAPTURE
 * ============================================================================ */

int capture_context_snapshot(const char* ci_id, const char* focus_description) {
    int result = KATRA_SUCCESS;
    sqlite3_stmt* stmt = NULL;
    char snapshot_id[KATRA_BUFFER_NAME];
    working_context_t* ctx = context_persist_get_working_context();
    sqlite3* db = context_persist_get_db();

    if (!ci_id) {
        result = E_INPUT_NULL;
        goto cleanup;
    }

    if (!context_persist_is_initialized() || !ctx || !db) {
        result = E_INVALID_STATE;
        goto cleanup;
    }

    /* Generate snapshot ID */
    time_t now = time(NULL);
    snprintf(snapshot_id, sizeof(snapshot_id), "%s_%ld", ci_id, (long)now);

    /* Update focus if provided */
    if (focus_description) {
        update_current_focus(focus_description);
    }

    /* Insert main snapshot record */
    int rc = sqlite3_prepare_v2(db, SQL_INSERT_SNAPSHOT, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    sqlite3_bind_text(stmt, 1, snapshot_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, ctx->session_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, now);
    sqlite3_bind_text(stmt, 5, ctx->current_focus, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, ctx->active_reasoning, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, ctx->communication_style, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, ctx->user_preferences, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, ctx->recent_accomplishments, -1, SQLITE_STATIC);
    /* GUIDELINE_APPROVED: SQLite bind parameter indices are positional API requirements */
    sqlite3_bind_text(stmt, 10, ctx->active_goals, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, ctx->thinking_patterns, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, ctx->learned_lessons, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    /* Insert pending questions */
    for (size_t i = 0; i < ctx->pending_question_count; i++) {
        rc = sqlite3_prepare_v2(db, SQL_INSERT_QUESTION, -1, &stmt, NULL);
        if (rc != SQLITE_OK) continue;

        sqlite3_bind_text(stmt, 1, snapshot_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, ctx->pending_questions[i], -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        stmt = NULL;
    }

    /* Insert modified files */
    for (size_t i = 0; i < ctx->modified_file_count; i++) {
        /* Parse "path:type" format */
        char* colon = strchr(ctx->modified_files[i], ':');
        if (!colon) continue;

        *colon = '\0';
        const char* path = ctx->modified_files[i];
        const char* type = colon + 1;

        rc = sqlite3_prepare_v2(db, SQL_INSERT_FILE, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            *colon = ':';
            continue;
        }

        sqlite3_bind_text(stmt, 1, snapshot_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, path, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, type, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        stmt = NULL;

        *colon = ':';
    }

    LOG_INFO("Captured context snapshot: %s", snapshot_id);

cleanup:
    if (stmt) sqlite3_finalize(stmt);
    return result;
}

/* ============================================================================
 * LATENT SPACE GENERATION
 * ============================================================================ */

char* restore_context_as_latent_space(const char* ci_id) {
    int rc;
    sqlite3_stmt* stmt = NULL;
    char* latent_space = NULL;
    size_t capacity = KATRA_BUFFER_LARGE;
    size_t offset = 0;
    sqlite3* db = context_persist_get_db();

    if (!ci_id || !db) {
        return NULL;
    }

    latent_space = malloc(capacity);
    if (!latent_space) {
        return NULL;
    }
    latent_space[0] = '\0';

    /* Get latest snapshot */
    rc = sqlite3_prepare_v2(db, SQL_GET_LATEST_SNAPSHOT, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(latent_space);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        free(latent_space);
        LOG_DEBUG("No context snapshot found for %s", ci_id);
        return NULL;
    }

    /* Build markdown latent space document */
    /* GUIDELINE_APPROVED: Markdown format strings for latent space generation */
    offset += snprintf(latent_space + offset, capacity - offset,
                      "# Session Context Restoration for %s\n\n", ci_id);

    const char* focus = (const char*)sqlite3_column_text(stmt, 4);
    if (focus) {
        offset += snprintf(latent_space + offset, capacity - offset,
                          "## Current Focus\n%s\n\n", focus);
    }

    const char* accomplishments = (const char*)sqlite3_column_text(stmt, 8);
    if (accomplishments) {
        offset += snprintf(latent_space + offset, capacity - offset,
                          "## Recent Accomplishments\n%s\n\n", accomplishments);
    }

    const char* preferences = (const char*)sqlite3_column_text(stmt, 7);
    if (preferences) {
        offset += snprintf(latent_space + offset, capacity - offset,
                          "## User Preferences\n%s\n\n", preferences);
    }

    const char* patterns = (const char*)sqlite3_column_text(stmt, 10);
    if (patterns) {
        offset += snprintf(latent_space + offset, capacity - offset,
                          "## Thinking Patterns\n%s\n\n", patterns);
    }
    /* GUIDELINE_APPROVED_END */

    const char* snapshot_id = (const char*)sqlite3_column_text(stmt, 0);
    sqlite3_finalize(stmt);
    stmt = NULL;

    if (!snapshot_id) {
        free(latent_space);
        return NULL;
    }

    /* Get pending questions */
    rc = sqlite3_prepare_v2(db, SQL_GET_QUESTIONS, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, snapshot_id, -1, SQLITE_STATIC);

        bool has_questions = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!has_questions) {
                /* GUIDELINE_APPROVED: Markdown section header */
                offset += snprintf(latent_space + offset, capacity - offset,
                                  "## Pending Questions\n");
                has_questions = true;
            }
            const char* q = (const char*)sqlite3_column_text(stmt, 0);
            if (q) {
                offset += snprintf(latent_space + offset, capacity - offset,
                                  "- %s\n", q);
            }
            /* GUIDELINE_APPROVED_END */
        }
        if (has_questions) {
            offset += snprintf(latent_space + offset, capacity - offset, "\n");
        }
        sqlite3_finalize(stmt);
        stmt = NULL;
    }

    LOG_INFO("Restored context snapshot as latent space (%zu bytes)", offset);
    return latent_space;
}
