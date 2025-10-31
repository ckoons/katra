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
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* content = json_string_value(json_object_get(args, MCP_PARAM_CONTENT));
    const char* context = json_string_value(json_object_get(args, MCP_PARAM_CONTEXT));

    if (!content || !context) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, MCP_ERR_BOTH_REQUIRED);
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = remember_semantic(content, context);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error(MCP_ERR_STORE_MEMORY_FAILED, details);
    }

    return mcp_tool_success(MCP_MSG_MEMORY_STORED);
}

/* Tool: katra_recall */
json_t* mcp_tool_recall(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* topic = json_string_value(json_object_get(args, MCP_PARAM_TOPIC));

    if (!topic) {
        return mcp_tool_error(MCP_ERR_MISSING_ARG_QUERY, MCP_ERR_TOPIC_REQUIRED);
    }

    size_t count = 0;
    char** results = NULL;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    results = recall_about(topic, &count);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (!results || count == 0) {
        return mcp_tool_success(MCP_MSG_NO_MEMORIES);
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
        snprintf(response, sizeof(response), MCP_FMT_FOUND_MEMORIES_TRUNCATED,
                original_count, MCP_MAX_RECALL_RESULTS);
    } else {
        snprintf(response, sizeof(response), MCP_FMT_FOUND_MEMORIES, count);
    }

    size_t offset = strlen(response);

    for (size_t i = 0; i < count; i++) {
        if (results[i]) {
            offset += snprintf(response + offset, sizeof(response) - offset,
                             MCP_FMT_MEMORY_ITEM, i + 1, results[i]);

            /* Safety check - stop if buffer nearly full */
            if (offset >= sizeof(response) - 100) {
                snprintf(response + offset, sizeof(response) - offset, MCP_FMT_TRUNCATED);
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
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* knowledge = json_string_value(json_object_get(args, MCP_PARAM_KNOWLEDGE));

    if (!knowledge) {
        return mcp_tool_error(MCP_ERR_MISSING_ARG_QUERY, MCP_ERR_KNOWLEDGE_REQUIRED);
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = learn(knowledge);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error(MCP_ERR_STORE_KNOWLEDGE_FAILED, details);
    }

    return mcp_tool_success(MCP_MSG_KNOWLEDGE_STORED);
}

/* Tool: katra_decide */
json_t* mcp_tool_decide(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* decision = json_string_value(json_object_get(args, MCP_PARAM_DECISION));
    const char* reasoning = json_string_value(json_object_get(args, MCP_PARAM_REASONING));

    if (!decision || !reasoning) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, MCP_ERR_DECISION_REASONING_REQUIRED);
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = decide(decision, reasoning);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error(MCP_ERR_STORE_DECISION_FAILED, details);
    }

    return mcp_tool_success(MCP_MSG_DECISION_STORED);
}
