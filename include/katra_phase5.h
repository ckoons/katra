/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_PHASE5_H
#define KATRA_PHASE5_H

#include <time.h>
#include <stdbool.h>
#include "katra_memory.h"

/* Phase 5: Memory-Augmented Reasoning
 *
 * Transforms memory from storage to intelligence.
 * Memory participates in reasoning, not just retrieval.
 *
 * Phase 5A: Basic Composition with Error Correction
 * - Semantic memory search
 * - Multi-source synthesis
 * - Multi-factor confidence
 * - Feedback mechanism
 * - Always include alternatives
 */

/* Query types */
typedef enum {
    QUERY_TYPE_PLACEMENT,      /* "Where should this function go?" */
    QUERY_TYPE_IMPACT,         /* "What breaks if I change this?" */
    QUERY_TYPE_USER_DOMAIN,    /* "Who would use this feature?" */
    QUERY_TYPE_GENERAL         /* General reasoning query */
} query_type_t;

/* Information source types */
typedef enum {
    SOURCE_MEMORY = (1 << 0),      /* Katra memory storage */
    SOURCE_CODE = (1 << 1),        /* Static code analysis */
    SOURCE_PATTERN = (1 << 2),     /* Learned conventions */
    SOURCE_REASONING = (1 << 3),   /* Logical inference */
    SOURCE_EXPERIENCE = (1 << 4)   /* Cross-project knowledge */
} source_type_t;

/* Recommendation outcome (for feedback) */
typedef enum {
    OUTCOME_ACCEPTED,    /* CI accepted recommendation */
    OUTCOME_REJECTED,    /* CI rejected recommendation */
    OUTCOME_MODIFIED     /* CI modified recommendation */
} outcome_t;

/* Multi-factor confidence breakdown */
typedef struct {
    float overall;               /* Combined confidence 0.0-1.0 */

    /* Individual factors */
    float source_agreement;      /* Do sources agree? 0.0-1.0 */
    float evidence_quality;      /* Quality of evidence 0.0-1.0 */
    float historical_accuracy;   /* Past accuracy for this query type 0.0-1.0 */
    float query_complexity;      /* Query complexity 0.0-1.0 (higher = more complex) */
    float temporal_recency;      /* Recency of sources 0.0-1.0 */

    /* Weights used in calculation */
    float weights[5];            /* How each factor was weighted */

    char* explanation;           /* Human-readable explanation */
} confidence_breakdown_t;

/* Alternative recommendation */
typedef struct {
    char* description;           /* What this alternative suggests */
    char* pros;                  /* Advantages */
    char* cons;                  /* Disadvantages */
    char* when_to_use;          /* Conditions favoring this option */
    float confidence;            /* Confidence in this alternative */
} alternative_t;

/* Source attribution for transparency */
typedef struct {
    source_type_t type;          /* Which source provided this */
    char* citation;              /* Reference (e.g., "katra_memory.c:123") */
    float contribution;          /* How much this source contributed 0.0-1.0 */
    time_t source_timestamp;     /* When source was created/modified */
} source_attribution_t;

/* Reasoning step for transparency */
typedef struct {
    source_type_t type;          /* Type of reasoning performed */
    char* description;           /* Human-readable step */
    source_attribution_t* sources; /* Citations */
    size_t source_count;
    float confidence;            /* Step confidence 0.0-1.0 */
    time_t source_timestamp;     /* When source was created */
} reasoning_step_t;

/* Composition result */
typedef struct {
    char* recommendation;        /* Primary answer */

    reasoning_step_t* reasoning; /* How we got here (citation trail) */
    size_t reasoning_count;

    alternative_t* alternatives; /* Other viable options (always >= 1) */
    size_t alternative_count;

    confidence_breakdown_t confidence; /* Multi-factor confidence */

    source_attribution_t* sources;     /* What informed this */
    size_t source_count;
} composition_result_t;

/* Composition query */
typedef struct {
    char* query_id;              /* Unique ID for feedback tracking */
    char* query_text;            /* "Where should this go?" */
    query_type_t type;           /* PLACEMENT, IMPACT, USER_DOMAIN */

    /* Configuration */
    int source_mask;             /* Which sources to use (bitmask) */
    size_t max_results;          /* Maximum results to return */
    size_t min_alternatives;     /* Minimum alternatives (default: 1) */
    float min_confidence;        /* Minimum confidence threshold */
    bool show_reasoning;         /* Include reasoning trace */
    bool show_alternatives;      /* Include alternatives (always true) */

    /* Result */
    composition_result_t* result; /* The answer */
} composition_query_t;

/* Phase 5 feedback for learning */
typedef struct {
    char* query_id;              /* Links to original query */
    char* recommended;           /* What Phase 5 suggested */
    outcome_t outcome;           /* ACCEPTED, REJECTED, MODIFIED */
    char* actual_choice;         /* What was actually done */
    char* explanation;           /* Why recommendation was wrong/modified */
    time_t timestamp;
    char* ci_id;                 /* Who provided feedback */
    query_type_t query_type;     /* Type of query for accuracy tracking */
} phase5_feedback_t;

/* Temporal validity tracking */
typedef struct {
    time_t valid_from;           /* When this became true */
    time_t superseded_at;        /* When it stopped being true (0 = current) */
    char* superseded_by;         /* ID of decision that replaced this */
    char* supersession_reason;   /* Why it was superseded */
} temporal_validity_t;

/* Phase 5A API Functions */

/* Initialize Phase 5 system */
int katra_phase5_init(const char* ci_id);

/* Cleanup Phase 5 system */
void katra_phase5_cleanup(void);

/* Create a composition query */
composition_query_t* katra_phase5_create_query(
    const char* query_text,
    query_type_t type
);

/* Execute composition query (main reasoning function) */
int katra_phase5_compose(composition_query_t* query);

/* Free query and results */
void katra_phase5_free_query(composition_query_t* query);

/* Submit feedback on a recommendation */
int katra_phase5_submit_feedback(phase5_feedback_t* feedback);

/* Get historical accuracy for a query type */
float katra_phase5_get_accuracy(query_type_t type);

/* Helper: Free confidence breakdown */
void katra_phase5_free_confidence(confidence_breakdown_t* conf);

/* Helper: Free composition result */
void katra_phase5_free_result(composition_result_t* result);

/* Helper: Free alternatives */
void katra_phase5_free_alternatives(alternative_t* alts, size_t count);

/* Helper: Free reasoning trace */
void katra_phase5_free_reasoning(reasoning_step_t* steps, size_t count);

/* Helper: Free source attributions */
void katra_phase5_free_sources(source_attribution_t* sources, size_t count);

#endif /* KATRA_PHASE5_H */
