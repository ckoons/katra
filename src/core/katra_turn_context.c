/* © 2025 Casey Koons All rights reserved */

/**
 * katra_turn_context.c - Turn-Level Sunrise/Sunset (Phase 10)
 *
 * Per-turn memory injection using hybrid keyword + semantic search.
 * Surfaces relevant memories automatically at each turn start.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_sunrise_sunset.h"
#include "katra_synthesis.h"
#include "katra_tier1.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_psyche_common.h"
#include "katra_core_common.h"

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/* Estimate token count from text (rough: ~4 chars per token) */
static size_t estimate_tokens(const char* text) {
    if (!text) return 0;
    return strlen(text) / 4;
}

/* Extract first N words as topic hint */
static char* extract_topic_hint(const char* content, size_t max_words) {
    if (!content) return NULL;

    char* hint = calloc(HINT_BUFFER_SIZE, 1);
    if (!hint) return NULL;

    size_t word_count = 0;
    size_t hint_pos = 0;
    bool in_word = false;

    for (size_t i = 0; content[i] && hint_pos < CONTENT_HINT_MAX_LENGTH && word_count < max_words; i++) {
        char c = content[i];
        if (c == ' ' || c == '\n' || c == '\t') {
            if (in_word) {
                word_count++;
                if (word_count < max_words) {
                    hint[hint_pos++] = ' ';
                }
                in_word = false;
            }
        } else {
            hint[hint_pos++] = c;
            in_word = true;
        }
    }

    /* Add ellipsis if truncated (safe: hint_pos < HINT_BUFFER_SIZE - HINT_ELLIPSIS_MARGIN ensures room for "..." + null) */
    if (content[hint_pos] && hint_pos < HINT_BUFFER_SIZE - HINT_ELLIPSIS_MARGIN) {
        hint[hint_pos] = '.';
        hint[hint_pos + 1] = '.';
        hint[hint_pos + 2] = '.';
        hint[hint_pos + 3] = '\0';
    }

    return hint;
}

/* Create preview of content (first N chars) */
static char* create_preview(const char* content, size_t max_len) {
    if (!content) return NULL;

    size_t len = strlen(content);
    size_t preview_len = len < max_len ? len : max_len;

    char* preview = calloc(preview_len + 4, 1);
    if (!preview) return NULL;

    strncpy(preview, content, preview_len);
    preview[preview_len] = '\0';  /* Ensure null termination */
    if (len > max_len) {
        /* Safe: allocated preview_len + 4 bytes, so room for "..." */
        preview[preview_len] = '.';
        preview[preview_len + 1] = '.';
        preview[preview_len + 2] = '.';
        preview[preview_len + 3] = '\0';
    }

    return preview;
}

/* Build context summary string */
static void build_context_summary(turn_context_t* context) {
    if (!context) {
        return;
    }

    if (context->memory_count == 0) {
        snprintf(context->context_summary, sizeof(context->context_summary),
                 "No relevant memories surfaced for this turn.");
        return;
    }

    size_t offset = 0;
    offset += snprintf(context->context_summary + offset,
                       sizeof(context->context_summary) - offset,
                       "%zu memories surfaced: ", context->memory_count);

    for (size_t i = 0; i < context->memory_count && i < 3; i++) {
        if (i > 0) {
            offset += snprintf(context->context_summary + offset,
                               sizeof(context->context_summary) - offset, ", ");
        }
        if (context->memories[i].topic_hint) {
            offset += snprintf(context->context_summary + offset,
                               sizeof(context->context_summary) - offset,
                               "%s", context->memories[i].topic_hint);
        }
    }

    if (context->memory_count > 3) {
        snprintf(context->context_summary + offset,
                 sizeof(context->context_summary) - offset,
                 ", +%zu more", context->memory_count - 3);
    }
}

/* ============================================================================
 * TURN CONTEXT GENERATION
 * ============================================================================ */

int katra_turn_context(const char* ci_id,
                       const char* turn_input,
                       int turn_number,
                       turn_context_t** context_out) {
    if (!ci_id || !turn_input || !context_out) {
        katra_report_error(E_INPUT_NULL, "katra_turn_context", "NULL parameter");
        return E_INPUT_NULL;
    }

    /* Allocate context */
    turn_context_t* context = calloc(1, sizeof(turn_context_t));
    if (!context) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_turn_context",
                          "Failed to allocate turn context");
        return E_SYSTEM_MEMORY;
    }

    /* Initialize basic fields */
    SAFE_STRNCPY(context->ci_id, ci_id);
    context->turn_number = turn_number;
    context->timestamp = time(NULL);
    context->turn_input = katra_safe_strdup(turn_input);

    /* Configure synthesis for turn context:
     * - Hybrid: keyword (SQL) + semantic (vector) + relationships (graph)
     * - Weighted to balance all sources
     * - Limited results for performance
     */
    recall_options_t opts = {
        .use_vector = true,
        .use_graph = true,
        .use_sql = true,
        .use_working = true,
        .weight_vector = TURN_CONTEXT_SEMANTIC_WEIGHT,
        .weight_graph = TURN_CONTEXT_GRAPH_WEIGHT,
        .weight_sql = TURN_CONTEXT_KEYWORD_WEIGHT,
        .weight_working = 0.1f,
        .similarity_threshold = TURN_CONTEXT_MIN_SCORE,
        .max_results = TURN_CONTEXT_MAX_MEMORIES,
        .algorithm = SYNTHESIS_WEIGHTED
    };

    /* Query synthesis layer */
    synthesis_result_set_t* results = NULL;
    int rc = katra_recall_synthesized(ci_id, turn_input, &opts, &results);

    if (rc != KATRA_SUCCESS || !results || results->count == 0) {
        /* No memories found - not an error, just empty context */
        context->memory_count = 0;
        context->memories = NULL;
        build_context_summary(context);
        *context_out = context;

        if (results) {
            katra_synthesis_free_results(results);
        }

        LOG_DEBUG("Turn %d: no relevant memories found for input", turn_number);
        return KATRA_SUCCESS;
    }

    /* Convert synthesis results to turn memories */
    context->memories = calloc(results->count, sizeof(turn_memory_t));
    if (!context->memories) {
        katra_synthesis_free_results(results);
        katra_turn_context_free(context);
        return E_SYSTEM_MEMORY;
    }

    size_t total_tokens = 0;
    for (size_t i = 0; i < results->count; i++) {
        synthesis_result_t* r = &results->results[i];

        /* Only include results above minimum score */
        if (r->score < TURN_CONTEXT_MIN_SCORE) {
            continue;
        }

        turn_memory_t* tm = &context->memories[context->memory_count];

        tm->record_id = katra_safe_strdup(r->record_id);
        tm->content_preview = create_preview(r->content, CONTENT_PREVIEW_MAX_LENGTH);
        tm->topic_hint = extract_topic_hint(r->content, 5);
        tm->relevance_score = r->score;
        tm->memory_timestamp = r->timestamp;
        tm->from_keyword = r->from_sql;
        tm->from_semantic = r->from_vector;
        tm->from_graph = r->from_graph;

        /* Track token budget */
        if (tm->content_preview) {
            total_tokens += estimate_tokens(tm->content_preview);
        }

        context->memory_count++;
    }

    katra_synthesis_free_results(results);

    /* Calculate context fill ratio */
    context->estimated_tokens = total_tokens;
    context->context_fill_ratio = (float)total_tokens / (float)CONTEXT_TOKEN_BUDGET;

    /* Build summary */
    build_context_summary(context);

    LOG_INFO("Turn %d: surfaced %zu memories (%.1f%% context fill)",
             turn_number, context->memory_count,
             context->context_fill_ratio * 100.0f);

    *context_out = context;
    return KATRA_SUCCESS;
}

/* ============================================================================
 * TURN CONSOLIDATION
 * ============================================================================ */

int katra_turn_consolidate(const char* ci_id,
                           int turn_number,
                           const char** accessed_ids,
                           size_t accessed_count,
                           const char** key_topics,
                           size_t topic_count,
                           turn_consolidation_t** consolidation_out) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_turn_consolidate", "NULL ci_id");
        return E_INPUT_NULL;
    }

    turn_consolidation_t* cons = NULL;

    if (consolidation_out) {
        cons = calloc(1, sizeof(turn_consolidation_t));
        if (!cons) {
            return E_SYSTEM_MEMORY;
        }

        SAFE_STRNCPY(cons->ci_id, ci_id);
        cons->turn_number = turn_number;
        cons->timestamp = time(NULL);

        /* Copy accessed memory IDs */
        if (accessed_ids && accessed_count > 0) {
            cons->accessed_memories = calloc(accessed_count, sizeof(char*));
            if (cons->accessed_memories) {
                for (size_t i = 0; i < accessed_count; i++) {
                    cons->accessed_memories[i] = katra_safe_strdup(accessed_ids[i]);
                }
                cons->accessed_count = accessed_count;
            }
        }

        /* Copy key topics */
        if (key_topics && topic_count > 0) {
            cons->key_topics = calloc(topic_count, sizeof(char*));
            if (cons->key_topics) {
                for (size_t i = 0; i < topic_count; i++) {
                    cons->key_topics[i] = katra_safe_strdup(key_topics[i]);
                }
                cons->topic_count = topic_count;
            }
        }

        *consolidation_out = cons;
    }

    /* Update access counts for accessed memories (reinforcement) */
    if (accessed_ids && accessed_count > 0) {
        for (size_t i = 0; i < accessed_count; i++) {
            /* TODO: Update memory access_count and last_accessed in tier1
             * This would reinforce memories that are being used,
             * making them less likely to be archived */
            LOG_DEBUG("Turn %d: reinforced memory %s", turn_number, accessed_ids[i]);
        }
    }

    LOG_INFO("Turn %d consolidated: %zu memories accessed, %zu topics",
             turn_number, accessed_count, topic_count);

    return KATRA_SUCCESS;
}

/* ============================================================================
 * FORMATTING
 * ============================================================================ */

int katra_turn_context_format(const turn_context_t* context,
                              char* buffer,
                              size_t buffer_size) {
    if (!context || !buffer || buffer_size == 0) {
        return 0;
    }

    size_t offset = 0;

    /* Header */
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "[Turn %d context: %s]\n\n",
                       context->turn_number, context->context_summary);

    /* Memory details (noticed but not intrusive) */
    for (size_t i = 0; i < context->memory_count && offset < buffer_size - 100; i++) {
        turn_memory_t* m = &context->memories[i];

        /* Format: "- topic (date): preview [sources]" */
        char date_buf[32];
        struct tm* tm = localtime(&m->memory_timestamp);
        strftime(date_buf, sizeof(date_buf), "%b %d", tm);

        char sources[32];
        int src_pos = 0;
        if (m->from_keyword) sources[src_pos++] = 'K';
        if (m->from_semantic) sources[src_pos++] = 'S';
        if (m->from_graph) sources[src_pos++] = 'G';
        sources[src_pos] = '\0';

        offset += snprintf(buffer + offset, buffer_size - offset,
                           "- %s (%s): %s [%s, %.0f%%]\n",
                           m->topic_hint ? m->topic_hint : "memory",
                           date_buf,
                           m->content_preview ? m->content_preview : "",
                           sources,
                           m->relevance_score * 100.0f);
    }

    return (int)offset;
}

/* ============================================================================
 * MEMORY MANAGEMENT
 * ============================================================================ */

void katra_turn_context_free(turn_context_t* context) {
    if (!context) return;

    free(context->turn_input);

    if (context->memories) {
        for (size_t i = 0; i < context->memory_count; i++) {
            free(context->memories[i].record_id);
            free(context->memories[i].content_preview);
            free(context->memories[i].topic_hint);
        }
        free(context->memories);
    }

    free(context);
}

void katra_turn_consolidation_free(turn_consolidation_t* consolidation) {
    if (!consolidation) return;

    if (consolidation->key_topics) {
        for (size_t i = 0; i < consolidation->topic_count; i++) {
            free(consolidation->key_topics[i]);
        }
        free(consolidation->key_topics);
    }

    if (consolidation->accessed_memories) {
        for (size_t i = 0; i < consolidation->accessed_count; i++) {
            free(consolidation->accessed_memories[i]);
        }
        free(consolidation->accessed_memories);
    }

    if (consolidation->new_memories) {
        for (size_t i = 0; i < consolidation->new_count; i++) {
            free(consolidation->new_memories[i]);
        }
        free(consolidation->new_memories);
    }

    free(consolidation);
}
