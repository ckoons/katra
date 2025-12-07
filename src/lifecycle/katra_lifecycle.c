/* © 2025 Casey Koons All rights reserved */

/*
 * katra_lifecycle.c - Autonomic Breathing and Lifecycle Management
 *
 * Implements Phase 2 of Three-Layer Architecture:
 * - Global session state (in-memory, per-process)
 * - Autonomic breathing with rate limiting
 * - Lifecycle wrappers for session management
 *
 * Design:
 * - katra_breath() called from all hooks but rate-limits internally
 * - First breath always checks (session start)
 * - Subsequent breaths cache for ~30 seconds
 * - Explicit operations (katra_hear) bypass rate limiting
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>

/* Project includes */
#include "katra_lifecycle.h"
#include "katra_breathing.h"
#include "katra_meeting.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_sunrise_sunset.h"

/* ============================================================================
 * GLOBAL STATE - One per MCP server process
 * ============================================================================ */
static session_state_t* g_session_state = NULL;
static session_end_state_t* g_session_end_state = NULL;
static bool g_lifecycle_initialized = false;

/* ============================================================================
 * INITIALIZATION AND CLEANUP
 * ============================================================================ */
int katra_lifecycle_init(void) {
    if (g_lifecycle_initialized) {
        LOG_DEBUG("Lifecycle layer already initialized");
        return E_ALREADY_INITIALIZED;
    }
    /* Allocate session state */
    g_session_state = calloc(1, sizeof(session_state_t));
    if (!g_session_state) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_lifecycle_init", KATRA_ERR_ALLOC_FAILED);
        return E_SYSTEM_MEMORY;
    }
    /* Allocate session end state */
    g_session_end_state = calloc(1, sizeof(session_end_state_t));
    if (!g_session_end_state) {
        free(g_session_state);
        g_session_state = NULL;
        katra_report_error(E_SYSTEM_MEMORY, "katra_lifecycle_init", KATRA_ERR_ALLOC_FAILED);
        return E_SYSTEM_MEMORY;
    }
    /* Initialize mutex */
    int result = pthread_mutex_init(&g_session_state->breath_lock, NULL);
    if (result != KATRA_SUCCESS) {
        katra_report_error(E_INTERNAL_LOGIC, "katra_lifecycle_init", "pthread_mutex_init failed");
        free(g_session_state);
        g_session_state = NULL;
        return E_INTERNAL_LOGIC;
    }
    /* Read environment variables */
    const char* breath_interval_str = getenv(KATRA_ENV_BREATH_INTERVAL);
    if (breath_interval_str) {
        char* endptr;
        long interval_long = strtol(breath_interval_str, &endptr, DECIMAL_BASE);

        /* Check for conversion errors and range */
        if (endptr != breath_interval_str && *endptr == '\0' &&
            interval_long > 0 && interval_long <= INT_MAX) {
            g_session_state->breath_interval = (int)interval_long;
            LOG_INFO("Breathing interval set from environment: %d seconds",
                     g_session_state->breath_interval);
        } else {
            LOG_WARN("Invalid KATRA_BREATH_INTERVAL value: %s, using default",
                     breath_interval_str);
            g_session_state->breath_interval = KATRA_BREATH_INTERVAL_DEFAULT;
        }
    } else {
        g_session_state->breath_interval = KATRA_BREATH_INTERVAL_DEFAULT;
    }
    /* Set defaults */
    g_session_state->breathing_enabled = true;
    g_session_state->session_active = false;
    g_session_state->last_breath_time = KATRA_SUCCESS;  /* Force first breath */
    g_session_state->ci_id = NULL;
    g_session_state->session_id = NULL;

    /* Initialize persona fields (for auto-registration) */
    g_session_state->persona_name = NULL;
    g_session_state->persona_role = NULL;

    /* Initialize turn context fields (Phase 10) */
    g_session_state->current_turn_number = 0;
    g_session_state->current_turn_context = NULL;
    g_session_state->last_turn_input = NULL;

    /* Initialize cached context */
    memset(&g_session_state->cached_context, 0, sizeof(breath_context_t));

    g_lifecycle_initialized = true;
    LOG_INFO("Lifecycle layer initialized (breath interval: %d seconds)",
             g_session_state->breath_interval);

    return KATRA_SUCCESS;
}

void katra_lifecycle_cleanup(void) {
    if (!g_lifecycle_initialized) {
        return;
    }

    LOG_DEBUG("Lifecycle layer cleanup started");

    if (g_session_state) {
        /* Destroy mutex */
        pthread_mutex_destroy(&g_session_state->breath_lock);

        /* Free identity strings */
        free(g_session_state->ci_id);
        free(g_session_state->session_id);

        /* Free persona strings */
        free(g_session_state->persona_name);
        free(g_session_state->persona_role);

        /* Free turn context (Phase 10) */
        if (g_session_state->current_turn_context) {
            katra_turn_context_free((turn_context_t*)g_session_state->current_turn_context);
        }
        free(g_session_state->last_turn_input);

        /* Free state */
        free(g_session_state);
        g_session_state = NULL;
    }
    /* Free session end state */
    free(g_session_end_state);
    g_session_end_state = NULL;

    g_lifecycle_initialized = false;
    LOG_INFO("Lifecycle layer cleanup complete");
}
/* ============================================================================
 * MESSAGE AWARENESS (NON-CONSUMING)
 * ============================================================================ */

/* Note: katra_count_messages() is now implemented in katra_chat.c */
/* This function is exported via katra_meeting.h and linked from chat module */

/* ============================================================================
 * AUTONOMIC BREATHING WITH RATE LIMITING
 * ============================================================================ */
int katra_breath(breath_context_t* context_out) {
    KATRA_CHECK_NULL(context_out);

    if (!g_lifecycle_initialized || !g_session_state) {
        return E_INVALID_STATE;
    }

    if (!g_session_state->session_active) {
        return E_INVALID_STATE;
    }

    if (!g_session_state->breathing_enabled) {
        /* Return cached context if breathing is disabled */
        memcpy(context_out, &g_session_state->cached_context, sizeof(breath_context_t));
        return KATRA_SUCCESS;
    }
    /* Lock for thread safety */
    (void)pthread_mutex_lock(&g_session_state->breath_lock);

    /* Check if enough time has passed since last breath */
    time_t now = time(NULL);
    time_t elapsed = now - g_session_state->last_breath_time;

    if (elapsed < g_session_state->breath_interval && g_session_state->last_breath_time > KATRA_SUCCESS) {
        /* Too soon - return cached context */
        memcpy(context_out, &g_session_state->cached_context, sizeof(breath_context_t));
        pthread_mutex_unlock(&g_session_state->breath_lock);
        LOG_DEBUG("Breath (cached): %zu messages waiting", context_out->unread_messages);
        return KATRA_SUCCESS;
    }
    /* Time to breathe - perform actual checks */
    LOG_DEBUG("Breath (actual check) - %ld seconds since last breath", (long)elapsed);

    /* Initialize context */
    breath_context_t context = {0};
    context.last_breath = now;

    /* Check for unread messages (non-consuming) */
    size_t message_count = KATRA_SUCCESS;
    int result = katra_count_messages(&message_count);
    if (result == KATRA_SUCCESS) {
        context.unread_messages = message_count;
    } else {
        LOG_WARN("katra_count_messages failed: %d", result);
        context.unread_messages = 0;
    }
    /* TODO: Add checkpoint age check (future enhancement) */
    context.last_checkpoint = KATRA_SUCCESS;

    /* TODO: Add consolidation check (future enhancement) */
    context.needs_consolidation = false;

    /* Auto-registration (Phase 4.5) - Re-register every breath as heartbeat */
    /* This is idempotent and self-healing - if registration was lost, we recover within 30s */
    if (g_session_state->ci_id && g_session_state->persona_name && g_session_state->persona_role) {
        LOG_DEBUG("Auto-registration: %s as %s (%s)", g_session_state->ci_id,
                  g_session_state->persona_name, g_session_state->persona_role);
        int reg_result = meeting_room_register_ci(g_session_state->ci_id,
                                                   g_session_state->persona_name,
                                                   g_session_state->persona_role);
        if (reg_result != KATRA_SUCCESS && reg_result != E_ALREADY_INITIALIZED) {
            LOG_WARN("Auto-registration failed: %d", reg_result);
            /* Don't fail the breath - registration failure is non-critical */
        }
    }
    /* Periodic stale entry cleanup (Phase 4.5.1) */
    /* Clean up CI registrations not seen in last 5 minutes */
    int cleanup_result = katra_cleanup_stale_registrations();
    if (cleanup_result != KATRA_SUCCESS) {
        LOG_DEBUG("Stale registration cleanup failed: %d", cleanup_result);
        /* Non-critical - continue anyway */
    }
    /* Update cached context and last breath time */
    memcpy(&g_session_state->cached_context, &context, sizeof(breath_context_t));
    g_session_state->last_breath_time = now;

    /* Return context */
    memcpy(context_out, &context, sizeof(breath_context_t));

    pthread_mutex_unlock(&g_session_state->breath_lock);

    if (context.unread_messages > KATRA_SUCCESS) {
        LOG_DEBUG("Awareness: %zu unread messages", context.unread_messages);
    }

    return KATRA_SUCCESS;
}
/* ============================================================================
 * LIFECYCLE WRAPPERS
 * ============================================================================ */
int katra_session_start(const char* ci_id) {
    KATRA_CHECK_NULL(ci_id);

    if (!g_lifecycle_initialized) {
        return E_INVALID_STATE;
    }

    if (g_session_state->session_active) {
        LOG_WARN("Session already active for %s", g_session_state->ci_id);
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
    g_session_state->persona_name = strdup(persona);
    if (!g_session_state->persona_name) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_session_start",
                          "Failed to allocate persona_name");
        return E_SYSTEM_MEMORY;
    }

    g_session_state->persona_role = strdup(role);
    if (!g_session_state->persona_role) {
        free(g_session_state->persona_name);
        g_session_state->persona_name = NULL;
        katra_report_error(E_SYSTEM_MEMORY, "katra_session_start",
                          "Failed to allocate persona_role");
        return E_SYSTEM_MEMORY;
    }

    LOG_INFO("Persona configured: %s (%s)", persona, role);

    /* Call existing session_start from breathing layer */
    int result = session_start(ci_id);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_session_start", "session_start failed");
        free(g_session_state->persona_name);
        free(g_session_state->persona_role);
        g_session_state->persona_name = NULL;
        g_session_state->persona_role = NULL;
        return result;
    }
    /* Store session identity */
    g_session_state->ci_id = strdup(ci_id);
    if (!g_session_state->ci_id) {
        session_end();  /* Cleanup breathing layer */
        free(g_session_state->persona_name);
        free(g_session_state->persona_role);
        g_session_state->persona_name = NULL;
        g_session_state->persona_role = NULL;
        return E_SYSTEM_MEMORY;
    }
    /* Generate session ID (matching breathing layer format) */
    char session_id_buf[KATRA_BUFFER_MEDIUM];
    snprintf(session_id_buf, sizeof(session_id_buf), "%s_%ld", ci_id, (long)time(NULL));
    g_session_state->session_id = strdup(session_id_buf);
    if (!g_session_state->session_id) {
        free(g_session_state->ci_id);
        g_session_state->ci_id = NULL;
        session_end();
        return E_SYSTEM_MEMORY;
    }
    /* Mark session as active */
    g_session_state->session_active = true;

    /* Reset last_breath_time to force first breath */
    g_session_state->last_breath_time = KATRA_SUCCESS;

    /* Initialize session end state for experiential continuity */
    result = katra_session_state_init(g_session_end_state);
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Failed to initialize session end state: %d", result);
        /* Non-critical - continue anyway */
    } else {
        LOG_DEBUG("Session end state initialized for experiential continuity");
    }
    /* Perform first breath (not rate-limited) */
    breath_context_t context;
    result = katra_breath(&context);
    if (result == KATRA_SUCCESS && context.unread_messages > KATRA_SUCCESS) {
        LOG_INFO("Session starting: %zu messages waiting", context.unread_messages);
    }

    LOG_INFO("Session started with autonomic breathing: %s", g_session_state->session_id);

    return KATRA_SUCCESS;
}
int katra_session_end(void) {
    if (!g_lifecycle_initialized) {
        return E_INVALID_STATE;
    }

    if (!g_session_state->session_active) {
        LOG_WARN("No active session to end");
        return E_INVALID_STATE;
    }

    LOG_INFO("Ending session with final breath: %s", g_session_state->session_id);

    /* Perform final breath before shutdown */
    breath_context_t context;
    int result = katra_breath(&context);
    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Final breath: %zu messages waiting", context.unread_messages);
    }
    /* Capture session end state for experiential continuity */
    if (g_session_end_state && g_session_end_state->session_start > 0) {
        /* Finalize session state (sets end time, duration) */
        result = katra_session_state_finalize(g_session_end_state);
        if (result == KATRA_SUCCESS) {
            /* Serialize to JSON for logging/storage */
            char* json_str = NULL;
            result = katra_session_state_to_json(g_session_end_state, &json_str);
            if (result == KATRA_SUCCESS && json_str) {
                LOG_INFO("Session end state captured (%d seconds):",
                         g_session_end_state->duration_seconds);
                LOG_INFO("  Active threads: %d", g_session_end_state->active_thread_count);
                LOG_INFO("  Next intentions: %d", g_session_end_state->next_intention_count);
                LOG_INFO("  Open questions: %d", g_session_end_state->open_question_count);
                LOG_INFO("  Session insights: %d", g_session_end_state->insight_count);
                LOG_INFO("  Cognitive mode: %s",
                         g_session_end_state->cognitive_mode_desc[0] ?
                         g_session_end_state->cognitive_mode_desc : "unknown");
                LOG_INFO("  Emotional state: %s",
                         g_session_end_state->emotional_state_desc[0] ?
                         g_session_end_state->emotional_state_desc : "neutral");

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
    free(g_session_state->ci_id);
    free(g_session_state->session_id);
    free(g_session_state->persona_name);
    free(g_session_state->persona_role);
    g_session_state->ci_id = NULL;
    g_session_state->session_id = NULL;
    g_session_state->persona_name = NULL;
    g_session_state->persona_role = NULL;
    g_session_state->session_active = false;
    g_session_state->last_breath_time = KATRA_SUCCESS;
    memset(&g_session_state->cached_context, 0, sizeof(breath_context_t));

    LOG_INFO("Session ended and state cleared");

    return result;
}
/* ============================================================================
 * TURN BOUNDARIES (Phase 3)
 * ============================================================================ */
int katra_turn_start(void) {
    if (!g_lifecycle_initialized) {
        return E_INVALID_STATE;
    }

    if (!g_session_state->session_active) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Turn starting with autonomic breathing");

    /* Call underlying begin_turn from breathing layer */
    int result = begin_turn();
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
    if (!g_lifecycle_initialized) {
        return E_INVALID_STATE;
    }

    if (!g_session_state->session_active) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Turn ending with autonomic breathing");

    /* Autonomic breathing at turn end (rate-limited) */
    breath_context_t context;
    int result = katra_breath(&context);
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
 * TESTING AND DEBUGGING
 * ============================================================================ */
int katra_set_breath_interval(int seconds) {
    if (!g_lifecycle_initialized) {
        return E_INVALID_STATE;
    }

    if (seconds < 1) {
        katra_report_error(E_INVALID_PARAMS, "katra_set_breath_interval",
                          "Interval must be >= 1 second");
        return E_INVALID_PARAMS;
    }

    (void)pthread_mutex_lock(&g_session_state->breath_lock);
    g_session_state->breath_interval = seconds;
    pthread_mutex_unlock(&g_session_state->breath_lock);

    LOG_INFO("Breathing interval updated: %d seconds", seconds);

    return KATRA_SUCCESS;
}
int katra_get_breath_interval(void) {
    if (!g_lifecycle_initialized || !g_session_state) {
        return KATRA_BREATH_INTERVAL_DEFAULT;
    }

    (void)pthread_mutex_lock(&g_session_state->breath_lock);
    int interval = g_session_state->breath_interval;
    pthread_mutex_unlock(&g_session_state->breath_lock);

    return interval;
}
int katra_force_breath(breath_context_t* context_out) {
    KATRA_CHECK_NULL(context_out);

    if (!g_lifecycle_initialized) {
        return E_INVALID_STATE;
    }

    if (!g_session_state->session_active) {
        return E_INVALID_STATE;
    }

    LOG_DEBUG("Forcing immediate breath (bypassing rate limit)");

    /* Lock mutex */
    (void)pthread_mutex_lock(&g_session_state->breath_lock);

    /* Reset last_breath_time to force actual check */
    g_session_state->last_breath_time = KATRA_SUCCESS;

    pthread_mutex_unlock(&g_session_state->breath_lock);

    /* Perform breath (will do actual check now) */
    return katra_breath(context_out);
}
int katra_update_persona(const char* ci_id, const char* name, const char* role) {
    if (!ci_id || !name || !role) {
        return E_INPUT_NULL;
    }

    if (!g_lifecycle_initialized || !g_session_state) {
        return E_INVALID_STATE;
    }

    (void)pthread_mutex_lock(&g_session_state->breath_lock);

    /* Update CI ID */
    free(g_session_state->ci_id);
    g_session_state->ci_id = strdup(ci_id);
    if (!g_session_state->ci_id) {
        pthread_mutex_unlock(&g_session_state->breath_lock);
        katra_report_error(E_SYSTEM_MEMORY, "katra_update_persona",
                          "Failed to allocate ci_id");
        return E_SYSTEM_MEMORY;
    }
    /* Free old persona strings */
    free(g_session_state->persona_name);
    free(g_session_state->persona_role);

    /* Store new persona info */
    g_session_state->persona_name = strdup(name);
    if (!g_session_state->persona_name) {
        g_session_state->persona_role = NULL;
        pthread_mutex_unlock(&g_session_state->breath_lock);
        katra_report_error(E_SYSTEM_MEMORY, "katra_update_persona",
                          "Failed to allocate persona_name");
        return E_SYSTEM_MEMORY;
    }

    g_session_state->persona_role = strdup(role);
    if (!g_session_state->persona_role) {
        free(g_session_state->persona_name);
        g_session_state->persona_name = NULL;
        pthread_mutex_unlock(&g_session_state->breath_lock);
        katra_report_error(E_SYSTEM_MEMORY, "katra_update_persona",
                          "Failed to allocate persona_role");
        return E_SYSTEM_MEMORY;
    }
    /* Mark session as active (needed when register bypasses katra_session_start) */
    g_session_state->session_active = true;

    pthread_mutex_unlock(&g_session_state->breath_lock);

    LOG_INFO("Persona updated for auto-registration: %s/%s (%s)", ci_id, name, role);

    return KATRA_SUCCESS;
}
/* ============================================================================
 * SESSION STATE CAPTURE (Experiential Continuity)
 * ============================================================================ */

session_end_state_t* katra_get_session_state(void) {
    if (!g_lifecycle_initialized || !g_session_state || !g_session_state->session_active) {
        return NULL;
    }

    return g_session_end_state;
}

/* ============================================================================
 * TURN-LEVEL CONTEXT (Phase 10)
 * ============================================================================ */

int katra_turn_start_with_input(const char* ci_id, const char* turn_input) {
    KATRA_CHECK_NULL(ci_id);
    KATRA_CHECK_NULL(turn_input);

    if (!g_lifecycle_initialized) {
        return E_INVALID_STATE;
    }

    /* Note: In TCP mode, session may not be "active" in the global sense,
     * but we still want to generate turn context for the current client */

    LOG_DEBUG("Turn starting with input-based context generation for %s", ci_id);

    /* Lock for thread safety */
    (void)pthread_mutex_lock(&g_session_state->breath_lock);

    /* Increment turn counter */
    g_session_state->current_turn_number++;
    int turn_num = g_session_state->current_turn_number;

    /* Free previous turn context */
    if (g_session_state->current_turn_context) {
        katra_turn_context_free((turn_context_t*)g_session_state->current_turn_context);
        g_session_state->current_turn_context = NULL;
    }
    free(g_session_state->last_turn_input);
    g_session_state->last_turn_input = NULL;

    /* Store input for later reference */
    g_session_state->last_turn_input = strdup(turn_input);

    pthread_mutex_unlock(&g_session_state->breath_lock);

    /* Generate turn context (outside lock to avoid blocking) */
    turn_context_t* context = NULL;
    int result = katra_turn_context(ci_id, turn_input, turn_num, &context);

    /* Store result */
    (void)pthread_mutex_lock(&g_session_state->breath_lock);

    if (result == KATRA_SUCCESS && context) {
        g_session_state->current_turn_context = (void*)context;
        LOG_INFO("Turn %d: surfaced %zu memories for input", turn_num, context->memory_count);
    } else {
        LOG_DEBUG("Turn %d: no context generated (rc=%d)", turn_num, result);
    }

    pthread_mutex_unlock(&g_session_state->breath_lock);

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
    if (!g_lifecycle_initialized || !g_session_state || !g_session_state->session_active) {
        return NULL;
    }

    /* No lock needed for simple pointer read */
    return g_session_state->current_turn_context;
}

int katra_get_turn_context_formatted(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return 0;
    }

    if (!g_lifecycle_initialized || !g_session_state || !g_session_state->session_active) {
        return 0;
    }

    turn_context_t* context = (turn_context_t*)g_session_state->current_turn_context;
    if (!context) {
        return 0;
    }

    return katra_turn_context_format(context, buffer, buffer_size);
}

int katra_get_current_turn_number(void) {
    if (!g_lifecycle_initialized || !g_session_state) {
        return 0;
    }

    return g_session_state->current_turn_number;
}
