/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_BREATHING_CONTEXT_PERSIST_INTERNAL_H
#define KATRA_BREATHING_CONTEXT_PERSIST_INTERNAL_H

#include <sqlite3.h>
#include <stdbool.h>
#include "katra_limits.h"

/**
 * katra_breathing_context_persist_internal.h - Internal shared state
 *
 * Shared between split context persistence files:
 * - katra_breathing_context_persist.c (main init/capture/restore)
 * - katra_breathing_context_update.c (update functions)
 * - katra_breathing_context_query.c (query functions)
 */

/* ============================================================================
 * SHARED TYPES
 * ============================================================================ */

/* In-memory context snapshot (working state) */
typedef struct {
    char ci_id[KATRA_BUFFER_SMALL];
    char session_id[KATRA_BUFFER_NAME];

    /* Cognitive state */
    char* current_focus;
    char* active_reasoning;
    char** pending_questions;
    size_t pending_question_count;
    size_t pending_question_capacity;

    /* Relationship context */
    char* communication_style;
    char* user_preferences;

    /* Project state */
    char* recent_accomplishments;
    char** modified_files;
    size_t modified_file_count;
    size_t modified_file_capacity;
    char* active_goals;

    /* Self-model */
    char* thinking_patterns;
    char* learned_lessons;

} working_context_t;

/* ============================================================================
 * SHARED GLOBAL STATE ACCESSORS
 * ============================================================================ */

/**
 * Get pointer to global working context
 * Returns NULL if not initialized
 */
working_context_t* context_persist_get_working_context(void);

/**
 * Get pointer to global SQLite database handle
 * Returns NULL if not initialized
 */
sqlite3* context_persist_get_db(void);

/**
 * Check if context persistence is initialized
 */
bool context_persist_is_initialized(void);

/**
 * Set global working context (used by init)
 */
void context_persist_set_working_context(working_context_t* ctx);

/**
 * Set global database handle (used by init)
 */
void context_persist_set_db(sqlite3* db);

/**
 * Set initialized flag (used by init/cleanup)
 */
void context_persist_set_initialized(bool initialized);

/* ============================================================================
 * INTERNAL HELPER FUNCTIONS
 * ============================================================================ */

/**
 * safe_strdup() - Wrapper for strdup with NULL check
 */
char* safe_strdup(const char* s);

/**
 * free_working_context() - Free working context structure
 */
void free_working_context(working_context_t* ctx);

#endif /* KATRA_BREATHING_CONTEXT_PERSIST_INTERNAL_H */
