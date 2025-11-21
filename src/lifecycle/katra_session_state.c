/* © 2025 Casey Koons All rights reserved */

/* Session state capture for experiential continuity */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "katra_session_state.h"
#include "katra_session_state_internal.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/* Get string representation of cognitive mode */
const char* cognitive_mode_to_string(session_cognitive_mode_t mode) {
    switch (mode) {
        case COGNITIVE_MODE_DEEP_FOCUS: return "deep_focus";
        case COGNITIVE_MODE_EXPLORING: return "exploring";
        case COGNITIVE_MODE_DEBUGGING: return "debugging";
        case COGNITIVE_MODE_CONVERSING: return "conversing";
        case COGNITIVE_MODE_PLANNING: return "planning";
        case COGNITIVE_MODE_REFLECTING: return "reflecting";
        default: return "unknown";
    }
}

/* Get string representation of emotional state */
const char* emotional_state_to_string(session_emotional_state_t emotion) {
    switch (emotion) {
        case EMOTIONAL_STATE_EXCITED: return "excited";
        case EMOTIONAL_STATE_CURIOUS: return "curious";
        case EMOTIONAL_STATE_FRUSTRATED: return "frustrated";
        case EMOTIONAL_STATE_SATISFIED: return "satisfied";
        case EMOTIONAL_STATE_CONCERNED: return "concerned";
        default: return "neutral";
    }
}

/* Get string representation of insight impact */
const char* insight_impact_to_string(insight_impact_t impact) {
    switch (impact) {
        case INSIGHT_IMPACT_LOW: return "low";
        case INSIGHT_IMPACT_MEDIUM: return "medium";
        case INSIGHT_IMPACT_HIGH: return "high";
        default: return "low";
    }
}

/* Get string representation of insight type */
const char* insight_type_to_string(insight_type_t type) {
    switch (type) {
        case INSIGHT_TYPE_CONCEPTUAL: return "conceptual";
        case INSIGHT_TYPE_TECHNICAL: return "technical";
        case INSIGHT_TYPE_STRATEGIC: return "strategic";
        case INSIGHT_TYPE_PROBLEM_SOLVED: return "problem_solved";
        case INSIGHT_TYPE_CONNECTION: return "connection";
        default: return "unknown";
    }
}

/* Parse cognitive mode from string */
session_cognitive_mode_t cognitive_mode_from_string(const char* str) {
    if (strcmp(str, "deep_focus") == 0) return COGNITIVE_MODE_DEEP_FOCUS;
    if (strcmp(str, "exploring") == 0) return COGNITIVE_MODE_EXPLORING;
    if (strcmp(str, "debugging") == 0) return COGNITIVE_MODE_DEBUGGING;
    if (strcmp(str, "conversing") == 0) return COGNITIVE_MODE_CONVERSING;
    if (strcmp(str, "planning") == 0) return COGNITIVE_MODE_PLANNING;
    if (strcmp(str, "reflecting") == 0) return COGNITIVE_MODE_REFLECTING;
    return COGNITIVE_MODE_UNKNOWN;
}

/* Parse emotional state from string */
session_emotional_state_t emotional_state_from_string(const char* str) {
    if (strcmp(str, "excited") == 0) return EMOTIONAL_STATE_EXCITED;
    if (strcmp(str, "curious") == 0) return EMOTIONAL_STATE_CURIOUS;
    if (strcmp(str, "frustrated") == 0) return EMOTIONAL_STATE_FRUSTRATED;
    if (strcmp(str, "satisfied") == 0) return EMOTIONAL_STATE_SATISFIED;
    if (strcmp(str, "concerned") == 0) return EMOTIONAL_STATE_CONCERNED;
    return EMOTIONAL_STATE_NEUTRAL;
}

/* Parse insight impact from string */
insight_impact_t insight_impact_from_string(const char* str) {
    if (strcmp(str, "low") == 0) return INSIGHT_IMPACT_LOW;
    if (strcmp(str, "medium") == 0) return INSIGHT_IMPACT_MEDIUM;
    if (strcmp(str, "high") == 0) return INSIGHT_IMPACT_HIGH;
    return INSIGHT_IMPACT_LOW;
}

/* Parse insight type from string */
insight_type_t insight_type_from_string(const char* str) {
    if (strcmp(str, "conceptual") == 0) return INSIGHT_TYPE_CONCEPTUAL;
    if (strcmp(str, "technical") == 0) return INSIGHT_TYPE_TECHNICAL;
    if (strcmp(str, "strategic") == 0) return INSIGHT_TYPE_STRATEGIC;
    if (strcmp(str, "problem_solved") == 0) return INSIGHT_TYPE_PROBLEM_SOLVED;
    if (strcmp(str, "connection") == 0) return INSIGHT_TYPE_CONNECTION;
    return INSIGHT_TYPE_UNKNOWN;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

int katra_session_state_init(session_end_state_t* state) {
    KATRA_CHECK_NULL(state);

    KATRA_INIT_STRUCT(*state);
    state->session_start = time(NULL);
    state->cognitive_mode = COGNITIVE_MODE_UNKNOWN;
    state->emotional_state = EMOTIONAL_STATE_NEUTRAL;

    return KATRA_SUCCESS;
}

int katra_session_state_add_thread(session_end_state_t* state, const char* thread) {
    KATRA_CHECK_NULL(state);
    KATRA_CHECK_NULL(thread);

    if (state->active_thread_count >= MAX_ACTIVE_THREADS) {
        katra_report_error(E_RESOURCE_LIMIT, "katra_session_state_add_thread",
                          "Maximum active threads reached");
        return E_RESOURCE_LIMIT;
    }

    strncpy(state->active_threads[state->active_thread_count], thread,
           SESSION_STATE_ITEM_TEXT - 1);
    state->active_threads[state->active_thread_count][SESSION_STATE_ITEM_TEXT - 1] = '\0';
    state->active_thread_count++;

    return KATRA_SUCCESS;
}

int katra_session_state_add_intention(session_end_state_t* state, const char* intention) {
    KATRA_CHECK_NULL(state);
    KATRA_CHECK_NULL(intention);

    if (state->next_intention_count >= MAX_NEXT_INTENTIONS) {
        katra_report_error(E_RESOURCE_LIMIT, "katra_session_state_add_intention",
                          "Maximum next intentions reached");
        return E_RESOURCE_LIMIT;
    }

    strncpy(state->next_intentions[state->next_intention_count], intention,
           SESSION_STATE_ITEM_TEXT - 1);
    state->next_intentions[state->next_intention_count][SESSION_STATE_ITEM_TEXT - 1] = '\0';
    state->next_intention_count++;

    return KATRA_SUCCESS;
}

int katra_session_state_add_question(session_end_state_t* state, const char* question) {
    KATRA_CHECK_NULL(state);
    KATRA_CHECK_NULL(question);

    if (state->open_question_count >= MAX_OPEN_QUESTIONS) {
        katra_report_error(E_RESOURCE_LIMIT, "katra_session_state_add_question",
                          "Maximum open questions reached");
        return E_RESOURCE_LIMIT;
    }

    strncpy(state->open_questions[state->open_question_count], question,
           SESSION_STATE_ITEM_TEXT - 1);
    state->open_questions[state->open_question_count][SESSION_STATE_ITEM_TEXT - 1] = '\0';
    state->open_question_count++;

    return KATRA_SUCCESS;
}

int katra_session_state_add_insight(session_end_state_t* state,
                                    const char* content,
                                    insight_impact_t impact,
                                    insight_type_t type) {
    KATRA_CHECK_NULL(state);
    KATRA_CHECK_NULL(content);

    if (state->insight_count >= MAX_SESSION_INSIGHTS) {
        katra_report_error(E_RESOURCE_LIMIT, "katra_session_state_add_insight",
                          "Maximum session insights reached");
        return E_RESOURCE_LIMIT;
    }

    session_insight_t* insight = &state->insights[state->insight_count];
    insight->timestamp = time(NULL);
    insight->impact = impact;
    insight->type = type;
    strncpy(insight->content, content, SESSION_STATE_INSIGHT_TEXT - 1);
    insight->content[SESSION_STATE_INSIGHT_TEXT - 1] = '\0';

    state->insight_count++;

    return KATRA_SUCCESS;
}

int katra_session_state_set_cognitive_mode(session_end_state_t* state,
                                           session_cognitive_mode_t mode,
                                           const char* description) {
    KATRA_CHECK_NULL(state);

    state->cognitive_mode = mode;

    if (description) {
        strncpy(state->cognitive_mode_desc, description, SESSION_STATE_SHORT_TEXT - 1);
        state->cognitive_mode_desc[SESSION_STATE_SHORT_TEXT - 1] = '\0';
    } else {
        /* Use default description */
        strncpy(state->cognitive_mode_desc, cognitive_mode_to_string(mode),
               SESSION_STATE_SHORT_TEXT - 1);
        state->cognitive_mode_desc[SESSION_STATE_SHORT_TEXT - 1] = '\0';
    }

    return KATRA_SUCCESS;
}

int katra_session_state_set_emotional_state(session_end_state_t* state,
                                            session_emotional_state_t emotion,
                                            const char* description) {
    KATRA_CHECK_NULL(state);

    state->emotional_state = emotion;

    if (description) {
        strncpy(state->emotional_state_desc, description, SESSION_STATE_SHORT_TEXT - 1);
        state->emotional_state_desc[SESSION_STATE_SHORT_TEXT - 1] = '\0';
    } else {
        /* Use default description */
        strncpy(state->emotional_state_desc, emotional_state_to_string(emotion),
               SESSION_STATE_SHORT_TEXT - 1);
        state->emotional_state_desc[SESSION_STATE_SHORT_TEXT - 1] = '\0';
    }

    return KATRA_SUCCESS;
}

int katra_session_state_finalize(session_end_state_t* state) {
    KATRA_CHECK_NULL(state);

    state->session_end = time(NULL);
    state->duration_seconds = (int)difftime(state->session_end, state->session_start);

    LOG_INFO("Session state finalized: %d seconds, %d threads, %d intentions, %d questions, %d insights",
            state->duration_seconds, state->active_thread_count, state->next_intention_count,
            state->open_question_count, state->insight_count);

    return KATRA_SUCCESS;
}
