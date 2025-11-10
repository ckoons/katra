/* © 2025 Casey Koons All rights reserved */

/*
 * hook_anthropic.c - Anthropic MCP Hook Adapter
 *
 * Maps Anthropic Claude Code lifecycle events to Katra lifecycle functions.
 * Provides autonomic breathing at all lifecycle boundaries.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>

/* Project includes */
#include "katra_hooks.h"
#include "katra_lifecycle.h"
#include "katra_error.h"
#include "katra_log.h"

/* ============================================================================
 * ANTHROPIC ADAPTER VERSION
 * ============================================================================ */

#define ANTHROPIC_ADAPTER_VERSION "1.0.0"

/* ============================================================================
 * HOOK IMPLEMENTATIONS
 * ============================================================================ */

static int anthropic_session_start(const char* ci_id) {
    KATRA_CHECK_NULL(ci_id);

    LOG_DEBUG("Anthropic adapter: session_start for %s", ci_id);

    /* Call lifecycle layer - includes first breath */
    int result = katra_session_start(ci_id);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "anthropic_session_start",
                          "katra_session_start failed");
        return result;
    }

    LOG_INFO("Anthropic session started with autonomic breathing: %s", ci_id);
    return KATRA_SUCCESS;
}

static int anthropic_session_end(void) {
    LOG_DEBUG("Anthropic adapter: session_end");

    /* Call lifecycle layer - includes final breath */
    int result = katra_session_end();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("katra_session_end failed: %d", result);
        /* Continue anyway - cleanup is best effort */
    }

    LOG_INFO("Anthropic session ended with final breath");
    return result;
}

static int anthropic_turn_start(void) {
    LOG_DEBUG("Anthropic adapter: turn_start");

    /* Call lifecycle layer - includes rate-limited breathing */
    int result = katra_turn_start();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("katra_turn_start failed: %d", result);
        /* Non-critical - breathing failures shouldn't block interaction */
        return KATRA_SUCCESS;
    }

    return KATRA_SUCCESS;
}

static int anthropic_turn_end(void) {
    LOG_DEBUG("Anthropic adapter: turn_end");

    /* Call lifecycle layer - includes rate-limited breathing */
    int result = katra_turn_end();
    if (result != KATRA_SUCCESS) {
        LOG_WARN("katra_turn_end failed: %d", result);
        /* Non-critical - breathing failures shouldn't block interaction */
        return KATRA_SUCCESS;
    }

    return KATRA_SUCCESS;
}

static int anthropic_pre_tool_use(const char* tool_name) {
    KATRA_CHECK_NULL(tool_name);

    LOG_DEBUG("Anthropic adapter: pre_tool_use (%s)", tool_name);

    /* Optionally add breathing before tool use */
    /* For now, just log - breathing happens at turn boundaries */
    /* Future: Could add forced breath for long-running tools */

    return KATRA_SUCCESS;
}

static int anthropic_post_tool_use(const char* tool_name, int tool_result) {
    KATRA_CHECK_NULL(tool_name);

    LOG_DEBUG("Anthropic adapter: post_tool_use (%s, result=%d)",
              tool_name, tool_result);

    /* Optionally add breathing after tool use */
    /* For now, just log - breathing happens at turn boundaries */
    /* Future: Could add forced breath after expensive operations */

    return KATRA_SUCCESS;
}

/* ============================================================================
 * ADAPTER STRUCTURE
 * ============================================================================ */

static const katra_hook_adapter_t anthropic_adapter = {
    .provider_name = "anthropic",
    .version = ANTHROPIC_ADAPTER_VERSION,
    .on_session_start = anthropic_session_start,
    .on_session_end = anthropic_session_end,
    .on_turn_start = anthropic_turn_start,
    .on_turn_end = anthropic_turn_end,
    .on_pre_tool_use = anthropic_pre_tool_use,
    .on_post_tool_use = anthropic_post_tool_use,
    .on_request_received = NULL,  /* Not implemented yet */
    .on_response_sent = NULL,     /* Not implemented yet */
};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

const katra_hook_adapter_t* katra_hook_anthropic_adapter(void) {
    return &anthropic_adapter;
}
