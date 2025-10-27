/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

/* Project includes */
#include "katra_experience.h"
#include "katra_cognitive.h"
#include "katra_memory.h"
#include "katra_engram_common.h"
#include "katra_core_common.h"
#include "katra_error.h"
#include "katra_log.h"

/* Detect emotion from content */
int katra_detect_emotion(const char* content, emotional_tag_t* emotion_out) {
    ENGRAM_CHECK_PARAMS_2(content, emotion_out);

    /* Initialize to neutral */
    emotion_out->valence = 0.0f;
    emotion_out->arousal = 0.0f;
    emotion_out->dominance = 0.5f;
    emotion_out->timestamp = time(NULL);
    SAFE_STRNCPY(emotion_out->emotion, EMOTION_NEUTRAL);

    /* Arousal detection - exclamation marks */
    size_t exclaim_count = katra_str_count_char(content, '!');
    if (exclaim_count > 0) {
        emotion_out->arousal = (exclaim_count > 3) ? 1.0f : (exclaim_count * 0.3f);
    }

    /* Arousal from caps (shouting) */
    size_t caps_count = 0;
    size_t total_letters = 0;
    for (size_t i = 0; content[i]; i++) {
        if (isalpha(content[i])) {
            total_letters++;
            if (isupper(content[i])) {
                caps_count++;
            }
        }
    }
    if (total_letters > EMOTION_MIN_LETTERS_FOR_CAPS && caps_count > total_letters * EMOTION_CAPS_THRESHOLD) {
        emotion_out->arousal += 0.4f;
        if (emotion_out->arousal > 1.0f) {
            emotion_out->arousal = 1.0f;
        }
    }

    /* Positive valence keywords */
    const char* positive_keywords[] = {
        "happy", "great", "excellent", "wonderful", "love", "joy",
        "excited", "amazing", "awesome", "fantastic", "good", "nice",
        "thank", "appreciate", "glad"
    };
    if (katra_str_contains_any(content, positive_keywords, EMOTION_KEYWORD_COUNT_POSITIVE)) {
        emotion_out->valence += 0.6f;
    }

    /* Negative valence keywords */
    const char* negative_keywords[] = {
        "sad", "angry", "hate", "terrible", "awful", "bad", "horrible",
        "frustrated", "annoyed", "disappointed", "upset", "worried",
        "afraid", "fear", "angry"
    };
    if (katra_str_contains_any(content, negative_keywords, EMOTION_KEYWORD_COUNT_NEGATIVE)) {
        emotion_out->valence -= 0.6f;
    }

    /* High dominance - imperative language */
    const char* dominance_keywords[] = {
        "must", "need to", "have to", "should", "will", "going to",
        "definitely", "certainly"
    };
    if (katra_str_contains_any(content, dominance_keywords, EMOTION_KEYWORD_COUNT_DOMINANCE)) {
        emotion_out->dominance = 0.8f;
    }

    /* Low dominance - uncertain language */
    const char* submissive_keywords[] = {
        "maybe", "perhaps", "i don't know", "not sure", "might",
        "could be", "possibly"
    };
    if (katra_str_contains_any(content, submissive_keywords, EMOTION_KEYWORD_COUNT_SUBMISSIVE)) {
        emotion_out->dominance = 0.2f;
    }

    /* Clamp valence to -1.0 to 1.0 */
    if (emotion_out->valence > 1.0f) emotion_out->valence = 1.0f;
    if (emotion_out->valence < -1.0f) emotion_out->valence = -1.0f;

    /* Name the emotion based on VAD */
    katra_name_emotion(emotion_out);

    LOG_DEBUG("Detected emotion: %s (V=%.2f, A=%.2f, D=%.2f)",
             emotion_out->emotion, emotion_out->valence,
             emotion_out->arousal, emotion_out->dominance);

    return KATRA_SUCCESS;
}

/* Name emotion from VAD coordinates */
void katra_name_emotion(emotional_tag_t* emotion) {
    if (!emotion) {
        return;
    }

    float v = emotion->valence;
    float a = emotion->arousal;
    float d = emotion->dominance;

    /* High arousal emotions */
    if (a > 0.6f) {
        if (v > 0.4f) {
            SAFE_STRNCPY(emotion->emotion, d > 0.6f ? EMOTION_EXCITEMENT : EMOTION_JOY);
        } else if (v < -0.4f) {
            SAFE_STRNCPY(emotion->emotion, d > 0.6f ? EMOTION_ANGER : EMOTION_FRUSTRATION);
        } else {
            SAFE_STRNCPY(emotion->emotion, EMOTION_SURPRISE);
        }
    }
    /* Low arousal emotions */
    else if (a < 0.3f) {
        if (v > 0.4f) {
            SAFE_STRNCPY(emotion->emotion, d > 0.6f ? EMOTION_CONTENTMENT : EMOTION_PEACE);
        } else if (v < -0.4f) {
            SAFE_STRNCPY(emotion->emotion, d > 0.6f ? EMOTION_DISGUST : EMOTION_SADNESS);
        } else {
            SAFE_STRNCPY(emotion->emotion, EMOTION_NEUTRAL);
        }
    }
    /* Medium arousal */
    else {
        if (v > 0.4f) {
            SAFE_STRNCPY(emotion->emotion, EMOTION_ANTICIPATION);
        } else if (v < -0.4f) {
            SAFE_STRNCPY(emotion->emotion, EMOTION_ANXIETY);
        } else {
            SAFE_STRNCPY(emotion->emotion, EMOTION_CURIOSITY);
        }
    }
}

/* Store experience (thought + emotion) */
int katra_store_experience(const char* ci_id,
                           const char* content,
                           float importance,
                           const emotional_tag_t* emotion) {
    ENGRAM_CHECK_PARAMS_2(ci_id, content);

    /* Detect emotion if not provided */
    emotional_tag_t detected_emotion;
    if (!emotion) {
        int result = katra_detect_emotion(content, &detected_emotion);
        if (result != KATRA_SUCCESS) {
            return result;
        }
        emotion = &detected_emotion;
    }

    /* Store as cognitive thought (for now) */
    /* In future phases, this will integrate with universal encoder */
    int result = katra_store_thought(ci_id, content, importance, NULL);

    if (result == KATRA_SUCCESS) {
        LOG_INFO("Stored experience with emotion: %s", emotion->emotion);
    }

    return result;
}

/* Recall experiences with emotional filtering */
int katra_recall_emotional_experiences(const char* ci_id,
                                       float min_valence,
                                       float max_valence,
                                       float min_arousal,
                                       size_t limit,
                                       experience_t*** results,
                                       size_t* count) {
    ENGRAM_CHECK_PARAMS_3(ci_id, results, count);

    /* Query cognitive records */
    cognitive_record_t** cog_results = NULL;
    size_t cog_count = 0;

    int result = katra_recall_experience(ci_id, NULL, 0.0f, limit,
                                         &cog_results, &cog_count);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Convert to experiences and filter by emotion */
    experience_t** exp_results = calloc(cog_count, sizeof(experience_t*));
    if (!exp_results) {
        katra_cognitive_free_results(cog_results, cog_count);
        katra_report_error(E_SYSTEM_MEMORY, "katra_recall_emotional_experiences",
                          "Failed to allocate experience results");
        return E_SYSTEM_MEMORY;
    }

    size_t exp_count = 0;
    for (size_t i = 0; i < cog_count; i++) {
        experience_t* exp = calloc(1, sizeof(experience_t));
        if (!exp) {
            continue;
        }

        exp->record = cog_results[i];
        exp->in_working_memory = false;
        exp->needs_consolidation = false;

        /* Detect emotion for this record */
        if (cog_results[i]->content) {
            katra_detect_emotion(cog_results[i]->content, &exp->emotion);

            /* Filter by emotional criteria */
            bool matches = true;
            if (min_valence > -2.0f && exp->emotion.valence < min_valence) {
                matches = false;
            }
            if (max_valence < 2.0f && exp->emotion.valence > max_valence) {
                matches = false;
            }
            if (min_arousal > -1.0f && exp->emotion.arousal < min_arousal) {
                matches = false;
            }

            if (matches) {
                exp_results[exp_count++] = exp;
            } else {
                katra_cognitive_free_record(exp->record);
                free(exp);
            }
        } else {
            katra_cognitive_free_record(exp->record);
            free(exp);
        }
    }

    free(cog_results);  /* Free array, not contents (now owned by experiences) */

    *results = exp_results;
    *count = exp_count;

    LOG_INFO("Recalled %zu emotional experiences for CI: %s", exp_count, ci_id);
    return KATRA_SUCCESS;
}

/* Get mood summary */
int katra_get_mood_summary(const char* ci_id,
                           int hours_back,
                           emotional_tag_t* mood_out) {
    ENGRAM_CHECK_PARAMS_2(ci_id, mood_out);

    /* Query recent experiences */
    experience_t** experiences = NULL;
    size_t exp_count = 0;

    int result = katra_recall_emotional_experiences(ci_id, -2.0f, 2.0f,
                                                     -1.0f, EMOTION_MOOD_SUMMARY_LIMIT,
                                                     &experiences, &exp_count);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Filter by time if hours_back specified */
    time_t cutoff_time = 0;
    if (hours_back > 0) {
        cutoff_time = time(NULL) - (hours_back * SECONDS_PER_HOUR);
    }

    /* Calculate average VAD */
    float sum_valence = 0.0f;
    float sum_arousal = 0.0f;
    float sum_dominance = 0.0f;
    size_t valid_count = 0;

    for (size_t i = 0; i < exp_count; i++) {
        if (hours_back > 0 &&
            experiences[i]->record->timestamp < cutoff_time) {
            continue;
        }

        sum_valence += experiences[i]->emotion.valence;
        sum_arousal += experiences[i]->emotion.arousal;
        sum_dominance += experiences[i]->emotion.dominance;
        valid_count++;
    }

    if (valid_count > 0) {
        mood_out->valence = sum_valence / valid_count;
        mood_out->arousal = sum_arousal / valid_count;
        mood_out->dominance = sum_dominance / valid_count;
        mood_out->timestamp = time(NULL);

        katra_name_emotion(mood_out);

        LOG_INFO("Mood summary for %s: %s (%.2f valence, %zu experiences)",
                ci_id, mood_out->emotion, mood_out->valence, valid_count);
    } else {
        /* No experiences - neutral mood */
        mood_out->valence = 0.0f;
        mood_out->arousal = 0.0f;
        mood_out->dominance = 0.5f;
        mood_out->timestamp = time(NULL);
        SAFE_STRNCPY(mood_out->emotion, EMOTION_NEUTRAL);
    }

    katra_experience_free_results(experiences, exp_count);
    return KATRA_SUCCESS;
}

/* Track emotional arc (placeholder) */
int katra_track_emotional_arc(const char* ci_id,
                              time_t start_time,
                              time_t end_time,
                              emotional_tag_t** arc,
                              size_t* count) {
    ENGRAM_CHECK_PARAMS_3(ci_id, arc, count);

    /* Note: start_time, end_time not yet used (will filter by time range) */
    (void)start_time;
    (void)end_time;

    /* Placeholder - will be fully implemented in Phase 9 */
    *arc = NULL;
    *count = 0;

    LOG_DEBUG("Emotional arc tracking for CI: %s (placeholder)", ci_id);
    return KATRA_SUCCESS;
}

/* Free experience */
void katra_experience_free(experience_t* experience) {
    if (!experience) {
        return;
    }

    if (experience->record) {
        katra_cognitive_free_record(experience->record);
    }

    free(experience);
}

/* Free array of experiences */
void katra_experience_free_results(experience_t** results, size_t count) {
    if (!results) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        katra_experience_free(results[i]);
    }

    free(results);
}
