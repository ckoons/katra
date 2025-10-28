/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_tier2_index.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_strings.h"

/* SQLite connection (one per process) */
static sqlite3* g_db = NULL;

/* Get database handle (for mgmt module) */
sqlite3* tier2_index_get_db(void) {
    return g_db;
}

/* SQL schema for index database */
static const char* SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS digests ("
    "  digest_id TEXT PRIMARY KEY,"
    "  ci_id TEXT NOT NULL,"
    "  timestamp INTEGER NOT NULL,"
    "  period_type INTEGER NOT NULL,"
    "  period_id TEXT NOT NULL,"
    "  digest_type INTEGER NOT NULL,"
    "  source_record_count INTEGER,"
    "  questions_asked INTEGER,"
    "  archived INTEGER DEFAULT 0,"
    "  file_path TEXT NOT NULL,"
    "  file_offset INTEGER NOT NULL"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_ci_time ON digests(ci_id, timestamp DESC);"
    "CREATE INDEX IF NOT EXISTS idx_period ON digests(period_type, period_id);"
    "CREATE INDEX IF NOT EXISTS idx_type ON digests(digest_type);"
    ""
    "CREATE TABLE IF NOT EXISTS themes ("
    "  digest_id TEXT NOT NULL,"
    "  theme TEXT NOT NULL,"
    "  FOREIGN KEY (digest_id) REFERENCES digests(digest_id)"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_themes ON themes(theme, digest_id);"
    ""
    "CREATE TABLE IF NOT EXISTS keywords ("
    "  digest_id TEXT NOT NULL,"
    "  keyword TEXT NOT NULL,"
    "  FOREIGN KEY (digest_id) REFERENCES digests(digest_id)"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_keywords ON keywords(keyword, digest_id);";

/* Get index database path */
static int get_index_db_path(const char* ci_id, char* buffer, size_t size) {
    (void)ci_id;  /* Unused - future multi-tenant support */

    int result = katra_build_path(buffer, size,
                                  KATRA_DIR_MEMORY, KATRA_DIR_TIER2,
                                  "index", NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Append database filename */
    size_t len = strlen(buffer);
    snprintf(buffer + len, size - len, "/digests.db");

    return KATRA_SUCCESS;
}

/* Initialize Tier 2 index database */
int tier2_index_init(const char* ci_id) {
    int result = KATRA_SUCCESS;
    char db_path[KATRA_PATH_MAX];
    char index_dir[KATRA_PATH_MAX];
    char* err_msg = NULL;

    if (!ci_id) {
        return E_INPUT_NULL;
    }

    /* Build index directory path */
    result = katra_build_and_ensure_dir(index_dir, sizeof(index_dir),
                                       KATRA_DIR_MEMORY, KATRA_DIR_TIER2,
                                       "index", NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Get database path */
    result = get_index_db_path(ci_id, db_path, sizeof(db_path));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Open or create database */
    int rc = sqlite3_open(db_path, &g_db);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier2_index_init",
                          "Failed to open SQLite database: %s",
                          sqlite3_errmsg(g_db));
        sqlite3_close(g_db);
        g_db = NULL;
        return E_SYSTEM_FILE;
    }

    /* Create schema */
    rc = sqlite3_exec(g_db, SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier2_index_init",
                          "Failed to create schema: %s", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(g_db);
        g_db = NULL;
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Tier 2 index initialized: %s", db_path);
    return KATRA_SUCCESS;
}

/* Add digest to index */
int tier2_index_add(const digest_record_t* digest,
                    const char* file_path,
                    long offset) {
    int result = KATRA_SUCCESS;
    sqlite3_stmt* stmt = NULL;
    int rc;

    if (!digest || !file_path) {
        return E_INPUT_NULL;
    }

    if (!g_db) {
        /* Index not initialized - this is expected when tier2 is not set up */
        LOG_DEBUG("Tier 2 index not initialized, skipping index add");
        return E_INTERNAL_LOGIC;
    }

    /* Begin transaction */
    rc = sqlite3_exec(g_db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier2_index_add",
                          "Failed to begin transaction: %s",
                          sqlite3_errmsg(g_db));
        return E_SYSTEM_FILE;
    }

    /* Insert digest record */
    const char* insert_sql =
        "INSERT OR REPLACE INTO digests "
        "(digest_id, ci_id, timestamp, period_type, period_id, "
        " digest_type, source_record_count, questions_asked, archived, "
        " file_path, file_offset) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    rc = sqlite3_prepare_v2(g_db, insert_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier2_index_add",
                          "Failed to prepare statement: %s",
                          sqlite3_errmsg(g_db));
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    /* Bind parameters */
    sqlite3_bind_text(stmt, 1, digest->digest_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, digest->ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)digest->timestamp);
    sqlite3_bind_int(stmt, 4, (int)digest->period_type);
    sqlite3_bind_text(stmt, 5, digest->period_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, (int)digest->digest_type);
    sqlite3_bind_int64(stmt, 7, (sqlite3_int64)digest->source_record_count);
    sqlite3_bind_int(stmt, 8, digest->questions_asked);
    sqlite3_bind_int(stmt, 9, digest->archived ? 1 : 0);
    sqlite3_bind_text(stmt, 10, file_path, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 11, (sqlite3_int64)offset);

    /* Execute */
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        katra_report_error(E_SYSTEM_FILE, "tier2_index_add",
                          "Failed to insert digest: %s",
                          sqlite3_errmsg(g_db));
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    /* Insert themes */
    const char* theme_sql = "INSERT INTO themes (digest_id, theme) VALUES (?, ?)";
    for (size_t i = 0; i < digest->theme_count; i++) {
        if (!digest->themes || !digest->themes[i]) continue;

        rc = sqlite3_prepare_v2(g_db, theme_sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) continue;

        sqlite3_bind_text(stmt, 1, digest->digest_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, digest->themes[i], -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        stmt = NULL;
    }

    /* Insert keywords */
    const char* keyword_sql = "INSERT INTO keywords (digest_id, keyword) VALUES (?, ?)";
    for (size_t i = 0; i < digest->keyword_count; i++) {
        if (!digest->keywords || !digest->keywords[i]) continue;

        rc = sqlite3_prepare_v2(g_db, keyword_sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) continue;

        sqlite3_bind_text(stmt, 1, digest->digest_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, digest->keywords[i], -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        stmt = NULL;
    }

    /* Commit transaction */
    rc = sqlite3_exec(g_db, "COMMIT", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier2_index_add",
                          "Failed to commit transaction: %s",
                          sqlite3_errmsg(g_db));
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    LOG_DEBUG("Added digest %s to index", digest->digest_id);
    return KATRA_SUCCESS;

cleanup:
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    sqlite3_exec(g_db, "ROLLBACK", NULL, NULL, NULL);
    return result;
}

/* Check if index exists */
bool tier2_index_exists(const char* ci_id) {
    char db_path[KATRA_PATH_MAX];

    if (get_index_db_path(ci_id, db_path, sizeof(db_path)) != KATRA_SUCCESS) {
        return false;
    }

    FILE* fp = fopen(db_path, "r");
    if (fp) {
        fclose(fp);
        return true;
    }

    return false;
}

/* Helper: Build SQL WHERE clause from query */
static int build_where_clause(const digest_query_t* query, char* buffer, size_t size) {
    size_t offset = 0;
    bool has_condition = false;

    offset += snprintf(buffer + offset, size - offset, " WHERE");

    /* CI ID filter */
    if (query->ci_id) {
        offset += snprintf(buffer + offset, size - offset, "%s ci_id = '%s'",
                          has_condition ? " AND" : "", query->ci_id);
        has_condition = true;
    }

    /* Time range filters */
    if (query->start_time > 0) {
        offset += snprintf(buffer + offset, size - offset, "%s timestamp >= %ld",
                          has_condition ? " AND" : "", (long)query->start_time);
        has_condition = true;
    }
    if (query->end_time > 0) {
        offset += snprintf(buffer + offset, size - offset, "%s timestamp <= %ld",
                          has_condition ? " AND" : "", (long)query->end_time);
        has_condition = true;
    }

    /* Period type filter (-1 means any) */
    if ((int)query->period_type != -1) {
        offset += snprintf(buffer + offset, size - offset, "%s period_type = %d",
                          has_condition ? " AND" : "", (int)query->period_type);
        has_condition = true;
    }

    /* Digest type filter (-1 means any) */
    if ((int)query->digest_type != -1) {
        offset += snprintf(buffer + offset, size - offset, "%s digest_type = %d",
                          has_condition ? " AND" : "", (int)query->digest_type);
        has_condition = true;
    }

    /* Archived filter (always exclude archived by default) */
    offset += snprintf(buffer + offset, size - offset, "%s archived = 0",
                      has_condition ? " AND" : "");
    has_condition = true;

    /* If no conditions, return empty WHERE clause */
    if (!has_condition) {
        buffer[0] = '\0';
    }

    return KATRA_SUCCESS;
}

/* Query index for matching digest IDs */
int tier2_index_query(const digest_query_t* query,
                      char*** digest_ids,
                      index_location_t** locations,
                      size_t* count) {
    int result = KATRA_SUCCESS;
    sqlite3_stmt* stmt = NULL;
    char sql_query[KATRA_BUFFER_LARGE];
    char where_clause[KATRA_BUFFER_MEDIUM];
    size_t capacity = 0;
    char** ids = NULL;
    index_location_t* locs = NULL;

    if (!query || !digest_ids || !locations || !count) {
        return E_INPUT_NULL;
    }

    if (!g_db) {
        /* Index not initialized - this is expected when tier2 is not set up */
        LOG_DEBUG("Tier 2 index not initialized, skipping index query");
        return E_INTERNAL_LOGIC;
    }

    *digest_ids = NULL;
    *locations = NULL;
    *count = 0;

    /* Build WHERE clause */
    result = build_where_clause(query, where_clause, sizeof(where_clause));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Build full SQL query */
    snprintf(sql_query, sizeof(sql_query),
            "SELECT digest_id, file_path, file_offset FROM digests%s ORDER BY timestamp DESC",
            where_clause);

    /* Add limit if specified */
    if (query->limit > 0) {
        size_t len = strlen(sql_query);
        snprintf(sql_query + len, sizeof(sql_query) - len, " LIMIT %zu", query->limit);
    }

    /* Prepare statement */
    int rc = sqlite3_prepare_v2(g_db, sql_query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier2_index_query",
                          "Failed to prepare query: %s", sqlite3_errmsg(g_db));
        return E_SYSTEM_FILE;
    }

    /* Execute query and collect results */
    capacity = 10;
    ids = calloc(capacity, sizeof(char*));
    locs = calloc(capacity, sizeof(index_location_t));

    if (!ids || !locs) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        /* Grow arrays if needed */
        if (*count >= capacity) {
            capacity *= 2;
            char** new_ids = realloc(ids, capacity * sizeof(char*));
            index_location_t* new_locs = realloc(locs, capacity * sizeof(index_location_t));

            if (!new_ids || !new_locs) {
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }

            ids = new_ids;
            locs = new_locs;
        }

        /* Extract digest_id */
        const char* digest_id = (const char*)sqlite3_column_text(stmt, 0);
        ids[*count] = strdup(digest_id ? digest_id : "");
        if (!ids[*count]) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }

        /* Extract file location */
        const char* file_path = (const char*)sqlite3_column_text(stmt, 1);
        long offset = (long)sqlite3_column_int64(stmt, 2);

        strncpy(locs[*count].file_path, file_path ? file_path : "",
                sizeof(locs[*count].file_path) - 1);
        locs[*count].file_path[sizeof(locs[*count].file_path) - 1] = '\0';
        locs[*count].offset = offset;

        (*count)++;
    }

    if (rc != SQLITE_DONE) {
        katra_report_error(E_SYSTEM_FILE, "tier2_index_query",
                          "Query execution failed: %s", sqlite3_errmsg(g_db));
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    *digest_ids = ids;
    *locations = locs;
    sqlite3_finalize(stmt);

    LOG_DEBUG("Index query found %zu digests", *count);
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

/* Load specific digests by locations */
int tier2_load_by_locations(const index_location_t* locations,
                             size_t count,
                             digest_record_t*** results,
                             size_t* result_count) {
    int result = KATRA_SUCCESS;
    digest_record_t** digests = NULL;
    size_t loaded = 0;

    if (!locations || !results || !result_count) {
        return E_INPUT_NULL;
    }

    *results = NULL;
    *result_count = 0;

    if (count == 0) {
        return KATRA_SUCCESS;
    }

    /* Allocate results array */
    digests = calloc(count, sizeof(digest_record_t*));
    if (!digests) {
        return E_SYSTEM_MEMORY;
    }

    /* Load each digest from its file location */
    for (size_t i = 0; i < count; i++) {
        FILE* fp = fopen(locations[i].file_path, "r");
        if (!fp) {
            continue;  /* Skip missing files */
        }

        /* Seek to offset */
        if (fseek(fp, locations[i].offset, SEEK_SET) != 0) {
            fclose(fp);
            continue;
        }

        /* Read line */
        char line[KATRA_BUFFER_LARGE];
        if (!fgets(line, sizeof(line), fp)) {
            fclose(fp);
            continue;
        }

        fclose(fp);

        /* Parse digest from JSON */
        digest_record_t* digest = NULL;
        result = katra_tier2_parse_json_digest(line, &digest);
        if (result == KATRA_SUCCESS && digest) {
            digests[loaded++] = digest;
        }
    }

    *results = digests;
    *result_count = loaded;

    LOG_DEBUG("Loaded %zu digests from %zu locations", loaded, count);
    return KATRA_SUCCESS;
}

/* Cleanup index resources */
void tier2_index_cleanup(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
    LOG_DEBUG("Tier 2 index cleanup complete");
}
