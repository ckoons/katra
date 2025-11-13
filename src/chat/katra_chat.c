/* © 2025 Casey Koons All rights reserved */

/*
 * katra_chat.c - Database-Backed Inter-CI Communication (Public API)
 *
 * Core API for ephemeral chat between CIs across multiple processes.
 * Uses SQLite for multi-process safe message queuing and history.
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

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/* GUIDELINE_APPROVED: SQL query constants and default values */

/**
 * store_broadcast_message() - Store broadcast in global history
 *
 * Returns message_id via out parameter.
 */
static int store_broadcast_message(const char* sender_ci_id, const char* sender_name,
                                   const char* content, time_t timestamp,
                                   sqlite3_int64* message_id_out) {
    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    sqlite3_stmt* stmt = NULL;
    const char* insert_msg_sql =
        "INSERT INTO katra_messages (sender_ci_id, sender_name, message, timestamp) "
        "VALUES (?, ?, ?, ?)";

    int rc = sqlite3_prepare_v2(g_chat_db, insert_msg_sql, -1, &stmt, NULL);
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
        *message_id_out = sqlite3_last_insert_rowid(g_chat_db);
    }
    sqlite3_finalize(stmt);

    pthread_mutex_unlock(&g_chat_lock);

    if (rc != SQLITE_DONE) {
        katra_report_error(E_SYSTEM_FILE, "store_broadcast_message",
                          sqlite3_errmsg(g_chat_db));
        return E_SYSTEM_FILE;
    }

    return KATRA_SUCCESS;
}

/**
 * queue_to_recipients() - Queue message to each recipient's personal queue
 */
static int queue_to_recipients(char** recipient_ci_ids, size_t recipient_count,
                               const char* sender_ci_id, const char* sender_name,
                               const char* content, time_t timestamp,
                               const char* recipients, bool broadcast,
                               sqlite3_int64 message_id) {
    int result = KATRA_SUCCESS;

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    const char* queue_sql =
        "INSERT INTO katra_queues "
        "(recipient_ci_id, recipient_name, sender_ci_id, sender_name, message, timestamp, recipients, message_id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    const char* lookup_sql = "SELECT name FROM katra_ci_registry WHERE ci_id = ?";
    sqlite3_stmt* stmt = NULL;
    sqlite3_stmt* lookup_stmt = NULL;

    for (size_t i = 0; i < recipient_count; i++) {
        /* Look up recipient name from registry */
        char recipient_name[KATRA_PERSONA_SIZE] = "Unknown";
        int rc = sqlite3_prepare_v2(g_chat_db, lookup_sql, -1, &lookup_stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(lookup_stmt, 1, recipient_ci_ids[i], -1, SQLITE_STATIC);
            if (sqlite3_step(lookup_stmt) == SQLITE_ROW) {
                const char* name = (const char*)sqlite3_column_text(lookup_stmt, 0);
                strncpy(recipient_name, name, sizeof(recipient_name) - 1);
                recipient_name[sizeof(recipient_name) - 1] = '\0';
            }
            sqlite3_finalize(lookup_stmt);
            lookup_stmt = NULL;
        }

        /* Skip sender (self-filtering by name) */
        if (strcmp(recipient_name, sender_name) == 0) {
            continue;
        }

        rc = sqlite3_prepare_v2(g_chat_db, queue_sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            result = E_SYSTEM_FILE;
            break;
        }

        sqlite3_bind_text(stmt, 1, recipient_ci_ids[i], -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, recipient_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, sender_ci_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, sender_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, content, -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 6, (sqlite3_int64)timestamp);
        sqlite3_bind_text(stmt, 7, broadcast ? "broadcast" : recipients, -1, SQLITE_STATIC);
        if (broadcast) {
            sqlite3_bind_int64(stmt, 8, message_id);
        } else {
            sqlite3_bind_null(stmt, 8);
        }

        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) {
            result = E_SYSTEM_FILE;
            break;
        }
    }

    pthread_mutex_unlock(&g_chat_lock);

    return result;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int katra_say(const char* content, const char* recipients) {
    char sender_ci_id[KATRA_CI_ID_SIZE];
    char sender_name[KATRA_PERSONA_SIZE] = "Unknown";
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

    /* Get sender name directly from MCP session (no registry lookup needed) */
    get_caller_name(sender_name, sizeof(sender_name));

    /* Determine if broadcast */
    broadcast = is_broadcast(recipients);
    time_t timestamp = time(NULL);
    sqlite3_int64 message_id = 0;

    /* Store broadcast in global history */
    if (broadcast) {
        result = store_broadcast_message(sender_ci_id, sender_name, content,
                                        timestamp, &message_id);
        if (result != KATRA_SUCCESS) {
            return result;
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
    result = queue_to_recipients(recipient_ci_ids, recipient_count,
                                 sender_ci_id, sender_name, content,
                                 timestamp, recipients, broadcast, message_id);

    LOG_DEBUG("CI %s sent message to %zu recipients (%s)",
              sender_name, recipient_count, broadcast ? "broadcast" : "direct");

    /* Cleanup */
    if (recipient_ci_ids) {
        for (size_t i = 0; i < recipient_count; i++) {
            free(recipient_ci_ids[i]);
        }
        free(recipient_ci_ids);
    }

    return result;
}

int katra_hear(heard_message_t* message_out) {
    char receiver_name[KATRA_PERSONA_SIZE];

    if (!message_out) {
        return E_INPUT_NULL;
    }

    if (!g_chat_initialized) {
        return E_INVALID_STATE;
    }

    memset(message_out, 0, sizeof(heard_message_t));

    /* Get receiver name */
    get_caller_name(receiver_name, sizeof(receiver_name));

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    /* Get oldest message from personal queue */
    sqlite3_stmt* stmt = NULL;
    const char* sql =
        "SELECT queue_id, sender_name, message, timestamp, recipients, message_id "
        "FROM katra_queues "
        "WHERE recipient_name = ? "
        "ORDER BY queue_id ASC "
        "LIMIT 1";

    int rc = sqlite3_prepare_v2(g_chat_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, receiver_name, -1, SQLITE_STATIC);

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
    strncpy(message_out->speaker_name, sender_name, KATRA_PERSONA_SIZE - 1);
    message_out->speaker_name[KATRA_PERSONA_SIZE - 1] = '\0';
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
        "SELECT COUNT(*) FROM katra_queues WHERE recipient_name = ?";
    rc = sqlite3_prepare_v2(g_chat_db, count_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, receiver_name, -1, SQLITE_STATIC);
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

int katra_count_messages(size_t* count_out) {
    char receiver_name[KATRA_PERSONA_SIZE];

    if (!count_out) {
        return E_INPUT_NULL;
    }

    if (!g_chat_initialized) {
        return E_INVALID_STATE;
    }

    *count_out = 0;

    /* Get receiver name */
    get_caller_name(receiver_name, sizeof(receiver_name));

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    /* Count messages in personal queue (non-consuming) */
    sqlite3_stmt* stmt = NULL;
    const char* count_sql =
        "SELECT COUNT(*) FROM katra_queues WHERE recipient_name = ?";

    int rc = sqlite3_prepare_v2(g_chat_db, count_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_chat_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, receiver_name, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *count_out = (size_t)sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_chat_lock);

    LOG_DEBUG("Message count for %s: %zu", receiver_name, *count_out);

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

        strncpy(cis[idx].name, name, KATRA_PERSONA_SIZE - 1);
        cis[idx].name[KATRA_PERSONA_SIZE - 1] = '\0';
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

        strncpy(messages[idx].speaker_name, sender_name, KATRA_PERSONA_SIZE - 1);
        messages[idx].speaker_name[KATRA_PERSONA_SIZE - 1] = '\0';
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
