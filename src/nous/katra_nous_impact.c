/* © 2025 Casey Koons All rights reserved */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "katra_nous.h"
#include "katra_error.h"
#include "katra_log.h"

/* Maximum dependencies and changes to track */
#define MAX_DEPENDENCIES NOUS_MAX_DEPENDENCIES
#define MAX_CHANGE_RECORDS NOUS_MAX_CHANGE_RECORDS

/* Impact analysis state */
static struct {
    dependency_t** dependencies;
    size_t dependency_count;
    size_t dependency_capacity;

    change_record_t** changes;
    size_t change_count;
    size_t change_capacity;

    size_t next_change_id;
} g_impact_state = {0};

/* Helper: Calculate transitive dependencies (simplified BFS) */
static size_t find_affected_items(const char* target,
                                  char*** affected_out,
                                  size_t max_depth) {
    if (!target || max_depth == 0) {
        return 0;
    }

    /* Simplified: just return direct dependencies for Phase 5C */
    size_t count = 0;
    for (size_t i = 0; i < g_impact_state.dependency_count; i++) {
        if (strcmp(g_impact_state.dependencies[i]->target, target) == 0) {
            count++;
        }
    }

    if (count == 0) {
        return 0;
    }

    char** affected = calloc(count, sizeof(char*));
    if (!affected) {
        return 0;
    }

    size_t idx = 0;
    for (size_t i = 0; i < g_impact_state.dependency_count; i++) {
        if (strcmp(g_impact_state.dependencies[i]->target, target) == 0) {
            affected[idx++] = strdup(g_impact_state.dependencies[i]->source);
        }
    }

    *affected_out = affected;
    return count;
}

/* Helper: Calculate risk score based on dependencies and history */
static float calculate_risk_score(const char* target, size_t affected_count) {
    (void)target;  /* Unused in simplified Phase 5C */

    /* Factor 1: Number of dependencies (more = higher risk) */
    float dependency_risk = fminf(1.0f, affected_count / NOUS_DEPENDENCY_SCALE);

    /* Factor 2: Historical failure rate for similar changes */
    size_t similar_failed = 0;
    size_t similar_total = 0;

    for (size_t i = 0; i < g_impact_state.change_count; i++) {
        change_record_t* change = g_impact_state.changes[i];
        /* Simplified: consider all changes as potentially similar */
        similar_total++;
        if (!change->successful) {
            similar_failed++;
        }
    }

    float historical_risk = similar_total > 0 ?
        (float)similar_failed / (float)similar_total : 0.5f;

    /* Combined risk (weighted) */
    float risk = dependency_risk * 0.6f + historical_risk * 0.4f;

    return risk;
}

/* Initialize Phase 5C */
int katra_phase5c_init(void) {
    if (g_impact_state.dependencies) {
        return KATRA_SUCCESS;  /* Already initialized */
    }

    g_impact_state.dependency_capacity = MAX_DEPENDENCIES;
    g_impact_state.dependencies = calloc(g_impact_state.dependency_capacity,
                                        sizeof(dependency_t*));
    if (!g_impact_state.dependencies) {
        return E_SYSTEM_MEMORY;
    }

    g_impact_state.change_capacity = MAX_CHANGE_RECORDS;
    g_impact_state.changes = calloc(g_impact_state.change_capacity,
                                   sizeof(change_record_t*));
    if (!g_impact_state.changes) {
        free(g_impact_state.dependencies);
        g_impact_state.dependencies = NULL;
        return E_SYSTEM_MEMORY;
    }

    g_impact_state.dependency_count = 0;
    g_impact_state.change_count = 0;
    g_impact_state.next_change_id = 1;

    LOG_INFO("Phase 5C impact analysis initialized");
    return KATRA_SUCCESS;
}

/* Cleanup Phase 5C */
void katra_phase5c_cleanup(void) {
    if (!g_impact_state.dependencies) {
        return;
    }

    /* Free dependencies */
    for (size_t i = 0; i < g_impact_state.dependency_count; i++) {
        free(g_impact_state.dependencies[i]->source);
        free(g_impact_state.dependencies[i]->target);
        free(g_impact_state.dependencies[i]);
    }
    free(g_impact_state.dependencies);

    /* Free change records */
    for (size_t i = 0; i < g_impact_state.change_count; i++) {
        free(g_impact_state.changes[i]->description);
        free(g_impact_state.changes[i]->issues_description);
        free(g_impact_state.changes[i]);
    }
    free(g_impact_state.changes);

    memset(&g_impact_state, 0, sizeof(g_impact_state));

    LOG_INFO("Phase 5C impact analysis cleaned up");
}

/* Add dependency */
int katra_phase5c_add_dependency(
    const char* source,
    const char* target,
    dependency_type_t type,
    float strength
) {
    if (!source || !target) {
        return E_INPUT_NULL;
    }

    if (g_impact_state.dependency_count >= g_impact_state.dependency_capacity) {
        LOG_ERROR("Dependency store full (%zu)", g_impact_state.dependency_count);
        return E_SYSTEM_MEMORY;
    }

    dependency_t* dep = calloc(1, sizeof(dependency_t));
    if (!dep) {
        return E_SYSTEM_MEMORY;
    }

    dep->source = strdup(source);
    dep->target = strdup(target);
    dep->type = type;
    dep->strength = strength;
    dep->discovered = time(NULL);

    if (!dep->source || !dep->target) {
        free(dep->source);
        free(dep->target);
        free(dep);
        return E_SYSTEM_MEMORY;
    }

    g_impact_state.dependencies[g_impact_state.dependency_count++] = dep;

    LOG_DEBUG("Added dependency: %s -> %s (strength=%.2f)",
              source, target, strength);

    return KATRA_SUCCESS;
}

/* Predict impact */
impact_prediction_t* katra_phase5c_predict_impact(const char* change_target) {
    if (!change_target) {
        return NULL;
    }

    impact_prediction_t* prediction = calloc(1, sizeof(impact_prediction_t));
    if (!prediction) {
        return NULL;
    }

    prediction->change_target = strdup(change_target);
    if (!prediction->change_target) {
        free(prediction);
        return NULL;
    }

    /* Find affected items */
    char** affected = NULL;
    size_t affected_count = find_affected_items(change_target, &affected, 3);

    prediction->affected_functions = affected;
    prediction->affected_function_count = affected_count;

    /* Determine severity based on affected count */
    if (affected_count == 0) {
        prediction->severity = IMPACT_NONE;
    } else if (affected_count <= 2) {
        prediction->severity = IMPACT_LOW;
    } else if (affected_count <= 5) {
        prediction->severity = IMPACT_MEDIUM;
    } else if (affected_count <= (size_t)NOUS_DEPENDENCY_SCALE) {
        prediction->severity = IMPACT_HIGH;
    } else {
        prediction->severity = IMPACT_CRITICAL;
    }

    /* Calculate risk score */
    prediction->risk_score = calculate_risk_score(change_target, affected_count);

    /* Calculate confidence based on historical data */
    prediction->similar_changes = g_impact_state.change_count;
    if (g_impact_state.change_count > 0) {
        size_t successful = 0;
        for (size_t i = 0; i < g_impact_state.change_count; i++) {
            if (g_impact_state.changes[i]->successful) {
                successful++;
            }
        }
        prediction->historical_success =
            (float)successful / (float)g_impact_state.change_count;
        prediction->confidence = 0.5f + (prediction->historical_success * 0.3f);
    } else {
        prediction->historical_success = 0.5f;
        prediction->confidence = 0.5f;  /* No history */
    }

    /* Generate risk explanation */
    char explanation[NOUS_MEDIUM_BUFFER];
    const char* severity_str = prediction->severity == IMPACT_NONE ? "none" :
                               prediction->severity == IMPACT_LOW ? "low" :
                               prediction->severity == IMPACT_MEDIUM ? "medium" :
                               prediction->severity == IMPACT_HIGH ? "high" : "critical";

    snprintf(explanation, sizeof(explanation),
            "Predicted impact: %s (%zu affected items). "
            "Risk score: %.0f%%. Based on %zu historical changes.",
            severity_str, affected_count,
            prediction->risk_score * NOUS_PERCENT_MULTIPLIER,
            prediction->similar_changes);

    prediction->risk_explanation = strdup(explanation);

    LOG_INFO("Predicted impact for '%s': severity=%s, risk=%.2f, affected=%zu",
             change_target, severity_str, prediction->risk_score, affected_count);

    return prediction;
}

/* Record change */
int katra_phase5c_record_change(
    const char* description,
    size_t files_changed,
    size_t functions_affected,
    bool successful,
    const char* issues
) {
    if (!description) {
        return E_INPUT_NULL;
    }

    if (g_impact_state.change_count >= g_impact_state.change_capacity) {
        LOG_ERROR("Change record store full (%zu)", g_impact_state.change_count);
        return E_SYSTEM_MEMORY;
    }

    change_record_t* change = calloc(1, sizeof(change_record_t));
    if (!change) {
        return E_SYSTEM_MEMORY;
    }

    snprintf(change->change_id, sizeof(change->change_id),
            "change_%zu", g_impact_state.next_change_id++);

    change->description = strdup(description);
    change->timestamp = time(NULL);
    change->files_changed = files_changed;
    change->functions_affected = functions_affected;
    change->successful = successful;
    change->caused_issues = (issues != NULL);
    change->issues_description = issues ? strdup(issues) : NULL;

    /* Calculate actual impact based on scope */
    float scope_factor = fminf(1.0f,
        (files_changed + functions_affected) / NOUS_IMPACT_SCALE);
    change->actual_impact = successful ?
        scope_factor * 0.5f : scope_factor;  /* Failed changes have higher impact */

    if (!change->description) {
        free(change);
        return E_SYSTEM_MEMORY;
    }

    g_impact_state.changes[g_impact_state.change_count++] = change;

    LOG_INFO("Recorded change '%s': %zu files, %zu functions, %s",
             change->change_id, files_changed, functions_affected,
             successful ? "successful" : "failed");

    return KATRA_SUCCESS;
}

/* Get dependencies */
dependency_t** katra_phase5c_get_dependencies(
    const char* target,
    size_t* count
) {
    if (!target || !count) {
        return NULL;
    }

    *count = 0;

    /* Count matching dependencies */
    size_t match_count = 0;
    for (size_t i = 0; i < g_impact_state.dependency_count; i++) {
        if (strcmp(g_impact_state.dependencies[i]->target, target) == 0) {
            match_count++;
        }
    }

    if (match_count == 0) {
        return NULL;
    }

    /* Allocate result array */
    dependency_t** results = calloc(match_count, sizeof(dependency_t*));
    if (!results) {
        return NULL;
    }

    /* Populate results */
    size_t idx = 0;
    for (size_t i = 0; i < g_impact_state.dependency_count; i++) {
        if (strcmp(g_impact_state.dependencies[i]->target, target) == 0) {
            results[idx++] = g_impact_state.dependencies[i];
        }
    }

    *count = match_count;
    return results;
}

/* Free prediction */
void katra_phase5c_free_prediction(impact_prediction_t* prediction) {
    if (!prediction) {
        return;
    }

    free(prediction->change_target);
    free(prediction->risk_explanation);

    for (size_t i = 0; i < prediction->affected_function_count; i++) {
        free(prediction->affected_functions[i]);
    }
    free(prediction->affected_functions);

    for (size_t i = 0; i < prediction->affected_file_count; i++) {
        free(prediction->affected_files[i]);
    }
    free(prediction->affected_files);

    free(prediction);
}

/* Free dependencies */
void katra_phase5c_free_dependencies(dependency_t** deps, size_t count) {
    (void)count;  /* Unused - deps are owned by state, just free array */

    if (!deps) {
        return;
    }

    free(deps);
}
