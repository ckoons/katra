/* © 2025 Casey Koons All rights reserved */

/*
 * katra_team_query.c - Team membership query functions (Phase 7)
 *
 * Query and list operations for team membership.
 * Extracted from katra_team.c for file size management.
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_team.h"
#include "katra_error.h"
#include "katra_log.h"

/* External globals from katra_team.c */
extern sqlite3* g_team_db;
extern pthread_mutex_t g_team_lock;
extern bool g_team_initialized;

/* ============================================================================
 * MEMBERSHIP QUERIES
 * ============================================================================ */

bool katra_team_is_member(const char* team_name, const char* ci_id) {
    if (!team_name || !ci_id || !g_team_initialized) {
        return false;
    }

    int lock_result = pthread_mutex_lock(&g_team_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_team_is_member", TEAM_ERR_MUTEX_LOCK);
        return false;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = TEAM_SQL_CHECK_MEMBER;
    int rc = sqlite3_prepare_v2(g_team_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_team_lock);
        return false;
    }

    sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);

    bool is_member = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    pthread_mutex_unlock(&g_team_lock);

    return is_member;
}

bool katra_team_is_owner(const char* team_name, const char* ci_id) {
    if (!team_name || !ci_id || !g_team_initialized) {
        return false;
    }

    int lock_result = pthread_mutex_lock(&g_team_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_team_is_owner", TEAM_ERR_MUTEX_LOCK);
        return false;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = TEAM_SQL_GET_MEMBER_STATUS;
    int rc = sqlite3_prepare_v2(g_team_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_team_lock);
        return false;
    }

    sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);

    bool is_owner = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        is_owner = (sqlite3_column_int(stmt, 0) == 1);
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_team_lock);

    return is_owner;
}

int katra_team_list_members(const char* team_name,
                             team_member_t** members_out,
                             size_t* count_out) {
    if (!team_name || !members_out || !count_out) {
        return E_INPUT_NULL;
    }

    if (!g_team_initialized) {
        return E_INVALID_STATE;
    }

    *members_out = NULL;
    *count_out = 0;

    int lock_result = pthread_mutex_lock(&g_team_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_team_list_members", TEAM_ERR_MUTEX_LOCK);
        return E_SYSTEM_PERMISSION;
    }

    /* Query all members */
    sqlite3_stmt* stmt = NULL;
    const char* sql = TEAM_SQL_LIST_MEMBERS;
    int rc = sqlite3_prepare_v2(g_team_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);

    /* Count members first */
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    sqlite3_reset(stmt);

    if (count == 0) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_team_lock);
        return E_NOT_FOUND;
    }

    /* Allocate array */
    team_member_t* members = calloc(count, sizeof(team_member_t));
    if (!members) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_MEMORY;
    }

    /* Fill array */
    size_t idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < count) {
        const char* ci_id = (const char*)sqlite3_column_text(stmt, 0);
        int is_owner = sqlite3_column_int(stmt, 1);
        sqlite3_int64 joined_at = sqlite3_column_int64(stmt, 2);

        members[idx].ci_id = strdup(ci_id);
        members[idx].team_name = strdup(team_name);
        members[idx].is_owner = (is_owner == 1);
        members[idx].joined_at = (time_t)joined_at;

        if (!members[idx].ci_id || !members[idx].team_name) {
            /* Allocation failed - cleanup */
            katra_team_free_members(members, idx + 1);
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&g_team_lock);
            return E_SYSTEM_MEMORY;
        }

        idx++;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_team_lock);

    *members_out = members;
    *count_out = count;

    return KATRA_SUCCESS;
}

int katra_team_list_for_ci(const char* ci_id,
                            char*** teams_out,
                            size_t* count_out) {
    if (!ci_id || !teams_out || !count_out) {
        return E_INPUT_NULL;
    }

    if (!g_team_initialized) {
        return E_INVALID_STATE;
    }

    *teams_out = NULL;
    *count_out = 0;

    int lock_result = pthread_mutex_lock(&g_team_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_team_list_for_ci", TEAM_ERR_MUTEX_LOCK);
        return E_SYSTEM_PERMISSION;
    }

    /* Query all teams for this CI */
    sqlite3_stmt* stmt = NULL;
    const char* sql = TEAM_SQL_LIST_FOR_CI;
    int rc = sqlite3_prepare_v2(g_team_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);

    /* Count teams */
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    sqlite3_reset(stmt);

    if (count == 0) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_team_lock);
        *count_out = 0;
        return KATRA_SUCCESS;  /* Not an error, just no teams */
    }

    /* Allocate array */
    char** teams = calloc(count, sizeof(char*));
    if (!teams) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_MEMORY;
    }

    /* Fill array */
    size_t idx = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && idx < count) {
        const char* team_name = (const char*)sqlite3_column_text(stmt, 0);
        teams[idx] = strdup(team_name);
        if (!teams[idx]) {
            /* Allocation failed - cleanup */
            katra_team_free_list(teams, idx);
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&g_team_lock);
            return E_SYSTEM_MEMORY;
        }
        idx++;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_team_lock);

    *teams_out = teams;
    *count_out = count;

    return KATRA_SUCCESS;
}

/* ============================================================================
 * CLEANUP FUNCTIONS
 * ============================================================================ */

void katra_team_free_members(team_member_t* members, size_t count) {
    if (!members) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(members[i].ci_id);
        free(members[i].team_name);
    }

    free(members);
}

void katra_team_free_list(char** teams, size_t count) {
    if (!teams) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(teams[i]);
    }

    free(teams);
}
