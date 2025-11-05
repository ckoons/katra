/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Project includes */
#include "katra_tier1_pattern.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_core_common.h"

/* Pattern detection thresholds */
#define SIMILARITY_THRESHOLD 0.4f         /* 40% keyword overlap = similar */
#define MIN_PATTERN_SIZE 3                /* Need 3+ memories to form pattern */
#define MIN_KEYWORD_LENGTH 4              /* Minimum keyword length for pattern matching */

/* Thane's Phase 4 Priority 4: Temporal clustering */
#define TEMPORAL_CLUSTER_RECENT_DAYS 30   /* "Recent" pattern = < 30 days old */
#define TEMPORAL_WINDOW_RECENT 7          /* Recent patterns: cluster within 7 days */
#define TEMPORAL_WINDOW_OLD 30            /* Old patterns: cluster within 30 days */

/* Static helper functions */
static void free_keywords_pattern(char** keywords, size_t count);
static bool is_stop_word_pattern(const char* word);
static int extract_keywords_pattern(const char* text, char*** keywords, size_t* count);
static float calculate_similarity(const char* content1, const char* content2);
static void assign_pattern_to_members(memory_record_t** records, size_t* pattern_members,
                                     size_t member_count, const char* pattern_id);
static void mark_standard_outliers(memory_record_t** records, size_t* pattern_members,
                                  size_t member_count);
static void find_and_mark_emotional_outlier(memory_record_t** records, size_t* pattern_members,
                                           size_t member_count);
static void add_pattern_summary_to_outliers(memory_record_t** records, size_t* pattern_members,
                                           size_t member_count);

/* Helper: Free keyword array (Phase 3) */
static void free_keywords_pattern(char** keywords, size_t count) {
    if (!keywords) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        free(keywords[i]);
    }
    free(keywords);
}

/* Helper: Check if word is a common stop word (Phase 3) */
static bool is_stop_word_pattern(const char* word) {
    for (int i = 0; KATRA_STOP_WORDS[i] != NULL; i++) {
        if (strcmp(word, KATRA_STOP_WORDS[i]) == 0) {
            return true;
        }
    }
    return false;
}

/* Helper: Extract keywords from text (Phase 3)
 *
 * Same approach as Phase 2 connection building:
 * - Split on whitespace and punctuation
 * - Keep words >= MIN_KEYWORD_LENGTH
 * - Convert to lowercase
 * - Remove stop words
 */
static int extract_keywords_pattern(const char* text, char*** keywords, size_t* count) {
    if (!text || !keywords || !count) {
        return E_INPUT_NULL;
    }

    *keywords = NULL;
    *count = 0;

    /* Allocate space for keywords (max: one per 5 characters) */
    size_t text_len = strlen(text);
    size_t max_keywords = (text_len / MIN_KEYWORD_LENGTH) + 1;
    char** kw_array = calloc(max_keywords, sizeof(char*));
    if (!kw_array) {
        return E_SYSTEM_MEMORY;
    }

    /* Copy text for tokenization */
    char* text_copy = strdup(text);
    if (!text_copy) {
        free(kw_array);
        return E_SYSTEM_MEMORY;
    }

    size_t kw_count = 0;
    char* token = strtok(text_copy, KATRA_TOKENIZE_DELIMITERS);

    while (token && kw_count < max_keywords) {
        /* Check length */
        if (strlen(token) < MIN_KEYWORD_LENGTH) {
            token = strtok(NULL, KATRA_TOKENIZE_DELIMITERS);
            continue;
        }

        /* Convert to lowercase */
        char lowercase[KATRA_BUFFER_MEDIUM];
        size_t i;
        for (i = 0; i < strlen(token) && i < sizeof(lowercase) - 1; i++) {
            lowercase[i] = tolower((unsigned char)token[i]);
        }
        lowercase[i] = '\0';

        /* Skip stop words */
        if (is_stop_word_pattern(lowercase)) {
            token = strtok(NULL, KATRA_TOKENIZE_DELIMITERS);
            continue;
        }

        /* Skip duplicates */
        bool duplicate = false;
        for (size_t j = 0; j < kw_count; j++) {
            if (strcmp(kw_array[j], lowercase) == 0) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate) {
            kw_array[kw_count] = strdup(lowercase);
            if (!kw_array[kw_count]) {
                free_keywords_pattern(kw_array, kw_count);
                free(text_copy);
                return E_SYSTEM_MEMORY;
            }
            kw_count++;
        }

        token = strtok(NULL, KATRA_TOKENIZE_DELIMITERS);
    }

    free(text_copy);

    *keywords = kw_array;
    *count = kw_count;
    return KATRA_SUCCESS;
}

/* Helper: Calculate semantic similarity between two memories (Phase 3)
 *
 * Enhanced keyword-based similarity for pattern detection.
 * Uses same approach as Phase 2 connection building for consistency.
 * Returns score 0.0-1.0 based on shared keywords.
 */
static float calculate_similarity(const char* content1, const char* content2) {
    if (!content1 || !content2) {
        return 0.0f;
    }

    /* Extract keywords from both memories */
    char** keywords1 = NULL;
    size_t count1 = 0;
    char** keywords2 = NULL;
    size_t count2 = 0;

    if (extract_keywords_pattern(content1, &keywords1, &count1) != KATRA_SUCCESS || count1 == 0) {
        return 0.0f;
    }

    if (extract_keywords_pattern(content2, &keywords2, &count2) != KATRA_SUCCESS || count2 == 0) {
        free_keywords_pattern(keywords1, count1);
        return 0.0f;
    }

    /* Count shared keywords */
    size_t shared = 0;
    for (size_t i = 0; i < count1; i++) {
        for (size_t j = 0; j < count2; j++) {
            if (strcmp(keywords1[i], keywords2[j]) == 0) {
                shared++;
                break;
            }
        }
    }

    free_keywords_pattern(keywords1, count1);
    free_keywords_pattern(keywords2, count2);

    /* Calculate similarity: shared / max(count1, count2)
     * This gives score 0.0-1.0 where 1.0 = all keywords match */
    size_t max_count = count1 > count2 ? count1 : count2;
    return max_count > 0 ? (float)shared / (float)max_count : 0.0f;
}

/* Helper: Check if two memories should cluster (Thane's Phase 4 Priority 4)
 *
 * Prevents clustering "debugging from 6 months ago" with "debugging from yesterday".
 * Different episodes should remain separate patterns.
 *
 * Recent patterns (< 30 days old): Tight temporal clustering (7 days)
 * Old patterns (> 30 days old): Looser temporal clustering (30 days)
 */
static bool should_cluster(memory_record_t* m1, memory_record_t* m2, float similarity) {
    if (similarity < SIMILARITY_THRESHOLD) {
        return false;  /* Not semantically similar enough */
    }

    /* Calculate time difference */
    time_t time_diff = m1->timestamp > m2->timestamp ?
                       m1->timestamp - m2->timestamp :
                       m2->timestamp - m1->timestamp;

    time_t days_diff = time_diff / SECONDS_PER_DAY;

    /* Determine if pattern is recent (< 30 days from now) */
    time_t now = time(NULL);
    time_t newer_timestamp = m1->timestamp > m2->timestamp ? m1->timestamp : m2->timestamp;
    time_t age_days = (now - newer_timestamp) / SECONDS_PER_DAY;

    /* Recent patterns: strict temporal clustering (7 days) */
    if (age_days < TEMPORAL_CLUSTER_RECENT_DAYS) {
        return days_diff < TEMPORAL_WINDOW_RECENT;
    }

    /* Old patterns: looser temporal clustering (30 days) */
    return days_diff < TEMPORAL_WINDOW_OLD;
}

/* Helper: Assign pattern to all members */
static void assign_pattern_to_members(memory_record_t** records, size_t* pattern_members,
                                     size_t member_count, const char* pattern_id) {
    for (size_t m = 0; m < member_count; m++) {
        size_t idx = pattern_members[m];
        records[idx]->pattern_id = strdup(pattern_id);
        records[idx]->pattern_frequency = member_count;
        records[idx]->semantic_similarity = 1.0f;  /* Simplified */
    }
}

/* Helper: Mark standard outliers (first, last, highest importance) */
static void mark_standard_outliers(memory_record_t** records, size_t* pattern_members,
                                  size_t member_count) {
    /* Mark first and last */
    records[pattern_members[0]]->is_pattern_outlier = true;  /* First */
    records[pattern_members[member_count - 1]]->is_pattern_outlier = true;  /* Last */

    /* Find and mark highest importance */
    size_t max_importance_idx = 0;
    float max_importance = 0.0f;
    for (size_t m = 0; m < member_count; m++) {
        size_t idx = pattern_members[m];
        if (records[idx]->importance > max_importance) {
            max_importance = records[idx]->importance;
            max_importance_idx = idx;
        }
    }
    records[max_importance_idx]->is_pattern_outlier = true;  /* Most important */
}

/* Helper: Find and mark emotional outlier (Thane's Phase 4 Priority 5) */
static void find_and_mark_emotional_outlier(memory_record_t** records, size_t* pattern_members,
                                           size_t member_count) {
    float avg_emotion = 0.0f;
    size_t emotion_count = 0;

    /* Calculate average emotion intensity for pattern */
    for (size_t m = 0; m < member_count; m++) {
        size_t idx = pattern_members[m];
        if (records[idx]->emotion_intensity > 0.0f) {
            avg_emotion += records[idx]->emotion_intensity;
            emotion_count++;
        }
    }

    if (emotion_count == 0) {
        return;  /* No emotional data */
    }

    avg_emotion /= emotion_count;

    /* Find member with maximum emotional distance from average */
    float max_emotion_distance = 0.0f;
    size_t max_emotion_idx = 0;

    for (size_t m = 0; m < member_count; m++) {
        size_t idx = pattern_members[m];
        float distance = records[idx]->emotion_intensity > avg_emotion ?
                         records[idx]->emotion_intensity - avg_emotion :
                         avg_emotion - records[idx]->emotion_intensity;

        if (distance > max_emotion_distance) {
            max_emotion_distance = distance;
            max_emotion_idx = idx;
        }
    }

    /* Mark emotional outlier (if distinct enough from average) */
    if (max_emotion_distance > 0.2f) {  /* Significant emotional difference */
        records[max_emotion_idx]->is_pattern_outlier = true;  /* Emotional outlier */
        LOG_DEBUG("Marked emotional outlier (distance=%.2f from avg=%.2f): %.50s...",
                 max_emotion_distance, avg_emotion, records[max_emotion_idx]->content);
    }
}

/* Helper: Add pattern context summary to outliers (Thane's Priority 1) */
static void add_pattern_summary_to_outliers(memory_record_t** records, size_t* pattern_members,
                                           size_t member_count) {
    /* Count outliers (3-4 depending on emotional outlier) */
    size_t outlier_count = 0;
    for (size_t m = 0; m < member_count; m++) {
        size_t idx = pattern_members[m];
        if (records[idx]->is_pattern_outlier) {
            outlier_count++;
        }
    }

    size_t archived_count = member_count - outlier_count;
    char summary[KATRA_BUFFER_LARGE];
    snprintf(summary, sizeof(summary),
            "Pattern: %zu occurrences (%zu archived, %zu preserved as outliers)",
            member_count, archived_count, outlier_count);

    /* Set summary for each outlier */
    for (size_t m = 0; m < member_count; m++) {
        size_t idx = pattern_members[m];
        if (records[idx]->is_pattern_outlier) {
            records[idx]->pattern_summary = strdup(summary);
            if (!records[idx]->pattern_summary) {
                LOG_WARN(KATRA_ERR_ALLOC_FAILED,
                        records[idx]->record_id);
            }
        }
    }
}

/* Detect patterns in memory set (Phase 3)
 *
 * Thane's insight: "I debugged 50 times" → pattern + count + outliers
 * Groups similar memories, marks patterns, preserves outliers.
 */
void katra_tier1_detect_patterns(memory_record_t** records, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (records[i]->pattern_id) {
            continue;  /* Already assigned to pattern */
        }

        /* Find similar memories */
        size_t pattern_members[TIER1_MAX_PATTERN_MEMBERS];  /* Simplified: max 256 pattern members */
        size_t member_count = 0;
        pattern_members[member_count++] = i;

        for (size_t j = i + 1; j < count && member_count < TIER1_MAX_PATTERN_MEMBERS; j++) {
            if (records[j]->pattern_id) {
                continue;
            }

            float similarity = calculate_similarity(records[i]->content, records[j]->content);

            /* Thane's Phase 4 Priority 4: Temporal clustering */
            if (should_cluster(records[i], records[j], similarity)) {
                pattern_members[member_count++] = j;
            }
        }

        /* If enough similar memories, create pattern */
        if (member_count >= MIN_PATTERN_SIZE) {
            /* Generate pattern ID */
            char pattern_id[KATRA_BUFFER_SMALL];
            snprintf(pattern_id, sizeof(pattern_id), "pattern_%zu_%ld",
                    i, (long)records[i]->timestamp);

            /* Assign pattern to all members */
            assign_pattern_to_members(records, pattern_members, member_count, pattern_id);

            /* Mark standard outliers: first, last, highest importance */
            mark_standard_outliers(records, pattern_members, member_count);

            /* Thane's Phase 4 Priority 5: Enhanced outlier selection
             * Add 4th outlier: most emotionally distinct member */
            find_and_mark_emotional_outlier(records, pattern_members, member_count);

            /* Add pattern context summary for outliers (Thane's Priority 1) */
            add_pattern_summary_to_outliers(records, pattern_members, member_count);

            LOG_DEBUG("Detected pattern %s with %zu members", pattern_id, member_count);
        }
    }
}

/* Filter pattern outliers from records
 *
 * Returns: Number of records to archive (excludes pattern outliers)
 */
size_t katra_tier1_filter_pattern_outliers(memory_record_t** records, size_t count) {
    size_t final_count = 0;

    for (size_t i = 0; i < count; i++) {
        if (records[i]->pattern_id && !records[i]->is_pattern_outlier) {
            /* Part of pattern, not an outlier - archive it */
            LOG_DEBUG("Archiving pattern member (pattern=%s, freq=%zu): %.50s...",
                     records[i]->pattern_id, records[i]->pattern_frequency,
                     records[i]->content);
            records[final_count++] = records[i];
        } else if (records[i]->is_pattern_outlier) {
            /* Pattern outlier - preserve it */
            LOG_DEBUG("Preserving pattern outlier (pattern=%s): %.50s...",
                     records[i]->pattern_id, records[i]->content);
            /* Note: In production, we'd write it back to tier1 with updated metadata */
            /* For now, just don't archive it */
            katra_memory_free_record(records[i]);
        } else {
            /* Not part of pattern - archive normally */
            records[final_count++] = records[i];
        }
    }

    return final_count;
}
