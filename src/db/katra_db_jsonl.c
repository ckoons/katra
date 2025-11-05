/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_db.h"
#include "katra_tier1.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* JSONL backend context */
typedef struct {
    char ci_id[KATRA_BUFFER_MEDIUM];
    bool tier1_initialized;
} jsonl_context_t;

/* Initialize JSONL backend */
static int jsonl_init(void* ctx, const char* ci_id) {
    jsonl_context_t* jsonl = (jsonl_context_t*)ctx;

    if (!jsonl || !ci_id) {
        katra_report_error(E_INPUT_NULL, "jsonl_init", KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    strncpy(jsonl->ci_id, ci_id, sizeof(jsonl->ci_id) - 1);
    jsonl->ci_id[sizeof(jsonl->ci_id) - 1] = '\0';

    int result = tier1_init(ci_id);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "jsonl_init", "tier1_init failed");
        return result;
    }

    jsonl->tier1_initialized = true;
    LOG_INFO("JSONL backend initialized for CI: %s", ci_id);

    return KATRA_SUCCESS;
}

/* Cleanup JSONL backend */
static void jsonl_cleanup(void* ctx) {
    jsonl_context_t* jsonl = (jsonl_context_t*)ctx;

    if (!jsonl) {
        return;
    }

    if (jsonl->tier1_initialized) {
        tier1_cleanup();
        jsonl->tier1_initialized = false;
    }

    LOG_DEBUG("JSONL backend cleaned up for CI: %s", jsonl->ci_id);
}

/* Store record in JSONL */
static int jsonl_store(void* ctx, const memory_record_t* record) {
    jsonl_context_t* jsonl = (jsonl_context_t*)ctx;

    if (!jsonl || !record) {
        katra_report_error(E_INPUT_NULL, "jsonl_store", KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (!jsonl->tier1_initialized) {
        katra_report_error(E_INVALID_STATE, "jsonl_store",
                          KATRA_ERR_BACKEND_NOT_INITIALIZED);
        return E_INVALID_STATE;
    }

    int result = tier1_store(record);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "jsonl_store", "tier1_store failed");
        return result;
    }

    LOG_DEBUG("JSONL backend stored record: %s", record->record_id);
    return KATRA_SUCCESS;
}

/* Retrieve record from JSONL (not efficiently implemented in Tier 1) */
static int jsonl_retrieve(void* ctx, const char* record_id,
                          memory_record_t** record) {
    jsonl_context_t* jsonl = (jsonl_context_t*)ctx;

    if (!jsonl || !record_id || !record) {
        katra_report_error(E_INPUT_NULL, "jsonl_retrieve", KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (!jsonl->tier1_initialized) {
        katra_report_error(E_INVALID_STATE, "jsonl_retrieve",
                          KATRA_ERR_BACKEND_NOT_INITIALIZED);
        return E_INVALID_STATE;
    }

    /* JSONL doesn't support direct ID lookup efficiently */
    /* Would need to query all and filter - not optimal */
    katra_report_error(E_INTERNAL_NOTIMPL, "jsonl_retrieve",
                      KATRA_ERR_JSONL_NO_DIRECT_RETRIEVAL);
    return E_INTERNAL_NOTIMPL;
}

/* Query records from JSONL */
static int jsonl_query(void* ctx, const db_query_t* query,
                       memory_record_t*** results, size_t* count) {
    jsonl_context_t* jsonl = (jsonl_context_t*)ctx;

    if (!jsonl || !query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "jsonl_query", KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (!jsonl->tier1_initialized) {
        katra_report_error(E_INVALID_STATE, "jsonl_query",
                          KATRA_ERR_BACKEND_NOT_INITIALIZED);
        return E_INVALID_STATE;
    }

    /* Convert db_query_t to memory_query_t */
    memory_query_t mem_query = {
        .ci_id = query->ci_id,
        .start_time = query->start_time,
        .end_time = query->end_time,
        .type = query->type,
        .min_importance = query->min_importance,
        .tier = KATRA_TIER1,
        .limit = query->limit
    };

    int result = tier1_query(&mem_query, results, count);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "jsonl_query", "tier1_query failed");
        return result;
    }

    LOG_DEBUG("JSONL backend query returned %zu results", *count);
    return KATRA_SUCCESS;
}

/* Get JSONL backend statistics */
static int jsonl_get_stats(void* ctx, size_t* record_count, size_t* bytes_used) {
    jsonl_context_t* jsonl = (jsonl_context_t*)ctx;

    if (!jsonl || !record_count || !bytes_used) {
        katra_report_error(E_INPUT_NULL, "jsonl_get_stats", KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (!jsonl->tier1_initialized) {
        katra_report_error(E_INVALID_STATE, "jsonl_get_stats",
                          KATRA_ERR_BACKEND_NOT_INITIALIZED);
        return E_INVALID_STATE;
    }

    int result = tier1_stats(jsonl->ci_id, record_count, bytes_used);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "jsonl_get_stats", "tier1_stats failed");
        return result;
    }

    return KATRA_SUCCESS;
}

/* Create JSONL backend instance */
db_backend_t* katra_db_create_jsonl_backend(const char* ci_id) {
    db_backend_t* backend = NULL;
    jsonl_context_t* context = NULL;

    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_db_create_jsonl_backend",
                          KATRA_ERR_CI_ID_NULL);
        return NULL;
    }

    /* Allocate backend structure */
    backend = calloc(1, sizeof(db_backend_t));
    if (!backend) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_db_create_jsonl_backend",
                          KATRA_ERR_ALLOC_FAILED);
        return NULL;
    }

    /* Allocate context */
    context = calloc(1, sizeof(jsonl_context_t));
    if (!context) {
        free(backend);
        katra_report_error(E_SYSTEM_MEMORY, "katra_db_create_jsonl_backend",
                          KATRA_ERR_ALLOC_FAILED);
        return NULL;
    }

    /* Initialize backend structure */
    strncpy(backend->name, KATRA_BACKEND_NAME_JSONL, sizeof(backend->name) - 1);
    backend->type = DB_BACKEND_JSONL;
    backend->context = context;
    backend->initialized = false;

    /* Set function pointers */
    backend->init = jsonl_init;
    backend->cleanup = jsonl_cleanup;
    backend->store = jsonl_store;
    backend->retrieve = jsonl_retrieve;
    backend->query = jsonl_query;
    backend->get_stats = jsonl_get_stats;
    backend->delete_record = NULL;  /* Not supported */
    backend->update = NULL;          /* Not supported */

    LOG_INFO("Created JSONL backend for CI: %s", ci_id);
    return backend;
}
