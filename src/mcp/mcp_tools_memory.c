/* © 2025 Casey Koons All rights reserved */

/* MCP Core Tools - remember, recall, learn, decide, personas */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_breathing.h"
#include "katra_lifecycle.h"
#include "katra_identity.h"
#include "katra_meeting.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_vector.h"
#include "mcp_tools_common.h"

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

    const char* session_name = mcp_get_session_name();

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = remember_semantic(content, context);

    /* Auto-generate embedding for semantic search (Phase 6.1) */
    if (result == KATRA_SUCCESS && g_vector_store) {
        /* Generate a simple record ID from content hash (for now) */
        /* TODO: Get actual record_id from remember_semantic */
        char record_id[256];
        snprintf(record_id, sizeof(record_id), "mem_%lu", (unsigned long)time(NULL));

        /* Store embedding (non-fatal if fails) */
        int vec_result = katra_vector_store(g_vector_store, record_id, content);
        if (vec_result != KATRA_SUCCESS) {
            LOG_WARN("Failed to store embedding for memory: %s", record_id);
        } else {
            LOG_DEBUG("Stored embedding for memory: %s", record_id);
        }
    }

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error(MCP_ERR_STORE_MEMORY_FAILED, details);
    }

    /* Personalized response */
    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response), "Memory stored, %s!", session_name);

    return mcp_tool_success(response);
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

    const char* session_name = mcp_get_session_name();
    size_t count = 0;
    char** results = NULL;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    /* Use breathing layer's recall (hybrid or keyword based on config) */
    results = recall_about(topic, &count);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (!results || count == 0) {
        char response[MCP_RESPONSE_BUFFER];
        snprintf(response, sizeof(response), "No memories found about '%s', %s", topic, session_name);
        return mcp_tool_success(response);
    }

    /* Truncate large result sets */
    bool truncated = false;
    size_t original_count = count;
    if (count > MCP_MAX_RECALL_RESULTS) {
        truncated = true;
        count = MCP_MAX_RECALL_RESULTS;
    }

    /* Build response text with personalization */
    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Here are your memories, %s:\n\n", session_name);

    if (truncated) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          MCP_FMT_FOUND_MEMORIES_TRUNCATED,
                          original_count, MCP_MAX_RECALL_RESULTS);
    } else {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          MCP_FMT_FOUND_MEMORIES, count);
    }

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

/* Tool: katra_recent */
json_t* mcp_tool_recent(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    /* Optional limit parameter (defaults to 20) */
    size_t limit = BREATHING_DEFAULT_RECENT_THOUGHTS;
    if (args) {
        json_t* limit_json = json_object_get(args, "limit");
        if (limit_json && json_is_integer(limit_json)) {
            limit = (size_t)json_integer_value(limit_json);
        }
    }

    const char* session_name = mcp_get_session_name();
    size_t count = 0;
    char** results = NULL;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    /* Use breathing layer's recent_thoughts() */
    results = recent_thoughts(limit, &count);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (!results || count == 0) {
        char response[MCP_RESPONSE_BUFFER];
        snprintf(response, sizeof(response), "No recent memories found, %s", session_name);
        return mcp_tool_success(response);
    }

    /* Build response text with personalization */
    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Your recent memories, %s:\n\n", session_name);

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Found %zu recent memories:\n", count);

    for (size_t i = 0; i < count; i++) {
        if (results[i]) {
            offset += snprintf(response + offset, sizeof(response) - offset,
                             "%zu. %s\n", i + 1, results[i]);

            /* Safety check - stop if buffer nearly full */
            if (offset >= sizeof(response) - 100) {
                snprintf(response + offset, sizeof(response) - offset, MCP_FMT_TRUNCATED);
                break;
            }
        }
    }

    free_memory_list(results, count);
    return mcp_tool_success(response);
}

/* Tool: katra_memory_digest */
json_t* mcp_tool_memory_digest(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    /* Optional parameters */
    size_t limit = 10;  /* Default: 10 memories */
    size_t offset = 0;   /* Default: start at newest */

    if (args) {
        json_t* limit_json = json_object_get(args, "limit");
        if (limit_json && json_is_integer(limit_json)) {
            limit = (size_t)json_integer_value(limit_json);
        }

        json_t* offset_json = json_object_get(args, "offset");
        if (offset_json && json_is_integer(offset_json)) {
            offset = (size_t)json_integer_value(offset_json);
        }
    }

    const char* session_name = mcp_get_session_name();

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    memory_digest_t* digest = NULL;
    int result = memory_digest(limit, offset, &digest);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS || !digest) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error("Failed to generate memory digest", details);
    }

    /* Build comprehensive response */
    char response[MCP_RESPONSE_BUFFER];
    size_t resp_offset = 0;

    /* Memory overview */
    resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                           "Memory Digest for %s:\n\n", session_name);

    resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                           "INVENTORY:\n");
    resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                           "- Total: %zu memories\n", digest->total_memories);

    if (digest->oldest_memory > 0) {
        char oldest_date[32];
        struct tm* tm_oldest = localtime(&digest->oldest_memory);
        strftime(oldest_date, sizeof(oldest_date), "%Y-%m-%d", tm_oldest);
        resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                               "- First memory: %s\n", oldest_date);
    }

    if (digest->newest_memory > 0) {
        char newest_date[32];
        struct tm* tm_newest = localtime(&digest->newest_memory);
        strftime(newest_date, sizeof(newest_date), "%Y-%m-%d %H:%M", tm_newest);
        resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                               "- Last memory: %s\n", newest_date);
    }

    /* Topics */
    if (digest->topic_count > 0) {
        resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                               "\nTOPICS (from recent memories):\n");
        size_t topics_to_show = (digest->topic_count > 10) ? 10 : digest->topic_count;
        for (size_t i = 0; i < topics_to_show; i++) {
            resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                                   "- %s (%zu)\n",
                                   digest->topics[i].name, digest->topics[i].count);

            if (resp_offset >= sizeof(response) - 500) break;
        }
    }

    /* Collections */
    if (digest->collection_count > 0) {
        resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                               "\nCOLLECTIONS:\n");
        for (size_t i = 0; i < digest->collection_count; i++) {
            resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                                   "- %s (%zu)\n",
                                   digest->collections[i].name, digest->collections[i].count);

            if (resp_offset >= sizeof(response) - 500) break;
        }
    }

    /* Recent memories */
    if (digest->memory_count > 0) {
        resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                               "\nRECENT MEMORIES (showing %zu", digest->memory_count);
        if (offset > 0) {
            resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                                   ", starting from #%zu", offset + 1);
        }
        resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset, "):\n");

        for (size_t i = 0; i < digest->memory_count; i++) {
            /* Check available space */
            size_t available = sizeof(response) - resp_offset;
            if (available < 300) {
                resp_offset += snprintf(response + resp_offset, available,
                                       "... (buffer limit reached, use smaller limit or recall for specific memories)\n");
                break;
            }

            /* Include full memory content */
            int written = snprintf(response + resp_offset, available,
                                 "%zu. %s\n", offset + i + 1, digest->memories[i]);

            if (written < 0 || (size_t)written >= available) {
                /* Memory too long for remaining buffer */
                resp_offset += snprintf(response + resp_offset, available,
                                       "%zu. [Memory too long - use katra_recall() to retrieve]\n",
                                       offset + i + 1);
            } else {
                resp_offset += written;
            }
        }
    }

    /* Navigation hints */
    resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                           "\nNAVIGATION:\n");
    resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                           "- katra_memory_digest(limit=%zu, offset=%zu) for more\n",
                           limit, offset + limit);
    resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                           "- katra_recall(\"topic\") to search by keyword\n");

    free_memory_digest(digest);
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

    const char* session_name = mcp_get_session_name();

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

    /* Personalized response */
    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response), "Learned, %s!", session_name);

    return mcp_tool_success(response);
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

    const char* session_name = mcp_get_session_name();

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

    /* Personalized response */
    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response), "Decision recorded, %s!", session_name);

    return mcp_tool_success(response);
}

/* Tool: katra_update_metadata */
json_t* mcp_tool_update_metadata(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* memory_id = json_string_value(json_object_get(args, MCP_PARAM_MEMORY_ID));

    if (!memory_id) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "memory_id is required");
    }

    /* Extract optional parameters */
    json_t* personal_json = json_object_get(args, MCP_PARAM_PERSONAL);
    json_t* not_to_archive_json = json_object_get(args, MCP_PARAM_NOT_TO_ARCHIVE);
    const char* collection = json_string_value(json_object_get(args, MCP_PARAM_COLLECTION));

    /* Convert JSON booleans to C booleans */
    bool personal_value = false;
    bool not_to_archive_value = false;
    const bool* personal_ptr = NULL;
    const bool* not_to_archive_ptr = NULL;

    if (personal_json && json_is_boolean(personal_json)) {
        personal_value = json_is_true(personal_json);
        personal_ptr = &personal_value;
    }

    if (not_to_archive_json && json_is_boolean(not_to_archive_json)) {
        not_to_archive_value = json_is_true(not_to_archive_json);
        not_to_archive_ptr = &not_to_archive_value;
    }

    /* If no metadata provided, return error */
    if (!personal_ptr && !not_to_archive_ptr && !collection) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS,
/* GUIDELINE_APPROVED: error message and response text constants */
                            "At least one metadata field must be provided (personal, not_to_archive, or collection)");
    }

    const char* session_name = mcp_get_session_name();

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = update_memory_metadata(memory_id, personal_ptr, not_to_archive_ptr, collection);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error(KATRA_ERR_FAILED_TO_UPDATE_METADATA, details);
    }

    /* Build success response with personalization */
    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response), "Updated metadata for memory %s, %s!", memory_id, session_name);

    return mcp_tool_success(response);
}

