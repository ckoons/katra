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

/* GUIDELINE_APPROVED: Semantic phrase arrays for importance detection */
/* Critical importance phrases */
static const char* CRITICAL_PHRASES[] = {
    "critical", "crucial", "life-changing", "must remember", /* GUIDELINE_APPROVED */
    "never forget", "extremely", NULL /* GUIDELINE_APPROVED */
};

/* Negation phrases (reduce importance) */
static const char* NEGATION_PHRASES[] = {
    "not important", "unimportant", NULL /* GUIDELINE_APPROVED */
};

/* High importance compound phrases (check before simple keywords) */
static const char* HIGH_COMPOUND_PHRASES[] = {
    "very important", "very significant", "very noteworthy", /* GUIDELINE_APPROVED */
    "very notable", NULL /* GUIDELINE_APPROVED */
};

/* High importance phrases */
static const char* HIGH_PHRASES[] = {
    "significant", "important", "matters", "key", "essential", NULL /* GUIDELINE_APPROVED */
};

/* Medium importance phrases */
static const char* MEDIUM_PHRASES[] = {
    "worth remembering", "interesting", "notable", /* GUIDELINE_APPROVED */
    "noteworthy", "remember", NULL /* GUIDELINE_APPROVED */
};

/* Low importance phrases */
static const char* LOW_PHRASES[] = {
    "routine", "normal", "everyday", "regular", "usual", NULL /* GUIDELINE_APPROVED */
};

/* Trivial importance phrases */
static const char* TRIVIAL_PHRASES[] = {
    "trivial", "fleeting", "forget", NULL /* GUIDELINE_APPROVED */
};
/* GUIDELINE_APPROVED_END */

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

int remember_semantic(const char* ci_id, const char* thought, const char* why_semantic) {
    if (!ci_id || !thought) {
        katra_report_error(E_INPUT_NULL, "remember_semantic", "NULL parameter");
        return E_INPUT_NULL;
    }

    /* Convert semantic string to importance */
    float importance = string_to_importance(why_semantic);

    /* Create memory record */
    memory_record_t* record = katra_memory_create_record(
        ci_id,
        MEMORY_TYPE_EXPERIENCE,
        thought,
        importance
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "remember_semantic", /* GUIDELINE_APPROVED: function name */
                          "Failed to create record"); /* GUIDELINE_APPROVED: error context */
        return E_SYSTEM_MEMORY;
    }

    /* Store semantic reason as importance note if provided */
    if (why_semantic) {
        record->importance_note = strdup(why_semantic);
        if (!record->importance_note) {
            katra_memory_free_record(record);
            katra_report_error(E_SYSTEM_MEMORY, "remember_semantic", /* GUIDELINE_APPROVED: function name */
                              "Failed to duplicate why_semantic"); /* GUIDELINE_APPROVED: error context */
            return E_SYSTEM_MEMORY;
        }
    }

    /* Add session context if available */
    int session_result = breathing_attach_session(record);
    if (session_result != KATRA_SUCCESS) {
        katra_memory_free_record(record);
        return session_result;
    }

    /* Save record ID and content for auto-edge creation (Phase 6.2) */
    char record_id_copy[256];  /* Match graph_node_t record_id size */
    strncpy(record_id_copy, record->record_id, sizeof(record_id_copy) - 1);
    record_id_copy[sizeof(record_id_copy) - 1] = '\0';

    /* Store memory */
    int result = katra_memory_store(record);

    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Memory stored successfully with semantic importance");
        why_remember_t why_enum = string_to_why_enum(why_semantic);
        breathing_track_semantic_remember(why_enum);

        /* Create automatic graph edges (Phase 6.2) */
        graph_store_t* graph_store = breathing_get_graph_store();
        if (graph_store) {
            vector_store_t* vector_store = breathing_get_vector_store();
            context_config_t* config = breathing_get_config_ptr();
            breathing_create_auto_edges(graph_store, vector_store, config,
                                       record_id_copy, thought);
        }
    }

    katra_memory_free_record(record);
    return result;
}

int remember_with_semantic_note(const char* thought,
                                 const char* why_semantic,
                                 const char* why_note) {
    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "remember_with_semantic_note", /* GUIDELINE_APPROVED: function name */
                          "Breathing layer not initialized"); /* GUIDELINE_APPROVED: error context */
        return E_INVALID_STATE;
    }

    if (!thought || !why_note) {
        katra_report_error(E_INPUT_NULL, "remember_with_semantic_note", /* GUIDELINE_APPROVED: function name */
                          "NULL parameter"); /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    /* Convert semantic string to importance */
    float importance = string_to_importance(why_semantic);

    LOG_DEBUG("Remembering (semantic: '%s' -> %.2f) with note: %s",
             why_semantic ? why_semantic : "default", importance, thought); /* GUIDELINE_APPROVED: fallback string */

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

/* ============================================================================
 * Tag-Based Memory API (Phase 1: Working Memory)
 * ============================================================================ */

/**
 * Helper: Map salience string to visual marker
 *
 * Converts semantic importance strings or visual markers to canonical form.
 */
static const char* map_salience_to_visual(const char* salience) {
    if (!salience) {
        return NULL;  /* Routine importance */
    }

    /* Already a visual marker? Return as-is */
    if (strcmp(salience, SALIENCE_HIGH) == 0 ||
        strcmp(salience, SALIENCE_MEDIUM) == 0 ||
        strcmp(salience, SALIENCE_LOW) == 0) {
        return salience;
    }

    /* Parse semantic string (reuse existing logic) */
    why_remember_t why_enum = string_to_why_enum(salience);
    float importance = why_to_importance(why_enum);

    /* Map to visual marker based on importance thresholds */
    if (importance >= IMPORTANCE_THRESHOLD_HIGH) {
        return SALIENCE_HIGH;
    } else if (importance >= IMPORTANCE_THRESHOLD_MEDIUM) {
        return SALIENCE_MEDIUM;
    } else if (importance >= IMPORTANCE_THRESHOLD_LOW) {
        return SALIENCE_LOW;
    }

    return NULL;  /* Below low threshold = routine */
}

/**
 * Helper: Check if tags array contains a specific tag
 */
static bool has_tag(const char** tags, size_t tag_count, const char* tag_name) {
    if (!tags || !tag_name) {
        return false;
    }

    for (size_t i = 0; i < tag_count; i++) {
        if (tags[i] && strcmp(tags[i], tag_name) == 0) {
            return true;
        }
    }

    return false;
}

int remember_with_tags(const char* ci_id,
                       const char* content,
                       const char** tags,
                       size_t tag_count,
                       const char* salience) {
    KATRA_CHECK_NULL(content);

    /* Validate tag count */
    if (tag_count > KATRA_MAX_TAGS_PER_MEMORY) {
        katra_report_error(E_INPUT_TOO_LARGE, "remember_with_tags",
                          "Tag count exceeds maximum");
        return E_INPUT_TOO_LARGE;
    }

    /* Map salience to visual marker and importance score */
    const char* visual_marker = map_salience_to_visual(salience);
    float importance = why_to_importance(string_to_why_enum(salience));

    /* Create base memory record */
    memory_record_t* record = katra_memory_create_record(
        ci_id,
        MEMORY_TYPE_EXPERIENCE,
        content,
        importance
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "remember_with_tags",
                          "Failed to create memory record");
        return E_SYSTEM_MEMORY;
    }

    int result = KATRA_SUCCESS;

    /* Store tags */
    if (tag_count > 0) {
        record->tags = (char**)calloc(tag_count, sizeof(char*));
        if (!record->tags) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }

        for (size_t i = 0; i < tag_count; i++) {
            if (tags[i]) {
                record->tags[i] = strdup(tags[i]);
                if (!record->tags[i]) {
                    result = E_SYSTEM_MEMORY;
                    goto cleanup;
                }
            }
        }
        record->tag_count = tag_count;
    }

    /* Store visual salience marker */
    if (visual_marker) {
        record->salience_visual = strdup(visual_marker);
        if (!record->salience_visual) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    /* Check for session-scoped tag */
    record->session_scoped = has_tag(tags, tag_count, TAG_SESSION);

    /* Check for permanent tag (affects archival) */
    if (has_tag(tags, tag_count, TAG_PERMANENT)) {
        record->marked_important = true;
    }

    /* Check for personal tag */
    if (has_tag(tags, tag_count, TAG_PERSONAL)) {
        record->personal = true;
    }

    /* Add session context */
    result = breathing_attach_session(record);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Store memory */
    result = katra_memory_store(record);

    if (result == KATRA_SUCCESS) {
        /* Track semantic usage if salience was provided */
        if (salience) {
            why_remember_t why_enum = string_to_why_enum(salience);
            breathing_track_semantic_remember(why_enum);
        }
    }

cleanup:
    katra_memory_free_record(record);
    return result;
}

int decide_with_tags(const char* ci_id,
                     const char* decision,
                     const char* reasoning,
                     const char** tags,
                     size_t tag_count) {
    KATRA_CHECK_NULL(decision);
    KATRA_CHECK_NULL(reasoning);

    /* Validate tag count */
    if (tag_count > KATRA_MAX_TAGS_PER_MEMORY) {
        katra_report_error(E_INPUT_TOO_LARGE, "decide_with_tags",
                          "Tag count exceeds maximum");
        return E_INPUT_TOO_LARGE;
    }

    /* Create decision memory record with high importance */
    memory_record_t* record = katra_memory_create_record(
        ci_id,
        MEMORY_TYPE_DECISION,
        decision,
        MEMORY_IMPORTANCE_HIGH
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "decide_with_tags",
                          "Failed to create memory record");
        return E_SYSTEM_MEMORY;
    }

    int result = KATRA_SUCCESS;

    /* Store reasoning in importance_note field */
    record->importance_note = strdup(reasoning);
    if (!record->importance_note) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    /* Store tags */
    if (tag_count > 0) {
        record->tags = (char**)calloc(tag_count, sizeof(char*));
        if (!record->tags) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }

        for (size_t i = 0; i < tag_count; i++) {
            if (tags[i]) {
                record->tags[i] = strdup(tags[i]);
                if (!record->tags[i]) {
                    result = E_SYSTEM_MEMORY;
                    goto cleanup;
                }
            }
        }
        record->tag_count = tag_count;
    }

    /* Decisions get high visual salience by default */
    record->salience_visual = strdup(SALIENCE_HIGH);
    if (!record->salience_visual) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    /* Check for session-scoped tag */
    record->session_scoped = has_tag(tags, tag_count, TAG_SESSION);

    /* Decisions are important by default, but check for permanent tag */
    if (has_tag(tags, tag_count, TAG_PERMANENT)) {
        record->marked_important = true;
    }

    /* Check for personal tag */
    if (has_tag(tags, tag_count, TAG_PERSONAL)) {
        record->personal = true;
    }

    /* Add session context */
    result = breathing_attach_session(record);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Store memory */
    result = katra_memory_store(record);

cleanup:
    katra_memory_free_record(record);
    return result;
}
