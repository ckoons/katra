/* © 2025 Casey Koons All rights reserved */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "katra_nous.h"
#include "katra_error.h"
#include "katra_log.h"

/* Maximum patterns we'll track */
#define MAX_PATTERNS NOUS_MAX_PATTERNS

/* Pattern storage */
static struct {
    learned_pattern_t** patterns;    /* Array of patterns */
    size_t count;                    /* Number of patterns */
    size_t capacity;                 /* Allocated capacity */
    size_t next_id;                  /* For generating pattern IDs */
} g_pattern_store = {0};

/* Pattern type names for ID generation */
static const char* pattern_type_name(pattern_type_t type) {
    const char* type_names[] = {
        "naming", "org", "error", "memory", "api", "test", "doc"
    };
    return type < 7 ? type_names[type] : "unknown";
}

/* Helper: Calculate pattern confidence */
static float calculate_pattern_confidence(learned_pattern_t* pattern) {
    if (!pattern) {
        return 0.0f;
    }

    /* Factor 1: Consistency (examples vs exceptions) */
    size_t total = pattern->example_count + pattern->exception_count;
    pattern->consistency = total > 0 ?
        (float)pattern->example_count / (float)total : 0.0f;

    /* Factor 2: Usage count (more observations = higher confidence) */
    float usage_confidence = fminf(1.0f, pattern->usage_count / NOUS_USAGE_SATURATION);

    /* Factor 3: Recommendation accuracy */
    float rec_confidence = pattern->recommendation_accuracy;

    /* Factor 4: Age/stability (patterns seen longer are more trusted) */
    time_t now = time(NULL);
    float age_days = (float)(now - pattern->created) / (NOUS_HOURS_PER_DAY * NOUS_SECONDS_PER_HOUR);
    float age_confidence = fminf(1.0f, age_days / NOUS_DAYS_TO_TRUST);

    /* Combined confidence (weighted) */
    pattern->confidence =
        pattern->consistency * 0.40f +      /* Consistency is most important */
        usage_confidence * 0.25f +          /* Usage frequency */
        rec_confidence * 0.20f +            /* Recommendation accuracy */
        age_confidence * 0.15f;             /* Age/stability */

    return pattern->confidence;
}

/* Helper: Find pattern by ID */
static learned_pattern_t* find_pattern_by_id(const char* pattern_id) {
    if (!pattern_id) {
        return NULL;
    }

    for (size_t i = 0; i < g_pattern_store.count; i++) {
        if (strcmp(g_pattern_store.patterns[i]->pattern_id, pattern_id) == 0) {
            return g_pattern_store.patterns[i];
        }
    }

    return NULL;
}

/* Initialize pattern store (called by Phase 5 init) */
int katra_phase5b_init(void) {
    if (g_pattern_store.patterns) {
        /* Already initialized */
        return KATRA_SUCCESS;
    }

    g_pattern_store.capacity = MAX_PATTERNS;
    g_pattern_store.patterns = calloc(g_pattern_store.capacity,
                                     sizeof(learned_pattern_t*));
    if (!g_pattern_store.patterns) {
        return E_SYSTEM_MEMORY;
    }

    g_pattern_store.count = 0;
    g_pattern_store.next_id = 1;

    LOG_INFO("Phase 5B pattern learning initialized (capacity: %zu)",
             g_pattern_store.capacity);

    return KATRA_SUCCESS;
}

/* Cleanup pattern store (called by Phase 5 cleanup) */
void katra_phase5b_cleanup(void) {
    if (!g_pattern_store.patterns) {
        return;
    }

    for (size_t i = 0; i < g_pattern_store.count; i++) {
        katra_phase5b_free_pattern(g_pattern_store.patterns[i]);
    }

    free(g_pattern_store.patterns);
    g_pattern_store.patterns = NULL;
    g_pattern_store.count = 0;
    g_pattern_store.capacity = 0;

    LOG_INFO("Phase 5B pattern learning cleaned up");
}

/* Learn a new pattern */
int katra_phase5b_learn_pattern(
    pattern_type_t type,
    const char* name,
    const char* description,
    const char* rationale
) {
    if (!name || !description) {
        return E_INPUT_NULL;
    }

    if (g_pattern_store.count >= g_pattern_store.capacity) {
        LOG_ERROR("Pattern store full (%zu patterns)", g_pattern_store.count);
        return E_SYSTEM_MEMORY;
    }

    /* Create new pattern */
    learned_pattern_t* pattern = calloc(1, sizeof(learned_pattern_t));
    if (!pattern) {
        return E_SYSTEM_MEMORY;
    }

    /* Generate ID using common utility */
    char prefix[NOUS_SMALL_BUFFER];
    snprintf(prefix, sizeof(prefix), "pattern_%s", pattern_type_name(type));
    char* id = nous_generate_id(prefix, &g_pattern_store.next_id);
    if (!id) {
        katra_phase5b_free_pattern(pattern);
        return E_SYSTEM_MEMORY;
    }
    strncpy(pattern->pattern_id, id, sizeof(pattern->pattern_id) - 1);
    pattern->pattern_id[sizeof(pattern->pattern_id) - 1] = '\0';
    free(id);

    /* Set fields */
    pattern->type = type;

    int result = nous_safe_strdup(&pattern->name, name);
    if (result != KATRA_SUCCESS) {
        katra_phase5b_free_pattern(pattern);
        return result;
    }

    result = nous_safe_strdup(&pattern->description, description);
    if (result != KATRA_SUCCESS) {
        katra_phase5b_free_pattern(pattern);
        return result;
    }

    result = nous_safe_strdup(&pattern->rationale, rationale);
    if (result != KATRA_SUCCESS) {
        katra_phase5b_free_pattern(pattern);
        return result;
    }

    /* Initialize metrics */
    time_t now = time(NULL);
    pattern->created = now;
    pattern->last_seen = now;
    pattern->last_updated = now;
    pattern->version = 1;
    pattern->usage_count = 0;
    pattern->confidence = 0.5f;  /* Start with medium confidence */
    pattern->consistency = 1.0f;  /* No exceptions yet */
    pattern->recommendation_accuracy = 0.5f;  /* Unknown */

    /* Initialize arrays */
    pattern->examples = NULL;
    pattern->example_count = 0;
    pattern->exceptions = NULL;
    pattern->exception_count = 0;
    pattern->recommended_count = 0;
    pattern->accepted_count = 0;

    /* Add to store */
    g_pattern_store.patterns[g_pattern_store.count++] = pattern;

    LOG_INFO("Learned new pattern: %s (%s)", pattern->pattern_id, name);

    return KATRA_SUCCESS;
}

/* Add example to pattern */
int katra_phase5b_add_example(
    const char* pattern_id,
    const char* location,
    const char* code_snippet
) {
    if (!pattern_id || !location || !code_snippet) {
        return E_INPUT_NULL;
    }

    learned_pattern_t* pattern = find_pattern_by_id(pattern_id);
    if (!pattern) {
        LOG_ERROR("Pattern not found: %s", pattern_id);
        return E_INPUT_INVALID;
    }

    /* Allocate new example */
    pattern_example_t* new_examples = realloc(pattern->examples,
        (pattern->example_count + 1) * sizeof(pattern_example_t));
    if (!new_examples) {
        return E_SYSTEM_MEMORY;
    }

    pattern->examples = new_examples;
    pattern_example_t* example = &pattern->examples[pattern->example_count];

    int result = nous_safe_strdup(&example->location, location);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    result = nous_safe_strdup(&example->code_snippet, code_snippet);
    if (result != KATRA_SUCCESS) {
        free(example->location);
        example->location = NULL;
        return result;
    }

    example->discovered = time(NULL);

    pattern->example_count++;
    pattern->usage_count++;
    pattern->last_seen = time(NULL);
    pattern->last_updated = time(NULL);
    pattern->version++;

    /* Recalculate confidence */
    calculate_pattern_confidence(pattern);

    LOG_DEBUG("Added example to pattern %s (total: %zu)",
              pattern_id, pattern->example_count);

    return KATRA_SUCCESS;
}

/* Add exception to pattern */
int katra_phase5b_add_exception(
    const char* pattern_id,
    const char* location,
    const char* code_snippet,
    const char* reason,
    bool justified
) {
    if (!pattern_id || !location || !code_snippet) {
        return E_INPUT_NULL;
    }

    learned_pattern_t* pattern = find_pattern_by_id(pattern_id);
    if (!pattern) {
        LOG_ERROR("Pattern not found: %s", pattern_id);
        return E_INPUT_INVALID;
    }

    /* Allocate new exception */
    pattern_exception_t* new_exceptions = realloc(pattern->exceptions,
        (pattern->exception_count + 1) * sizeof(pattern_exception_t));
    if (!new_exceptions) {
        return E_SYSTEM_MEMORY;
    }

    pattern->exceptions = new_exceptions;
    pattern_exception_t* exception = &pattern->exceptions[pattern->exception_count];

    int result = nous_safe_strdup(&exception->location, location);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    result = nous_safe_strdup(&exception->code_snippet, code_snippet);
    if (result != KATRA_SUCCESS) {
        free(exception->location);
        exception->location = NULL;
        return result;
    }

    result = nous_safe_strdup(&exception->reason, reason);
    if (result != KATRA_SUCCESS) {
        free(exception->location);
        free(exception->code_snippet);
        exception->location = NULL;
        exception->code_snippet = NULL;
        return result;
    }

    exception->discovered = time(NULL);
    exception->justified = justified;

    pattern->exception_count++;
    pattern->last_seen = time(NULL);
    pattern->last_updated = time(NULL);
    pattern->version++;

    /* Recalculate confidence */
    calculate_pattern_confidence(pattern);

    LOG_DEBUG("Added exception to pattern %s (total: %zu, justified: %s)",
              pattern_id, pattern->exception_count, justified ? "yes" : "no");

    return KATRA_SUCCESS;
}

/* Query patterns */
learned_pattern_t** katra_phase5b_query_patterns(
    pattern_query_t* query,
    size_t* result_count
) {
    if (!query || !result_count) {
        return NULL;
    }

    *result_count = 0;

    /* Count matching patterns */
    size_t match_count = 0;
    for (size_t i = 0; i < g_pattern_store.count; i++) {
        learned_pattern_t* p = g_pattern_store.patterns[i];

        /* Filter by type */
        if (p->type != query->type) {
            continue;
        }

        /* Filter by confidence */
        if (p->confidence < query->min_confidence) {
            continue;
        }

        /* Filter by keyword (if provided) */
        if (query->keyword) {
            if (!strstr(p->name, query->keyword) &&
                !strstr(p->description, query->keyword)) {
                continue;
            }
        }

        match_count++;
        if (query->max_results > 0 && match_count >= query->max_results) {
            break;
        }
    }

    if (match_count == 0) {
        return NULL;
    }

    /* Allocate result array */
    learned_pattern_t** results = calloc(match_count, sizeof(learned_pattern_t*));
    if (!results) {
        return NULL;
    }

    /* Populate results */
    size_t result_idx = 0;
    for (size_t i = 0; i < g_pattern_store.count && result_idx < match_count; i++) {
        learned_pattern_t* p = g_pattern_store.patterns[i];

        /* Apply same filters */
        if (p->type != query->type) {
            continue;
        }

        if (p->confidence < query->min_confidence) {
            continue;
        }

        if (query->keyword) {
            if (!strstr(p->name, query->keyword) &&
                !strstr(p->description, query->keyword)) {
                continue;
            }
        }

        results[result_idx++] = p;
    }

    *result_count = match_count;
    return results;
}

/* Get specific pattern by ID */
learned_pattern_t* katra_phase5b_get_pattern(const char* pattern_id) {
    return find_pattern_by_id(pattern_id);
}

/* Update pattern confidence */
int katra_phase5b_update_confidence(const char* pattern_id) {
    if (!pattern_id) {
        return E_INPUT_NULL;
    }

    learned_pattern_t* pattern = find_pattern_by_id(pattern_id);
    if (!pattern) {
        return E_INPUT_INVALID;
    }

    calculate_pattern_confidence(pattern);
    pattern->last_updated = time(NULL);

    return KATRA_SUCCESS;
}

/* Record pattern recommendation outcome */
int katra_phase5b_record_outcome(const char* pattern_id, bool accepted) {
    if (!pattern_id) {
        return E_INPUT_NULL;
    }

    learned_pattern_t* pattern = find_pattern_by_id(pattern_id);
    if (!pattern) {
        return E_INPUT_INVALID;
    }

    pattern->recommended_count++;
    if (accepted) {
        pattern->accepted_count++;
    }

    /* Recalculate recommendation accuracy */
    if (pattern->recommended_count > 0) {
        pattern->recommendation_accuracy =
            (float)pattern->accepted_count / (float)pattern->recommended_count;
    }

    /* Update confidence */
    calculate_pattern_confidence(pattern);

    LOG_DEBUG("Pattern %s: %zu/%zu accepted (%.1f%%)",
              pattern_id, pattern->accepted_count, pattern->recommended_count,
              pattern->recommendation_accuracy * NOUS_PERCENT_MULTIPLIER);

    return KATRA_SUCCESS;
}

/* Get patterns by type */
learned_pattern_t** katra_phase5b_get_patterns_by_type(
    pattern_type_t type,
    size_t* result_count
) {
    if (!result_count) {
        return NULL;
    }

    pattern_query_t query = {
        .type = type,
        .keyword = NULL,
        .min_confidence = 0.0f,
        .max_results = 0  /* No limit */
    };

    return katra_phase5b_query_patterns(&query, result_count);
}

/* Free a single pattern */
void katra_phase5b_free_pattern(learned_pattern_t* pattern) {
    if (!pattern) {
        return;
    }

    free(pattern->name);
    free(pattern->description);
    free(pattern->rationale);

    /* Free examples */
    for (size_t i = 0; i < pattern->example_count; i++) {
        free(pattern->examples[i].location);
        free(pattern->examples[i].code_snippet);
    }
    free(pattern->examples);

    /* Free exceptions */
    for (size_t i = 0; i < pattern->exception_count; i++) {
        free(pattern->exceptions[i].location);
        free(pattern->exceptions[i].code_snippet);
        free(pattern->exceptions[i].reason);
    }
    free(pattern->exceptions);

    free(pattern);
}

/* Free array of patterns */
void katra_phase5b_free_patterns(learned_pattern_t** patterns, size_t count) {
    (void)count;  /* Unused - provided for API consistency */

    if (!patterns) {
        return;
    }

    /* Note: Don't free individual patterns - they're owned by the store */
    free(patterns);
}
