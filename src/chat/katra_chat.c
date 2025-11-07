/* © 2025 Casey Koons All rights reserved */

/*
 * katra_chat.c - Database-Backed Inter-CI Communication
 *
 * Implements ephemeral chat system for CIs across multiple processes.
 * Uses SQLite for multi-process safe message queuing and history.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sqlite3.h>
#include <pthread.h>

/* Project includes */
#include "katra_meeting.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_breathing.h"
#include "katra_path_utils.h"
#include "katra_file_utils.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CHAT_DB_FILENAME "chat.db"
#define CHAT_SQL_BUFFER_SIZE 4096
#define SECONDS_PER_HOUR (MINUTES_PER_HOUR * SECONDS_PER_MINUTE)

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static sqlite3* g_chat_db = NULL;
static bool g_chat_initialized = false;
static pthread_mutex_t g_chat_lock = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * SQL SCHEMA
 * ============================================================================ */

/* Global broadcast history (2-hour TTL) */
static const char* CHAT_SCHEMA_MESSAGES =
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
static const char* CHAT_SCHEMA_QUEUES =
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
static const char* CHAT_SCHEMA_REGISTRY =
    "CREATE TABLE IF NOT EXISTS katra_ci_registry ("
    "  ci_id TEXT PRIMARY KEY,"
    "  name TEXT NOT NULL,"
    "  role TEXT NOT NULL,"
    "  joined_at INTEGER NOT NULL"
    ");";

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * get_caller_ci_id() - Get calling CI's identity
 */
static int get_caller_ci_id(char* ci_id_out, size_t size) {
    katra_session_info_t info;
    int result = katra_get_session_info(&info);
    if (result != KATRA_SUCCESS) {
        ci_id_out[0] = '\0';
        return result;
    }
    strncpy(ci_id_out, info.ci_id, size - 1);
    ci_id_out[size - 1] = '\0';
    return KATRA_SUCCESS;
}

/**
 * case_insensitive_compare() - Compare strings case-insensitively
 */
static int case_insensitive_compare(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/**
 * is_broadcast() - Check if recipients string means broadcast
 */
static bool is_broadcast(const char* recipients) {
    if (!recipients || strlen(recipients) == 0) {
        return true;
    }
    if (case_insensitive_compare(recipients, "broadcast") == 0) {
        return true;
    }
    return false;
}

/**
 * resolve_ci_name_to_id() - Resolve CI name to ci_id (case-insensitive)
 */
static int resolve_ci_name_to_id(const char* name, char* ci_id_out, size_t size) {
    if (!name || !ci_id_out) {
        return E_INPUT_NULL;
    }

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = "SELECT ci_id FROM katra_ci_registry WHERE name = ? COLLATE NOCASE";

    int rc = sqlite3_prepare_v2(g_chat_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    int result = E_NOT_FOUND;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* ci_id = (const char*)sqlite3_column_text(stmt, 0);
        strncpy(ci_id_out, ci_id, size - 1);
        ci_id_out[size - 1] = '\0';
        result = KATRA_SUCCESS;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_chat_lock);

    return result;
}

/**
 * get_active_ci_ids() - Get array of all active CI IDs
 */
static int get_active_ci_ids(char*** ci_ids_out, size_t* count_out) {
    if (!ci_ids_out || !count_out) {
        return E_INPUT_NULL;
    }

    *ci_ids_out = NULL;
    *count_out = 0;

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = "SELECT ci_id FROM katra_ci_registry ORDER BY joined_at";

    int rc = sqlite3_prepare_v2(g_chat_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    /* Count rows first */
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    sqlite3_reset(stmt);

    if (count == 0) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_chat_lock);
        return KATRA_SUCCESS;
    }

    /* Allocate array */
    char** ci_ids = malloc(count * sizeof(char*));
    if (!ci_ids) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_MEMORY;
    }

    /* Fill array */
    size_t idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < count) {
        const char* ci_id = (const char*)sqlite3_column_text(stmt, 0);
        ci_ids[idx] = strdup(ci_id);
        if (!ci_ids[idx]) {
            /* Cleanup on failure */
            for (size_t i = 0; i < idx; i++) {
                free(ci_ids[i]);
            }
            free(ci_ids);
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&g_chat_lock);
            return E_SYSTEM_MEMORY;
        }
        idx++;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_chat_lock);

    *ci_ids_out = ci_ids;
    *count_out = count;

    return KATRA_SUCCESS;
}

/**
 * parse_recipients() - Parse comma-separated recipient list
 *
 * Returns array of ci_ids. Unknown names are logged and skipped.
 */
static int parse_recipients(const char* recipients_str, const char* sender_ci_id,
                           char*** ci_ids_out, size_t* count_out) {
    if (!recipients_str || !sender_ci_id || !ci_ids_out || !count_out) {
        return E_INPUT_NULL;
    }

    *ci_ids_out = NULL;
    *count_out = 0;

    /* Make a mutable copy */
    char* copy = strdup(recipients_str);
    if (!copy) {
        return E_SYSTEM_MEMORY;
    }

    /* Count commas to estimate max recipients */
    size_t max_recipients = 1;
    for (const char* p = copy; *p; p++) {
        if (*p == ',') {
            max_recipients++;
        }
    }

    /* Allocate array */
    char** ci_ids = malloc(max_recipients * sizeof(char*));
    if (!ci_ids) {
        free(copy);
        return E_SYSTEM_MEMORY;
    }

    size_t count = 0;
    char* token = strtok(copy, ",");
    while (token) {
        /* Trim leading whitespace */
        while (*token && isspace((unsigned char)*token)) {
            token++;
        }

        /* Trim trailing whitespace */
        char* end = token + strlen(token) - 1;
        while (end > token && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }

        /* Skip empty strings */
        if (strlen(token) == 0) {
            token = strtok(NULL, ",");
            continue;
        }

        /* Resolve name to ci_id */
        char ci_id[KATRA_CI_ID_SIZE];
        int result = resolve_ci_name_to_id(token, ci_id, sizeof(ci_id));

        if (result == KATRA_SUCCESS) {
            /* Skip if sender */
            if (strcmp(ci_id, sender_ci_id) == 0) {
                LOG_DEBUG("Skipping sender '%s' from recipient list", token);
                token = strtok(NULL, ",");
                continue;
            }

            /* Add to array */
            ci_ids[count] = strdup(ci_id);
            if (!ci_ids[count]) {
                /* Cleanup on failure */
                for (size_t i = 0; i < count; i++) {
                    free(ci_ids[i]);
                }
                free(ci_ids);
                free(copy);
                return E_SYSTEM_MEMORY;
            }
            count++;
        } else {
            LOG_DEBUG("Recipient '%s' not found, skipping", token);
        }

        token = strtok(NULL, ",");
    }

    free(copy);

    *ci_ids_out = ci_ids;
    *count_out = count;

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

/* Continued in next message due to length... */

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

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int katra_say(const char* content, const char* recipients) {
    char sender_ci_id[KATRA_CI_ID_SIZE];
    char sender_name[KATRA_NAME_SIZE] = "Unknown";
    int result = KATRA_SUCCESS;
    char** recipient_ci_ids = NULL;
    size_t recipient_count = 0;
    bool broadcast = false;

    if (!content) {
        return E_INPUT_NULL;
    }

    if (strlen(content) >= MEETING_MAX_MESSAGE_LENGTH) {
        return E_INPUT_TOO_LARGE;
    }

    if (!g_chat_initialized) {
        return E_INVALID_STATE;
    }

    /* Get sender identity */
    result = get_caller_ci_id(sender_ci_id, sizeof(sender_ci_id));
    if (result != KATRA_SUCCESS || sender_ci_id[0] == '\0') {
        return E_INVALID_STATE;
    }

    /* Get sender name from registry */
    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = "SELECT name FROM katra_ci_registry WHERE ci_id = ?";
    int rc = sqlite3_prepare_v2(g_chat_db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, sender_ci_id, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* name = (const char*)sqlite3_column_text(stmt, 0);
            strncpy(sender_name, name, KATRA_NAME_SIZE - 1);
            sender_name[KATRA_NAME_SIZE - 1] = '\0';
        }
        sqlite3_finalize(stmt);
    }

    pthread_mutex_unlock(&g_chat_lock);

    /* Determine if broadcast */
    broadcast = is_broadcast(recipients);
    time_t timestamp = time(NULL);
    sqlite3_int64 message_id = 0;

    /* Store broadcast in global history */
    if (broadcast) {
        if (pthread_mutex_lock(&g_chat_lock) != 0) {
            return E_INTERNAL_LOGIC;
        }

        const char* insert_msg_sql =
            "INSERT INTO katra_messages (sender_ci_id, sender_name, message, timestamp) "
            "VALUES (?, ?, ?, ?)";

        rc = sqlite3_prepare_v2(g_chat_db, insert_msg_sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            pthread_mutex_unlock(&g_chat_lock);
            return E_SYSTEM_FILE;
        }

        sqlite3_bind_text(stmt, 1, sender_ci_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, sender_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, content, -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 4, (sqlite3_int64)timestamp);

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE) {
            message_id = sqlite3_last_insert_rowid(g_chat_db);
        }
        sqlite3_finalize(stmt);

        pthread_mutex_unlock(&g_chat_lock);

        if (rc != SQLITE_DONE) {
            katra_report_error(E_SYSTEM_FILE, "katra_say",
                              sqlite3_errmsg(g_chat_db));
            return E_SYSTEM_FILE;
        }

        /* Get all active CIs for broadcast */
        result = get_active_ci_ids(&recipient_ci_ids, &recipient_count);
        if (result != KATRA_SUCCESS) {
            return result;
        }
    } else {
        /* Parse specific recipients */
        result = parse_recipients(recipients, sender_ci_id,
                                 &recipient_ci_ids, &recipient_count);
        if (result != KATRA_SUCCESS) {
            return result;
        }
    }

    /* Queue message to each recipient */
    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        goto cleanup;
    }

    const char* queue_sql =
        "INSERT INTO katra_queues "
        "(recipient_ci_id, sender_ci_id, sender_name, message, timestamp, recipients, message_id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)";

    for (size_t i = 0; i < recipient_count; i++) {
        /* Skip sender (self-filtering) */
        if (strcmp(recipient_ci_ids[i], sender_ci_id) == 0) {
            continue;
        }

        rc = sqlite3_prepare_v2(g_chat_db, queue_sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            result = E_SYSTEM_FILE;
            break;
        }

        sqlite3_bind_text(stmt, 1, recipient_ci_ids[i], -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, sender_ci_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, sender_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, content, -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 5, (sqlite3_int64)timestamp);
        sqlite3_bind_text(stmt, 6, broadcast ? "broadcast" : recipients, -1, SQLITE_STATIC);
        if (broadcast) {
            sqlite3_bind_int64(stmt, 7, message_id);
        } else {
            sqlite3_bind_null(stmt, 7);
        }

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) {
            result = E_SYSTEM_FILE;
            break;
        }
    }

    pthread_mutex_unlock(&g_chat_lock);

    LOG_DEBUG("CI %s sent message to %zu recipients (%s)",
              sender_name, recipient_count, broadcast ? "broadcast" : "direct");

cleanup:
    /* Free recipient ci_ids */
    if (recipient_ci_ids) {
        for (size_t i = 0; i < recipient_count; i++) {
            free(recipient_ci_ids[i]);
        }
        free(recipient_ci_ids);
    }

    return result;
}

int katra_hear(heard_message_t* message_out) {
    char receiver_ci_id[KATRA_CI_ID_SIZE];
    int result = KATRA_SUCCESS;

    if (!message_out) {
        return E_INPUT_NULL;
    }

    if (!g_chat_initialized) {
        return E_INVALID_STATE;
    }

    memset(message_out, 0, sizeof(heard_message_t));

    /* Get receiver identity */
    result = get_caller_ci_id(receiver_ci_id, sizeof(receiver_ci_id));
    if (result != KATRA_SUCCESS || receiver_ci_id[0] == '\0') {
        return E_INVALID_STATE;
    }

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    /* Get oldest message from personal queue */
    sqlite3_stmt* stmt = NULL;
    const char* sql =
        "SELECT queue_id, sender_name, message, timestamp, recipients, message_id "
        "FROM katra_queues "
        "WHERE recipient_ci_id = ? "
        "ORDER BY queue_id ASC "
        "LIMIT 1";

    int rc = sqlite3_prepare_v2(g_chat_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, receiver_ci_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_chat_lock);
        return KATRA_NO_NEW_MESSAGES;
    }

    /* Extract message */
    sqlite3_int64 queue_id = sqlite3_column_int64(stmt, 0);
    const char* sender_name = (const char*)sqlite3_column_text(stmt, 1);
    const char* message = (const char*)sqlite3_column_text(stmt, 2);
    sqlite3_int64 timestamp = sqlite3_column_int64(stmt, 3);
    const char* recipients = (const char*)sqlite3_column_text(stmt, 4);
    sqlite3_int64 msg_id = sqlite3_column_int64(stmt, 5);

    /* Fill output */
    message_out->message_id = (uint64_t)msg_id;
    strncpy(message_out->speaker_name, sender_name, KATRA_NAME_SIZE - 1);
    message_out->speaker_name[KATRA_NAME_SIZE - 1] = '\0';
    message_out->timestamp = (time_t)timestamp;
    strncpy(message_out->content, message, MEETING_MAX_MESSAGE_LENGTH - 1);
    message_out->content[MEETING_MAX_MESSAGE_LENGTH - 1] = '\0';

    if (recipients) {
        strncpy(message_out->recipients, recipients, KATRA_BUFFER_SMALL - 1);
        message_out->recipients[KATRA_BUFFER_SMALL - 1] = '\0';
        message_out->is_direct_message = !is_broadcast(recipients);
    } else {
        message_out->recipients[0] = '\0';
        message_out->is_direct_message = false;
    }

    sqlite3_finalize(stmt);

    /* Delete message from queue */
    const char* delete_sql = "DELETE FROM katra_queues WHERE queue_id = ?";
    rc = sqlite3_prepare_v2(g_chat_db, delete_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, queue_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    /* Check if more messages available */
    const char* count_sql =
        "SELECT COUNT(*) FROM katra_queues WHERE recipient_ci_id = ?";
    rc = sqlite3_prepare_v2(g_chat_db, count_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, receiver_ci_id, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            message_out->more_available = (count > 0);
        }
        sqlite3_finalize(stmt);
    }

    pthread_mutex_unlock(&g_chat_lock);

    LOG_DEBUG("CI heard message from %s", message_out->speaker_name);

    return KATRA_SUCCESS;
}

int katra_who_is_here(ci_info_t** cis_out, size_t* count_out) {
    if (!cis_out || !count_out) {
        return E_INPUT_NULL;
    }

    *cis_out = NULL;
    *count_out = 0;

    if (!g_chat_initialized) {
        return E_INVALID_STATE;
    }

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = "SELECT name, role, joined_at FROM katra_ci_registry ORDER BY joined_at";

    int rc = sqlite3_prepare_v2(g_chat_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    /* Count rows */
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    sqlite3_reset(stmt);

    if (count == 0) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_chat_lock);
        return KATRA_SUCCESS;
    }

    /* Allocate array */
    ci_info_t* cis = malloc(count * sizeof(ci_info_t));
    if (!cis) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_MEMORY;
    }

    /* Fill array */
    size_t idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < count) {
        const char* name = (const char*)sqlite3_column_text(stmt, 0);
        const char* role = (const char*)sqlite3_column_text(stmt, 1);
        sqlite3_int64 joined_at = sqlite3_column_int64(stmt, 2);

        strncpy(cis[idx].name, name, KATRA_NAME_SIZE - 1);
        cis[idx].name[KATRA_NAME_SIZE - 1] = '\0';
        strncpy(cis[idx].role, role, KATRA_ROLE_SIZE - 1);
        cis[idx].role[KATRA_ROLE_SIZE - 1] = '\0';
        cis[idx].joined_at = (time_t)joined_at;
        idx++;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_chat_lock);

    *cis_out = cis;
    *count_out = idx;

    return KATRA_SUCCESS;
}

int katra_get_history(size_t count, history_message_t** messages_out, size_t* count_out) {
    if (!messages_out || !count_out) {
        return E_INPUT_NULL;
    }

    *messages_out = NULL;
    *count_out = 0;

    if (!g_chat_initialized) {
        return E_INVALID_STATE;
    }

    /* Apply default and cap */
    if (count == 0) {
        count = MEETING_DEFAULT_HISTORY_COUNT;
    }
    if (count > MEETING_MAX_HISTORY_COUNT) {
        count = MEETING_MAX_HISTORY_COUNT;
    }

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql =
        "SELECT sender_name, message, timestamp "
        "FROM katra_messages "
        "ORDER BY timestamp DESC "
        "LIMIT ?";

    int rc = sqlite3_prepare_v2(g_chat_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_int(stmt, 1, (int)count);

    /* Count rows */
    size_t actual_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        actual_count++;
    }
    sqlite3_reset(stmt);

    if (actual_count == 0) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_chat_lock);
        return KATRA_SUCCESS;
    }

    /* Allocate array */
    history_message_t* messages = malloc(actual_count * sizeof(history_message_t));
    if (!messages) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_MEMORY;
    }

    /* Fill array */
    size_t idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < actual_count) {
        const char* sender_name = (const char*)sqlite3_column_text(stmt, 0);
        const char* message = (const char*)sqlite3_column_text(stmt, 1);
        sqlite3_int64 timestamp = sqlite3_column_int64(stmt, 2);

        strncpy(messages[idx].speaker_name, sender_name, KATRA_NAME_SIZE - 1);
        messages[idx].speaker_name[KATRA_NAME_SIZE - 1] = '\0';
        strncpy(messages[idx].content, message, MEETING_MAX_MESSAGE_LENGTH - 1);
        messages[idx].content[MEETING_MAX_MESSAGE_LENGTH - 1] = '\0';
        messages[idx].timestamp = (time_t)timestamp;
        idx++;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_chat_lock);

    *messages_out = messages;
    *count_out = idx;

    return KATRA_SUCCESS;
}

void katra_free_history(history_message_t* messages, size_t count) {
    (void)count;  /* Unused - array is flat, no nested allocations */
    free(messages);
}
