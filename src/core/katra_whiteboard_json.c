/* © 2025 Casey Koons All rights reserved */

/*
 * katra_whiteboard_json.c - JSON Parsing Helpers for Whiteboard
 *
 * Implements JSON parsing functions for goal, scope, and decision structures.
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

/* Project includes */
#include "katra_whiteboard.h"
#include "katra_core_common.h"
#include "katra_psyche_common.h"

/* ============================================================================
 * JSON PARSING HELPERS
 * ============================================================================ */

/* Parse goal JSON: {"criteria": ["criterion1", "criterion2", ...]} */
void wb_parse_goal_json(const char* json_str, wb_goal_t* goal) {
    json_t* root = NULL;
    json_t* criteria = NULL;
    json_error_t error;
    size_t count = 0;
    size_t i = 0;

    if (!json_str || !goal) return;

    root = json_loads(json_str, 0, &error);
    if (!root) return;

    criteria = json_object_get(root, "criteria");
    if (json_is_array(criteria)) {
        count = json_array_size(criteria);
        if (count > WB_MAX_CRITERIA) count = WB_MAX_CRITERIA;

        goal->criteria = calloc(count, sizeof(char*));
        if (goal->criteria) {
            goal->criteria_count = 0;
            for (i = 0; i < count; i++) {
                json_t* item = json_array_get(criteria, i);
                if (json_is_string(item)) {
                    goal->criteria[goal->criteria_count] = katra_safe_strdup(json_string_value(item));
                    if (goal->criteria[goal->criteria_count]) {
                        goal->criteria_count++;
                    }
                }
            }
        }
    }

    json_decref(root);
}

/* Parse scope JSON: {"included": [...], "excluded": [...], "phases": [...]} */
void wb_parse_scope_json(const char* json_str, wb_scope_t* scope) {
    json_t* root = NULL;
    json_t* arr = NULL;
    json_error_t error;
    size_t count = 0;
    size_t i = 0;

    if (!json_str || !scope) return;

    root = json_loads(json_str, 0, &error);
    if (!root) return;

    /* Parse included items */
    arr = json_object_get(root, "included");
    if (json_is_array(arr)) {
        count = json_array_size(arr);
        if (count > WB_MAX_SCOPE_ITEMS) count = WB_MAX_SCOPE_ITEMS;

        scope->included = calloc(count, sizeof(char*));
        if (scope->included) {
            scope->included_count = 0;
            for (i = 0; i < count; i++) {
                json_t* item = json_array_get(arr, i);
                if (json_is_string(item)) {
                    scope->included[scope->included_count] = katra_safe_strdup(json_string_value(item));
                    if (scope->included[scope->included_count]) {
                        scope->included_count++;
                    }
                }
            }
        }
    }

    /* Parse excluded items */
    arr = json_object_get(root, "excluded");
    if (json_is_array(arr)) {
        count = json_array_size(arr);
        if (count > WB_MAX_SCOPE_ITEMS) count = WB_MAX_SCOPE_ITEMS;

        scope->excluded = calloc(count, sizeof(char*));
        if (scope->excluded) {
            scope->excluded_count = 0;
            for (i = 0; i < count; i++) {
                json_t* item = json_array_get(arr, i);
                if (json_is_string(item)) {
                    scope->excluded[scope->excluded_count] = katra_safe_strdup(json_string_value(item));
                    if (scope->excluded[scope->excluded_count]) {
                        scope->excluded_count++;
                    }
                }
            }
        }
    }

    /* Parse phases */
    arr = json_object_get(root, "phases");
    if (json_is_array(arr)) {
        count = json_array_size(arr);
        if (count > WB_MAX_SCOPE_ITEMS) count = WB_MAX_SCOPE_ITEMS;

        scope->phases = calloc(count, sizeof(char*));
        if (scope->phases) {
            scope->phase_count = 0;
            for (i = 0; i < count; i++) {
                json_t* item = json_array_get(arr, i);
                if (json_is_string(item)) {
                    scope->phases[scope->phase_count] = katra_safe_strdup(json_string_value(item));
                    if (scope->phases[scope->phase_count]) {
                        scope->phase_count++;
                    }
                }
            }
        }
    }

    json_decref(root);
}

/* Parse decision JSON: {"selected_approach": "...", "decided_by": "...", "decided_at": N, "notes": "..."} */
void wb_parse_decision_json(const char* json_str, wb_decision_t* decision) {
    json_t* root = NULL;
    json_t* val = NULL;
    json_error_t error;

    if (!json_str || !decision) return;

    root = json_loads(json_str, 0, &error);
    if (!root) return;

    val = json_object_get(root, "selected_approach");
    if (json_is_string(val)) {
        SAFE_STRNCPY(decision->selected_approach, json_string_value(val));
    }

    val = json_object_get(root, "decided_by");
    if (json_is_string(val)) {
        SAFE_STRNCPY(decision->decided_by, json_string_value(val));
    }

    val = json_object_get(root, "decided_at");
    if (json_is_integer(val)) {
        decision->decided_at = json_integer_value(val);
    }

    val = json_object_get(root, "notes");
    if (json_is_string(val)) {
        SAFE_STRNCPY(decision->notes, json_string_value(val));
    }

    json_decref(root);
}
