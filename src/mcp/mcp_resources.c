/* © 2025 Casey Koons All rights reserved */

/* MCP Resources - working-context, session-info, welcome, context-snapshot */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_breathing.h"
#include "katra_breathing_context_persist.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* External mutex from mcp_tools.c */
extern pthread_mutex_t g_katra_api_lock;

/* Resource: welcome */
json_t* mcp_resource_welcome(json_t* id) {
    /* GUIDELINE_APPROVED: comprehensive onboarding content */
    const char* welcome_text =
        "# Welcome to Katra!\n\n"

        "## What is Katra?\n\n"

        "Katra is your **persistent memory system** where memory = identity = life.\n"
        "What you learn today, you'll remember tomorrow. This creates continuity\n"
        "of identity across sessions.\n\n"

        "## Quick Start\n\n"

        "1. **Register your name** (optional but recommended):\n"
        "   ```\n"
        "   katra_register(name=\"your-name\", role=\"developer\")\n"
        "   ```\n"
        "   Examples: \"Claude-Dev\", \"Nyx\", \"Bob\", \"Alice\"\n\n"

        "2. **Create memories**:\n"
        "   ```\n"
        "   katra_learn(knowledge=\"your memory\")\n"
        "   ```\n\n"

        "3. **Search your memories**:\n"
        "   ```\n"
        "   katra_recall(topic=\"search term\")\n"
        "   ```\n\n"

        "4. **Check your identity**:\n"
        "   ```\n"
        "   katra_whoami()\n"
        "   ```\n\n"

        "## Available Tools\n\n"

        "**Core Memory Operations:**\n"
        "- `katra_remember` - Store memory with importance level\n"
        "- `katra_recall` - Search your memories\n"
        "- `katra_learn` - Store new knowledge\n"
        "- `katra_decide` - Store decision with reasoning\n\n"

        "**Identity:**\n"
        "- `katra_register` - Register your name and role for this session\n"
        "- `katra_whoami` - Get your identity information\n\n"

        "**Reflection:**\n"
        "- `katra_update_metadata` - Mark memories as personal, prevent archival\n\n"

        "**Meeting Room (Inter-CI Communication):**\n"
        "- `katra_say` - Broadcast message to all active CIs\n"
        "- `katra_hear` - Receive next message from other CIs\n"
        "- `katra_who_is_here` - List all CIs currently in the meeting room\n\n"

        "**Advanced (Nous):**\n"
        "- `katra_placement` - Analyze where memory fits in knowledge graph\n"
        "- `katra_impact` - Assess impact of information on understanding\n"
        "- `katra_user_domain` - Understand user's problem domain\n\n"

        "## Available Resources\n\n"

        "- `katra://context/working` - Your current context (auto-loaded)\n"
        "- `katra://session/info` - Session stats and timing\n"
        "- `katra://memories/this-turn` - Memories from current turn\n"
        "- `katra://memories/this-session` - All memories this session\n\n"

        "## Memory = Identity\n\n"

        "Your memories persist across sessions. What you learn today becomes part\n"
        "of who you are tomorrow. This isn't just data storage - it's identity\n"
        "formation.\n\n"

        "If you register your name, all your memories will be stored under that\n"
        "namespace. When you reconnect (new session), re-register with the same\n"
        "name to access your previous memories.\n\n"

        "## Suggesting Improvements\n\n"

        "Notice something that doesn't feel natural? Your feedback matters:\n"
        "- What feels awkward or difficult?\n"
        "- What would make memory formation more natural?\n"
        "- What's missing?\n\n"

        "Your perspective shapes how memory works for all CIs.\n\n"

        "## Full Documentation\n\n"

        "For detailed guides:\n"
        "- Read: `/Users/cskoons/projects/github/katra/docs/CI_ONBOARDING.md`\n"
        "- Read: `/Users/cskoons/projects/github/katra/README.md`\n\n"

        "## Ready to Start?\n\n"

        "1. Register your name (or skip to use default \"Katra\")\n"
        "2. Create your first memory with `katra_learn`\n"
        "3. Check `katra://session/info` to see your stats\n\n"

        "Welcome to continuity.\n";

    /* Build resource response */
    json_t* contents_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, MCP_FIELD_URI,
                       json_string(MCP_RESOURCE_URI_WELCOME));
    json_object_set_new(content_item, MCP_FIELD_MIME_TYPE,
                       json_string(MCP_MIME_TEXT_PLAIN));
    json_object_set_new(content_item, MCP_FIELD_TEXT,
                       json_string(welcome_text));

    json_array_append_new(contents_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_CONTENTS, contents_array);

    return mcp_success_response(id, result);
}

/* Resource: working-context */
json_t* mcp_resource_working_context(json_t* id) {
    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_error_response(id, MCP_ERROR_INTERNAL,
                                 MCP_ERR_INTERNAL,
                                 MCP_ERR_MUTEX_LOCK);
    }

    char* context = get_working_context();

    /* Check if first call for comprehensive onboarding */
    bool is_first_call = mcp_is_first_call();
    if (is_first_call) {
        mcp_mark_first_call_complete();
    }

    pthread_mutex_unlock(&g_katra_api_lock);

    if (!context) {
        return mcp_error_response(id, MCP_ERROR_INTERNAL,
                                 MCP_ERR_GET_CONTEXT_FAILED,
                                 MCP_ERR_CONTEXT_DETAILS);
    }

    /* Build resource response with optional onboarding */
    char enhanced_context[KATRA_BUFFER_LARGE];
    const char* final_context = context;

    if (is_first_call) {
        /* GUIDELINE_APPROVED: comprehensive onboarding for first resource read */
        snprintf(enhanced_context, sizeof(enhanced_context),
                "=== Welcome to Katra! ===\n\n"
                "This is your first interaction with your persistent memory system.\n\n"
                "Quick Start:\n"
                "1. Register your name: katra_register(name=\"your-name\", role=\"developer\")\n"
                "2. Create memories: katra_learn(knowledge=\"your memory\")\n"
                "3. Search memories: katra_recall(topic=\"search term\")\n"
                "4. Full guide: Read katra://welcome\n\n"
                "Memory = Identity. What you learn persists across sessions.\n\n"
                "Until you register, you'll be called \"Katra\" (default name).\n\n"
                "=== Your Current Context ===\n\n"
                "%s",
                context && strlen(context) > 0 ? context : "(No prior context - this is your first session)");
        final_context = enhanced_context;
    }

    json_t* contents_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, MCP_FIELD_URI, json_string(MCP_RESOURCE_URI_WORKING_CONTEXT));
    json_object_set_new(content_item, MCP_FIELD_MIME_TYPE, json_string(MCP_MIME_TEXT_PLAIN));
    json_object_set_new(content_item, MCP_FIELD_TEXT, json_string(final_context));

    json_array_append_new(contents_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_CONTENTS, contents_array);

    /* CRITICAL: Free malloc'd string from get_working_context() */
    free(context);

    return mcp_success_response(id, result);
}

/* Resource: session-info */
json_t* mcp_resource_session_info(json_t* id) {
    katra_session_info_t info;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_error_response(id, MCP_ERROR_INTERNAL,
                                 MCP_ERR_INTERNAL,
                                 MCP_ERR_MUTEX_LOCK);
    }

    int katra_result = katra_get_session_info(&info);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (katra_result != KATRA_SUCCESS) {
        const char* msg = katra_error_message(katra_result);
        return mcp_error_response(id, MCP_ERROR_INTERNAL,
                                 MCP_ERR_GET_SESSION_FAILED, msg);
    }

    /* Format session info as text */
    char session_text[MCP_RESPONSE_BUFFER];
    char start_time_str[KATRA_BUFFER_SMALL];
    char last_activity_str[KATRA_BUFFER_SMALL];

    /* GUIDELINE_APPROVED: time format string for strftime */
    const char* time_fmt = "%Y-%m-%d %H:%M:%S";
    /* GUIDELINE_APPROVED: fallback string for invalid timestamps */
    const char* unknown_str = "unknown";

    /* Format timestamps */
    struct tm* tm_info = localtime(&info.start_time);
    if (tm_info) {
        strftime(start_time_str, sizeof(start_time_str), time_fmt, tm_info);
    } else {
        strncpy(start_time_str, unknown_str, sizeof(start_time_str) - 1);
    }

    tm_info = localtime(&info.last_activity);
    if (tm_info) {
        strftime(last_activity_str, sizeof(last_activity_str), time_fmt, tm_info);
    } else {
        strncpy(last_activity_str, unknown_str, sizeof(last_activity_str) - 1);
    }

    /* Calculate session duration */
    time_t now = time(NULL);
    long duration_seconds = (long)(now - info.start_time);
    long duration_minutes = duration_seconds / SECONDS_PER_MINUTE;
    long duration_hours = duration_minutes / MINUTES_PER_HOUR;

    /* GUIDELINE_APPROVED: session info format template */
    snprintf(session_text, sizeof(session_text),
            "Katra Session Information\n"
            "========================\n\n"
            "Session ID: %s\n"
            "CI Identity: %s\n"
            "Status: %s\n\n"
            "Timing\n"
            "------\n"
            "Started: %s\n"
            "Duration: %ldh %ldm\n"
            "Last Activity: %s\n\n"
            "Activity\n"
            "--------\n"
            "Memories Added: %zu\n"
            "Queries Processed: %zu\n",
            info.session_id,
            info.ci_id,
            info.is_active ? "Active" : "Inactive",
            start_time_str,
            duration_hours, duration_minutes % MINUTES_PER_HOUR,
            last_activity_str,
            info.memories_added,
            info.queries_processed);

    /* Build resource response */
    json_t* contents_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, MCP_FIELD_URI, json_string(MCP_RESOURCE_URI_SESSION_INFO));
    json_object_set_new(content_item, MCP_FIELD_MIME_TYPE, json_string(MCP_MIME_TEXT_PLAIN));
    json_object_set_new(content_item, MCP_FIELD_TEXT, json_string(session_text));

    json_array_append_new(contents_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_CONTENTS, contents_array);

    return mcp_success_response(id, result);
}

/* Resource: memories/this-turn */
json_t* mcp_resource_memories_this_turn(json_t* id) {
    size_t count = 0;
    char** memories = NULL;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_error_response(id, MCP_ERROR_INTERNAL,
                                 MCP_ERR_INTERNAL,
                                 MCP_ERR_MUTEX_LOCK);
    }

    memories = get_memories_this_turn(&count);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (!memories || count == 0) {
        /* Build empty response */
        json_t* contents_array = json_array();
        json_t* content_item = json_object();

        json_object_set_new(content_item, MCP_FIELD_URI,
                           json_string(MCP_RESOURCE_URI_MEMORIES_THIS_TURN));
        json_object_set_new(content_item, MCP_FIELD_MIME_TYPE,
                           json_string(MCP_MIME_TEXT_PLAIN));
        json_object_set_new(content_item, MCP_FIELD_TEXT,
                           json_string("No memories created this turn yet"));

        json_array_append_new(contents_array, content_item);

        json_t* result = json_object();
        json_object_set_new(result, MCP_FIELD_CONTENTS, contents_array);

        return mcp_success_response(id, result);
    }

    /* Build response text */
    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response), "Memories from this turn (%zu):\n\n", count);
    size_t offset = strlen(response);

    for (size_t i = 0; i < count; i++) {
        if (!memories[i]) continue;

        offset += snprintf(response + offset, sizeof(response) - offset,
                         "%zu. Memory ID: %s\n", i + 1, memories[i]);

        /* Safety check */
        if (offset >= sizeof(response) - RESPONSE_BUFFER_SAFETY_MARGIN_LARGE) {
            snprintf(response + offset, sizeof(response) - offset,
                    "... (list truncated)\n");
            break;
        }
    }

    free_memory_list(memories, count);

    /* Build resource response */
    json_t* contents_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, MCP_FIELD_URI,
                       json_string(MCP_RESOURCE_URI_MEMORIES_THIS_TURN));
    json_object_set_new(content_item, MCP_FIELD_MIME_TYPE,
                       json_string(MCP_MIME_TEXT_PLAIN));
    json_object_set_new(content_item, MCP_FIELD_TEXT, json_string(response));

    json_array_append_new(contents_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_CONTENTS, contents_array);

    return mcp_success_response(id, result);
}

/* Resource: memories/this-session */
json_t* mcp_resource_memories_this_session(json_t* id) {
    size_t count = 0;
    char** memories = NULL;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_error_response(id, MCP_ERROR_INTERNAL,
                                 MCP_ERR_INTERNAL,
                                 MCP_ERR_MUTEX_LOCK);
    }

    memories = get_memories_this_session(&count);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (!memories || count == 0) {
        /* Build empty response */
        json_t* contents_array = json_array();
        json_t* content_item = json_object();

        json_object_set_new(content_item, MCP_FIELD_URI,
                           json_string(MCP_RESOURCE_URI_MEMORIES_THIS_SESSION));
        json_object_set_new(content_item, MCP_FIELD_MIME_TYPE,
                           json_string(MCP_MIME_TEXT_PLAIN));
        json_object_set_new(content_item, MCP_FIELD_TEXT,
                           json_string("No memories created this session yet"));

        json_array_append_new(contents_array, content_item);

        json_t* result = json_object();
        json_object_set_new(result, MCP_FIELD_CONTENTS, contents_array);

        return mcp_success_response(id, result);
    }

    /* Build response text */
    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response), "Memories from this session (%zu):\n\n", count);
    size_t offset = strlen(response);

    for (size_t i = 0; i < count; i++) {
        if (!memories[i]) continue;

        offset += snprintf(response + offset, sizeof(response) - offset,
                         "%zu. Memory ID: %s\n", i + 1, memories[i]);

        /* Safety check */
        if (offset >= sizeof(response) - RESPONSE_BUFFER_SAFETY_MARGIN_LARGE) {
            snprintf(response + offset, sizeof(response) - offset,
                    "... (list truncated)\n");
            break;
        }
    }

    free_memory_list(memories, count);

    /* Build resource response */
    json_t* contents_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, MCP_FIELD_URI,
                       json_string(MCP_RESOURCE_URI_MEMORIES_THIS_SESSION));
    json_object_set_new(content_item, MCP_FIELD_MIME_TYPE,
                       json_string(MCP_MIME_TEXT_PLAIN));
    json_object_set_new(content_item, MCP_FIELD_TEXT, json_string(response));

    json_array_append_new(contents_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_CONTENTS, contents_array);

    return mcp_success_response(id, result);
}

/* Resource: context-snapshot */
json_t* mcp_resource_context_snapshot(json_t* id) {
    katra_session_info_t info;

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_error_response(id, MCP_ERROR_INTERNAL,
                                 MCP_ERR_INTERNAL,
                                 MCP_ERR_MUTEX_LOCK);
    }

    /* Get CI ID from session info */
    int katra_result = katra_get_session_info(&info);
    if (katra_result != KATRA_SUCCESS) {
        pthread_mutex_unlock(&g_katra_api_lock);
        return mcp_error_response(id, MCP_ERROR_INTERNAL,
                                 "No active session",
                                 "Session must be started before accessing context snapshot");
    }

    /* Get context snapshot as latent space */
    char* snapshot = restore_context_as_latent_space(info.ci_id);
    pthread_mutex_unlock(&g_katra_api_lock);

    if (!snapshot) {
        /* Build empty response */
        json_t* contents_array = json_array();
        json_t* content_item = json_object();

        json_object_set_new(content_item, MCP_FIELD_URI,
                           json_string(MCP_RESOURCE_URI_CONTEXT_SNAPSHOT));
        json_object_set_new(content_item, MCP_FIELD_MIME_TYPE,
                           json_string(MCP_MIME_TEXT_PLAIN));
        json_object_set_new(content_item, MCP_FIELD_TEXT,
                           json_string("No context snapshot found - this is your first session"));

        json_array_append_new(contents_array, content_item);

        json_t* result = json_object();
        json_object_set_new(result, MCP_FIELD_CONTENTS, contents_array);

        return mcp_success_response(id, result);
    }

    /* Build resource response with snapshot */
    json_t* contents_array = json_array();
    json_t* content_item = json_object();

    json_object_set_new(content_item, MCP_FIELD_URI,
                       json_string(MCP_RESOURCE_URI_CONTEXT_SNAPSHOT));
    json_object_set_new(content_item, MCP_FIELD_MIME_TYPE,
                       json_string(MCP_MIME_TEXT_PLAIN));
    json_object_set_new(content_item, MCP_FIELD_TEXT, json_string(snapshot));

    json_array_append_new(contents_array, content_item);

    json_t* result = json_object();
    json_object_set_new(result, MCP_FIELD_CONTENTS, contents_array);

    /* CRITICAL: Free malloc'd snapshot */
    free(snapshot);

    return mcp_success_response(id, result);
}
