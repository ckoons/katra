/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_HOOKS_H
#define KATRA_HOOKS_H

#include <stdbool.h>
#include "katra_error.h"

/**
 * katra_hooks.h - Hook Adapter Interface (Layer C)
 *
 * Defines the standard interface for provider-specific hook adapters.
 * Each provider (Anthropic, OpenAI, Gemini, etc.) implements this interface
 * to map their lifecycle events to Katra lifecycle functions.
 *
 * Architecture:
 *   Provider (Claude Code, etc.)
 *   ↓ fires lifecycle events
 *   Hook Adapter (Layer C) - implements katra_hook_adapter_t
 *   ↓ calls Katra functions
 *   Katra Lifecycle (Layer A) - katra_session_start, katra_breath, etc.
 *
 * Design Principles:
 * - Hooks are simple: map events to Katra calls
 * - Breathing is automatic: all hooks call katra_breath() (rate-limited)
 * - Failures are graceful: autonomic failures log warnings, don't crash
 */

/* ============================================================================
 * HOOK ADAPTER INTERFACE
 * ============================================================================ */

/**
 * katra_hook_adapter_t - Standard interface for provider hook adapters
 *
 * Each provider implements this interface to integrate with Katra.
 * All function pointers are optional (can be NULL if not supported).
 */
typedef struct {
    /* Provider identification */
    const char* provider_name;      /* e.g., "anthropic", "openai", "gemini" */
    const char* version;            /* Hook adapter version */

    /* Session lifecycle hooks */
    int (*on_session_start)(const char* ci_id);
    int (*on_session_end)(void);

    /* Turn lifecycle hooks */
    int (*on_turn_start)(void);
    int (*on_turn_end)(void);

    /* Tool execution hooks (optional, for fine-grained breathing) */
    int (*on_pre_tool_use)(const char* tool_name);
    int (*on_post_tool_use)(const char* tool_name, int tool_result);

    /* Request/response hooks (optional) */
    int (*on_request_received)(const char* request);
    int (*on_response_sent)(const char* response);

} katra_hook_adapter_t;

/* ============================================================================
 * HOOK REGISTRY API
 * ============================================================================ */

/**
 * katra_hooks_init() - Initialize hook registry system
 *
 * Must be called before registering or using hooks.
 *
 * Returns:
 *   KATRA_SUCCESS - Registry initialized
 *   E_ALREADY_INITIALIZED - Already initialized
 */
int katra_hooks_init(void);

/**
 * katra_hooks_cleanup() - Cleanup hook registry
 *
 * Frees all registered adapters and cleans up registry.
 */
void katra_hooks_cleanup(void);

/**
 * katra_hooks_register() - Register a hook adapter
 *
 * Registers a provider-specific hook adapter with the registry.
 * The adapter becomes active and all subsequent lifecycle events
 * will be routed through it.
 *
 * Parameters:
 *   adapter: Pointer to hook adapter structure (must remain valid)
 *
 * Returns:
 *   KATRA_SUCCESS - Adapter registered
 *   E_INPUT_NULL - NULL adapter
 *   E_INVALID_STATE - Registry not initialized
 *   E_DUPLICATE - Adapter with this name already registered
 */
int katra_hooks_register(const katra_hook_adapter_t* adapter);

/**
 * katra_hooks_get_active() - Get currently active hook adapter
 *
 * Returns: Pointer to active adapter, or NULL if none registered
 */
const katra_hook_adapter_t* katra_hooks_get_active(void);

/* ============================================================================
 * HOOK INVOCATION API (called by MCP server or other runtimes)
 * ============================================================================ */

/**
 * katra_hook_session_start() - Invoke session start hook
 *
 * Calls the active adapter's on_session_start() if registered.
 * If no adapter registered, calls katra_session_start() directly.
 *
 * Parameters:
 *   ci_id: CI identity
 *
 * Returns:
 *   KATRA_SUCCESS or error from hook/lifecycle
 */
int katra_hook_session_start(const char* ci_id);

/**
 * katra_hook_session_end() - Invoke session end hook
 *
 * Calls the active adapter's on_session_end() if registered.
 * If no adapter registered, calls katra_session_end() directly.
 *
 * Returns:
 *   KATRA_SUCCESS or error from hook/lifecycle
 */
int katra_hook_session_end(void);

/**
 * katra_hook_turn_start() - Invoke turn start hook
 *
 * Calls the active adapter's on_turn_start() if registered.
 * If no adapter registered, calls katra_turn_start() directly.
 *
 * Returns:
 *   KATRA_SUCCESS or error from hook/lifecycle
 */
int katra_hook_turn_start(void);

/**
 * katra_hook_turn_start_with_input() - Invoke turn start with context generation
 *
 * Enhanced version that takes the user input and automatically generates
 * turn context by surfacing relevant memories. The context can then be
 * retrieved via katra_get_turn_context().
 *
 * Parameters:
 *   ci_id: CI identifier to use for memory search (from current session)
 *   turn_input: The user's input for this turn (used for memory search)
 *
 * Returns:
 *   KATRA_SUCCESS or error from hook/lifecycle
 */
int katra_hook_turn_start_with_input(const char* ci_id, const char* turn_input);

/**
 * katra_hook_turn_end() - Invoke turn end hook
 *
 * Calls the active adapter's on_turn_end() if registered.
 * If no adapter registered, calls katra_turn_end() directly.
 *
 * Returns:
 *   KATRA_SUCCESS or error from hook/lifecycle
 */
int katra_hook_turn_end(void);

/**
 * katra_hook_pre_tool_use() - Invoke pre-tool-use hook
 *
 * Called before executing a tool. Allows breathing during long operations.
 *
 * Parameters:
 *   tool_name: Name of tool about to be executed
 *
 * Returns:
 *   KATRA_SUCCESS or error from hook
 */
int katra_hook_pre_tool_use(const char* tool_name);

/**
 * katra_hook_post_tool_use() - Invoke post-tool-use hook
 *
 * Called after executing a tool. Allows breathing after operations.
 *
 * Parameters:
 *   tool_name: Name of tool that was executed
 *   tool_result: Result code from tool execution
 *
 * Returns:
 *   KATRA_SUCCESS or error from hook
 */
int katra_hook_post_tool_use(const char* tool_name, int tool_result);

/* ============================================================================
 * BUILT-IN ADAPTERS
 * ============================================================================ */

/**
 * katra_hook_anthropic_adapter() - Get Anthropic MCP hook adapter
 *
 * Returns pointer to the built-in Anthropic Claude Code adapter.
 * This adapter maps Anthropic MCP lifecycle events to Katra functions.
 *
 * Returns: Pointer to Anthropic adapter (static, never NULL)
 */
const katra_hook_adapter_t* katra_hook_anthropic_adapter(void);

#endif /* KATRA_HOOKS_H */
