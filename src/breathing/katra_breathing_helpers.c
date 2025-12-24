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
#include "katra_consent.h"
#include "katra_limits.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_core_common.h"
#include "katra_breathing_internal.h"
#include "katra_breathing_helpers.h"
#include "katra_experience.h"

/* ============================================================================
 * NAMESPACE ISOLATION STATE (Phase 7)
 * ============================================================================ */

/* Next memory isolation settings (one-time use, resets after storage) */
static memory_isolation_t g_next_isolation = ISOLATION_PRIVATE;
static char* g_next_team_name = NULL;
static char** g_next_shared_with = NULL;
static size_t g_next_shared_with_count = 0;

/* Reset isolation settings to defaults */
static void reset_isolation_settings(void) {
    g_next_isolation = ISOLATION_PRIVATE;
    free(g_next_team_name);
    g_next_team_name = NULL;
    katra_free_string_array(g_next_shared_with, g_next_shared_with_count);
    g_next_shared_with = NULL;
    g_next_shared_with_count = 0;
}

/* ============================================================================
 * MEMORY FORMATION HELPERS
 * ============================================================================ */

int breathing_store_typed_memory(const char* ci_id,
                                 memory_type_t type,
                                 const char* content,
                                 float importance,
                                 const char* importance_note,
                                 why_remember_t why_enum,
                                 const char* func_name) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, func_name, "ci_id is NULL");
        return E_INPUT_NULL;
    }

    /* Check breathing layer is initialized before attempting memory operations */
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, func_name, "Breathing layer not initialized");
        return E_INVALID_STATE;
    }

    /* Dedup check: skip exact duplicates within time window (Phase 4.5.1) */
    if (content && KATRA_DEDUP_ENABLED_DEFAULT) {
        dedup_result_t dedup;
        int dedup_result = katra_memory_dedup_check(ci_id, content, 0.0f, &dedup);
        if (dedup_result == KATRA_SUCCESS && dedup.has_exact_duplicate) {
            LOG_DEBUG("Skipping duplicate memory for %s: %.40s...", ci_id, content);
            katra_memory_dedup_result_free(&dedup);
            return KATRA_SUCCESS;  /* Silent success - memory already exists */
        }
        katra_memory_dedup_result_free(&dedup);
    }

    /* Set consent context for this CI's memory access */
    katra_consent_set_context(ci_id);

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

    /* Apply namespace isolation settings (Phase 7) */
    record->isolation = g_next_isolation;
    if (g_next_team_name) {
        record->team_name = strdup(g_next_team_name);
        if (!record->team_name) {
            katra_memory_free_record(record);
            reset_isolation_settings();
            return E_SYSTEM_MEMORY;
        }
    }
    if (g_next_shared_with && g_next_shared_with_count > 0) {
        record->shared_with = malloc(g_next_shared_with_count * sizeof(char*));
        if (!record->shared_with) {
            katra_memory_free_record(record);
            reset_isolation_settings();
            return E_SYSTEM_MEMORY;
        }
        for (size_t i = 0; i < g_next_shared_with_count; i++) {
            record->shared_with[i] = strdup(g_next_shared_with[i]);
            if (!record->shared_with[i]) {
                record->shared_with_count = i;  /* For proper cleanup */
                katra_memory_free_record(record);
                reset_isolation_settings();
                return E_SYSTEM_MEMORY;
            }
        }
        record->shared_with_count = g_next_shared_with_count;
    }
    reset_isolation_settings();  /* Reset after applying */

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

        /* Index for semantic search if enabled (Phase 6.1f) */
        vector_store_t* vector_store = breathing_get_vector_store();
        if (vector_store && content) {
            int vector_result = katra_vector_store(vector_store, record_id, content);
            if (vector_result == KATRA_SUCCESS) {
                LOG_DEBUG("Indexed memory for semantic search: %s", record_id);
            } else {
                LOG_WARN("Failed to index memory for semantic search: %s", record_id);
                /* Non-fatal - continue without semantic indexing */
            }
        }
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

/* ============================================================================
 * NAMESPACE ISOLATION API (Phase 7)
 * ============================================================================ */

int set_memory_isolation(memory_isolation_t isolation, const char* team_name) {
    /* Validate inputs */
    if (isolation == ISOLATION_TEAM && !team_name) {
        katra_report_error(E_INPUT_NULL, "set_memory_isolation",
                          "team_name required for TEAM isolation");
        return E_INPUT_NULL;
    }

    /* Reset any previous settings */
    reset_isolation_settings();

    /* Set new isolation level */
    g_next_isolation = isolation;

    /* Copy team name if provided */
    if (team_name) {
        g_next_team_name = strdup(team_name);
        if (!g_next_team_name) {
            reset_isolation_settings();
            return E_SYSTEM_MEMORY;
        }
    }

    LOG_DEBUG("Set isolation for next memory: %d (team=%s)",
              (int)isolation, team_name ? team_name : "none");
    return KATRA_SUCCESS;
}

int share_memory_with(const char** ci_ids, size_t count) {
    if (!ci_ids || count == 0) {
        katra_report_error(E_INPUT_NULL, "share_memory_with",
                          "ci_ids array cannot be NULL or empty");
        return E_INPUT_NULL;
    }

    /* Free any previous sharing list */
    katra_free_string_array(g_next_shared_with, g_next_shared_with_count);
    g_next_shared_with = NULL;
    g_next_shared_with_count = 0;

    /* Allocate new sharing list */
    g_next_shared_with = malloc(count * sizeof(char*));
    if (!g_next_shared_with) {
        return E_SYSTEM_MEMORY;
    }

    /* Copy CI IDs */
    for (size_t i = 0; i < count; i++) {
        g_next_shared_with[i] = strdup(ci_ids[i]);
        if (!g_next_shared_with[i]) {
            /* Cleanup on failure */
            g_next_shared_with_count = i;
            katra_free_string_array(g_next_shared_with, g_next_shared_with_count);
            g_next_shared_with = NULL;
            g_next_shared_with_count = 0;
            return E_SYSTEM_MEMORY;
        }
    }
    g_next_shared_with_count = count;

    LOG_DEBUG("Set explicit sharing for next memory with %zu CIs", count);
    return KATRA_SUCCESS;
}
