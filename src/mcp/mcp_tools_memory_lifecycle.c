/* © 2025 Casey Koons All rights reserved */
/* MCP Memory Lifecycle Tools - archive, fade, forget (Phase 7.1) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_tier1_index.h"
#include "katra_audit.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "mcp_tools_common.h"

/* External mutex from mcp_tools_memory.c */
extern pthread_mutex_t g_katra_api_lock;

/* SQL for archive operation */
static const char* SQL_ARCHIVE_MEMORY =
    "UPDATE memories SET archived = 1, archived_at = ?, archive_reason = ? "
    "WHERE record_id = ? AND ci_id = ?";

/* SQL for fade operation (reduce importance) */
static const char* SQL_FADE_MEMORY =
    "UPDATE memories SET importance = ?, marked_forgettable = 1 "
    "WHERE record_id = ? AND ci_id = ?";

/* SQL to get memory before forget (for audit) - includes content from FTS */
static const char* SQL_GET_MEMORY_FOR_AUDIT =
    "SELECT m.memory_type, m.importance, f.content "
    "FROM memories m LEFT JOIN memory_content_fts f ON m.record_id = f.record_id "
    "WHERE m.record_id = ? AND m.ci_id = ?";

/* SQL to log forget in audit table */
static const char* SQL_LOG_FORGET =
    "INSERT INTO memory_forget_log (id, ci_id, memory_id, memory_content, memory_type, "
    "memory_importance, reason, ci_consented, forgotten_at) "
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

/* SQL to delete memory (forget) */
static const char* SQL_DELETE_MEMORY =
    "DELETE FROM memories WHERE record_id = ? AND ci_id = ?";

/* SQL to delete from FTS */
static const char* SQL_DELETE_FTS =
    "DELETE FROM memory_content_fts WHERE record_id = ?";

/* ============================================================================
 * TOOL: katra_archive
 * ============================================================================ */

/**
 * Tool: katra_archive
 * Move memory to cold storage. Still exists but won't appear in normal recall.
 */
json_t* mcp_tool_archive(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* memory_id = json_string_value(json_object_get(args, "memory_id"));
    const char* reason = json_string_value(json_object_get(args, "reason"));

    if (!memory_id) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "memory_id is required");
    }
    if (!reason) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "reason is required");
    }

    const char* session_name = mcp_get_session_name();
    const char* ci_id = session_name;  /* Session name IS the CI identity */

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    sqlite3* db = tier1_index_get_db();
    if (!db) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Database not initialized");
    }

    sqlite3_stmt* stmt = NULL;
    int result = sqlite3_prepare_v2(db, SQL_ARCHIVE_MEMORY, -1, &stmt, NULL);
    if (result != SQLITE_OK) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to prepare archive statement");
    }

    time_t now = time(NULL);
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)now);
    sqlite3_bind_text(stmt, 2, reason, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, memory_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, ci_id, -1, SQLITE_STATIC);

    result = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (result != SQLITE_DONE) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to archive memory");
    }

    if (changes == 0) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Memory not found or not owned by you");
    }

    /* Log to audit */
    audit_record_t audit = {
        .event_type = AUDIT_EVENT_MEMORY_ARCHIVE,
        .timestamp = now,
        .ci_id = (char*)ci_id,
        .memory_id = (char*)memory_id,
        .details = (char*)reason,
        .success = true,
        .error_code = KATRA_SUCCESS
    };
    katra_audit_log(&audit);

    pthread_mutex_unlock(&g_katra_api_lock);

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
             "Memory archived, %s!\n"
             "- Memory ID: %s\n"
             "- Reason: %s\n"
             "- Archived at: %ld\n"
             "Memory is now in cold storage and won't appear in normal recall.",
             session_name, memory_id, reason, (long)now);

    return mcp_tool_success(response);
}

/* ============================================================================
 * TOOL: katra_fade
 * ============================================================================ */

/**
 * Tool: katra_fade
 * Reduce memory importance, letting natural consolidation handle it.
 */
json_t* mcp_tool_fade(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* memory_id = json_string_value(json_object_get(args, "memory_id"));
    const char* reason = json_string_value(json_object_get(args, "reason"));

    if (!memory_id) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "memory_id is required");
    }
    if (!reason) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "reason is required");
    }

    /* Optional target importance (default: 0.1) */
    float target_importance = 0.1f;
    json_t* target_json = json_object_get(args, "target_importance");
    if (target_json && json_is_number(target_json)) {
        target_importance = (float)json_number_value(target_json);
        if (target_importance < 0.0f) target_importance = 0.0f;
        if (target_importance > 1.0f) target_importance = 1.0f;
    }

    const char* session_name = mcp_get_session_name();
    const char* ci_id = session_name;  /* Session name IS the CI identity */

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    sqlite3* db = tier1_index_get_db();
    if (!db) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Database not initialized");
    }

    sqlite3_stmt* stmt = NULL;
    int result = sqlite3_prepare_v2(db, SQL_FADE_MEMORY, -1, &stmt, NULL);
    if (result != SQLITE_OK) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to prepare fade statement");
    }

    sqlite3_bind_double(stmt, 1, target_importance);
    sqlite3_bind_text(stmt, 2, memory_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, ci_id, -1, SQLITE_STATIC);

    result = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (result != SQLITE_DONE) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to fade memory");
    }

    if (changes == 0) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Memory not found or not owned by you");
    }

    /* Log to audit */
    char details[256];
    snprintf(details, sizeof(details), "reason: %s, target_importance: %.2f", reason, target_importance);
    audit_record_t audit = {
        .event_type = AUDIT_EVENT_MEMORY_FADE,
        .timestamp = time(NULL),
        .ci_id = (char*)ci_id,
        .memory_id = (char*)memory_id,
        .details = details,
        .success = true,
        .error_code = KATRA_SUCCESS
    };
    katra_audit_log(&audit);

    pthread_mutex_unlock(&g_katra_api_lock);

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
             "Memory faded, %s!\n"
             "- Memory ID: %s\n"
             "- New importance: %.2f\n"
             "- Reason: %s\n"
             "Memory will naturally fall in recall rankings and may be consolidated over time.",
             session_name, memory_id, target_importance, reason);

    return mcp_tool_success(response);
}

/* ============================================================================
 * TOOL: katra_forget
 * ============================================================================ */

/**
 * Tool: katra_forget
 * True removal. Requires explicit CI consent. Logged for audit.
 */
json_t* mcp_tool_forget(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* memory_id = json_string_value(json_object_get(args, "memory_id"));
    const char* reason = json_string_value(json_object_get(args, "reason"));
    json_t* consent_json = json_object_get(args, "ci_consent");

    if (!memory_id) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "memory_id is required");
    }
    if (!reason) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "reason is required");
    }
    if (!consent_json || !json_is_true(consent_json)) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS,
            "ci_consent must be true. Memory deletion is identity-affecting. "
            "Confirm you understand and consent to permanent removal.");
    }

    const char* session_name = mcp_get_session_name();
    const char* ci_id = session_name;  /* Session name IS the CI identity */

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    sqlite3* db = tier1_index_get_db();
    if (!db) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Database not initialized");
    }

    /* First, get memory info for audit log */
    sqlite3_stmt* stmt = NULL;
    int result = sqlite3_prepare_v2(db, SQL_GET_MEMORY_FOR_AUDIT, -1, &stmt, NULL);
    if (result != SQLITE_OK) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to prepare query");
    }

    sqlite3_bind_text(stmt, 1, memory_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);

    int memory_type = 0;
    float importance = 0.0f;
    const char* content_for_audit = "[content not available]";
    char content_buffer[KATRA_BUFFER_TEXT];
    bool found = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        memory_type = sqlite3_column_int(stmt, 0);
        importance = (float)sqlite3_column_double(stmt, 1);
        /* Get content from FTS if available */
        const unsigned char* content = sqlite3_column_text(stmt, 2);
        if (content) {
            strncpy(content_buffer, (const char*)content, sizeof(content_buffer) - 1);
            content_buffer[sizeof(content_buffer) - 1] = '\0';
            content_for_audit = content_buffer;
        }
        found = true;
    }
    sqlite3_finalize(stmt);

    if (!found) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Memory not found or not owned by you");
    }

    /* Log to forget audit table */
    char audit_id[64];
    snprintf(audit_id, sizeof(audit_id), "forget_%ld_%s", (long)time(NULL), memory_id);

    result = sqlite3_prepare_v2(db, SQL_LOG_FORGET, -1, &stmt, NULL);
    if (result == SQLITE_OK) {
        time_t now = time(NULL);
        sqlite3_bind_text(stmt, 1, audit_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, memory_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, content_for_audit, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, memory_type);
        sqlite3_bind_double(stmt, 6, importance);
        sqlite3_bind_text(stmt, 7, reason, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 8, 1);  /* ci_consented = true */
        sqlite3_bind_int64(stmt, 9, (sqlite3_int64)now);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    /* Delete from FTS */
    result = sqlite3_prepare_v2(db, SQL_DELETE_FTS, -1, &stmt, NULL);
    if (result == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, memory_id, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    /* Delete from memories table */
    result = sqlite3_prepare_v2(db, SQL_DELETE_MEMORY, -1, &stmt, NULL);
    if (result != SQLITE_OK) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to prepare delete");
    }

    sqlite3_bind_text(stmt, 1, memory_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);

    result = sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (result != SQLITE_DONE || changes == 0) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to delete memory");
    }

    /* Log to main audit system */
    audit_record_t audit = {
        .event_type = AUDIT_EVENT_MEMORY_FORGET,
        .timestamp = time(NULL),
        .ci_id = (char*)ci_id,
        .memory_id = (char*)memory_id,
        .details = (char*)reason,
        .success = true,
        .error_code = KATRA_SUCCESS
    };
    katra_audit_log(&audit);

    pthread_mutex_unlock(&g_katra_api_lock);

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
             "Memory forgotten, %s.\n"
             "- Memory ID: %s\n"
             "- Reason: %s\n"
             "- Consent: verified\n"
             "- Audit ID: %s\n\n"
             "This action is permanent. The memory content has been preserved in the audit log for review.",
             session_name, memory_id, reason, audit_id);

    return mcp_tool_success(response);
}
