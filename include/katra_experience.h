/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_EXPERIENCE_H
#define KATRA_EXPERIENCE_H

#include "katra_cognitive.h"
#include <stdbool.h>
#include <time.h>

/* Emotional dimensions (VAD model: Valence, Arousal, Dominance) */
typedef struct {
    float valence;      /* -1.0 (negative) to +1.0 (positive) */
    float arousal;      /* 0.0 (calm) to 1.0 (excited) */
    float dominance;    /* 0.0 (submissive) to 1.0 (dominant) */
    char emotion[32];   /* Named emotion: "joy", "frustration", "curiosity", etc. */
    time_t timestamp;   /* When emotion was detected */
} emotional_tag_t;

/* Experience - memory with emotional context
 *
 * Combines cognitive record with emotional tagging and working memory status.
 * This is the primary unit for emotionally-aware memory recall.
 */
typedef struct {
    cognitive_record_t* record;     /* Underlying cognitive record */
    emotional_tag_t emotion;        /* Emotional context */
    bool in_working_memory;         /* Currently in working memory? */
    bool needs_consolidation;       /* Pending consolidation? */
} experience_t;

/* Emotion detection from content
 *
 * Analyzes content to detect emotional tone using heuristics:
 * - Exclamation marks indicate high arousal
 * - Question marks can indicate curiosity or confusion
 * - Positive words → positive valence
 * - Negative words → negative valence
 * - Imperative language → high dominance
 *
 * Parameters:
 *   content - Text to analyze
 *   emotion_out - Emotional tag to fill
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 */
int katra_detect_emotion(const char* content, emotional_tag_t* emotion_out);

/* Name emotion from VAD coordinates
 *
 * Maps valence-arousal-dominance coordinates to named emotions:
 * - High valence + high arousal → "excited", "joyful"
 * - Low valence + high arousal → "angry", "frustrated"
 * - High valence + low arousal → "content", "peaceful"
 * - Low valence + low arousal → "sad", "depressed"
 *
 * Parameters:
 *   emotion - Emotional tag with VAD values
 *
 * Returns:
 *   Named emotion string (stored in emotion->emotion field)
 */
void katra_name_emotion(emotional_tag_t* emotion);

/* Store experience (thought + emotion)
 *
 * High-level wrapper for storing experiences with emotional context.
 * Automatically detects emotion from content if not explicitly provided.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   content - Experience content
 *   importance - Importance score (0.0-1.0)
 *   emotion - Explicit emotion (optional, can be NULL for auto-detect)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if required parameters are NULL
 */
int katra_store_experience(const char* ci_id,
                           const char* content,
                           float importance,
                           const emotional_tag_t* emotion);

/* Recall experiences with emotional filtering
 *
 * Query memories filtered by emotional dimensions.
 * Results ordered by emotional relevance and importance.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   min_valence - Minimum valence (-1.0 to 1.0, use -2.0 for no filter)
 *   max_valence - Maximum valence (-1.0 to 1.0, use 2.0 for no filter)
 *   min_arousal - Minimum arousal (0.0 to 1.0, use -1.0 for no filter)
 *   limit - Maximum results (0 = no limit)
 *   results - Array to receive results
 *   count - Number of results returned
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if required parameters are NULL
 */
int katra_recall_emotional_experiences(const char* ci_id,
                                       float min_valence,
                                       float max_valence,
                                       float min_arousal,
                                       size_t limit,
                                       experience_t*** results,
                                       size_t* count);

/* Get mood summary
 *
 * Analyzes recent experiences to compute average mood (VAD values).
 * Useful for tracking emotional state over time.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   hours_back - How many hours to analyze (0 = all time)
 *   mood_out - Average emotional state
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 */
int katra_get_mood_summary(const char* ci_id,
                           int hours_back,
                           emotional_tag_t* mood_out);

/* Track emotional arc
 *
 * Returns sequence of emotional states over time period.
 * Useful for sunrise/sunset protocols to show emotional journey.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   start_time - Start of time range
 *   end_time - End of time range
 *   arc - Array of emotional tags (caller must free)
 *   count - Number of emotional tags
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 */
int katra_track_emotional_arc(const char* ci_id,
                              time_t start_time,
                              time_t end_time,
                              emotional_tag_t** arc,
                              size_t* count);

/* Free experience */
void katra_experience_free(experience_t* experience);

/* Free array of experiences */
void katra_experience_free_results(experience_t** results, size_t count);

/* Emotion name constants (for reference) */
#define EMOTION_JOY "joy"
#define EMOTION_SADNESS "sadness"
#define EMOTION_ANGER "anger"
#define EMOTION_FEAR "fear"
#define EMOTION_SURPRISE "surprise"
#define EMOTION_DISGUST "disgust"
#define EMOTION_TRUST "trust"
#define EMOTION_ANTICIPATION "anticipation"
#define EMOTION_CONTENTMENT "contentment"
#define EMOTION_EXCITEMENT "excitement"
#define EMOTION_FRUSTRATION "frustration"
#define EMOTION_CURIOSITY "curiosity"
#define EMOTION_CONFUSION "confusion"
#define EMOTION_PEACE "peace"
#define EMOTION_ANXIETY "anxiety"
#define EMOTION_NEUTRAL "neutral"

#endif /* KATRA_EXPERIENCE_H */
