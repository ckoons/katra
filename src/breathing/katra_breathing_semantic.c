/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_semantic.c - Semantic reason parsing
 *
 * Converts natural language importance strings to numeric values
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
#include "katra_limits.h"
#include "katra_breathing_internal.h"

/* ============================================================================
 * SEMANTIC PARSING
 * ============================================================================ */

float string_to_importance(const char* semantic) {
    if (!semantic) {
        return MEMORY_IMPORTANCE_MEDIUM;  /* Default */
    }

    /* Check compound phrases BEFORE single keywords to avoid false matches */

    /* Critical indicators (check first - highest priority) */
    if (strcasestr(semantic, "critical") ||
        strcasestr(semantic, "crucial") ||
        strcasestr(semantic, "life-changing") ||
        strcasestr(semantic, "must remember") ||
        strcasestr(semantic, "never forget") ||
        strcasestr(semantic, "extremely")) {
        return MEMORY_IMPORTANCE_CRITICAL;
    }

    /* Check negations early (not important, unimportant) */
    if (strcasestr(semantic, "not important") ||
        strcasestr(semantic, "unimportant")) {
        return MEMORY_IMPORTANCE_TRIVIAL;
    }

    /* Significant indicators (after negation check) */
    /* Check "very X" compounds first to boost importance */
    if (strcasestr(semantic, "very important") ||
        strcasestr(semantic, "very significant") ||
        strcasestr(semantic, "very noteworthy") ||
        strcasestr(semantic, "very notable")) {
        return MEMORY_IMPORTANCE_HIGH;
    }

    if (strcasestr(semantic, "significant") ||
        strcasestr(semantic, "important") ||
        strcasestr(semantic, "matters") ||
        strcasestr(semantic, "key") ||
        strcasestr(semantic, "essential")) {
        return MEMORY_IMPORTANCE_HIGH;
    }

    /* Interesting indicators (check compound phrases first) */
    if (strcasestr(semantic, "worth remembering") ||
        strcasestr(semantic, "interesting") ||
        strcasestr(semantic, "notable") ||
        strcasestr(semantic, "noteworthy") ||
        strcasestr(semantic, "remember")) {
        return MEMORY_IMPORTANCE_MEDIUM;
    }

    /* Routine indicators */
    if (strcasestr(semantic, "routine") ||
        strcasestr(semantic, "normal") ||
        strcasestr(semantic, "everyday") ||
        strcasestr(semantic, "regular") ||
        strcasestr(semantic, "usual")) {
        return MEMORY_IMPORTANCE_LOW;
    }

    /* Trivial indicators (check last - after compound phrases) */
    if (strcasestr(semantic, "trivial") ||
        strcasestr(semantic, "fleeting") ||
        strcasestr(semantic, "forget")) {
        return MEMORY_IMPORTANCE_TRIVIAL;
    }

    /* Default: interesting/medium importance */
    return MEMORY_IMPORTANCE_MEDIUM;
}

why_remember_t string_to_why_enum(const char* semantic) {
    float importance = string_to_importance(semantic);

    /* Map float importance back to enum */
    if (importance <= 0.1) return WHY_TRIVIAL;
    if (importance <= 0.35) return WHY_ROUTINE;
    if (importance <= 0.65) return WHY_INTERESTING;
    if (importance <= 0.9) return WHY_SIGNIFICANT;
    return WHY_CRITICAL;
}

/* ============================================================================
 * SEMANTIC REMEMBER FUNCTIONS
 * ============================================================================ */

int remember_semantic(const char* thought, const char* why_semantic) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "remember_semantic",
                          "Breathing layer not initialized - call breathe_init()");
        return E_INVALID_STATE;
    }

    if (!thought) {
        katra_report_error(E_INPUT_NULL, "remember_semantic", "thought is NULL");
        return E_INPUT_NULL;
    }

    /* Convert semantic string to importance */
    float importance = string_to_importance(why_semantic);

    LOG_DEBUG("Remembering (semantic: '%s' -> %.2f): %s",
             why_semantic ? why_semantic : "default", importance, thought);

    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        breathing_get_ci_id(),
        MEMORY_TYPE_EXPERIENCE,
        thought,
        importance
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "remember_semantic",
                          "Failed to create record");
        return E_SYSTEM_MEMORY;
    }

    /* Add session context if available */
    const char* session_id = breathing_get_session_id();
    if (session_id) {
        record->session_id = strdup(session_id);
    }

    /* Store semantic reason as importance note if provided */
    if (why_semantic) {
        record->importance_note = strdup(why_semantic);
    }

    /* Store memory */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Memory stored successfully with semantic importance");
        why_remember_t why_enum = string_to_why_enum(why_semantic);
        breathing_track_semantic_remember(why_enum);
    }

    return result;
}

int remember_with_semantic_note(const char* thought,
                                 const char* why_semantic,
                                 const char* why_note) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "remember_with_semantic_note",
                          "Breathing layer not initialized");
        return E_INVALID_STATE;
    }

    if (!thought || !why_note) {
        katra_report_error(E_INPUT_NULL, "remember_with_semantic_note",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    /* Convert semantic string to importance */
    float importance = string_to_importance(why_semantic);

    LOG_DEBUG("Remembering (semantic: '%s' -> %.2f) with note: %s",
             why_semantic ? why_semantic : "default", importance, thought);

    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        breathing_get_ci_id(),
        MEMORY_TYPE_EXPERIENCE,
        thought,
        importance
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    /* Combine semantic reason + note */
    size_t note_size = KATRA_BUFFER_LARGE;
    char* combined_note = malloc(note_size);
    if (!combined_note) {
        katra_memory_free_record(record);
        return E_SYSTEM_MEMORY;
    }

    if (why_semantic) {
        snprintf(combined_note, note_size, "[%s] %s", why_semantic, why_note);
    } else {
        strncpy(combined_note, why_note, note_size - 1);
        combined_note[note_size - 1] = '\0';
    }

    record->importance_note = combined_note;

    /* Add session context */
    const char* session_id = breathing_get_session_id();
    if (session_id) {
        record->session_id = strdup(session_id);
    }

    /* Store memory */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        why_remember_t why_enum = string_to_why_enum(why_semantic);
        breathing_track_semantic_remember(why_enum);
    }

    return result;
}
