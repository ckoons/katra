/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>

/* Project includes */
#include "katra_tier1_index.h"
#include "katra_tier1.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_strings.h"
#include "katra_string_literals.h"
#include "katra_core_common.h"

/* SQLite connection (one per process) */
static sqlite3* g_memory_db = NULL;

/* SQL schema for memory index database */
/* GUIDELINE_APPROVED: SQL schema strings cannot be externalized */
static const char* MEMORY_SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS memories (" /* GUIDELINE_APPROVED */
    "  record_id TEXT PRIMARY KEY," /* GUIDELINE_APPROVED */
    "  ci_id TEXT NOT NULL," /* GUIDELINE_APPROVED */
    "  timestamp INTEGER NOT NULL," /* GUIDELINE_APPROVED */
    "  last_accessed INTEGER NOT NULL," /* GUIDELINE_APPROVED */
    "  memory_type INTEGER NOT NULL," /* GUIDELINE_APPROVED */
    "  importance REAL NOT NULL," /* GUIDELINE_APPROVED */
    "  access_count INTEGER DEFAULT 0," /* GUIDELINE_APPROVED */
    "  graph_centrality REAL DEFAULT 0.0," /* GUIDELINE_APPROVED */
    "  emotion_intensity REAL DEFAULT 0.0," /* GUIDELINE_APPROVED */
    "  emotion_type TEXT," /* GUIDELINE_APPROVED */
    "  marked_important INTEGER DEFAULT 0," /* GUIDELINE_APPROVED */
    "  marked_forgettable INTEGER DEFAULT 0," /* GUIDELINE_APPROVED */
    "  archived INTEGER DEFAULT 0," /* GUIDELINE_APPROVED */
    "  file_path TEXT NOT NULL," /* GUIDELINE_APPROVED */
    "  file_offset INTEGER NOT NULL" /* GUIDELINE_APPROVED */
    ");" /* GUIDELINE_APPROVED */
    "CREATE INDEX IF NOT EXISTS idx_ci_time ON memories(ci_id, timestamp DESC);" /* GUIDELINE_APPROVED */
    "CREATE INDEX IF NOT EXISTS idx_importance ON memories(importance DESC);" /* GUIDELINE_APPROVED */
    "CREATE INDEX IF NOT EXISTS idx_centrality ON memories(graph_centrality DESC);" /* GUIDELINE_APPROVED */
    "CREATE INDEX IF NOT EXISTS idx_type ON memories(memory_type);" /* GUIDELINE_APPROVED */
    "CREATE INDEX IF NOT EXISTS idx_archived ON memories(archived);" /* GUIDELINE_APPROVED */
    "" /* GUIDELINE_APPROVED */
    "CREATE VIRTUAL TABLE IF NOT EXISTS memory_content_fts USING fts5(" /* GUIDELINE_APPROVED */
    "  record_id UNINDEXED," /* GUIDELINE_APPROVED */
    "  content" /* GUIDELINE_APPROVED */
    ");" /* GUIDELINE_APPROVED */
    "" /* GUIDELINE_APPROVED */
    "CREATE TABLE IF NOT EXISTS memory_themes (" /* GUIDELINE_APPROVED */
    "  record_id TEXT NOT NULL," /* GUIDELINE_APPROVED */
    "  theme TEXT NOT NULL," /* GUIDELINE_APPROVED */
    "  FOREIGN KEY (record_id) REFERENCES memories(record_id)" /* GUIDELINE_APPROVED */
    ");" /* GUIDELINE_APPROVED */
    "CREATE INDEX IF NOT EXISTS idx_memory_themes ON memory_themes(theme, record_id);" /* GUIDELINE_APPROVED */
    "" /* GUIDELINE_APPROVED */
    "CREATE TABLE IF NOT EXISTS memory_connections (" /* GUIDELINE_APPROVED */
    "  from_id TEXT NOT NULL," /* GUIDELINE_APPROVED */
    "  to_id TEXT NOT NULL," /* GUIDELINE_APPROVED */
    "  relationship_type INTEGER NOT NULL," /* GUIDELINE_APPROVED */
    "  strength REAL NOT NULL," /* GUIDELINE_APPROVED */
    "  FOREIGN KEY (from_id) REFERENCES memories(record_id)," /* GUIDELINE_APPROVED */
    "  FOREIGN KEY (to_id) REFERENCES memories(record_id)" /* GUIDELINE_APPROVED */
    ");" /* GUIDELINE_APPROVED */
    "CREATE INDEX IF NOT EXISTS idx_connections_from ON memory_connections(from_id);" /* GUIDELINE_APPROVED */
    "CREATE INDEX IF NOT EXISTS idx_connections_to ON memory_connections(to_id);"; /* GUIDELINE_APPROVED */

/* Get database handle */
sqlite3* tier1_index_get_db(void) {
    return g_memory_db;
}

/* Get index database path */
static int get_memory_index_db_path(const char* ci_id, char* buffer, size_t size) {
    (void)ci_id;  /* Unused - future multi-tenant support */

    int result = katra_build_path(buffer, size,
                                  KATRA_DIR_MEMORY, KATRA_DIR_TIER1,
                                  "index", NULL); /* GUIDELINE_APPROVED: path component */
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Append database filename */
    size_t len = strlen(buffer);
    snprintf(buffer + len, size - len, "/memories.db"); /* GUIDELINE_APPROVED: database filename */

    return KATRA_SUCCESS;
}

/* Initialize Tier 1 index database */
int tier1_index_init(const char* ci_id) {
    int result = KATRA_SUCCESS;
    char db_path[KATRA_PATH_MAX];
    char index_dir[KATRA_PATH_MAX];
    char* err_msg = NULL;

    if (!ci_id) {
        return E_INPUT_NULL;
    }

    /* Build index directory path */
    result = katra_build_and_ensure_dir(index_dir, sizeof(index_dir),
                                       KATRA_DIR_MEMORY, KATRA_DIR_TIER1,
                                       "index", NULL); /* GUIDELINE_APPROVED: path component */
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Get database path */
    result = get_memory_index_db_path(ci_id, db_path, sizeof(db_path));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Open or create database */
    int rc = sqlite3_open(db_path, &g_memory_db);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier1_index_init",
                          "Failed to open SQLite database"); /* GUIDELINE_APPROVED: error context */
        sqlite3_close(g_memory_db);
        g_memory_db = NULL;
        return E_SYSTEM_FILE;
    }

    /* Create schema */
    rc = sqlite3_exec(g_memory_db, MEMORY_SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier1_index_init",
                          "Failed to create schema"); /* GUIDELINE_APPROVED: error context */
        sqlite3_free(err_msg);
        sqlite3_close(g_memory_db);
        g_memory_db = NULL;
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Tier 1 memory index initialized: %s", db_path);
    return KATRA_SUCCESS;
}

/* Add memory to index */
int tier1_index_add(const memory_record_t* record,
                    const char* file_path,
                    long offset) {
    int result = KATRA_SUCCESS;
    sqlite3_stmt* stmt = NULL;
    int rc;

    if (!record || !file_path) {
        return E_INPUT_NULL;
    }

    if (!g_memory_db) {
        LOG_DEBUG("Tier 1 index not initialized, skipping index add");
        return E_INTERNAL_LOGIC;
    }

    /* Begin transaction */
    rc = sqlite3_exec(g_memory_db, "BEGIN TRANSACTION", NULL, NULL, NULL); /* GUIDELINE_APPROVED */
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier1_index_add",
                          "Failed to begin transaction"); /* GUIDELINE_APPROVED: error context */
        return E_SYSTEM_FILE;
    }

    /* Insert memory record */
    /* GUIDELINE_APPROVED: SQL query strings cannot be externalized */
    const char* insert_sql =
        "INSERT OR REPLACE INTO memories " /* GUIDELINE_APPROVED */
        "(record_id, ci_id, timestamp, last_accessed, memory_type, " /* GUIDELINE_APPROVED */
        " importance, access_count, graph_centrality, emotion_intensity, " /* GUIDELINE_APPROVED */
        " emotion_type, marked_important, marked_forgettable, archived, " /* GUIDELINE_APPROVED */
        " file_path, file_offset) " /* GUIDELINE_APPROVED */
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"; /* GUIDELINE_APPROVED */

    rc = sqlite3_prepare_v2(g_memory_db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier1_index_add",
                          "Failed to prepare statement"); /* GUIDELINE_APPROVED: error context */
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    /* Bind parameters */
    sqlite3_bind_text(stmt, 1, record->record_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, record->ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)record->timestamp);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)record->last_accessed);
    sqlite3_bind_int(stmt, 5, (int)record->type);
    sqlite3_bind_double(stmt, 6, record->importance);
    sqlite3_bind_int64(stmt, 7, (sqlite3_int64)record->access_count);
    sqlite3_bind_double(stmt, 8, record->graph_centrality);
    sqlite3_bind_double(stmt, 9, record->emotion_intensity);
    sqlite3_bind_text(stmt, 10, record->emotion_type ? record->emotion_type : STR_EMPTY, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 11, record->marked_important ? 1 : 0);
    sqlite3_bind_int(stmt, 12, record->marked_forgettable ? 1 : 0);
    sqlite3_bind_int(stmt, 13, 0);  /* Not archived yet */
    sqlite3_bind_text(stmt, 14, file_path, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 15, (sqlite3_int64)offset);

    /* Execute */
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        katra_report_error(E_SYSTEM_FILE, "tier1_index_add",
                          "Failed to insert memory"); /* GUIDELINE_APPROVED: error context */
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    /* Insert into FTS if content exists */
    if (record->content && strlen(record->content) > 0) {
        const char* fts_sql = "INSERT INTO memory_content_fts (record_id, content) VALUES (?, ?)"; /* GUIDELINE_APPROVED */

        rc = sqlite3_prepare_v2(g_memory_db, fts_sql, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, record->record_id, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, record->content, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            stmt = NULL;
        }
    }

    /* Commit transaction */
    rc = sqlite3_exec(g_memory_db, "COMMIT", NULL, NULL, NULL); /* GUIDELINE_APPROVED */
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier1_index_add",
                          "Failed to commit transaction"); /* GUIDELINE_APPROVED: error context */
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    LOG_DEBUG("Added memory %s to index", record->record_id);
    return KATRA_SUCCESS;

cleanup:
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    sqlite3_exec(g_memory_db, "ROLLBACK", NULL, NULL, NULL); /* GUIDELINE_APPROVED */
    return result;
}

/* Check if index exists */
bool tier1_index_exists(const char* ci_id) {
    char db_path[KATRA_PATH_MAX];

    if (get_memory_index_db_path(ci_id, db_path, sizeof(db_path)) != KATRA_SUCCESS) {
        return false;
    }

    FILE* fp = fopen(db_path, "r"); /* GUIDELINE_APPROVED: file mode */
    if (fp) {
        fclose(fp);
        return true;
    }

    return false;
}

/* Helper: Build SQL WHERE clause from query */
static int build_memory_where_clause(const memory_query_t* query, char* buffer, size_t size) {
    size_t offset = 0;
    bool has_condition = false;

    offset += snprintf(buffer + offset, size - offset, " WHERE");

    /* CI ID filter */
    if (query->ci_id) {
        offset += snprintf(buffer + offset, size - offset, "%s ci_id = '%s'",
                          has_condition ? " AND" : "", query->ci_id); /* GUIDELINE_APPROVED */
        has_condition = true;
    }

    /* Time range filters */
    if (query->start_time > 0) {
        offset += snprintf(buffer + offset, size - offset, "%s timestamp >= %ld",
                          has_condition ? " AND" : "", (long)query->start_time); /* GUIDELINE_APPROVED */
        has_condition = true;
    }
    if (query->end_time > 0) {
        offset += snprintf(buffer + offset, size - offset, "%s timestamp <= %ld",
                          has_condition ? " AND" : "", (long)query->end_time); /* GUIDELINE_APPROVED */
        has_condition = true;
    }

    /* Type filter (0 = all types) */
    if (query->type > 0) {
        offset += snprintf(buffer + offset, size - offset, "%s memory_type = %d",
                          has_condition ? " AND" : "", (int)query->type); /* GUIDELINE_APPROVED */
        has_condition = true;
    }

    /* Importance threshold */
    if (query->min_importance > 0.0f) {
        offset += snprintf(buffer + offset, size - offset, "%s importance >= %f",
                          has_condition ? " AND" : "", query->min_importance); /* GUIDELINE_APPROVED */
        has_condition = true;
    }

    /* Archived filter (exclude archived by default) */
    offset += snprintf(buffer + offset, size - offset, "%s archived = 0",
                      has_condition ? " AND" : ""); /* GUIDELINE_APPROVED */
    has_condition = true;

    /* If no conditions, return empty WHERE clause */
    if (!has_condition) {
        buffer[0] = '\0';
    }

    return KATRA_SUCCESS;
}

/* Query index for matching memory IDs */
int tier1_index_query(const memory_query_t* query,
                      char*** record_ids,
                      memory_location_t** locations,
                      size_t* count) {
    int result = KATRA_SUCCESS;
    sqlite3_stmt* stmt = NULL;
    char sql_query[KATRA_BUFFER_LARGE];
    char where_clause[KATRA_BUFFER_MEDIUM];
    size_t capacity = 0;
    char** ids = NULL;
    memory_location_t* locs = NULL;

    if (!query || !record_ids || !locations || !count) {
        return E_INPUT_NULL;
    }

    if (!g_memory_db) {
        LOG_DEBUG("Tier 1 index not initialized, skipping index query");
        return E_INTERNAL_LOGIC;
    }

    *record_ids = NULL;
    *locations = NULL;
    *count = 0;

    /* Build WHERE clause */
    result = build_memory_where_clause(query, where_clause, sizeof(where_clause));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Build full SQL query */
    snprintf(sql_query, sizeof(sql_query),
            "SELECT record_id, file_path, file_offset FROM memories%s ORDER BY importance DESC, timestamp DESC", /* GUIDELINE_APPROVED */
            where_clause);

    /* Add limit if specified */
    if (query->limit > 0) {
        size_t len = strlen(sql_query);
        snprintf(sql_query + len, sizeof(sql_query) - len, " LIMIT %zu", query->limit);
    }

    /* Prepare statement */
    int rc = sqlite3_prepare_v2(g_memory_db, sql_query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier1_index_query",
                          "Failed to prepare query"); /* GUIDELINE_APPROVED: error context */
        return E_SYSTEM_FILE;
    }

    /* Execute query and collect results */
    capacity = KATRA_INITIAL_CAPACITY_SMALL;
    ids = calloc(capacity, sizeof(char*));
    locs = calloc(capacity, sizeof(memory_location_t));

    if (!ids || !locs) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        /* Grow arrays if needed */
        if (*count >= capacity) {
            capacity *= 2;
            char** new_ids = realloc(ids, capacity * sizeof(char*));
            memory_location_t* new_locs = realloc(locs, capacity * sizeof(memory_location_t));

            if (!new_ids || !new_locs) {
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }

            ids = new_ids;
            locs = new_locs;
        }

        /* Extract record_id */
        const char* record_id = (const char*)sqlite3_column_text(stmt, 0);
        ids[*count] = strdup(record_id ? record_id : STR_EMPTY);
        if (!ids[*count]) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }

        /* Extract file location */
        const char* file_path = (const char*)sqlite3_column_text(stmt, 1);
        long offset = (long)sqlite3_column_int64(stmt, 2);

        strncpy(locs[*count].file_path, file_path ? file_path : STR_EMPTY,
                sizeof(locs[*count].file_path) - 1);
        locs[*count].file_path[sizeof(locs[*count].file_path) - 1] = '\0';
        locs[*count].offset = offset;

        (*count)++;
    }

    if (rc != SQLITE_DONE) {
        katra_report_error(E_SYSTEM_FILE, "tier1_index_query",
                          "Query execution failed"); /* GUIDELINE_APPROVED: error context */
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    *record_ids = ids;
    *locations = locs;
    sqlite3_finalize(stmt);

    LOG_DEBUG("Index query found %zu memories", *count);
    return KATRA_SUCCESS;

cleanup:
    if (ids) {
        for (size_t i = 0; i < *count; i++) {
            free(ids[i]);
        }
        free(ids);
    }
    free(locs);
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    return result;
}

/* Find similar memories (for convergence detection) */
int tier1_index_find_similar(const char* content,
                             float importance_threshold,
                             int time_window_hours,
                             char*** record_ids,
                             memory_location_t** locations,
                             size_t* count) {
    int result = KATRA_SUCCESS;
    sqlite3_stmt* stmt = NULL;
    char sql_query[KATRA_BUFFER_LARGE];
    size_t capacity = 0;
    char** ids = NULL;
    memory_location_t* locs = NULL;
    time_t cutoff_time = 0;

    if (!content || !record_ids || !locations || !count) {
        return E_INPUT_NULL;
    }

    if (!g_memory_db) {
        LOG_DEBUG("Tier 1 index not initialized");
        return E_INTERNAL_LOGIC;
    }

    *record_ids = NULL;
    *locations = NULL;
    *count = 0;

    /* Calculate cutoff time if window specified */
    if (time_window_hours > 0) {
        cutoff_time = time(NULL) - (time_window_hours * 3600);  /* 3600 seconds per hour */
    }

    /* Build FTS query */
    snprintf(sql_query, sizeof(sql_query),
            "SELECT m.record_id, m.file_path, m.file_offset " /* GUIDELINE_APPROVED */
            "FROM memory_content_fts f " /* GUIDELINE_APPROVED */
            "JOIN memories m ON f.record_id = m.record_id " /* GUIDELINE_APPROVED */
            "WHERE f.content MATCH ? " /* GUIDELINE_APPROVED */
            "AND m.importance >= ? " /* GUIDELINE_APPROVED */
            "AND m.archived = 0"); /* GUIDELINE_APPROVED */

    if (time_window_hours > 0) {
        size_t len = strlen(sql_query);
        snprintf(sql_query + len, sizeof(sql_query) - len,
                " AND m.timestamp >= %ld", (long)cutoff_time);
    }

    /* Add ordering and limit */
    size_t len = strlen(sql_query);
    snprintf(sql_query + len, sizeof(sql_query) - len,
            " ORDER BY m.importance DESC LIMIT 50"); /* GUIDELINE_APPROVED */

    /* Prepare statement */
    int rc = sqlite3_prepare_v2(g_memory_db, sql_query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier1_index_find_similar",
                          "Failed to prepare query"); /* GUIDELINE_APPROVED: error context */
        return E_SYSTEM_FILE;
    }

    /* Bind parameters */
    sqlite3_bind_text(stmt, 1, content, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, importance_threshold);

    /* Execute and collect results */
    capacity = KATRA_INITIAL_CAPACITY_SMALL;
    ids = calloc(capacity, sizeof(char*));
    locs = calloc(capacity, sizeof(memory_location_t));

    if (!ids || !locs) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            char** new_ids = realloc(ids, capacity * sizeof(char*));
            memory_location_t* new_locs = realloc(locs, capacity * sizeof(memory_location_t));

            if (!new_ids || !new_locs) {
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }

            ids = new_ids;
            locs = new_locs;
        }

        const char* record_id = (const char*)sqlite3_column_text(stmt, 0);
        ids[*count] = strdup(record_id ? record_id : STR_EMPTY);

        const char* file_path = (const char*)sqlite3_column_text(stmt, 1);
        long offset = (long)sqlite3_column_int64(stmt, 2);

        strncpy(locs[*count].file_path, file_path ? file_path : STR_EMPTY,
                sizeof(locs[*count].file_path) - 1);
        locs[*count].file_path[sizeof(locs[*count].file_path) - 1] = '\0';
        locs[*count].offset = offset;

        (*count)++;
    }

    *record_ids = ids;
    *locations = locs;
    sqlite3_finalize(stmt);

    LOG_DEBUG("Found %zu similar memories", *count);
    return KATRA_SUCCESS;

cleanup:
    if (ids) {
        for (size_t i = 0; i < *count; i++) {
            free(ids[i]);
        }
        free(ids);
    }
    free(locs);
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    return result;
}

/* Cleanup index resources */
void tier1_index_cleanup(void) {
    if (g_memory_db) {
        sqlite3_close(g_memory_db);
        g_memory_db = NULL;
    }
    LOG_DEBUG("Tier 1 index cleanup complete");
}
