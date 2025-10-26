/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_db.h"
#include "katra_error.h"
#include "katra_log.h"

/* Initialize database backend */
int katra_db_backend_init(db_backend_t* backend, const char* ci_id) {
    if (!backend || !ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_db_backend_init",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!backend->init) {
        katra_report_error(E_INVALID_STATE, "katra_db_backend_init",
                          "Backend has no init function");
        return E_INVALID_STATE;
    }

    int result = backend->init(backend->context, ci_id);
    if (result == KATRA_SUCCESS) {
        backend->initialized = true;
        LOG_INFO("Initialized %s backend for CI: %s", backend->name, ci_id);
    }

    return result;
}

/* Cleanup database backend */
void katra_db_backend_cleanup(db_backend_t* backend) {
    if (!backend) {
        return;
    }

    if (backend->cleanup && backend->initialized) {
        backend->cleanup(backend->context);
        backend->initialized = false;
        LOG_DEBUG("Cleaned up %s backend", backend->name);
    }
}

/* Store record to backend */
int katra_db_backend_store(db_backend_t* backend, const memory_record_t* record) {
    if (!backend || !record) {
        katra_report_error(E_INPUT_NULL, "katra_db_backend_store",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!backend->initialized) {
        katra_report_error(E_INVALID_STATE, "katra_db_backend_store",
                          "Backend not initialized");
        return E_INVALID_STATE;
    }

    if (!backend->store) {
        katra_report_error(E_INTERNAL_NOTIMPL, "katra_db_backend_store",
                          "Backend does not support store operation");
        return E_INTERNAL_NOTIMPL;
    }

    return backend->store(backend->context, record);
}

/* Retrieve record from backend */
int katra_db_backend_retrieve(db_backend_t* backend, const char* record_id,
                               memory_record_t** record) {
    if (!backend || !record_id || !record) {
        katra_report_error(E_INPUT_NULL, "katra_db_backend_retrieve",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!backend->initialized) {
        katra_report_error(E_INVALID_STATE, "katra_db_backend_retrieve",
                          "Backend not initialized");
        return E_INVALID_STATE;
    }

    if (!backend->retrieve) {
        katra_report_error(E_INTERNAL_NOTIMPL, "katra_db_backend_retrieve",
                          "Backend does not support retrieve operation");
        return E_INTERNAL_NOTIMPL;
    }

    return backend->retrieve(backend->context, record_id, record);
}

/* Query records from backend */
int katra_db_backend_query(db_backend_t* backend, const db_query_t* query,
                            memory_record_t*** results, size_t* count) {
    if (!backend || !query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "katra_db_backend_query",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!backend->initialized) {
        katra_report_error(E_INVALID_STATE, "katra_db_backend_query",
                          "Backend not initialized");
        return E_INVALID_STATE;
    }

    if (!backend->query) {
        katra_report_error(E_INTERNAL_NOTIMPL, "katra_db_backend_query",
                          "Backend does not support query operation");
        return E_INTERNAL_NOTIMPL;
    }

    return backend->query(backend->context, query, results, count);
}

/* Free backend instance */
void katra_db_backend_free(db_backend_t* backend) {
    if (!backend) {
        return;
    }

    /* Cleanup if still initialized */
    if (backend->initialized && backend->cleanup) {
        backend->cleanup(backend->context);
        backend->initialized = false;
    }

    /* Free context */
    if (backend->context) {
        free(backend->context);
        backend->context = NULL;
    }

    /* Free backend structure */
    free(backend);
    LOG_DEBUG("Freed backend instance");
}
