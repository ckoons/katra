/* © 2025 Casey Koons All rights reserved */

/*
 * katra_lifecycle_session.c - Session and Turn Management
 *
 * Implements session lifecycle wrappers and turn boundary management.
 * Part of the Phase 2 Three-Layer Architecture.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* Project includes */
#include "katra_lifecycle.h"
#include "katra_lifecycle_internal.h"
#include "katra_breathing.h"
#include "katra_meeting.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_sunrise_sunset.h"

/* ============================================================================
 * SESSION LIFECYCLE WRAPPERS
 * ============================================================================ */

int katra_session_start(const char* ci_id) {
    int result = KATRA_SUCCESS;
    session_state_t* state = NULL;

    KATRA_CHECK_NULL(ci_id);

    state = katra_lifecycle_get_state();
    if (!katra_lifecycle_is_initialized() || !state) {
        return E_INVALID_STATE;
    }

    if (state->session_active) {
        LOG_WARN("Session already active for %s", state->ci_id);
        return E_ALREADY_INITIALIZED;
    }

    LOG_INFO("Starting session with autonomic breathing for %s", ci_id);

    /* Get persona from environment or use defaults */
    const char* persona = getenv("KATRA_PERSONA");
    if (!persona) {
        persona = "Katra";  /* Default persona name */
    }

    const char* role = getenv("KATRA_ROLE");
    if (!role) {
        role = "developer";  /* Default role */
    }

    /* Store persona info for auto-registration */
    state->persona_name = strdup(persona);
    if (!state->persona_name) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_session_start",
                          "Failed to allocate persona_name");
        return E_SYSTEM_MEMORY;
    }

    state->persona_role = strdup(role);
    if (!state->persona_role) {
        free(state->persona_name);
        state->persona_name = NULL;
        katra_report_error(E_SYSTEM_MEMORY, "katra_session_start",
                          "Failed to allocate persona_role");
        return E_SYSTEM_MEMORY;
    }

    LOG_INFO("Persona configured: %s (%s)", persona, role);

    /* Call existing session_start from breathing layer */
    result = session_start(ci_id);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_session_start", "session_start failed");
        free(state->persona_name);
        free(state->persona_role);
        state->persona_name = NULL;
        state->persona_role = NULL;
        return result;
    }

    /* Store session identity */
    state->ci_id = strdup(ci_id);
    if (!state->ci_id) {
        session_end();  /* Cleanup breathing layer */
        free(state->persona_name);
        free(state->persona_role);
        state->persona_name = NULL;
        state->persona_role = NULL;
        return E_SYSTEM_MEMORY;
    }

    /* Generate session ID (matching breathing layer format) */
    char session_id_buf[KATRA_BUFFER_MEDIUM];
    snprintf(session_id_buf, sizeof(session_id_buf), "%s_%ld", ci_id, (long)time(NULL));
    state->session_id = strdup(session_id_buf);
    if (!state->session_id) {
        free(state->ci_id);
        state->ci_id = NULL;
        session_end();
        return E_SYSTEM_MEMORY;
    }

    /* Mark session as active */
    state->session_active = true;

    /* Reset last_breath_time to force first breath */
    state->last_breath_time = KATRA_SUCCESS;

    /* Initialize session end state for experiential continuity */
    session_end_state_t* end_state = katra_get_session_state();
    if (end_state) {
        result = katra_session_state_init(end_state);
        if (result != KATRA_SUCCESS) {
            LOG_WARN("Failed to initialize session end state: %d", result);
            /* Non-critical - continue anyway */
        } else {
            LOG_DEBUG("Session end state initialized for experiential continuity");
        }
    }

    /* Perform first breath (not rate-limited) */
    breath_context_t context;
    result = katra_breath(&context);
    if (result == KATRA_SUCCESS && context.unread_messages > KATRA_SUCCESS) {
        LOG_INFO("Session starting: %zu messages waiting", context.unread_messages);
    }

    LOG_INFO("Session started with autonomic breathing: %s", state->session_id);

    return KATRA_SUCCESS;
}

int katra_session_end(void) {
    int result = KATRA_SUCCESS;
    session_state_t* state = NULL;
    session_end_state_t* end_state = NULL;

    state = katra_lifecycle_get_state();
    if (!katra_lifecycle_is_initialized() || !state) {
        return E_INVALID_STATE;
    }

    if (!state->session_active) {
        LOG_WARN("No active session to end");
        return E_INVALID_STATE;
    }

    LOG_INFO("Ending session with final breath: %s", state->session_id);

    /* Perform final breath before shutdown */
    breath_context_t context;
    result = katra_breath(&context);
    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Final breath: %zu messages waiting", context.unread_messages);
    }

    /* Capture session end state for experiential continuity */
    end_state = katra_get_session_state();
    if (end_state && end_state->session_start > 0) {
        /* Finalize session state (sets end time, duration) */
        result = katra_session_state_finalize(end_state);
        if (result == KATRA_SUCCESS) {
            /* Serialize to JSON for logging/storage */
            char* json_str = NULL;
            result = katra_session_state_to_json(end_state, &json_str);
            if (result == KATRA_SUCCESS && json_str) {
                LOG_INFO("Session end state captured (%d seconds):",
                         end_state->duration_seconds);
                LOG_INFO("  Active threads: %d", end_state->active_thread_count);
                LOG_INFO("  Next intentions: %d", end_state->next_intention_count);
                LOG_INFO("  Open questions: %d", end_state->open_question_count);
                LOG_INFO("  Session insights: %d", end_state->insight_count);
                LOG_INFO("  Cognitive mode: %s",
                         end_state->cognitive_mode_desc[0] ?
                         end_state->cognitive_mode_desc : "unknown");
                LOG_INFO("  Emotional state: %s",
                         end_state->emotional_state_desc[0] ?
                         end_state->emotional_state_desc : "neutral");

                /* TODO: Store in sunrise.md or database for next session */
                /* For now, log the JSON for debugging */
                LOG_DEBUG("Session state JSON:\n%s", json_str);

                free(json_str);
            } else {
                LOG_WARN("Failed to serialize session end state: %d", result);
            }
        } else {
            LOG_WARN("Failed to finalize session end state: %d", result);
        }
    }

    /* Call existing session_end from breathing layer */
    /* This handles: sunset, consolidation, cleanup, unregister */
    result = session_end();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("session_end failed: %d", result);
        /* Continue with cleanup anyway */
    }

    /* Clear session state */
    free(state->ci_id);
    free(state->session_id);
    free(state->persona_name);
    free(state->persona_role);
    state->ci_id = NULL;
    state->session_id = NULL;
    state->persona_name = NULL;
    state->persona_role = NULL;
    state->session_active = false;
    state->last_breath_time = KATRA_SUCCESS;
    memset(&state->cached_context, 0, sizeof(breath_context_t));

    LOG_INFO("Session ended and state cleared");

    return result;
}

/* ============================================================================
 * TURN BOUNDARIES (Phase 3)
 * ============================================================================ */

int katra_turn_start(void) {
    int result = KATRA_SUCCESS;
    session_state_t* state = NULL;

    state = katra_lifecycle_get_state();
    if (!katra_lifecycle_is_initialized() || !state) {
        return E_INVALID_STATE;
    }

    if (!state->session_active) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Turn starting with autonomic breathing");

    /* Call underlying begin_turn from breathing layer */
    result = begin_turn();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("begin_turn failed: %d", result);
        /* Continue anyway - turn tracking is non-critical */
    }

    /* Autonomic breathing at turn start (rate-limited) */
    breath_context_t context;
    result = katra_breath(&context);
    if (result == KATRA_SUCCESS && context.unread_messages > KATRA_SUCCESS) {
        LOG_DEBUG("Turn awareness: %zu unread messages", context.unread_messages);
    }

    return KATRA_SUCCESS;
}

int katra_turn_end(void) {
    int result = KATRA_SUCCESS;
    session_state_t* state = NULL;

    state = katra_lifecycle_get_state();
    if (!katra_lifecycle_is_initialized() || !state) {
        return E_INVALID_STATE;
    }

    if (!state->session_active) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Turn ending with autonomic breathing");

    /* Autonomic breathing at turn end (rate-limited) */
    breath_context_t context;
    result = katra_breath(&context);
    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Turn end breath: %zu messages waiting", context.unread_messages);
    }

    /* Call underlying end_turn from breathing layer */
    result = end_turn();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("end_turn failed: %d", result);
        /* Continue anyway - turn tracking is non-critical */
    }

    return KATRA_SUCCESS;
}

/* ============================================================================
 * TURN-LEVEL CONTEXT (Phase 10)
 * ============================================================================ */

int katra_turn_start_with_input(const char* ci_id, const char* turn_input) {
    int result = KATRA_SUCCESS;
    session_state_t* state = NULL;
    turn_context_t* context = NULL;
    int turn_num = 0;

    KATRA_CHECK_NULL(ci_id);
    KATRA_CHECK_NULL(turn_input);

    state = katra_lifecycle_get_state();
    if (!katra_lifecycle_is_initialized() || !state) {
        return E_INVALID_STATE;
    }

    /* Note: In TCP mode, session may not be "active" in the global sense,
     * but we still want to generate turn context for the current client */

    LOG_DEBUG("Turn starting with input-based context generation for %s", ci_id);

    /* Lock for thread safety */
    (void)pthread_mutex_lock(&state->breath_lock);

    /* Increment turn counter */
    state->current_turn_number++;
    turn_num = state->current_turn_number;

    /* Free previous turn context */
    if (state->current_turn_context) {
        katra_turn_context_free((turn_context_t*)state->current_turn_context);
        state->current_turn_context = NULL;
    }
    free(state->last_turn_input);
    state->last_turn_input = NULL;

    /* Store input for later reference */
    state->last_turn_input = strdup(turn_input);

    pthread_mutex_unlock(&state->breath_lock);

    /* Generate turn context (outside lock to avoid blocking) */
    result = katra_turn_context(ci_id, turn_input, turn_num, &context);

    /* Store result */
    (void)pthread_mutex_lock(&state->breath_lock);

    if (result == KATRA_SUCCESS && context) {
        state->current_turn_context = (void*)context;
        LOG_INFO("Turn %d: surfaced %zu memories for input", turn_num, context->memory_count);
    } else {
        LOG_DEBUG("Turn %d: no context generated (rc=%d)", turn_num, result);
    }

    pthread_mutex_unlock(&state->breath_lock);

    /* Call underlying begin_turn from breathing layer */
    result = begin_turn();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("begin_turn failed: %d", result);
    }

    /* Autonomic breathing at turn start (rate-limited) */
    breath_context_t breath;
    result = katra_breath(&breath);
    if (result == KATRA_SUCCESS && breath.unread_messages > KATRA_SUCCESS) {
        LOG_DEBUG("Turn awareness: %zu unread messages", breath.unread_messages);
    }

    return KATRA_SUCCESS;
}

void* katra_get_turn_context(void) {
    session_state_t* state = katra_lifecycle_get_state();
    if (!katra_lifecycle_is_initialized() || !state || !state->session_active) {
        return NULL;
    }

    /* No lock needed for simple pointer read */
    return state->current_turn_context;
}

int katra_get_turn_context_formatted(char* buffer, size_t buffer_size) {
    session_state_t* state = NULL;
    turn_context_t* context = NULL;

    if (!buffer || buffer_size == 0) {
        return 0;
    }

    state = katra_lifecycle_get_state();
    if (!katra_lifecycle_is_initialized() || !state || !state->session_active) {
        return 0;
    }

    context = (turn_context_t*)state->current_turn_context;
    if (!context) {
        return 0;
    }

    return katra_turn_context_format(context, buffer, buffer_size);
}

int katra_get_current_turn_number(void) {
    session_state_t* state = katra_lifecycle_get_state();
    if (!katra_lifecycle_is_initialized() || !state) {
        return 0;
    }

    return state->current_turn_number;
}
