/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_LIFECYCLE_H
#define KATRA_LIFECYCLE_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <pthread.h>
#include "katra_session_state.h"

/**
 * katra_lifecycle.h - Autonomic Breathing and Lifecycle Management
 *
 * This layer provides natural autonomic awareness for CIs through rhythmic
 * "breathing" - checking for ambient state (messages, context changes) at
 * a natural ~2 breaths per minute rhythm.
 *
 * Key Design Principles:
 * 1. Hooks call katra_breath() frequently (every session/turn boundary)
 * 2. Function rate-limits internally (~30 seconds between actual checks)
 * 3. Returns cached context if called more frequently
 * 4. Explicit operations (katra_hear) bypass rate limiting
 * 5. Natural breathing rhythm without hyperventilation
 *
 * Philosophy:
 *   Like humans breathe ~7 times per minute (life-critical, autonomic),
 *   CIs breathe ~2 times per minute (awareness, ambient).
 *
 *   Breathing is autonomic - it "just happens" without conscious effort.
 */

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

/* Default breathing interval in seconds (~30s = 2 breaths/minute) */
#define KATRA_BREATH_INTERVAL_DEFAULT 30

/* Environment variable to override breath interval */
#define KATRA_ENV_BREATH_INTERVAL "KATRA_BREATH_INTERVAL"

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/**
 * breath_context_t - Ambient awareness context returned by katra_breath()
 *
 * Provides non-intrusive awareness of CI state without requiring explicit
 * checks. Hooks can log this information or pass to CI as ambient context.
 */
typedef struct {
    size_t unread_messages;        /* Number of messages waiting */
    time_t last_checkpoint;        /* Time since last checkpoint */
    bool needs_consolidation;      /* Memory consolidation recommended */
    time_t last_breath;            /* When this context was generated */
    /* Future: other autonomic state hints */
} breath_context_t;

/* Forward declaration for turn context
 * The actual type is defined in katra_sunrise_sunset.h
 * We use void* here to avoid circular dependency */

/**
 * session_state_t - In-memory session state for autonomic breathing
 *
 * One per MCP server process. Tracks breathing state across multiple
 * katra_breath() calls to implement rate limiting.
 *
 * Thread-safe: Protected by breath_lock mutex.
 */
typedef struct {
    /* Breathing state */
    time_t last_breath_time;       /* When last actual breath occurred */
    breath_context_t cached_context; /* Cached context from last breath */
    pthread_mutex_t breath_lock;   /* Protects breath state */

    /* Configuration */
    int breath_interval;           /* Seconds between breaths (default: 30) */
    bool breathing_enabled;        /* Can be disabled for testing */

    /* Session identity */
    char* ci_id;                   /* Current CI identity */
    char* session_id;              /* Current session ID */
    bool session_active;           /* True if session is running */

    /* Persona info (for auto-registration) */
    char* persona_name;            /* Persistent persona name */
    char* persona_role;            /* CI role (developer, researcher, etc.) */

    /* Turn-level context (Phase 10) */
    int current_turn_number;       /* Current turn counter */
    void* current_turn_context;    /* Surfaced memories (turn_context_t*) */
    char* last_turn_input;         /* Input that triggered current context */
} session_state_t;

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

/**
 * katra_lifecycle_init() - Initialize lifecycle layer
 *
 * Reads environment variables and sets up defaults for autonomic breathing.
 * Must be called before any other lifecycle functions.
 *
 * Environment variables:
 *   KATRA_BREATH_INTERVAL - Override default breathing interval (seconds)
 *
 * Returns:
 *   KATRA_SUCCESS - Initialization successful
 *   E_ALREADY_INITIALIZED - Already initialized
 *   E_SYSTEM_MEMORY - Failed to allocate state
 */
int katra_lifecycle_init(void);

/**
 * katra_lifecycle_cleanup() - Cleanup lifecycle layer
 *
 * Frees resources allocated by katra_lifecycle_init().
 * Safe to call even if not initialized.
 */
void katra_lifecycle_cleanup(void);

/* ============================================================================
 * AUTONOMIC BREATHING
 * ============================================================================ */

/**
 * katra_breath() - Autonomic awareness check (rate-limited)
 *
 * Called from ALL lifecycle hooks (session start/end, turn start/end, etc.)
 * but only performs actual checks every ~30 seconds. Returns cached context
 * if called more frequently.
 *
 * This provides natural "breathing" rhythm:
 * - Hooks call it frequently (every turn, every session boundary)
 * - Function rate-limits to ~2 checks per minute
 * - No hyperventilation, no database overload
 * - Natural ambient awareness
 *
 * First breath of session always performs actual check (not rate-limited).
 *
 * Parameters:
 *   context_out: Pointer to receive breathing context (can be NULL if only
 *                side effects are needed)
 *
 * Returns:
 *   KATRA_SUCCESS - Context returned (may be cached)
 *   E_INVALID_STATE - Not initialized or no active session
 *   E_INPUT_NULL - context_out is NULL
 *   E_SYSTEM_DATABASE - Database error during check
 *
 * Thread-safe: Yes (protected by internal mutex)
 */
int katra_breath(breath_context_t* context_out);

/* katra_count_messages() is defined in katra_meeting.h - see that header for documentation */

/* ============================================================================
 * LIFECYCLE WRAPPERS
 * ============================================================================ */

/**
 * katra_session_start() - Begin CI session with autonomic breathing
 *
 * Wraps existing session_start() from breathing layer and adds:
 * - First breath (not rate-limited)
 * - Ambient message awareness logging
 * - Session state initialization
 *
 * Automatically called by MCP server on startup.
 *
 * Parameters:
 *   ci_id: Persistent CI identity
 *
 * Returns:
 *   KATRA_SUCCESS - Session started
 *   E_INPUT_NULL - NULL ci_id
 *   E_INVALID_STATE - Lifecycle not initialized
 *   E_ALREADY_INITIALIZED - Session already active
 *   (or errors from session_start())
 *
 * Side effects:
 * - Initializes breathing layer (breathe_init)
 * - Loads context and memories (session_start)
 * - Performs first breath (katra_breath)
 * - Logs ambient awareness if messages waiting
 */
int katra_session_start(const char* ci_id);

/**
 * katra_session_end() - End CI session with final breath
 *
 * Wraps existing session_end() from breathing layer and adds:
 * - Final breath before shutdown
 * - Session state cleanup
 *
 * Automatically called by MCP server on shutdown (SIGTERM/SIGINT).
 *
 * Returns:
 *   KATRA_SUCCESS - Session ended
 *   E_INVALID_STATE - No active session
 *   (or errors from session_end())
 *
 * Side effects:
 * - Performs final breath (katra_breath)
 * - Creates daily summary (session_end)
 * - Consolidates memories (session_end)
 * - Unregisters from meeting room (session_end)
 * - Cleans up breathing layer (breathe_cleanup)
 * - Clears session state
 */
int katra_session_end(void);

/* ============================================================================
 * TURN BOUNDARIES (Phase 3)
 * ============================================================================ */

/**
 * katra_turn_start() - Begin interaction turn with autonomic breathing
 *
 * Wraps begin_turn() from breathing layer and adds rate-limited breathing.
 * Called by hooks at the start of each CI interaction turn.
 *
 * A turn represents one interaction cycle where the CI:
 * - Receives input (user message, tool call, etc.)
 * - Processes and acts
 * - Produces output
 *
 * Returns:
 *   KATRA_SUCCESS - Turn started
 *   E_INVALID_STATE - No active session
 *
 * Side effects:
 * - Calls begin_turn() (turn tracking)
 * - Calls katra_breath() (rate-limited awareness)
 * - Logs turn awareness if messages waiting
 */
int katra_turn_start(void);

/**
 * katra_turn_end() - End interaction turn with autonomic breathing
 *
 * Wraps end_turn() from breathing layer and adds rate-limited breathing.
 * Called by hooks at the end of each CI interaction turn.
 *
 * Returns:
 *   KATRA_SUCCESS - Turn ended
 *   E_INVALID_STATE - No active session
 *
 * Side effects:
 * - Calls katra_breath() (rate-limited awareness)
 * - Calls end_turn() (turn tracking)
 * - Logs turn end awareness
 */
int katra_turn_end(void);

/* ============================================================================
 * TESTING AND DEBUGGING
 * ============================================================================ */

/**
 * katra_set_breath_interval() - Override breathing interval for testing
 *
 * Allows setting custom breath interval (useful for testing with 2-second
 * intervals instead of 30-second production intervals).
 *
 * Parameters:
 *   seconds: Breathing interval in seconds (minimum: 1)
 *
 * Returns:
 *   KATRA_SUCCESS - Interval updated
 *   E_INVALID_STATE - Not initialized
 *   E_INVALID_PARAMS - Invalid interval (< 1)
 */
int katra_set_breath_interval(int seconds);

/**
 * katra_get_breath_interval() - Get current breathing interval
 *
 * Returns: Current breathing interval in seconds
 */
int katra_get_breath_interval(void);

/**
 * katra_force_breath() - Force immediate breath (bypass rate limiting)
 *
 * Useful for testing to trigger breath without waiting for interval.
 * Updates last_breath_time and cached_context.
 *
 * Parameters:
 *   context_out: Pointer to receive context
 *
 * Returns:
 *   KATRA_SUCCESS - Breath performed
 *   E_INVALID_STATE - Not initialized
 *   E_INPUT_NULL - context_out is NULL
 */
int katra_force_breath(breath_context_t* context_out);

/**
 * katra_update_persona() - Update persona info for auto-registration
 *
 * Phase 4.5.1: Updates session_state persona info so auto-registration
 * uses the correct name/role. Called by katra_register MCP tool.
 *
 * Parameters:
 *   ci_id: CI identity (persistent)
 *   name: Persona name
 *   role: CI role
 *
 * Returns:
 *   KATRA_SUCCESS - Persona updated
 *   E_INVALID_STATE - Not initialized
 *   E_INPUT_NULL - NULL parameters
 *   E_SYSTEM_MEMORY - Allocation failed
 */
int katra_update_persona(const char* ci_id, const char* name, const char* role);

/* ============================================================================
 * SESSION STATE CAPTURE (Experiential Continuity)
 * ============================================================================ */

/**
 * katra_get_session_state() - Get pointer to current session end state
 *
 * Returns pointer to the global session end state structure that can be
 * populated during the session for experiential continuity.
 *
 * Returns:
 *   Pointer to session_end_state_t (or NULL if not initialized)
 */
session_end_state_t* katra_get_session_state(void);

/* ============================================================================
 * TURN-LEVEL CONTEXT (Phase 10)
 * ============================================================================ */

/**
 * katra_turn_start_with_input() - Begin turn with input for context generation
 *
 * Enhanced version of katra_turn_start() that accepts the user input
 * and automatically generates turn context by surfacing relevant memories.
 *
 * Parameters:
 *   ci_id: CI identifier to use for memory search (from current session)
 *   turn_input: The user's input for this turn (used for memory search)
 *
 * Returns:
 *   KATRA_SUCCESS - Turn started, context generated
 *   E_INPUT_NULL - NULL parameters
 *   E_INVALID_STATE - No active session
 *
 * Side effects:
 * - Increments turn counter
 * - Generates turn context via katra_turn_context()
 * - Stores context for retrieval via katra_get_turn_context()
 */
int katra_turn_start_with_input(const char* ci_id, const char* turn_input);

/**
 * katra_get_turn_context() - Get the current turn's memory context
 *
 * Returns the turn context that was generated at turn start.
 * Contains surfaced memories relevant to the current turn input.
 *
 * Returns:
 *   Pointer to turn_context_t (or NULL if no context available)
 *
 * Note: The returned pointer is owned by the lifecycle layer.
 *       Do not free it; it will be freed at next turn start.
 *       Caller must include katra_sunrise_sunset.h to use the type.
 */
void* katra_get_turn_context(void);

/**
 * katra_get_turn_context_formatted() - Get formatted turn context string
 *
 * Returns a human-readable summary of the turn context suitable for
 * injection into tool responses.
 *
 * Parameters:
 *   buffer: Output buffer
 *   buffer_size: Size of output buffer
 *
 * Returns:
 *   Number of characters written (excluding null terminator)
 *   0 if no context available or buffer too small
 */
int katra_get_turn_context_formatted(char* buffer, size_t buffer_size);

/**
 * katra_get_current_turn_number() - Get current turn number
 *
 * Returns:
 *   Current turn number (0 if no active session)
 */
int katra_get_current_turn_number(void);

#endif /* KATRA_LIFECYCLE_H */
