/* © 2025 Casey Koons All rights reserved */

/* MCP Core Tools - remember, recall, learn, decide */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_breathing.h"
#include "katra_error.h"
#include "katra_log.h"

/* Global mutex for Katra API access */
pthread_mutex_t g_katra_api_lock = PTHREAD_MUTEX_INITIALIZER;

/* Tool: katra_remember */
json_t* mcp_tool_remember(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error("Missing required arguments", "");
    }

    const char* content = json_string_value(json_object_get(args, "content"));
    const char* context = json_string_value(json_object_get(args, "context"));

    if (!content || !context) {
        return mcp_tool_error("Missing required arguments", "Both 'content' and 'context' are required");
    }

    pthread_mutex_lock(&g_katra_api_lock);
    int result = remember_semantic(content, context);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "%s. %s", msg, suggestion);
        return mcp_tool_error("Failed to store memory", details);
    }

    return mcp_tool_success("Memory stored successfully");
}

/* Tool: katra_recall */
json_t* mcp_tool_recall(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error("Missing required arguments", "");
    }

    const char* topic = json_string_value(json_object_get(args, "topic"));

    if (!topic) {
        return mcp_tool_error("Missing required argument", "'topic' is required");
    }

    size_t count = 0;
    char** results = NULL;

    pthread_mutex_lock(&g_katra_api_lock);
    results = recall_about(topic, &count);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (!results || count == 0) {
        return mcp_tool_success("No memories found for topic");
    }

    /* Truncate large result sets */
    bool truncated = false;
    size_t original_count = count;
    if (count > MCP_MAX_RECALL_RESULTS) {
        truncated = true;
        count = MCP_MAX_RECALL_RESULTS;
    }

    /* Build response text */
    char response[MCP_RESPONSE_BUFFER];
    if (truncated) {
        snprintf(response, sizeof(response),
                "Found %zu memories (showing first %d):\n",
                original_count, MCP_MAX_RECALL_RESULTS);
    } else {
        snprintf(response, sizeof(response),
                "Found %zu memories:\n", count);
    }

    size_t offset = strlen(response);

    for (size_t i = 0; i < count; i++) {
        if (results[i]) {
            offset += snprintf(response + offset, sizeof(response) - offset,
                             "\n%zu. %s", i + 1, results[i]);

            /* Safety check - stop if buffer nearly full */
            if (offset >= sizeof(response) - 100) {
                snprintf(response + offset, sizeof(response) - offset,
                        "\n... (truncated for display)");
                break;
            }
        }
    }

    free_memory_list(results, original_count);

    return mcp_tool_success(response);
}

/* Tool: katra_learn */
json_t* mcp_tool_learn(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error("Missing required arguments", "");
    }

    const char* knowledge = json_string_value(json_object_get(args, "knowledge"));

    if (!knowledge) {
        return mcp_tool_error("Missing required argument", "'knowledge' is required");
    }

    pthread_mutex_lock(&g_katra_api_lock);
    int result = learn(knowledge);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "%s. %s", msg, suggestion);
        return mcp_tool_error("Failed to store knowledge", details);
    }

    return mcp_tool_success("Knowledge stored successfully");
}

/* Tool: katra_decide */
json_t* mcp_tool_decide(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error("Missing required arguments", "");
    }

    const char* decision = json_string_value(json_object_get(args, "decision"));
    const char* reasoning = json_string_value(json_object_get(args, "reasoning"));

    if (!decision || !reasoning) {
        return mcp_tool_error("Missing required arguments", "Both 'decision' and 'reasoning' are required");
    }

    pthread_mutex_lock(&g_katra_api_lock);
    int result = decide(decision, reasoning);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), "%s. %s", msg, suggestion);
        return mcp_tool_error("Failed to store decision", details);
    }

    return mcp_tool_success("Decision stored successfully");
}
