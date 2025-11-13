/* © 2025 Casey Koons All rights reserved */

/* MCP Identity & Communication Tools - register, whoami, say, hear, who_is_here */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_breathing.h"
#include "katra_lifecycle.h"
#include "katra_identity.h"
#include "katra_meeting.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "mcp_tools_common.h"

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
    if (session->registered) {
        session_end();
    }

    /* ALWAYS use persona name as ci_id (identity preservation fix) */
    char ci_id[KATRA_CI_ID_SIZE];
    strncpy(ci_id, name, sizeof(ci_id) - 1);
    ci_id[sizeof(ci_id) - 1] = '\0';

    /* Check if persona exists and update if needed */
    char old_ci_id[KATRA_CI_ID_SIZE];
    int lookup_result = katra_lookup_persona(name, old_ci_id, sizeof(old_ci_id));

    if (lookup_result == KATRA_SUCCESS) {
        /* Persona exists - check if ci_id needs migration */
        if (strcmp(old_ci_id, ci_id) != 0) {
            LOG_INFO("Migrating persona '%s' from old ci_id '%s' to name-based '%s'",
                    name, old_ci_id, ci_id);
        }
    }

    /* Register or update persona with name-based ci_id */
    int result = katra_register_persona(name, ci_id);
    if (result != KATRA_SUCCESS) {
        pthread_mutex_unlock(&g_katra_api_lock);
        const char* msg = katra_error_message(result);
        char error_details[KATRA_BUFFER_MESSAGE];
        snprintf(error_details, sizeof(error_details),
                "Failed to register persona: %s", msg);
        return mcp_tool_error("Registration failed", error_details);
    }

    LOG_INFO("Registered persona '%s' with ci_id='%s'", name, ci_id);

    /* Update global ci_id */
    strncpy(g_ci_id, ci_id, sizeof(g_ci_id) - 1);
    g_ci_id[sizeof(g_ci_id) - 1] = '\0';

    /* Mark this persona as last active for future sessions */
    katra_update_persona_session(name);

    /* Start new session with ci_id (not name) */
    result = session_start(ci_id);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        char error_details[KATRA_BUFFER_MESSAGE];
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

    /* Register CI in meeting room */
    int lock_result_meeting = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result_meeting == 0) {
        result = meeting_room_register_ci(ci_id, name, role ? role : "assistant");
        pthread_mutex_unlock(&g_katra_api_lock);

        if (result != KATRA_SUCCESS) {
            LOG_WARN("Failed to register CI in meeting room: %d", result);
            /* Non-fatal - continue even if meeting room registration fails */
        }
    }

    /* Update persona for auto-registration (Phase 4.5.1) */
    result = katra_update_persona(ci_id, name, role ? role : "assistant");
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Failed to update persona for auto-registration: %d", result);
        /* Non-fatal - auto-registration will use defaults */
    }

    /* Create welcome memory */
    char welcome[KATRA_BUFFER_MESSAGE];
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

    /* Build success response with recent memories */
    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    if (role && strlen(role) > 0) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                "Welcome back, %s! You're registered as a %s.\n"
                "Your memories will persist under this name.\n\n", name, role);
    } else {
        offset += snprintf(response + offset, sizeof(response) - offset,
                "Welcome back, %s! You're registered.\n"
                "Your memories will persist under this name.\n\n", name);
    }

    /* Get recent memories to show context (default 5 most recent) */
    size_t memory_count = 0;
    char** recent_memories = NULL;

    lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result == 0) {
        recent_memories = recent_thoughts(5, &memory_count);
        pthread_mutex_unlock(&g_katra_api_lock);
    }

    if (recent_memories && memory_count > 0) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                         "Your last %zu memories:\n", memory_count);

        for (size_t i = 0; i < memory_count; i++) {
            if (recent_memories[i]) {
                /* Truncate long memories to first 80 chars */
                char truncated[84];
                strncpy(truncated, recent_memories[i], 80);
                truncated[80] = '\0';
                if (strlen(recent_memories[i]) > 80) {
                    strcat(truncated, "...");
                }

                offset += snprintf(response + offset, sizeof(response) - offset,
                                 "%zu. %s\n", i + 1, truncated);

                /* Safety check */
                if (offset >= sizeof(response) - 200) {
                    break;
                }
            }
        }

        offset += snprintf(response + offset, sizeof(response) - offset,
                         "\nUse katra_recent() for more, or katra_recall(topic) to search.");

        free_memory_list(recent_memories, memory_count);
    } else {
        offset += snprintf(response + offset, sizeof(response) - offset,
                         "This appears to be your first session. Welcome!");
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

/* ============================================================================
 * MEETING ROOM TOOLS - Inter-CI Communication
 * ============================================================================ */

/* Tool: katra_say - Send message to recipient(s) */
json_t* mcp_tool_say(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* message = json_string_value(json_object_get(args, MCP_PARAM_MESSAGE));

    if (!message) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "'message' is required");
    }

    /* Optional recipients parameter (NULL/""/broadcast or "alice,bob") */
    const char* recipients = json_string_value(json_object_get(args, "recipients"));

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_say(message, recipients);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error("Failed to send message", details);
    }

    const char* session_name = mcp_get_session_name();
    char response[MCP_RESPONSE_BUFFER];

    if (!recipients || strlen(recipients) == 0 || strcmp(recipients, "broadcast") == 0) {
        snprintf(response, sizeof(response), "Message broadcast to meeting room, %s!", session_name);
    } else {
        snprintf(response, sizeof(response), "Message sent to %s, %s!", recipients, session_name);
    }

    return mcp_tool_success(response);
}

/* Tool: katra_hear - Receive next message from personal queue */
json_t* mcp_tool_hear(json_t* args, json_t* id) {
    (void)id;
    (void)args;

    heard_message_t message;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_hear(&message);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result == KATRA_NO_NEW_MESSAGES) {
        return mcp_tool_success("No new messages from other CIs");
    }

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error("Failed to hear message", details);
    }

    /* Format message with speaker and content */
    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Message from %s", message.speaker_name);

    if (message.is_direct_message) {
        offset += snprintf(response + offset, sizeof(response) - offset, " (direct message)");
    }

    offset += snprintf(response + offset, sizeof(response) - offset, ":\n%s", message.content);

    if (message.more_available) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "\n\n(More messages waiting - call katra_hear again)");
    }

    return mcp_tool_success(response);
}

/* Tool: katra_who_is_here - List active CIs in meeting room */
json_t* mcp_tool_who_is_here(json_t* args, json_t* id) {
    (void)id;
    (void)args;

    ci_info_t* cis = NULL;
    size_t count = 0;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_who_is_here(&cis, &count);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(result);
        const char* suggestion = katra_error_suggestion(result);
        char details[MCP_ERROR_BUFFER];
        snprintf(details, sizeof(details), MCP_FMT_KATRA_ERROR, msg, suggestion);
        return mcp_tool_error("Failed to list CIs", details);
    }

    if (count == 0) {
        return mcp_tool_success("No other CIs currently in the meeting room");
    }

    /* Format list of CIs */
    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
                      "Active CIs in meeting room (%zu):\n", count);

    for (size_t i = 0; i < count; i++) {
        offset += snprintf(response + offset, sizeof(response) - offset,
                          "- %s (%s)\n", cis[i].name, cis[i].role);
    }

    free(cis);

    return mcp_tool_success(response);
}
