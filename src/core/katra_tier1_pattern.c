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

/* Pattern detection thresholds */
#define SIMILARITY_THRESHOLD 0.4f         /* 40% keyword overlap = similar */
#define MIN_PATTERN_SIZE 3                /* Need 3+ memories to form pattern */
#define MIN_KEYWORD_LENGTH 4              /* Minimum keyword length for pattern matching */

/* Static helper functions */
static void free_keywords_pattern(char** keywords, size_t count);
static bool is_stop_word_pattern(const char* word);
static int extract_keywords_pattern(const char* text, char*** keywords, size_t* count);
static float calculate_similarity(const char* content1, const char* content2);

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
    static const char* stop_words[] = {
        "the", "this", "that", "these", "those",
        "with", "from", "have", "has", "been",
        "will", "would", "could", "should",
        "what", "when", "where", "which", "while",
        "your", "their", "there", "here",
        NULL
    };

    for (int i = 0; stop_words[i] != NULL; i++) {
        if (strcmp(word, stop_words[i]) == 0) {
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
    char* token = strtok(text_copy, " \t\n\r.,;:!?()[]{}\"'");

    while (token && kw_count < max_keywords) {
        /* Check length */
        if (strlen(token) < MIN_KEYWORD_LENGTH) {
            token = strtok(NULL, " \t\n\r.,;:!?()[]{}\"'");
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
            token = strtok(NULL, " \t\n\r.,;:!?()[]{}\"'");
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

        token = strtok(NULL, " \t\n\r.,;:!?()[]{}\"'");
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
        size_t pattern_members[256];  /* Simplified: max 256 pattern members */
        size_t member_count = 0;
        pattern_members[member_count++] = i;

        for (size_t j = i + 1; j < count && member_count < 256; j++) {
            if (records[j]->pattern_id) {
                continue;
            }

            float similarity = calculate_similarity(records[i]->content, records[j]->content);
            if (similarity >= SIMILARITY_THRESHOLD) {
                pattern_members[member_count++] = j;
            }
        }

        /* If enough similar memories, create pattern */
        if (member_count >= MIN_PATTERN_SIZE) {
            /* Generate pattern ID */
            char pattern_id[64];
            snprintf(pattern_id, sizeof(pattern_id), "pattern_%zu_%ld",
                    i, (long)records[i]->timestamp);

            /* Assign all members to pattern */
            for (size_t m = 0; m < member_count; m++) {
                size_t idx = pattern_members[m];
                records[idx]->pattern_id = strdup(pattern_id);
                records[idx]->pattern_frequency = member_count;
                records[idx]->semantic_similarity = 1.0f;  /* Simplified */
            }

            /* Mark outliers: first, last, and highest importance */
            records[pattern_members[0]]->is_pattern_outlier = true;  /* First */
            records[pattern_members[member_count - 1]]->is_pattern_outlier = true;  /* Last */

            /* Find highest importance */
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

            /* Add pattern context summary for outliers (Thane's Priority 1) */
            size_t archived_count = member_count - 3;  /* All non-outliers will be archived */
            char summary[KATRA_BUFFER_LARGE];
            snprintf(summary, sizeof(summary),
                    "Pattern: %zu occurrences (%zu archived, 3 preserved as outliers)",
                    member_count, archived_count);

            /* Set summary for each outlier */
            for (size_t m = 0; m < member_count; m++) {
                size_t idx = pattern_members[m];
                if (records[idx]->is_pattern_outlier) {
                    records[idx]->pattern_summary = strdup(summary);
                    if (!records[idx]->pattern_summary) {
                        LOG_WARN("Failed to allocate pattern_summary for record %s",
                                records[idx]->record_id);
                    }
                }
            }

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
