/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_context_update.c - Context update functions
 *
 * Part of context persistence split. Contains all update_* functions
 * for modifying in-memory working context.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_breathing_context_persist.h"
#include "katra_breathing_context_persist_internal.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* ============================================================================
 * UPDATE FUNCTIONS
 * ============================================================================ */

int update_current_focus(const char* focus) {
    working_context_t* ctx = context_persist_get_working_context();

    if (!context_persist_is_initialized() || !ctx) {
        return E_INVALID_STATE;
    }

    if (!focus) {
        return E_INPUT_NULL;
    }

    free(ctx->current_focus);
    ctx->current_focus = safe_strdup(focus);

    if (!ctx->current_focus) {
        return E_SYSTEM_MEMORY;
    }

    LOG_DEBUG("Updated focus: %s", focus);
    return KATRA_SUCCESS;
}

int add_pending_question(const char* question) {
    working_context_t* ctx = context_persist_get_working_context();

    if (!context_persist_is_initialized() || !ctx) {
        return E_INVALID_STATE;
    }

    if (!question) {
        return E_INPUT_NULL;
    }

    /* Grow array if needed */
    if (ctx->pending_question_count >= ctx->pending_question_capacity) {
        size_t new_cap = ctx->pending_question_capacity == 0 ?
            KATRA_INITIAL_CAPACITY_SMALL :
            ctx->pending_question_capacity * BREATHING_GROWTH_FACTOR;

        char** new_array = realloc(ctx->pending_questions,
                                   new_cap * sizeof(char*));
        if (!new_array) {
            return E_SYSTEM_MEMORY;
        }

        ctx->pending_questions = new_array;
        ctx->pending_question_capacity = new_cap;
    }

    /* Add question */
    char* q = safe_strdup(question);
    if (!q) {
        return E_SYSTEM_MEMORY;
    }

    ctx->pending_questions[ctx->pending_question_count++] = q;
    LOG_DEBUG("Added pending question: %s", question);

    return KATRA_SUCCESS;
}

int mark_file_modified(const char* file_path, const char* modification_type) {
    working_context_t* ctx = context_persist_get_working_context();

    if (!context_persist_is_initialized() || !ctx) {
        return E_INVALID_STATE;
    }

    if (!file_path || !modification_type) {
        return E_INPUT_NULL;
    }

    /* Grow array if needed */
    if (ctx->modified_file_count >= ctx->modified_file_capacity) {
        size_t new_cap = ctx->modified_file_capacity == 0 ?
            KATRA_INITIAL_CAPACITY_SMALL :
            ctx->modified_file_capacity * BREATHING_GROWTH_FACTOR;

        char** new_array = realloc(ctx->modified_files,
                                   new_cap * sizeof(char*));
        if (!new_array) {
            return E_SYSTEM_MEMORY;
        }

        ctx->modified_files = new_array;
        ctx->modified_file_capacity = new_cap;
    }

    /* Store as "path:type" */
    char file_info[KATRA_PATH_MAX + KATRA_BUFFER_TINY];
    snprintf(file_info, sizeof(file_info), "%s:%s", file_path, modification_type);

    char* info = safe_strdup(file_info);
    if (!info) {
        return E_SYSTEM_MEMORY;
    }

    ctx->modified_files[ctx->modified_file_count++] = info;
    LOG_DEBUG("Marked file modified: %s", file_info);

    return KATRA_SUCCESS;
}

int record_accomplishment(const char* accomplishment) {
    working_context_t* ctx = context_persist_get_working_context();

    if (!context_persist_is_initialized() || !ctx) {
        return E_INVALID_STATE;
    }

    if (!accomplishment) {
        return E_INPUT_NULL;
    }

    /* Append to existing accomplishments */
    if (ctx->recent_accomplishments) {
        size_t old_len = strlen(ctx->recent_accomplishments);
        size_t new_len = old_len + strlen(accomplishment) + 3;
        char* new_accom = realloc(ctx->recent_accomplishments, new_len);

        if (!new_accom) {
            return E_SYSTEM_MEMORY;
        }

        ctx->recent_accomplishments = new_accom;
        strncat(ctx->recent_accomplishments, "\n- ", new_len - old_len - 1);
        strncat(ctx->recent_accomplishments, accomplishment,
                new_len - strlen(ctx->recent_accomplishments) - 1);
    } else {
        ctx->recent_accomplishments = safe_strdup(accomplishment);
        if (!ctx->recent_accomplishments) {
            return E_SYSTEM_MEMORY;
        }
    }

    LOG_DEBUG("Recorded accomplishment: %s", accomplishment);
    return KATRA_SUCCESS;
}

int update_communication_style(const char* style) {
    working_context_t* ctx = context_persist_get_working_context();

    if (!context_persist_is_initialized() || !ctx) {
        return E_INVALID_STATE;
    }

    if (!style) {
        return E_INPUT_NULL;
    }

    free(ctx->communication_style);
    ctx->communication_style = safe_strdup(style);

    if (!ctx->communication_style) {
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}

int update_user_preferences(const char* preferences) {
    working_context_t* ctx = context_persist_get_working_context();

    if (!context_persist_is_initialized() || !ctx) {
        return E_INVALID_STATE;
    }

    if (!preferences) {
        return E_INPUT_NULL;
    }

    free(ctx->user_preferences);
    ctx->user_preferences = safe_strdup(preferences);

    if (!ctx->user_preferences) {
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}

int update_thinking_patterns(const char* patterns) {
    working_context_t* ctx = context_persist_get_working_context();

    if (!context_persist_is_initialized() || !ctx) {
        return E_INVALID_STATE;
    }

    if (!patterns) {
        return E_INPUT_NULL;
    }

    free(ctx->thinking_patterns);
    ctx->thinking_patterns = safe_strdup(patterns);

    if (!ctx->thinking_patterns) {
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}
