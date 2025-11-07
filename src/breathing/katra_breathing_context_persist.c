/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_context_persist.c - Context persistence for session continuity
 *
 * This file has been split into 4 modules for maintainability:
 * - katra_breathing_context_persist.c (this file): Global state, init/cleanup
 * - katra_breathing_context_update.c: Update functions
 * - katra_breathing_context_capture.c: Snapshot capture and latent space generation
 * - katra_breathing_context_query.c: Query functions
 *
 * All share state via katra_breathing_context_persist_internal.h
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_breathing_context_persist.h"
#include "katra_breathing_context_persist_internal.h"
#include "katra_breathing_internal.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_string_literals.h"
#include "katra_path_utils.h"
#include "katra_config.h"
#include "katra_env_utils.h"

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static working_context_t* g_working_context = NULL;
static sqlite3* g_context_db = NULL;
static bool g_context_persist_initialized = false;

/* ============================================================================
 * DATABASE SCHEMA
 * ============================================================================ */

/* GUIDELINE_APPROVED: SQL DDL statements */
static const char* SQL_CREATE_SNAPSHOTS =
    "CREATE TABLE IF NOT EXISTS context_snapshots ("
    "  snapshot_id TEXT PRIMARY KEY,"
    "  ci_id TEXT NOT NULL,"
    "  session_id TEXT,"
    "  snapshot_time INTEGER,"
    "  current_focus TEXT,"
    "  active_reasoning TEXT,"
    "  communication_style TEXT,"
    "  user_preferences TEXT,"
    "  recent_accomplishments TEXT,"
    "  active_goals TEXT,"
    "  thinking_patterns TEXT,"
    "  learned_lessons TEXT,"
    "  conversation_summary TEXT,"
    "  context_digest TEXT"
    ");";

static const char* SQL_CREATE_QUESTIONS =
    "CREATE TABLE IF NOT EXISTS pending_questions ("
    "  snapshot_id TEXT,"
    "  question_text TEXT,"
    "  priority INTEGER DEFAULT 0,"
    "  FOREIGN KEY (snapshot_id) REFERENCES context_snapshots(snapshot_id)"
    ");";

static const char* SQL_CREATE_FILES =
    "CREATE TABLE IF NOT EXISTS modified_files ("
    "  snapshot_id TEXT,"
    "  file_path TEXT,"
    "  modification_type TEXT,"
    "  FOREIGN KEY (snapshot_id) REFERENCES context_snapshots(snapshot_id)"
    ");";
/* GUIDELINE_APPROVED_END */

/* ============================================================================
 * GLOBAL STATE ACCESSORS (for split files)
 * ============================================================================ */

working_context_t* context_persist_get_working_context(void) {
    return g_working_context;
}

sqlite3* context_persist_get_db(void) {
    return g_context_db;
}

bool context_persist_is_initialized(void) {
    return g_context_persist_initialized;
}

void context_persist_set_working_context(working_context_t* ctx) {
    g_working_context = ctx;
}

void context_persist_set_db(sqlite3* db) {
    g_context_db = db;
}

void context_persist_set_initialized(bool initialized) {
    g_context_persist_initialized = initialized;
}

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

char* safe_strdup(const char* str) {
    if (!str) return NULL;
    return strdup(str);
}

void free_working_context(working_context_t* ctx) {
    if (!ctx) return;

    free(ctx->current_focus);
    free(ctx->active_reasoning);
    free(ctx->communication_style);
    free(ctx->user_preferences);
    free(ctx->recent_accomplishments);
    free(ctx->active_goals);
    free(ctx->thinking_patterns);
    free(ctx->learned_lessons);

    if (ctx->pending_questions) {
        for (size_t i = 0; i < ctx->pending_question_count; i++) {
            free(ctx->pending_questions[i]);
        }
        free(ctx->pending_questions);
    }

    if (ctx->modified_files) {
        for (size_t i = 0; i < ctx->modified_file_count; i++) {
            free(ctx->modified_files[i]);
        }
        free(ctx->modified_files);
    }

    free(ctx);
}

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int context_persist_init(const char* ci_id) {
    int result = KATRA_SUCCESS;
    char db_path[KATRA_PATH_MAX];
    sqlite3_stmt* stmt = NULL;

    if (!ci_id) {
        result = E_INPUT_NULL;
        goto cleanup;
    }

    if (g_context_persist_initialized) {
        LOG_DEBUG("Context persistence already initialized");
        return KATRA_SUCCESS;
    }

    /* Initialize working context */
    g_working_context = calloc(1, sizeof(working_context_t));
    if (!g_working_context) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    strncpy(g_working_context->ci_id, ci_id, sizeof(g_working_context->ci_id) - 1);

    /* Get database path */
    const char* katra_root = katra_getenv(KATRA_ROOT_VAR);
    if (!katra_root) {
        katra_report_error(E_INPUT_INVALID, "context_persist_init", "KATRA_ROOT not set");
        result = E_INPUT_INVALID;
        goto cleanup;
    }

    snprintf(db_path, sizeof(db_path), "%s/context/context.db", katra_root);

    /* Create context directory */
    char context_dir[KATRA_PATH_MAX];
    snprintf(context_dir, sizeof(context_dir), "%s/context", katra_root);
    katra_ensure_dir(context_dir);

    /* Open database */
    int rc = sqlite3_open(db_path, &g_context_db);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "context_persist_init", sqlite3_errmsg(g_context_db));
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    /* Create tables */
    rc = sqlite3_exec(g_context_db, SQL_CREATE_SNAPSHOTS, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    rc = sqlite3_exec(g_context_db, SQL_CREATE_QUESTIONS, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    rc = sqlite3_exec(g_context_db, SQL_CREATE_FILES, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    g_context_persist_initialized = true;
    LOG_INFO("Context persistence initialized for %s", ci_id);

cleanup:
    if (stmt) sqlite3_finalize(stmt);
    if (result != KATRA_SUCCESS && g_working_context) {
        free_working_context(g_working_context);
        g_working_context = NULL;
    }
    if (result != KATRA_SUCCESS && g_context_db) {
        sqlite3_close(g_context_db);
        g_context_db = NULL;
    }
    return result;
}

void context_persist_cleanup(void) {
    if (!g_context_persist_initialized) {
        return;
    }

    free_working_context(g_working_context);
    g_working_context = NULL;

    if (g_context_db) {
        sqlite3_close(g_context_db);
        g_context_db = NULL;
    }

    g_context_persist_initialized = false;
    LOG_DEBUG("Context persistence cleaned up");
}
