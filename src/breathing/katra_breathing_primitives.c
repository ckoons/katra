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

    return breathing_store_typed_memory(
        MEMORY_TYPE_EXPERIENCE,
        thought,
        why_to_importance(why),
        NULL,  /* No importance note */
        why,
        "remember"
    );
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

    return breathing_store_typed_memory(
        MEMORY_TYPE_EXPERIENCE,
        thought,
        why_to_importance(why),
        why_note,  /* Importance note provided */
        why,
        "remember_with_note"
    );
}

int reflect(const char* insight) {
    if (!breathing_get_initialized() || !insight) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Reflecting: %s", insight);

    return breathing_store_typed_memory(
        MEMORY_TYPE_REFLECTION,
        insight,
        MEMORY_IMPORTANCE_HIGH,  /* Reflections are usually significant */
        NULL,
        WHY_SIGNIFICANT,
        "reflect"
    );
}

int learn(const char* knowledge) {
    if (!breathing_get_initialized() || !knowledge) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Learning: %s", knowledge);

    return breathing_store_typed_memory(
        MEMORY_TYPE_KNOWLEDGE,
        knowledge,
        MEMORY_IMPORTANCE_HIGH,  /* New knowledge is important */
        NULL,
        WHY_SIGNIFICANT,
        "learn"
    );
}

int decide(const char* decision, const char* reasoning) {
    if (!breathing_get_initialized() || !decision || !reasoning) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Deciding: %s (because: %s)", decision, reasoning);

    return breathing_store_typed_memory(
        MEMORY_TYPE_DECISION,
        decision,
        MEMORY_IMPORTANCE_HIGH,  /* Decisions are important */
        reasoning,  /* Use importance_note for reasoning */
        WHY_SIGNIFICANT,
        "decide"
    );
}

int notice_pattern(const char* pattern) {
    if (!breathing_get_initialized() || !pattern) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Noticing pattern: %s", pattern);

    return breathing_store_typed_memory(
        MEMORY_TYPE_PATTERN,
        pattern,
        MEMORY_IMPORTANCE_HIGH,  /* Patterns are significant */
        NULL,
        WHY_SIGNIFICANT,
        "notice_pattern"
    );
}
