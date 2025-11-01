/* © 2025 Casey Koons All rights reserved */
/* Nous Compose: Basic Composition with Error Correction
 * Core composition engine that synthesizes recommendations from multiple sources.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "katra_nous.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_breathing.h"
#include "katra_psyche_common.h"

/* Nous Compose state */
typedef struct {
    char* ci_id;
    bool initialized;
    /* Accuracy tracking per query type */
    struct {
        size_t total_queries;
        size_t accepted;
        size_t rejected;
        size_t modified;
        float accuracy;  /* accepted / total */
    } accuracy[4];  /* One per query_type_t */
} nous_state_t;

static nous_state_t g_nous_state = {0};

/* Query ID counter */
static size_t g_query_counter = 0;

/* Helper: Get query type name */
static const char* query_type_name(query_type_t type) {
    switch (type) {
        case QUERY_TYPE_PLACEMENT: return "placement";
        case QUERY_TYPE_IMPACT: return "impact";
        case QUERY_TYPE_USER_DOMAIN: return "user_domain";
        case QUERY_TYPE_GENERAL: return "general";
        default: return "unknown";
    }
}

/* Helper: Get source type name */
static const char* source_type_name(source_type_t type) __attribute__((unused));
static const char* source_type_name(source_type_t type) {
    switch (type) {
        case SOURCE_MEMORY: return "MEMORY";
        case SOURCE_CODE: return "CODE";
        case SOURCE_PATTERN: return "PATTERN";
        case SOURCE_REASONING: return "REASONING";
        case SOURCE_EXPERIENCE: return "EXPERIENCE";
        default: return "UNKNOWN";
    }
}

/* Initialize Nous system */
int katra_nous_init(const char* ci_id) {
    if (!ci_id) {
        return E_INPUT_NULL;
    }

    if (g_nous_state.initialized) {
        return KATRA_SUCCESS;  /* Already initialized */
    }

    g_nous_state.ci_id = strdup(ci_id);
    if (!g_nous_state.ci_id) {
        return E_SYSTEM_MEMORY;
    }

    /* Initialize accuracy tracking with default 0.5 (no history) */
    for (int i = 0; i < 4; i++) {
        g_nous_state.accuracy[i].accuracy = 0.5f;
    }

    /* Initialize Nous Patterns pattern learning */
    int result = katra_nous_patterns_init();
    if (result != KATRA_SUCCESS) {
        free(g_nous_state.ci_id);
        return result;
    }

    /* Initialize Nous Impact impact analysis */
    result = katra_nous_impact_init();
    if (result != KATRA_SUCCESS) {
        katra_nous_patterns_cleanup();
        free(g_nous_state.ci_id);
        return result;
    }

    /* Initialize Nous Reasoning advanced reasoning */
    result = katra_nous_reasoning_init();
    if (result != KATRA_SUCCESS) {
        katra_nous_impact_cleanup();
        katra_nous_patterns_cleanup();
        free(g_nous_state.ci_id);
        return result;
    }

    /* Initialize Nous Cross-Project cross-project learning */
    result = katra_nous_crossproject_init();
    if (result != KATRA_SUCCESS) {
        katra_nous_reasoning_cleanup();
        katra_nous_impact_cleanup();
        katra_nous_patterns_cleanup();
        free(g_nous_state.ci_id);
        return result;
    }

    g_nous_state.initialized = true;

    LOG_INFO("Nous Compose initialized for CI: %s", ci_id);
    return KATRA_SUCCESS;
}

/* Cleanup Nous system */
void katra_nous_cleanup(void) {
    if (!g_nous_state.initialized) {
        return;
    }

    /* Cleanup Nous Cross-Project cross-project learning */
    katra_nous_crossproject_cleanup();

    /* Cleanup Nous Reasoning advanced reasoning */
    katra_nous_reasoning_cleanup();

    /* Cleanup Nous Impact impact analysis */
    katra_nous_impact_cleanup();

    /* Cleanup Nous Patterns pattern learning */
    katra_nous_patterns_cleanup();

    free(g_nous_state.ci_id);
    memset(&g_nous_state, 0, sizeof(g_nous_state));

    LOG_INFO("Nous Compose cleanup complete");
}

/* Create a composition query */
composition_query_t* katra_nous_create_query(
    const char* query_text,
    query_type_t type
) {
    if (!query_text) {
        return NULL;
    }

    composition_query_t* query = calloc(1, sizeof(composition_query_t));
    if (!query) {
        return NULL;
    }

    /* Generate unique query ID */
    char temp_prefix[NOUS_SMALL_BUFFER];
    snprintf(temp_prefix, sizeof(temp_prefix), "q5_%ld", (long)time(NULL));
    query->query_id = nous_generate_id(temp_prefix, &g_query_counter);
    query->query_text = strdup(query_text);
    query->type = type;

    /* Default configuration */
    query->source_mask = SOURCE_MEMORY | SOURCE_PATTERN;  /* Start simple */
    query->max_results = 3;
    query->min_alternatives = 1;  /* Always at least 1 */
    query->min_confidence = 0.3f;
    query->show_reasoning = true;
    query->show_alternatives = true;  /* Always true */

    if (!query->query_id || !query->query_text) {
        katra_nous_free_query(query);
        return NULL;
    }

    return query;
}

/* Helper: Calculate multi-factor confidence
 * Confidence from 5 factors:
 * - source_agreement: Do sources agree?
 * - evidence_quality: Quality of evidence (CODE > MEMORY > PATTERN)
 * - historical_accuracy: Past accuracy for this query type
 * - query_complexity: Simple > Complex
 * - temporal_recency: Recent > Old
 */
static confidence_breakdown_t calculate_confidence(
    query_type_t query_type,
    size_t source_count,
    time_t oldest_source,
    bool sources_agree
) {
    confidence_breakdown_t conf = {0};

    /* Factor 1: Source Agreement */
    conf.source_agreement = sources_agree ? 1.0f : 0.5f;

    /* Factor 2: Evidence Quality (simplified for Nous Compose) */
    conf.evidence_quality = source_count > 0 ? 0.7f : 0.3f;

    /* Factor 3: Historical Accuracy */
    if (query_type >= 0 && query_type < 4) {
        conf.historical_accuracy = g_nous_state.accuracy[query_type].accuracy;
    } else {
        conf.historical_accuracy = 0.5f;  /* Default: unknown */
    }

    /* Factor 4: Query Complexity (simplified) */
    conf.query_complexity = 0.5f;  /* Medium complexity assumption */

    /* Factor 5: Temporal Recency */
    if (oldest_source > 0) {
        time_t now = time(NULL);
        float age_days = (float)(now - oldest_source) / (NOUS_HOURS_PER_DAY * NOUS_SECONDS_PER_HOUR);
        /* Exponential decay with 90-day half-life */
        conf.temporal_recency = expf(-age_days / NOUS_DECAY_HALFLIFE);
    } else {
        conf.temporal_recency = 0.5f;
    }

    /* Weights (query-type dependent - simplified for Nous Compose) */
    conf.weights[0] = 0.25f;  /* source_agreement */
    conf.weights[1] = 0.25f;  /* evidence_quality */
    conf.weights[2] = 0.20f;  /* historical_accuracy */
    conf.weights[3] = 0.15f;  /* query_complexity */
    conf.weights[4] = 0.15f;  /* temporal_recency */

    /* Combined confidence (weighted sum) */
    conf.overall =
        conf.source_agreement * conf.weights[0] +
        conf.evidence_quality * conf.weights[1] +
        conf.historical_accuracy * conf.weights[2] +
        (1.0f - conf.query_complexity) * conf.weights[3] +  /* Invert complexity */
        conf.temporal_recency * conf.weights[4];

    /* Generate explanation */
    char explanation[KATRA_BUFFER_LARGE];
    snprintf(explanation, sizeof(explanation),
            "Confidence breakdown:\n"
            "  Source agreement: %.0f%%\n"
            "  Evidence quality: %.0f%%\n"
            "  Historical accuracy: %.0f%%\n"
            "  Query simplicity: %.0f%%\n"
            "  Temporal recency: %.0f%%",
            conf.source_agreement * NOUS_PERCENT_MULTIPLIER,
            conf.evidence_quality * NOUS_PERCENT_MULTIPLIER,
            conf.historical_accuracy * NOUS_PERCENT_MULTIPLIER,
            (1.0f - conf.query_complexity) * NOUS_PERCENT_MULTIPLIER,
            conf.temporal_recency * NOUS_PERCENT_MULTIPLIER);

    conf.explanation = katra_safe_strdup(explanation);

    return conf;
}

/* Helper: Synthesize recommendation text based on query type */
static char* synthesize_recommendation(query_type_t type, size_t memory_count) {
    char recommendation[KATRA_BUFFER_LARGE];

    switch (type) {
        case QUERY_TYPE_PLACEMENT:
            snprintf(recommendation, sizeof(recommendation),
                    "Recommended placement: Based on %zu related memories, "
                    "consider placing near similar functionality",
                    memory_count);
            break;

        case QUERY_TYPE_IMPACT:
            snprintf(recommendation, sizeof(recommendation),
                    "Impact analysis: Found %zu related memories. "
                    "Review dependencies before proceeding",
                    memory_count);
            break;

        case QUERY_TYPE_USER_DOMAIN:
            snprintf(recommendation, sizeof(recommendation),
                    "Target users: Based on %zu project memories, "
                    "primary users are technical developers",
                    memory_count);
            break;

        default:
            snprintf(recommendation, sizeof(recommendation),
                    "Based on %zu related memories, recommend careful consideration",
                    memory_count);
            break;
    }

    return katra_safe_strdup(recommendation);
}

/* Helper: Create default alternatives for composition result */
static int create_default_alternatives(composition_result_t* result) {
    if (!result) return E_INPUT_NULL;

    result->alternatives = calloc(2, sizeof(alternative_t));
    if (!result->alternatives) {
        return E_SYSTEM_MEMORY;
    }

    result->alternative_count = 2;

    /* Alternative 1: Conservative approach */
    result->alternatives[0].description =
        katra_safe_strdup("Conservative approach: Maintain current structure");
    result->alternatives[0].pros =
        katra_safe_strdup("Lower risk, proven pattern");
    result->alternatives[0].cons =
        katra_safe_strdup("May not be optimal");
    result->alternatives[0].when_to_use =
        katra_safe_strdup("When stability is priority");
    result->alternatives[0].confidence = 0.6f;

    /* Alternative 2: Experimental approach */
    result->alternatives[1].description =
        katra_safe_strdup("Experimental approach: Try new pattern");
    result->alternatives[1].pros =
        katra_safe_strdup("Potentially better architecture");
    result->alternatives[1].cons =
        katra_safe_strdup("Higher risk, unproven");
    result->alternatives[1].when_to_use =
        katra_safe_strdup("When innovation is priority");
    result->alternatives[1].confidence = 0.4f;

    return KATRA_SUCCESS;
}

/* Helper: Add source attribution to result */
static int add_source_attribution(composition_result_t* result,
                                   size_t memory_count,
                                   time_t oldest_source) {
    if (!result || memory_count == 0) {
        return KATRA_SUCCESS;  /* No sources to add */
    }

    result->sources = calloc(1, sizeof(source_attribution_t));
    if (!result->sources) {
        return E_SYSTEM_MEMORY;
    }

    result->source_count = 1;
    result->sources[0].type = SOURCE_MEMORY;
    result->sources[0].citation = katra_safe_strdup("Project memory");
    result->sources[0].contribution = 1.0f;
    result->sources[0].source_timestamp = oldest_source;

    return KATRA_SUCCESS;
}

/* Helper: Create a simple alternative */
static alternative_t* create_alternative(
    const char* description,
    const char* pros,
    const char* cons,
    const char* when_to_use,
    float confidence
) __attribute__((unused));

static alternative_t* create_alternative(
    const char* description,
    const char* pros,
    const char* cons,
    const char* when_to_use,
    float confidence
) {
    alternative_t* alt = calloc(1, sizeof(alternative_t));
    if (!alt) return NULL;

    alt->description = katra_safe_strdup(description);
    alt->pros = pros ? katra_safe_strdup(pros) : NULL;
    alt->cons = cons ? katra_safe_strdup(cons) : NULL;
    alt->when_to_use = when_to_use ? katra_safe_strdup(when_to_use) : NULL;
    alt->confidence = confidence;

    if (!alt->description) {
        katra_nous_free_alternatives(alt, 1);
        return NULL;
    }

    return alt;
}

/* Execute composition query (Nous Compose simplified implementation)
 * This is a basic implementation that:
 * - Queries memory for relevant information
 * - Synthesizes a simple recommendation
 * - Always includes alternatives
 * - Calculates multi-factor confidence
 */
int katra_nous_compose(composition_query_t* query) {
    int result = KATRA_SUCCESS;
    composition_result_t* comp_result = NULL;
    char** memories = NULL;
    size_t memory_count = 0;

    if (!query || !g_nous_state.initialized) {
        return E_INPUT_NULL;
    }

    LOG_INFO("Nous Compose composing answer for query: %s (type=%s)",
            query->query_text, query_type_name(query->type));

    /* Step 1: Gather context from memory */
    memories = recall_about(query->query_text, &memory_count);
    time_t oldest_source = time(NULL);
    bool sources_agree = true;  /* Simplified for Nous Compose */

    /* Step 2: Allocate result structure */
    comp_result = calloc(1, sizeof(composition_result_t));
    if (!comp_result) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    /* Step 3: Synthesize recommendation */
    comp_result->recommendation = synthesize_recommendation(query->type, memory_count);
    if (!comp_result->recommendation) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    /* Step 4: Add reasoning trace */
    comp_result->reasoning = calloc(1, sizeof(reasoning_step_t));
    if (comp_result->reasoning) {
        comp_result->reasoning_count = 1;
        comp_result->reasoning[0].type = SOURCE_MEMORY;
        comp_result->reasoning[0].description = katra_safe_strdup("Queried project memory");
        comp_result->reasoning[0].confidence = memory_count > 0 ? 0.7f : 0.3f;
        comp_result->reasoning[0].source_timestamp = oldest_source;
    }

    /* Step 5: Create alternatives */
    result = create_default_alternatives(comp_result);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Step 6: Calculate confidence */
    comp_result->confidence = calculate_confidence(
        query->type, memory_count, oldest_source, sources_agree
    );

    /* Step 7: Add source attributions */
    result = add_source_attribution(comp_result, memory_count, oldest_source);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Success: attach result to query */
    query->result = comp_result;
    comp_result = NULL;  /* Transfer ownership */

    LOG_INFO("Nous Compose composed recommendation with confidence=%.2f",
            query->result->confidence.overall);

cleanup:
    if (memories) {
        free_memory_list(memories, memory_count);
    }
    if (comp_result) {
        katra_nous_free_result(comp_result);
    }

    return result;
}

/* Submit feedback on a recommendation */
int katra_nous_submit_feedback(nous_feedback_t* feedback) {
    if (!feedback || !feedback->query_id || !g_nous_state.initialized) {
        return E_INPUT_NULL;
    }

    if (feedback->query_type < 0 || feedback->query_type >= 4) {
        return E_INPUT_RANGE;
    }

    /* Update accuracy tracking */
    size_t idx = (size_t)feedback->query_type;
    g_nous_state.accuracy[idx].total_queries++;

    switch (feedback->outcome) {
        case OUTCOME_ACCEPTED:
            g_nous_state.accuracy[idx].accepted++;
            LOG_INFO("Nous Compose feedback: Query %s ACCEPTED (type=%s)",
                    feedback->query_id, query_type_name(feedback->query_type));
            break;

        case OUTCOME_REJECTED:
            g_nous_state.accuracy[idx].rejected++;
            LOG_INFO("Nous Compose feedback: Query %s REJECTED (type=%s): %s",
                    feedback->query_id, query_type_name(feedback->query_type),
                    feedback->explanation ? feedback->explanation : "no reason given");
            break;

        case OUTCOME_MODIFIED:
            g_nous_state.accuracy[idx].modified++;
            LOG_INFO("Nous Compose feedback: Query %s MODIFIED (type=%s)",
                    feedback->query_id, query_type_name(feedback->query_type));
            break;
    }

    /* Recalculate accuracy */
    if (g_nous_state.accuracy[idx].total_queries > 0) {
        g_nous_state.accuracy[idx].accuracy =
            (float)g_nous_state.accuracy[idx].accepted /
            (float)g_nous_state.accuracy[idx].total_queries;

        LOG_DEBUG("Updated accuracy for %s queries: %.2f%% (%zu/%zu accepted)",
                 query_type_name(feedback->query_type),
                 g_nous_state.accuracy[idx].accuracy * NOUS_PERCENT_MULTIPLIER,
                 g_nous_state.accuracy[idx].accepted,
                 g_nous_state.accuracy[idx].total_queries);
    }

    /* Store feedback as memory for future learning */
    char feedback_memory[KATRA_BUFFER_LARGE];
    snprintf(feedback_memory, sizeof(feedback_memory),
            "Phase 5 feedback: Query '%s' was %s. %s",
            feedback->recommended,
            feedback->outcome == OUTCOME_ACCEPTED ? "accepted" :
            feedback->outcome == OUTCOME_REJECTED ? "rejected" : "modified",
            feedback->explanation ? feedback->explanation : "");

    learn(feedback_memory);

    return KATRA_SUCCESS;
}

/* Get historical accuracy for a query type */
float katra_nous_get_accuracy(query_type_t type) {
    if (!g_nous_state.initialized || type < 0 || type >= 4) {
        return 0.5f;  /* Default: unknown */
    }

    return g_nous_state.accuracy[type].accuracy;
}

/* Free query and results */
void katra_nous_free_query(composition_query_t* query) {
    if (!query) return;

    free(query->query_id);
    free(query->query_text);

    if (query->result) {
        katra_nous_free_result(query->result);
    }

    free(query);
}

/* Helper: Free composition result */
void katra_nous_free_result(composition_result_t* result) {
    if (!result) return;

    free(result->recommendation);

    if (result->reasoning) {
        katra_nous_free_reasoning(result->reasoning, result->reasoning_count);
    }

    if (result->alternatives) {
        katra_nous_free_alternatives(result->alternatives, result->alternative_count);
    }

    if (result->sources) {
        katra_nous_free_sources(result->sources, result->source_count);
    }

    free(result->confidence.explanation);
    free(result);
}

/* Helper: Free alternatives */
void katra_nous_free_alternatives(alternative_t* alts, size_t count) {
    if (!alts) return;

    for (size_t i = 0; i < count; i++) {
        free(alts[i].description);
        free(alts[i].pros);
        free(alts[i].cons);
        free(alts[i].when_to_use);
    }

    free(alts);
}

/* Helper: Free reasoning trace */
void katra_nous_free_reasoning(reasoning_step_t* steps, size_t count) {
    if (!steps) return;

    for (size_t i = 0; i < count; i++) {
        free(steps[i].description);
        if (steps[i].sources) {
            katra_nous_free_sources(steps[i].sources, steps[i].source_count);
        }
    }

    free(steps);
}

/* Helper: Free source attributions */
void katra_nous_free_sources(source_attribution_t* sources, size_t count) {
    if (!sources) return;

    for (size_t i = 0; i < count; i++) {
        free(sources[i].citation);
    }

    free(sources);
}
