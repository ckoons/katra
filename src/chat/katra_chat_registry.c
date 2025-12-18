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

/* GUIDELINE_APPROVED: SQL schema definitions (database constants) */
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
    "  recipient_name TEXT NOT NULL,"
    "  sender_ci_id TEXT NOT NULL,"
    "  sender_name TEXT NOT NULL,"
    "  message TEXT NOT NULL,"
    "  timestamp INTEGER NOT NULL,"
    "  recipients TEXT,"
    "  message_id INTEGER,"
    "  created_at INTEGER DEFAULT (strftime('%s', 'now')),"
    "  read_at INTEGER DEFAULT NULL"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_queues_recipient "
    "  ON katra_queues(recipient_name);";

/* Active CI registry */
const char* CHAT_SCHEMA_REGISTRY =
    "CREATE TABLE IF NOT EXISTS katra_ci_registry ("
    "  ci_id TEXT PRIMARY KEY,"
    "  name TEXT NOT NULL,"
    "  role TEXT NOT NULL,"
    "  joined_at INTEGER NOT NULL,"
    "  last_seen INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
    "  status TEXT NOT NULL DEFAULT 'available'"
    ");";

/* ============================================================================
 * MIGRATIONS
 * ============================================================================ */

/**
 * migrate_add_last_seen() - Add last_seen column if missing (Phase 4.5.1)
 *
 * Existing databases from Phase 4 don't have last_seen column.
 * This migration adds it safely.
 */
static int migrate_add_last_seen(void) {
    /* Check if column exists */
    sqlite3_stmt* stmt = NULL;
    const char* check_sql = "PRAGMA table_info(katra_ci_registry)";

    int rc = sqlite3_prepare_v2(g_chat_db, check_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    bool has_last_seen = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* col_name = (const char*)sqlite3_column_text(stmt, 1);
        if (col_name && strcmp(col_name, "last_seen") == 0) {
            has_last_seen = true;
            break;
        }
    }
    sqlite3_finalize(stmt);

    if (has_last_seen) {
        LOG_DEBUG("Migration: last_seen column already exists");
        return KATRA_SUCCESS;
    }

    /* Add last_seen column with default value */
    const char* alter_sql =
        "ALTER TABLE katra_ci_registry "
        "ADD COLUMN last_seen INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))";

    char* err_msg = NULL;
    rc = sqlite3_exec(g_chat_db, alter_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Migration failed: %s", err_msg);
        sqlite3_free(err_msg);
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Migration: Added last_seen column to katra_ci_registry");
    return KATRA_SUCCESS;
}

/**
 * migrate_add_status() - Add status column if missing (Phase 7)
 */
static int migrate_add_status(void) {
    /* Check if column exists */
    sqlite3_stmt* stmt = NULL;
    const char* check_sql = "PRAGMA table_info(katra_ci_registry)";

    int rc = sqlite3_prepare_v2(g_chat_db, check_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    bool has_status = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* col_name = (const char*)sqlite3_column_text(stmt, 1);
        if (col_name && strcmp(col_name, "status") == 0) {
            has_status = true;
            break;
        }
    }
    sqlite3_finalize(stmt);

    if (has_status) {
        LOG_DEBUG("Migration: status column already exists");
        return KATRA_SUCCESS;
    }

    /* Add status column with default value */
    const char* alter_sql =
        "ALTER TABLE katra_ci_registry "
        "ADD COLUMN status TEXT NOT NULL DEFAULT 'available'";

    char* err_msg = NULL;
    rc = sqlite3_exec(g_chat_db, alter_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Migration failed: %s", err_msg);
        sqlite3_free(err_msg);
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Migration: Added status column to katra_ci_registry");
    return KATRA_SUCCESS;
}

/**
 * migrate_add_read_at() - Add read_at column if missing (Phase 8)
 */
static int migrate_add_read_at(void) {
    /* Check if column exists */
    sqlite3_stmt* stmt = NULL;
    const char* check_sql = "PRAGMA table_info(katra_queues)";

    int rc = sqlite3_prepare_v2(g_chat_db, check_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    bool has_read_at = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* col_name = (const char*)sqlite3_column_text(stmt, 1);
        if (col_name && strcmp(col_name, "read_at") == 0) {
            has_read_at = true;
            break;
        }
    }
    sqlite3_finalize(stmt);

    if (has_read_at) {
        LOG_DEBUG("Migration: read_at column already exists");
        return KATRA_SUCCESS;
    }

    /* Add read_at column with default NULL */
    const char* alter_sql =
        "ALTER TABLE katra_queues "
        "ADD COLUMN read_at INTEGER DEFAULT NULL";

    char* err_msg = NULL;
    rc = sqlite3_exec(g_chat_db, alter_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Migration failed: %s", err_msg);
        sqlite3_free(err_msg);
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Migration: Added read_at column to katra_queues");
    return KATRA_SUCCESS;
}

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
    /* GUIDELINE_APPROVED: directory name constant */
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

    /* Run migration for Phase 4.5.1 (add last_seen column) */
    result = migrate_add_last_seen();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Migration failed: %d", result);
        /* Non-fatal - new installs have column already */
    }

    /* Run migration for Phase 7 (add status column) */
    result = migrate_add_status();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Status migration failed: %d", result);
        /* Non-fatal - new installs have column already */
    }

    /* Run migration for Phase 8 (add read_at column for read receipts) */
    result = migrate_add_read_at();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Read receipts migration failed: %d", result);
        /* Non-fatal - new installs have column already */
    }

    /* Run cleanup on startup */
    result = katra_cleanup_old_messages();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Initial message cleanup failed: %d", result);
        /* Non-fatal */
    }

    /* Clean up stale registry entries */
    result = katra_cleanup_stale_registrations();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Initial registry cleanup failed: %d", result);
        /* Non-fatal */
    }

    return KATRA_SUCCESS;
}

void meeting_room_cleanup(void) {
    if (!g_chat_initialized) {
        return;
    }

    if (g_chat_db) {
        /* Force WAL checkpoint before close (prevents data loss on restart) */
        sqlite3_wal_checkpoint_v2(g_chat_db, NULL, SQLITE_CHECKPOINT_FULL, NULL, NULL);
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
    /* GUIDELINE_APPROVED: SQL query constant */
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

int katra_cleanup_stale_registrations(void) {
    if (!g_chat_initialized) {
        return E_INVALID_STATE;
    }

    /* Remove registrations not seen in last 5 minutes */
    time_t cutoff = time(NULL) - (STALE_REGISTRATION_TIMEOUT_MINUTES * SECONDS_PER_MINUTE);

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    sqlite3_stmt* stmt = NULL;
    /* GUIDELINE_APPROVED: SQL query constant */
    const char* sql = "DELETE FROM katra_ci_registry WHERE last_seen < ?";

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
        katra_report_error(E_SYSTEM_FILE, "katra_cleanup_stale_registrations",
                          sqlite3_errmsg(g_chat_db));
        return E_SYSTEM_FILE;
    }

    if (changes > 0) {
        LOG_INFO("Cleaned up %d stale CI registrations (not seen in %d minutes)",
                 changes, STALE_REGISTRATION_TIMEOUT_MINUTES);
    }

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

    /* Insert or replace registration (updates last_seen as heartbeat) */
    sqlite3_stmt* stmt = NULL;
    time_t now = time(NULL);

    /* GUIDELINE_APPROVED: SQL query constants */
    const char* insert_sql =
        "INSERT OR REPLACE INTO katra_ci_registry "
        "(ci_id, name, role, joined_at, last_seen) "
        "VALUES (?, ?, ?, "
        "  COALESCE((SELECT joined_at FROM katra_ci_registry WHERE ci_id = ?), ?), "
        "  ?)";

    int rc = sqlite3_prepare_v2(g_chat_db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, role, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, ci_id, -1, SQLITE_STATIC);  /* For COALESCE */
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)now);  /* joined_at if new */
    sqlite3_bind_int64(stmt, 6, (sqlite3_int64)now);  /* last_seen (always updated) */

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
    /* GUIDELINE_APPROVED: SQL query constant */
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
