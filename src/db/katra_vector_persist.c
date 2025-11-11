/* © 2025 Casey Koons All rights reserved */

/* Vector embedding persistence layer - SQLite storage for embeddings */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_vector.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_strings.h"

/* SQL for creating vectors table */
static const char* SQL_CREATE_VECTORS_TABLE =
    "CREATE TABLE IF NOT EXISTS vectors ("
    "  record_id TEXT PRIMARY KEY,"
    "  dimensions INTEGER NOT NULL,"
    "  embedding_values BLOB NOT NULL,"
    "  magnitude REAL NOT NULL,"
    "  created_at INTEGER DEFAULT (strftime('%s', 'now'))"
    ")";

/* SQL for storing vector */
static const char* SQL_STORE_VECTOR =
    "INSERT OR REPLACE INTO vectors (record_id, dimensions, embedding_values, magnitude) "
    "VALUES (?, ?, ?, ?)";

/* SQL for loading vectors */
static const char* SQL_LOAD_VECTORS =
    "SELECT record_id, dimensions, embedding_values, magnitude FROM vectors";

/* SQL for deleting vector */
static const char* SQL_DELETE_VECTOR =
    "DELETE FROM vectors WHERE record_id = ?";

/* Open database connection for vector storage */
static int open_vector_db(const char* ci_id, sqlite3** db_out) {
    if (!ci_id || !db_out) {
        katra_report_error(E_INPUT_NULL, __func__, KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    /* Build database path */
    char db_path[KATRA_PATH_MAX];
    int result = katra_build_path(db_path, sizeof(db_path),
                                  KATRA_DIR_MEMORY, KATRA_DIR_TIER2,
                                  "vectors", NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Ensure directory exists */
    result = katra_ensure_dir(db_path);
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Failed to create vector directory: %s", db_path);
        /* Continue anyway - sqlite3_open might succeed */
    }

    /* Append database filename */
    size_t path_len = strlen(db_path);
    snprintf(db_path + path_len, sizeof(db_path) - path_len, "/vectors.db");

    /* Open database */
    int rc = sqlite3_open(db_path, db_out);
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, __func__, sqlite3_errmsg(*db_out));
        sqlite3_close(*db_out);
        *db_out = NULL;
        return E_SYSTEM_FILE;
    }

    LOG_DEBUG("Opened vector database: %s", db_path);
    return KATRA_SUCCESS;
}

/* Initialize persistent vector storage */
int katra_vector_persist_init(const char* ci_id) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, __func__, KATRA_ERR_CI_ID_NULL);
        return E_INPUT_NULL;
    }

    sqlite3* db = NULL;
    int result = open_vector_db(ci_id, &db);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Create vectors table */
    char* err_msg = NULL;
    int rc = sqlite3_exec(db, SQL_CREATE_VECTORS_TABLE, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        katra_report_error(E_INTERNAL_LOGIC, __func__, err_msg ? err_msg : "Unknown error");
        if (err_msg) {
            sqlite3_free(err_msg);
        }
        sqlite3_close(db);
        return E_INTERNAL_LOGIC;
    }

    sqlite3_close(db);
    LOG_INFO("Initialized vector persistence for CI: %s", ci_id);
    return KATRA_SUCCESS;
}

/* Save embedding to persistent storage */
int katra_vector_persist_save(const char* ci_id,
                               const vector_embedding_t* embedding) {
    if (!ci_id || !embedding) {
        katra_report_error(E_INPUT_NULL, __func__, KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int result = KATRA_SUCCESS;

    /* Open database */
    result = open_vector_db(ci_id, &db);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Prepare statement */
    int rc = sqlite3_prepare_v2(db, SQL_STORE_VECTOR, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        katra_report_error(E_INTERNAL_LOGIC, __func__, sqlite3_errmsg(db));
        result = E_INTERNAL_LOGIC;
        goto cleanup;
    }

    /* Bind parameters */
    sqlite3_bind_text(stmt, 1, embedding->record_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, (int)embedding->dimensions);
    sqlite3_bind_blob(stmt, 3, embedding->values,
                     (int)(embedding->dimensions * sizeof(float)), SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, embedding->magnitude);

    /* Execute */
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        katra_report_error(E_INTERNAL_LOGIC, __func__, sqlite3_errmsg(db));
        result = E_INTERNAL_LOGIC;
        goto cleanup;
    }

    LOG_DEBUG("Saved vector to persistent storage: %s", embedding->record_id);

cleanup:
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    if (db) {
        sqlite3_close(db);
    }
    return result;
}

/* Load all embeddings from persistent storage */
int katra_vector_persist_load(const char* ci_id, vector_store_t* store) {
    if (!ci_id || !store) {
        katra_report_error(E_INPUT_NULL, __func__, KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int result = KATRA_SUCCESS;
    size_t loaded_count = 0;

    /* Open database */
    result = open_vector_db(ci_id, &db);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Prepare statement */
    int rc = sqlite3_prepare_v2(db, SQL_LOAD_VECTORS, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        katra_report_error(E_INTERNAL_LOGIC, __func__, sqlite3_errmsg(db));
        result = E_INTERNAL_LOGIC;
        goto cleanup;
    }

    /* Load embeddings */
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* record_id = (const char*)sqlite3_column_text(stmt, 0);
        int dimensions = sqlite3_column_int(stmt, 1);
        const void* values_blob = sqlite3_column_blob(stmt, 2);
        int blob_size = sqlite3_column_bytes(stmt, 2);
        double magnitude = sqlite3_column_double(stmt, 3);

        /* Validate data */
        if (!record_id || !values_blob || dimensions != VECTOR_DIMENSIONS) {
            LOG_WARN("Skipping invalid vector: %s", record_id ? record_id : "NULL");
            continue;
        }

        if (blob_size != (int)(dimensions * sizeof(float))) {
            LOG_WARN("Skipping vector with wrong size: %s", record_id);
            continue;
        }

        /* Check if we need to expand capacity */
        if (store->count >= store->capacity) {
            size_t new_capacity = store->capacity * 2;
            vector_embedding_t** new_embeddings = realloc(store->embeddings,
                                                           new_capacity * sizeof(vector_embedding_t*));
            if (!new_embeddings) {
                katra_report_error(E_SYSTEM_MEMORY, __func__,
                                  KATRA_ERR_FAILED_TO_EXPAND_EMBEDDINGS);
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }

            store->embeddings = new_embeddings;
            store->capacity = new_capacity;

            /* Initialize new slots */
            for (size_t i = store->count; i < new_capacity; i++) {
                store->embeddings[i] = NULL;
            }
        }

        /* Create embedding */
        vector_embedding_t* embedding = calloc(1, sizeof(vector_embedding_t));
        if (!embedding) {
            katra_report_error(E_SYSTEM_MEMORY, __func__, KATRA_ERR_ALLOC_FAILED);
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }

        embedding->dimensions = (size_t)dimensions;
        embedding->magnitude = (float)magnitude;

        /* Copy record ID */
        strncpy(embedding->record_id, record_id, sizeof(embedding->record_id) - 1);
        embedding->record_id[sizeof(embedding->record_id) - 1] = '\0';

        /* Allocate and copy values */
        embedding->values = calloc(dimensions, sizeof(float));
        if (!embedding->values) {
            free(embedding);
            katra_report_error(E_SYSTEM_MEMORY, __func__, KATRA_ERR_ALLOC_FAILED);
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }

        memcpy(embedding->values, values_blob, blob_size);

        /* Add to store */
        store->embeddings[store->count] = embedding;
        store->count++;
        loaded_count++;
    }

    if (rc != SQLITE_DONE) {
        katra_report_error(E_INTERNAL_LOGIC, __func__, sqlite3_errmsg(db));
        result = E_INTERNAL_LOGIC;
        goto cleanup;
    }

    LOG_INFO("Loaded %zu vectors from persistent storage", loaded_count);

cleanup:
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    if (db) {
        sqlite3_close(db);
    }
    return result;
}

/* Delete embedding from persistent storage */
int katra_vector_persist_delete(const char* ci_id, const char* record_id) {
    if (!ci_id || !record_id) {
        katra_report_error(E_INPUT_NULL, __func__, KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    sqlite3* db = NULL;
    sqlite3_stmt* stmt = NULL;
    int result = KATRA_SUCCESS;

    /* Open database */
    result = open_vector_db(ci_id, &db);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Prepare statement */
    int rc = sqlite3_prepare_v2(db, SQL_DELETE_VECTOR, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        katra_report_error(E_INTERNAL_LOGIC, __func__, sqlite3_errmsg(db));
        result = E_INTERNAL_LOGIC;
        goto cleanup;
    }

    /* Bind parameters */
    sqlite3_bind_text(stmt, 1, record_id, -1, SQLITE_STATIC);

    /* Execute */
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        katra_report_error(E_INTERNAL_LOGIC, __func__, sqlite3_errmsg(db));
        result = E_INTERNAL_LOGIC;
        goto cleanup;
    }

    LOG_DEBUG("Deleted vector from persistent storage: %s", record_id);

cleanup:
    if (stmt) {
        sqlite3_finalize(stmt);
    }
    if (db) {
        sqlite3_close(db);
    }
    return result;
}
