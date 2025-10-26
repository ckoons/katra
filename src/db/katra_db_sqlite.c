/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_db.h"
#include "katra_tier2_index.h"
#include "katra_error.h"
#include "katra_log.h"

/* SQLite backend context */
typedef struct {
    char ci_id[256];
    bool index_initialized;
    sqlite3* db;  /* Direct DB handle for custom queries */
} sqlite_context_t;

/* Initialize SQLite backend */
static int sqlite_init(void* ctx, const char* ci_id) {
    sqlite_context_t* sqlite_ctx = (sqlite_context_t*)ctx;

    if (!sqlite_ctx || !ci_id) {
        katra_report_error(E_INPUT_NULL, "sqlite_init", "NULL parameter");
        return E_INPUT_NULL;
    }

    strncpy(sqlite_ctx->ci_id, ci_id, sizeof(sqlite_ctx->ci_id) - 1);
    sqlite_ctx->ci_id[sizeof(sqlite_ctx->ci_id) - 1] = '\0';

    int result = tier2_index_init(ci_id);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "sqlite_init", "tier2_index_init failed");
        return result;
    }

    sqlite_ctx->index_initialized = true;
    LOG_INFO("SQLite backend initialized for CI: %s", ci_id);

    return KATRA_SUCCESS;
}

/* Cleanup SQLite backend */
static void sqlite_cleanup(void* ctx) {
    sqlite_context_t* sqlite_ctx = (sqlite_context_t*)ctx;

    if (!sqlite_ctx) {
        return;
    }

    if (sqlite_ctx->index_initialized) {
        tier2_index_cleanup();
        sqlite_ctx->index_initialized = false;
    }

    if (sqlite_ctx->db) {
        sqlite3_close(sqlite_ctx->db);
        sqlite_ctx->db = NULL;
    }

    LOG_DEBUG("SQLite backend cleaned up for CI: %s", sqlite_ctx->ci_id);
}

/* Store record in SQLite (via index) */
static int sqlite_store(void* ctx, const memory_record_t* record) {
    sqlite_context_t* sqlite_ctx = (sqlite_context_t*)ctx;

    if (!sqlite_ctx || !record) {
        katra_report_error(E_INPUT_NULL, "sqlite_store", "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!sqlite_ctx->index_initialized) {
        katra_report_error(E_INVALID_STATE, "sqlite_store",
                          "Backend not initialized");
        return E_INVALID_STATE;
    }

    /* SQLite backend stores via Tier 2 index */
    /* Note: This is a simplified implementation - would need to convert
     * memory_record_t to digest_record_t for full support */
    katra_report_error(E_INTERNAL_NOTIMPL, "sqlite_store",
                      "Direct memory record storage not yet implemented");
    return E_INTERNAL_NOTIMPL;
}

/* Retrieve record from SQLite by ID */
static int sqlite_retrieve(void* ctx, const char* record_id,
                           memory_record_t** record) {
    sqlite_context_t* sqlite_ctx = (sqlite_context_t*)ctx;

    if (!sqlite_ctx || !record_id || !record) {
        katra_report_error(E_INPUT_NULL, "sqlite_retrieve", "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!sqlite_ctx->index_initialized) {
        katra_report_error(E_INVALID_STATE, "sqlite_retrieve",
                          "Backend not initialized");
        return E_INVALID_STATE;
    }

    /* SQLite index supports ID-based lookups */
    /* Would need to implement tier2_index_retrieve_by_id() */
    katra_report_error(E_INTERNAL_NOTIMPL, "sqlite_retrieve",
                      "ID-based retrieval not yet implemented");
    return E_INTERNAL_NOTIMPL;
}

/* Query records from SQLite */
static int sqlite_query(void* ctx, const db_query_t* query,
                        memory_record_t*** results, size_t* count) {
    sqlite_context_t* sqlite_ctx = (sqlite_context_t*)ctx;

    if (!sqlite_ctx || !query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "sqlite_query", "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!sqlite_ctx->index_initialized) {
        katra_report_error(E_INVALID_STATE, "sqlite_query",
                          "Backend not initialized");
        return E_INVALID_STATE;
    }

    /* SQLite backend queries via Tier 2 index */
    /* Note: This is a simplified implementation - would need full conversion */
    katra_report_error(E_INTERNAL_NOTIMPL, "sqlite_query",
                      "Query not yet fully implemented");
    return E_INTERNAL_NOTIMPL;
}

/* Get SQLite backend statistics */
static int sqlite_get_stats(void* ctx, size_t* record_count, size_t* bytes_used) {
    sqlite_context_t* sqlite_ctx = (sqlite_context_t*)ctx;

    if (!sqlite_ctx || !record_count || !bytes_used) {
        katra_report_error(E_INPUT_NULL, "sqlite_get_stats", "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!sqlite_ctx->index_initialized) {
        katra_report_error(E_INVALID_STATE, "sqlite_get_stats",
                          "Backend not initialized");
        return E_INVALID_STATE;
    }

    size_t theme_count = 0;
    size_t keyword_count = 0;

    int result = tier2_index_stats(sqlite_ctx->ci_id, record_count,
                                    &theme_count, &keyword_count);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "sqlite_get_stats",
                          "tier2_index_stats failed");
        return result;
    }

    *bytes_used = 0;  /* SQLite doesn't track bytes directly */
    return KATRA_SUCCESS;
}

/* Create SQLite backend instance */
db_backend_t* katra_db_create_sqlite_backend(const char* ci_id) {
    db_backend_t* backend = NULL;
    sqlite_context_t* context = NULL;

    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_db_create_sqlite_backend",
                          "ci_id is NULL");
        return NULL;
    }

    /* Allocate backend structure */
    backend = calloc(1, sizeof(db_backend_t));
    if (!backend) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_db_create_sqlite_backend",
                          "Failed to allocate backend");
        return NULL;
    }

    /* Allocate context */
    context = calloc(1, sizeof(sqlite_context_t));
    if (!context) {
        free(backend);
        katra_report_error(E_SYSTEM_MEMORY, "katra_db_create_sqlite_backend",
                          "Failed to allocate context");
        return NULL;
    }

    /* Initialize backend structure */
    strncpy(backend->name, "sqlite", sizeof(backend->name) - 1);
    backend->type = DB_BACKEND_SQLITE;
    backend->context = context;
    backend->initialized = false;

    /* Set function pointers */
    backend->init = sqlite_init;
    backend->cleanup = sqlite_cleanup;
    backend->store = sqlite_store;
    backend->retrieve = sqlite_retrieve;
    backend->query = sqlite_query;
    backend->get_stats = sqlite_get_stats;
    backend->delete_record = NULL;  /* Not implemented */
    backend->update = NULL;          /* Not implemented */

    LOG_INFO("Created SQLite backend for CI: %s", ci_id);
    return backend;
}
