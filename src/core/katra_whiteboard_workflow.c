/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_whiteboard.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_psyche_common.h"
#include "katra_core_common.h"

/* External database handle from katra_whiteboard.c */
extern sqlite3* wb_db;
extern bool wb_initialized;

/* ============================================================================
 * GOAL SETTING
 * ============================================================================ */

int katra_whiteboard_set_goal(const char* whiteboard_id,
                              const char** criteria, size_t count) {
    if (!whiteboard_id || !criteria || count == 0) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        return E_INVALID_STATE;
    }

    /* Get current whiteboard */
    whiteboard_t* wb = NULL;
    int result = katra_whiteboard_get(whiteboard_id, &wb);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check status - must be draft */
    if (wb->status != WB_STATUS_DRAFT) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_set_goal",
                          "Can only set goal in draft status");
        return E_INVALID_STATE;
    }

    /* Build JSON array of criteria */
    char goal_json[KATRA_BUFFER_TEXT];
    size_t offset = 0;
    offset += snprintf(goal_json + offset, sizeof(goal_json) - offset, "[");
    for (size_t i = 0; i < count && offset < sizeof(goal_json) - JSON_ARRAY_CLOSE_RESERVE; i++) {
        if (i > 0) offset += snprintf(goal_json + offset, sizeof(goal_json) - offset, ",");
        offset += snprintf(goal_json + offset, sizeof(goal_json) - offset, "\"%s\"", criteria[i]);
    }
    snprintf(goal_json + offset, sizeof(goal_json) - offset, "]");

    /* Update database */
    const char* sql = "UPDATE whiteboards SET goal_json = ?, status = ? WHERE id = ?";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        katra_whiteboard_free(wb);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, goal_json, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, WB_STATUS_QUESTIONING);
    sqlite3_bind_text(stmt, 3, whiteboard_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    katra_whiteboard_free(wb);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: goal set, now in questioning phase", whiteboard_id);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * QUESTIONING PHASE
 * ============================================================================ */

int katra_whiteboard_add_question(const char* whiteboard_id,
                                  const char* author, const char* question) {
    if (!whiteboard_id || !author || !question) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        return E_INVALID_STATE;
    }

    /* Get current whiteboard */
    whiteboard_t* wb = NULL;
    int result = katra_whiteboard_get(whiteboard_id, &wb);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check status - must be questioning */
    if (wb->status != WB_STATUS_QUESTIONING) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_add_question",
                          "Can only add questions in questioning status");
        return E_INVALID_STATE;
    }

    katra_whiteboard_free(wb);

    /* Generate question ID */
    char q_id[KATRA_BUFFER_SMALL];
    katra_whiteboard_generate_id("q", q_id, sizeof(q_id));

    /* Insert question */
    const char* sql = "INSERT INTO whiteboard_questions "
                      "(id, whiteboard_id, author, question, answered, created_at) "
                      "VALUES (?, ?, ?, ?, 0, ?)";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, q_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, whiteboard_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, author, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, question, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: question added by %s", whiteboard_id, author);
    return KATRA_SUCCESS;
}

int katra_whiteboard_answer_question(const char* whiteboard_id,
                                     const char* question_id, const char* answer) {
    if (!whiteboard_id || !question_id || !answer) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        return E_INVALID_STATE;
    }

    const char* sql = "UPDATE whiteboard_questions SET answered = 1, answer = ? "
                      "WHERE id = ? AND whiteboard_id = ?";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, answer, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, question_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, whiteboard_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: question %s answered", whiteboard_id, question_id);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * SCOPING PHASE
 * ============================================================================ */

int katra_whiteboard_set_scope(const char* whiteboard_id,
                               const char** included, size_t inc_count,
                               const char** excluded, size_t exc_count) {
    if (!whiteboard_id) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        return E_INVALID_STATE;
    }

    /* Get current whiteboard */
    whiteboard_t* wb = NULL;
    int result = katra_whiteboard_get(whiteboard_id, &wb);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check status - must be questioning */
    if (wb->status != WB_STATUS_QUESTIONING) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_set_scope",
                          "Can only set scope from questioning status");
        return E_INVALID_STATE;
    }

    katra_whiteboard_free(wb);

    /* Build scope JSON */
    char scope_json[KATRA_BUFFER_TEXT];
    size_t offset = 0;
    offset += snprintf(scope_json + offset, sizeof(scope_json) - offset, "{\"included\":[");

    for (size_t i = 0; i < inc_count && offset < sizeof(scope_json) - RESPONSE_BUFFER_SAFETY_MARGIN_SMALL; i++) {
        if (i > 0) offset += snprintf(scope_json + offset, sizeof(scope_json) - offset, ",");
        offset += snprintf(scope_json + offset, sizeof(scope_json) - offset, "\"%s\"",
                          included ? included[i] : "");
    }

    offset += snprintf(scope_json + offset, sizeof(scope_json) - offset, "],\"excluded\":[");

    for (size_t i = 0; i < exc_count && offset < sizeof(scope_json) - JSON_ARRAY_ELEMENT_RESERVE; i++) {
        if (i > 0) offset += snprintf(scope_json + offset, sizeof(scope_json) - offset, ",");
        offset += snprintf(scope_json + offset, sizeof(scope_json) - offset, "\"%s\"",
                          excluded ? excluded[i] : "");
    }

    snprintf(scope_json + offset, sizeof(scope_json) - offset, "]}");

    /* Update database - transition to proposing */
    const char* sql = "UPDATE whiteboards SET scope_json = ?, status = ? WHERE id = ?";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, scope_json, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, WB_STATUS_PROPOSING);
    sqlite3_bind_text(stmt, 3, whiteboard_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: scope set, now in proposing phase", whiteboard_id);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * PROPOSING PHASE
 * ============================================================================ */

int katra_whiteboard_propose(const char* whiteboard_id,
                             const char* author,
                             const char* title, const char* description,
                             const char** pros, size_t pros_count,
                             const char** cons, size_t cons_count,
                             char* approach_id_out) {
    if (!whiteboard_id || !author || !title || !description) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        return E_INVALID_STATE;
    }

    /* Get current whiteboard */
    whiteboard_t* wb = NULL;
    int result = katra_whiteboard_get(whiteboard_id, &wb);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check status - must be proposing */
    if (wb->status != WB_STATUS_PROPOSING) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_propose",
                          "Can only propose in proposing status");
        return E_INVALID_STATE;
    }

    katra_whiteboard_free(wb);

    /* Generate approach ID */
    char a_id[KATRA_BUFFER_SMALL];
    katra_whiteboard_generate_id("approach", a_id, sizeof(a_id));

    /* Build pros/cons JSON */
    char pros_json[KATRA_BUFFER_MEDIUM] = "[]";
    char cons_json[KATRA_BUFFER_MEDIUM] = "[]";

    if (pros && pros_count > 0) {
        size_t offset = 0;
        offset += snprintf(pros_json + offset, sizeof(pros_json) - offset, "[");
        for (size_t i = 0; i < pros_count && offset < sizeof(pros_json) - JSON_ARRAY_CLOSE_RESERVE; i++) {
            if (i > 0) offset += snprintf(pros_json + offset, sizeof(pros_json) - offset, ",");
            offset += snprintf(pros_json + offset, sizeof(pros_json) - offset, "\"%s\"", pros[i]);
        }
        snprintf(pros_json + offset, sizeof(pros_json) - offset, "]");
    }

    if (cons && cons_count > 0) {
        size_t offset = 0;
        offset += snprintf(cons_json + offset, sizeof(cons_json) - offset, "[");
        for (size_t i = 0; i < cons_count && offset < sizeof(cons_json) - JSON_ARRAY_CLOSE_RESERVE; i++) {
            if (i > 0) offset += snprintf(cons_json + offset, sizeof(cons_json) - offset, ",");
            offset += snprintf(cons_json + offset, sizeof(cons_json) - offset, "\"%s\"", cons[i]);
        }
        snprintf(cons_json + offset, sizeof(cons_json) - offset, "]");
    }

    /* Insert approach */
    const char* sql = "INSERT INTO whiteboard_approaches "
                      "(id, whiteboard_id, author, title, description, pros_json, cons_json, created_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, a_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, whiteboard_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, author, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, pros_json, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, cons_json, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 8, time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    if (approach_id_out) {
        strncpy(approach_id_out, a_id, KATRA_BUFFER_SMALL);
    }

    LOG_INFO("Whiteboard %s: approach '%s' proposed by %s", whiteboard_id, title, author);
    return KATRA_SUCCESS;
}

int katra_whiteboard_support(const char* whiteboard_id,
                             const char* approach_id, const char* supporter) {
    if (!whiteboard_id || !approach_id || !supporter) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        return E_INVALID_STATE;
    }

    const char* sql = "INSERT OR IGNORE INTO whiteboard_supporters "
                      "(whiteboard_id, approach_id, supporter, created_at) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, whiteboard_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, approach_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, supporter, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: %s supports approach %s", whiteboard_id, supporter, approach_id);
    return KATRA_SUCCESS;
}

/* Voting, designing, approval, regression, archive are in katra_whiteboard_phases.c */
