/* © 2025 Casey Koons All rights reserved */

/* TF-IDF (Term Frequency-Inverse Document Frequency) embeddings - Phase 6.1b */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* Project includes */
#include "katra_vector.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Token structure for TF-IDF calculation */
typedef struct {
    char text[TFIDF_MAX_TOKEN_LENGTH];
    int frequency;
} token_t;

/* Document statistics for IDF calculation */
typedef struct {
    char** vocabulary;      /* Unique terms across all documents */
    int* doc_frequencies;   /* How many docs contain each term */
    size_t vocab_size;
    size_t total_docs;
} idf_stats_t;

/* Global IDF statistics (shared across all embeddings) */
static idf_stats_t g_idf_stats = {NULL, NULL, 0, 0};

/* Tokenize text into words */
static int tokenize_text(const char* text, token_t* tokens, size_t* count_out) {
    if (!text || !tokens || !count_out) {
        return E_INPUT_NULL;
    }

    size_t count = 0;
    const char* ptr = text;
    char current_token[TFIDF_MAX_TOKEN_LENGTH];
    size_t token_len = 0;

    while (*ptr && count < TFIDF_MAX_TOKENS) {
        /* Skip whitespace and punctuation */
        while (*ptr && !isalnum((unsigned char)*ptr)) {
            ptr++;
        }

        if (!*ptr) {
            break;
        }

        /* Extract word */
        token_len = 0;
        while (*ptr && isalnum((unsigned char)*ptr) && token_len < TFIDF_MAX_TOKEN_LENGTH - 1) {
            current_token[token_len++] = tolower((unsigned char)*ptr);
            ptr++;
        }
        current_token[token_len] = '\0';

        if (token_len == 0) {
            continue;
        }

        /* Skip very short or very long tokens */
        if (token_len < TFIDF_MIN_TOKEN_LEN || token_len > TFIDF_MAX_TOKEN_LEN) {
            continue;
        }

        /* Check if token already exists */
        int found = 0;
        for (size_t i = 0; i < count; i++) {
            if (strcmp(tokens[i].text, current_token) == 0) {
                tokens[i].frequency++;
                found = 1;
                break;
            }
        }

        if (!found && count < TFIDF_MAX_TOKENS) {
            strncpy(tokens[count].text, current_token, TFIDF_MAX_TOKEN_LENGTH - 1);
            tokens[count].text[TFIDF_MAX_TOKEN_LENGTH - 1] = '\0';
            tokens[count].frequency = 1;
            count++;
        }
    }

    *count_out = count;
    return KATRA_SUCCESS;
}

/* Update IDF statistics with new document */
int katra_vector_tfidf_update_stats(const char* text) {
    if (!text) {
        return E_INPUT_NULL;
    }

    /* Tokenize document */
    token_t* tokens = calloc(TFIDF_MAX_TOKENS, sizeof(token_t));
    if (!tokens) {
        return E_SYSTEM_MEMORY;
    }

    size_t token_count = 0;
    int result = tokenize_text(text, tokens, &token_count);
    if (result != KATRA_SUCCESS) {
        free(tokens);
        return result;
    }

    /* Update vocabulary and document frequencies */
    for (size_t i = 0; i < token_count; i++) {
        /* Check if term exists in vocabulary */
        int found = -1;
        for (size_t j = 0; j < g_idf_stats.vocab_size; j++) {
            if (strcmp(g_idf_stats.vocabulary[j], tokens[i].text) == 0) {
                found = (int)j;
                break;
            }
        }

        if (found >= 0) {
            /* Term exists - increment document frequency */
            g_idf_stats.doc_frequencies[found]++;
        } else {
            /* New term - add to vocabulary */
            size_t new_size = g_idf_stats.vocab_size + 1;

            char** new_vocab = realloc(g_idf_stats.vocabulary, new_size * sizeof(char*));
            if (!new_vocab) {
                free(tokens);
                return E_SYSTEM_MEMORY;
            }
            g_idf_stats.vocabulary = new_vocab;

            int* new_freqs = realloc(g_idf_stats.doc_frequencies, new_size * sizeof(int));
            if (!new_freqs) {
                free(tokens);
                return E_SYSTEM_MEMORY;
            }
            g_idf_stats.doc_frequencies = new_freqs;

            /* Add new term */
            g_idf_stats.vocabulary[g_idf_stats.vocab_size] = strdup(tokens[i].text);
            if (!g_idf_stats.vocabulary[g_idf_stats.vocab_size]) {
                free(tokens);
                return E_SYSTEM_MEMORY;
            }
            g_idf_stats.doc_frequencies[g_idf_stats.vocab_size] = 1;
            g_idf_stats.vocab_size++;
        }
    }

    g_idf_stats.total_docs++;
    free(tokens);

    LOG_DEBUG("Updated IDF stats: %zu terms, %zu docs",
             g_idf_stats.vocab_size, g_idf_stats.total_docs);

    return KATRA_SUCCESS;
}

/* Calculate TF-IDF vector for text */
int katra_vector_tfidf_create(const char* text, vector_embedding_t** embedding_out) {
    if (!text || !embedding_out) {
        return E_INPUT_NULL;
    }

    /* Tokenize text */
    token_t* tokens = calloc(TFIDF_MAX_TOKENS, sizeof(token_t));
    if (!tokens) {
        return E_SYSTEM_MEMORY;
    }

    size_t token_count = 0;
    int result = tokenize_text(text, tokens, &token_count);
    if (result != KATRA_SUCCESS) {
        free(tokens);
        return result;
    }

    /* Create embedding structure */
    vector_embedding_t* embedding = calloc(1, sizeof(vector_embedding_t));
    if (!embedding) {
        free(tokens);
        return E_SYSTEM_MEMORY;
    }

    embedding->dimensions = VECTOR_DIMENSIONS;
    embedding->values = calloc(VECTOR_DIMENSIONS, sizeof(float));
    if (!embedding->values) {
        free(embedding);
        free(tokens);
        return E_SYSTEM_MEMORY;
    }

    /* Calculate total terms in document */
    int total_terms = 0;
    for (size_t i = 0; i < token_count; i++) {
        total_terms += tokens[i].frequency;
    }

    if (total_terms == 0) {
        /* Empty document - return zero vector */
        embedding->magnitude = 0.0f;
        *embedding_out = embedding;
        free(tokens);
        return KATRA_SUCCESS;
    }

    /* Calculate TF-IDF for each term */
    for (size_t i = 0; i < token_count; i++) {
        /* Find term in vocabulary */
        int vocab_idx = -1;
        for (size_t j = 0; j < g_idf_stats.vocab_size; j++) {
            if (strcmp(g_idf_stats.vocabulary[j], tokens[i].text) == 0) {
                vocab_idx = (int)j;
                break;
            }
        }

        if (vocab_idx < 0) {
            /* Term not in vocabulary - skip */
            continue;
        }

        /* Calculate TF (Term Frequency) */
        float tf = (float)tokens[i].frequency / (float)total_terms;

        /* Calculate IDF (Inverse Document Frequency) */
        float idf = 0.0f;
        if (g_idf_stats.total_docs > 0 && g_idf_stats.doc_frequencies[vocab_idx] > 0) {
            idf = logf((float)g_idf_stats.total_docs /
                      (float)g_idf_stats.doc_frequencies[vocab_idx]);
        }

        /* TF-IDF score */
        float tfidf = tf * idf;

        /* Map to vector dimension (use hash of term) */
        unsigned int hash = 0;
        for (const char* p = tokens[i].text; *p; p++) {
            hash = hash * 31 + (unsigned char)*p;
        }
        size_t dim = hash % VECTOR_DIMENSIONS;

        /* Add TF-IDF score to dimension */
        embedding->values[dim] += tfidf;

        /* Add to neighboring dimensions for smoother distribution */
        if (dim > 0) {
            embedding->values[dim - 1] += tfidf * 0.5f;
        }
        if (dim < VECTOR_DIMENSIONS - 1) {
            embedding->values[dim + 1] += tfidf * 0.5f;
        }
    }

    /* Normalize vector (L2 norm) */
    float magnitude = 0.0f;
    for (size_t i = 0; i < VECTOR_DIMENSIONS; i++) {
        magnitude += embedding->values[i] * embedding->values[i];
    }
    embedding->magnitude = sqrtf(magnitude);

    if (embedding->magnitude > 0.0f) {
        for (size_t i = 0; i < VECTOR_DIMENSIONS; i++) {
            embedding->values[i] /= embedding->magnitude;
        }
        /* Recalculate magnitude after normalization (should be 1.0) */
        embedding->magnitude = 1.0f;
    }

    *embedding_out = embedding;
    free(tokens);

    LOG_DEBUG("Created TF-IDF embedding: %zu tokens, magnitude: %.3f",
             token_count, embedding->magnitude);

    return KATRA_SUCCESS;
}

/* Get IDF statistics (for debugging/monitoring) */
int katra_vector_tfidf_get_stats(size_t* vocab_size_out, size_t* total_docs_out) {
    if (!vocab_size_out || !total_docs_out) {
        return E_INPUT_NULL;
    }

    *vocab_size_out = g_idf_stats.vocab_size;
    *total_docs_out = g_idf_stats.total_docs;

    return KATRA_SUCCESS;
}

/* Cleanup TF-IDF statistics */
void katra_vector_tfidf_cleanup(void) {
    if (g_idf_stats.vocabulary) {
        for (size_t i = 0; i < g_idf_stats.vocab_size; i++) {
            free(g_idf_stats.vocabulary[i]);
        }
        free(g_idf_stats.vocabulary);
        g_idf_stats.vocabulary = NULL;
    }

    free(g_idf_stats.doc_frequencies);
    g_idf_stats.doc_frequencies = NULL;

    g_idf_stats.vocab_size = 0;
    g_idf_stats.total_docs = 0;

    LOG_DEBUG("TF-IDF statistics cleaned up");
}
