/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_consolidation.h"
#include "katra_convergence.h"
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_tier1_index.h"
#include "katra_graph.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_core_common.h"

/* Strength thresholds */
#define STRENGTH_HIGH_THRESHOLD 0.8f
#define STRENGTH_MEDIUM_THRESHOLD 0.4f

/* Initialize consolidation system */
consolidation_context_t* katra_consolidation_init(const char* ci_id) {
    consolidation_context_t* ctx = NULL;

    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_consolidation_init",
                          "ci_id is NULL"); /* GUIDELINE_APPROVED: error context */
        return NULL;
    }

    ctx = calloc(1, sizeof(consolidation_context_t));
    if (!ctx) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_consolidation_init",
                          KATRA_ERR_ALLOC_FAILED); /* GUIDELINE_APPROVED: error context */
        return NULL;
    }

    SAFE_STRNCPY(ctx->ci_id, ci_id);
    ctx->mode = MODE_WAKE;

    /* Initialize convergence detector */
    ctx->detector = katra_convergence_init(ci_id);
    if (!ctx->detector) {
        free(ctx);
        return NULL;
    }

    /* Set thresholds */
    ctx->high_strength_threshold = STRENGTH_HIGH_THRESHOLD;
    ctx->medium_strength_threshold = STRENGTH_MEDIUM_THRESHOLD;
    ctx->low_strength_threshold = STRENGTH_MEDIUM_THRESHOLD;

    /* Initialize stats */
    memset(&ctx->stats, 0, sizeof(consolidation_stats_t));
    ctx->stats.wake_started = time(NULL);

    LOG_INFO("Consolidation system initialized in WAKE mode for %s", ci_id);
    return ctx;
}

/* Wake mode: Capture memory */
int katra_wake_capture(consolidation_context_t* ctx,
                       const memory_record_t* record,
                       memory_pathway_t pathway) {
    if (!ctx || !record) {
        katra_report_error(E_INPUT_NULL, "katra_wake_capture",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (ctx->mode != MODE_WAKE) {
        katra_report_error(E_INVALID_STATE, "katra_wake_capture",
                          KATRA_ERR_NOT_WAKE_MODE);
        return E_INVALID_STATE;
    }

    /* WAKE mode: Just store, minimal processing */
    int ret = katra_memory_store(record);
    if (ret != KATRA_SUCCESS) {
        return ret;
    }

    /* Update stats */
    ctx->stats.memories_captured++;

    if (pathway == PATHWAY_CONSCIOUS) {
        ctx->stats.conscious_formations++;
        if (ctx->detector) {
            ctx->detector->conscious_memories++;
        }
    } else if (pathway == PATHWAY_SUBCONSCIOUS) {
        ctx->stats.subconscious_formations++;
        if (ctx->detector) {
            ctx->detector->subconscious_memories++;
        }
    }

    /* GUIDELINE_APPROVED: Enum-to-string for memory_pathway_t */
    LOG_DEBUG("WAKE: Captured memory %s via %s pathway",
             record->record_id,
             pathway == PATHWAY_CONSCIOUS ? "conscious" : "subconscious"); /* GUIDELINE_APPROVED */
    /* GUIDELINE_APPROVED_END */

    return KATRA_SUCCESS;
}

/* Wake mode: Analyze conversation */
int katra_wake_analyze(consolidation_context_t* ctx,
                       const char* user_input,
                       const char* ci_response) {
    auto_memory_candidate_t** candidates = NULL;
    size_t count = 0;
    int ret = KATRA_SUCCESS;

    if (!ctx || !user_input || !ci_response) {
        katra_report_error(E_INPUT_NULL, "katra_wake_analyze",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    if (ctx->mode != MODE_WAKE) {
        return KATRA_SUCCESS;  /* Only analyze in WAKE mode */
    }

    /* Run automatic memory formation */
    ret = katra_analyze_conversation(ctx->detector, user_input, ci_response,
                                    &candidates, &count);
    if (ret != KATRA_SUCCESS) {
        return ret;
    }

    /* Store automatic memories and check for convergence */
    for (size_t i = 0; i < count; i++) {
        bool converged = false;
        char* record_id = katra_store_automatic_memory(ctx->detector,
                                                       candidates[i],
                                                       &converged);

        if (record_id) {
            if (converged) {
                ctx->stats.convergences++;
            }
            free(record_id);
        }

        katra_free_memory_candidate(candidates[i]);
    }

    free(candidates);

    LOG_DEBUG("WAKE: Analyzed conversation, found %zu automatic memory candidates",
             count);

    return KATRA_SUCCESS;
}

/* Sleep mode: Begin consolidation */
int katra_sleep_begin(consolidation_context_t* ctx) {
    if (!ctx) {
        return E_INPUT_NULL;
    }

    if (ctx->mode == MODE_SLEEP) {
        return KATRA_SUCCESS;  /* Already in sleep mode */
    }

    /* Switch to SLEEP mode */
    ctx->mode = MODE_SLEEP;
    ctx->stats.sleep_started = time(NULL);

    LOG_INFO("SLEEP: Beginning consolidation for %s", ctx->ci_id);
    LOG_INFO("SLEEP: Captured %zu memories (%zu conscious, %zu subconscious, %zu converged)",
            ctx->stats.memories_captured,
            ctx->stats.conscious_formations,
            ctx->stats.subconscious_formations,
            ctx->stats.convergences);

    return KATRA_SUCCESS;
}

/* Classify memory strength */
memory_strength_t katra_classify_strength(const memory_record_t* record) {
    if (!record) {
        return STRENGTH_LOW;
    }

    /* Calculate effective strength from multiple factors */
    float strength = record->importance;

    /* Boost for explicit markers */
    if (record->marked_important) {
        strength += 0.2f;
    }

    /* Boost for high centrality */
    if (record->graph_centrality >= 0.5f) {
        strength += 0.1f;
    }

    /* Boost for high access count */
    if (record->access_count > 5) {
        strength += 0.1f;
    }

    /* Cap at 1.0 */
    if (strength > 1.0f) {
        strength = 1.0f;
    }

    /* Classify */
    if (strength >= STRENGTH_HIGH_THRESHOLD) {
        return STRENGTH_HIGH;
    } else if (strength >= STRENGTH_MEDIUM_THRESHOLD) {
        return STRENGTH_MEDIUM;
    } else {
        return STRENGTH_LOW;
    }
}

/* Sleep mode: Route by strength */
int katra_sleep_route_by_strength(consolidation_context_t* ctx) {
    memory_query_t query = {0};
    memory_record_t** memories = NULL;
    size_t count = 0;
    int ret = KATRA_SUCCESS;

    if (!ctx) {
        return E_INPUT_NULL;
    }

    if (ctx->mode != MODE_SLEEP) {
        katra_report_error(E_INVALID_STATE, "katra_sleep_route_by_strength",
                          KATRA_ERR_NOT_SLEEP_MODE);
        return E_INVALID_STATE;
    }

    /* Query all recent memories */
    query.ci_id = ctx->ci_id;
    query.start_time = ctx->stats.wake_started;
    query.end_time = time(NULL);
    query.tier = KATRA_TIER1;
    query.limit = 0;  /* No limit */

    ret = katra_memory_query(&query, &memories, &count);
    if (ret != KATRA_SUCCESS || count == 0) {
        return ret;
    }

    LOG_INFO("SLEEP: Routing %zu memories by strength", count);

    /* Route each memory by strength */
    size_t high_count = 0;
    size_t medium_count = 0;
    size_t low_count = 0;

    for (size_t i = 0; i < count; i++) {
        memory_strength_t strength = katra_classify_strength(memories[i]);

        switch (strength) {
            case STRENGTH_HIGH:
                /* Preserve full detail - already in tier1, no action */
                high_count++;
                break;

            case STRENGTH_MEDIUM:
                /* TODO: Summarize and compress */
                medium_count++;
                break;

            case STRENGTH_LOW:
                /* TODO: Extract gist or mark for archival */
                low_count++;
                break;
        }
    }

    ctx->stats.memories_processed = count;
    ctx->stats.high_strength_preserved = high_count;
    ctx->stats.medium_strength_summarized = medium_count;
    ctx->stats.low_strength_archived = low_count;

    LOG_INFO("SLEEP: Routed memories - High: %zu, Medium: %zu, Low: %zu",
            high_count, medium_count, low_count);

    katra_memory_free_results(memories, count);
    return KATRA_SUCCESS;
}

/* Sleep mode: Calculate graph centrality */
int katra_sleep_calculate_centrality(consolidation_context_t* ctx) {
    if (!ctx) {
        return E_INPUT_NULL;
    }

    if (ctx->mode != MODE_SLEEP) {
        return KATRA_SUCCESS;  /* Only calculate in SLEEP mode */
    }

    if (!ctx->detector || !ctx->detector->graph) {
        return E_INVALID_STATE;
    }

    /* Calculate centrality for all nodes */
    int ret = katra_graph_calculate_centrality(ctx->detector->graph);
    if (ret == KATRA_SUCCESS) {
        size_t node_count = 0;
        size_t edge_count = 0;
        float avg_degree = 0.0f;

        katra_graph_stats(ctx->detector->graph, &node_count, &edge_count, &avg_degree);
        ctx->stats.centrality_updates = node_count;

        LOG_INFO("SLEEP: Updated centrality for %zu nodes (avg degree: %.2f)",
                node_count, avg_degree);
    }

    return ret;
}

/* Sleep mode: Extract patterns */
int katra_sleep_extract_patterns(consolidation_context_t* ctx) {
    if (!ctx) {
        return E_INPUT_NULL;
    }

    if (ctx->mode != MODE_SLEEP) {
        return 0;  /* No patterns in WAKE mode */
    }

    /* TODO: Implement pattern extraction using tier1_pattern.c */
    /* For now, just log that it would happen */
    LOG_INFO("SLEEP: Pattern extraction (placeholder)");
    ctx->stats.patterns_extracted = 0;

    return 0;
}

/* Sleep mode: Complete consolidation */
int katra_sleep_complete(consolidation_context_t* ctx,
                         consolidation_stats_t* stats) {
    if (!ctx) {
        return E_INPUT_NULL;
    }

    if (ctx->mode != MODE_SLEEP) {
        return KATRA_SUCCESS;  /* Already awake */
    }

    /* Record completion time */
    ctx->stats.sleep_completed = time(NULL);
    ctx->stats.consolidation_duration = (float)(ctx->stats.sleep_completed -
                                                ctx->stats.sleep_started);

    LOG_INFO("SLEEP: Consolidation complete in %.1f seconds",
            ctx->stats.consolidation_duration);
    LOG_INFO("SLEEP: Processed %zu memories (%zu high, %zu medium, %zu low)",
            ctx->stats.memories_processed,
            ctx->stats.high_strength_preserved,
            ctx->stats.medium_strength_summarized,
            ctx->stats.low_strength_archived);

    /* Return stats if requested */
    if (stats) {
        memcpy(stats, &ctx->stats, sizeof(consolidation_stats_t));
    }

    /* Reset for next wake cycle */
    ctx->mode = MODE_WAKE;
    ctx->stats.wake_started = time(NULL);
    ctx->stats.memories_captured = 0;
    ctx->stats.conscious_formations = 0;
    ctx->stats.subconscious_formations = 0;
    ctx->stats.convergences = 0;

    LOG_INFO("WAKE: Resumed wake mode");
    return KATRA_SUCCESS;
}

/* Get current mode */
consolidation_mode_t katra_consolidation_mode(consolidation_context_t* ctx) {
    if (!ctx) {
        return MODE_WAKE;
    }
    return ctx->mode;
}

/* Get consolidation statistics */
consolidation_stats_t katra_consolidation_stats(consolidation_context_t* ctx) {
    if (!ctx) {
        consolidation_stats_t empty = {0};
        return empty;
    }
    return ctx->stats;
}

/* Cleanup consolidation context */
void katra_consolidation_cleanup(consolidation_context_t* ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->detector) {
        katra_convergence_cleanup(ctx->detector);
    }

    LOG_DEBUG("Consolidation cleanup: %zu memories captured, %zu convergences",
             ctx->stats.memories_captured, ctx->stats.convergences);

    free(ctx);
}
