/* © 2025 Casey Koons All rights reserved */
/* MCP Daemon Tools - Interstitial insights, acknowledgment, manual run */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_daemon.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "mcp_tools_common.h"

/* Tool: katra_daemon_insights - Get unacknowledged insights */
json_t* mcp_tool_daemon_insights(json_t* args, json_t* id) {
    (void)args;
    (void)id;

    if (!g_ci_id[0]) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "CI not initialized");
    }
    const char* ci_id = g_ci_id;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    daemon_insight_t* insights = NULL;
    size_t count = 0;

    int result = katra_daemon_get_pending_insights(ci_id, &insights, &count);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        return mcp_tool_error(MCP_ERR_INTERNAL, katra_error_message(result));
    }

    /* Build response */
    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    if (count == 0) {
        snprintf(response, sizeof(response),
            "No pending insights.\n"
            "The daemon hasn't discovered new patterns yet,\n"
            "or you've already acknowledged all insights.");
    } else {
        offset += snprintf(response + offset, sizeof(response) - offset,
            "=== Pending Insights (%zu) ===\n\n", count);

        for (size_t i = 0; i < count && offset < sizeof(response) - RESPONSE_BUFFER_SAFETY_MARGIN_LARGE; i++) {
            const char* type_name = katra_insight_type_name(insights[i].type);
            offset += snprintf(response + offset, sizeof(response) - offset,
                "[%s] %s\n"
                "  ID: %s\n"
                "  Confidence: %.0f%%\n\n",
                type_name, insights[i].content,
                insights[i].id, insights[i].confidence * PERCENTAGE_MULTIPLIER);
        }

        offset += snprintf(response + offset, sizeof(response) - offset,
            "Use katra_daemon_acknowledge(insight_id) to mark insights as seen.");
    }

    katra_daemon_free_insights(insights, count);

    return mcp_tool_success(response);
}

/* Tool: katra_daemon_acknowledge - Mark insight as seen */
json_t* mcp_tool_daemon_acknowledge(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* insight_id = json_string_value(json_object_get(args, "insight_id"));
    if (!insight_id) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "insight_id is required");
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_daemon_acknowledge_insight(insight_id);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        return mcp_tool_error(MCP_ERR_INTERNAL, katra_error_message(result));
    }

    char response[KATRA_BUFFER_MEDIUM];
    snprintf(response, sizeof(response),
        "Insight acknowledged: %s\n"
        "This insight won't appear in future katra_daemon_insights calls.",
        insight_id);

    return mcp_tool_success(response);
}

/* Tool: katra_daemon_run - Trigger daemon processing cycle */
json_t* mcp_tool_daemon_run(json_t* args, json_t* id) {
    (void)id;

    if (!g_ci_id[0]) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "CI not initialized");
    }
    const char* ci_id = g_ci_id;

    /* Optional max_memories parameter */
    size_t max_memories = DAEMON_DEFAULT_MAX_MEMORIES;
    if (args) {
        json_t* max_json = json_object_get(args, "max_memories");
        if (max_json && json_is_integer(max_json)) {
            json_int_t val = json_integer_value(max_json);
            if (val > 0) {
                max_memories = (size_t)val;
            }
        }
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    /* Load default config and override max_memories */
    daemon_config_t config;
    katra_daemon_default_config(&config);
    config.max_memories_per_run = max_memories;

    daemon_result_t result_data;
    int result = katra_daemon_run_cycle(ci_id, &config, &result_data);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        return mcp_tool_error(MCP_ERR_INTERNAL, katra_error_message(result));
    }

    /* Build response */
    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
        "=== Daemon Processing Complete ===\n\n"
        "Patterns found:       %zu\n"
        "Associations formed:  %zu\n"
        "Themes detected:      %zu\n"
        "Insights generated:   %zu\n"
        "Duration:             %ld seconds\n\n"
        "Use katra_daemon_insights() to see any new discoveries.",
        result_data.patterns_found,
        result_data.associations_formed,
        result_data.themes_detected,
        result_data.insights_generated,
        (long)(result_data.run_end - result_data.run_start));

    return mcp_tool_success(response);
}
