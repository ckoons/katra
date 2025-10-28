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
#include "katra_breathing_helpers.h"

/* ============================================================================
 * SEMANTIC PARSING - Phrase Lists
 * ============================================================================ */

/* Critical importance phrases */
static const char* CRITICAL_PHRASES[] = {
    "critical", "crucial", "life-changing", "must remember",
    "never forget", "extremely", NULL
};

/* Negation phrases (reduce importance) */
static const char* NEGATION_PHRASES[] = {
    "not important", "unimportant", NULL
};

/* High importance compound phrases (check before simple keywords) */
static const char* HIGH_COMPOUND_PHRASES[] = {
    "very important", "very significant", "very noteworthy",
    "very notable", NULL
};

/* High importance phrases */
static const char* HIGH_PHRASES[] = {
    "significant", "important", "matters", "key", "essential", NULL
};

/* Medium importance phrases */
static const char* MEDIUM_PHRASES[] = {
    "worth remembering", "interesting", "notable",
    "noteworthy", "remember", NULL
};

/* Low importance phrases */
static const char* LOW_PHRASES[] = {
    "routine", "normal", "everyday", "regular", "usual", NULL
};

/* Trivial importance phrases */
static const char* TRIVIAL_PHRASES[] = {
    "trivial", "fleeting", "forget", NULL
};

/* ============================================================================
 * SEMANTIC PARSING
 * ============================================================================ */

float string_to_importance(const char* semantic) {
    if (!semantic) {
        return MEMORY_IMPORTANCE_MEDIUM;  /* Default */
    }

    /* Check compound phrases BEFORE single keywords to avoid false matches */

    /* Critical indicators (check first - highest priority) */
    if (breathing_contains_any_phrase(semantic, CRITICAL_PHRASES)) {
        return MEMORY_IMPORTANCE_CRITICAL;
    }

    /* Check negations early (not important, unimportant) */
    if (breathing_contains_any_phrase(semantic, NEGATION_PHRASES)) {
        return MEMORY_IMPORTANCE_TRIVIAL;
    }

    /* Significant indicators (after negation check) */
    /* Check "very X" compounds first to boost importance */
    if (breathing_contains_any_phrase(semantic, HIGH_COMPOUND_PHRASES)) {
        return MEMORY_IMPORTANCE_HIGH;
    }

    if (breathing_contains_any_phrase(semantic, HIGH_PHRASES)) {
        return MEMORY_IMPORTANCE_HIGH;
    }

    /* Interesting indicators (check compound phrases first) */
    if (breathing_contains_any_phrase(semantic, MEDIUM_PHRASES)) {
        return MEMORY_IMPORTANCE_MEDIUM;
    }

    /* Routine indicators */
    if (breathing_contains_any_phrase(semantic, LOW_PHRASES)) {
        return MEMORY_IMPORTANCE_LOW;
    }

    /* Trivial indicators (check last - after compound phrases) */
    if (breathing_contains_any_phrase(semantic, TRIVIAL_PHRASES)) {
        return MEMORY_IMPORTANCE_TRIVIAL;
    }

    /* Default: interesting/medium importance */
    return MEMORY_IMPORTANCE_MEDIUM;
}

why_remember_t string_to_why_enum(const char* semantic) {
    float importance = string_to_importance(semantic);

    /* Map float importance back to enum */
    if (importance <= MEMORY_IMPORTANCE_TRIVIAL) return WHY_TRIVIAL;
    if (importance <= BREATHING_IMPORTANCE_THRESHOLD_ROUTINE) return WHY_ROUTINE;
    if (importance <= BREATHING_IMPORTANCE_THRESHOLD_INTERESTING) return WHY_INTERESTING;
    if (importance <= MEMORY_IMPORTANCE_HIGH) return WHY_SIGNIFICANT;
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

    /* Store semantic reason as importance note if provided */
    if (why_semantic) {
        record->importance_note = strdup(why_semantic);
        if (!record->importance_note) {
            katra_memory_free_record(record);
            katra_report_error(E_SYSTEM_MEMORY, "remember_semantic",
                              "Failed to duplicate why_semantic");
            return E_SYSTEM_MEMORY;
        }
    }

    /* Add session context if available */
    int session_result = breathing_attach_session(record);
    if (session_result != KATRA_SUCCESS) {
        katra_memory_free_record(record);
        return session_result;
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
    int session_result = breathing_attach_session(record);
    if (session_result != KATRA_SUCCESS) {
        katra_memory_free_record(record);
        return session_result;
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
