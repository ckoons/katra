/* © 2025 Casey Koons All rights reserved */

/**
 * @file katra_mm_index.c
 * @brief SQLite persistence layer for metamemory nodes
 *
 * Provides:
 *   - Database initialization and schema creation
 *   - CRUD operations for metamemory nodes
 *   - Link management (bidirectional relationships)
 *   - Full-text search on concepts
 *   - File hash tracking for change detection
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sqlite3.h>

#include "katra_mm_index.h"
#include "katra_metamemory.h"
#include "katra_error.h"
#include "katra_limits.h"
#include "katra_log.h"

/* ============================================================================
 * Constants
 * ============================================================================ */

#define MM_INDEX_DB_NAME "metamemory.db"
#define MM_INDEX_SCHEMA_VERSION 1

/* SQL statements */
static const char* SQL_CREATE_NODES =
    "CREATE TABLE IF NOT EXISTS nodes ("
    "  id TEXT PRIMARY KEY,"
    "  type INTEGER NOT NULL,"
    "  project_id TEXT NOT NULL,"
    "  name TEXT NOT NULL,"
    "  purpose TEXT,"
    "  file_path TEXT,"
    "  line_start INTEGER,"
    "  line_end INTEGER,"
    "  column_start INTEGER,"
    "  column_end INTEGER,"
    "  signature TEXT,"
    "  return_type TEXT,"
    "  visibility INTEGER,"
    "  source_hash TEXT,"
    "  created_at INTEGER,"
    "  updated_at INTEGER,"
    "  ci_curated INTEGER DEFAULT 0,"
    "  ci_curated_at INTEGER,"
    "  ci_notes TEXT"
    ")";

static const char* SQL_CREATE_LINKS =
    "CREATE TABLE IF NOT EXISTS links ("
    "  source_id TEXT NOT NULL,"
    "  link_type TEXT NOT NULL,"
    "  target_id TEXT NOT NULL,"
    "  PRIMARY KEY (source_id, link_type, target_id),"
    "  FOREIGN KEY (source_id) REFERENCES nodes(id) ON DELETE CASCADE"
    ")";

static const char* SQL_CREATE_TASKS =
    "CREATE TABLE IF NOT EXISTS tasks ("
    "  node_id TEXT NOT NULL,"
    "  task TEXT NOT NULL,"
    "  PRIMARY KEY (node_id, task),"
    "  FOREIGN KEY (node_id) REFERENCES nodes(id) ON DELETE CASCADE"
    ")";

static const char* SQL_CREATE_PARAMS =
    "CREATE TABLE IF NOT EXISTS params ("
    "  node_id TEXT NOT NULL,"
    "  param_index INTEGER NOT NULL,"
    "  name TEXT NOT NULL,"
    "  type TEXT NOT NULL,"
    "  description TEXT,"
    "  PRIMARY KEY (node_id, param_index),"
    "  FOREIGN KEY (node_id) REFERENCES nodes(id) ON DELETE CASCADE"
    ")";

static const char* SQL_CREATE_FIELDS =
    "CREATE TABLE IF NOT EXISTS fields ("
    "  node_id TEXT NOT NULL,"
    "  field_index INTEGER NOT NULL,"
    "  name TEXT NOT NULL,"
    "  type TEXT NOT NULL,"
    "  PRIMARY KEY (node_id, field_index),"
    "  FOREIGN KEY (node_id) REFERENCES nodes(id) ON DELETE CASCADE"
    ")";

static const char* SQL_CREATE_FTS =
    "CREATE VIRTUAL TABLE IF NOT EXISTS nodes_fts USING fts5("
    "  name, purpose, tasks,"
    "  content=nodes,"
    "  content_rowid=rowid"
    ")";

static const char* SQL_CREATE_FILE_HASHES =
    "CREATE TABLE IF NOT EXISTS file_hashes ("
    "  file_path TEXT PRIMARY KEY,"
    "  hash TEXT NOT NULL,"
    "  indexed_at INTEGER NOT NULL"
    ")";

static const char* SQL_CREATE_PROJECT_META =
    "CREATE TABLE IF NOT EXISTS project_meta ("
    "  key TEXT PRIMARY KEY,"
    "  value TEXT"
    ")";

static const char* SQL_CREATE_INDEXES =
    "CREATE INDEX IF NOT EXISTS idx_nodes_project ON nodes(project_id);"
    "CREATE INDEX IF NOT EXISTS idx_nodes_type ON nodes(type);"
    "CREATE INDEX IF NOT EXISTS idx_nodes_file ON nodes(file_path);"
    "CREATE INDEX IF NOT EXISTS idx_links_source ON links(source_id);"
    "CREATE INDEX IF NOT EXISTS idx_links_target ON links(target_id);"
    "CREATE INDEX IF NOT EXISTS idx_links_type ON links(link_type);";

/* ============================================================================
 * Module State
 * ============================================================================ */

/* Database connection per project (simple single-project for now) */
static sqlite3* g_db = NULL;
static char g_current_project[KATRA_BUFFER_MEDIUM] = {0};
static bool g_index_initialized = false;

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

/**
 * Build database path for a project.
 */
static int build_db_path(const char* project_id, char* buffer, size_t size)
{
    const char* home = getenv("HOME");
    if (!home) {
        return E_SYSTEM_FILE;
    }

    int written = snprintf(buffer, size, "%s/.katra/softdev/%s/%s",
                          home, project_id, MM_INDEX_DB_NAME);
    if (written < 0 || (size_t)written >= size) {
        return E_BUFFER_OVERFLOW;
    }

    return KATRA_SUCCESS;
}

/**
 * Ensure directory exists for database.
 */
static int ensure_db_directory(const char* project_id)
{
    const char* home = getenv("HOME");
    if (!home) {
        return E_SYSTEM_FILE;
    }

    char path[KATRA_PATH_MAX];
    int result = KATRA_SUCCESS;

    /* Create ~/.katra */
    snprintf(path, sizeof(path), "%s/.katra", home);
    if (mkdir(path, KATRA_DIR_PERMISSIONS) != 0 && errno != EEXIST) {
        return E_SYSTEM_FILE;
    }

    /* Create ~/.katra/softdev */
    snprintf(path, sizeof(path), "%s/.katra/softdev", home);
    if (mkdir(path, KATRA_DIR_PERMISSIONS) != 0 && errno != EEXIST) {
        return E_SYSTEM_FILE;
    }

    /* Create ~/.katra/softdev/<project_id> */
    snprintf(path, sizeof(path), "%s/.katra/softdev/%s", home, project_id);
    if (mkdir(path, KATRA_DIR_PERMISSIONS) != 0 && errno != EEXIST) {
        return E_SYSTEM_FILE;
    }

    return result;
}

/**
 * Execute a SQL statement (no results expected).
 */
static int exec_sql(const char* sql)
{
    if (!g_db) {
        return E_INVALID_STATE;
    }

    char* errmsg = NULL;
    int rc = sqlite3_exec(g_db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("SQL error: %s", errmsg ? errmsg : "unknown");
        sqlite3_free(errmsg);
        return E_SYSTEM_FILE;
    }

    return KATRA_SUCCESS;
}

/**
 * Serialize node links to database.
 */
static int store_node_links(const metamemory_node_t* node)
{
    sqlite3_stmt* stmt = NULL;
    int result = KATRA_SUCCESS;

    const char* sql = "INSERT OR REPLACE INTO links (source_id, link_type, target_id) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    /* Helper macro to insert links */
    #define INSERT_LINKS(array, count, link_type) \
        for (size_t i = 0; i < (count); i++) { \
            sqlite3_reset(stmt); \
            sqlite3_bind_text(stmt, 1, node->id, -1, SQLITE_STATIC); \
            sqlite3_bind_text(stmt, 2, (link_type), -1, SQLITE_STATIC); \
            sqlite3_bind_text(stmt, 3, (array)[i], -1, SQLITE_STATIC); \
            if (sqlite3_step(stmt) != SQLITE_DONE) { \
                result = E_SYSTEM_FILE; \
                goto cleanup; \
            } \
        }

    INSERT_LINKS(node->parent_concepts, node->parent_concept_count, "parent_concept");
    INSERT_LINKS(node->child_concepts, node->child_concept_count, "child_concept");
    INSERT_LINKS(node->implements, node->implements_count, "implements");
    INSERT_LINKS(node->implemented_by, node->implemented_by_count, "implemented_by");
    INSERT_LINKS(node->calls, node->calls_count, "calls");
    INSERT_LINKS(node->called_by, node->called_by_count, "called_by");
    INSERT_LINKS(node->uses_types, node->uses_types_count, "uses_types");
    INSERT_LINKS(node->used_by, node->used_by_count, "used_by");
    INSERT_LINKS(node->includes, node->includes_count, "includes");
    INSERT_LINKS(node->included_by, node->included_by_count, "included_by");
    INSERT_LINKS(node->related, node->related_count, "related");

    #undef INSERT_LINKS

cleanup:
    sqlite3_finalize(stmt);
    return result;
}

/**
 * Store node tasks.
 */
static int store_node_tasks(const metamemory_node_t* node)
{
    if (node->task_count == 0) {
        return KATRA_SUCCESS;
    }

    sqlite3_stmt* stmt = NULL;
    int result = KATRA_SUCCESS;

    const char* sql = "INSERT OR REPLACE INTO tasks (node_id, task) VALUES (?, ?)";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    for (size_t i = 0; i < node->task_count; i++) {
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, node->id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, node->typical_tasks[i], -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            result = E_SYSTEM_FILE;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

/**
 * Store function parameters.
 */
static int store_node_params(const metamemory_node_t* node)
{
    if (node->param_count == 0) {
        return KATRA_SUCCESS;
    }

    sqlite3_stmt* stmt = NULL;
    int result = KATRA_SUCCESS;

    const char* sql = "INSERT OR REPLACE INTO params (node_id, param_index, name, type, description) "
                      "VALUES (?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    for (size_t i = 0; i < node->param_count; i++) {
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, node->id, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, (int)i);
        sqlite3_bind_text(stmt, 3, node->parameters[i].name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, node->parameters[i].type, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, node->parameters[i].description, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            result = E_SYSTEM_FILE;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

/**
 * Store struct fields.
 */
static int store_node_fields(const metamemory_node_t* node)
{
    if (node->field_count == 0) {
        return KATRA_SUCCESS;
    }

    sqlite3_stmt* stmt = NULL;
    int result = KATRA_SUCCESS;

    const char* sql = "INSERT OR REPLACE INTO fields (node_id, field_index, name, type) "
                      "VALUES (?, ?, ?, ?)";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    for (size_t i = 0; i < node->field_count; i++) {
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, node->id, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, (int)i);
        sqlite3_bind_text(stmt, 3, node->field_names[i], -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, node->field_types[i], -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            result = E_SYSTEM_FILE;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

/**
 * Load links for a node.
 */
static int load_node_links(metamemory_node_t* node)
{
    sqlite3_stmt* stmt = NULL;
    int result = KATRA_SUCCESS;

    const char* sql = "SELECT link_type, target_id FROM links WHERE source_id = ?";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, node->id, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* link_type = (const char*)sqlite3_column_text(stmt, 0);
        const char* target_id = (const char*)sqlite3_column_text(stmt, 1);

        if (link_type && target_id) {
            metamemory_add_link(node, link_type, target_id);
        }
    }

    sqlite3_finalize(stmt);
    return result;
}

/**
 * Load tasks for a node.
 */
static int load_node_tasks(metamemory_node_t* node)
{
    sqlite3_stmt* stmt = NULL;

    const char* sql = "SELECT task FROM tasks WHERE node_id = ?";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, node->id, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* task = (const char*)sqlite3_column_text(stmt, 0);
        if (task) {
            metamemory_add_task(node, task);
        }
    }

    sqlite3_finalize(stmt);
    return KATRA_SUCCESS;
}

/**
 * Load parameters for a function node.
 */
static int load_node_params(metamemory_node_t* node)
{
    sqlite3_stmt* stmt = NULL;

    const char* sql = "SELECT name, type, description FROM params WHERE node_id = ? ORDER BY param_index";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, node->id, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = (const char*)sqlite3_column_text(stmt, 0);
        const char* type = (const char*)sqlite3_column_text(stmt, 1);
        const char* desc = (const char*)sqlite3_column_text(stmt, 2);
        if (name && type) {
            metamemory_add_parameter(node, name, type, desc);
        }
    }

    sqlite3_finalize(stmt);
    return KATRA_SUCCESS;
}

/**
 * Load fields for a struct node.
 */
static int load_node_fields(metamemory_node_t* node)
{
    sqlite3_stmt* stmt = NULL;

    const char* sql = "SELECT name, type FROM fields WHERE node_id = ? ORDER BY field_index";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, node->id, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = (const char*)sqlite3_column_text(stmt, 0);
        const char* type = (const char*)sqlite3_column_text(stmt, 1);
        if (name && type) {
            metamemory_add_field(node, name, type);
        }
    }

    sqlite3_finalize(stmt);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * Public API
 * ============================================================================ */

int mm_index_init(const char* project_id)
{
    int result = KATRA_SUCCESS;
    char db_path[KATRA_PATH_MAX];

    if (!project_id) {
        katra_report_error(E_INPUT_NULL, "mm_index_init", "project_id is NULL");
        return E_INPUT_NULL;
    }

    /* Close existing connection if different project */
    if (g_db && strcmp(g_current_project, project_id) != 0) {
        mm_index_close();
    }

    if (g_db) {
        return KATRA_SUCCESS;  /* Already initialized for this project */
    }

    /* Ensure directory exists */
    result = ensure_db_directory(project_id);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "mm_index_init", "Failed to create database directory");
        return result;
    }

    /* Build path and open database */
    result = build_db_path(project_id, db_path, sizeof(db_path));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    if (sqlite3_open(db_path, &g_db) != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "mm_index_init", "Failed to open database");
        g_db = NULL;
        return E_SYSTEM_FILE;
    }

    /* Enable foreign keys */
    exec_sql("PRAGMA foreign_keys = ON");

    /* Create schema */
    if ((result = exec_sql(SQL_CREATE_NODES)) != KATRA_SUCCESS) goto cleanup;
    if ((result = exec_sql(SQL_CREATE_LINKS)) != KATRA_SUCCESS) goto cleanup;
    if ((result = exec_sql(SQL_CREATE_TASKS)) != KATRA_SUCCESS) goto cleanup;
    if ((result = exec_sql(SQL_CREATE_PARAMS)) != KATRA_SUCCESS) goto cleanup;
    if ((result = exec_sql(SQL_CREATE_FIELDS)) != KATRA_SUCCESS) goto cleanup;
    if ((result = exec_sql(SQL_CREATE_FILE_HASHES)) != KATRA_SUCCESS) goto cleanup;
    if ((result = exec_sql(SQL_CREATE_PROJECT_META)) != KATRA_SUCCESS) goto cleanup;
    if ((result = exec_sql(SQL_CREATE_INDEXES)) != KATRA_SUCCESS) goto cleanup;

    /* FTS table - separate error handling as it may fail on some SQLite builds */
    exec_sql(SQL_CREATE_FTS);

    strncpy(g_current_project, project_id, sizeof(g_current_project) - 1);
    g_current_project[sizeof(g_current_project) - 1] = '\0';
    g_index_initialized = true;

    LOG_INFO("Metamemory index initialized for project: %s", project_id);
    return KATRA_SUCCESS;

cleanup:
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
    return result;
}

void mm_index_close(void)
{
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
    g_current_project[0] = '\0';
    g_index_initialized = false;
}

bool mm_index_is_initialized(void)
{
    return g_index_initialized && g_db != NULL;
}

int mm_index_store_node(const metamemory_node_t* node)
{
    int result = KATRA_SUCCESS;
    sqlite3_stmt* stmt = NULL;

    if (!node || !node->id) {
        katra_report_error(E_INPUT_NULL, "mm_index_store_node", "node or id is NULL");
        return E_INPUT_NULL;
    }

    if (!g_db) {
        katra_report_error(E_INVALID_STATE, "mm_index_store_node", "Index not initialized");
        return E_INVALID_STATE;
    }

    const char* sql =
        "INSERT OR REPLACE INTO nodes ("
        "  id, type, project_id, name, purpose, file_path,"
        "  line_start, line_end, column_start, column_end,"
        "  signature, return_type, visibility, source_hash,"
        "  created_at, updated_at, ci_curated, ci_curated_at, ci_notes"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "mm_index_store_node", "Failed to prepare statement");
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, node->id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, node->type);
    sqlite3_bind_text(stmt, 3, node->project_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, node->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, node->purpose, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, node->location.file_path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, node->location.line_start);
    sqlite3_bind_int(stmt, 8, node->location.line_end);
    sqlite3_bind_int(stmt, 9, node->location.column_start);
    sqlite3_bind_int(stmt, 10, node->location.column_end);
    sqlite3_bind_text(stmt, 11, node->signature, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, node->return_type, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 13, node->visibility);
    sqlite3_bind_text(stmt, 14, node->source_hash, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 15, node->created_at);
    sqlite3_bind_int64(stmt, 16, node->updated_at);
    sqlite3_bind_int(stmt, 17, node->ci_curated ? 1 : 0);
    sqlite3_bind_int64(stmt, 18, node->ci_curated_at);
    sqlite3_bind_text(stmt, 19, node->ci_notes, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        katra_report_error(E_SYSTEM_FILE, "mm_index_store_node", "Failed to insert node");
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    /* Store related data */
    if ((result = store_node_links(node)) != KATRA_SUCCESS) goto cleanup;
    if ((result = store_node_tasks(node)) != KATRA_SUCCESS) goto cleanup;
    if ((result = store_node_params(node)) != KATRA_SUCCESS) goto cleanup;
    if ((result = store_node_fields(node)) != KATRA_SUCCESS) goto cleanup;

    return KATRA_SUCCESS;

cleanup:
    if (stmt) sqlite3_finalize(stmt);
    return result;
}

int mm_index_load_node(const char* node_id, metamemory_node_t** out)
{
    sqlite3_stmt* stmt = NULL;
    metamemory_node_t* node = NULL;

    if (!node_id || !out) {
        return E_INPUT_NULL;
    }

    *out = NULL;

    if (!g_db) {
        return E_INVALID_STATE;
    }

    const char* sql =
        "SELECT type, project_id, name, purpose, file_path,"
        "  line_start, line_end, column_start, column_end,"
        "  signature, return_type, visibility, source_hash,"
        "  created_at, updated_at, ci_curated, ci_curated_at, ci_notes "
        "FROM nodes WHERE id = ?";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, node_id, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return E_NOT_FOUND;
    }

    /* Create node from row */
    metamemory_type_t type = sqlite3_column_int(stmt, 0);
    const char* project_id = (const char*)sqlite3_column_text(stmt, 1);
    const char* name = (const char*)sqlite3_column_text(stmt, 2);

    node = metamemory_create_node(type, project_id, name);
    if (!node) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    /* Populate node fields */
    if (sqlite3_column_text(stmt, 3)) {
        metamemory_set_purpose(node, (const char*)sqlite3_column_text(stmt, 3));
    }

    if (sqlite3_column_text(stmt, 4)) {
        free(node->location.file_path);
        node->location.file_path = strdup((const char*)sqlite3_column_text(stmt, 4));
    }
    node->location.line_start = sqlite3_column_int(stmt, 5);
    node->location.line_end = sqlite3_column_int(stmt, 6);
    node->location.column_start = sqlite3_column_int(stmt, 7);
    node->location.column_end = sqlite3_column_int(stmt, 8);

    if (sqlite3_column_text(stmt, 9)) {
        node->signature = strdup((const char*)sqlite3_column_text(stmt, 9));
    }
    if (sqlite3_column_text(stmt, 10)) {
        node->return_type = strdup((const char*)sqlite3_column_text(stmt, 10));
    }
    node->visibility = sqlite3_column_int(stmt, 11);

    if (sqlite3_column_text(stmt, 12)) {
        node->source_hash = strdup((const char*)sqlite3_column_text(stmt, 12));
    }

    node->created_at = sqlite3_column_int64(stmt, 13);
    node->updated_at = sqlite3_column_int64(stmt, 14);
    node->ci_curated = sqlite3_column_int(stmt, 15) != 0;
    node->ci_curated_at = sqlite3_column_int64(stmt, 16);

    if (sqlite3_column_text(stmt, 17)) {
        metamemory_set_ci_notes(node, (const char*)sqlite3_column_text(stmt, 17));
    }

    sqlite3_finalize(stmt);

    /* Load related data */
    load_node_links(node);
    load_node_tasks(node);
    load_node_params(node);
    load_node_fields(node);

    *out = node;
    return KATRA_SUCCESS;
}

int mm_index_delete_node(const char* node_id)
{
    if (!node_id) {
        return E_INPUT_NULL;
    }

    if (!g_db) {
        return E_INVALID_STATE;
    }

    /* Foreign key cascade will clean up links, tasks, params, fields */
    sqlite3_stmt* stmt = NULL;
    const char* sql = "DELETE FROM nodes WHERE id = ?";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, node_id, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return E_SYSTEM_FILE;
    }

    return KATRA_SUCCESS;
}

int mm_index_search_concepts(const char* query, metamemory_node_t*** results, size_t* count)
{
    int result = KATRA_SUCCESS;
    sqlite3_stmt* stmt = NULL;
    metamemory_node_t** nodes = NULL;
    size_t capacity = 16;
    size_t node_count = 0;

    if (!query || !results || !count) {
        return E_INPUT_NULL;
    }

    *results = NULL;
    *count = 0;

    if (!g_db) {
        return E_INVALID_STATE;
    }

    nodes = calloc(capacity, sizeof(metamemory_node_t*));
    if (!nodes) {
        return E_SYSTEM_MEMORY;
    }

    /* Search concepts by name, purpose, or tasks */
    const char* sql =
        "SELECT id FROM nodes WHERE type = ? AND "
        "(name LIKE ? OR purpose LIKE ?) "
        "ORDER BY name LIMIT 50";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(nodes);
        return E_SYSTEM_FILE;
    }

    char pattern[KATRA_BUFFER_MEDIUM];
    snprintf(pattern, sizeof(pattern), "%%%s%%", query);

    sqlite3_bind_int(stmt, 1, METAMEMORY_TYPE_CONCEPT);
    sqlite3_bind_text(stmt, 2, pattern, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, pattern, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* node_id = (const char*)sqlite3_column_text(stmt, 0);
        if (!node_id) continue;

        metamemory_node_t* node = NULL;
        if (mm_index_load_node(node_id, &node) == KATRA_SUCCESS && node) {
            if (node_count >= capacity) {
                capacity *= 2;
                metamemory_node_t** new_nodes = realloc(nodes, capacity * sizeof(metamemory_node_t*));
                if (!new_nodes) {
                    metamemory_free_node(node);
                    result = E_SYSTEM_MEMORY;
                    goto cleanup;
                }
                nodes = new_nodes;
            }
            nodes[node_count++] = node;
        }
    }

    sqlite3_finalize(stmt);

    *results = nodes;
    *count = node_count;
    return KATRA_SUCCESS;

cleanup:
    for (size_t i = 0; i < node_count; i++) {
        metamemory_free_node(nodes[i]);
    }
    free(nodes);
    if (stmt) sqlite3_finalize(stmt);
    return result;
}

int mm_index_search_code(const char* query, const metamemory_type_t* types,
                         size_t type_count, metamemory_node_t*** results, size_t* count)
{
    int result = KATRA_SUCCESS;
    sqlite3_stmt* stmt = NULL;
    metamemory_node_t** nodes = NULL;
    size_t capacity = 32;
    size_t node_count = 0;

    if (!query || !results || !count) {
        return E_INPUT_NULL;
    }

    *results = NULL;
    *count = 0;

    if (!g_db) {
        return E_INVALID_STATE;
    }

    nodes = calloc(capacity, sizeof(metamemory_node_t*));
    if (!nodes) {
        return E_SYSTEM_MEMORY;
    }

    /* Build type filter if provided */
    char type_filter[KATRA_BUFFER_MEDIUM] = "";
    if (types && type_count > 0) {
        char* p = type_filter;
        size_t remaining = sizeof(type_filter);
        int written = snprintf(p, remaining, " AND type IN (");
        p += written;
        remaining -= written;

        for (size_t i = 0; i < type_count; i++) {
            written = snprintf(p, remaining, "%s%d", i > 0 ? "," : "", types[i]);
            p += written;
            remaining -= written;
        }
        snprintf(p, remaining, ")");
    } else {
        /* Default: search functions and structs */
        snprintf(type_filter, sizeof(type_filter), " AND type IN (%d, %d)",
                METAMEMORY_TYPE_FUNCTION, METAMEMORY_TYPE_STRUCT);
    }

    char sql[KATRA_BUFFER_LARGE];
    snprintf(sql, sizeof(sql),
        "SELECT id FROM nodes WHERE "
        "(name LIKE ? OR signature LIKE ?) %s "
        "ORDER BY name LIMIT 100", type_filter);

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(nodes);
        return E_SYSTEM_FILE;
    }

    char pattern[KATRA_BUFFER_MEDIUM];
    snprintf(pattern, sizeof(pattern), "%%%s%%", query);

    sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pattern, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* node_id = (const char*)sqlite3_column_text(stmt, 0);
        if (!node_id) continue;

        metamemory_node_t* node = NULL;
        if (mm_index_load_node(node_id, &node) == KATRA_SUCCESS && node) {
            if (node_count >= capacity) {
                capacity *= 2;
                metamemory_node_t** new_nodes = realloc(nodes, capacity * sizeof(metamemory_node_t*));
                if (!new_nodes) {
                    metamemory_free_node(node);
                    result = E_SYSTEM_MEMORY;
                    goto cleanup;
                }
                nodes = new_nodes;
            }
            nodes[node_count++] = node;
        }
    }

    sqlite3_finalize(stmt);

    *results = nodes;
    *count = node_count;
    return KATRA_SUCCESS;

cleanup:
    for (size_t i = 0; i < node_count; i++) {
        metamemory_free_node(nodes[i]);
    }
    free(nodes);
    if (stmt) sqlite3_finalize(stmt);
    return result;
}

int mm_index_get_links(const char* node_id, const char* link_type,
                       char*** target_ids, size_t* count)
{
    sqlite3_stmt* stmt = NULL;
    char** ids = NULL;
    size_t capacity = 16;
    size_t id_count = 0;

    if (!node_id || !target_ids || !count) {
        return E_INPUT_NULL;
    }

    *target_ids = NULL;
    *count = 0;

    if (!g_db) {
        return E_INVALID_STATE;
    }

    ids = calloc(capacity, sizeof(char*));
    if (!ids) {
        return E_SYSTEM_MEMORY;
    }

    const char* sql = link_type
        ? "SELECT target_id FROM links WHERE source_id = ? AND link_type = ?"
        : "SELECT target_id FROM links WHERE source_id = ?";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(ids);
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, node_id, -1, SQLITE_STATIC);
    if (link_type) {
        sqlite3_bind_text(stmt, 2, link_type, -1, SQLITE_STATIC);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* target = (const char*)sqlite3_column_text(stmt, 0);
        if (!target) continue;

        if (id_count >= capacity) {
            capacity *= 2;
            char** new_ids = realloc(ids, capacity * sizeof(char*));
            if (!new_ids) {
                goto cleanup;
            }
            ids = new_ids;
        }

        ids[id_count] = strdup(target);
        if (!ids[id_count]) {
            goto cleanup;
        }
        id_count++;
    }

    sqlite3_finalize(stmt);

    *target_ids = ids;
    *count = id_count;
    return KATRA_SUCCESS;

cleanup:
    for (size_t i = 0; i < id_count; i++) {
        free(ids[i]);
    }
    free(ids);
    if (stmt) sqlite3_finalize(stmt);
    return E_SYSTEM_MEMORY;
}

int mm_index_store_file_hash(const char* file_path, const char* hash)
{
    if (!file_path || !hash) {
        return E_INPUT_NULL;
    }

    if (!g_db) {
        return E_INVALID_STATE;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = "INSERT OR REPLACE INTO file_hashes (file_path, hash, indexed_at) VALUES (?, ?, ?)";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hash, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, time(NULL));

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? KATRA_SUCCESS : E_SYSTEM_FILE;
}

int mm_index_get_file_hash(const char* file_path, char* hash, size_t hash_size)
{
    if (!file_path || !hash || hash_size == 0) {
        return E_INPUT_NULL;
    }

    if (!g_db) {
        return E_INVALID_STATE;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = "SELECT hash FROM file_hashes WHERE file_path = ?";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_STATIC);

    int result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* stored_hash = (const char*)sqlite3_column_text(stmt, 0);
        if (stored_hash) {
            strncpy(hash, stored_hash, hash_size - 1);
            hash[hash_size - 1] = '\0';
            result = KATRA_SUCCESS;
        } else {
            result = E_NOT_FOUND;
        }
    } else {
        result = E_NOT_FOUND;
    }

    sqlite3_finalize(stmt);
    return result;
}

int mm_index_get_stats(mm_index_stats_t* stats)
{
    if (!stats) {
        return E_INPUT_NULL;
    }

    memset(stats, 0, sizeof(*stats));

    if (!g_db) {
        return E_INVALID_STATE;
    }

    sqlite3_stmt* stmt = NULL;

    /* Count nodes by type */
    const char* sql = "SELECT type, COUNT(*) FROM nodes GROUP BY type";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int type = sqlite3_column_int(stmt, 0);
            size_t count = (size_t)sqlite3_column_int(stmt, 1);

            switch (type) {
                case METAMEMORY_TYPE_CONCEPT:
                    stats->concept_count = count;
                    break;
                case METAMEMORY_TYPE_DIRECTORY:
                case METAMEMORY_TYPE_FILE:
                    stats->component_count += count;
                    break;
                case METAMEMORY_TYPE_FUNCTION:
                    stats->function_count = count;
                    break;
                case METAMEMORY_TYPE_STRUCT:
                    stats->struct_count = count;
                    break;
                default:
                    break;
            }
            stats->total_nodes += count;
        }
        sqlite3_finalize(stmt);
    }

    /* Count links */
    sql = "SELECT COUNT(*) FROM links";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats->link_count = (size_t)sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    /* Count indexed files */
    sql = "SELECT COUNT(*) FROM file_hashes";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats->file_count = (size_t)sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return KATRA_SUCCESS;
}

int mm_index_delete_by_file(const char* file_path)
{
    if (!file_path) {
        return E_INPUT_NULL;
    }

    if (!g_db) {
        return E_INVALID_STATE;
    }

    sqlite3_stmt* stmt = NULL;
    const char* sql = "DELETE FROM nodes WHERE file_path = ?";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    /* Also delete file hash */
    sql = "DELETE FROM file_hashes WHERE file_path = ?";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    return (rc == SQLITE_DONE) ? KATRA_SUCCESS : E_SYSTEM_FILE;
}
