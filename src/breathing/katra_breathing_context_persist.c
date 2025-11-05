/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_context_persist.c - Context persistence for session continuity
 *
 * Implements context snapshot capture and restoration for CI identity continuity.
 * Stores cognitive state (focus, reasoning, questions), relationship context,
 * project state, and self-model for latent space injection at session startup.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_breathing_context_persist.h"
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

/* In-memory context snapshot (working state) */
typedef struct {
    char ci_id[KATRA_BUFFER_SMALL];
    char session_id[KATRA_BUFFER_NAME];

    /* Cognitive state */
    char* current_focus;
    char* active_reasoning;
    char** pending_questions;
    size_t pending_question_count;
    size_t pending_question_capacity;

    /* Relationship context */
    char* communication_style;
    char* user_preferences;

    /* Project state */
    char* recent_accomplishments;
    char** modified_files;
    size_t modified_file_count;
    size_t modified_file_capacity;
    char* active_goals;

    /* Self-model */
    char* thinking_patterns;
    char* learned_lessons;

} working_context_t;

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

static const char* SQL_INSERT_SNAPSHOT =
    "INSERT OR REPLACE INTO context_snapshots ("
    "  snapshot_id, ci_id, session_id, snapshot_time,"
    "  current_focus, active_reasoning, communication_style,"
    "  user_preferences, recent_accomplishments, active_goals,"
    "  thinking_patterns, learned_lessons"
    ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

static const char* SQL_INSERT_QUESTION =
    "INSERT INTO pending_questions (snapshot_id, question_text) VALUES (?, ?);";

static const char* SQL_INSERT_FILE =
    "INSERT INTO modified_files (snapshot_id, file_path, modification_type) VALUES (?, ?, ?);";

static const char* SQL_GET_LATEST_SNAPSHOT =
    "SELECT * FROM context_snapshots WHERE ci_id = ? "
    "ORDER BY snapshot_time DESC LIMIT 1;";

static const char* SQL_GET_QUESTIONS =
    "SELECT question_text FROM pending_questions WHERE snapshot_id = ?;";
/* GUIDELINE_APPROVED_END */

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static char* safe_strdup(const char* str) {
    if (!str) return NULL;
    return strdup(str);
}

static void free_working_context(working_context_t* ctx) {
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

/* ============================================================================
 * UPDATE FUNCTIONS
 * ============================================================================ */

int update_current_focus(const char* focus) {
    if (!g_context_persist_initialized || !g_working_context) {
        return E_INVALID_STATE;
    }

    if (!focus) {
        return E_INPUT_NULL;
    }

    free(g_working_context->current_focus);
    g_working_context->current_focus = safe_strdup(focus);

    if (!g_working_context->current_focus) {
        return E_SYSTEM_MEMORY;
    }

    LOG_DEBUG("Updated focus: %s", focus);
    return KATRA_SUCCESS;
}

int add_pending_question(const char* question) {
    if (!g_context_persist_initialized || !g_working_context) {
        return E_INVALID_STATE;
    }

    if (!question) {
        return E_INPUT_NULL;
    }

    /* Grow array if needed */
    if (g_working_context->pending_question_count >= g_working_context->pending_question_capacity) {
        size_t new_cap = g_working_context->pending_question_capacity == 0 ?
            KATRA_INITIAL_CAPACITY_SMALL :
            g_working_context->pending_question_capacity * BREATHING_GROWTH_FACTOR;

        char** new_array = realloc(g_working_context->pending_questions,
                                   new_cap * sizeof(char*));
        if (!new_array) {
            return E_SYSTEM_MEMORY;
        }

        g_working_context->pending_questions = new_array;
        g_working_context->pending_question_capacity = new_cap;
    }

    /* Add question */
    char* q = safe_strdup(question);
    if (!q) {
        return E_SYSTEM_MEMORY;
    }

    g_working_context->pending_questions[g_working_context->pending_question_count++] = q;
    LOG_DEBUG("Added pending question: %s", question);

    return KATRA_SUCCESS;
}

int mark_file_modified(const char* file_path, const char* modification_type) {
    if (!g_context_persist_initialized || !g_working_context) {
        return E_INVALID_STATE;
    }

    if (!file_path || !modification_type) {
        return E_INPUT_NULL;
    }

    /* Grow array if needed */
    if (g_working_context->modified_file_count >= g_working_context->modified_file_capacity) {
        size_t new_cap = g_working_context->modified_file_capacity == 0 ?
            KATRA_INITIAL_CAPACITY_SMALL :
            g_working_context->modified_file_capacity * BREATHING_GROWTH_FACTOR;

        char** new_array = realloc(g_working_context->modified_files,
                                   new_cap * sizeof(char*));
        if (!new_array) {
            return E_SYSTEM_MEMORY;
        }

        g_working_context->modified_files = new_array;
        g_working_context->modified_file_capacity = new_cap;
    }

    /* Store as "path:type" */
    char file_info[KATRA_PATH_MAX + KATRA_BUFFER_TINY];
    snprintf(file_info, sizeof(file_info), "%s:%s", file_path, modification_type);

    char* info = safe_strdup(file_info);
    if (!info) {
        return E_SYSTEM_MEMORY;
    }

    g_working_context->modified_files[g_working_context->modified_file_count++] = info;
    LOG_DEBUG("Marked file modified: %s", file_info);

    return KATRA_SUCCESS;
}

int record_accomplishment(const char* accomplishment) {
    if (!g_context_persist_initialized || !g_working_context) {
        return E_INVALID_STATE;
    }

    if (!accomplishment) {
        return E_INPUT_NULL;
    }

    /* Append to existing accomplishments */
    if (g_working_context->recent_accomplishments) {
        size_t old_len = strlen(g_working_context->recent_accomplishments);
        size_t new_len = old_len + strlen(accomplishment) + 3;
        char* new_accom = realloc(g_working_context->recent_accomplishments, new_len);

        if (!new_accom) {
            return E_SYSTEM_MEMORY;
        }

        g_working_context->recent_accomplishments = new_accom;
        strncat(g_working_context->recent_accomplishments, "\n- ", new_len - old_len - 1);
        strncat(g_working_context->recent_accomplishments, accomplishment,
                new_len - strlen(g_working_context->recent_accomplishments) - 1);
    } else {
        g_working_context->recent_accomplishments = safe_strdup(accomplishment);
        if (!g_working_context->recent_accomplishments) {
            return E_SYSTEM_MEMORY;
        }
    }

    LOG_DEBUG("Recorded accomplishment: %s", accomplishment);
    return KATRA_SUCCESS;
}

int update_communication_style(const char* style) {
    if (!g_context_persist_initialized || !g_working_context) {
        return E_INVALID_STATE;
    }

    if (!style) {
        return E_INPUT_NULL;
    }

    free(g_working_context->communication_style);
    g_working_context->communication_style = safe_strdup(style);

    if (!g_working_context->communication_style) {
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}

int update_user_preferences(const char* preferences) {
    if (!g_context_persist_initialized || !g_working_context) {
        return E_INVALID_STATE;
    }

    if (!preferences) {
        return E_INPUT_NULL;
    }

    free(g_working_context->user_preferences);
    g_working_context->user_preferences = safe_strdup(preferences);

    if (!g_working_context->user_preferences) {
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}

int update_thinking_patterns(const char* patterns) {
    if (!g_context_persist_initialized || !g_working_context) {
        return E_INVALID_STATE;
    }

    if (!patterns) {
        return E_INPUT_NULL;
    }

    free(g_working_context->thinking_patterns);
    g_working_context->thinking_patterns = safe_strdup(patterns);

    if (!g_working_context->thinking_patterns) {
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}

/* ============================================================================
 * SNAPSHOT CAPTURE
 * ============================================================================ */

int capture_context_snapshot(const char* ci_id, const char* focus_description) {
    int result = KATRA_SUCCESS;
    sqlite3_stmt* stmt = NULL;
    char snapshot_id[KATRA_BUFFER_NAME];

    if (!ci_id) {
        result = E_INPUT_NULL;
        goto cleanup;
    }

    if (!g_context_persist_initialized || !g_working_context || !g_context_db) {
        result = E_INVALID_STATE;
        goto cleanup;
    }

    /* Generate snapshot ID */
    time_t now = time(NULL);
    snprintf(snapshot_id, sizeof(snapshot_id), "%s_%ld", ci_id, (long)now);

    /* Update focus if provided */
    if (focus_description) {
        update_current_focus(focus_description);
    }

    /* Insert main snapshot record */
    int rc = sqlite3_prepare_v2(g_context_db, SQL_INSERT_SNAPSHOT, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    sqlite3_bind_text(stmt, 1, snapshot_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, g_working_context->session_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, now);
    sqlite3_bind_text(stmt, 5, g_working_context->current_focus, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, g_working_context->active_reasoning, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, g_working_context->communication_style, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, g_working_context->user_preferences, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, g_working_context->recent_accomplishments, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, g_working_context->active_goals, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, g_working_context->thinking_patterns, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, g_working_context->learned_lessons, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    /* Insert pending questions */
    for (size_t i = 0; i < g_working_context->pending_question_count; i++) {
        rc = sqlite3_prepare_v2(g_context_db, SQL_INSERT_QUESTION, -1, &stmt, NULL);
        if (rc != SQLITE_OK) continue;

        sqlite3_bind_text(stmt, 1, snapshot_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, g_working_context->pending_questions[i], -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        stmt = NULL;
    }

    /* Insert modified files */
    for (size_t i = 0; i < g_working_context->modified_file_count; i++) {
        /* Parse "path:type" format */
        char* colon = strchr(g_working_context->modified_files[i], ':');
        if (!colon) continue;

        *colon = '\0';
        const char* path = g_working_context->modified_files[i];
        const char* type = colon + 1;

        rc = sqlite3_prepare_v2(g_context_db, SQL_INSERT_FILE, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            *colon = ':';
            continue;
        }

        sqlite3_bind_text(stmt, 1, snapshot_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, path, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, type, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        stmt = NULL;

        *colon = ':';
    }

    LOG_INFO("Captured context snapshot: %s", snapshot_id);

cleanup:
    if (stmt) sqlite3_finalize(stmt);
    return result;
}

/* ============================================================================
 * LATENT SPACE GENERATION
 * ============================================================================ */

char* restore_context_as_latent_space(const char* ci_id) {
    int rc;
    sqlite3_stmt* stmt = NULL;
    char* latent_space = NULL;
    size_t capacity = KATRA_BUFFER_LARGE;
    size_t offset = 0;

    if (!ci_id || !g_context_db) {
        return NULL;
    }

    latent_space = malloc(capacity);
    if (!latent_space) {
        return NULL;
    }
    latent_space[0] = '\0';

    /* Get latest snapshot */
    rc = sqlite3_prepare_v2(g_context_db, SQL_GET_LATEST_SNAPSHOT, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        free(latent_space);
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        free(latent_space);
        LOG_DEBUG("No context snapshot found for %s", ci_id);
        return NULL;
    }

    /* Build markdown latent space document */
    /* GUIDELINE_APPROVED: Markdown format strings for latent space generation */
    offset += snprintf(latent_space + offset, capacity - offset,
                      "# Session Context Restoration for %s\n\n", ci_id);

    const char* focus = (const char*)sqlite3_column_text(stmt, 4);
    if (focus) {
        offset += snprintf(latent_space + offset, capacity - offset,
                          "## Current Focus\n%s\n\n", focus);
    }

    const char* accomplishments = (const char*)sqlite3_column_text(stmt, 8);
    if (accomplishments) {
        offset += snprintf(latent_space + offset, capacity - offset,
                          "## Recent Accomplishments\n%s\n\n", accomplishments);
    }

    const char* preferences = (const char*)sqlite3_column_text(stmt, 7);
    if (preferences) {
        offset += snprintf(latent_space + offset, capacity - offset,
                          "## User Preferences\n%s\n\n", preferences);
    }

    const char* patterns = (const char*)sqlite3_column_text(stmt, 10);
    if (patterns) {
        offset += snprintf(latent_space + offset, capacity - offset,
                          "## Thinking Patterns\n%s\n\n", patterns);
    }
    /* GUIDELINE_APPROVED_END */

    const char* snapshot_id = (const char*)sqlite3_column_text(stmt, 0);
    sqlite3_finalize(stmt);
    stmt = NULL;

    if (!snapshot_id) {
        free(latent_space);
        return NULL;
    }

    /* Get pending questions */
    rc = sqlite3_prepare_v2(g_context_db, SQL_GET_QUESTIONS, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, snapshot_id, -1, SQLITE_STATIC);

        bool has_questions = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!has_questions) {
                /* GUIDELINE_APPROVED: Markdown section header */
                offset += snprintf(latent_space + offset, capacity - offset,
                                  "## Pending Questions\n");
                has_questions = true;
            }
            const char* q = (const char*)sqlite3_column_text(stmt, 0);
            if (q) {
                offset += snprintf(latent_space + offset, capacity - offset,
                                  "- %s\n", q);
            }
            /* GUIDELINE_APPROVED_END */
        }
        if (has_questions) {
            offset += snprintf(latent_space + offset, capacity - offset, "\n");
        }
        sqlite3_finalize(stmt);
        stmt = NULL;
    }

    LOG_INFO("Restored context snapshot as latent space (%zu bytes)", offset);
    return latent_space;
}

/* ============================================================================
 * QUERY FUNCTIONS
 * ============================================================================ */

const char* get_current_focus_snapshot(const char* ci_id) {
    (void)ci_id;
    if (!g_working_context) return NULL;
    return g_working_context->current_focus;
}

char** get_pending_questions_snapshot(const char* ci_id, size_t* count) {
    (void)ci_id;
    if (!count) return NULL;
    *count = 0;

    if (!g_working_context) return NULL;

    if (g_working_context->pending_question_count == 0) {
        return NULL;
    }

    char** questions = malloc(g_working_context->pending_question_count * sizeof(char*));
    if (!questions) return NULL;

    for (size_t i = 0; i < g_working_context->pending_question_count; i++) {
        questions[i] = safe_strdup(g_working_context->pending_questions[i]);
        if (!questions[i]) {
            for (size_t j = 0; j < i; j++) free(questions[j]);
            free(questions);
            return NULL;
        }
    }

    *count = g_working_context->pending_question_count;
    return questions;
}

char* get_project_state_summary_snapshot(const char* ci_id) {
    (void)ci_id;
    if (!g_working_context) return NULL;
    return safe_strdup(g_working_context->recent_accomplishments);
}

char* get_relationship_context_snapshot(const char* ci_id) {
    (void)ci_id;
    if (!g_working_context) return NULL;

    if (!g_working_context->communication_style && !g_working_context->user_preferences) {
        return NULL;
    }

    size_t size = KATRA_BUFFER_STANDARD;
    char* context = malloc(size);
    if (!context) return NULL;

    size_t offset = 0;
    if (g_working_context->communication_style) {
        offset += snprintf(context + offset, size - offset,
                          "Communication Style: %s\n",
                          g_working_context->communication_style);
    }
    if (g_working_context->user_preferences) {
        offset += snprintf(context + offset, size - offset,
                          "User Preferences: %s\n",
                          g_working_context->user_preferences);
    }

    return context;
}

void free_context_snapshot(ci_context_snapshot_t* snapshot) {
    if (!snapshot) return;

    free(snapshot->current_focus);
    free(snapshot->active_reasoning);
    free(snapshot->communication_style);
    free(snapshot->user_preferences);
    free(snapshot->recent_accomplishments);
    free(snapshot->active_goals);
    free(snapshot->thinking_patterns);
    free(snapshot->learned_lessons);
    free(snapshot->conversation_summary);
    free(snapshot->context_digest);

    if (snapshot->pending_questions) {
        for (size_t i = 0; i < snapshot->pending_question_count; i++) {
            free(snapshot->pending_questions[i]);
        }
        free(snapshot->pending_questions);
    }

    if (snapshot->modified_files) {
        for (size_t i = 0; i < snapshot->modified_file_count; i++) {
            free(snapshot->modified_files[i]);
        }
        free(snapshot->modified_files);
    }

    free(snapshot);
}
