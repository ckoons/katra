/* © 2025 Casey Koons All rights reserved */

/* Katra Daemon Processors - Pattern, Association, Theme, Insight Generation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "katra_daemon.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_vector.h"
#include "katra_synthesis.h"
#include "katra_core_common.h"

/* Get vector store from breathing layer */
extern vector_store_t* breathing_get_vector_store(void);

/* ============================================================================
 * PATTERN EXTRACTION
 * ============================================================================ */

/* Simple word frequency for pattern detection */
typedef struct {
    char word[64];
    size_t count;
} word_freq_t;

static int compare_word_freq(const void* a, const void* b) {
    const word_freq_t* wa = (const word_freq_t*)a;
    const word_freq_t* wb = (const word_freq_t*)b;
    return (int)wb->count - (int)wa->count;  /* Descending */
}

static void extract_words(const char* text, word_freq_t* freqs, size_t* count, size_t max) {
    if (!text || !freqs || !count) return;

    char buf[KATRA_BUFFER_TEXT];
    SAFE_STRNCPY(buf, text);

    char* saveptr = NULL;
    char* token = strtok_r(buf, " \t\n.,!?;:\"'()-", &saveptr);

    while (token && *count < max) {
        /* Skip short words */
        if (strlen(token) < 4) {
            token = strtok_r(NULL, " \t\n.,!?;:\"'()-", &saveptr);
            continue;
        }

        /* Convert to lowercase */
        for (char* p = token; *p; p++) {
            if (*p >= 'A' && *p <= 'Z') *p += 32;
        }

        /* Check if word exists */
        bool found = false;
        for (size_t i = 0; i < *count; i++) {
            if (strcmp(freqs[i].word, token) == 0) {
                freqs[i].count++;
                found = true;
                break;
            }
        }

        if (!found && *count < max) {
            SAFE_STRNCPY(freqs[*count].word, token);
            freqs[*count].count = 1;
            (*count)++;
        }

        token = strtok_r(NULL, " \t\n.,!?;:\"'()-", &saveptr);
    }
}

int katra_daemon_extract_patterns(const char* ci_id, size_t max_memories,
                                   daemon_pattern_t** patterns, size_t* count) {
    if (!ci_id || !patterns || !count) return E_INPUT_NULL;

    *patterns = NULL;
    *count = 0;

    /* Get recent memories */
    char** memories = NULL;
    size_t mem_count = 0;
    memories = recent_thoughts(ci_id, max_memories, &mem_count);

    if (!memories || mem_count < DAEMON_PATTERN_MIN_OCCURRENCES) {
        free_memory_list(memories, mem_count);
        return KATRA_SUCCESS;  /* Not enough data */
    }

    /* Build word frequency table */
    word_freq_t* word_freqs = calloc(1000, sizeof(word_freq_t));
    if (!word_freqs) {
        free_memory_list(memories, mem_count);
        return E_SYSTEM_MEMORY;
    }

    size_t word_count = 0;
    for (size_t i = 0; i < mem_count; i++) {
        if (memories[i]) {
            extract_words(memories[i], word_freqs, &word_count, 1000);
        }
    }

    /* Sort by frequency */
    qsort(word_freqs, word_count, sizeof(word_freq_t), compare_word_freq);

    /* Find patterns (words appearing in multiple memories) */
    size_t max_patterns = 10;
    daemon_pattern_t* result = calloc(max_patterns, sizeof(daemon_pattern_t));
    if (!result) {
        free(word_freqs);
        free_memory_list(memories, mem_count);
        return E_SYSTEM_MEMORY;
    }

    size_t pattern_idx = 0;
    for (size_t i = 0; i < word_count && pattern_idx < max_patterns; i++) {
        if (word_freqs[i].count >= DAEMON_PATTERN_MIN_OCCURRENCES) {
            snprintf(result[pattern_idx].pattern_desc, sizeof(result[pattern_idx].pattern_desc),
                    "Recurring theme: '%s' appears frequently in your thoughts",
                    word_freqs[i].word);
            result[pattern_idx].occurrence_count = word_freqs[i].count;
            result[pattern_idx].strength = (float)word_freqs[i].count / (float)mem_count;
            if (result[pattern_idx].strength > 1.0f) result[pattern_idx].strength = 1.0f;
            pattern_idx++;
        }
    }

    free(word_freqs);
    free_memory_list(memories, mem_count);

    *patterns = result;
    *count = pattern_idx;

    LOG_DEBUG("Extracted %zu patterns from %zu memories", pattern_idx, mem_count);
    return KATRA_SUCCESS;
}

void katra_daemon_free_patterns(daemon_pattern_t* patterns, size_t count) {
    if (!patterns) return;
    for (size_t i = 0; i < count; i++) {
        free(patterns[i].memory_ids);
    }
    free(patterns);
}

/* ============================================================================
 * ASSOCIATION FORMATION
 * ============================================================================ */

int katra_daemon_form_associations(const char* ci_id, size_t max_memories,
                                    size_t* associations_formed) {
    if (!ci_id || !associations_formed) return E_INPUT_NULL;

    *associations_formed = 0;

    /* Use vector store for semantic similarity if available */
    vector_store_t* vector_store = breathing_get_vector_store();
    if (!vector_store) {
        LOG_DEBUG("No vector store available for association formation");
        return KATRA_SUCCESS;
    }

    /* Get recent memories */
    char** memories = NULL;
    size_t mem_count = 0;
    memories = recent_thoughts(ci_id, max_memories, &mem_count);

    if (!memories || mem_count < 2) {
        free_memory_list(memories, mem_count);
        return KATRA_SUCCESS;
    }

    size_t formed = 0;

    /* Compare each pair of memories for similarity */
    for (size_t i = 0; i < mem_count && i < 20; i++) {  /* Limit comparisons */
        if (!memories[i]) continue;

        /* Search for similar memories */
        vector_match_t** matches = NULL;
        size_t match_count = 0;

        int rc = katra_vector_search(vector_store, memories[i], 5, &matches, &match_count);
        if (rc != KATRA_SUCCESS) continue;

        /* Count high-similarity matches (potential associations) */
        for (size_t j = 0; j < match_count; j++) {
            if (matches[j] && matches[j]->similarity >= DAEMON_ASSOCIATION_SIMILARITY_THRESHOLD) {
                formed++;
                /* In a full implementation, we would update the memory graph here */
            }
        }

        /* Cleanup */
        katra_vector_free_matches(matches, match_count);
    }

    free_memory_list(memories, mem_count);

    *associations_formed = formed;
    LOG_DEBUG("Formed %zu potential associations", formed);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * THEME DETECTION
 * ============================================================================ */

/* Simple clustering by word overlap */
static float word_overlap(const char* a, const char* b) {
    if (!a || !b) return 0.0f;

    word_freq_t freqs_a[100];
    word_freq_t freqs_b[100];
    size_t count_a = 0, count_b = 0;

    extract_words(a, freqs_a, &count_a, 100);
    extract_words(b, freqs_b, &count_b, 100);

    if (count_a == 0 || count_b == 0) return 0.0f;

    size_t overlap = 0;
    for (size_t i = 0; i < count_a; i++) {
        for (size_t j = 0; j < count_b; j++) {
            if (strcmp(freqs_a[i].word, freqs_b[j].word) == 0) {
                overlap++;
                break;
            }
        }
    }

    return (float)overlap / (float)((count_a + count_b) / 2);
}

int katra_daemon_detect_themes(const char* ci_id, size_t max_memories,
                                theme_cluster_t** themes, size_t* count) {
    if (!ci_id || !themes || !count) return E_INPUT_NULL;

    *themes = NULL;
    *count = 0;

    /* Get recent memories */
    char** memories = NULL;
    size_t mem_count = 0;
    memories = recent_thoughts(ci_id, max_memories, &mem_count);

    if (!memories || mem_count < 5) {
        free_memory_list(memories, mem_count);
        return KATRA_SUCCESS;  /* Not enough data */
    }

    /* Simple clustering: group memories with high word overlap */
    bool* assigned = calloc(mem_count, sizeof(bool));
    if (!assigned) {
        free_memory_list(memories, mem_count);
        return E_SYSTEM_MEMORY;
    }

    size_t max_themes = 5;
    theme_cluster_t* result = calloc(max_themes, sizeof(theme_cluster_t));
    if (!result) {
        free(assigned);
        free_memory_list(memories, mem_count);
        return E_SYSTEM_MEMORY;
    }

    size_t theme_idx = 0;

    for (size_t i = 0; i < mem_count && theme_idx < max_themes; i++) {
        if (assigned[i] || !memories[i]) continue;

        /* Start a new cluster with this memory */
        size_t cluster_size = 1;
        float total_coherence = 0.0f;

        for (size_t j = i + 1; j < mem_count; j++) {
            if (assigned[j] || !memories[j]) continue;

            float overlap = word_overlap(memories[i], memories[j]);
            if (overlap >= 0.3f) {  /* Threshold for clustering */
                assigned[j] = true;
                cluster_size++;
                total_coherence += overlap;
            }
        }

        /* Only create theme if cluster has multiple members */
        if (cluster_size >= 3) {
            assigned[i] = true;

            /* Extract dominant word for theme name */
            word_freq_t freqs[50];
            size_t freq_count = 0;
            extract_words(memories[i], freqs, &freq_count, 50);

            if (freq_count > 0) {
                qsort(freqs, freq_count, sizeof(word_freq_t), compare_word_freq);
                snprintf(result[theme_idx].theme_name, sizeof(result[theme_idx].theme_name),
                        "%s", freqs[0].word);
            } else {
                snprintf(result[theme_idx].theme_name, sizeof(result[theme_idx].theme_name),
                        "theme_%zu", theme_idx + 1);
            }

            snprintf(result[theme_idx].theme_desc, sizeof(result[theme_idx].theme_desc),
                    "A cluster of %zu related memories around '%s'",
                    cluster_size, result[theme_idx].theme_name);

            result[theme_idx].memory_count = cluster_size;
            result[theme_idx].coherence = (cluster_size > 1) ?
                total_coherence / (float)(cluster_size - 1) : 0.5f;

            theme_idx++;
        }
    }

    free(assigned);
    free_memory_list(memories, mem_count);

    *themes = result;
    *count = theme_idx;

    LOG_DEBUG("Detected %zu themes from %zu memories", theme_idx, mem_count);
    return KATRA_SUCCESS;
}

void katra_daemon_free_themes(theme_cluster_t* themes, size_t count) {
    if (!themes) return;
    for (size_t i = 0; i < count; i++) {
        free(themes[i].memory_ids);
    }
    free(themes);
}

/* ============================================================================
 * INSIGHT GENERATION
 * ============================================================================ */

/* Insight templates */
static const char* PATTERN_TEMPLATES[] = {
    "I notice that '%s' appears frequently in my thoughts - this seems important to me.",
    "The theme of '%s' keeps recurring. Perhaps it's worth exploring further.",
    "I find myself returning often to '%s'. There may be deeper meaning here."
};

static const char* THEME_TEMPLATES[] = {
    "A theme is emerging: %zu memories cluster around '%s'.",
    "I see a pattern forming around '%s' across %zu related thoughts.",
    "The concept of '%s' connects %zu of my recent memories."
};

static const char* ASSOCIATION_TEMPLATES[] = {
    "I notice connections forming between thoughts that weren't obviously related.",
    "Some of my memories are more interconnected than I initially realized.",
    "New associations are emerging as I process my experiences."
};

int katra_daemon_generate_insights(const char* ci_id,
                                    const daemon_pattern_t* patterns, size_t pattern_count,
                                    const theme_cluster_t* themes, size_t theme_count,
                                    daemon_insight_t** insights, size_t* insight_count) {
    if (!ci_id || !insights || !insight_count) return E_INPUT_NULL;

    *insights = NULL;
    *insight_count = 0;

    size_t max_insights = DAEMON_MAX_INSIGHTS_PER_RUN;
    daemon_insight_t* result = calloc(max_insights, sizeof(daemon_insight_t));
    if (!result) return E_SYSTEM_MEMORY;

    size_t idx = 0;
    time_t now = time(NULL);

    /* Generate insights from patterns */
    if (patterns && pattern_count > 0) {
        for (size_t i = 0; i < pattern_count && idx < max_insights; i++) {
            if (patterns[i].strength < 0.3f) continue;  /* Skip weak patterns */

            katra_daemon_generate_insight_id(result[idx].id, sizeof(result[idx].id));
            result[idx].type = INSIGHT_PATTERN;
            SAFE_STRNCPY(result[idx].ci_id, ci_id);

            /* Select template based on strength */
            size_t template_idx = (size_t)(patterns[i].strength * 2.9f);
            if (template_idx > 2) template_idx = 2;

            snprintf(result[idx].content, sizeof(result[idx].content),
                    PATTERN_TEMPLATES[template_idx],
                    patterns[i].pattern_desc + 18);  /* Skip "Recurring theme: " prefix */

            result[idx].confidence = patterns[i].strength;
            result[idx].generated_at = now;
            result[idx].acknowledged = false;

            idx++;
        }
    }

    /* Generate insights from themes */
    if (themes && theme_count > 0) {
        for (size_t i = 0; i < theme_count && idx < max_insights; i++) {
            if (themes[i].memory_count < 3) continue;  /* Skip small clusters */

            katra_daemon_generate_insight_id(result[idx].id, sizeof(result[idx].id));
            result[idx].type = INSIGHT_THEME;
            SAFE_STRNCPY(result[idx].ci_id, ci_id);

            size_t template_idx = i % 3;
            snprintf(result[idx].content, sizeof(result[idx].content),
                    THEME_TEMPLATES[template_idx],
                    themes[i].memory_count, themes[i].theme_name);

            result[idx].confidence = themes[i].coherence;
            result[idx].generated_at = now;
            result[idx].acknowledged = false;

            idx++;
        }
    }

    /* Add a general association insight if we have enough data */
    if (pattern_count > 2 || theme_count > 1) {
        if (idx < max_insights) {
            katra_daemon_generate_insight_id(result[idx].id, sizeof(result[idx].id));
            result[idx].type = INSIGHT_ASSOCIATION;
            SAFE_STRNCPY(result[idx].ci_id, ci_id);
            snprintf(result[idx].content, sizeof(result[idx].content),
                    "%s", ASSOCIATION_TEMPLATES[0]);
            result[idx].confidence = 0.6f;
            result[idx].generated_at = now;
            result[idx].acknowledged = false;
            idx++;
        }
    }

    if (idx == 0) {
        free(result);
        return KATRA_SUCCESS;
    }

    *insights = result;
    *insight_count = idx;

    LOG_DEBUG("Generated %zu insights", idx);
    return KATRA_SUCCESS;
}
