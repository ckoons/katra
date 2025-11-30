/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

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
static int wb_load_questions(const char* wb_id, wb_question_t** questions, size_t* count);
static int wb_load_approaches(const char* wb_id, wb_approach_t** approaches, size_t* count);
static int wb_load_votes(const char* wb_id, wb_vote_t** votes, size_t* count);

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int katra_whiteboard_init(void) {
    if (wb_initialized) {
        return KATRA_SUCCESS;
    }

    int result = KATRA_SUCCESS;
    char db_path[KATRA_PATH_MAX];

    /* Build database path */
    const char* base_path = katra_config_get("KATRA_DATA_PATH");
    if (!base_path) {
        base_path = "/tmp/katra";
    }

    result = katra_build_path(db_path, sizeof(db_path), base_path, "whiteboard.db", NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Ensure directory exists */
    result = katra_ensure_dir(base_path);
    if (result != KATRA_SUCCESS) {
        return result;
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
    int random = rand() % 10000;
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

    /* TODO: Parse goal_json, scope_json, decision_json */

    text = (const char*)sqlite3_column_text(stmt, 10);
    if (text) wb->design.content = katra_safe_strdup(text);

    text = (const char*)sqlite3_column_text(stmt, 11);
    if (text) SAFE_STRNCPY(wb->design.author, text);

    wb->design.approved = sqlite3_column_int(stmt, 12) != 0;

    text = (const char*)sqlite3_column_text(stmt, 13);
    if (text) SAFE_STRNCPY(wb->design.approved_by, text);

    wb->design.approved_at = sqlite3_column_int64(stmt, 14);

    sqlite3_finalize(stmt);

    /* Load related data */
    wb_load_questions(whiteboard_id, &wb->questions, &wb->question_count);
    wb_load_approaches(whiteboard_id, &wb->approaches, &wb->approach_count);
    wb_load_votes(whiteboard_id, &wb->votes, &wb->vote_count);

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
    char id_copy[64];
    SAFE_STRNCPY(id_copy, wb_id);
    sqlite3_finalize(stmt);

    return katra_whiteboard_get(id_copy, whiteboard_out);
}

int katra_whiteboard_list(const char* project, wb_summary_t** summaries_out, size_t* count_out) {
    if (!summaries_out || !count_out) {
        return E_INPUT_NULL;
    }

    if (!wb_initialized) {
        int result = katra_whiteboard_init();
        if (result != KATRA_SUCCESS) return result;
    }

    const char* sql;
    sqlite3_stmt* stmt = NULL;

    if (project) {
        sql = "SELECT id, project, problem, status, created_at, design_approved "
              "FROM whiteboards WHERE project = ? ORDER BY created_at DESC";
    } else {
        sql = "SELECT id, project, problem, status, created_at, design_approved "
              "FROM whiteboards ORDER BY created_at DESC";
    }

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    if (project) {
        sqlite3_bind_text(stmt, 1, project, -1, SQLITE_STATIC);
    }

    /* Count results first */
    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    sqlite3_reset(stmt);

    if (count == 0) {
        sqlite3_finalize(stmt);
        *summaries_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    /* Allocate array */
    wb_summary_t* summaries = calloc(count, sizeof(wb_summary_t));
    if (!summaries) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    /* Populate */
    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < count) {
        const char* text;

        text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) SAFE_STRNCPY(summaries[i].id, text);

        text = (const char*)sqlite3_column_text(stmt, 1);
        if (text) SAFE_STRNCPY(summaries[i].project, text);

        text = (const char*)sqlite3_column_text(stmt, 2);
        if (text) {
            strncpy(summaries[i].problem, text, sizeof(summaries[i].problem) - 1);
            summaries[i].problem[sizeof(summaries[i].problem) - 1] = '\0';
        }

        summaries[i].status = sqlite3_column_int(stmt, 3);
        summaries[i].created_at = sqlite3_column_int64(stmt, 4);
        summaries[i].design_approved = sqlite3_column_int(stmt, 5) != 0;

        i++;
    }

    sqlite3_finalize(stmt);
    *summaries_out = summaries;
    *count_out = count;
    return KATRA_SUCCESS;
}

void katra_whiteboard_free(whiteboard_t* wb) {
    if (!wb) return;

    /* Free questions */
    if (wb->questions) {
        free(wb->questions);
    }

    /* Free approaches */
    if (wb->approaches) {
        for (size_t i = 0; i < wb->approach_count; i++) {
            if (wb->approaches[i].pros) {
                for (size_t j = 0; j < wb->approaches[i].pros_count; j++) {
                    free(wb->approaches[i].pros[j]);
                }
                free(wb->approaches[i].pros);
            }
            if (wb->approaches[i].cons) {
                for (size_t j = 0; j < wb->approaches[i].cons_count; j++) {
                    free(wb->approaches[i].cons[j]);
                }
                free(wb->approaches[i].cons);
            }
            if (wb->approaches[i].supporters) {
                for (size_t j = 0; j < wb->approaches[i].supporter_count; j++) {
                    free(wb->approaches[i].supporters[j]);
                }
                free(wb->approaches[i].supporters);
            }
        }
        free(wb->approaches);
    }

    /* Free votes */
    if (wb->votes) {
        free(wb->votes);
    }

    /* Free goal criteria */
    if (wb->goal.criteria) {
        for (size_t i = 0; i < wb->goal.criteria_count; i++) {
            free(wb->goal.criteria[i]);
        }
        free(wb->goal.criteria);
    }

    /* Free scope */
    if (wb->scope.included) {
        for (size_t i = 0; i < wb->scope.included_count; i++) {
            free(wb->scope.included[i]);
        }
        free(wb->scope.included);
    }
    if (wb->scope.excluded) {
        for (size_t i = 0; i < wb->scope.excluded_count; i++) {
            free(wb->scope.excluded[i]);
        }
        free(wb->scope.excluded);
    }

    /* Free design content */
    free(wb->design.content);
    if (wb->design.reviewers) {
        for (size_t i = 0; i < wb->design.reviewer_count; i++) {
            free(wb->design.reviewers[i]);
        }
        free(wb->design.reviewers);
    }

    free(wb);
}

void katra_whiteboard_summaries_free(wb_summary_t* summaries, size_t count) {
    (void)count;
    free(summaries);
}

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
 * HELPER FUNCTIONS
 * ============================================================================ */

static int wb_load_questions(const char* wb_id, wb_question_t** questions, size_t* count) {
    const char* sql = "SELECT id, author, question, answered, answer, created_at "
                      "FROM whiteboard_questions WHERE whiteboard_id = ? ORDER BY created_at";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *questions = NULL;
        *count = 0;
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, wb_id, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) n++;
    sqlite3_reset(stmt);

    if (n == 0) {
        sqlite3_finalize(stmt);
        *questions = NULL;
        *count = 0;
        return KATRA_SUCCESS;
    }

    wb_question_t* q = calloc(n, sizeof(wb_question_t));
    if (!q) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        const char* text;

        text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) SAFE_STRNCPY(q[i].id, text);

        text = (const char*)sqlite3_column_text(stmt, 1);
        if (text) SAFE_STRNCPY(q[i].author, text);

        text = (const char*)sqlite3_column_text(stmt, 2);
        if (text) SAFE_STRNCPY(q[i].text, text);

        q[i].answered = sqlite3_column_int(stmt, 3) != 0;

        text = (const char*)sqlite3_column_text(stmt, 4);
        if (text) SAFE_STRNCPY(q[i].answer, text);

        q[i].created_at = sqlite3_column_int64(stmt, 5);
        i++;
    }

    sqlite3_finalize(stmt);
    *questions = q;
    *count = n;
    return KATRA_SUCCESS;
}

static int wb_load_approaches(const char* wb_id, wb_approach_t** approaches, size_t* count) {
    const char* sql = "SELECT id, author, title, description, pros_json, cons_json, created_at "
                      "FROM whiteboard_approaches WHERE whiteboard_id = ? ORDER BY created_at";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *approaches = NULL;
        *count = 0;
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, wb_id, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) n++;
    sqlite3_reset(stmt);

    if (n == 0) {
        sqlite3_finalize(stmt);
        *approaches = NULL;
        *count = 0;
        return KATRA_SUCCESS;
    }

    wb_approach_t* a = calloc(n, sizeof(wb_approach_t));
    if (!a) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        const char* text;

        text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) SAFE_STRNCPY(a[i].id, text);

        text = (const char*)sqlite3_column_text(stmt, 1);
        if (text) SAFE_STRNCPY(a[i].author, text);

        text = (const char*)sqlite3_column_text(stmt, 2);
        if (text) SAFE_STRNCPY(a[i].title, text);

        text = (const char*)sqlite3_column_text(stmt, 3);
        if (text) SAFE_STRNCPY(a[i].description, text);

        /* TODO: Parse pros_json and cons_json */

        a[i].created_at = sqlite3_column_int64(stmt, 6);
        i++;
    }

    sqlite3_finalize(stmt);
    *approaches = a;
    *count = n;
    return KATRA_SUCCESS;
}

static int wb_load_votes(const char* wb_id, wb_vote_t** votes, size_t* count) {
    const char* sql = "SELECT id, approach_id, voter, position, reasoning, created_at "
                      "FROM whiteboard_votes WHERE whiteboard_id = ? ORDER BY created_at";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(wb_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        *votes = NULL;
        *count = 0;
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, wb_id, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) n++;
    sqlite3_reset(stmt);

    if (n == 0) {
        sqlite3_finalize(stmt);
        *votes = NULL;
        *count = 0;
        return KATRA_SUCCESS;
    }

    wb_vote_t* v = calloc(n, sizeof(wb_vote_t));
    if (!v) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        const char* text;

        text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) SAFE_STRNCPY(v[i].id, text);

        text = (const char*)sqlite3_column_text(stmt, 1);
        if (text) SAFE_STRNCPY(v[i].approach_id, text);

        text = (const char*)sqlite3_column_text(stmt, 2);
        if (text) SAFE_STRNCPY(v[i].voter, text);

        v[i].position = sqlite3_column_int(stmt, 3);

        text = (const char*)sqlite3_column_text(stmt, 4);
        if (text) SAFE_STRNCPY(v[i].reasoning, text);

        v[i].created_at = sqlite3_column_int64(stmt, 5);
        i++;
    }

    sqlite3_finalize(stmt);
    *votes = v;
    *count = n;
    return KATRA_SUCCESS;
}
