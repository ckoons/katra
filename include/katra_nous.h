/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_NOUS_H
#define KATRA_NOUS_H

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

/* Phase 5 Constants */

/* Buffer sizes */
#define NOUS_QUERY_ID_SIZE 64
#define NOUS_SMALL_BUFFER 256
#define NOUS_MEDIUM_BUFFER 512
#define NOUS_LARGE_BUFFER 1024
#define NOUS_PATTERN_ID_SIZE 64
#define NOUS_CHAIN_ID_SIZE 64
#define NOUS_CHANGE_ID_SIZE 64
#define NOUS_PRACTICE_ID_SIZE 64

/* Capacity limits */
#define NOUS_MAX_PATTERNS 256
#define NOUS_MAX_REASONING_CHAINS 128
#define NOUS_MAX_INFERENCE_RULES 64
#define NOUS_MAX_DEPENDENCIES 1024
#define NOUS_MAX_CHANGE_RECORDS 256
#define NOUS_MAX_PRACTICES 256
#define NOUS_MAX_ANTIPATTERNS 128

/* Confidence calculation constants */
#define NOUS_USAGE_SATURATION 10.0f        /* Observations needed for full confidence */
#define NOUS_IMPACT_SCALE 20.0f             /* Scale factor for impact calculation */
#define NOUS_DEPENDENCY_SCALE 10.0f         /* Scale factor for dependency risk */

/* Time constants */
#define NOUS_HOURS_PER_DAY 24.0f
#define NOUS_SECONDS_PER_HOUR 3600.0f
#define NOUS_DAYS_TO_TRUST 30.0f            /* Days until pattern fully trusted */
#define NOUS_DECAY_HALFLIFE 90.0f           /* Temporal decay half-life in days */

/* Display constants */
#define NOUS_PERCENT_MULTIPLIER 100.0f

/* Phase 5 Common Utilities API */

/* Generate unique ID with prefix (caller must free) */
char* nous_generate_id(const char* prefix, size_t* counter);

/* Calculate weighted confidence from factors */
typedef struct {
    float factors[5];      /* Up to 5 confidence factors */
    float weights[5];      /* Weights for each factor */
    size_t factor_count;   /* Number of factors used */
} nous_confidence_calc_t;

float nous_calculate_confidence(const nous_confidence_calc_t* calc);

/* Safe strdup with NULL check (returns error code) */
int nous_safe_strdup(char** dest, const char* src);

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
} nous_feedback_t;

/* Temporal validity tracking */
typedef struct {
    time_t valid_from;           /* When this became true */
    time_t superseded_at;        /* When it stopped being true (0 = current) */
    char* superseded_by;         /* ID of decision that replaced this */
    char* supersession_reason;   /* Why it was superseded */
} temporal_validity_t;

/* Phase 5B: Pattern Learning
 *
 * Automatic extraction and tracking of coding patterns.
 * Patterns inform recommendations and evolve over time.
 */

/* Pattern types */
typedef enum {
    PATTERN_NAMING,              /* Naming conventions (functions, variables) */
    PATTERN_ORGANIZATION,        /* Code organization (file structure, grouping) */
    PATTERN_ERROR_HANDLING,      /* Error handling strategies */
    PATTERN_MEMORY,              /* Memory management patterns */
    PATTERN_API_DESIGN,          /* API design patterns */
    PATTERN_TESTING,             /* Testing conventions */
    PATTERN_DOCUMENTATION        /* Documentation patterns */
} pattern_type_t;

/* Pattern example (code that follows the pattern) */
typedef struct {
    char* location;              /* File:line reference */
    char* code_snippet;          /* Actual code example */
    time_t discovered;           /* When example was found */
} pattern_example_t;

/* Pattern exception (code that violates the pattern) */
typedef struct {
    char* location;              /* File:line reference */
    char* code_snippet;          /* Violating code */
    char* reason;                /* Why it's an exception */
    time_t discovered;           /* When exception was found */
    bool justified;              /* Is this a justified exception? */
} pattern_exception_t;

/* Learned pattern */
typedef struct {
    char pattern_id[NOUS_PATTERN_ID_SIZE];         /* Unique pattern identifier */
    pattern_type_t type;         /* Type of pattern */
    char* name;                  /* Human-readable name */
    char* description;           /* What this pattern represents */
    char* rationale;             /* Why this pattern exists */

    /* Evidence */
    pattern_example_t* examples; /* Examples following pattern */
    size_t example_count;
    pattern_exception_t* exceptions; /* Exceptions to pattern */
    size_t exception_count;

    /* Confidence metrics */
    float confidence;            /* Overall pattern confidence 0.0-1.0 */
    float consistency;           /* examples / (examples + exceptions) */
    size_t usage_count;          /* How often pattern is observed */

    /* Evolution tracking */
    time_t created;              /* When pattern was first learned */
    time_t last_seen;            /* Last time pattern was observed */
    time_t last_updated;         /* Last time pattern was modified */
    size_t version;              /* Pattern version (increments on change) */

    /* Recommendation tracking */
    size_t recommended_count;    /* Times pattern was recommended */
    size_t accepted_count;       /* Times recommendation was accepted */
    float recommendation_accuracy; /* accepted / recommended */
} learned_pattern_t;

/* Pattern query (for searching patterns) */
typedef struct {
    pattern_type_t type;         /* Type of pattern to find */
    char* keyword;               /* Keyword search (optional) */
    float min_confidence;        /* Minimum confidence threshold */
    size_t max_results;          /* Maximum results to return */
} pattern_query_t;

/* Phase 5A API Functions */

/* Initialize Phase 5 system */
int katra_nous_init(const char* ci_id);

/* Cleanup Phase 5 system */
void katra_nous_cleanup(void);

/* Create a composition query */
composition_query_t* katra_nous_create_query(
    const char* query_text,
    query_type_t type
);

/* Execute composition query (main reasoning function) */
int katra_nous_compose(composition_query_t* query);

/* Free query and results */
void katra_nous_free_query(composition_query_t* query);

/* Submit feedback on a recommendation */
int katra_nous_submit_feedback(nous_feedback_t* feedback);

/* Get historical accuracy for a query type */
float katra_nous_get_accuracy(query_type_t type);

/* Helper: Free confidence breakdown */
void katra_nous_free_confidence(confidence_breakdown_t* conf);

/* Helper: Free composition result */
void katra_nous_free_result(composition_result_t* result);

/* Helper: Free alternatives */
void katra_nous_free_alternatives(alternative_t* alts, size_t count);

/* Helper: Free reasoning trace */
void katra_nous_free_reasoning(reasoning_step_t* steps, size_t count);

/* Helper: Free source attributions */
void katra_nous_free_sources(source_attribution_t* sources, size_t count);

/* Phase 5C: Impact Analysis
 *
 * Analyzes dependencies and predicts change impact.
 * Learns from historical changes to improve predictions.
 */

/* Dependency types */
typedef enum {
    DEP_FUNCTION_CALL,           /* Function calls another function */
    DEP_DATA_ACCESS,             /* Accesses data structure/variable */
    DEP_INCLUDE,                 /* File includes another file */
    DEP_SYMBOL_REFERENCE,        /* References symbol from another module */
    DEP_INDIRECT                 /* Indirect dependency (transitive) */
} dependency_type_t;

/* Impact severity */
typedef enum {
    IMPACT_NONE,                 /* No impact */
    IMPACT_LOW,                  /* Low impact (isolated change) */
    IMPACT_MEDIUM,               /* Medium impact (local effects) */
    IMPACT_HIGH,                 /* High impact (wide-reaching) */
    IMPACT_CRITICAL              /* Critical impact (system-wide) */
} impact_severity_t;

/* Dependency relationship */
typedef struct {
    char* source;                /* Source item (function, file, etc.) */
    char* target;                /* Target item being depended on */
    dependency_type_t type;      /* Type of dependency */
    float strength;              /* Dependency strength 0.0-1.0 */
    time_t discovered;           /* When dependency was discovered */
} dependency_t;

/* Change impact prediction */
typedef struct {
    char* change_target;         /* What's being changed */
    impact_severity_t severity;  /* Predicted impact severity */
    float confidence;            /* Confidence in prediction 0.0-1.0 */

    /* Affected items */
    char** affected_functions;   /* Functions that may be affected */
    size_t affected_function_count;
    char** affected_files;       /* Files that may be affected */
    size_t affected_file_count;

    /* Risk assessment */
    float risk_score;            /* Overall risk 0.0-1.0 */
    char* risk_explanation;      /* Human-readable risk explanation */

    /* Historical data */
    size_t similar_changes;      /* Similar past changes */
    float historical_success;    /* Success rate of similar changes */
} impact_prediction_t;

/* Historical change record */
typedef struct {
    char change_id[NOUS_CHANGE_ID_SIZE];          /* Unique change identifier */
    char* description;           /* What was changed */
    time_t timestamp;            /* When change occurred */

    /* Impact */
    size_t files_changed;        /* Number of files changed */
    size_t functions_affected;   /* Number of functions affected */
    bool caused_issues;          /* Did it cause problems? */
    char* issues_description;    /* Description of issues if any */

    /* Success metrics */
    bool successful;             /* Was change successful? */
    float actual_impact;         /* Actual impact 0.0-1.0 */
} change_record_t;

/* Phase 5B API Functions */

/* Initialize Phase 5B pattern learning (called by Phase 5 init) */
int katra_phase5b_init(void);

/* Cleanup Phase 5B pattern learning (called by Phase 5 cleanup) */
void katra_phase5b_cleanup(void);

/* Learn a pattern from code observation */
int katra_phase5b_learn_pattern(
    pattern_type_t type,
    const char* name,
    const char* description,
    const char* rationale
);

/* Add example to existing pattern */
int katra_phase5b_add_example(
    const char* pattern_id,
    const char* location,
    const char* code_snippet
);

/* Add exception to existing pattern */
int katra_phase5b_add_exception(
    const char* pattern_id,
    const char* location,
    const char* code_snippet,
    const char* reason,
    bool justified
);

/* Query patterns */
learned_pattern_t** katra_phase5b_query_patterns(
    pattern_query_t* query,
    size_t* result_count
);

/* Get specific pattern by ID */
learned_pattern_t* katra_phase5b_get_pattern(const char* pattern_id);

/* Update pattern confidence based on observation */
int katra_phase5b_update_confidence(const char* pattern_id);

/* Record pattern recommendation outcome */
int katra_phase5b_record_outcome(const char* pattern_id, bool accepted);

/* Get all patterns of a specific type */
learned_pattern_t** katra_phase5b_get_patterns_by_type(
    pattern_type_t type,
    size_t* result_count
);

/* Free a single pattern */
void katra_phase5b_free_pattern(learned_pattern_t* pattern);

/* Free array of patterns */
void katra_phase5b_free_patterns(learned_pattern_t** patterns, size_t count);

/* Phase 5D: Advanced Reasoning
 *
 * Multi-step inference chains and analogical reasoning.
 * Builds complex conclusions from simple facts.
 */

/* Inference step in reasoning chain */
typedef struct {
    char* premise;               /* Starting premise */
    char* conclusion;            /* What we conclude */
    char* rule;                  /* Inference rule applied */
    float confidence;            /* Confidence in this step 0.0-1.0 */
    char** supporting_facts;     /* Facts supporting this inference */
    size_t supporting_fact_count;
} inference_step_t;

/* Reasoning chain (multi-step inference) */
typedef struct {
    char chain_id[NOUS_CHAIN_ID_SIZE];           /* Unique chain identifier */
    char* goal;                  /* What we're trying to conclude */

    inference_step_t* steps;     /* Chain of inference steps */
    size_t step_count;

    char* final_conclusion;      /* Final conclusion */
    float overall_confidence;    /* Combined confidence 0.0-1.0 */

    bool valid;                  /* Is chain logically valid? */
} reasoning_chain_t;

/* Analogy between two situations */
typedef struct {
    char* source_domain;         /* Source situation */
    char* target_domain;         /* Target situation */

    char** similarities;         /* What's similar */
    size_t similarity_count;
    char** differences;          /* What's different */
    size_t difference_count;

    float analogy_strength;      /* How strong is analogy 0.0-1.0 */
    char* inference;             /* What we can infer from analogy */
    float inference_confidence;  /* Confidence in inference 0.0-1.0 */
} analogy_t;

/* Phase 5E: Cross-Project Learning
 *
 * Learns patterns and best practices from multiple projects.
 * Builds knowledge base that improves recommendations.
 */

/* Best practice record */
typedef struct {
    char practice_id[NOUS_PRACTICE_ID_SIZE];        /* Unique identifier */
    char* name;                  /* Practice name */
    char* description;           /* What this practice is */
    char* rationale;             /* Why it's a best practice */
    char* category;              /* Category (naming, testing, etc.) */

    /* Evidence */
    char** example_projects;     /* Projects using this */
    size_t example_count;

    float adoption_rate;         /* How widely adopted 0.0-1.0 */
    float effectiveness;         /* How effective 0.0-1.0 */
    bool recommended;            /* Do we recommend this? */
} best_practice_t;

/* Anti-pattern record */
typedef struct {
    char antipattern_id[NOUS_PRACTICE_ID_SIZE];     /* Unique identifier */
    char* name;                  /* Anti-pattern name */
    char* description;           /* What to avoid */
    char* why_bad;               /* Why it's problematic */

    char** common_consequences;  /* Common negative outcomes */
    size_t consequence_count;

    char* better_alternative;    /* What to do instead */
} antipattern_t;

/* Cross-project knowledge entry */
typedef struct {
    char* project_name;          /* Source project */
    char* domain;                /* Project domain (e.g., "systems", "web") */

    learned_pattern_t** patterns; /* Patterns from this project */
    size_t pattern_count;

    best_practice_t** practices; /* Best practices observed */
    size_t practice_count;

    float quality_score;         /* Project quality assessment 0.0-1.0 */
    bool publicly_shareable;     /* Can we share this knowledge? */
} project_knowledge_t;

/* Phase 5C API Functions */

/* Initialize Phase 5C impact analysis (called by Phase 5 init) */
int katra_phase5c_init(void);

/* Cleanup Phase 5C impact analysis (called by Phase 5 cleanup) */
void katra_phase5c_cleanup(void);

/* Record a dependency relationship */
int katra_phase5c_add_dependency(
    const char* source,
    const char* target,
    dependency_type_t type,
    float strength
);

/* Predict impact of a change */
impact_prediction_t* katra_phase5c_predict_impact(const char* change_target);

/* Record a completed change for learning */
int katra_phase5c_record_change(
    const char* description,
    size_t files_changed,
    size_t functions_affected,
    bool successful,
    const char* issues
);

/* Get dependencies for an item */
dependency_t** katra_phase5c_get_dependencies(
    const char* target,
    size_t* count
);

/* Free impact prediction */
void katra_phase5c_free_prediction(impact_prediction_t* prediction);

/* Free dependencies */
void katra_phase5c_free_dependencies(dependency_t** deps, size_t count);

/* Phase 5D API Functions */

/* Initialize Phase 5D advanced reasoning (called by Phase 5 init) */
int katra_phase5d_init(void);

/* Cleanup Phase 5D advanced reasoning (called by Phase 5 cleanup) */
void katra_phase5d_cleanup(void);

/* Build reasoning chain to reach goal */
reasoning_chain_t* katra_phase5d_build_chain(const char* goal);

/* Find analogies between situations */
analogy_t* katra_phase5d_find_analogy(
    const char* source_domain,
    const char* target_domain
);

/* Add inference rule */
int katra_phase5d_add_rule(
    const char* rule_name,
    const char* pattern
);

/* Free reasoning chain */
void katra_phase5d_free_chain(reasoning_chain_t* chain);

/* Free analogy */
void katra_phase5d_free_analogy(analogy_t* analogy);

/* Phase 5E API Functions */

/* Initialize Phase 5E cross-project learning (called by Phase 5 init) */
int katra_phase5e_init(void);

/* Cleanup Phase 5E cross-project learning (called by Phase 5 cleanup) */
void katra_phase5e_cleanup(void);

/* Add best practice */
int katra_phase5e_add_practice(
    const char* name,
    const char* description,
    const char* rationale,
    const char* category
);

/* Add anti-pattern */
int katra_phase5e_add_antipattern(
    const char* name,
    const char* description,
    const char* why_bad,
    const char* alternative
);

/* Get best practices by category */
best_practice_t** katra_phase5e_get_practices(
    const char* category,
    size_t* count
);

/* Get anti-patterns */
antipattern_t** katra_phase5e_get_antipatterns(size_t* count);

/* Import project knowledge */
int katra_phase5e_import_project(
    const char* project_name,
    const char* domain,
    float quality_score
);

/* Free best practice */
void katra_phase5e_free_practice(best_practice_t* practice);

/* Free practices array */
void katra_phase5e_free_practices(best_practice_t** practices, size_t count);

/* Free anti-pattern */
void katra_phase5e_free_antipattern(antipattern_t* antipattern);

/* Free anti-patterns array */
void katra_phase5e_free_antipatterns(antipattern_t** antipatterns, size_t count);

#endif /* KATRA_NOUS_H */
