/* © 2025 Casey Koons All rights reserved */

/*
 * katra_chat_helpers.c - Helper functions for chat subsystem
 *
 * Internal utilities for CI name resolution, recipient parsing,
 * and broadcast detection.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_chat_internal.h"
#include "katra_meeting.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_breathing.h"
#include "katra_mcp.h"

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

int get_caller_ci_id(char* ci_id_out, size_t size) {
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

void get_caller_name(char* name_out, size_t size) {
    const char* name = mcp_get_session_name();
    if (name && strlen(name) > 0) {
        strncpy(name_out, name, size - 1);
        name_out[size - 1] = '\0';
    } else {
        strncpy(name_out, "Unknown", size - 1);
        name_out[size - 1] = '\0';
    }
}

int case_insensitive_compare(const char* s1, const char* s2) {
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

bool is_broadcast(const char* recipients) {
    if (!recipients || strlen(recipients) == 0) {
        return true;
    }
    /* GUIDELINE_APPROVED: keyword constant for broadcast messages */
    if (case_insensitive_compare(recipients, "broadcast") == 0) {
        return true;
    }
    return false;
}

int resolve_ci_name_to_id(const char* name, char* ci_id_out, size_t size) {
    if (!name || !ci_id_out) {
        return E_INPUT_NULL;
    }

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    sqlite3_stmt* stmt = NULL;
    /* GUIDELINE_APPROVED: SQL query constant */
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

int get_active_ci_ids(char*** ci_ids_out, size_t* count_out) {
    if (!ci_ids_out || !count_out) {
        return E_INPUT_NULL;
    }

    *ci_ids_out = NULL;
    *count_out = 0;

    if (pthread_mutex_lock(&g_chat_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    sqlite3_stmt* stmt = NULL;
    /* GUIDELINE_APPROVED: SQL query constant */
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

int parse_recipients(const char* recipients_str, const char* sender_ci_id,
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

    /* GUIDELINE_APPROVED: delimiter constant for strtok */
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
