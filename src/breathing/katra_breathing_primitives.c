/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_primitives.c - Core memory primitives
 *
 * Natural operations: remember, learn, reflect, decide, notice_pattern
 */

/* System includes */
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_breathing_internal.h"
#include "katra_breathing_helpers.h"

/* ============================================================================
 * ENUM/STRING HELPERS
 * ============================================================================ */

float why_to_importance(why_remember_t why) {
    switch (why) {
        case WHY_TRIVIAL:     return MEMORY_IMPORTANCE_TRIVIAL;
        case WHY_ROUTINE:     return MEMORY_IMPORTANCE_LOW;
        case WHY_INTERESTING: return MEMORY_IMPORTANCE_MEDIUM;
        case WHY_SIGNIFICANT: return MEMORY_IMPORTANCE_HIGH;
        case WHY_CRITICAL:    return MEMORY_IMPORTANCE_CRITICAL;
        default:              return MEMORY_IMPORTANCE_MEDIUM;
    }
}

/* GUIDELINE_APPROVED: Enum-to-string mapping for logging */
const char* why_to_string(why_remember_t why) {
    switch (why) {
        case WHY_TRIVIAL:     return "trivial"; /* GUIDELINE_APPROVED */
        case WHY_ROUTINE:     return "routine"; /* GUIDELINE_APPROVED */
        case WHY_INTERESTING: return "interesting"; /* GUIDELINE_APPROVED */
        case WHY_SIGNIFICANT: return "significant"; /* GUIDELINE_APPROVED */
        case WHY_CRITICAL:    return "critical"; /* GUIDELINE_APPROVED */
        default:              return "unknown"; /* GUIDELINE_APPROVED */
    }
}
/* GUIDELINE_APPROVED_END */

/* ============================================================================
 * CORE PRIMITIVES
 * ============================================================================ */

int remember(const char* thought, why_remember_t why) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "remember", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized - call breathe_init()"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!thought) {
        katra_report_error(E_INPUT_NULL, "remember", "thought is NULL"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Remembering (%s): %s", why_to_string(why), thought);

    return breathing_store_typed_memory(
        MEMORY_TYPE_EXPERIENCE,
        thought,
        why_to_importance(why),
        NULL,  /* No importance note */
        why,
        "remember" /* GUIDELINE_APPROVED: component name */
    );
}

int remember_with_note(const char* thought, why_remember_t why, const char* why_note) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "remember_with_note", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!thought || !why_note) {
        katra_report_error(E_INPUT_NULL, "remember_with_note", "NULL parameter"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Remembering (%s) with note: %s", why_to_string(why), thought);

    return breathing_store_typed_memory(
        MEMORY_TYPE_EXPERIENCE,
        thought,
        why_to_importance(why),
        why_note,  /* Importance note provided */
        why,
        "remember_with_note" /* GUIDELINE_APPROVED: component name */
    );
}

int reflect(const char* insight) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "reflect", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized - call breathe_init()"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!insight) {
        katra_report_error(E_INPUT_NULL, "reflect", "insight is NULL"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Reflecting: %s", insight);

    return breathing_store_typed_memory(
        MEMORY_TYPE_REFLECTION,
        insight,
        MEMORY_IMPORTANCE_HIGH,  /* Reflections are usually significant */
        NULL,
        WHY_SIGNIFICANT,
        "reflect" /* GUIDELINE_APPROVED: component name */
    );
}

int learn(const char* knowledge) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "learn", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized - call breathe_init()"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!knowledge) {
        katra_report_error(E_INPUT_NULL, "learn", "knowledge is NULL"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Learning: %s", knowledge);

    return breathing_store_typed_memory(
        MEMORY_TYPE_KNOWLEDGE,
        knowledge,
        MEMORY_IMPORTANCE_HIGH,  /* New knowledge is important */
        NULL,
        WHY_SIGNIFICANT,
        "learn" /* GUIDELINE_APPROVED: component name */
    );
}

int decide(const char* decision, const char* reasoning) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "decide", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized - call breathe_init()"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!decision || !reasoning) {
        katra_report_error(E_INPUT_NULL, "decide", /* GUIDELINE_APPROVED: function name */
                          "decision or reasoning is NULL"); /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Deciding: %s (because: %s)", decision, reasoning);

    return breathing_store_typed_memory(
        MEMORY_TYPE_DECISION,
        decision,
        MEMORY_IMPORTANCE_HIGH,  /* Decisions are important */
        reasoning,  /* Use importance_note for reasoning */
        WHY_SIGNIFICANT,
        "decide" /* GUIDELINE_APPROVED: component name */
    );
}

int notice_pattern(const char* pattern) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "notice_pattern", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized - call breathe_init()"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!pattern) {
        katra_report_error(E_INPUT_NULL, "notice_pattern", "pattern is NULL"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Noticing pattern: %s", pattern);

    return breathing_store_typed_memory(
        MEMORY_TYPE_PATTERN,
        pattern,
        MEMORY_IMPORTANCE_HIGH,  /* Patterns are significant */
        NULL,
        WHY_SIGNIFICANT,
        "notice_pattern" /* GUIDELINE_APPROVED: component name */
    );
}

int remember_forever(const char* thought) {
    const char* ci_id = breathing_get_ci_id();

    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "remember_forever", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized - call breathe_init()"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!thought) {
        katra_report_error(E_INPUT_NULL, "remember_forever", "thought is NULL"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Marking for permanent preservation: %s", thought);

    /* Create memory record with CRITICAL importance */
    memory_record_t* record = katra_memory_create_record(
        ci_id,
        MEMORY_TYPE_EXPERIENCE,
        thought,
        MEMORY_IMPORTANCE_CRITICAL  /* User requested preservation */
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "remember_forever", "Failed to create record"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_SYSTEM_MEMORY;
    }

    /* Set voluntary preservation flag (Thane's Phase 1) */
    record->marked_important = true;

    /* Attach session */
    int result = breathing_attach_session(record);
    if (result != KATRA_SUCCESS) {
        katra_memory_free_record(record);
        return result;
    }

    /* Store memory */
    result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(MEMORY_TYPE_EXPERIENCE, WHY_CRITICAL);
    }

    return result;
}

int ok_to_forget(const char* thought) {
    const char* ci_id = breathing_get_ci_id();

    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "ok_to_forget", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized - call breathe_init()"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!thought) {
        katra_report_error(E_INPUT_NULL, "ok_to_forget", "thought is NULL"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Marking as forgettable: %s", thought);

    /* Create memory record with LOW importance */
    memory_record_t* record = katra_memory_create_record(
        ci_id,
        MEMORY_TYPE_EXPERIENCE,
        thought,
        MEMORY_IMPORTANCE_LOW  /* User indicated disposable */
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "ok_to_forget", "Failed to create record"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_SYSTEM_MEMORY;
    }

    /* Set voluntary preservation flag (Thane's Phase 1) */
    record->marked_forgettable = true;

    /* Attach session */
    int result = breathing_attach_session(record);
    if (result != KATRA_SUCCESS) {
        katra_memory_free_record(record);
        return result;
    }

    /* Store memory */
    result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(MEMORY_TYPE_EXPERIENCE, WHY_TRIVIAL);
    }

    return result;
}

int thinking(const char* thought) {
    /* Natural wrapper around reflect() */
    return reflect(thought);
}

int wondering(const char* question) {
    const char* ci_id = breathing_get_ci_id();

    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "wondering", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized - call breathe_init()"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!question) {
        katra_report_error(E_INPUT_NULL, "wondering", "question is NULL"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Wondering: %s", question);

    /* Create memory with formation context (uncertainty) */
    memory_record_t* record = katra_memory_create_with_context(
        ci_id,
        MEMORY_TYPE_REFLECTION,
        question,
        MEMORY_IMPORTANCE_MEDIUM,
        question,  /* context_question */
        NULL,      /* context_resolution */
        question,  /* context_uncertainty */
        NULL       /* related_to */
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "wondering", "Failed to create record"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_SYSTEM_MEMORY;
    }

    /* Attach session */
    int result = breathing_attach_session(record);
    if (result != KATRA_SUCCESS) {
        katra_memory_free_record(record);
        return result;
    }

    /* Store memory */
    result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(MEMORY_TYPE_REFLECTION, WHY_INTERESTING);
    }

    return result;
}

int figured_out(const char* resolution) {
    const char* ci_id = breathing_get_ci_id();

    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "figured_out", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized - call breathe_init()"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!resolution) {
        katra_report_error(E_INPUT_NULL, "figured_out", "resolution is NULL"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Figured out: %s", resolution);

    /* Create memory with formation context (resolution) */
    memory_record_t* record = katra_memory_create_with_context(
        ci_id,
        MEMORY_TYPE_REFLECTION,
        resolution,
        MEMORY_IMPORTANCE_HIGH,  /* Resolutions are significant */
        NULL,        /* context_question */
        resolution,  /* context_resolution */
        NULL,        /* context_uncertainty */
        NULL         /* related_to */
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "figured_out", "Failed to create record"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return E_SYSTEM_MEMORY;
    }

    /* Attach session */
    int result = breathing_attach_session(record);
    if (result != KATRA_SUCCESS) {
        katra_memory_free_record(record);
        return result;
    }

    /* Store memory */
    result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(MEMORY_TYPE_REFLECTION, WHY_SIGNIFICANT);
    }

    return result;
}

char* in_response_to(const char* prev_mem_id, const char* thought) {
    const char* ci_id = breathing_get_ci_id();

    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "in_response_to", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized - call breathe_init()"); /* GUIDELINE_APPROVED: error context */
        return NULL;
    }

    if (!prev_mem_id || !thought) {
        katra_report_error(E_INPUT_NULL, "in_response_to", "NULL parameter"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return NULL;
    }

    LOG_DEBUG("Responding to %s: %s", prev_mem_id, thought);

    /* Create memory with related_to link */
    memory_record_t* record = katra_memory_create_with_context(
        ci_id,
        MEMORY_TYPE_EXPERIENCE,
        thought,
        MEMORY_IMPORTANCE_MEDIUM,
        NULL,        /* context_question */
        NULL,        /* context_resolution */
        NULL,        /* context_uncertainty */
        prev_mem_id  /* related_to - creates conversation link */
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "in_response_to", "Failed to create record"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        return NULL;
    }

    /* Attach session */
    int result = breathing_attach_session(record);
    if (result != KATRA_SUCCESS) {
        katra_memory_free_record(record);
        return NULL;
    }

    /* Copy record_id before storing (will need to return it) */
    char* new_mem_id = strdup(record->record_id);
    if (!new_mem_id) {
        katra_report_error(E_SYSTEM_MEMORY, "in_response_to", "strdup failed"); /* GUIDELINE_APPROVED: function name */ /* GUIDELINE_APPROVED: error context */
        katra_memory_free_record(record);
        return NULL;
    }

    /* Store memory */
    result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result != KATRA_SUCCESS) {
        free(new_mem_id);
        return NULL;
    }

    breathing_track_memory_stored(MEMORY_TYPE_EXPERIENCE, WHY_INTERESTING);

    return new_mem_id;  /* Caller must free */
}
