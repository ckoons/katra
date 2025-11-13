/* © 2025 Casey Koons All rights reserved */

/*
 * katra_team.c - Team Management for Namespace Isolation (Phase 7)
 *
 * SQLite-based team registry for memory sharing across CIs.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <pthread.h>

/* Project includes */
#include "katra_team.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_core_common.h"

/* Global state (non-static for use by katra_team_query.c) */
sqlite3* g_team_db = NULL;
pthread_mutex_t g_team_lock = PTHREAD_MUTEX_INITIALIZER;
bool g_team_initialized = false;

/* SQL schema for teams table */
/* GUIDELINE_APPROVED: Multi-line SQL schema string */
#define TEAMS_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS teams (" \
    "  team_name TEXT PRIMARY KEY," \
    "  owner_ci_id TEXT NOT NULL," \
    "  created_at INTEGER NOT NULL" \
    ")"

/* SQL schema for team_members table */
/* GUIDELINE_APPROVED: Multi-line SQL schema string */
#define TEAM_MEMBERS_TABLE_SCHEMA \
    "CREATE TABLE IF NOT EXISTS team_members (" \
    "  team_name TEXT NOT NULL," \
    "  ci_id TEXT NOT NULL," \
    "  is_owner INTEGER NOT NULL DEFAULT 0," \
    "  joined_at INTEGER NOT NULL," \
    "  PRIMARY KEY (team_name, ci_id)," \
    "  FOREIGN KEY (team_name) REFERENCES teams(team_name) ON DELETE CASCADE" \
    ")"

/* Indices for fast lookup */
/* GUIDELINE_APPROVED: SQL index creation strings */
#define CREATE_MEMBER_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_team_members_ci ON team_members(ci_id)"

/* GUIDELINE_APPROVED: SQL index creation strings */
#define CREATE_TEAM_INDEX \
    "CREATE INDEX IF NOT EXISTS idx_team_members_team ON team_members(team_name)"

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int katra_team_init(void) {
    int result = KATRA_SUCCESS;
    char db_path[KATRA_PATH_MAX];
    char* err_msg = NULL;

    int lock_result = pthread_mutex_lock(&g_team_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_team_init", TEAM_ERR_MUTEX_LOCK);
        return E_SYSTEM_PERMISSION;
    }

    if (g_team_initialized) {
        pthread_mutex_unlock(&g_team_lock);
        return KATRA_SUCCESS;
    }

    /* Get team database path: ~/.katra/teams.db */
    result = katra_build_path(db_path, sizeof(db_path), "teams.db", NULL);
    if (result != KATRA_SUCCESS) {
        pthread_mutex_unlock(&g_team_lock);
        return result;
    }

    /* Open database */
    int rc = sqlite3_open(db_path, &g_team_db);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "katra_team_init",
                          "Failed to open team database");
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    /* Enable foreign keys */
    sqlite3_exec(g_team_db, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);  /* GUIDELINE_APPROVED: SQLite pragma */

    /* Create teams table */
    rc = sqlite3_exec(g_team_db, TEAMS_TABLE_SCHEMA, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create teams table: %s", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(g_team_db);
        g_team_db = NULL;
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    /* Create team_members table */
    rc = sqlite3_exec(g_team_db, TEAM_MEMBERS_TABLE_SCHEMA, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create team_members table: %s", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(g_team_db);
        g_team_db = NULL;
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    /* Create indices */
    sqlite3_exec(g_team_db, CREATE_MEMBER_INDEX, NULL, NULL, NULL);
    sqlite3_exec(g_team_db, CREATE_TEAM_INDEX, NULL, NULL, NULL);

    g_team_initialized = true;
    LOG_INFO("Team registry initialized: %s", db_path);

    pthread_mutex_unlock(&g_team_lock);
    return KATRA_SUCCESS;
}

void katra_team_cleanup(void) {
    int lock_result = pthread_mutex_lock(&g_team_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_team_cleanup", TEAM_ERR_MUTEX_LOCK);
        /* Continue cleanup anyway to avoid leaks */
    }

    if (g_team_db) {
        sqlite3_close(g_team_db);
        g_team_db = NULL;
    }

    g_team_initialized = false;
    LOG_DEBUG("Team registry cleaned up");

    pthread_mutex_unlock(&g_team_lock);
}

/* ============================================================================
 * TEAM MANAGEMENT
 * ============================================================================ */

int katra_team_create(const char* team_name, const char* owner_ci_id) {
    if (!team_name || !owner_ci_id) {
        return E_INPUT_NULL;
    }

    if (!g_team_initialized) {
        return E_INVALID_STATE;
    }

    int lock_result = pthread_mutex_lock(&g_team_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_team_create", TEAM_ERR_MUTEX_LOCK);
        return E_SYSTEM_PERMISSION;
    }

    /* Check if team already exists */
    sqlite3_stmt* stmt = NULL;
    const char* check_sql = TEAM_SQL_CHECK_EXISTS;
    int rc = sqlite3_prepare_v2(g_team_db, check_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            /* Team exists */
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&g_team_lock);
            return E_DUPLICATE;
        }
        sqlite3_finalize(stmt);
    }

    /* Insert team */
    const char* insert_sql = TEAM_SQL_CREATE;
    rc = sqlite3_prepare_v2(g_team_db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, owner_ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    /* Add owner as first member */
    const char* member_sql = TEAM_SQL_ADD_MEMBER;
    rc = sqlite3_prepare_v2(g_team_db, member_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, owner_ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, 1);  /* is_owner = 1 */
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    pthread_mutex_unlock(&g_team_lock);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Team created: %s (owner: %s)", team_name, owner_ci_id);
    return KATRA_SUCCESS;
}

int katra_team_join(const char* team_name, const char* ci_id,
                    const char* invited_by) {
    if (!team_name || !ci_id || !invited_by) {
        return E_INPUT_NULL;
    }

    if (!g_team_initialized) {
        return E_INVALID_STATE;
    }

    int lock_result = pthread_mutex_lock(&g_team_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_team_join", TEAM_ERR_MUTEX_LOCK);
        return E_SYSTEM_PERMISSION;
    }

    /* Check if team exists */
    sqlite3_stmt* stmt = NULL;
    const char* check_sql = TEAM_SQL_CHECK_EXISTS;
    int rc = sqlite3_prepare_v2(g_team_db, check_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            /* Team doesn't exist */
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&g_team_lock);
            return E_NOT_FOUND;
        }
        sqlite3_finalize(stmt);
    }

    /* Check if inviter is a member */
    const char* member_check = TEAM_SQL_CHECK_MEMBER;
    rc = sqlite3_prepare_v2(g_team_db, member_check, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, invited_by, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            /* Inviter not authorized */
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&g_team_lock);
            return E_CONSENT_DENIED;
        }
        sqlite3_finalize(stmt);
    }

    /* Check if CI already member */
    const char* already_member = TEAM_SQL_CHECK_MEMBER;
    rc = sqlite3_prepare_v2(g_team_db, already_member, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            /* Already a member */
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&g_team_lock);
            return E_DUPLICATE;
        }
        sqlite3_finalize(stmt);
    }

    /* Add new member */
    const char* insert_sql = TEAM_SQL_ADD_MEMBER;
    rc = sqlite3_prepare_v2(g_team_db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, 0);  /* is_owner = 0 */
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    pthread_mutex_unlock(&g_team_lock);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("CI %s joined team %s (invited by %s)", ci_id, team_name, invited_by);
    return KATRA_SUCCESS;
}

int katra_team_leave(const char* team_name, const char* ci_id) {
    if (!team_name || !ci_id) {
        return E_INPUT_NULL;
    }

    if (!g_team_initialized) {
        return E_INVALID_STATE;
    }

    int lock_result = pthread_mutex_lock(&g_team_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_team_leave", TEAM_ERR_MUTEX_LOCK);
        return E_SYSTEM_PERMISSION;
    }

    /* Check if CI is owner */
    sqlite3_stmt* stmt = NULL;
    const char* owner_check = TEAM_SQL_GET_MEMBER_STATUS;
    int rc = sqlite3_prepare_v2(g_team_db, owner_check, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int is_owner = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            if (is_owner) {
                /* Owner cannot leave, must delete team */
                pthread_mutex_unlock(&g_team_lock);
                return E_CONSENT_DENIED;
            }
        } else {
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&g_team_lock);
            return E_NOT_FOUND;
        }
    }

    /* Remove member */
    const char* delete_sql = TEAM_SQL_REMOVE_MEMBER;
    rc = sqlite3_prepare_v2(g_team_db, delete_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    pthread_mutex_unlock(&g_team_lock);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("CI %s left team %s", ci_id, team_name);
    return KATRA_SUCCESS;
}

int katra_team_delete(const char* team_name, const char* owner_ci_id) {
    if (!team_name || !owner_ci_id) {
        return E_INPUT_NULL;
    }

    if (!g_team_initialized) {
        return E_INVALID_STATE;
    }

    int lock_result = pthread_mutex_lock(&g_team_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_team_delete", TEAM_ERR_MUTEX_LOCK);
        return E_SYSTEM_PERMISSION;
    }

    /* Verify ownership */
    sqlite3_stmt* stmt = NULL;
    const char* owner_check = TEAM_SQL_GET_OWNER;
    int rc = sqlite3_prepare_v2(g_team_db, owner_check, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* actual_owner = (const char*)sqlite3_column_text(stmt, 0);
            if (strcmp(actual_owner, owner_ci_id) != 0) {
                /* Not the owner */
                sqlite3_finalize(stmt);
                pthread_mutex_unlock(&g_team_lock);
                return E_CONSENT_DENIED;
            }
        } else {
            /* Team not found */
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&g_team_lock);
            return E_NOT_FOUND;
        }
        sqlite3_finalize(stmt);
    }

    /* Delete team (cascade will remove members) */
    const char* delete_sql = TEAM_SQL_DELETE;
    rc = sqlite3_prepare_v2(g_team_db, delete_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_team_lock);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, team_name, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    pthread_mutex_unlock(&g_team_lock);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Team deleted: %s (by %s)", team_name, owner_ci_id);
    return KATRA_SUCCESS;
}

