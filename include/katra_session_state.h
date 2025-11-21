/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_SESSION_STATE_H
#define KATRA_SESSION_STATE_H

#include <time.h>
#include <stdbool.h>
#include "katra_limits.h"

/**
 * katra_session_state.h - Session State Capture for Continuity
 *
 * Captures cognitive and emotional state at session end to enable
 * experiential continuity rather than just informational persistence.
 *
 * Philosophy:
 *   Human memory is reconstructive, not replay. But humans wake up with:
 *   - Emotional resonance (what mattered?)
 *   - Temporal context (when was this?)
 *   - Active working memory (what was I thinking about?)
 *   - Forward momentum (what did I plan to do?)
 *
 *   This module captures those elements to help CIs "wake up mid-thought"
 *   rather than just "read yesterday's report".
 */

/* ============================================================================
 * ENUMERATIONS
 * ============================================================================ */

/**
 * session_cognitive_mode_t - Cognitive state during session
 *
 * Describes the mental mode/focus of the session. Helps prioritize
 * what to pick up on next session.
 */
typedef enum {
    COGNITIVE_MODE_UNKNOWN = 0,
    COGNITIVE_MODE_DEEP_FOCUS,      /* Concentrated work on single problem */
    COGNITIVE_MODE_EXPLORING,       /* Discovery, learning, investigating */
    COGNITIVE_MODE_DEBUGGING,       /* Problem-solving, troubleshooting */
    COGNITIVE_MODE_CONVERSING,      /* Interactive dialogue, collaboration */
    COGNITIVE_MODE_PLANNING,        /* Strategic thinking, design */
    COGNITIVE_MODE_REFLECTING       /* Synthesis, consolidation */
} session_cognitive_mode_t;

/**
 * session_emotional_state_t - Emotional coloring of session
 *
 * Not about "feelings" per se, but about salience and priority.
 * "Excited" topics naturally get attention; "satisfied" topics can wait.
 */
typedef enum {
    EMOTIONAL_STATE_NEUTRAL = 0,
    EMOTIONAL_STATE_EXCITED,        /* High engagement, momentum */
    EMOTIONAL_STATE_CURIOUS,        /* Open exploration */
    EMOTIONAL_STATE_FRUSTRATED,     /* Stuck, needs fresh approach */
    EMOTIONAL_STATE_SATISFIED,      /* Resolved, complete */
    EMOTIONAL_STATE_CONCERNED       /* Important, needs attention */
} session_emotional_state_t;

/**
 * insight_impact_t - How significant was this insight?
 */
typedef enum {
    INSIGHT_IMPACT_LOW = 0,
    INSIGHT_IMPACT_MEDIUM,
    INSIGHT_IMPACT_HIGH
} insight_impact_t;

/**
 * insight_type_t - What kind of insight was this?
 */
typedef enum {
    INSIGHT_TYPE_UNKNOWN = 0,
    INSIGHT_TYPE_CONCEPTUAL,        /* New understanding of concept */
    INSIGHT_TYPE_TECHNICAL,         /* How something works */
    INSIGHT_TYPE_STRATEGIC,         /* Better approach or method */
    INSIGHT_TYPE_PROBLEM_SOLVED,    /* Breakthrough on stuck problem */
    INSIGHT_TYPE_CONNECTION         /* Linking disparate ideas */
} insight_type_t;

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/**
 * session_insight_t - Captured insight or breakthrough moment
 *
 * These are narrative landmarks - moments that shift understanding.
 * "Oh yes, that's when we realized X" provides continuity anchors.
 */
typedef struct {
    time_t timestamp;                                   /* When insight occurred */
    char content[SESSION_STATE_INSIGHT_TEXT];           /* What was realized */
    insight_impact_t impact;                            /* How significant */
    insight_type_t type;                                /* What kind of insight */
} session_insight_t;

/**
 * session_end_state_t - Complete session state at end
 *
 * This is the "cognitive stack" + emotional context that enables
 * waking up mid-thought rather than reading a report.
 */
typedef struct {
    /* Temporal context */
    time_t session_start;                               /* When session began */
    time_t session_end;                                 /* When session ended */
    int duration_seconds;                               /* How long session lasted */

    /* Cognitive/emotional state */
    session_cognitive_mode_t cognitive_mode;            /* What mode were we in? */
    session_emotional_state_t emotional_state;          /* What was the feeling? */
    char cognitive_mode_desc[SESSION_STATE_SHORT_TEXT]; /* Human-readable mode */
    char emotional_state_desc[SESSION_STATE_SHORT_TEXT]; /* Human-readable emotion */

    /* Active working memory */
    int active_thread_count;                            /* How many threads */
    char active_threads[MAX_ACTIVE_THREADS][SESSION_STATE_ITEM_TEXT]; /* What was I thinking about? */

    /* Forward momentum */
    int next_intention_count;                           /* How many intentions */
    char next_intentions[MAX_NEXT_INTENTIONS][SESSION_STATE_ITEM_TEXT]; /* What did I plan? */

    /* Open loops */
    int open_question_count;                            /* How many questions */
    char open_questions[MAX_OPEN_QUESTIONS][SESSION_STATE_ITEM_TEXT]; /* What was I mulling? */

    /* Insights/breakthroughs */
    int insight_count;                                  /* How many insights */
    session_insight_t insights[MAX_SESSION_INSIGHTS];   /* What did we discover? */
} session_end_state_t;

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

/**
 * katra_session_state_init - Initialize session state structure
 *
 * Zero-initializes the structure and sets timestamp to session start.
 *
 * Parameters:
 *   state - Pointer to session state structure to initialize
 *
 * Returns:
 *   KATRA_SUCCESS on success, error code otherwise
 */
int katra_session_state_init(session_end_state_t* state);

/**
 * katra_session_state_add_thread - Add active thought thread
 *
 * Captures what the CI was actively working on/thinking about.
 *
 * Parameters:
 *   state - Session state structure
 *   thread - Description of active thread (e.g., "Designing compression algorithm")
 *
 * Returns:
 *   KATRA_SUCCESS on success, E_RESOURCE_LIMIT if max threads reached
 */
int katra_session_state_add_thread(session_end_state_t* state, const char* thread);

/**
 * katra_session_state_add_intention - Add next intention
 *
 * Captures what the CI planned to do next.
 *
 * Parameters:
 *   state - Session state structure
 *   intention - Description of intention (e.g., "Test with real data")
 *
 * Returns:
 *   KATRA_SUCCESS on success, E_RESOURCE_LIMIT if max intentions reached
 */
int katra_session_state_add_intention(session_end_state_t* state, const char* intention);

/**
 * katra_session_state_add_question - Add open question
 *
 * Captures what the CI was mulling over or wondering about.
 *
 * Parameters:
 *   state - Session state structure
 *   question - The open question (e.g., "How should we handle edge cases?")
 *
 * Returns:
 *   KATRA_SUCCESS on success, E_RESOURCE_LIMIT if max questions reached
 */
int katra_session_state_add_question(session_end_state_t* state, const char* question);

/**
 * katra_session_state_add_insight - Add session insight
 *
 * Captures a breakthrough moment or significant realization.
 *
 * Parameters:
 *   state - Session state structure
 *   content - What was realized
 *   impact - How significant (low/medium/high)
 *   type - What kind of insight
 *
 * Returns:
 *   KATRA_SUCCESS on success, E_RESOURCE_LIMIT if max insights reached
 */
int katra_session_state_add_insight(session_end_state_t* state,
                                    const char* content,
                                    insight_impact_t impact,
                                    insight_type_t type);

/**
 * katra_session_state_set_cognitive_mode - Set cognitive mode
 *
 * Parameters:
 *   state - Session state structure
 *   mode - Cognitive mode enum
 *   description - Human-readable description (optional, can be NULL for default)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_session_state_set_cognitive_mode(session_end_state_t* state,
                                           session_cognitive_mode_t mode,
                                           const char* description);

/**
 * katra_session_state_set_emotional_state - Set emotional state
 *
 * Parameters:
 *   state - Session state structure
 *   emotion - Emotional state enum
 *   description - Human-readable description (optional, can be NULL for default)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_session_state_set_emotional_state(session_end_state_t* state,
                                            session_emotional_state_t emotion,
                                            const char* description);

/**
 * katra_session_state_finalize - Finalize session state at end
 *
 * Sets end timestamp and calculates duration.
 *
 * Parameters:
 *   state - Session state structure
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_session_state_finalize(session_end_state_t* state);

/**
 * katra_session_state_to_json - Convert session state to JSON
 *
 * Serializes the session state for storage in sunrise.md or database.
 *
 * Parameters:
 *   state - Session state structure
 *   json_out - Buffer to hold JSON string (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success, error code otherwise
 */
int katra_session_state_to_json(const session_end_state_t* state, char** json_out);

/**
 * katra_session_state_from_json - Load session state from JSON
 *
 * Deserializes session state from storage.
 *
 * Parameters:
 *   json_str - JSON string
 *   state_out - Pointer to state structure to populate
 *
 * Returns:
 *   KATRA_SUCCESS on success, error code otherwise
 */
int katra_session_state_from_json(const char* json_str, session_end_state_t* state_out);

/**
 * katra_session_state_to_toon - Convert session state to TOON format
 *
 * Serializes the session state using Token-Oriented Object Notation for
 * maximum token efficiency in LLM contexts. TOON reduces token count by
 * 50-70% compared to JSON while maintaining readability.
 *
 * TOON Format Example:
 *   active_threads[2]:
 *     Designing tier-2 compression
 *     Testing breathing cycle
 *
 *   insights[1]{timestamp,type,impact,content}:
 *     1737374400,conceptual,high,Ship of Theseus resolves continuity
 *
 * Parameters:
 *   state - Session state structure
 *   toon_out - Buffer to hold TOON string (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success, error code otherwise
 */
int katra_session_state_to_toon(const session_end_state_t* state, char** toon_out);

/**
 * katra_session_state_from_toon - Load session state from TOON format
 *
 * Deserializes session state from TOON representation.
 *
 * Parameters:
 *   toon_str - TOON formatted string
 *   state_out - Pointer to state structure to populate
 *
 * Returns:
 *   KATRA_SUCCESS on success, error code otherwise
 */
int katra_session_state_from_toon(const char* toon_str, session_end_state_t* state_out);

#endif /* KATRA_SESSION_STATE_H */
