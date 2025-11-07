/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_context_query.c - Context query functions
 *
 * Part of context persistence split. Contains all get_* query functions
 * for retrieving context snapshot information.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_breathing_context_persist.h"
#include "katra_breathing_context_persist_internal.h"
#include "katra_limits.h"

/* ============================================================================
 * QUERY FUNCTIONS
 * ============================================================================ */

const char* get_current_focus_snapshot(const char* ci_id) {
    (void)ci_id;
    working_context_t* ctx = context_persist_get_working_context();
    if (!ctx) return NULL;
    return ctx->current_focus;
}

char** get_pending_questions_snapshot(const char* ci_id, size_t* count) {
    (void)ci_id;
    working_context_t* ctx = context_persist_get_working_context();

    if (!count) return NULL;
    *count = 0;

    if (!ctx) return NULL;

    if (ctx->pending_question_count == 0) {
        return NULL;
    }

    char** questions = malloc(ctx->pending_question_count * sizeof(char*));
    if (!questions) return NULL;

    for (size_t i = 0; i < ctx->pending_question_count; i++) {
        questions[i] = safe_strdup(ctx->pending_questions[i]);
        if (!questions[i]) {
            for (size_t j = 0; j < i; j++) free(questions[j]);
            free(questions);
            return NULL;
        }
    }

    *count = ctx->pending_question_count;
    return questions;
}

char* get_project_state_summary_snapshot(const char* ci_id) {
    (void)ci_id;
    working_context_t* ctx = context_persist_get_working_context();
    if (!ctx) return NULL;
    return safe_strdup(ctx->recent_accomplishments);
}

char* get_relationship_context_snapshot(const char* ci_id) {
    (void)ci_id;
    working_context_t* ctx = context_persist_get_working_context();
    if (!ctx) return NULL;

    if (!ctx->communication_style && !ctx->user_preferences) {
        return NULL;
    }

    size_t size = KATRA_BUFFER_STANDARD;
    char* context = malloc(size);
    if (!context) return NULL;

    size_t offset = 0;
    if (ctx->communication_style) {
        offset += snprintf(context + offset, size - offset,
                          "Communication Style: %s\n",
                          ctx->communication_style);
    }
    if (ctx->user_preferences) {
        offset += snprintf(context + offset, size - offset,
                          "User Preferences: %s\n",
                          ctx->user_preferences);
    }

    return context;
}

void free_context_snapshot(ci_context_snapshot_t* snapshot) {
    if (!snapshot) return;

    free(snapshot->current_focus);
    free(snapshot->active_reasoning);
    free(snapshot->communication_style);
    free(snapshot->user_preferences);
    free(snapshot->recent_accomplishments);
    free(snapshot->active_goals);
    free(snapshot->thinking_patterns);
    free(snapshot->learned_lessons);
    free(snapshot->conversation_summary);
    free(snapshot->context_digest);

    if (snapshot->pending_questions) {
        for (size_t i = 0; i < snapshot->pending_question_count; i++) {
            free(snapshot->pending_questions[i]);
        }
        free(snapshot->pending_questions);
    }

    if (snapshot->modified_files) {
        for (size_t i = 0; i < snapshot->modified_file_count; i++) {
            free(snapshot->modified_files[i]);
        }
        free(snapshot->modified_files);
    }

    free(snapshot);
}
