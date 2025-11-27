/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_emotion.c - Emotional tagging with PAD model (Phase 6.3)
 *
 * Implements affective memory formation and recall using the PAD
 * (Pleasure, Arousal, Dominance) model for emotional representation.
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_breathing_internal.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* ============================================================================
 * EMOTION UTILITIES
 * ============================================================================ */

/* Validate PAD values are in range [-1.0, 1.0] */
static bool is_valid_emotion(const emotion_t* emotion) {
    if (!emotion) {
        return false;
    }

    if (emotion->pleasure < -1.0f || emotion->pleasure > 1.0f) {
        return false;
    }
    if (emotion->arousal < -1.0f || emotion->arousal > 1.0f) {
        return false;
    }
    if (emotion->dominance < -1.0f || emotion->dominance > 1.0f) {
        return false;
    }

    return true;
}

/* Calculate Euclidean distance between two emotions in PAD space */
static float emotion_distance(const emotion_t* e1, const emotion_t* e2) {
    if (!e1 || !e2) {
        return INFINITY;
    }

    float dp = e1->pleasure - e2->pleasure;
    float da = e1->arousal - e2->arousal;
    float dd = e1->dominance - e2->dominance;

    return sqrtf(dp*dp + da*da + dd*dd);
}

/* Convert PAD emotion to legacy emotion_intensity and emotion_type */
static void pad_to_legacy_emotion(const emotion_t* pad, float* intensity, char** type) {
    if (!pad || !intensity || !type) {
        return;
    }

    /* Intensity = magnitude in PAD space */
    float magnitude = sqrtf(pad->pleasure * pad->pleasure +
                           pad->arousal * pad->arousal +
                           pad->dominance * pad->dominance);
    *intensity = magnitude / sqrtf(3.0f);  /* Normalize to [0, 1] */

    /* Type = classify based on dominant dimension */
    if (fabsf(pad->pleasure) > fabsf(pad->arousal) &&
        fabsf(pad->pleasure) > fabsf(pad->dominance)) {
        /* Pleasure-dominant */
        *type = strdup(pad->pleasure > 0 ? "joy" : "sadness");
    } else if (fabsf(pad->arousal) > fabsf(pad->dominance)) {
        /* Arousal-dominant */
        *type = strdup(pad->arousal > 0 ? "excitement" : "calmness");
    } else {
        /* Dominance-dominant */
        *type = strdup(pad->dominance > 0 ? "confidence" : "anxiety");
    }
}

/* ============================================================================
 * PUBLIC API - Emotional Memory Formation
 * ============================================================================ */

int remember_with_emotion(const char* thought, why_remember_t why, const emotion_t* emotion) {
    if (!thought) {
        katra_report_error(E_INPUT_NULL, "remember_with_emotion",
                          "Thought content is NULL");
        return E_INPUT_NULL;
    }

    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "remember_with_emotion",
                          "Breathing layer not initialized");
        return E_INVALID_STATE;
    }

    /* Validate emotion if provided */
    if (emotion && !is_valid_emotion(emotion)) {
        katra_report_error(E_INPUT_RANGE, "remember_with_emotion",
                          "Emotion values must be in range [-1.0, 1.0]");
        return E_INPUT_RANGE;
    }

    /* Create memory record */
    const char* ci_id = breathing_get_ci_id();
    memory_record_t* record = katra_memory_create_record(
        ci_id,
        MEMORY_TYPE_EXPERIENCE,
        thought,
        why_to_importance(why)
    );

    if (!record) {
        katra_report_error(E_SYSTEM_MEMORY, "remember_with_emotion",
                          "Failed to create memory record");
        return E_SYSTEM_MEMORY;
    }

    /* Add PAD emotion if provided */
    if (emotion) {
        /* Store PAD values in context field as JSON */
        char pad_json[KATRA_BUFFER_MEDIUM];
        snprintf(pad_json, sizeof(pad_json),
                "{\"emotion\":{\"pad\":{\"pleasure\":%.2f,\"arousal\":%.2f,\"dominance\":%.2f}}}",
                emotion->pleasure, emotion->arousal, emotion->dominance);

        if (record->context) {
            free(record->context);
        }
        record->context = strdup(pad_json);

        /* Also populate legacy emotion fields for backward compatibility */
        float intensity = 0.0f;
        char* type = NULL;
        pad_to_legacy_emotion(emotion, &intensity, &type);

        record->emotion_intensity = intensity;
        if (record->emotion_type) {
            free(record->emotion_type);
        }
        record->emotion_type = type;

        LOG_DEBUG("Stored emotion: PAD(%.2f, %.2f, %.2f) -> %s (intensity: %.2f)",
                 emotion->pleasure, emotion->arousal, emotion->dominance,
                 type ? type : "unknown", intensity);
    }

    /* Store memory */
    int result = katra_memory_store(record);
    if (result == KATRA_SUCCESS) {
        breathing_track_memory_stored(MEMORY_TYPE_EXPERIENCE, why);
        LOG_DEBUG("Memory stored with %s emotion",
                 emotion ? "explicit" : "no");
    }

    katra_memory_free_record(record);
    return result;
}

/* ============================================================================
 * PUBLIC API - Emotional Recall
 * ============================================================================ */

char** recall_by_emotion(const emotion_t* target_emotion, float threshold, size_t* count) {
    if (!target_emotion || !count) {
        katra_report_error(E_INPUT_NULL, "recall_by_emotion",
                          "Target emotion or count pointer is NULL");
        return NULL;
    }

    if (!breathing_get_initialized()) {
        katra_report_error(E_INVALID_STATE, "recall_by_emotion",
                          "Breathing layer not initialized");
        return NULL;
    }

    *count = 0;

    if (!is_valid_emotion(target_emotion)) {
        katra_report_error(E_INPUT_RANGE, "recall_by_emotion",
                          "Target emotion values must be in range [-1.0, 1.0]");
        return NULL;
    }

    /* Get recent memories to search through */
    const char* ci_id = breathing_get_ci_id();
    memory_record_t** records = NULL;
    size_t record_count = 0;

    /* Build query for recent memories */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = BREATHING_DEFAULT_TOPIC_RECALL * EMOTION_SEARCH_MULTIPLIER  /* Broad search */
    };

    int result = katra_memory_query(&query, &records, &record_count);

    if (result != KATRA_SUCCESS || !records) {
        LOG_DEBUG("No memories found to search (result=%d)", result);
        return NULL;
    }

    LOG_DEBUG("Found %zu memories to search for emotional matches", record_count);

    /* Collect matching memories */
    char** matches = NULL;
    size_t match_capacity = INITIAL_CAPACITY_FALLBACK;  /* Initial capacity for matches */
    size_t match_count = 0;

    matches = calloc(match_capacity, sizeof(char*));
    if (!matches) {
        katra_report_error(E_SYSTEM_MEMORY, "recall_by_emotion",
                          "Failed to allocate match array");
        katra_memory_free_results(records, record_count);
        return NULL;
    }

    /* Search for emotionally similar memories */
    for (size_t i = 0; i < record_count; i++) {
        memory_record_t* rec = records[i];

        /* Extract PAD values from context JSON */
        if (rec->context && strstr(rec->context, "pad")) {
            emotion_t mem_emotion;
            /* Simple JSON parsing - handle both escaped and unescaped quotes */
            int parsed = sscanf(rec->context,
                              "{\"emotion\":{\"pad\":{\"pleasure\":%f,\"arousal\":%f,\"dominance\":%f",
                              &mem_emotion.pleasure,
                              &mem_emotion.arousal,
                              &mem_emotion.dominance);

            if (parsed == 3) {
                float distance = emotion_distance(target_emotion, &mem_emotion);

                if (distance <= threshold) {
                    /* Expand array if needed */
                    if (match_count >= match_capacity) {
                        match_capacity *= 2;
                        char** new_matches = realloc(matches,
                                                    match_capacity * sizeof(char*));
                        if (!new_matches) {
                            katra_report_error(E_SYSTEM_MEMORY, "recall_by_emotion",
                                             "Failed to expand match array");
                            goto cleanup;
                        }
                        matches = new_matches;
                    }

                    /* Add match */
                    matches[match_count] = strdup(rec->content);
                    if (!matches[match_count]) {
                        katra_report_error(E_SYSTEM_MEMORY, "recall_by_emotion",
                                         "Failed to copy memory content");
                        goto cleanup;
                    }
                    match_count++;

                    LOG_DEBUG("Match: distance=%.2f, content='%.50s...'",
                             distance, rec->content);
                }
            }
        }
    }

    katra_memory_free_results(records, record_count);

    if (match_count == 0) {
        free(matches);
        LOG_DEBUG("No emotionally similar memories found within threshold %.2f", threshold);
        return NULL;
    }

    *count = match_count;
    LOG_INFO("Found %zu memories within emotional distance %.2f", match_count, threshold);
    return matches;

cleanup:
    /* Free partial results on error */
    for (size_t i = 0; i < match_count; i++) {
        free(matches[i]);
    }
    free(matches);
    katra_memory_free_results(records, record_count);
    return NULL;
}
