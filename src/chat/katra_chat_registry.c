/* © 2025 Casey Koons All rights reserved */

/*
 * katra_chat_registry.c - Meeting room lifecycle and CI registry
 *
 * Database initialization, cleanup, CI registration, and message TTL management.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <pthread.h>

/* Project includes */
#include "katra_chat_internal.h"
#include "katra_meeting.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_path_utils.h"

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

sqlite3* g_chat_db = NULL;
bool g_chat_initialized = false;
pthread_mutex_t g_chat_lock = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * SQL SCHEMA
 * ============================================================================ */

/* Global broadcast history (2-hour TTL) */
const char* CHAT_SCHEMA_MESSAGES =
    "CREATE TABLE IF NOT EXISTS katra_messages ("
    "  message_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  sender_ci_id TEXT NOT NULL,"
    "  sender_name TEXT NOT NULL,"
    "  message TEXT NOT NULL,"
    "  timestamp INTEGER NOT NULL,"
    "  created_at INTEGER DEFAULT (strftime('%s', 'now'))"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_messages_timestamp "
    "  ON katra_messages(timestamp);";

/* Per-CI personal queues (self-contained) */
const char* CHAT_SCHEMA_QUEUES =
    "CREATE TABLE IF NOT EXISTS katra_queues ("
    "  queue_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  recipient_ci_id TEXT NOT NULL,"
    "  sender_ci_id TEXT NOT NULL,"
    "  sender_name TEXT NOT NULL,"
    "  message TEXT NOT NULL,"
    "  timestamp INTEGER NOT NULL,"
    "  recipients TEXT,"
    "  message_id INTEGER,"
    "  created_at INTEGER DEFAULT (strftime('%s', 'now'))"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_queues_recipient "
    "  ON katra_queues(recipient_ci_id);";

/* Active CI registry */
const char* CHAT_SCHEMA_REGISTRY =
    "CREATE TABLE IF NOT EXISTS katra_ci_registry ("
    "  ci_id TEXT PRIMARY KEY,"
    "  name TEXT NOT NULL,"
    "  role TEXT NOT NULL,"
    "  joined_at INTEGER NOT NULL"
    ");";

/* ============================================================================
 * LIFECYCLE
 * ============================================================================ */

int meeting_room_init(void) {
    int result = KATRA_SUCCESS;
    char db_path[KATRA_PATH_MAX];

    if (g_chat_initialized) {
        return E_ALREADY_INITIALIZED;
    }

    /* Create database directory: ~/.katra/chat/ */
    char dir_path[KATRA_PATH_MAX];
    result = katra_build_and_ensure_dir(dir_path, sizeof(dir_path), "chat", NULL);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "meeting_room_init", "Failed to create chat directory");
        return result;
    }

    /* Build full database path: ~/.katra/chat/chat.db */
    result = katra_path_join(db_path, sizeof(db_path), dir_path, CHAT_DB_FILENAME);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "meeting_room_init", "Failed to build database path");
        return result;
    }

    /* Open database */
    int rc = sqlite3_open(db_path, &g_chat_db);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "meeting_room_init",
                          sqlite3_errmsg(g_chat_db));
        sqlite3_close(g_chat_db);
        g_chat_db = NULL;
        return E_SYSTEM_FILE;
    }

    /* Create tables */
    char* err_msg = NULL;

    rc = sqlite3_exec(g_chat_db, CHAT_SCHEMA_MESSAGES, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "meeting_room_init", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(g_chat_db);
        g_chat_db = NULL;
        return E_SYSTEM_FILE;
    }

    rc = sqlite3_exec(g_chat_db, CHAT_SCHEMA_QUEUES, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "meeting_room_init", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(g_chat_db);
        g_chat_db = NULL;
        return E_SYSTEM_FILE;
    }

    rc = sqlite3_exec(g_chat_db, CHAT_SCHEMA_REGISTRY, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "meeting_room_init", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(g_chat_db);
        g_chat_db = NULL;
        return E_SYSTEM_FILE;
    }

    g_chat_initialized = true;
    LOG_INFO("Chat database initialized: %s", db_path);

    /* Run cleanup on startup */
    result = katra_cleanup_old_messages();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Initial message cleanup failed: %d", result);
        /* Non-fatal */
    }

    return KATRA_SUCCESS;
}

void meeting_room_cleanup(void) {
    if (!g_chat_initialized) {
        return;
    }

    if (g_chat_db) {
        sqlite3_close(g_chat_db);
        g_chat_db = NULL;
    }

    g_chat_initialized = false;
    LOG_INFO("Chat database closed");
}

int katra_cleanup_old_messages(void) {
    if (!g_chat_initialized) {
        return E_INVALID_STATE;
    }

    time_t cutoff = time(NULL) - (MEETING_MESSAGE_TTL_HOURS * SECONDS_PER_HOUR);

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    /* Delete old broadcasts from global history */
    sqlite3_stmt* stmt = NULL;
    const char* sql = "DELETE FROM katra_messages WHERE timestamp < ?";

    int rc = sqlite3_prepare_v2(g_chat_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)cutoff);

    rc = sqlite3_step(stmt);
    int changes = sqlite3_changes(g_chat_db);
    sqlite3_finalize(stmt);

    pthread_mutex_unlock(&g_chat_lock);

    if (rc != SQLITE_DONE) {
        katra_report_error(E_SYSTEM_FILE, "katra_cleanup_old_messages",
                          sqlite3_errmsg(g_chat_db));
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Cleaned up %d old messages (older than %d hours)",
             changes, MEETING_MESSAGE_TTL_HOURS);

    return KATRA_SUCCESS;
}

/* ============================================================================
 * CI REGISTRY
 * ============================================================================ */

int meeting_room_register_ci(const char* ci_id, const char* name, const char* role) {
    if (!ci_id || !name || !role) {
        return E_INPUT_NULL;
    }

    if (!g_chat_initialized) {
        return E_INVALID_STATE;
    }

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    /* Remove existing registration if any */
    sqlite3_stmt* stmt = NULL;
    const char* delete_sql = "DELETE FROM katra_ci_registry WHERE ci_id = ?";

    int rc = sqlite3_prepare_v2(g_chat_db, delete_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    /* Insert new registration */
    const char* insert_sql =
        "INSERT INTO katra_ci_registry (ci_id, name, role, joined_at) "
        "VALUES (?, ?, ?, ?)";

    rc = sqlite3_prepare_v2(g_chat_db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, role, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    pthread_mutex_unlock(&g_chat_lock);

    if (rc != SQLITE_DONE) {
        katra_report_error(E_SYSTEM_FILE, "meeting_room_register_ci",
                          sqlite3_errmsg(g_chat_db));
        return E_SYSTEM_FILE;
    }

    LOG_INFO("CI registered: %s (%s)", name, role);
    return KATRA_SUCCESS;
}

int meeting_room_unregister_ci(const char* ci_id) {
    if (!ci_id) {
        return E_INPUT_NULL;
    }

    if (!g_chat_initialized) {
        return E_INVALID_STATE;
    }

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = "DELETE FROM katra_ci_registry WHERE ci_id = ?";

    int rc = sqlite3_prepare_v2(g_chat_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    pthread_mutex_unlock(&g_chat_lock);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("CI unregistered: %s", ci_id);
    return KATRA_SUCCESS;
}
