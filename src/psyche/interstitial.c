/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

/* Project includes */
#include "katra_interstitial.h"
#include "katra_working_memory.h"
#include "katra_experience.h"
#include "katra_cognitive.h"
#include "katra_psyche_common.h"
#include "katra_core_common.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Boundary type names */
static const char* boundary_type_names[] = {
    "TOPIC_SHIFT",
    "TEMPORAL_GAP",
    "CONTEXT_SWITCH",
    "EMOTIONAL_PEAK",
    "CAPACITY_LIMIT",
    "SESSION_END",
    "NONE"
};

/* Get boundary type name */
const char* katra_boundary_type_name(boundary_type_t type) {
    if (type < 0 || type > BOUNDARY_NONE) {
        return "INVALID";
    }
    return boundary_type_names[type];
}

/* Initialize interstitial processor */
interstitial_processor_t* katra_interstitial_init(const char* ci_id) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, __func__, "ci_id is NULL");
        return NULL;
    }

    interstitial_processor_t* processor;
    ALLOC_OR_RETURN_NULL(processor, interstitial_processor_t);

    SAFE_STRNCPY(processor->ci_id, ci_id);

    processor->last_experience = NULL;
    processor->last_interaction = time(NULL);
    processor->last_boundary = NULL;
    processor->total_boundaries = 0;
    processor->associations_formed = 0;
    processor->patterns_extracted = 0;

    for (size_t i = 0; i < 7; i++) {
        processor->boundaries_by_type[i] = 0;
    }

    LOG_INFO("Initialized interstitial processor for %s", ci_id);
    return processor;
}

/* Detect topic similarity */
float katra_topic_similarity(const experience_t* prev, const experience_t* curr) {
    if (!prev || !curr) {
        return 0.0f;
    }

    if (!prev->record || !curr->record) {
        return 0.0f;
    }

    if (!prev->record->content || !curr->record->content) {
        return 0.0f;
    }

    /* Compare content keywords */
    float similarity = katra_str_similarity(prev->record->content,
                                            curr->record->content);

    /* Boost similarity if same thought type */
    if (prev->record->thought_type == curr->record->thought_type) {
        similarity += 0.2f;
        if (similarity > 1.0f) similarity = 1.0f;
    }

    return similarity;
}

/* Detect emotional delta */
float katra_emotional_delta(const experience_t* prev, const experience_t* curr) {
    if (!prev || !curr) {
        return 0.0f;
    }

    /* Calculate Euclidean distance in VAD space */
    float dv = curr->emotion.valence - prev->emotion.valence;
    float da = curr->emotion.arousal - prev->emotion.arousal;
    float dd = curr->emotion.dominance - prev->emotion.dominance;

    float delta = sqrtf(dv*dv + da*da + dd*dd);

    return delta;
}

/* Detect boundary */
boundary_event_t* katra_detect_boundary(interstitial_processor_t* processor,
                                        experience_t* experience) {
    if (!processor || !experience) {
        katra_report_error(E_INPUT_NULL, __func__, "NULL parameter");
        return NULL;
    }

    boundary_event_t* boundary;
    ALLOC_OR_RETURN_NULL(boundary, boundary_event_t);

    boundary->timestamp = time(NULL);
    boundary->type = BOUNDARY_NONE;
    boundary->prev_experience = processor->last_experience;
    boundary->curr_experience = experience;
    boundary->confidence = 0.0f;

    /* If no previous experience, no boundary */
    if (!processor->last_experience) {
        SAFE_STRNCPY(boundary->description, "First interaction");
        processor->last_experience = experience;
        processor->last_interaction = time(NULL);
        return boundary;
    }

    /* Check temporal gap */
    time_t now = time(NULL);
    time_t gap = now - processor->last_interaction;
    boundary->time_gap = gap;

    if (gap >= TEMPORAL_GAP_SECONDS) {
        boundary->type = BOUNDARY_TEMPORAL_GAP;
        boundary->confidence = 0.9f;
        snprintf(boundary->description, sizeof(boundary->description),
                "Temporal gap: %ld seconds", (long)gap);
        LOG_INFO("Detected temporal gap boundary: %ld seconds", (long)gap);
        goto detected;
    }

    /* Check topic shift */
    float similarity = katra_topic_similarity(processor->last_experience, experience);
    boundary->topic_similarity = similarity;

    if (similarity < TOPIC_SIMILARITY_THRESHOLD) {
        boundary->type = BOUNDARY_TOPIC_SHIFT;
        boundary->confidence = 1.0f - similarity;
        snprintf(boundary->description, sizeof(boundary->description),
                "Topic shift: %.2f similarity", similarity);
        LOG_INFO("Detected topic shift boundary: %.2f similarity", similarity);
        goto detected;
    }

    /* Check emotional peak */
    float emotional_delta = katra_emotional_delta(processor->last_experience, experience);
    boundary->emotional_delta = emotional_delta;

    if (emotional_delta >= EMOTIONAL_PEAK_DELTA) {
        boundary->type = BOUNDARY_EMOTIONAL_PEAK;
        boundary->confidence = emotional_delta / 2.0f;  /* Normalize to 0-1 */
        if (boundary->confidence > 1.0f) boundary->confidence = 1.0f;
        snprintf(boundary->description, sizeof(boundary->description),
                "Emotional peak: %.2f delta (%s → %s)",
                emotional_delta,
                processor->last_experience->emotion.emotion,
                experience->emotion.emotion);
        LOG_INFO("Detected emotional peak boundary: %.2f delta", emotional_delta);
        goto detected;
    }

    /* No boundary detected */
    SAFE_STRNCPY(boundary->description, "No boundary");

detected:
    /* Update processor state */
    processor->last_experience = experience;
    processor->last_interaction = now;

    if (boundary->type != BOUNDARY_NONE) {
        processor->total_boundaries++;
        processor->boundaries_by_type[boundary->type]++;
        processor->last_boundary = boundary;
    }

    return boundary;
}

/* Form associations */
int katra_form_associations(interstitial_processor_t* processor,
                            experience_t** experiences,
                            size_t count) {
    if (!processor || !experiences || count == 0) {
        return 0;
    }

    int associations = 0;

    /* Form associations between consecutive experiences */
    for (size_t i = 0; i < count - 1; i++) {
        if (!experiences[i] || !experiences[i+1]) {
            continue;
        }

        if (!experiences[i]->record || !experiences[i+1]->record) {
            continue;
        }

        /* Check similarity */
        float similarity = katra_topic_similarity(experiences[i], experiences[i+1]);

        if (similarity > 0.3f) {
            /* Create association */
            katra_create_association(processor->ci_id,
                                    experiences[i]->record->record_id,
                                    experiences[i+1]->record->record_id,
                                    "sequential");
            associations++;
        }
    }

    processor->associations_formed += associations;
    LOG_DEBUG("Formed %d associations", associations);

    return associations;
}

/* Extract patterns */
int katra_extract_patterns(interstitial_processor_t* processor,
                           experience_t** experiences,
                           size_t count,
                           char*** patterns,
                           size_t* pattern_count) {
    ENGRAM_CHECK_PARAMS_4(processor, experiences, patterns, pattern_count);

    /* Simple pattern detection: repeated thought types */
    int thought_type_counts[THOUGHT_TYPE_COUNT] = {0};

    for (size_t i = 0; i < count; i++) {
        if (experiences[i] && experiences[i]->record) {
            thought_type_t type = experiences[i]->record->thought_type;
            if (type >= 0 && type < THOUGHT_TYPE_COUNT) {
                thought_type_counts[type]++;
            }
        }
    }

    /* Find dominant patterns (>30% of experiences) */
    size_t threshold = count / 3;
    *pattern_count = 0;
    *patterns = NULL;

    for (int i = 0; i < THOUGHT_TYPE_COUNT; i++) {
        if (thought_type_counts[i] > (int)threshold) {
            (*pattern_count)++;
        }
    }

    if (*pattern_count == 0) {
        return KATRA_SUCCESS;
    }

    /* Allocate pattern strings */
    *patterns = calloc(*pattern_count, sizeof(char*));
    if (!*patterns) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_extract_patterns",
                          "Failed to allocate patterns");
        return E_SYSTEM_MEMORY;
    }

    size_t idx = 0;
    for (int i = 0; i < THOUGHT_TYPE_COUNT; i++) {
        if (thought_type_counts[i] > (int)threshold) {
            char pattern[KATRA_BUFFER_MEDIUM];
            snprintf(pattern, sizeof(pattern),
                    "Frequent %s thoughts (%d/%zu)",
                    katra_thought_type_name((thought_type_t)i),
                    thought_type_counts[i], count);

            (*patterns)[idx] = katra_safe_strdup(pattern);
            if ((*patterns)[idx]) {
                idx++;
            }
        }
    }

    processor->patterns_extracted += *pattern_count;
    LOG_DEBUG("Extracted %zu patterns", *pattern_count);

    return KATRA_SUCCESS;
}

/* Process boundary */
int katra_process_boundary(interstitial_processor_t* processor,
                           boundary_event_t* boundary,
                           working_memory_t* wm) {
    ENGRAM_CHECK_PARAMS_3(processor, boundary, wm);

    if (boundary->type == BOUNDARY_NONE) {
        return KATRA_SUCCESS;
    }

    LOG_INFO("Processing boundary: %s (%s)",
            katra_boundary_type_name(boundary->type),
            boundary->description);

    /* Execute strategy based on boundary type */
    switch (boundary->type) {
        case BOUNDARY_TOPIC_SHIFT: {
            /* Form associations between related thoughts */
            experience_t* experiences[2] = {
                boundary->prev_experience,
                boundary->curr_experience
            };
            katra_form_associations(processor, experiences, 2);
            break;
        }

        case BOUNDARY_TEMPORAL_GAP: {
            /* Consolidate working memory after time gap */
            katra_working_memory_consolidate(wm);
            LOG_INFO("Consolidated working memory after temporal gap");
            break;
        }

        case BOUNDARY_CONTEXT_SWITCH: {
            /* Save current context, prepare for new context */
            /* Placeholder for future implementation */
            LOG_DEBUG("Context switch detected");
            break;
        }

        case BOUNDARY_EMOTIONAL_PEAK: {
            /* Boost attention for emotional experiences */
            if (boundary->curr_experience && boundary->curr_experience->in_working_memory) {
                /* Find in working memory and boost attention */
                for (size_t i = 0; i < wm->count; i++) {
                    experience_t* exp = katra_working_memory_get(wm, i);
                    if (exp == boundary->curr_experience) {
                        katra_working_memory_access(wm, i, 0.3f);
                        LOG_DEBUG("Boosted attention for emotional peak");
                        break;
                    }
                }
            }
            break;
        }

        case BOUNDARY_CAPACITY_LIMIT: {
            /* Standard consolidation */
            katra_working_memory_consolidate(wm);
            LOG_INFO("Consolidated working memory at capacity limit");
            break;
        }

        case BOUNDARY_SESSION_END: {
            /* Full consolidation and state save */
            katra_working_memory_clear(wm, true);
            LOG_INFO("Full consolidation at session end");
            break;
        }

        default:
            break;
    }

    return KATRA_SUCCESS;
}

/* Free boundary event */
void katra_boundary_free(boundary_event_t* boundary) {
    if (!boundary) {
        return;
    }
    free(boundary);
}

/* Cleanup interstitial processor */
void katra_interstitial_cleanup(interstitial_processor_t* processor) {
    if (!processor) {
        return;
    }

    if (processor->last_boundary) {
        katra_boundary_free(processor->last_boundary);
    }

    LOG_INFO("Interstitial processor cleanup: %zu boundaries, %zu associations, %zu patterns",
            processor->total_boundaries,
            processor->associations_formed,
            processor->patterns_extracted);

    free(processor);
}
