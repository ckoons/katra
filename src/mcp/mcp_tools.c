/* © 2025 Casey Koons All rights reserved */

/* MCP Core Tools - remember, recall, learn, decide, personas */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_breathing.h"
#include "katra_identity.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* External globals from katra_mcp_server.c */
extern char g_persona_name[256];
extern char g_ci_id[256];

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

    size_t count = 0;
    char** results = NULL;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    results = recall_about(topic, &count);
    pthread_mutex_unlock(&g_katra_api_lock);

    const char* session_name = mcp_get_session_name();

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

/* Tool: katra_my_name_is */
json_t* mcp_tool_my_name_is(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* name = json_string_value(json_object_get(args, "name"));

    if (!name || strlen(name) == 0) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "Name is required");
    }

    /* Check if name already exists */
    char existing_ci_id[256];
    int result = katra_lookup_persona(name, existing_ci_id, sizeof(existing_ci_id));

    if (result == KATRA_SUCCESS) {
        /* Name exists - check if it's us */
        if (strcmp(existing_ci_id, g_ci_id) == 0) {
            char response[512];
            snprintf(response, sizeof(response), "You're already %s", name);
            return mcp_tool_success(response);
        } else {
            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg),
                    "%s is already another persona. Please choose a different name.", name);
            return mcp_tool_error("Name already exists", error_msg);
        }
    }

    /* Check if we already have a non-anonymous name */
    if (strlen(g_persona_name) > 0 && strncmp(g_persona_name, "anonymous_", 10) != 0) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg),
                "You're already %s. Cannot change to %s.", g_persona_name, name);
        return mcp_tool_error("Already named", error_msg);
    }

    /* Register current ci_id with new name */
    result = katra_register_persona(name, g_ci_id);
    if (result != KATRA_SUCCESS) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to register persona");
    }

    /* Update global persona name */
    strncpy(g_persona_name, name, sizeof(g_persona_name) - 1);
    g_persona_name[sizeof(g_persona_name) - 1] = '\0';

    /* Set as last_active */
    katra_set_last_active(name);

    char response[512];
    snprintf(response, sizeof(response), "You are now %s", name);

    LOG_INFO("Persona renamed to: %s", name);

    return mcp_tool_success(response);
}

/* Tool: katra_list_personas */
json_t* mcp_tool_list_personas(json_t* args, json_t* id) {
    (void)args;  /* No arguments */
    (void)id;    /* Unused - id handled by caller */

    persona_info_t** personas = NULL;
    size_t count = 0;

    int result = katra_list_personas(&personas, &count);
    if (result != KATRA_SUCCESS) {
        return mcp_tool_error(MCP_ERR_INTERNAL, "Failed to list personas");
    }

    if (count == 0) {
        return mcp_tool_success("No personas registered");
    }

    /* Build response text */
    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response), "Available personas:\n");
    size_t offset = strlen(response);

    /* Calculate time differences */
    time_t now = time(NULL);

    for (size_t i = 0; i < count; i++) {
        if (!personas[i]) continue;

        /* Calculate how long ago last session was */
        long time_diff = (long)(now - personas[i]->last_session);
        char time_ago[128];

        if (time_diff < (MINUTES_PER_HOUR * SECONDS_PER_MINUTE)) {
            long minutes = time_diff / SECONDS_PER_MINUTE;
            snprintf(time_ago, sizeof(time_ago), "%ld minute%s ago",
                    minutes, minutes == 1 ? "" : "s");
        } else if (time_diff < SECONDS_PER_DAY) {
            long hours = time_diff / (MINUTES_PER_HOUR * SECONDS_PER_MINUTE);
            snprintf(time_ago, sizeof(time_ago), "%ld hour%s ago",
                    hours, hours == 1 ? "" : "s");
        } else {
            long days = time_diff / SECONDS_PER_DAY;
            snprintf(time_ago, sizeof(time_ago), "%ld day%s ago",
                    days, days == 1 ? "" : "s");
        }

        /* Format persona line */
        offset += snprintf(response + offset, sizeof(response) - offset,
                         "- %s (%d session%s, last active %s)\n",
                         personas[i]->name,
                         personas[i]->sessions,
                         personas[i]->sessions == 1 ? "" : "s",
                         time_ago);

        /* Safety check - stop if buffer nearly full */
        if (offset >= sizeof(response) - 200) {
            snprintf(response + offset, sizeof(response) - offset,
                    "...(list truncated)\n");
            break;
        }
    }

    katra_free_persona_list(personas, count);

    return mcp_tool_success(response);
}

/* Tool: katra_review_turn */
json_t* mcp_tool_review_turn(json_t* args, json_t* id) {
    (void)args;  /* No arguments */
    (void)id;    /* Unused - id handled by caller */

    const char* session_name = mcp_get_session_name();

    size_t count = 0;
    char** memories = NULL;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    memories = get_memories_this_turn(&count);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (!memories || count == 0) {
        char response[MCP_RESPONSE_BUFFER];
        snprintf(response, sizeof(response), "No memories created this turn yet, %s", session_name);
        return mcp_tool_success(response);
    }

    /* Build response text with personalization */
    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "%s, here are your memories from this turn (%zu):\n",
                      session_name, count);

    for (size_t i = 0; i < count; i++) {
        if (!memories[i]) continue;

        offset += snprintf(response + offset, sizeof(response) - offset,
                         "\n%zu. Memory ID: %s", i + 1, memories[i]);

        /* Safety check - stop if buffer nearly full */
        if (offset >= sizeof(response) - 200) {
            snprintf(response + offset, sizeof(response) - offset,
                    "\n...(list truncated)\n");
            break;
        }
    }

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "\n\nUse katra_update_metadata with these memory IDs to mark as personal, "
                      "add to collections, or prevent archival.");

    free_memory_list(memories, count);

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
        return mcp_tool_error("Failed to update metadata", details);
    }

    /* Build success response with personalization */
    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response), "Updated metadata for memory %s, %s!", memory_id, session_name);

    return mcp_tool_success(response);
}

/* Tool: katra_register */
json_t* mcp_tool_register(json_t* args, json_t* id) {
    (void)id;  /* Unused - id handled by caller */

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* name = json_string_value(json_object_get(args, MCP_PARAM_NAME));
    const char* role = json_string_value(json_object_get(args, MCP_PARAM_ROLE));

    if (!name || strlen(name) == 0) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "Name is required");
    }

    /* Get session state */
    mcp_session_t* session = mcp_get_session();

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    /* End current session if active */
    session_end();

    /* Start new session with chosen name as namespace */
    int result = session_start(name);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        char error_details[512];
        snprintf(error_details, sizeof(error_details),
                "Failed to start session with name '%s': %s", name, msg);
        return mcp_tool_error("Registration failed", error_details);
    }

    /* Store in session state */
    strncpy(session->chosen_name, name, sizeof(session->chosen_name) - 1);
    session->chosen_name[sizeof(session->chosen_name) - 1] = '\0';

    if (role && strlen(role) > 0) {
        strncpy(session->role, role, sizeof(session->role) - 1);
        session->role[sizeof(session->role) - 1] = '\0';
    }

    session->registered = true;

    /* Create welcome memory */
    char welcome[512];
    if (role && strlen(role) > 0) {
        snprintf(welcome, sizeof(welcome),
                "Session started. My name is %s, I'm a %s.", name, role);
    } else {
        snprintf(welcome, sizeof(welcome),
                "Session started. My name is %s.", name);
    }

    lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result == 0) {
        result = learn(welcome);
        pthread_mutex_unlock(&g_katra_api_lock);

        if (result != KATRA_SUCCESS) {
            LOG_ERROR("Failed to create welcome memory");
            /* Don't fail registration if memory creation fails */
        }
    }

    /* Build success response */
    char response[MCP_RESPONSE_BUFFER];
    if (role && strlen(role) > 0) {
        snprintf(response, sizeof(response),
                "Welcome, %s! You're registered as a %s. "
                "Your memories will persist under this name.", name, role);
    } else {
        snprintf(response, sizeof(response),
                "Welcome, %s! You're registered. "
                "Your memories will persist under this name.", name);
    }

    LOG_INFO("Registered session: %s (role: %s)", name, role ? role : "unspecified");

    return mcp_tool_success(response);
}

/* Tool: katra_whoami */
json_t* mcp_tool_whoami(json_t* args, json_t* id) {
    (void)args;  /* No arguments */
    (void)id;    /* Unused - id handled by caller */

    mcp_session_t* session = mcp_get_session();

    /* Build response */
    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Your Identity:\n\n");

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Name: %s\n", session->chosen_name);

    if (session->registered) {
        if (strlen(session->role) > 0) {
            offset += snprintf(response + offset, sizeof(response) - offset,
                             "Role: %s\n", session->role);
        }
        offset += snprintf(response + offset, sizeof(response) - offset,
                         "Status: Registered\n");
    } else {
        offset += snprintf(response + offset, sizeof(response) - offset,
                         "Status: Not registered (using default name)\n");
    }

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "CI Identity: %s\n", g_ci_id);

    /* Get session info for memory count */
    katra_session_info_t info;
    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result == 0) {
        int result = katra_get_session_info(&info);
        pthread_mutex_unlock(&g_katra_api_lock);

        if (result == KATRA_SUCCESS) {
            offset += snprintf(response + offset, sizeof(response) - offset,
                             "Memories: %zu\n", info.memories_added);
        }
    }

    if (!session->registered) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                         "\nTo register: katra_register(name=\"your-name\", role=\"developer\")");
    }

    return mcp_tool_success(response);
}
