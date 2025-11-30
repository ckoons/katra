/* © 2025 Casey Koons All rights reserved */

/* Katra Whiteboard Loaders - Load related data from database */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_whiteboard.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_core_common.h"

/* External database handle from katra_whiteboard.c */
extern sqlite3* wb_db;
extern bool wb_initialized;

/* ============================================================================
 * QUESTION LOADING
 * ============================================================================ */

int katra_whiteboard_load_questions(const char* wb_id, wb_question_t** questions, size_t* count) {
    if (!wb_id || !questions || !count) {
        return E_INPUT_NULL;
    }

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "SELECT id, author, question, answered, answer, created_at "
                      "FROM whiteboard_questions WHERE whiteboard_id = ? ORDER BY created_at";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *questions = NULL;
        *count = 0;
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, wb_id, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) n++;
    sqlite3_reset(stmt);

    if (n == 0) {
        sqlite3_finalize(stmt);
        *questions = NULL;
        *count = 0;
        return KATRA_SUCCESS;
    }

    wb_question_t* q = calloc(n, sizeof(wb_question_t));
    if (!q) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        const char* text;

        text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) SAFE_STRNCPY(q[i].id, text);

        text = (const char*)sqlite3_column_text(stmt, 1);
        if (text) SAFE_STRNCPY(q[i].author, text);

        text = (const char*)sqlite3_column_text(stmt, 2);
        if (text) SAFE_STRNCPY(q[i].text, text);

        q[i].answered = sqlite3_column_int(stmt, 3) != 0;

        text = (const char*)sqlite3_column_text(stmt, 4);
        if (text) SAFE_STRNCPY(q[i].answer, text);

        q[i].created_at = sqlite3_column_int64(stmt, 5);
        i++;
    }

    sqlite3_finalize(stmt);
    *questions = q;
    *count = n;
    return KATRA_SUCCESS;
}

/* ============================================================================
 * APPROACH LOADING
 * ============================================================================ */

int katra_whiteboard_load_approaches(const char* wb_id, wb_approach_t** approaches, size_t* count) {
    if (!wb_id || !approaches || !count) {
        return E_INPUT_NULL;
    }

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "SELECT id, author, title, description, pros_json, cons_json, created_at "
                      "FROM whiteboard_approaches WHERE whiteboard_id = ? ORDER BY created_at";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *approaches = NULL;
        *count = 0;
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, wb_id, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) n++;
    sqlite3_reset(stmt);

    if (n == 0) {
        sqlite3_finalize(stmt);
        *approaches = NULL;
        *count = 0;
        return KATRA_SUCCESS;
    }

    wb_approach_t* a = calloc(n, sizeof(wb_approach_t));
    if (!a) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        const char* text;

        text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) SAFE_STRNCPY(a[i].id, text);

        text = (const char*)sqlite3_column_text(stmt, 1);
        if (text) SAFE_STRNCPY(a[i].author, text);

        text = (const char*)sqlite3_column_text(stmt, 2);
        if (text) SAFE_STRNCPY(a[i].title, text);

        text = (const char*)sqlite3_column_text(stmt, 3);
        if (text) SAFE_STRNCPY(a[i].description, text);

        /* TODO: Parse pros_json and cons_json */

        a[i].created_at = sqlite3_column_int64(stmt, 6);
        i++;
    }

    sqlite3_finalize(stmt);
    *approaches = a;
    *count = n;
    return KATRA_SUCCESS;
}

/* ============================================================================
 * VOTE LOADING
 * ============================================================================ */

int katra_whiteboard_load_votes(const char* wb_id, wb_vote_t** votes, size_t* count) {
    if (!wb_id || !votes || !count) {
        return E_INPUT_NULL;
    }

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "SELECT id, approach_id, voter, position, reasoning, created_at "
                      "FROM whiteboard_votes WHERE whiteboard_id = ? ORDER BY created_at";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *votes = NULL;
        *count = 0;
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, wb_id, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) n++;
    sqlite3_reset(stmt);

    if (n == 0) {
        sqlite3_finalize(stmt);
        *votes = NULL;
        *count = 0;
        return KATRA_SUCCESS;
    }

    wb_vote_t* v = calloc(n, sizeof(wb_vote_t));
    if (!v) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        const char* text;

        text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) SAFE_STRNCPY(v[i].id, text);

        text = (const char*)sqlite3_column_text(stmt, 1);
        if (text) SAFE_STRNCPY(v[i].approach_id, text);

        text = (const char*)sqlite3_column_text(stmt, 2);
        if (text) SAFE_STRNCPY(v[i].voter, text);

        v[i].position = sqlite3_column_int(stmt, 3);

        text = (const char*)sqlite3_column_text(stmt, 4);
        if (text) SAFE_STRNCPY(v[i].reasoning, text);

        v[i].created_at = sqlite3_column_int64(stmt, 5);
        i++;
    }

    sqlite3_finalize(stmt);
    *votes = v;
    *count = n;
    return KATRA_SUCCESS;
}

/* ============================================================================
 * WHITEBOARD LIST
 * ============================================================================ */

int katra_whiteboard_list(const char* project, wb_summary_t** summaries_out, size_t* count_out) {
    if (!summaries_out || !count_out) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        return E_INVALID_STATE;
    }

    const char* sql;
    sqlite3_stmt* stmt = NULL;

    if (project) {
        /* GUIDELINE_APPROVED: SQL query string */
        sql = "SELECT id, project, problem, status, created_at, design_approved "
              "FROM whiteboards WHERE project = ? ORDER BY created_at DESC";
    } else {
        /* GUIDELINE_APPROVED: SQL query string */
        sql = "SELECT id, project, problem, status, created_at, design_approved "
              "FROM whiteboards ORDER BY created_at DESC";
    }

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    if (project) {
        sqlite3_bind_text(stmt, 1, project, -1, SQLITE_STATIC);
    }

    /* Count results first */
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    sqlite3_reset(stmt);

    if (count == 0) {
        sqlite3_finalize(stmt);
        *summaries_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    /* Allocate array */
    wb_summary_t* summaries = calloc(count, sizeof(wb_summary_t));
    if (!summaries) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    /* Populate */
    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        const char* text;

        text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) SAFE_STRNCPY(summaries[i].id, text);

        text = (const char*)sqlite3_column_text(stmt, 1);
        if (text) SAFE_STRNCPY(summaries[i].project, text);

        text = (const char*)sqlite3_column_text(stmt, 2);
        if (text) {
            strncpy(summaries[i].problem, text, sizeof(summaries[i].problem) - 1);
            summaries[i].problem[sizeof(summaries[i].problem) - 1] = '\0';
        }

        summaries[i].status = sqlite3_column_int(stmt, 3);
        summaries[i].created_at = sqlite3_column_int64(stmt, 4);
        summaries[i].design_approved = sqlite3_column_int(stmt, 5) != 0;

        i++;
    }

    sqlite3_finalize(stmt);
    *summaries_out = summaries;
    *count_out = count;
    return KATRA_SUCCESS;
}

/* ============================================================================
 * MEMORY FREE FUNCTIONS
 * ============================================================================ */

void katra_whiteboard_free(whiteboard_t* wb) {
    if (!wb) return;

    /* Free questions */
    if (wb->questions) {
        free(wb->questions);
    }

    /* Free approaches */
    if (wb->approaches) {
        for (size_t i = 0; i < wb->approach_count; i++) {
            if (wb->approaches[i].pros) {
                for (size_t j = 0; j < wb->approaches[i].pros_count; j++) {
                    free(wb->approaches[i].pros[j]);
                }
                free(wb->approaches[i].pros);
            }
            if (wb->approaches[i].cons) {
                for (size_t j = 0; j < wb->approaches[i].cons_count; j++) {
                    free(wb->approaches[i].cons[j]);
                }
                free(wb->approaches[i].cons);
            }
            if (wb->approaches[i].supporters) {
                for (size_t j = 0; j < wb->approaches[i].supporter_count; j++) {
                    free(wb->approaches[i].supporters[j]);
                }
                free(wb->approaches[i].supporters);
            }
        }
        free(wb->approaches);
    }

    /* Free votes */
    if (wb->votes) {
        free(wb->votes);
    }

    /* Free goal criteria */
    if (wb->goal.criteria) {
        for (size_t i = 0; i < wb->goal.criteria_count; i++) {
            free(wb->goal.criteria[i]);
        }
        free(wb->goal.criteria);
    }

    /* Free scope */
    if (wb->scope.included) {
        for (size_t i = 0; i < wb->scope.included_count; i++) {
            free(wb->scope.included[i]);
        }
        free(wb->scope.included);
    }
    if (wb->scope.excluded) {
        for (size_t i = 0; i < wb->scope.excluded_count; i++) {
            free(wb->scope.excluded[i]);
        }
        free(wb->scope.excluded);
    }

    /* Free design content */
    free(wb->design.content);
    if (wb->design.reviewers) {
        for (size_t i = 0; i < wb->design.reviewer_count; i++) {
            free(wb->design.reviewers[i]);
        }
        free(wb->design.reviewers);
    }

    free(wb);
}

void katra_whiteboard_summaries_free(wb_summary_t* summaries, size_t count) {
    (void)count;
    free(summaries);
}
