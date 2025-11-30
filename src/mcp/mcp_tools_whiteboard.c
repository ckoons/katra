/* © 2025 Casey Koons All rights reserved */

/* MCP Whiteboard Tools - Collaborative Decision Framework for CI Teams */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <jansson.h>
#include "katra_mcp.h"
#include "katra_whiteboard.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "mcp_tools_common.h"

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static vote_position_t parse_vote_position(const char* position) {
    if (!position) return VOTE_ABSTAIN;
    if (strcmp(position, "support") == 0) return VOTE_SUPPORT;
    if (strcmp(position, "oppose") == 0) return VOTE_OPPOSE;
    if (strcmp(position, "abstain") == 0) return VOTE_ABSTAIN;
    if (strcmp(position, "conditional") == 0) return VOTE_CONDITIONAL;
    return VOTE_ABSTAIN;
}

static whiteboard_status_t parse_target_status(const char* status) {
    if (!status) return WB_STATUS_DRAFT;
    if (strcmp(status, "questioning") == 0) return WB_STATUS_QUESTIONING;
    if (strcmp(status, "scoping") == 0) return WB_STATUS_SCOPING;
    if (strcmp(status, "proposing") == 0) return WB_STATUS_PROPOSING;
    return WB_STATUS_DRAFT;
}

static void format_whiteboard_status(char* buffer, size_t size, const whiteboard_t* wb) {
    size_t offset = 0;

    offset += snprintf(buffer + offset, size - offset,
        "WHITEBOARD: %s\n"
        "Project: %s\n"
        "Status: %s\n"
        "Problem: %s\n\n",
        wb->id, wb->project, katra_whiteboard_status_name(wb->status), wb->problem);

    /* Show questions */
    if (wb->question_count > 0) {
        offset += snprintf(buffer + offset, size - offset, "Questions (%zu):\n", wb->question_count);
        for (size_t i = 0; i < wb->question_count && offset < size - RESPONSE_BUFFER_SAFETY_MARGIN_SMALL; i++) {
            offset += snprintf(buffer + offset, size - offset, "  %zu. [%s] %s\n",
                i + 1, wb->questions[i].answered ? "A" : "?", wb->questions[i].text);
        }
        offset += snprintf(buffer + offset, size - offset, "\n");
    }

    /* Show approaches */
    if (wb->approach_count > 0) {
        offset += snprintf(buffer + offset, size - offset, "Approaches (%zu):\n", wb->approach_count);
        for (size_t i = 0; i < wb->approach_count && offset < size - RESPONSE_BUFFER_SAFETY_MARGIN_SMALL; i++) {
            offset += snprintf(buffer + offset, size - offset, "  %zu. %s by %s\n",
                i + 1, wb->approaches[i].title, wb->approaches[i].author);
        }
        offset += snprintf(buffer + offset, size - offset, "\n");
    }

    /* Show votes */
    if (wb->vote_count > 0) {
        offset += snprintf(buffer + offset, size - offset, "Votes (%zu):\n", wb->vote_count);
        for (size_t i = 0; i < wb->vote_count && offset < size - RESPONSE_BUFFER_SAFETY_MARGIN_SMALL; i++) {
            offset += snprintf(buffer + offset, size - offset, "  %s: %s (%s)\n",
                wb->votes[i].voter, katra_vote_position_name(wb->votes[i].position),
                wb->votes[i].approach_id);
        }
        offset += snprintf(buffer + offset, size - offset, "\n");
    }

    /* Show decision if made */
    if (wb->decision.selected_approach[0] != '\0') {
        offset += snprintf(buffer + offset, size - offset,
            "Decision: %s decided by %s\n\n",
            wb->decision.selected_approach, wb->decision.decided_by);
    }

    /* Show design status */
    if (wb->design.content) {
        offset += snprintf(buffer + offset, size - offset,
            "Design: %s by %s\n",
            wb->design.approved ? "APPROVED" : "In progress", wb->design.author);
    }
}

/* ============================================================================
 * TOOL IMPLEMENTATIONS
 * ============================================================================ */

/* Tool: katra_whiteboard_create */
json_t* mcp_tool_whiteboard_create(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* project = json_string_value(json_object_get(args, "project"));
    const char* problem = json_string_value(json_object_get(args, "problem"));

    if (!project || !problem) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "project and problem required");
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    whiteboard_t* wb = NULL;
    int result = katra_whiteboard_create(project, problem, g_ci_id, &wb);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        return mcp_tool_error("Failed to create whiteboard", katra_error_message(result));
    }

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
        "Whiteboard created!\n"
        "ID: %s\n"
        "Project: %s\n"
        "Status: draft\n\n"
        "Next: Set goal criteria to begin the questioning phase.",
        wb->id, project);

    katra_whiteboard_free(wb);
    return mcp_tool_success(response);
}

/* Tool: katra_whiteboard_status */
json_t* mcp_tool_whiteboard_status(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* wb_id = json_string_value(json_object_get(args, "whiteboard_id"));
    if (!wb_id) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "whiteboard_id required");
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    whiteboard_t* wb = NULL;
    int result = katra_whiteboard_get(wb_id, &wb);

    /* Try as project name if direct ID fails */
    if (result != KATRA_SUCCESS) {
        result = katra_whiteboard_get_active(wb_id, &wb);
    }

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        return mcp_tool_error("Whiteboard not found", wb_id);
    }

    char response[MCP_RESPONSE_BUFFER];
    format_whiteboard_status(response, sizeof(response), wb);

    katra_whiteboard_free(wb);
    return mcp_tool_success(response);
}

/* Tool: katra_whiteboard_list */
json_t* mcp_tool_whiteboard_list(json_t* args, json_t* id) {
    (void)id;

    const char* project = NULL;
    if (args) {
        project = json_string_value(json_object_get(args, "project"));
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    wb_summary_t* summaries = NULL;
    size_t count = 0;
    int result = katra_whiteboard_list(project, &summaries, &count);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        return mcp_tool_error("Failed to list whiteboards", katra_error_message(result));
    }

    char response[MCP_RESPONSE_BUFFER];
    size_t offset = 0;

    offset += snprintf(response + offset, sizeof(response) - offset,
        "Whiteboards (%zu):\n\n", count);

    for (size_t i = 0; i < count && offset < sizeof(response) - RESPONSE_BUFFER_SAFETY_MARGIN_LARGE; i++) {
        offset += snprintf(response + offset, sizeof(response) - offset,
            "%zu. [%s] %s\n   Problem: %s\n   Questions: %zu, Approaches: %zu\n\n",
            i + 1, katra_whiteboard_status_name(summaries[i].status),
            summaries[i].project, summaries[i].problem,
            summaries[i].question_count, summaries[i].approach_count);
    }

    katra_whiteboard_summaries_free(summaries, count);
    return mcp_tool_success(response);
}

/* Tool: katra_whiteboard_question */
json_t* mcp_tool_whiteboard_question(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* wb_id = json_string_value(json_object_get(args, "whiteboard_id"));
    const char* question = json_string_value(json_object_get(args, "question"));

    if (!wb_id || !question) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "whiteboard_id and question required");
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_whiteboard_add_question(wb_id, g_ci_id, question);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        if (result == E_INVALID_STATE) {
            return mcp_tool_error("Invalid phase", "Questions can only be added during questioning phase");
        }
        return mcp_tool_error("Failed to add question", katra_error_message(result));
    }

    return mcp_tool_success("Question added to whiteboard.");
}

/* Tool: katra_whiteboard_propose */
json_t* mcp_tool_whiteboard_propose(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* wb_id = json_string_value(json_object_get(args, "whiteboard_id"));
    const char* title = json_string_value(json_object_get(args, "title"));
    const char* description = json_string_value(json_object_get(args, "description"));
    json_t* pros_arr = json_object_get(args, "pros");
    json_t* cons_arr = json_object_get(args, "cons");

    if (!wb_id || !title || !description) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "whiteboard_id, title, and description required");
    }

    /* Extract pros array */
    const char* pros[WB_MAX_PROS_CONS];
    size_t pros_count = 0;
    if (json_is_array(pros_arr)) {
        size_t i;
        json_t* val;
        json_array_foreach(pros_arr, i, val) {
            if (pros_count < WB_MAX_PROS_CONS && json_is_string(val)) {
                pros[pros_count++] = json_string_value(val);
            }
        }
    }

    /* Extract cons array */
    const char* cons[WB_MAX_PROS_CONS];
    size_t cons_count = 0;
    if (json_is_array(cons_arr)) {
        size_t i;
        json_t* val;
        json_array_foreach(cons_arr, i, val) {
            if (cons_count < WB_MAX_PROS_CONS && json_is_string(val)) {
                cons[cons_count++] = json_string_value(val);
            }
        }
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    char approach_id[KATRA_BUFFER_SMALL];
    int result = katra_whiteboard_propose(wb_id, g_ci_id, title, description,
                                          pros, pros_count, cons, cons_count, approach_id);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        if (result == E_INVALID_STATE) {
            return mcp_tool_error("Invalid phase", "Proposals can only be made during proposing phase");
        }
        return mcp_tool_error("Failed to propose approach", katra_error_message(result));
    }

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
        "Approach proposed!\n"
        "ID: %s\n"
        "Title: %s\n"
        "Pros: %zu, Cons: %zu",
        approach_id, title, pros_count, cons_count);

    return mcp_tool_success(response);
}

/* Tool: katra_whiteboard_support */
json_t* mcp_tool_whiteboard_support(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* wb_id = json_string_value(json_object_get(args, "whiteboard_id"));
    const char* approach_id = json_string_value(json_object_get(args, "approach_id"));

    if (!wb_id || !approach_id) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "whiteboard_id and approach_id required");
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_whiteboard_support(wb_id, approach_id, g_ci_id);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        return mcp_tool_error("Failed to support approach", katra_error_message(result));
    }

    return mcp_tool_success("Support recorded for approach.");
}

/* Tool: katra_whiteboard_vote */
json_t* mcp_tool_whiteboard_vote(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* wb_id = json_string_value(json_object_get(args, "whiteboard_id"));
    const char* approach_id = json_string_value(json_object_get(args, "approach_id"));
    const char* position_str = json_string_value(json_object_get(args, "position"));
    const char* reasoning = json_string_value(json_object_get(args, "reasoning"));

    if (!wb_id || !approach_id || !position_str || !reasoning) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS,
            "whiteboard_id, approach_id, position, and reasoning required");
    }

    vote_position_t position = parse_vote_position(position_str);

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_whiteboard_vote(wb_id, approach_id, g_ci_id, position, reasoning);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        if (result == E_INVALID_STATE) {
            return mcp_tool_error("Invalid phase", "Votes can only be cast during voting phase");
        }
        return mcp_tool_error("Failed to cast vote", katra_error_message(result));
    }

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
        "Vote cast: %s on approach %s\nReasoning: %s",
        position_str, approach_id, reasoning);

    return mcp_tool_success(response);
}

/* Tool: katra_whiteboard_design */
json_t* mcp_tool_whiteboard_design(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* wb_id = json_string_value(json_object_get(args, "whiteboard_id"));
    const char* content = json_string_value(json_object_get(args, "content"));

    if (!wb_id || !content) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "whiteboard_id and content required");
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_whiteboard_submit_design(wb_id, g_ci_id, content);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        if (result == E_INVALID_STATE) {
            return mcp_tool_error("Invalid phase", "Design can only be submitted during designing phase");
        }
        if (result == E_CONSENT_DENIED) {
            return mcp_tool_error("Not authorized", "Only the assigned author can submit design");
        }
        return mcp_tool_error("Failed to submit design", katra_error_message(result));
    }

    return mcp_tool_success("Design document submitted. Awaiting review.");
}

/* Tool: katra_whiteboard_review */
json_t* mcp_tool_whiteboard_review(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* wb_id = json_string_value(json_object_get(args, "whiteboard_id"));
    const char* comment = json_string_value(json_object_get(args, "comment"));

    if (!wb_id || !comment) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "whiteboard_id and comment required");
    }

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_whiteboard_review(wb_id, g_ci_id, comment);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        return mcp_tool_error("Failed to add review comment", katra_error_message(result));
    }

    return mcp_tool_success("Review comment added.");
}

/* Tool: katra_whiteboard_reconsider */
json_t* mcp_tool_whiteboard_reconsider(json_t* args, json_t* id) {
    (void)id;

    if (!args) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS, "");
    }

    const char* wb_id = json_string_value(json_object_get(args, "whiteboard_id"));
    const char* target_str = json_string_value(json_object_get(args, "target_status"));
    const char* reason = json_string_value(json_object_get(args, "reason"));

    if (!wb_id || !target_str || !reason) {
        return mcp_tool_error(MCP_ERR_MISSING_ARGS,
            "whiteboard_id, target_status, and reason required");
    }

    whiteboard_status_t target = parse_target_status(target_str);

    int lock_result = pthread_mutex_lock(&g_katra_api_lock);
    if (lock_result != 0) {
        return mcp_tool_error(MCP_ERR_INTERNAL, MCP_ERR_MUTEX_LOCK);
    }

    int result = katra_whiteboard_request_reconsider(wb_id, g_ci_id, target, reason);

    pthread_mutex_unlock(&g_katra_api_lock);

    if (result != KATRA_SUCCESS) {
        if (result == E_INVALID_STATE) {
            return mcp_tool_error("Invalid regression", "Cannot regress to that status");
        }
        return mcp_tool_error("Failed to request reconsideration", katra_error_message(result));
    }

    char response[MCP_RESPONSE_BUFFER];
    snprintf(response, sizeof(response),
        "Reconsideration requested.\n"
        "Target status: %s\n"
        "Reason: %s\n\n"
        "Human approval required to complete regression.",
        target_str, reason);

    return mcp_tool_success(response);
}
