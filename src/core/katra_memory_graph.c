/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* Project includes */
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_consent.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_core_common.h"

/* Connection detection thresholds */
#define MIN_KEYWORD_LENGTH 4        /* Minimum keyword length for matching */
#define MIN_SHARED_KEYWORDS 2       /* Minimum shared keywords for connection */
#define MAX_CONNECTIONS_PER_MEMORY 20  /* Limit connections to prevent bloat */
#define CENTRALITY_NORMALIZATION_MIN 5  /* Min connections for normalization */

/* Static function declarations */
static int extract_keywords(const char* text, char*** keywords, size_t* count);
static bool is_stop_word(const char* word);
static size_t count_shared_keywords(char** keywords1, size_t count1,
                                     char** keywords2, size_t count2);
static void free_keywords(char** keywords, size_t count);

/* Build connections for a single memory based on content similarity
 *
 * Analyzes a memory's content and finds connections to other memories based on:
 * - Shared keywords (simple text similarity)
 * - Explicit related_to links (already specified)
 *
 * Updates the memory record's connection_count field (but doesn't store it).
 * Caller is responsible for using this info in consolidation decisions.
 *
 * This is Phase 2 - simple keyword-based matching.
 * Phase 3 would add semantic embeddings for better accuracy.
 */
int katra_memory_build_connections_for_record(memory_record_t* record,
                                                memory_record_t** all_memories,
                                                size_t memory_count) {
    if (!record || !all_memories) {
        katra_report_error(E_INPUT_NULL, "katra_memory_build_connections_for_record",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    /* Extract keywords from this memory's content */
    char** my_keywords = NULL;
    size_t my_keyword_count = 0;
    int result = extract_keywords(record->content, &my_keywords, &my_keyword_count);
    if (result != KATRA_SUCCESS || my_keyword_count == 0) {
        record->connection_count = 0;
        return KATRA_SUCCESS;  /* No keywords = no connections */
    }

    /* Count connections by comparing with other memories */
    size_t connection_count = 0;

    for (size_t i = 0; i < memory_count; i++) {
        memory_record_t* other = all_memories[i];

        /* Skip self */
        if (strcmp(other->record_id, record->record_id) == 0) {
            continue;
        }

        /* Check for explicit related_to link */
        bool explicitly_linked = false;
        if (record->related_to && strcmp(record->related_to, other->record_id) == 0) {
            explicitly_linked = true;
        }
        if (other->related_to && strcmp(other->related_to, record->record_id) == 0) {
            explicitly_linked = true;
        }

        if (explicitly_linked) {
            connection_count++;
            continue;
        }

        /* Check keyword similarity */
        char** other_keywords = NULL;
        size_t other_keyword_count = 0;
        result = extract_keywords(other->content, &other_keywords, &other_keyword_count);
        if (result != KATRA_SUCCESS) {
            continue;  /* Skip this memory if keyword extraction fails */
        }

        size_t shared_count = count_shared_keywords(my_keywords, my_keyword_count,
                                                     other_keywords, other_keyword_count);

        free_keywords(other_keywords, other_keyword_count);

        if (shared_count >= MIN_SHARED_KEYWORDS) {
            connection_count++;
        }
    }

    free_keywords(my_keywords, my_keyword_count);

    /* Update connection count in record */
    record->connection_count = connection_count;

    return KATRA_SUCCESS;
}

/* Extract keywords from text for connection matching
 *
 * Simple approach for Phase 2:
 * - Split on whitespace and punctuation
 * - Keep words >= MIN_KEYWORD_LENGTH
 * - Convert to lowercase
 * - Remove common stop words
 */
static int extract_keywords(const char* text, char*** keywords, size_t* count) {
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
        katra_report_error(E_SYSTEM_MEMORY, "extract_keywords",
                          KATRA_ERR_ALLOC_FAILED);
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
        if (is_stop_word(lowercase)) {
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
                free_keywords(kw_array, kw_count);
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

/* Check if word is a common stop word */
static bool is_stop_word(const char* word) {
    for (int i = 0; KATRA_STOP_WORDS[i] != NULL; i++) {
        if (strcmp(word, KATRA_STOP_WORDS[i]) == 0) {
            return true;
        }
    }
    return false;
}

/* Count shared keywords between two sets */
static size_t count_shared_keywords(char** keywords1, size_t count1,
                                     char** keywords2, size_t count2) {
    size_t shared = 0;
    for (size_t i = 0; i < count1; i++) {
        for (size_t j = 0; j < count2; j++) {
            if (strcmp(keywords1[i], keywords2[j]) == 0) {
                shared++;
                break;
            }
        }
    }
    return shared;
}

/* Free keyword array */
static void free_keywords(char** keywords, size_t count) {
    if (!keywords) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        free(keywords[i]);
    }
    free(keywords);
}

/* Calculate graph centrality for a set of memories
 *
 * Centrality measures how "central" or important a memory is based on
 * how many other memories connect to it. High centrality = hub memory.
 *
 * Algorithm:
 * 1. Build connection counts for all memories
 * 2. Normalize: centrality = connections / max_connections_in_graph
 * 3. Update memory records with centrality scores (in memory)
 *
 * Caller must have already loaded memories and should persist if needed.
 */
int katra_memory_calculate_centrality_for_records(memory_record_t** memories,
                                                    size_t count) {
    if (!memories) {
        katra_report_error(E_INPUT_NULL, "katra_memory_calculate_centrality_for_records",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (count == 0) {
        return KATRA_SUCCESS;  /* No memories to process */
    }

    /* First pass: build connection counts for all memories */
    for (size_t i = 0; i < count; i++) {
        int result = katra_memory_build_connections_for_record(memories[i], memories, count);
        if (result != KATRA_SUCCESS) {
            LOG_WARN("Failed to build connections for memory %s", memories[i]->record_id);
            /* Continue with other memories */
        }
    }

    /* Find max connection count for normalization */
    size_t max_connections = 0;
    for (size_t i = 0; i < count; i++) {
        if (memories[i]->connection_count > max_connections) {
            max_connections = memories[i]->connection_count;
        }
    }

    /* Use minimum threshold if graph is too sparse */
    if (max_connections < CENTRALITY_NORMALIZATION_MIN) {
        max_connections = CENTRALITY_NORMALIZATION_MIN;
    }

    /* Second pass: calculate normalized centrality scores */
    for (size_t i = 0; i < count; i++) {
        memory_record_t* rec = memories[i];

        /* Centrality = connection_count / max_connections */
        rec->graph_centrality = (float)rec->connection_count / (float)max_connections;

        /* Clamp to [0.0, 1.0] */
        if (rec->graph_centrality > 1.0f) {
            rec->graph_centrality = 1.0f;
        }
    }

    LOG_DEBUG("Calculated centrality for %zu memories (max connections: %zu)",
              count, max_connections);

    return KATRA_SUCCESS;
}
