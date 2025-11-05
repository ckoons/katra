/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_BREATHING_CONTEXT_PERSIST_H
#define KATRA_BREATHING_CONTEXT_PERSIST_H

#include <stdbool.h>
#include <time.h>
#include "katra_error.h"

/**
 * katra_breathing_context_persist.h - Context Persistence for Session Continuity
 *
 * Enables CI identity continuity across sessions by capturing and restoring:
 * - Cognitive state (current focus, active reasoning, pending questions)
 * - Relationship context (communication style, trust level, preferences)
 * - Project state (recent accomplishments, modified files, active goals)
 * - Self-model (thinking patterns, approach, learned lessons)
 *
 * Architecture:
 * 1. SQLite storage for queryable metadata
 * 2. JSONL snapshots for latent space injection
 * 3. Markdown formatting for MCP resources
 *
 * Integration:
 * - session_start() automatically restores latest snapshot
 * - session_end() automatically captures current state
 * - MCP resource: katra://context/snapshot provides formatted context
 */

/* ============================================================================
 * CONTEXT SNAPSHOT STRUCTURE
 * ============================================================================ */

/**
 * ci_context_snapshot_t - Complete snapshot of CI cognitive state
 *
 * Designed to be serialized to JSONL and injected as latent space context
 * at session startup. Captures both structured metadata and free-form text
 * for maximum flexibility in restoration.
 */
typedef struct {
    /* Identity */
    char snapshot_id[64];               /* Unique snapshot ID */
    char ci_id[64];                     /* Who this context belongs to */
    char session_id[128];               /* Source session ID */
    time_t snapshot_time;               /* When captured */

    /* Cognitive State */
    char* current_focus;                /* What CI is actively working on */
    char* active_reasoning;             /* Mid-thought chains */
    char** pending_questions;           /* Unanswered questions */
    size_t pending_question_count;

    /* Relationship Context */
    char* communication_style;          /* How this CI interacts */
    char* trust_level;                  /* Relationship state */
    char* user_preferences;             /* Known preferences */

    /* Project State */
    char* recent_accomplishments;       /* What was just done */
    char** modified_files;              /* Recent file changes */
    size_t modified_file_count;
    char* active_goals;                 /* Current objectives */

    /* Self-Model */
    char* thinking_patterns;            /* How this CI approaches problems */
    char* learned_lessons;              /* Session-specific insights */

    /* Working Memory Summary */
    char* conversation_summary;         /* Key points from recent dialogue */
    char* context_digest;               /* Compressed working context */

} ci_context_snapshot_t;

/* ============================================================================
 * CONTEXT PERSISTENCE API
 * ============================================================================ */

/**
 * capture_context_snapshot() - Capture current cognitive state
 *
 * Creates a snapshot of the current CI's cognitive state for later restoration.
 * Called automatically by session_end(), or can be called explicitly.
 *
 * Parameters:
 *   ci_id: CI identity
 *   focus_description: Brief description of current focus (or NULL for auto-detect)
 *
 * Returns: KATRA_SUCCESS or error code
 */
int capture_context_snapshot(const char* ci_id, const char* focus_description);

/**
 * restore_context_as_latent_space() - Restore context for session startup
 *
 * Loads the most recent context snapshot and formats it as latent space text
 * for injection into system prompt at session startup.
 *
 * Format: Markdown document with structured sections suitable for LLM consumption.
 *
 * Parameters:
 *   ci_id: CI identity
 *
 * Returns: Allocated markdown string (caller must free), or NULL if no snapshot exists
 */
char* restore_context_as_latent_space(const char* ci_id);

/**
 * update_current_focus() - Update active cognitive focus
 *
 * Call this when CI shifts attention to new task/topic.
 * Automatically captured in next snapshot.
 *
 * Example:
 *   update_current_focus("Debugging tier1 consolidation issue");
 *
 * Returns: KATRA_SUCCESS or error code
 */
int update_current_focus(const char* focus);

/**
 * add_pending_question() - Record unanswered question
 *
 * Captures questions/uncertainties for continuity across sessions.
 * Questions are included in latent space restoration.
 *
 * Example:
 *   add_pending_question("Why does consolidation skip tier2 sometimes?");
 *
 * Returns: KATRA_SUCCESS or error code
 */
int add_pending_question(const char* question);

/**
 * mark_file_modified() - Track file modification for project state
 *
 * Records file changes as part of project state context.
 * Helps CI remember what was recently worked on.
 *
 * Parameters:
 *   file_path: Path to modified file
 *   modification_type: "created", "edited", "deleted"
 *
 * Returns: KATRA_SUCCESS or error code
 */
int mark_file_modified(const char* file_path, const char* modification_type);

/**
 * record_accomplishment() - Log significant accomplishment
 *
 * Captures what CI has achieved for session continuity and self-model updates.
 *
 * Example:
 *   record_accomplishment("Completed context persistence implementation");
 *
 * Returns: KATRA_SUCCESS or error code
 */
int record_accomplishment(const char* accomplishment);

/**
 * update_communication_style() - Update relationship context
 *
 * Captures how CI communicates with user for consistent interaction.
 *
 * Example:
 *   update_communication_style("Direct technical collaboration, uses metaphors");
 *
 * Returns: KATRA_SUCCESS or error code
 */
int update_communication_style(const char* style);

/**
 * update_user_preferences() - Record user preferences
 *
 * Stores known user preferences for persistent personalization.
 *
 * Example:
 *   update_user_preferences("Prefers goto cleanup, no magic numbers, analysis first");
 *
 * Returns: KATRA_SUCCESS or error code
 */
int update_user_preferences(const char* preferences);

/**
 * update_thinking_patterns() - Update self-model
 *
 * Records how this CI approaches problems for identity continuity.
 *
 * Example:
 *   update_thinking_patterns("Systematic phases, verify with tests, extract at 3rd usage");
 *
 * Returns: KATRA_SUCCESS or error code
 */
int update_thinking_patterns(const char* patterns);

/**
 * get_current_focus() - Get active cognitive focus
 *
 * Returns: Current focus string, or NULL if not set
 * Caller must NOT free (returns internal pointer)
 */
const char* get_current_focus_snapshot(const char* ci_id);

/**
 * get_pending_questions() - Get unanswered questions
 *
 * Returns: Array of question strings (caller must call free_memory_list())
 */
char** get_pending_questions_snapshot(const char* ci_id, size_t* count);

/**
 * get_project_state_summary() - Get formatted project state
 *
 * Returns: Allocated string with recent accomplishments and goals
 * Caller must free
 */
char* get_project_state_summary_snapshot(const char* ci_id);

/**
 * get_relationship_context() - Get relationship state
 *
 * Returns: Allocated string with communication style and preferences
 * Caller must free
 */
char* get_relationship_context_snapshot(const char* ci_id);

/**
 * free_context_snapshot() - Free snapshot structure
 *
 * Properly frees all dynamically allocated fields in snapshot.
 */
void free_context_snapshot(ci_context_snapshot_t* snapshot);

/* ============================================================================
 * INTERNAL INITIALIZATION
 * ============================================================================ */

/**
 * context_persist_init() - Initialize context persistence subsystem
 *
 * Called automatically by breathe_init(). Creates SQLite database and tables.
 *
 * Returns: KATRA_SUCCESS or error code
 */
int context_persist_init(const char* ci_id);

/**
 * context_persist_cleanup() - Cleanup context persistence
 *
 * Called automatically by breathe_cleanup(). Closes database connections.
 */
void context_persist_cleanup(void);

#endif /* KATRA_BREATHING_CONTEXT_PERSIST_H */
