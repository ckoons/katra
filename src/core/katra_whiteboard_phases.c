/* © 2025 Casey Koons All rights reserved */

/* Whiteboard Phases - Voting, Designing, Approval, Regression, Archive */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

#include "katra_whiteboard.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_core_common.h"

/* External database handle from katra_whiteboard.c */
extern sqlite3* wb_db;
extern bool wb_initialized;

/* Forward declaration for internal helper */
static int wb_update_status(const char* wb_id, whiteboard_status_t status);

/* ============================================================================
 * VOTING PHASE
 * ============================================================================ */

int katra_whiteboard_call_votes(const char* whiteboard_id) {
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

    /* Check status - must be proposing */
    if (wb->status != WB_STATUS_PROPOSING) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_call_votes",
                          "Can only call votes from proposing status");
        return E_INVALID_STATE;
    }

    /* Must have at least one approach */
    if (wb->approach_count == 0) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_call_votes",
                          "Must have at least one approach before voting");
        return E_INVALID_STATE;
    }

    katra_whiteboard_free(wb);

    /* Transition to voting */
    result = wb_update_status(whiteboard_id, WB_STATUS_VOTING);
    if (result == KATRA_SUCCESS) {
        LOG_INFO("Whiteboard %s: voting phase started", whiteboard_id);
    }

    return result;
}

int katra_whiteboard_vote(const char* whiteboard_id,
                          const char* approach_id,
                          const char* voter,
                          vote_position_t position,
                          const char* reasoning) {
    if (!whiteboard_id || !approach_id || !voter || !reasoning) {
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

    /* Check status - must be voting */
    if (wb->status != WB_STATUS_VOTING) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_vote",
                          "Can only vote in voting status");
        return E_INVALID_STATE;
    }

    katra_whiteboard_free(wb);

    /* Generate vote ID */
    char v_id[WB_ID_SIZE];
    katra_whiteboard_generate_id("vote", v_id, sizeof(v_id));

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "INSERT OR REPLACE INTO whiteboard_votes "
                      "(id, whiteboard_id, approach_id, voter, position, reasoning, created_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, v_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, whiteboard_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, approach_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, voter, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, position);
    sqlite3_bind_text(stmt, 6, reasoning, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 7, time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: %s voted %s on approach %s",
             whiteboard_id, voter, katra_vote_position_name(position), approach_id);
    return KATRA_SUCCESS;
}

int katra_whiteboard_decide(const char* whiteboard_id,
                            const char* approach_id,
                            const char* decided_by,
                            const char* notes) {
    if (!whiteboard_id || !approach_id || !decided_by) {
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

    /* Check status - must be voting */
    if (wb->status != WB_STATUS_VOTING) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_decide",
                          "Can only decide from voting status");
        return E_INVALID_STATE;
    }

    katra_whiteboard_free(wb);

    /* Build decision JSON */
    char decision_json[KATRA_BUFFER_MEDIUM];
    snprintf(decision_json, sizeof(decision_json),
             "{\"selected_approach\":\"%s\",\"decided_by\":\"%s\",\"decided_at\":%ld,\"notes\":\"%s\"}",
             approach_id, decided_by, (long)time(NULL), notes ? notes : "");

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "UPDATE whiteboards SET decision_json = ?, status = ? WHERE id = ?";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, decision_json, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, WB_STATUS_DESIGNING);
    sqlite3_bind_text(stmt, 3, whiteboard_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: decision made by %s, approach %s selected",
             whiteboard_id, decided_by, approach_id);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * DESIGNING PHASE
 * ============================================================================ */

int katra_whiteboard_assign_design(const char* whiteboard_id, const char* ci_id) {
    if (!whiteboard_id || !ci_id) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        return E_INVALID_STATE;
    }

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "UPDATE whiteboards SET design_author = ? WHERE id = ?";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, whiteboard_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: %s assigned as design author", whiteboard_id, ci_id);
    return KATRA_SUCCESS;
}

int katra_whiteboard_submit_design(const char* whiteboard_id,
                                   const char* author, const char* content) {
    if (!whiteboard_id || !author || !content) {
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

    /* Check status - must be designing */
    if (wb->status != WB_STATUS_DESIGNING) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_submit_design",
                          "Can only submit design in designing status");
        return E_INVALID_STATE;
    }

    /* Check author matches assigned */
    if (wb->design.author[0] != '\0' && strcmp(wb->design.author, author) != 0) {
        katra_whiteboard_free(wb);
        katra_report_error(E_CONSENT_DENIED, "katra_whiteboard_submit_design",
                          "Only assigned author can submit design");
        return E_CONSENT_DENIED;
    }

    katra_whiteboard_free(wb);

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "UPDATE whiteboards SET design_content = ?, design_author = ? WHERE id = ?";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, content, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, author, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, whiteboard_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: design submitted by %s", whiteboard_id, author);
    return KATRA_SUCCESS;
}

int katra_whiteboard_review(const char* whiteboard_id,
                            const char* reviewer, const char* comment) {
    if (!whiteboard_id || !reviewer || !comment) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        return E_INVALID_STATE;
    }

    /* Generate review ID */
    char r_id[WB_ID_SIZE];
    katra_whiteboard_generate_id("review", r_id, sizeof(r_id));

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "INSERT INTO whiteboard_reviews "
                      "(id, whiteboard_id, reviewer, comment, created_at) "
                      "VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, r_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, whiteboard_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, reviewer, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, comment, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: review comment added by %s", whiteboard_id, reviewer);
    return KATRA_SUCCESS;
}

int katra_whiteboard_approve(const char* whiteboard_id, const char* approved_by) {
    if (!whiteboard_id || !approved_by) {
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

    /* Check status - must be designing */
    if (wb->status != WB_STATUS_DESIGNING) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_approve",
                          "Can only approve from designing status");
        return E_INVALID_STATE;
    }

    /* Must have design content */
    if (!wb->design.content || strlen(wb->design.content) == 0) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_approve",
                          "No design content to approve");
        return E_INVALID_STATE;
    }

    katra_whiteboard_free(wb);

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "UPDATE whiteboards SET design_approved = 1, "
                      "design_approved_by = ?, design_approved_at = ?, status = ? WHERE id = ?";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, approved_by, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, time(NULL));
    sqlite3_bind_int(stmt, 3, WB_STATUS_APPROVED);
    sqlite3_bind_text(stmt, 4, whiteboard_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: design approved by %s", whiteboard_id, approved_by);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * REGRESSION / RECONSIDERATION
 * ============================================================================ */

int katra_whiteboard_request_reconsider(const char* whiteboard_id,
                                        const char* requested_by,
                                        whiteboard_status_t target_status,
                                        const char* reason) {
    if (!whiteboard_id || !requested_by || !reason) {
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

    /* Check if regression is valid */
    if (!katra_whiteboard_can_transition(wb->status, target_status)) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_request_reconsider",
                          "Invalid regression target");
        return E_INVALID_STATE;
    }

    whiteboard_status_t from_status = wb->status;
    katra_whiteboard_free(wb);

    /* Generate regression ID */
    char reg_id[WB_ID_SIZE];
    katra_whiteboard_generate_id("reg", reg_id, sizeof(reg_id));

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "INSERT INTO whiteboard_regressions "
                      "(id, whiteboard_id, from_status, to_status, requested_by, reason, created_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, reg_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, whiteboard_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, from_status);
    sqlite3_bind_int(stmt, 4, target_status);
    sqlite3_bind_text(stmt, 5, requested_by, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, reason, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 7, time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Whiteboard %s: regression requested by %s to %s",
             whiteboard_id, requested_by, katra_whiteboard_status_name(target_status));
    return KATRA_SUCCESS;
}

int katra_whiteboard_approve_regression(const char* whiteboard_id,
                                        const char* approved_by) {
    if (!whiteboard_id || !approved_by) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        return E_INVALID_STATE;
    }

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "SELECT id, to_status FROM whiteboard_regressions "
                      "WHERE whiteboard_id = ? AND approved_by IS NULL "
                      "ORDER BY created_at DESC LIMIT 1";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, whiteboard_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        katra_report_error(E_NOT_FOUND, "katra_whiteboard_approve_regression",
                          "No pending regression request");
        return E_NOT_FOUND;
    }

    const char* reg_id = (const char*)sqlite3_column_text(stmt, 0);
    whiteboard_status_t target = sqlite3_column_int(stmt, 1);

    char reg_id_copy[WB_ID_SIZE];
    SAFE_STRNCPY(reg_id_copy, reg_id);
    sqlite3_finalize(stmt);

    /* GUIDELINE_APPROVED: SQL query string */
    sql = "UPDATE whiteboard_regressions SET approved_by = ?, approved_at = ? WHERE id = ?";
    rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, approved_by, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, time(NULL));
    sqlite3_bind_text(stmt, 3, reg_id_copy, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    /* Update whiteboard status */
    int result = wb_update_status(whiteboard_id, target);
    if (result == KATRA_SUCCESS) {
        LOG_INFO("Whiteboard %s: regression approved by %s to %s",
                 whiteboard_id, approved_by, katra_whiteboard_status_name(target));
    }

    return result;
}

/* ============================================================================
 * ARCHIVE
 * ============================================================================ */

int katra_whiteboard_archive(const char* whiteboard_id) {
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

    /* Check status - must be approved */
    if (wb->status != WB_STATUS_APPROVED) {
        katra_whiteboard_free(wb);
        katra_report_error(E_INVALID_STATE, "katra_whiteboard_archive",
                          "Can only archive from approved status");
        return E_INVALID_STATE;
    }

    katra_whiteboard_free(wb);

    result = wb_update_status(whiteboard_id, WB_STATUS_ARCHIVED);
    if (result == KATRA_SUCCESS) {
        LOG_INFO("Whiteboard %s: archived", whiteboard_id);
    }

    return result;
}

/* ============================================================================
 * INTERNAL HELPER
 * ============================================================================ */

static int wb_update_status(const char* wb_id, whiteboard_status_t status) {
    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "UPDATE whiteboards SET status = ? WHERE id = ?";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_text(stmt, 2, wb_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    return KATRA_SUCCESS;
}
