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
#include "katra_experience.h"

/* ============================================================================
 * MEMORY FORMATION HELPERS
 * ============================================================================ */

int breathing_store_typed_memory(memory_type_t type,
                                 const char* content,
                                 float importance,
                                 const char* importance_note,
                                 why_remember_t why_enum,
                                 const char* func_name) {
    const char* ci_id = breathing_get_ci_id();

    /* Check memory pressure and enforce limits in degraded mode */
    memory_health_t* health = get_memory_health(ci_id);
    if (health) {
        if (health->degraded_mode) {
            /* Critical memory pressure - only accept HIGH or CRITICAL importance */
            if (importance < MEMORY_IMPORTANCE_HIGH) {
                LOG_DEBUG("Rejecting low-importance memory in degraded mode (%.2f < %.2f)",
                         importance, MEMORY_IMPORTANCE_HIGH);
                free(health);
                return E_MEMORY_TIER_FULL;  /* Signal memory pressure */
            }
            LOG_DEBUG("Accepting high-importance memory despite degraded mode (%.2f)",
                     importance);
        }
        free(health);
    }

    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        ci_id,
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

    /* Detect emotion from content (Thane's Phase 1 - emotional salience) */
    emotional_tag_t emotion;
    int emotion_result = katra_detect_emotion(content, &emotion);
    if (emotion_result == KATRA_SUCCESS) {
        /* Map emotional_tag_t to memory_record_t fields */
        record->emotion_intensity = emotion.arousal;  /* Arousal = intensity (0-1) */
        if (emotion.emotion[0] != '\0') {
            record->emotion_type = strdup(emotion.emotion);
            if (!record->emotion_type) {
                katra_memory_free_record(record);
                return E_SYSTEM_MEMORY;
            }
        }
        LOG_DEBUG("Detected emotion for memory: %s (intensity=%.2f)",
                 emotion.emotion, emotion.arousal);
    }

    /* Attach session ID */
    int session_result = breathing_attach_session(record);
    if (session_result != KATRA_SUCCESS) {
        katra_memory_free_record(record);
        return session_result;
    }

    /* Set turn ID for reflection tracking */
    record->turn_id = get_current_turn();

    /* Copy record_id before storing (need it for turn tracking) */
    char* record_id = strdup(record->record_id);
    if (!record_id) {
        katra_memory_free_record(record);
        return E_SYSTEM_MEMORY;
    }

    /* Store memory */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    /* Track stats and turn on success */
    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(type, why_enum);
        track_memory_in_turn(record_id);  /* Track for end-of-turn reflection */
    }

    free(record_id);
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
                              KATRA_ERR_FAILED_TO_DUPLICATE_SESSION_ID);
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
        if (!results[i]->content) {
            strings[i] = NULL;
            continue;
        }

        /* For decisions, include both content and reasoning */
        if (results[i]->type == MEMORY_TYPE_DECISION && results[i]->importance_note) {
            /* Format: "Decision: <content> (Reasoning: <importance_note>)" */
            size_t needed = strlen(results[i]->content) +
                          strlen(results[i]->importance_note) +
                          strlen("Decision: ") +
                          strlen(" (Reasoning: )") + 1;

            strings[i] = malloc(needed);
            if (!strings[i]) {
                /* Allocation failed - clean up and return NULL */
                for (size_t j = 0; j < i; j++) {
                    free(strings[j]);
                }
                free(strings);
                *out_count = 0;
                return NULL;
            }

            snprintf(strings[i], needed, "Decision: %s (Reasoning: %s)",
                    results[i]->content, results[i]->importance_note);
        } else {
            /* For all other types, just copy content */
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
