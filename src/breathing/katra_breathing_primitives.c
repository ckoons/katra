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

const char* why_to_string(why_remember_t why) {
    switch (why) {
        case WHY_TRIVIAL:     return "trivial";
        case WHY_ROUTINE:     return "routine";
        case WHY_INTERESTING: return "interesting";
        case WHY_SIGNIFICANT: return "significant";
        case WHY_CRITICAL:    return "critical";
        default:              return "unknown";
    }
}

/* ============================================================================
 * CORE PRIMITIVES
 * ============================================================================ */

int remember(const char* thought, why_remember_t why) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "remember",
                          "Breathing layer not initialized - call breathe_init()");
        return E_INVALID_STATE;
    }

    if (!thought) {
        katra_report_error(E_INPUT_NULL, "remember", "thought is NULL");
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Remembering (%s): %s", why_to_string(why), thought);

    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        breathing_get_ci_id(),
        MEMORY_TYPE_EXPERIENCE,
        thought,
        why_to_importance(why)
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "remember", "Failed to create record");
        return E_SYSTEM_MEMORY;
    }

    /* Add session context if available */
    const char* session_id = breathing_get_session_id();
    if (session_id) {
        record->session_id = strdup(session_id);
    }

    /* Store memory */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Memory stored successfully");
        breathing_track_memory_stored(MEMORY_TYPE_EXPERIENCE, why);
    }

    return result;
}

int remember_with_note(const char* thought, why_remember_t why, const char* why_note) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "remember_with_note",
                          "Breathing layer not initialized");
        return E_INVALID_STATE;
    }

    if (!thought || !why_note) {
        katra_report_error(E_INPUT_NULL, "remember_with_note", "NULL parameter");
        return E_INPUT_NULL;
    }

    LOG_DEBUG("Remembering (%s) with note: %s", why_to_string(why), thought);

    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        breathing_get_ci_id(),
        MEMORY_TYPE_EXPERIENCE,
        thought,
        why_to_importance(why)
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    /* Add importance note */
    record->importance_note = strdup(why_note);
    if (!record->importance_note) {
        katra_memory_free_record(record);
        return E_SYSTEM_MEMORY;
    }

    /* Add session context */
    const char* session_id = breathing_get_session_id();
    if (session_id) {
        record->session_id = strdup(session_id);
    }

    /* Store memory */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(MEMORY_TYPE_EXPERIENCE, why);
    }

    return result;
}

int reflect(const char* insight) {
    if (!breathing_get_initialized() || !insight) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Reflecting: %s", insight);

    memory_record_t* record = katra_memory_create_record(
        breathing_get_ci_id(),
        MEMORY_TYPE_REFLECTION,
        insight,
        MEMORY_IMPORTANCE_HIGH  /* Reflections are usually significant */
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    const char* session_id = breathing_get_session_id();
    if (session_id) {
        record->session_id = strdup(session_id);
    }

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(MEMORY_TYPE_REFLECTION, WHY_SIGNIFICANT);
    }

    return result;
}

int learn(const char* knowledge) {
    if (!breathing_get_initialized() || !knowledge) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Learning: %s", knowledge);

    memory_record_t* record = katra_memory_create_record(
        breathing_get_ci_id(),
        MEMORY_TYPE_KNOWLEDGE,
        knowledge,
        MEMORY_IMPORTANCE_HIGH  /* New knowledge is important */
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    const char* session_id = breathing_get_session_id();
    if (session_id) {
        record->session_id = strdup(session_id);
    }

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(MEMORY_TYPE_KNOWLEDGE, WHY_SIGNIFICANT);
    }

    return result;
}

int decide(const char* decision, const char* reasoning) {
    if (!breathing_get_initialized() || !decision || !reasoning) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Deciding: %s (because: %s)", decision, reasoning);

    memory_record_t* record = katra_memory_create_record(
        breathing_get_ci_id(),
        MEMORY_TYPE_DECISION,
        decision,
        MEMORY_IMPORTANCE_HIGH  /* Decisions are important */
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    /* Use importance_note for reasoning */
    record->importance_note = strdup(reasoning);
    if (!record->importance_note) {
        katra_memory_free_record(record);
        return E_SYSTEM_MEMORY;
    }

    const char* session_id = breathing_get_session_id();
    if (session_id) {
        record->session_id = strdup(session_id);
    }

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(MEMORY_TYPE_DECISION, WHY_SIGNIFICANT);
    }

    return result;
}

int notice_pattern(const char* pattern) {
    if (!breathing_get_initialized() || !pattern) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Noticing pattern: %s", pattern);

    memory_record_t* record = katra_memory_create_record(
        breathing_get_ci_id(),
        MEMORY_TYPE_PATTERN,
        pattern,
        MEMORY_IMPORTANCE_HIGH  /* Patterns are significant */
    );

    if (!record) {
        return E_SYSTEM_MEMORY;
    }

    const char* session_id = breathing_get_session_id();
    if (session_id) {
        record->session_id = strdup(session_id);
    }

    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(MEMORY_TYPE_PATTERN, WHY_SIGNIFICANT);
    }

    return result;
}
