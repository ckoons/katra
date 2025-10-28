/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_helpers.c - Internal helper functions
 *
 * Shared utilities to reduce boilerplate across breathing layer
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_breathing_internal.h"
#include "katra_breathing_helpers.h"

/* ============================================================================
 * MEMORY FORMATION HELPERS
 * ============================================================================ */

int breathing_store_typed_memory(memory_type_t type,
                                 const char* content,
                                 float importance,
                                 const char* importance_note,
                                 why_remember_t why_enum,
                                 const char* func_name) {
    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        breathing_get_ci_id(),
        type,
        content,
        importance
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, func_name, "Failed to create record");
        return E_SYSTEM_MEMORY;
    }

    /* Add importance note if provided */
    if (importance_note) {
        record->importance_note = strdup(importance_note);
        if (!record->importance_note) {
            katra_memory_free_record(record);
            return E_SYSTEM_MEMORY;
        }
    }

    /* Attach session ID */
    int session_result = breathing_attach_session(record);
    if (session_result != KATRA_SUCCESS) {
        katra_memory_free_record(record);
        return session_result;
    }

    /* Store memory */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    /* Track stats on success */
    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(type, why_enum);
    }

    return result;
}

int breathing_attach_session(memory_record_t* record) {
    if (!record) {
        return E_INPUT_NULL;
    }

    const char* session_id = breathing_get_session_id();
    if (session_id) {
        record->session_id = strdup(session_id);
        if (!record->session_id) {
            katra_report_error(E_SYSTEM_MEMORY, "breathing_attach_session",
                              "Failed to duplicate session_id");
            return E_SYSTEM_MEMORY;
        }
    }

    return KATRA_SUCCESS;
}

/* ============================================================================
 * QUERY RESULT HELPERS
 * ============================================================================ */

char** breathing_copy_memory_contents(memory_record_t** results,
                                     size_t result_count,
                                     size_t* out_count) {
    if (!results || !out_count || result_count == 0) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    /* Allocate array for owned string copies */
    char** strings = calloc(result_count, sizeof(char*));
    if (!strings) {
        *out_count = 0;
        return NULL;
    }

    /* Copy strings (caller owns these) */
    for (size_t i = 0; i < result_count; i++) {
        if (results[i]->content) {
            strings[i] = strdup(results[i]->content);
            if (!strings[i]) {
                /* Allocation failed - clean up and return NULL */
                for (size_t j = 0; j < i; j++) {
                    free(strings[j]);
                }
                free(strings);
                *out_count = 0;
                return NULL;
            }
        } else {
            strings[i] = NULL;
        }
    }

    *out_count = result_count;
    return strings;
}

/* ============================================================================
 * SEMANTIC PARSING HELPERS
 * ============================================================================ */

bool breathing_contains_any_phrase(const char* semantic, const char** phrases) {
    if (!semantic || !phrases) {
        return false;
    }

    for (size_t i = 0; phrases[i] != NULL; i++) {
        if (strcasestr(semantic, phrases[i])) {
            return true;
        }
    }

    return false;
}
