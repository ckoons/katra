/* © 2025 Casey Koons All rights reserved */

/* Katra Daemon Insights - Storage, retrieval, acknowledgment, and history */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

#include "katra_daemon.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_breathing.h"
#include "katra_core_common.h"

/* External daemon database handle (from katra_daemon.c) */
extern sqlite3* daemon_db;
extern bool daemon_initialized;

/* Insight type names */
static const char* INSIGHT_TYPE_NAMES[] = {
    "pattern",
    "association",
    "theme",
    "temporal",
    "emotional"
};

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

const char* katra_insight_type_name(insight_type_t type) {
    if (type >= 0 && type <= INSIGHT_EMOTIONAL) {
        return INSIGHT_TYPE_NAMES[type];
    }
    return "unknown";
}

void katra_daemon_generate_insight_id(char* id_out, size_t size) {
    if (!id_out || size == 0) return;

    time_t now = time(NULL);
    unsigned int rand_part = (unsigned int)rand();
    snprintf(id_out, size, "ins_%ld_%u", (long)now, rand_part % DAEMON_ID_MODULO);
}

/* ============================================================================
 * INSIGHT STORAGE
 * ============================================================================ */

int katra_daemon_store_insight(const char* ci_id, const daemon_insight_t* insight) {
    if (!ci_id || !insight) return E_INPUT_NULL;
    if (!daemon_initialized) return E_INVALID_STATE;

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql =
        "INSERT INTO daemon_insights (id, ci_id, type, content, source_ids, "
        "confidence, generated_at, acknowledged) VALUES (?, ?, ?, ?, ?, ?, ?, 0)";

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(daemon_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return E_SYSTEM_FILE;

    sqlite3_bind_text(stmt, 1, insight->id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, insight->type);
    sqlite3_bind_text(stmt, 4, insight->content, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, insight->source_ids, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 6, insight->confidence);
    sqlite3_bind_int64(stmt, 7, insight->generated_at);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return E_SYSTEM_FILE;

    /* Also store as a memory with daemon tag */
    char memory_content[KATRA_BUFFER_TEXT];
    snprintf(memory_content, sizeof(memory_content),
             "[%s] %s", DAEMON_TAG_INSIGHT, insight->content);

    /* Use breathing layer to store as memory */
    int result = learn(ci_id, memory_content);
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Failed to store insight as memory: %d", result);
    }

    return KATRA_SUCCESS;
}

/* ============================================================================
 * INSIGHT RETRIEVAL
 * ============================================================================ */

int katra_daemon_get_pending_insights(const char* ci_id,
                                       daemon_insight_t** insights, size_t* count) {
    if (!ci_id || !insights || !count) return E_INPUT_NULL;
    if (!daemon_initialized) {
        int init_result = katra_daemon_init();
        if (init_result != KATRA_SUCCESS) return init_result;
    }

    *insights = NULL;
    *count = 0;

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql =
        "SELECT id, type, content, source_ids, confidence, generated_at "
        "FROM daemon_insights WHERE ci_id = ? AND acknowledged = 0 "
        "ORDER BY generated_at DESC LIMIT " KATRA_STRINGIFY(DAEMON_PENDING_INSIGHTS_LIMIT);

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(daemon_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return E_SYSTEM_FILE;

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) n++;
    sqlite3_reset(stmt);

    if (n == 0) {
        sqlite3_finalize(stmt);
        return KATRA_SUCCESS;
    }

    daemon_insight_t* arr = calloc(n, sizeof(daemon_insight_t));
    if (!arr) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        const char* text;

        text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) SAFE_STRNCPY(arr[i].id, text);

        arr[i].type = sqlite3_column_int(stmt, 1);

        text = (const char*)sqlite3_column_text(stmt, 2);
        if (text) SAFE_STRNCPY(arr[i].content, text);

        text = (const char*)sqlite3_column_text(stmt, 3);
        if (text) arr[i].source_ids = strdup(text);

        arr[i].confidence = (float)sqlite3_column_double(stmt, 4);
        arr[i].generated_at = sqlite3_column_int64(stmt, 5);
        arr[i].acknowledged = false;

        SAFE_STRNCPY(arr[i].ci_id, ci_id);
        i++;
    }

    sqlite3_finalize(stmt);

    *insights = arr;
    *count = i;
    return KATRA_SUCCESS;
}

int katra_daemon_acknowledge_insight(const char* insight_id) {
    if (!insight_id) return E_INPUT_NULL;
    if (!daemon_initialized) return E_INVALID_STATE;

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql = "UPDATE daemon_insights SET acknowledged = 1 WHERE id = ?";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(daemon_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return E_SYSTEM_FILE;

    sqlite3_bind_text(stmt, 1, insight_id, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? KATRA_SUCCESS : E_SYSTEM_FILE;
}

/* ============================================================================
 * INSIGHT FORMATTING
 * ============================================================================ */

int katra_daemon_format_sunrise_insights(const daemon_insight_t* insights, size_t count,
                                          char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return E_INPUT_NULL;

    if (!insights || count == 0) {
        buffer[0] = '\0';
        return KATRA_SUCCESS;
    }

    size_t offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "While you rested, I noticed:\n\n");

    for (size_t i = 0; i < count && offset < buffer_size - DAEMON_RESPONSE_RESERVE; i++) {
        const char* type_name = katra_insight_type_name(insights[i].type);
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "- %s: %s\n", type_name, insights[i].content);
    }

    return KATRA_SUCCESS;
}

void katra_daemon_free_insights(daemon_insight_t* insights, size_t count) {
    if (!insights) return;
    for (size_t i = 0; i < count; i++) {
        free(insights[i].source_ids);
    }
    free(insights);
}

/* ============================================================================
 * DAEMON HISTORY
 * ============================================================================ */

int katra_daemon_get_history(const char* ci_id, daemon_result_t** history, size_t* count) {
    if (!ci_id || !history || !count) return E_INPUT_NULL;
    if (!daemon_initialized) {
        int init_result = katra_daemon_init();
        if (init_result != KATRA_SUCCESS) return init_result;
    }

    *history = NULL;
    *count = 0;

    /* GUIDELINE_APPROVED: SQL query string */
    const char* sql =
        "SELECT run_start, run_end, memories_processed, patterns_found, "
        "associations_formed, themes_detected, insights_generated, error_code "
        "FROM daemon_runs WHERE ci_id = ? ORDER BY run_start DESC LIMIT " KATRA_STRINGIFY(DAEMON_HISTORY_LIMIT);

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(daemon_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return E_SYSTEM_FILE;

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) n++;
    sqlite3_reset(stmt);

    if (n == 0) {
        sqlite3_finalize(stmt);
        return KATRA_SUCCESS;
    }

    daemon_result_t* arr = calloc(n, sizeof(daemon_result_t));
    if (!arr) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        arr[i].run_start = sqlite3_column_int64(stmt, 0);
        arr[i].run_end = sqlite3_column_int64(stmt, 1);
        arr[i].memories_processed = sqlite3_column_int(stmt, 2);
        arr[i].patterns_found = sqlite3_column_int(stmt, 3);
        arr[i].associations_formed = sqlite3_column_int(stmt, 4);
        arr[i].themes_detected = sqlite3_column_int(stmt, 5);
        arr[i].insights_generated = sqlite3_column_int(stmt, 6);
        arr[i].error_code = sqlite3_column_int(stmt, 7);
        i++;
    }

    sqlite3_finalize(stmt);

    *history = arr;
    *count = i;
    return KATRA_SUCCESS;
}

void katra_daemon_free_history(daemon_result_t* history, size_t count) {
    (void)count;
    free(history);
}
