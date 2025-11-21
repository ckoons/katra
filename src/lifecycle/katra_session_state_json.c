/* © 2025 Casey Koons All rights reserved */

/* Session state JSON serialization */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <jansson.h>
#include "katra_session_state.h"
#include "katra_session_state_internal.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

int katra_session_state_to_json(const session_end_state_t* state, char** json_out) {
    KATRA_CHECK_NULL(state);
    KATRA_CHECK_NULL(json_out);

    json_t* root = json_object();
    if (!root) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_session_state_to_json",
                          "Failed to create JSON object");
        return E_SYSTEM_MEMORY;
    }

    /* Temporal context */
    json_object_set_new(root, "session_start", json_integer((json_int_t)state->session_start));
    json_object_set_new(root, "session_end", json_integer((json_int_t)state->session_end));
    json_object_set_new(root, "duration_seconds", json_integer(state->duration_seconds));

    /* Cognitive/emotional state */
    json_object_set_new(root, "cognitive_mode", json_string(cognitive_mode_to_string(state->cognitive_mode)));
    json_object_set_new(root, "cognitive_mode_desc", json_string(state->cognitive_mode_desc));
    json_object_set_new(root, "emotional_state", json_string(emotional_state_to_string(state->emotional_state)));
    json_object_set_new(root, "emotional_state_desc", json_string(state->emotional_state_desc));

    /* Active threads */
    json_t* threads_array = json_array();
    for (int i = 0; i < state->active_thread_count; i++) {
        json_array_append_new(threads_array, json_string(state->active_threads[i]));
    }
    json_object_set_new(root, "active_threads", threads_array);

    /* Next intentions */
    json_t* intentions_array = json_array();
    for (int i = 0; i < state->next_intention_count; i++) {
        json_array_append_new(intentions_array, json_string(state->next_intentions[i]));
    }
    json_object_set_new(root, "next_intentions", intentions_array);

    /* Open questions */
    json_t* questions_array = json_array();
    for (int i = 0; i < state->open_question_count; i++) {
        json_array_append_new(questions_array, json_string(state->open_questions[i]));
    }
    json_object_set_new(root, "open_questions", questions_array);

    /* Insights */
    json_t* insights_array = json_array();
    for (int i = 0; i < state->insight_count; i++) {
        const session_insight_t* insight = &state->insights[i];
        json_t* insight_obj = json_object();
        json_object_set_new(insight_obj, "timestamp", json_integer((json_int_t)insight->timestamp));
        json_object_set_new(insight_obj, "content", json_string(insight->content));
        json_object_set_new(insight_obj, "impact", json_string(insight_impact_to_string(insight->impact)));
        json_object_set_new(insight_obj, "type", json_string(insight_type_to_string(insight->type)));
        json_array_append_new(insights_array, insight_obj);
    }
    json_object_set_new(root, "insights", insights_array);

    /* Convert to string */
    char* json_str = json_dumps(root, JSON_INDENT(2));
    json_decref(root);

    if (!json_str) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_session_state_to_json",
                          "Failed to convert JSON to string");
        return E_SYSTEM_MEMORY;
    }

    *json_out = json_str;
    return KATRA_SUCCESS;
}

int katra_session_state_from_json(const char* json_str, session_end_state_t* state_out) {
    KATRA_CHECK_NULL(json_str);
    KATRA_CHECK_NULL(state_out);

    json_error_t error;
    json_t* root = json_loads(json_str, 0, &error);
    if (!root) {
        katra_report_error(E_INPUT_FORMAT, "katra_session_state_from_json",
                          "Failed to parse JSON");
        return E_INPUT_FORMAT;
    }

    KATRA_INIT_STRUCT(*state_out);

    /* Temporal context */
    json_t* val = json_object_get(root, "session_start");
    if (val && json_is_integer(val)) {
        state_out->session_start = (time_t)json_integer_value(val);
    }

    val = json_object_get(root, "session_end");
    if (val && json_is_integer(val)) {
        state_out->session_end = (time_t)json_integer_value(val);
    }

    val = json_object_get(root, "duration_seconds");
    if (val && json_is_integer(val)) {
        state_out->duration_seconds = (int)json_integer_value(val);
    }

    /* Cognitive/emotional state */
    val = json_object_get(root, "cognitive_mode");
    if (val && json_is_string(val)) {
        state_out->cognitive_mode = cognitive_mode_from_string(json_string_value(val));
    }

    val = json_object_get(root, "cognitive_mode_desc");
    if (val && json_is_string(val)) {
        strncpy(state_out->cognitive_mode_desc, json_string_value(val),
               SESSION_STATE_SHORT_TEXT - 1);
        state_out->cognitive_mode_desc[SESSION_STATE_SHORT_TEXT - 1] = '\0';
    }

    val = json_object_get(root, "emotional_state");
    if (val && json_is_string(val)) {
        state_out->emotional_state = emotional_state_from_string(json_string_value(val));
    }

    val = json_object_get(root, "emotional_state_desc");
    if (val && json_is_string(val)) {
        strncpy(state_out->emotional_state_desc, json_string_value(val),
               SESSION_STATE_SHORT_TEXT - 1);
        state_out->emotional_state_desc[SESSION_STATE_SHORT_TEXT - 1] = '\0';
    }

    /* Active threads */
    val = json_object_get(root, "active_threads");
    if (val && json_is_array(val)) {
        size_t count = json_array_size(val);
        if (count > MAX_ACTIVE_THREADS) count = MAX_ACTIVE_THREADS;
        for (size_t i = 0; i < count; i++) {
            json_t* thread = json_array_get(val, i);
            if (thread && json_is_string(thread)) {
                strncpy(state_out->active_threads[i], json_string_value(thread),
                       SESSION_STATE_ITEM_TEXT - 1);
                state_out->active_threads[i][SESSION_STATE_ITEM_TEXT - 1] = '\0';
                state_out->active_thread_count++;
            }
        }
    }

    /* Next intentions */
    val = json_object_get(root, "next_intentions");
    if (val && json_is_array(val)) {
        size_t count = json_array_size(val);
        if (count > MAX_NEXT_INTENTIONS) count = MAX_NEXT_INTENTIONS;
        for (size_t i = 0; i < count; i++) {
            json_t* intention = json_array_get(val, i);
            if (intention && json_is_string(intention)) {
                strncpy(state_out->next_intentions[i], json_string_value(intention),
                       SESSION_STATE_ITEM_TEXT - 1);
                state_out->next_intentions[i][SESSION_STATE_ITEM_TEXT - 1] = '\0';
                state_out->next_intention_count++;
            }
        }
    }

    /* Open questions */
    val = json_object_get(root, "open_questions");
    if (val && json_is_array(val)) {
        size_t count = json_array_size(val);
        if (count > MAX_OPEN_QUESTIONS) count = MAX_OPEN_QUESTIONS;
        for (size_t i = 0; i < count; i++) {
            json_t* question = json_array_get(val, i);
            if (question && json_is_string(question)) {
                strncpy(state_out->open_questions[i], json_string_value(question),
                       SESSION_STATE_ITEM_TEXT - 1);
                state_out->open_questions[i][SESSION_STATE_ITEM_TEXT - 1] = '\0';
                state_out->open_question_count++;
            }
        }
    }

    /* Insights */
    val = json_object_get(root, "insights");
    if (val && json_is_array(val)) {
        size_t count = json_array_size(val);
        if (count > MAX_SESSION_INSIGHTS) count = MAX_SESSION_INSIGHTS;
        for (size_t i = 0; i < count; i++) {
            json_t* insight_obj = json_array_get(val, i);
            if (!insight_obj || !json_is_object(insight_obj)) continue;

            session_insight_t* insight = &state_out->insights[state_out->insight_count];

            json_t* ts = json_object_get(insight_obj, "timestamp");
            if (ts && json_is_integer(ts)) {
                insight->timestamp = (time_t)json_integer_value(ts);
            }

            json_t* content = json_object_get(insight_obj, "content");
            if (content && json_is_string(content)) {
                strncpy(insight->content, json_string_value(content),
                       SESSION_STATE_INSIGHT_TEXT - 1);
                insight->content[SESSION_STATE_INSIGHT_TEXT - 1] = '\0';
            }

            json_t* impact = json_object_get(insight_obj, "impact");
            if (impact && json_is_string(impact)) {
                insight->impact = insight_impact_from_string(json_string_value(impact));
            }

            json_t* type = json_object_get(insight_obj, "type");
            if (type && json_is_string(type)) {
                insight->type = insight_type_from_string(json_string_value(type));
            }

            state_out->insight_count++;
        }
    }

    json_decref(root);
    return KATRA_SUCCESS;
}
