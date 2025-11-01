/* © 2025 Casey Koons All rights reserved */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "katra_nous.h"
#include "katra_error.h"
#include "katra_log.h"

/* Reasoning state capacity limits */
#define MAX_REASONING_CHAINS NOUS_MAX_REASONING_CHAINS
#define MAX_INFERENCE_RULES NOUS_MAX_INFERENCE_RULES

/* Inference rule */
typedef struct {
    char* name;
    char* pattern;
    float confidence;
} inference_rule_t;

/* Nous Reasoning state */
static struct {
    inference_rule_t** rules;
    size_t rule_count;
    size_t rule_capacity;

    size_t next_chain_id;
} g_reasoning_state = {0};

/* Initialize Nous Reasoning */
int katra_nous_reasoning_init(void) {
    if (g_reasoning_state.rules) {
        return KATRA_SUCCESS;  /* Already initialized */
    }

    g_reasoning_state.rule_capacity = MAX_INFERENCE_RULES;
    g_reasoning_state.rules = calloc(g_reasoning_state.rule_capacity,
                                    sizeof(inference_rule_t*));
    if (!g_reasoning_state.rules) {
        return E_SYSTEM_MEMORY;
    }

    g_reasoning_state.rule_count = 0;
    g_reasoning_state.next_chain_id = 1;

    /* Add some default inference rules */
    katra_nous_reasoning_add_rule("modus_ponens", "If A implies B, and A is true, then B is true");
    katra_nous_reasoning_add_rule("transitive", "If A relates to B, and B relates to C, then A relates to C");
    katra_nous_reasoning_add_rule("similar_context", "Similar contexts suggest similar outcomes");

    LOG_INFO("Nous Reasoning advanced reasoning initialized");
    return KATRA_SUCCESS;
}

/* Cleanup Nous Reasoning */
void katra_nous_reasoning_cleanup(void) {
    if (!g_reasoning_state.rules) {
        return;
    }

    for (size_t i = 0; i < g_reasoning_state.rule_count; i++) {
        free(g_reasoning_state.rules[i]->name);
        free(g_reasoning_state.rules[i]->pattern);
        free(g_reasoning_state.rules[i]);
    }
    free(g_reasoning_state.rules);

    memset(&g_reasoning_state, 0, sizeof(g_reasoning_state));

    LOG_INFO("Nous Reasoning advanced reasoning cleaned up");
}

/* Add inference rule */
int katra_nous_reasoning_add_rule(const char* rule_name, const char* pattern) {
    if (!rule_name || !pattern) {
        return E_INPUT_NULL;
    }

    if (g_reasoning_state.rule_count >= g_reasoning_state.rule_capacity) {
        return E_SYSTEM_MEMORY;
    }

    inference_rule_t* rule = calloc(1, sizeof(inference_rule_t));
    if (!rule) {
        return E_SYSTEM_MEMORY;
    }

    int result = nous_safe_strdup(&rule->name, rule_name);
    if (result != KATRA_SUCCESS) {
        free(rule);
        return result;
    }

    result = nous_safe_strdup(&rule->pattern, pattern);
    if (result != KATRA_SUCCESS) {
        free(rule->name);
        free(rule);
        return result;
    }

    rule->confidence = 0.8f;  /* Default confidence */

    g_reasoning_state.rules[g_reasoning_state.rule_count++] = rule;

    LOG_DEBUG("Added inference rule: %s", rule_name);
    return KATRA_SUCCESS;
}

/* Build reasoning chain (simplified for Nous Reasoning) */
reasoning_chain_t* katra_nous_reasoning_build_chain(const char* goal) {
    if (!goal) {
        return NULL;
    }

    reasoning_chain_t* chain = calloc(1, sizeof(reasoning_chain_t));
    if (!chain) {
        return NULL;
    }

    /* Generate chain ID using common utility */
    char* id = nous_generate_id("chain", &g_reasoning_state.next_chain_id);
    if (!id) {
        free(chain);
        return NULL;
    }
    strncpy(chain->chain_id, id, sizeof(chain->chain_id) - 1);
    chain->chain_id[sizeof(chain->chain_id) - 1] = '\0';
    free(id);

    if (nous_safe_strdup(&chain->goal, goal) != KATRA_SUCCESS) {
        free(chain);
        return NULL;
    }

    /* Build simplified 3-step reasoning chain */
    chain->step_count = 3;
    chain->steps = calloc(chain->step_count, sizeof(inference_step_t));
    if (!chain->steps) {
        free(chain->goal);
        free(chain);
        return NULL;
    }

    /* Step 1: Observation */
    chain->steps[0].premise = strdup("Observed similar pattern in memory");
    chain->steps[0].conclusion = strdup("Pattern suggests solution direction");
    chain->steps[0].rule = strdup("similar_context");
    chain->steps[0].confidence = 0.7f;

    /* Step 2: Inference */
    chain->steps[1].premise = strdup("Pattern suggests solution direction");
    chain->steps[1].conclusion = strdup("Solution likely applicable here");
    chain->steps[1].rule = strdup("modus_ponens");
    chain->steps[1].confidence = 0.8f;

    /* Step 3: Conclusion */
    chain->steps[2].premise = strdup("Solution likely applicable here");
    chain->steps[2].conclusion = strdup(goal);
    chain->steps[2].rule = strdup("transitive");
    chain->steps[2].confidence = 0.75f;

    /* Calculate overall confidence (product of step confidences) */
    chain->overall_confidence = 1.0f;
    for (size_t i = 0; i < chain->step_count; i++) {
        chain->overall_confidence *= chain->steps[i].confidence;
    }

    /* Generate final conclusion */
    char conclusion[NOUS_MEDIUM_BUFFER];
    snprintf(conclusion, sizeof(conclusion),
            "Through %zu-step reasoning (confidence: %.0f%%), conclude: %s",
            chain->step_count, chain->overall_confidence * NOUS_PERCENT_MULTIPLIER, goal);

    chain->final_conclusion = strdup(conclusion);
    chain->valid = true;

    LOG_INFO("Built reasoning chain '%s' with confidence %.2f",
             chain->chain_id, chain->overall_confidence);

    return chain;
}

/* Find analogy (simplified for Nous Reasoning) */
analogy_t* katra_nous_reasoning_find_analogy(
    const char* source_domain,
    const char* target_domain
) {
    if (!source_domain || !target_domain) {
        return NULL;
    }

    analogy_t* analogy = calloc(1, sizeof(analogy_t));
    if (!analogy) {
        return NULL;
    }

    if (nous_safe_strdup(&analogy->source_domain, source_domain) != KATRA_SUCCESS) {
        free(analogy);
        return NULL;
    }

    if (nous_safe_strdup(&analogy->target_domain, target_domain) != KATRA_SUCCESS) {
        free(analogy->source_domain);
        free(analogy);
        return NULL;
    }

    /* Simplified: identify 2 similarities and 1 difference */
    analogy->similarity_count = 2;
    analogy->similarities = calloc(2, sizeof(char*));
    if (analogy->similarities) {
        analogy->similarities[0] = strdup("Both involve systematic approach");
        analogy->similarities[1] = strdup("Both require careful planning");
    }

    analogy->difference_count = 1;
    analogy->differences = calloc(1, sizeof(char*));
    if (analogy->differences) {
        analogy->differences[0] = strdup("Different scale and complexity");
    }

    /* Calculate analogy strength based on similarities vs differences */
    analogy->analogy_strength =
        (float)analogy->similarity_count /
        (float)(analogy->similarity_count + analogy->difference_count);

    /* Generate inference from analogy */
    char inference[NOUS_MEDIUM_BUFFER];
    snprintf(inference, sizeof(inference),
            "Since %s succeeded with approach X, and %s is similar "
            "(%.0f%% similarity), approach X may work for %s",
            source_domain, target_domain,
            analogy->analogy_strength * NOUS_PERCENT_MULTIPLIER, target_domain);

    analogy->inference = strdup(inference);
    analogy->inference_confidence = analogy->analogy_strength * 0.7f;

    LOG_INFO("Found analogy between '%s' and '%s' (strength: %.2f)",
             source_domain, target_domain, analogy->analogy_strength);

    return analogy;
}

/* Free reasoning chain */
void katra_nous_reasoning_free_chain(reasoning_chain_t* chain) {
    if (!chain) {
        return;
    }

    free(chain->goal);
    free(chain->final_conclusion);

    for (size_t i = 0; i < chain->step_count; i++) {
        free(chain->steps[i].premise);
        free(chain->steps[i].conclusion);
        free(chain->steps[i].rule);

        for (size_t j = 0; j < chain->steps[i].supporting_fact_count; j++) {
            free(chain->steps[i].supporting_facts[j]);
        }
        free(chain->steps[i].supporting_facts);
    }
    free(chain->steps);

    free(chain);
}

/* Free analogy */
void katra_nous_reasoning_free_analogy(analogy_t* analogy) {
    if (!analogy) {
        return;
    }

    free(analogy->source_domain);
    free(analogy->target_domain);
    free(analogy->inference);

    for (size_t i = 0; i < analogy->similarity_count; i++) {
        free(analogy->similarities[i]);
    }
    free(analogy->similarities);

    for (size_t i = 0; i < analogy->difference_count; i++) {
        free(analogy->differences[i]);
    }
    free(analogy->differences);

    free(analogy);
}
