/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_encoder.h"
#include "katra_error.h"
#include "katra_log.h"

/* Create universal encoder */
universal_encoder_t* katra_encoder_create(const char* ci_id) {
    universal_encoder_t* encoder = NULL;

    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_encoder_create",
                          KATRA_ERR_CI_ID_NULL);
        return NULL;
    }

    encoder = calloc(1, sizeof(universal_encoder_t));
    if (!encoder) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_encoder_create",
                          KATRA_ERR_ALLOC_FAILED);
        return NULL;
    }

    strncpy(encoder->ci_id, ci_id, sizeof(encoder->ci_id) - 1);
    encoder->ci_id[sizeof(encoder->ci_id) - 1] = '\0';
    encoder->backend_count = 0;
    encoder->initialized = false;

    LOG_INFO("Created universal encoder for CI: %s", ci_id);
    return encoder;
}

/* Add backend to encoder */
int katra_encoder_add_backend(universal_encoder_t* encoder, db_backend_t* backend) {
    if (!encoder || !backend) {
        katra_report_error(E_INPUT_NULL, "katra_encoder_add_backend",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (encoder->backend_count >= MAX_BACKENDS) {
        katra_report_error(E_INVALID_STATE, "katra_encoder_add_backend",
                          "Maximum backends reached");
        return E_INVALID_STATE;
    }

    encoder->backends[encoder->backend_count] = backend;
    encoder->backend_count++;

    LOG_INFO("Added %s backend to encoder (total: %zu)",
             backend->name, encoder->backend_count);

    return KATRA_SUCCESS;
}

/* Initialize encoder */
int katra_encoder_init(universal_encoder_t* encoder) {
    int result = KATRA_SUCCESS;

    if (!encoder) {
        katra_report_error(E_INPUT_NULL, "katra_encoder_init",
                          "encoder is NULL");
        return E_INPUT_NULL;
    }

    if (encoder->backend_count == 0) {
        katra_report_error(E_INVALID_STATE, "katra_encoder_init",
                          "No backends added");
        return E_INVALID_STATE;
    }

    /* Initialize all backends */
    for (size_t i = 0; i < encoder->backend_count; i++) {
        result = katra_db_backend_init(encoder->backends[i], encoder->ci_id);
        if (result != KATRA_SUCCESS) {
            katra_report_error(result, "katra_encoder_init",
                              "Failed to initialize backend");

            /* Cleanup already initialized backends */
            for (size_t j = 0; j < i; j++) {
                katra_db_backend_cleanup(encoder->backends[j]);
            }

            return result;
        }
    }

    encoder->initialized = true;
    LOG_INFO("Initialized encoder with %zu backends", encoder->backend_count);

    return KATRA_SUCCESS;
}

/* Store to all backends simultaneously */
int katra_encoder_store(universal_encoder_t* encoder,
                        const memory_record_t* record) {
    int success_count = 0;
    int first_error = KATRA_SUCCESS;

    if (!encoder || !record) {
        katra_report_error(E_INPUT_NULL, "katra_encoder_store",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (!encoder->initialized) {
        katra_report_error(E_INVALID_STATE, "katra_encoder_store",
                          "Encoder not initialized");
        return E_INVALID_STATE;
    }

    /* Store to all backends - collect errors but continue */
    for (size_t i = 0; i < encoder->backend_count; i++) {
        int result = katra_db_backend_store(encoder->backends[i], record);

        if (result == KATRA_SUCCESS) {
            success_count++;
            LOG_DEBUG("Stored to %s backend", encoder->backends[i]->name);
        } else if (result != E_INTERNAL_NOTIMPL) {
            /* Record first non-NOTIMPL error */
            if (first_error == KATRA_SUCCESS) {
                first_error = result;
            }
            LOG_WARN("Failed to store to %s backend: %d",
                    encoder->backends[i]->name, result);
        }
    }

    /* Success if at least one backend succeeded */
    if (success_count > 0) {
        LOG_INFO("Stored record %s to %d/%zu backends",
                record->record_id, success_count, encoder->backend_count);
        return KATRA_SUCCESS;
    }

    /* All backends failed */
    if (first_error != KATRA_SUCCESS) {
        katra_report_error(first_error, "katra_encoder_store",
                          "All backends failed to store");
        return first_error;
    }

    /* All backends returned NOTIMPL */
    katra_report_error(E_INTERNAL_NOTIMPL, "katra_encoder_store",
                      "No backends support store operation");
    return E_INTERNAL_NOTIMPL;
}

/* Query from best backend (with fallback) */
int katra_encoder_query(universal_encoder_t* encoder,
                        const db_query_t* query,
                        memory_record_t*** results,
                        size_t* count) {
    int result = KATRA_SUCCESS;

    if (!encoder || !query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "katra_encoder_query",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (!encoder->initialized) {
        katra_report_error(E_INVALID_STATE, "katra_encoder_query",
                          "Encoder not initialized");
        return E_INVALID_STATE;
    }

    /* Try backends in order until one succeeds */
    for (size_t i = 0; i < encoder->backend_count; i++) {
        result = katra_db_backend_query(encoder->backends[i], query,
                                         results, count);

        if (result == KATRA_SUCCESS) {
            LOG_INFO("Query succeeded from %s backend (%zu results)",
                    encoder->backends[i]->name, *count);
            return KATRA_SUCCESS;
        } else if (result != E_INTERNAL_NOTIMPL) {
            LOG_WARN("Query failed from %s backend: %d",
                    encoder->backends[i]->name, result);
        }
    }

    /* No backend succeeded */
    if (result == E_INTERNAL_NOTIMPL) {
        katra_report_error(E_INTERNAL_NOTIMPL, "katra_encoder_query",
                          "No backends support query operation");
    } else {
        katra_report_error(result, "katra_encoder_query",
                          "All backends failed to query");
    }

    return result;
}

/* Cleanup encoder */
void katra_encoder_cleanup(universal_encoder_t* encoder) {
    if (!encoder) {
        return;
    }

    if (encoder->initialized) {
        /* Cleanup all backends */
        for (size_t i = 0; i < encoder->backend_count; i++) {
            katra_db_backend_cleanup(encoder->backends[i]);
        }
        encoder->initialized = false;
        LOG_DEBUG("Cleaned up encoder for CI: %s", encoder->ci_id);
    }
}

/* Free encoder instance */
void katra_encoder_free(universal_encoder_t* encoder) {
    if (!encoder) {
        return;
    }

    /* Cleanup if still initialized */
    if (encoder->initialized) {
        katra_encoder_cleanup(encoder);
    }

    /* Free all backends */
    for (size_t i = 0; i < encoder->backend_count; i++) {
        katra_db_backend_free(encoder->backends[i]);
    }

    /* Free encoder structure */
    free(encoder);
    LOG_DEBUG("Freed encoder instance");
}
