/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_cognitive.h"
#include "katra_memory.h"
#include "katra_encoder.h"
#include "katra_psyche_common.h"
#include "katra_core_common.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Thought type names for logging */
static const char* thought_type_names[] = {
    "IDEA", "MEMORY", "FACT", "OPINION", "QUESTION",
    "ANSWER", "PLAN", "REFLECTION", "FEELING",
    "OBSERVATION", "UNKNOWN"
};

/* Get thought type name */
const char* katra_thought_type_name(thought_type_t type) {
    if (type < 0 || type > THOUGHT_TYPE_UNKNOWN) {
        return "INVALID";
    }
    return thought_type_names[type];
}

/* Detect thought type from content */
thought_type_t katra_detect_thought_type(const char* content) {
    if (!content || strlen(content) == 0) {
        return THOUGHT_TYPE_UNKNOWN;
    }

    size_t len = strlen(content);

    /* Questions - end with '?' */
    if (content[len - 1] == '?') {
        return THOUGHT_TYPE_QUESTION;
    }

    /* Reflections - meta-cognitive phrases */
    const char* reflection_keywords[] = {
        "i think", "i realize", "i wonder", "i notice", "i believe",
        "it seems", "i feel like", "i understand", "i learned"
    };
    if (katra_str_contains_any(content, reflection_keywords, 9)) {
        return THOUGHT_TYPE_REFLECTION;
    }

    /* Plans - future tense and intentions */
    const char* plan_keywords[] = {
        "will ", "going to", "should ", "plan to", "intend to",
        "tomorrow", "next ", "later "
    };
    if (katra_str_contains_any(content, plan_keywords, 8)) {
        return THOUGHT_TYPE_PLAN;
    }

    /* Feelings - emotion words */
    const char* feeling_keywords[] = {
        "i feel", "i'm happy", "i'm sad", "i'm angry", "i'm excited",
        "i'm frustrated", "i'm worried", "i'm glad", "i'm disappointed"
    };
    if (katra_str_contains_any(content, feeling_keywords, 9)) {
        return THOUGHT_TYPE_FEELING;
    }

    /* Ideas - creative language */
    const char* idea_keywords[] = {
        "what if", "maybe we could", "i have an idea", "i thought of",
        "we could", "it might be"
    };
    if (katra_str_contains_any(content, idea_keywords, 6)) {
        return THOUGHT_TYPE_IDEA;
    }

    /* Opinions - subjective language */
    const char* opinion_keywords[] = {
        "i prefer", "i like", "i don't like", "in my opinion",
        "i'd rather", "better than", "worse than"
    };
    if (katra_str_contains_any(content, opinion_keywords, 7)) {
        return THOUGHT_TYPE_OPINION;
    }

    /* Observations - noticing patterns */
    const char* observation_keywords[] = {
        "i see", "i notice", "i observe", "i found", "i discovered",
        "it appears", "looks like"
    };
    if (katra_str_contains_any(content, observation_keywords, 7)) {
        return THOUGHT_TYPE_OBSERVATION;
    }

    /* Facts - definitive statements (harder to detect, default if no hedging) */
    const char* hedge_keywords[] = {
        "maybe", "perhaps", "might", "could be", "possibly",
        "probably", "i think"
    };
    if (!katra_str_contains_any(content, hedge_keywords, HEDGE_KEYWORD_COUNT) && len > MIN_HEDGE_DETECTION_LENGTH) {
        return THOUGHT_TYPE_FACT;
    }

    return THOUGHT_TYPE_UNKNOWN;
}

/* Calculate confidence score */
float katra_calculate_confidence(const char* content, thought_type_t thought_type) {
    if (!content || strlen(content) == 0) {
        return 0.0f;
    }

    /* Base confidence by thought type */
    float confidence = 0.5f;
    switch (thought_type) {
        case THOUGHT_TYPE_FACT: confidence = 0.8f; break;
        case THOUGHT_TYPE_QUESTION: confidence = 0.3f; break;
        case THOUGHT_TYPE_OPINION: confidence = 0.6f; break;
        case THOUGHT_TYPE_REFLECTION: confidence = 0.5f; break;
        case THOUGHT_TYPE_PLAN: confidence = 0.7f; break;
        case THOUGHT_TYPE_IDEA: confidence = 0.5f; break;
        case THOUGHT_TYPE_OBSERVATION: confidence = 0.7f; break;
        case THOUGHT_TYPE_FEELING: confidence = 0.9f; break;
        default: confidence = 0.5f;
    }

    /* Hedging words reduce confidence */
    const char* hedge_words[] = {
        "maybe", "perhaps", "might", "could be", "possibly",
        "probably", "i guess", "i'm not sure"
    };
    if (katra_str_contains_any(content, hedge_words, 8)) {
        confidence *= 0.7f;
    }

    /* Definitive language increases confidence */
    const char* definitive_words[] = {
        "definitely", "certainly", "absolutely", "clearly",
        "obviously", "without doubt"
    };
    if (katra_str_contains_any(content, definitive_words, 6)) {
        confidence *= 1.2f;
        if (confidence > 1.0f) confidence = 1.0f;
    }

    /* Multiple exclamation marks reduce confidence (emotional, not factual) */
    size_t exclaim_count = katra_str_count_char(content, '!');
    if (exclaim_count > 1 && thought_type != THOUGHT_TYPE_FEELING) {
        confidence *= 0.8f;
    }

    /* Clamp to 0.0-1.0 */
    if (confidence < 0.0f) confidence = 0.0f;
    if (confidence > 1.0f) confidence = 1.0f;

    return confidence;
}

/* Store a thought (auto-detect type and confidence) */
int katra_store_thought(const char* ci_id, const char* content,
                        float importance, const char* context) {
    ENGRAM_CHECK_PARAMS_2(ci_id, content);

    /* Detect thought type and confidence */
    thought_type_t thought_type = katra_detect_thought_type(content);
    float confidence = katra_calculate_confidence(content, thought_type);

    LOG_DEBUG("Detected thought type: %s, confidence: %.2f",
             katra_thought_type_name(thought_type), confidence);

    return katra_store_thought_typed(ci_id, content, thought_type,
                                     confidence, importance, context);
}

/* Store a thought with explicit type and confidence */
int katra_store_thought_typed(const char* ci_id, const char* content,
                              thought_type_t thought_type, float confidence,
                              float importance, const char* context) {
    ENGRAM_CHECK_PARAMS_2(ci_id, content);

    /* Create base memory record */
    memory_record_t* record = katra_memory_create_record(
        ci_id, MEMORY_TYPE_EXPERIENCE, content, importance);

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          "Failed to create memory record");
        return E_SYSTEM_MEMORY;
    }

    /* Add context if provided */
    if (context) {
        record->context = katra_safe_strdup(context);
        if (context && !record->context) {
            katra_memory_free_record(record);
            return E_SYSTEM_MEMORY;
        }
    }

    /* Store via memory system */
    int result = katra_memory_store(record);

    if (result == KATRA_SUCCESS) {
        LOG_INFO("Stored thought: type=%s, confidence=%.2f, record_id=%s",
                katra_thought_type_name(thought_type), confidence,
                record->record_id);
    }

    katra_memory_free_record(record);
    return result;
}

/* Recall experiences */
int katra_recall_experience(const char* ci_id, const char* query_text,
                            float min_confidence, size_t limit,
                            cognitive_record_t*** results, size_t* count) {
    ENGRAM_CHECK_PARAMS_3(ci_id, results, count);

    (void)query_text;  /* Not yet used - for semantic search in future */

    /* Query memory records */
    memory_query_t query = {
        .ci_id = ci_id, .start_time = 0, .end_time = 0,
        .type = MEMORY_TYPE_EXPERIENCE, .min_importance = 0.0f,
        .tier = KATRA_TIER1, .limit = limit
    };

    memory_record_t** base_results = NULL;
    size_t base_count = 0;

    int result = katra_memory_query(&query, &base_results, &base_count);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Convert to cognitive records */
    cognitive_record_t** cog_results = calloc(base_count, sizeof(cognitive_record_t*));
    if (!cog_results) {
        katra_memory_free_results(base_results, base_count);
        katra_report_error(E_SYSTEM_MEMORY, __func__,
                          "Failed to allocate cognitive results");
        return E_SYSTEM_MEMORY;
    }

    size_t cog_count = 0;
    for (size_t i = 0; i < base_count; i++) {
        cognitive_record_t* cog = katra_memory_to_cognitive(base_results[i]);
        if (cog && cog->confidence >= min_confidence) {
            cog_results[cog_count++] = cog;
        } else if (cog) {
            katra_cognitive_free_record(cog);
        }
    }

    katra_memory_free_results(base_results, base_count);

    *results = cog_results;
    *count = cog_count;

    LOG_INFO("Recalled %zu experiences for CI: %s", cog_count, ci_id);
    return KATRA_SUCCESS;
}

/* Convert base memory record to cognitive record */
cognitive_record_t* katra_memory_to_cognitive(const memory_record_t* base_record) {
    if (!base_record) {
        return NULL;
    }

    cognitive_record_t* cog;
    ALLOC_OR_RETURN_NULL(cog, cognitive_record_t);

    /* Copy base fields with safe strdup */
    cog->record_id = katra_safe_strdup(base_record->record_id);
    cog->timestamp = base_record->timestamp;
    cog->type = base_record->type;
    cog->importance = base_record->importance;
    cog->content = katra_safe_strdup(base_record->content);
    cog->response = katra_safe_strdup(base_record->response);
    cog->context = katra_safe_strdup(base_record->context);
    cog->ci_id = katra_safe_strdup(base_record->ci_id);
    cog->session_id = katra_safe_strdup(base_record->session_id);
    cog->component = katra_safe_strdup(base_record->component);
    cog->tier = base_record->tier;
    cog->archived = base_record->archived;

    /* Detect cognitive fields */
    if (cog->content) {
        cog->thought_type = katra_detect_thought_type(cog->content);
        cog->confidence = katra_calculate_confidence(cog->content, cog->thought_type);
    } else {
        cog->thought_type = THOUGHT_TYPE_UNKNOWN;
        cog->confidence = 0.0f;
    }

    /* Initialize association tracking */
    cog->related_ids = NULL;
    cog->related_count = 0;

    /* Initialize access tracking */
    cog->access_count = 0;
    cog->last_accessed = time(NULL);

    return cog;
}

/* Convert cognitive record to base memory record */
memory_record_t* katra_cognitive_to_memory(const cognitive_record_t* cognitive_record) {
    if (!cognitive_record) {
        return NULL;
    }

    memory_record_t* mem;
    ALLOC_OR_RETURN_NULL(mem, memory_record_t);

    /* Copy base fields with safe strdup */
    mem->record_id = katra_safe_strdup(cognitive_record->record_id);
    mem->timestamp = cognitive_record->timestamp;
    mem->type = cognitive_record->type;
    mem->importance = cognitive_record->importance;
    mem->content = katra_safe_strdup(cognitive_record->content);
    mem->response = katra_safe_strdup(cognitive_record->response);
    mem->context = katra_safe_strdup(cognitive_record->context);
    mem->ci_id = katra_safe_strdup(cognitive_record->ci_id);
    mem->session_id = katra_safe_strdup(cognitive_record->session_id);
    mem->component = katra_safe_strdup(cognitive_record->component);
    mem->tier = cognitive_record->tier;
    mem->archived = cognitive_record->archived;

    return mem;
}

/* Free cognitive record */
void katra_cognitive_free_record(cognitive_record_t* record) {
    if (!record) {
        return;
    }

    free(record->record_id);
    free(record->content);
    free(record->response);
    free(record->context);
    free(record->ci_id);
    free(record->session_id);
    free(record->component);

    /* Free related IDs */
    katra_free_string_array(record->related_ids, record->related_count);

    free(record);
}

/* Free cognitive query results */
void katra_cognitive_free_results(cognitive_record_t** results, size_t count) {
    if (!results) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        katra_cognitive_free_record(results[i]);
    }
    free(results);
}

/* Create association (placeholder for Phase 8: Graph Database) */
int katra_create_association(const char* ci_id, const char* memory_id_1,
                              const char* memory_id_2, const char* relationship) {
    ENGRAM_CHECK_PARAMS_3(ci_id, memory_id_1, memory_id_2);

    /* Placeholder - will be implemented in Phase 8 */
    LOG_DEBUG("Association created: %s <-> %s (relationship: %s)",
             memory_id_1, memory_id_2,
             relationship ? relationship : "unspecified");

    return KATRA_SUCCESS;
}

/* Record access (placeholder for memory metabolism) */
int katra_record_access(const char* ci_id, const char* record_id) {
    ENGRAM_CHECK_PARAMS_2(ci_id, record_id);

    /* Placeholder - will be implemented with access tracking persistence */
    LOG_DEBUG("Access recorded for memory: %s", record_id);

    return KATRA_SUCCESS;
}
