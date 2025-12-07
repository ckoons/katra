/* © 2025 Casey Koons All rights reserved */

/*
 * hook_registry.c - Hook Adapter Registry
 *
 * Manages registration and invocation of provider-specific hook adapters.
 * Single active adapter per process (matching one CI = one session model).
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_hooks.h"
#include "katra_lifecycle.h"
#include "katra_error.h"
#include "katra_log.h"

/* ============================================================================
 * GLOBAL STATE - One adapter per process
 * ============================================================================ */

static const katra_hook_adapter_t* g_active_adapter = NULL;
static bool g_registry_initialized = false;

/* ============================================================================
 * INITIALIZATION AND CLEANUP
 * ============================================================================ */

int katra_hooks_init(void) {
    if (g_registry_initialized) {
        LOG_DEBUG("Hook registry already initialized");
        return E_ALREADY_INITIALIZED;
    }

    g_active_adapter = NULL;
    g_registry_initialized = true;

    LOG_INFO("Hook registry initialized");
    return KATRA_SUCCESS;
}

void katra_hooks_cleanup(void) {
    if (!g_registry_initialized) {
        return;
    }

    LOG_DEBUG("Hook registry cleanup started");

    /* Clear active adapter (doesn't own memory) */
    g_active_adapter = NULL;
    g_registry_initialized = false;

    LOG_INFO("Hook registry cleanup complete");
}

/* ============================================================================
 * ADAPTER REGISTRATION
 * ============================================================================ */

int katra_hooks_register(const katra_hook_adapter_t* adapter) {
    KATRA_CHECK_NULL(adapter);

    if (!g_registry_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_hooks_register",
                          "Registry not initialized");
        return E_INVALID_STATE;
    }

    if (!adapter->provider_name) {
        katra_report_error(E_INPUT_NULL, "katra_hooks_register",
                          "Adapter provider_name is NULL");
        return E_INPUT_NULL;
    }

    /* Check for duplicate registration */
    if (g_active_adapter) {
        if (strcmp(g_active_adapter->provider_name, adapter->provider_name) == 0) {
            LOG_DEBUG("Adapter '%s' already registered", adapter->provider_name);
            return E_DUPLICATE;
        }
        LOG_WARN("Replacing active adapter '%s' with '%s'",
                 g_active_adapter->provider_name, adapter->provider_name);
    }

    /* Register adapter */
    g_active_adapter = adapter;

    LOG_INFO("Hook adapter registered: %s v%s",
             adapter->provider_name,
             adapter->version ? adapter->version : "unknown");

    return KATRA_SUCCESS;
}

const katra_hook_adapter_t* katra_hooks_get_active(void) {
    if (!g_registry_initialized) {
        return NULL;
    }
    return g_active_adapter;
}

/* ============================================================================
 * HOOK INVOCATION - Routes through active adapter
 * ============================================================================ */

int katra_hook_session_start(const char* ci_id) {
    KATRA_CHECK_NULL(ci_id);

    if (!g_registry_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_hook_session_start",
                          "Registry not initialized");
        return E_INVALID_STATE;
    }

    /* If adapter registered and has hook, call it */
    if (g_active_adapter && g_active_adapter->on_session_start) {
        LOG_DEBUG("Invoking adapter session_start hook: %s",
                  g_active_adapter->provider_name);
        return g_active_adapter->on_session_start(ci_id);
    }

    /* No adapter - call lifecycle function directly */
    LOG_DEBUG("No adapter registered - calling katra_session_start directly");
    return katra_session_start(ci_id);
}

int katra_hook_session_end(void) {
    if (!g_registry_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_hook_session_end",
                          "Registry not initialized");
        return E_INVALID_STATE;
    }

    /* If adapter registered and has hook, call it */
    if (g_active_adapter && g_active_adapter->on_session_end) {
        LOG_DEBUG("Invoking adapter session_end hook: %s",
                  g_active_adapter->provider_name);
        return g_active_adapter->on_session_end();
    }

    /* No adapter - call lifecycle function directly */
    LOG_DEBUG("No adapter registered - calling katra_session_end directly");
    return katra_session_end();
}

int katra_hook_turn_start(void) {
    if (!g_registry_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_hook_turn_start",
                          "Registry not initialized");
        return E_INVALID_STATE;
    }

    /* If adapter registered and has hook, call it */
    if (g_active_adapter && g_active_adapter->on_turn_start) {
        LOG_DEBUG("Invoking adapter turn_start hook: %s",
                  g_active_adapter->provider_name);
        return g_active_adapter->on_turn_start();
    }

    /* No adapter - call lifecycle function directly */
    LOG_DEBUG("No adapter registered - calling katra_turn_start directly");
    return katra_turn_start();
}

int katra_hook_turn_start_with_input(const char* ci_id, const char* turn_input) {
    KATRA_CHECK_NULL(ci_id);
    KATRA_CHECK_NULL(turn_input);

    if (!g_registry_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_hook_turn_start_with_input",
                          "Registry not initialized");
        return E_INVALID_STATE;
    }

    /* Always call the lifecycle function with input for context generation */
    LOG_DEBUG("Turn start with input-based context generation for %s", ci_id);
    return katra_turn_start_with_input(ci_id, turn_input);
}

int katra_hook_turn_end(void) {
    if (!g_registry_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_hook_turn_end",
                          "Registry not initialized");
        return E_INVALID_STATE;
    }

    /* If adapter registered and has hook, call it */
    if (g_active_adapter && g_active_adapter->on_turn_end) {
        LOG_DEBUG("Invoking adapter turn_end hook: %s",
                  g_active_adapter->provider_name);
        return g_active_adapter->on_turn_end();
    }

    /* No adapter - call lifecycle function directly */
    LOG_DEBUG("No adapter registered - calling katra_turn_end directly");
    return katra_turn_end();
}

int katra_hook_pre_tool_use(const char* tool_name) {
    KATRA_CHECK_NULL(tool_name);

    if (!g_registry_initialized) {
        /* Non-critical - just log warning */
        LOG_WARN("Hook registry not initialized - skipping pre_tool_use hook");
        return KATRA_SUCCESS;
    }

    /* If adapter registered and has hook, call it */
    if (g_active_adapter && g_active_adapter->on_pre_tool_use) {
        LOG_DEBUG("Invoking adapter pre_tool_use hook: %s (tool: %s)",
                  g_active_adapter->provider_name, tool_name);
        return g_active_adapter->on_pre_tool_use(tool_name);
    }

    /* No hook - not an error, just skip */
    LOG_DEBUG("No pre_tool_use hook registered for tool: %s", tool_name);
    return KATRA_SUCCESS;
}

int katra_hook_post_tool_use(const char* tool_name, int tool_result) {
    KATRA_CHECK_NULL(tool_name);

    if (!g_registry_initialized) {
        /* Non-critical - just log warning */
        LOG_WARN("Hook registry not initialized - skipping post_tool_use hook");
        return KATRA_SUCCESS;
    }

    /* If adapter registered and has hook, call it */
    if (g_active_adapter && g_active_adapter->on_post_tool_use) {
        LOG_DEBUG("Invoking adapter post_tool_use hook: %s (tool: %s, result: %d)",
                  g_active_adapter->provider_name, tool_name, tool_result);
        return g_active_adapter->on_post_tool_use(tool_name, tool_result);
    }

    /* No hook - not an error, just skip */
    LOG_DEBUG("No post_tool_use hook registered for tool: %s", tool_name);
    return KATRA_SUCCESS;
}
