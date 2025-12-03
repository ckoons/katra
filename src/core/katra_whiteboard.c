/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <jansson.h>

/* Project includes */
#include "katra_whiteboard.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_config.h"
#include "katra_memory.h"
#include "katra_path_utils.h"
#include "katra_file_utils.h"
#include "katra_psyche_common.h"
#include "katra_core_common.h"

/* Database handle (shared with katra_whiteboard_workflow.c) */
sqlite3* wb_db = NULL;
bool wb_initialized = false;

/* Status name strings */
static const char* STATUS_NAMES[] = {
    "draft",
    "questioning",
    "scoping",
    "proposing",
    "voting",
    "designing",
    "approved",
    "archived"
};

/* Vote position name strings */
static const char* VOTE_POSITION_NAMES[] = {
    "support",
    "oppose",
    "abstain",
    "conditional"
};

/* Valid state transitions */
static const bool VALID_TRANSITIONS[8][8] = {
    /* From draft: can go to questioning */
    {0, 1, 0, 0, 0, 0, 0, 0},
    /* From questioning: can go to scoping */
    {0, 0, 1, 0, 0, 0, 0, 0},
    /* From scoping: can go to proposing */
    {0, 0, 0, 1, 0, 0, 0, 0},
    /* From proposing: can go to voting */
    {0, 0, 0, 0, 1, 0, 0, 0},
    /* From voting: can go to designing */
    {0, 0, 0, 0, 0, 1, 0, 0},
    /* From designing: can go to approved, or regress */
    {0, 1, 1, 0, 0, 0, 1, 0},
    /* From approved: can go to archived */
    {0, 0, 0, 0, 0, 0, 0, 1},
    /* From archived: terminal state */
    {0, 0, 0, 0, 0, 0, 0, 0}
};

/* Forward declarations */
static int wb_create_tables(void);
static void wb_parse_goal_json(const char* json_str, wb_goal_t* goal);
static void wb_parse_scope_json(const char* json_str, wb_scope_t* scope);
static void wb_parse_decision_json(const char* json_str, wb_decision_t* decision);

/* External loader functions from katra_whiteboard_loaders.c */
extern int katra_whiteboard_load_questions(const char* wb_id, wb_question_t** questions, size_t* count);
extern int katra_whiteboard_load_approaches(const char* wb_id, wb_approach_t** approaches, size_t* count);
extern int katra_whiteboard_load_votes(const char* wb_id, wb_vote_t** votes, size_t* count);

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int katra_whiteboard_init(void) {
    if (wb_initialized) {
        return KATRA_SUCCESS;
    }

    int result = KATRA_SUCCESS;
    char db_path[KATRA_PATH_MAX];

    /* Build database path - use home-based path or KATRA_DATA_PATH */
    const char* base_path = katra_config_get("KATRA_DATA_PATH");
    if (base_path && strlen(base_path) > 0) {
        /* Custom path specified - use katra_path_join */
        result = katra_path_join(db_path, sizeof(db_path), base_path, "whiteboard.db");
        if (result != KATRA_SUCCESS) {
            return result;
        }
        /* Ensure directory exists */
        result = katra_ensure_dir(base_path);
        if (result != KATRA_SUCCESS) {
            return result;
        }
    } else {
        /* Default: use ~/.katra/whiteboard.db */
        result = katra_build_path(db_path, sizeof(db_path), "whiteboard.db", NULL);
        if (result != KATRA_SUCCESS) {
            return result;
        }
        /* Ensure ~/.katra directory exists */
        char katra_dir[KATRA_PATH_MAX];
        result = katra_build_path(katra_dir, sizeof(katra_dir), NULL);
        if (result != KATRA_SUCCESS) {
            return result;
        }
        result = katra_ensure_dir(katra_dir);
        if (result != KATRA_SUCCESS) {
            return result;
        }
    }

    /* Open database */
    int rc = sqlite3_open(db_path, &wb_db);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "katra_whiteboard_init",
                          "Failed to open whiteboard database");
        return E_SYSTEM_FILE;
    }

    /* Create tables if needed */
    result = wb_create_tables();
    if (result != KATRA_SUCCESS) {
        sqlite3_close(wb_db);
        wb_db = NULL;
        return result;
    }

    wb_initialized = true;
    LOG_INFO("Whiteboard system initialized: %s", db_path);
    return KATRA_SUCCESS;
}

void katra_whiteboard_cleanup(void) {
    if (wb_db) {
        sqlite3_close(wb_db);
        wb_db = NULL;
    }
    wb_initialized = false;
}

static int wb_create_tables(void) {
    const char* schema =
        /* Main whiteboards table */
        "CREATE TABLE IF NOT EXISTS whiteboards ("
        "  id TEXT PRIMARY KEY,"
        "  project TEXT NOT NULL,"
        "  parent_id TEXT,"
        "  status INTEGER NOT NULL DEFAULT 0,"
        "  created_at INTEGER NOT NULL,"
        "  created_by TEXT NOT NULL,"
        "  problem TEXT,"
        "  goal_json TEXT,"
        "  scope_json TEXT,"
        "  decision_json TEXT,"
        "  design_content TEXT,"
        "  design_author TEXT,"
        "  design_approved INTEGER DEFAULT 0,"
        "  design_approved_by TEXT,"
        "  design_approved_at INTEGER,"
        "  FOREIGN KEY (parent_id) REFERENCES whiteboards(id)"
        ");"

        /* Questions table */
        "CREATE TABLE IF NOT EXISTS whiteboard_questions ("
        "  id TEXT PRIMARY KEY,"
        "  whiteboard_id TEXT NOT NULL,"
        "  author TEXT NOT NULL,"
        "  question TEXT NOT NULL,"
        "  answered INTEGER DEFAULT 0,"
        "  answer TEXT,"
        "  created_at INTEGER NOT NULL,"
        "  FOREIGN KEY (whiteboard_id) REFERENCES whiteboards(id)"
        ");"

        /* Approaches table */
        "CREATE TABLE IF NOT EXISTS whiteboard_approaches ("
        "  id TEXT PRIMARY KEY,"
        "  whiteboard_id TEXT NOT NULL,"
        "  author TEXT NOT NULL,"
        "  title TEXT NOT NULL,"
        "  description TEXT NOT NULL,"
        "  pros_json TEXT,"
        "  cons_json TEXT,"
        "  created_at INTEGER NOT NULL,"
        "  FOREIGN KEY (whiteboard_id) REFERENCES whiteboards(id)"
        ");"

        /* Supporters table */
        "CREATE TABLE IF NOT EXISTS whiteboard_supporters ("
        "  whiteboard_id TEXT NOT NULL,"
        "  approach_id TEXT NOT NULL,"
        "  supporter TEXT NOT NULL,"
        "  created_at INTEGER NOT NULL,"
        "  PRIMARY KEY (whiteboard_id, approach_id, supporter),"
        "  FOREIGN KEY (approach_id) REFERENCES whiteboard_approaches(id)"
        ");"

        /* Votes table */
        "CREATE TABLE IF NOT EXISTS whiteboard_votes ("
        "  id TEXT PRIMARY KEY,"
        "  whiteboard_id TEXT NOT NULL,"
        "  approach_id TEXT NOT NULL,"
        "  voter TEXT NOT NULL,"
        "  position INTEGER NOT NULL,"
        "  reasoning TEXT NOT NULL,"
        "  created_at INTEGER NOT NULL,"
        "  UNIQUE (whiteboard_id, approach_id, voter),"
        "  FOREIGN KEY (approach_id) REFERENCES whiteboard_approaches(id)"
        ");"

        /* Regression audit log */
        "CREATE TABLE IF NOT EXISTS whiteboard_regressions ("
        "  id TEXT PRIMARY KEY,"
        "  whiteboard_id TEXT NOT NULL,"
        "  from_status INTEGER NOT NULL,"
        "  to_status INTEGER NOT NULL,"
        "  requested_by TEXT NOT NULL,"
        "  approved_by TEXT,"
        "  reason TEXT NOT NULL,"
        "  created_at INTEGER NOT NULL,"
        "  approved_at INTEGER,"
        "  FOREIGN KEY (whiteboard_id) REFERENCES whiteboards(id)"
        ");"

        /* Design reviews */
        "CREATE TABLE IF NOT EXISTS whiteboard_reviews ("
        "  id TEXT PRIMARY KEY,"
        "  whiteboard_id TEXT NOT NULL,"
        "  reviewer TEXT NOT NULL,"
        "  comment TEXT NOT NULL,"
        "  created_at INTEGER NOT NULL,"
        "  FOREIGN KEY (whiteboard_id) REFERENCES whiteboards(id)"
        ");"

        /* Indices */
        "CREATE INDEX IF NOT EXISTS idx_wb_project ON whiteboards(project);"
        "CREATE INDEX IF NOT EXISTS idx_wb_status ON whiteboards(status);"
        "CREATE INDEX IF NOT EXISTS idx_wb_questions ON whiteboard_questions(whiteboard_id);"
        "CREATE INDEX IF NOT EXISTS idx_wb_approaches ON whiteboard_approaches(whiteboard_id);"
        "CREATE INDEX IF NOT EXISTS idx_wb_votes ON whiteboard_votes(whiteboard_id);";

    char* errmsg = NULL;
    int rc = sqlite3_exec(wb_db, schema, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create whiteboard tables: %s", errmsg);
        sqlite3_free(errmsg);
        return E_SYSTEM_FILE;
    }

    return KATRA_SUCCESS;
}

/* ============================================================================
 * WHITEBOARD MANAGEMENT
 * ============================================================================ */

void katra_whiteboard_generate_id(const char* prefix, char* id_out, size_t size) {
    time_t now = time(NULL);
    int random = rand() % WM_RECORD_ID_RANDOM_MAX;
    snprintf(id_out, size, "%s_%ld_%04d", prefix, (long)now, random);
}

int katra_whiteboard_create(const char* project, const char* problem,
                            const char* created_by, whiteboard_t** whiteboard_out) {
    if (!project || !problem || !created_by || !whiteboard_out) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        int result = katra_whiteboard_init();
        if (result != KATRA_SUCCESS) return result;
    }

    int result = KATRA_SUCCESS;
    whiteboard_t* wb = NULL;

    /* Allocate whiteboard */
    wb = calloc(1, sizeof(whiteboard_t));
    if (!wb) {
        return E_SYSTEM_MEMORY;
    }

    /* Generate ID and populate */
    katra_whiteboard_generate_id("wb", wb->id, sizeof(wb->id));
    SAFE_STRNCPY(wb->project, project);
    SAFE_STRNCPY(wb->problem, problem);
    SAFE_STRNCPY(wb->created_by, created_by);
    wb->status = WB_STATUS_DRAFT;
    wb->created_at = time(NULL);

    /* Insert into database */
    const char* sql = "INSERT INTO whiteboards (id, project, status, created_at, "
                      "created_by, problem) VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    sqlite3_bind_text(stmt, 1, wb->id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, wb->project, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, wb->status);
    sqlite3_bind_int64(stmt, 4, wb->created_at);
    sqlite3_bind_text(stmt, 5, wb->created_by, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, wb->problem, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    LOG_INFO("Created whiteboard %s for project %s", wb->id, project);
    *whiteboard_out = wb;
    wb = NULL;

cleanup:
    if (stmt) sqlite3_finalize(stmt);
    if (wb) free(wb);
    return result;
}

int katra_whiteboard_create_sub(const char* parent_id, const char* problem,
                                const char* created_by, whiteboard_t** whiteboard_out) {
    if (!parent_id || !problem || !created_by || !whiteboard_out) {
        return E_INPUT_NULL;
    }

    /* Get parent to inherit project */
    whiteboard_t* parent = NULL;
    int result = katra_whiteboard_get(parent_id, &parent);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    result = katra_whiteboard_create(parent->project, problem, created_by, whiteboard_out);
    if (result == KATRA_SUCCESS) {
        SAFE_STRNCPY((*whiteboard_out)->parent_id, parent_id);

        /* Update parent_id in database */
        const char* sql = "UPDATE whiteboards SET parent_id = ? WHERE id = ?";
        sqlite3_stmt* stmt = NULL;
        sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, parent_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, (*whiteboard_out)->id, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    katra_whiteboard_free(parent);
    return result;
}

int katra_whiteboard_get(const char* whiteboard_id, whiteboard_t** whiteboard_out) {
    if (!whiteboard_id || !whiteboard_out) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        int init_result = katra_whiteboard_init();
        if (init_result != KATRA_SUCCESS) return init_result;
    }

    whiteboard_t* wb = NULL;
    sqlite3_stmt* stmt = NULL;

    const char* sql = "SELECT id, project, parent_id, status, created_at, created_by, "
                      "problem, goal_json, scope_json, decision_json, design_content, "
                      "design_author, design_approved, design_approved_by, design_approved_at "
                      "FROM whiteboards WHERE id = ?";

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, whiteboard_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return E_NOT_FOUND;
    }

    /* Allocate and populate */
    wb = calloc(1, sizeof(whiteboard_t));
    if (!wb) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    const char* text;
    text = (const char*)sqlite3_column_text(stmt, 0);
    if (text) SAFE_STRNCPY(wb->id, text);

    text = (const char*)sqlite3_column_text(stmt, 1);
    if (text) SAFE_STRNCPY(wb->project, text);

    text = (const char*)sqlite3_column_text(stmt, 2);
    if (text) SAFE_STRNCPY(wb->parent_id, text);

    wb->status = sqlite3_column_int(stmt, 3);
    wb->created_at = sqlite3_column_int64(stmt, 4);

    text = (const char*)sqlite3_column_text(stmt, 5);
    if (text) SAFE_STRNCPY(wb->created_by, text);

    text = (const char*)sqlite3_column_text(stmt, 6);
    if (text) SAFE_STRNCPY(wb->problem, text);

    /* Parse JSON fields - columns 7, 8, 9 per schema */
    /* GUIDELINE_APPROVED: SQLite column indices are positional API requirements */
    text = (const char*)sqlite3_column_text(stmt, 7); /* goal_json */
    if (text) wb_parse_goal_json(text, &wb->goal);

    text = (const char*)sqlite3_column_text(stmt, 8); /* scope_json */
    if (text) wb_parse_scope_json(text, &wb->scope);

    text = (const char*)sqlite3_column_text(stmt, 9); /* decision_json */
    if (text) wb_parse_decision_json(text, &wb->decision);

    /* GUIDELINE_APPROVED: SQLite column indices are positional API requirements */
    text = (const char*)sqlite3_column_text(stmt, 10); /* GUIDELINE_APPROVED */
    if (text) wb->design.content = katra_safe_strdup(text);

    text = (const char*)sqlite3_column_text(stmt, 11); /* GUIDELINE_APPROVED */
    if (text) SAFE_STRNCPY(wb->design.author, text);

    wb->design.approved = sqlite3_column_int(stmt, 12) != 0; /* GUIDELINE_APPROVED */

    text = (const char*)sqlite3_column_text(stmt, 13); /* GUIDELINE_APPROVED */
    if (text) SAFE_STRNCPY(wb->design.approved_by, text);

    wb->design.approved_at = sqlite3_column_int64(stmt, 14); /* GUIDELINE_APPROVED */

    sqlite3_finalize(stmt);

    /* Load related data */
    katra_whiteboard_load_questions(whiteboard_id, &wb->questions, &wb->question_count);
    katra_whiteboard_load_approaches(whiteboard_id, &wb->approaches, &wb->approach_count);
    katra_whiteboard_load_votes(whiteboard_id, &wb->votes, &wb->vote_count);

    *whiteboard_out = wb;
    return KATRA_SUCCESS;
}

int katra_whiteboard_get_active(const char* project, whiteboard_t** whiteboard_out) {
    if (!project || !whiteboard_out) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        int result = katra_whiteboard_init();
        if (result != KATRA_SUCCESS) return result;
    }

    const char* sql = "SELECT id FROM whiteboards WHERE project = ? AND status < ? "
                      "ORDER BY created_at DESC LIMIT 1";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, project, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, WB_STATUS_ARCHIVED);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return E_NOT_FOUND;
    }

    const char* wb_id = (const char*)sqlite3_column_text(stmt, 0);
    char id_copy[KATRA_BUFFER_SMALL];
    SAFE_STRNCPY(id_copy, wb_id);
    sqlite3_finalize(stmt);

    return katra_whiteboard_get(id_copy, whiteboard_out);
}

/* katra_whiteboard_list, katra_whiteboard_free, katra_whiteboard_summaries_free
 * are in katra_whiteboard_loaders.c */

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

const char* katra_whiteboard_status_name(whiteboard_status_t status) {
    if (status >= 0 && status <= WB_STATUS_ARCHIVED) {
        return STATUS_NAMES[status];
    }
    return "unknown";
}

const char* katra_vote_position_name(vote_position_t position) {
    if (position >= 0 && position <= VOTE_CONDITIONAL) {
        return VOTE_POSITION_NAMES[position];
    }
    return "unknown";
}

bool katra_whiteboard_can_transition(whiteboard_status_t from, whiteboard_status_t to) {
    if (from < 0 || from > WB_STATUS_ARCHIVED) return false;
    if (to < 0 || to > WB_STATUS_ARCHIVED) return false;
    return VALID_TRANSITIONS[from][to];
}

/* ============================================================================
 * JSON PARSING HELPERS
 * ============================================================================ */

/* Parse goal JSON: {"criteria": ["criterion1", "criterion2", ...]} */
static void wb_parse_goal_json(const char* json_str, wb_goal_t* goal) {
    if (!json_str || !goal) return;

    json_error_t error;
    json_t* root = json_loads(json_str, 0, &error);
    if (!root) return;

    json_t* criteria = json_object_get(root, "criteria");
    if (json_is_array(criteria)) {
        size_t count = json_array_size(criteria);
        if (count > WB_MAX_CRITERIA) count = WB_MAX_CRITERIA;

        goal->criteria = calloc(count, sizeof(char*));
        if (goal->criteria) {
            goal->criteria_count = 0;
            for (size_t i = 0; i < count; i++) {
                json_t* item = json_array_get(criteria, i);
                if (json_is_string(item)) {
                    goal->criteria[goal->criteria_count] = katra_safe_strdup(json_string_value(item));
                    if (goal->criteria[goal->criteria_count]) {
                        goal->criteria_count++;
                    }
                }
            }
        }
    }

    json_decref(root);
}

/* Parse scope JSON: {"included": [...], "excluded": [...], "phases": [...]} */
static void wb_parse_scope_json(const char* json_str, wb_scope_t* scope) {
    if (!json_str || !scope) return;

    json_error_t error;
    json_t* root = json_loads(json_str, 0, &error);
    if (!root) return;

    /* Parse included items */
    json_t* included = json_object_get(root, "included");
    if (json_is_array(included)) {
        size_t count = json_array_size(included);
        if (count > WB_MAX_SCOPE_ITEMS) count = WB_MAX_SCOPE_ITEMS;

        scope->included = calloc(count, sizeof(char*));
        if (scope->included) {
            scope->included_count = 0;
            for (size_t i = 0; i < count; i++) {
                json_t* item = json_array_get(included, i);
                if (json_is_string(item)) {
                    scope->included[scope->included_count] = katra_safe_strdup(json_string_value(item));
                    if (scope->included[scope->included_count]) {
                        scope->included_count++;
                    }
                }
            }
        }
    }

    /* Parse excluded items */
    json_t* excluded = json_object_get(root, "excluded");
    if (json_is_array(excluded)) {
        size_t count = json_array_size(excluded);
        if (count > WB_MAX_SCOPE_ITEMS) count = WB_MAX_SCOPE_ITEMS;

        scope->excluded = calloc(count, sizeof(char*));
        if (scope->excluded) {
            scope->excluded_count = 0;
            for (size_t i = 0; i < count; i++) {
                json_t* item = json_array_get(excluded, i);
                if (json_is_string(item)) {
                    scope->excluded[scope->excluded_count] = katra_safe_strdup(json_string_value(item));
                    if (scope->excluded[scope->excluded_count]) {
                        scope->excluded_count++;
                    }
                }
            }
        }
    }

    /* Parse phases */
    json_t* phases = json_object_get(root, "phases");
    if (json_is_array(phases)) {
        size_t count = json_array_size(phases);
        if (count > WB_MAX_SCOPE_ITEMS) count = WB_MAX_SCOPE_ITEMS;

        scope->phases = calloc(count, sizeof(char*));
        if (scope->phases) {
            scope->phase_count = 0;
            for (size_t i = 0; i < count; i++) {
                json_t* item = json_array_get(phases, i);
                if (json_is_string(item)) {
                    scope->phases[scope->phase_count] = katra_safe_strdup(json_string_value(item));
                    if (scope->phases[scope->phase_count]) {
                        scope->phase_count++;
                    }
                }
            }
        }
    }

    json_decref(root);
}

/* Parse decision JSON: {"selected_approach": "...", "decided_by": "...", "decided_at": N, "notes": "..."} */
static void wb_parse_decision_json(const char* json_str, wb_decision_t* decision) {
    if (!json_str || !decision) return;

    json_error_t error;
    json_t* root = json_loads(json_str, 0, &error);
    if (!root) return;

    json_t* val;

    val = json_object_get(root, "selected_approach");
    if (json_is_string(val)) {
        SAFE_STRNCPY(decision->selected_approach, json_string_value(val));
    }

    val = json_object_get(root, "decided_by");
    if (json_is_string(val)) {
        SAFE_STRNCPY(decision->decided_by, json_string_value(val));
    }

    val = json_object_get(root, "decided_at");
    if (json_is_integer(val)) {
        decision->decided_at = json_integer_value(val);
    }

    val = json_object_get(root, "notes");
    if (json_is_string(val)) {
        SAFE_STRNCPY(decision->notes, json_string_value(val));
    }

    json_decref(root);
}

/* Loader functions are in katra_whiteboard_loaders.c */
