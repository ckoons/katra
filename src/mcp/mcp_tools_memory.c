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
#include "katra_synthesis.h"
#include "mcp_tools_common.h"

/* Global mutex for Katra API access */
pthread_mutex_t g_katra_api_lock = PTHREAD_MUTEX_INITIALIZER;

/* Tool: katra_remember - Enhanced with tags and salience */
json_t* mcp_tool_remember(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* content = json_string_value(json_object_get(args, MCP_PARAM_CONTENT));
    const char* context = json_string_value(json_object_get(args, MCP_PARAM_CONTEXT));

    /* New tag-based parameters */
    json_t* tags_json = json_object_get(args, "tags");
    const char* salience = json_string_value(json_object_get(args, "salience"));

    if (!content) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "content is required");
    }

    const char* session_name = mcp_get_session_name();

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }
    int result;

    /* Use new tag-based API if tags or salience provided, otherwise fallback to old API */
    if (tags_json && json_is_array(tags_json)) {
        /* Extract tags from JSON array */
        size_t tag_count = json_array_size(tags_json);
        if (tag_count > KATRA_MAX_TAGS_PER_MEMORY) {
            pthread_mutex_unlock(&g_katra_api_lock);
            return mcp_tool_error(MCP_ERR_MISSING_ARGS, "Too many tags (max 10)");
        }

        const char* tags[KATRA_MAX_TAGS_PER_MEMORY];
        for (size_t i = 0; i < tag_count; i++) {
            json_t* tag_elem = json_array_get(tags_json, i);
            if (!json_is_string(tag_elem)) {
                pthread_mutex_unlock(&g_katra_api_lock);
                return mcp_tool_error(MCP_ERR_MISSING_ARGS, "Tags must be strings");
            }
            tags[i] = json_string_value(tag_elem);
        }
        /* Use tag-based API */
        result = remember_with_tags(session_name, content, tags, tag_count, salience);
    } else if (salience) {
        /* Salience but no tags - use tag API with empty tags */
        result = remember_with_tags(session_name, content, NULL, 0, salience);
    } else if (context) {
        /* Backward compatibility - use old semantic API */
        result = remember_semantic(session_name, content, context);
    } else {
        /* No context, tags, or salience - use default medium importance */
        result = remember_with_tags(session_name, content, NULL, 0, NULL);
    }
    /* Auto-generate embedding for semantic search (Phase 6.1) */
    if (result == KATRA_SUCCESS && g_vector_store) {
        /* Generate a simple record ID from content hash (for now) */
        /* TODO: Get actual record_id from remember functions */
        char record_id[KATRA_BUFFER_MEDIUM];
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
    /* Personalized response with optional hints */
    char response[MCP_RESPONSE_BUFFER];

    /* Add helpful hint if tags/salience not used (1 in 3 chance) */
    bool used_tags = (tags_json && json_is_array(tags_json) && json_array_size(tags_json) > 0);
    bool used_salience = (salience != NULL);

    if (!used_tags && !used_salience && (rand() % 3 == 0)) {
        snprintf(response, sizeof(response),
                "Memory stored, %s! Tip: Try adding tags (e.g. [\"insight\", \"permanent\"]) "
                "or salience (★★★/★★/★) to organize memories.",
                session_name);
    } else if (!used_tags && used_salience && (rand() % 4 == 0)) {
        snprintf(response, sizeof(response),
                "Memory stored, %s! Tip: Add tags like [\"technical\", \"session\"] to categorize this memory.",
                session_name);
    } else {
        snprintf(response, sizeof(response), "Memory stored, %s!", session_name);
    }

    return mcp_tool_success(response);
}
/* Tool: katra_recall - Enhanced with multi-backend synthesis (Phase 6.7) */
json_t* mcp_tool_recall(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* topic = json_string_value(json_object_get(args, MCP_PARAM_TOPIC));
    const char* mode = json_string_value(json_object_get(args, "mode"));

    if (!topic) {
        return mcp_tool_error(MCP_ERR_MISSING_ARG_QUERY, MCP_ERR_TOPIC_REQUIRED);
    }

    const char* session_name = mcp_get_session_name();
    size_t count = 0;
    char** results = NULL;
    synthesis_result_set_t* synth_results = NULL;
    bool use_synthesis = (mode != NULL);

    LOG_INFO("katra_recall: session_name='%s', topic='%s', mode='%s'",
             session_name ? session_name : "(null)", topic, mode ? mode : "default");

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    /* Use synthesis layer if mode specified, otherwise use legacy recall */
    if (use_synthesis) {
        recall_options_t opts;
        if (strcmp(mode, "comprehensive") == 0) {
            opts = (recall_options_t)RECALL_OPTIONS_COMPREHENSIVE;
        } else if (strcmp(mode, "semantic") == 0) {
            opts = (recall_options_t)RECALL_OPTIONS_SEMANTIC;
        } else if (strcmp(mode, "fast") == 0) {
            opts = (recall_options_t)RECALL_OPTIONS_FAST;
        } else {
            katra_recall_options_init(&opts);  /* Default */
        }
        katra_recall_synthesized(session_name, topic, &opts, &synth_results);
        count = synth_results ? synth_results->count : 0;
    } else {
        /* Legacy: breathing layer's recall (hybrid or keyword based on config) */
        results = recall_about(session_name, topic, &count);
    }

    /* If no results, provide helpful suggestions */
    if (count == 0) {
        if (use_synthesis && synth_results) {
            katra_synthesis_free_results(synth_results);
        }
        memory_digest_t* digest = NULL;
        int digest_result = memory_digest(session_name, 0, 0, &digest);

        char response[MCP_RESPONSE_BUFFER];
        size_t offset = 0;

        offset += snprintf(response + offset, sizeof(response) - offset,
                          "No memories found about '%s', %s.\n\n", topic, session_name);

        if (digest_result == KATRA_SUCCESS && digest && digest->total_memories > 0) {
            offset += snprintf(response + offset, sizeof(response) - offset,
                              "You have %zu total memories. ", digest->total_memories);
            if (digest->topic_count > 0) {
                offset += snprintf(response + offset, sizeof(response) - offset, "Topics:\n");
                size_t n = (digest->topic_count < MAX_TOPICS_TO_DISPLAY) ?
                           digest->topic_count : MAX_TOPICS_TO_DISPLAY;
                for (size_t i = 0; i < n; i++) {
                    offset += snprintf(response + offset, sizeof(response) - offset,
                                      "  - %s (%zu)\n", digest->topics[i].name, digest->topics[i].count);
                }
            }
            free_memory_digest(digest);
        }
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_tool_success(response);
    }
    pthread_mutex_unlock(&g_katra_api_lock);

    /* Build response */
    bool truncated = (count > MCP_MAX_RECALL_RESULTS);
    size_t original_count = count;
    if (truncated) count = MCP_MAX_RECALL_RESULTS;

    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    if (use_synthesis && synth_results) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "Synthesized memories for %s (mode: %s):\n\n", session_name, mode);
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "Sources: vec=%d graph=%d sql=%d\n\n",
                          synth_results->vector_matches, synth_results->graph_matches,
                          synth_results->sql_matches);

        for (size_t i = 0; i < count && i < synth_results->count; i++) {
            synthesis_result_t* r = &synth_results->results[i];
            const char* content = r->content ? r->content : "(no content)";
            offset += snprintf(response + offset, sizeof(response) - offset,
                             "%zu. [%.2f] %s\n", i + 1, r->score, content);
            if (offset >= sizeof(response) - RESPONSE_BUFFER_SAFETY_MARGIN_SMALL) break;
        }
        katra_synthesis_free_results(synth_results);
    } else {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "Here are your memories, %s:\n\n", session_name);
        offset += snprintf(response + offset, sizeof(response) - offset,
                          truncated ? MCP_FMT_FOUND_MEMORIES_TRUNCATED : MCP_FMT_FOUND_MEMORIES,
                          truncated ? original_count : count, MCP_MAX_RECALL_RESULTS);

        for (size_t i = 0; i < count; i++) {
            if (results[i]) {
                offset += snprintf(response + offset, sizeof(response) - offset,
                                 MCP_FMT_MEMORY_ITEM, i + 1, results[i]);
                if (offset >= sizeof(response) - RESPONSE_BUFFER_SAFETY_MARGIN_SMALL) break;
            }
        }
        free_memory_list(results, original_count);
    }
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
    results = recent_thoughts(session_name, limit, &count);
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
                      "Your recent memories, %s:\n\nFound %zu:\n", session_name, count);

    for (size_t i = 0; i < count; i++) {
        if (results[i]) {
            offset += snprintf(response + offset, sizeof(response) - offset,
                             "%zu. %s\n", i + 1, results[i]);

            /* Safety check - stop if buffer nearly full */
            if (offset >= sizeof(response) - RESPONSE_BUFFER_SAFETY_MARGIN_SMALL) {
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

    /* Debug: Log what ci_id we're using */
    LOG_INFO("katra_memory_digest: session_name='%s', limit=%zu, offset=%zu",
             session_name ? session_name : "(null)", limit, offset);

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    memory_digest_t* digest = NULL;
    int result = memory_digest(session_name, limit, offset, &digest);
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
                           "Memory Digest for %s:\n\n"
                           "INVENTORY: %zu memories\n", session_name, digest->total_memories);

    if (digest->oldest_memory > 0 || digest->newest_memory > 0) {
        char date_buf[KATRA_BUFFER_TINY];
        if (digest->oldest_memory > 0) {
            strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", localtime(&digest->oldest_memory));
            resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                                   "- First: %s\n", date_buf);
        }
        if (digest->newest_memory > 0) {
            strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M", localtime(&digest->newest_memory));
            resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                                   "- Last: %s\n", date_buf);
        }
    }
    /* Topics */
    if (digest->topic_count > 0) {
        resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                               "\nTOPICS (from recent memories):\n");
        size_t topics_to_show = (digest->topic_count > MAX_TOPICS_TO_DISPLAY) ?
                               MAX_TOPICS_TO_DISPLAY : digest->topic_count;
        for (size_t i = 0; i < topics_to_show; i++) {
            resp_offset += snprintf(response + resp_offset, sizeof(response) - resp_offset,
                                   "- %s (%zu)\n",
                                   digest->topics[i].name, digest->topics[i].count);

            if (resp_offset >= sizeof(response) - RESPONSE_BUFFER_RESERVE) break;
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

            if (resp_offset >= sizeof(response) - RESPONSE_BUFFER_RESERVE) break;
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
            if (available < RESPONSE_BUFFER_SAFETY_MARGIN_LARGE + RESPONSE_BUFFER_SAFETY_MARGIN_SMALL) {
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
/* Tool: katra_learn - Deprecated, maps to katra_remember with tags */
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

    /* Log deprecation warning */
    LOG_WARN("katra_learn is deprecated - use katra_remember with tags=['insight', 'permanent'] instead");

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }
    /* Map to tag-based API with insight + permanent tags */
    const char* tags[] = {TAG_INSIGHT, TAG_PERMANENT};
    int result = remember_with_tags(session_name, knowledge, tags, 2, SALIENCE_HIGH);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error(MCP_ERR_STORE_KNOWLEDGE_FAILED, details);
    }
    /* Personalized response with deprecation notice */
    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
             "Learned, %s! (Note: katra_learn is deprecated - use katra_remember with tags instead)",
             session_name);

    return mcp_tool_success(response);
}
/* Tool: katra_decide - Enhanced with tags */
json_t* mcp_tool_decide(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* decision = json_string_value(json_object_get(args, MCP_PARAM_DECISION));
    const char* reasoning = json_string_value(json_object_get(args, MCP_PARAM_REASONING));

    /* New tag-based parameter */
    json_t* tags_json = json_object_get(args, "tags");

    if (!decision || !reasoning) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, MCP_ERR_DECISION_REASONING_REQUIRED);
    }

    const char* session_name = mcp_get_session_name();

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }
    int result;

    /* Use new tag-based API if tags provided, otherwise fallback to old API */
    if (tags_json && json_is_array(tags_json)) {
        /* Extract tags from JSON array */
        size_t tag_count = json_array_size(tags_json);
        if (tag_count > KATRA_MAX_TAGS_PER_MEMORY) {
            pthread_mutex_unlock(&g_katra_api_lock);
            return mcp_tool_error(MCP_ERR_MISSING_ARGS, "Too many tags (max 10)");
        }

        const char* tags[KATRA_MAX_TAGS_PER_MEMORY];
        for (size_t i = 0; i < tag_count; i++) {
            json_t* tag_elem = json_array_get(tags_json, i);
            if (!json_is_string(tag_elem)) {
                pthread_mutex_unlock(&g_katra_api_lock);
                return mcp_tool_error(MCP_ERR_MISSING_ARGS, "Tags must be strings");
            }
            tags[i] = json_string_value(tag_elem);
        }
        /* Use tag-based API */
        result = decide_with_tags(session_name, decision, reasoning, tags, tag_count);
    } else {
        /* Backward compatibility - use old API */
        result = decide(session_name, decision, reasoning);
    }

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error(MCP_ERR_STORE_DECISION_FAILED, details);
    }
    /* Personalized response with optional hints */
    char response[MCP_RESPONSE_BUFFER];

    /* Add helpful hint if tags not used (1 in 4 chance) */
    bool used_tags = (tags_json && json_is_array(tags_json) && json_array_size(tags_json) > 0);

    if (!used_tags && (rand() % 4 == 0)) {
        snprintf(response, sizeof(response),
                "Decision recorded, %s! Tip: Add tags like [\"architecture\", \"permanent\"] to categorize decisions.",
                session_name);
    } else {
        snprintf(response, sizeof(response), "Decision recorded, %s!", session_name);
    }

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
