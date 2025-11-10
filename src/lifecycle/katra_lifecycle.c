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

/* Project includes */
#include "katra_lifecycle.h"
#include "katra_breathing.h"
#include "katra_meeting.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* ============================================================================
 * GLOBAL STATE - One per MCP server process
 * ============================================================================ */

static session_state_t* g_session_state = NULL;
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
        int interval = atoi(breath_interval_str);
        if (interval > KATRA_SUCCESS) {
            g_session_state->breath_interval = interval;
            LOG_INFO("Breathing interval set from environment: %d seconds", interval);
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

        /* Free state */
        free(g_session_state);
        g_session_state = NULL;
    }

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
    pthread_mutex_lock(&g_session_state->breath_lock);

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

    pthread_mutex_lock(&g_session_state->breath_lock);
    g_session_state->breath_interval = seconds;
    pthread_mutex_unlock(&g_session_state->breath_lock);

    LOG_INFO("Breathing interval updated: %d seconds", seconds);

    return KATRA_SUCCESS;
}

int katra_get_breath_interval(void) {
    if (!g_lifecycle_initialized || !g_session_state) {
        return KATRA_BREATH_INTERVAL_DEFAULT;
    }

    pthread_mutex_lock(&g_session_state->breath_lock);
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
    pthread_mutex_lock(&g_session_state->breath_lock);

    /* Reset last_breath_time to force actual check */
    g_session_state->last_breath_time = KATRA_SUCCESS;

    pthread_mutex_unlock(&g_session_state->breath_lock);

    /* Perform breath (will do actual check now) */
    return katra_breath(context_out);
}
