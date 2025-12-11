/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* Project includes */
#include "katra_sunrise_sunset.h"
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_psyche_common.h"
#include "katra_core_common.h"
#include "katra_experience.h"
#include "katra_working_memory.h"
#include "katra_daemon.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Extract topic clusters from day's memories using vector similarity */
int katra_extract_topics(const char* ci_id,
                         vector_store_t* vectors,
                         topic_cluster_t*** clusters_out,
                         size_t* count_out) {
    PSYCHE_CHECK_PARAMS_3(ci_id, vectors, clusters_out);

    /* Query today's memories */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = time(NULL),
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.0f,
        .tier = KATRA_TIER1,
        .limit = SUNRISE_MEMORY_QUERY_LIMIT
    };

    memory_record_t** records = NULL;
    size_t record_count = 0;
    int result = katra_memory_query(&query, &records, &record_count);
    if (result != KATRA_SUCCESS || record_count == 0) {
        *clusters_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    /* Simple clustering: group similar memories */
    topic_cluster_t** clusters;
    /* Note: Using manual allocation due to complex cleanup with records */
    clusters = calloc(SUNRISE_MAX_CLUSTERS, sizeof(topic_cluster_t*));
    if (!clusters) {
        katra_memory_free_results(records, record_count);
        katra_report_error(E_SYSTEM_MEMORY, __func__, KATRA_ERR_ALLOC_FAILED);
        return E_SYSTEM_MEMORY;
    }

    size_t cluster_count = 0;

    /* Create first cluster from first memory */
    if (record_count > 0) {
        clusters[0] = calloc(1, sizeof(topic_cluster_t));
        if (clusters[0]) {
            snprintf(clusters[0]->topic_name, sizeof(clusters[0]->topic_name),
                    "Topic %zu", cluster_count + 1);
            clusters[0]->record_ids = calloc(record_count, sizeof(char*));
            clusters[0]->record_ids[0] = katra_safe_strdup(records[0]->record_id);
            clusters[0]->record_count = 1;
            clusters[0]->coherence = 1.0f;
            cluster_count = 1;
        }
    }

    /* Assign remaining records to clusters */
    for (size_t i = 1; i < record_count && i < SUNRISE_MAX_RECORDS_TO_PROCESS; i++) {
        if (!records[i]->content) continue;

        /* Find best matching cluster using vector similarity */
        int best_cluster = 0;
        float best_similarity = 0.0f;

        for (size_t c = 0; c < cluster_count; c++) {
            /* Compare to first record in cluster */
            vector_embedding_t* emb1 = katra_vector_get(vectors,
                                                        clusters[c]->record_ids[0]);
            vector_embedding_t* emb2 = katra_vector_get(vectors,
                                                        records[i]->record_id);
            if (emb1 && emb2) {
                float sim = katra_vector_cosine_similarity(emb1, emb2);
                if (sim > best_similarity) {
                    best_similarity = sim;
                    best_cluster = c;
                }
            }
        }

        /* Add to cluster if similar enough, otherwise create new cluster */
        if (best_similarity >= SUNRISE_SIMILARITY_THRESHOLD) {
            topic_cluster_t* cluster = clusters[best_cluster];
            cluster->record_ids[cluster->record_count] =
                katra_safe_strdup(records[i]->record_id);
            cluster->record_count++;
        } else if (cluster_count < SUNRISE_MAX_CLUSTERS) {
            clusters[cluster_count] = calloc(1, sizeof(topic_cluster_t));
            if (clusters[cluster_count]) {
                snprintf(clusters[cluster_count]->topic_name,
                        sizeof(clusters[cluster_count]->topic_name),
                        "Topic %zu", cluster_count + 1);
                clusters[cluster_count]->record_ids = calloc(record_count, sizeof(char*));
                clusters[cluster_count]->record_ids[0] =
                    katra_safe_strdup(records[i]->record_id);
                clusters[cluster_count]->record_count = 1;
                clusters[cluster_count]->coherence = 1.0f;
                cluster_count++;
            }
        }
    }

    katra_memory_free_results(records, record_count);

    *clusters_out = clusters;
    *count_out = cluster_count;

    LOG_INFO("Extracted %zu topic clusters for CI: %s", cluster_count, ci_id);
    return KATRA_SUCCESS;
}

/* Trace conversation threads using graph traversal */
int katra_trace_threads(const char* ci_id,
                        graph_store_t* graph,
                        conversation_thread_t*** threads_out,
                        size_t* count_out) {
    PSYCHE_CHECK_PARAMS_3(ci_id, graph, threads_out);

    /* Query today's memories */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = time(NULL),
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.0f,
        .tier = KATRA_TIER1,
        .limit = SUNRISE_MEMORY_QUERY_LIMIT
    };

    memory_record_t** records = NULL;
    size_t record_count = 0;
    int result = katra_memory_query(&query, &records, &record_count);
    if (result != KATRA_SUCCESS || record_count == 0) {
        *threads_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    /* Create threads from sequential relationships */
    conversation_thread_t** threads = calloc(SUNRISE_MAX_THREADS, sizeof(conversation_thread_t*));
    if (!threads) {
        katra_memory_free_results(records, record_count);
        return E_SYSTEM_MEMORY;
    }

    size_t thread_count = 0;

    /* Simple thread detection: follow sequential edges */
    for (size_t i = 0; i < record_count && thread_count < SUNRISE_MAX_THREADS; i++) {
        graph_path_node_t** path = NULL;
        size_t path_len = 0;

        /* Traverse from this record */
        result = katra_graph_traverse(graph, records[i]->record_id, SUNRISE_GRAPH_TRAVERSAL_DEPTH,
                                      &path, &path_len);

        if (result == KATRA_SUCCESS && path_len >= SUNRISE_MIN_THREAD_LENGTH) {
            conversation_thread_t* thread = calloc(1, sizeof(conversation_thread_t));
            if (thread) {
                snprintf(thread->thread_id, sizeof(thread->thread_id),
                        "thread_%zu", thread_count);

                thread->record_ids = calloc(path_len, sizeof(char*));
                thread->record_count = path_len;

                for (size_t j = 0; j < path_len; j++) {
                    thread->record_ids[j] = katra_safe_strdup(path[j]->record_id);
                }

                /* GUIDELINE_APPROVED: Thread placeholder topic names */
                SAFE_STRNCPY(thread->start_topic, "Conversation"); /* GUIDELINE_APPROVED */
                SAFE_STRNCPY(thread->end_topic, "Discussion"); /* GUIDELINE_APPROVED */
                /* GUIDELINE_APPROVED_END */
                thread->resolved = false;

                threads[thread_count++] = thread;
            }

            katra_graph_free_paths(path, path_len);
        }
    }

    katra_memory_free_results(records, record_count);

    *threads_out = threads;
    *count_out = thread_count;

    LOG_INFO("Traced %zu conversation threads for CI: %s", thread_count, ci_id);
    return KATRA_SUCCESS;
}

/* Build emotional arc for the day */
int katra_build_emotional_arc(const char* ci_id,
                              emotional_tag_t** arc_out,
                              size_t* count_out) {
    PSYCHE_CHECK_PARAMS_3(ci_id, arc_out, count_out);

    /* Query today's memories */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = time(NULL),
        .type = MEMORY_TYPE_EXPERIENCE,
        .min_importance = 0.0f,
        .tier = KATRA_TIER1,
        .limit = SUNRISE_MEMORY_QUERY_LIMIT
    };

    memory_record_t** records = NULL;
    size_t record_count = 0;
    int result = katra_memory_query(&query, &records, &record_count);
    if (result != KATRA_SUCCESS || record_count == 0) {
        *arc_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    /* Sample emotional state at regular intervals */
    size_t samples = (record_count < SUNRISE_EMOTIONAL_ARC_SAMPLES) ? record_count : SUNRISE_EMOTIONAL_ARC_SAMPLES;
    emotional_tag_t* arc = calloc(samples, sizeof(emotional_tag_t));
    if (!arc) {
        katra_memory_free_results(records, record_count);
        return E_SYSTEM_MEMORY;
    }

    size_t step = record_count / samples;
    if (step == 0) step = 1;

    for (size_t i = 0; i < samples && i * step < record_count; i++) {
        size_t idx = i * step;
        if (records[idx]->content) {
            katra_detect_emotion(records[idx]->content, &arc[i]);
        } else {
            arc[i].valence = 0.0f;
            arc[i].arousal = 0.0f;
            arc[i].dominance = 0.5f;
            SAFE_STRNCPY(arc[i].emotion, EMOTION_NEUTRAL);
        }
        arc[i].timestamp = records[idx]->timestamp;
    }

    katra_memory_free_results(records, record_count);

    *arc_out = arc;
    *count_out = samples;

    LOG_INFO("Built emotional arc with %zu samples for CI: %s", samples, ci_id);
    return KATRA_SUCCESS;
}

/* Detect insights from patterns */
int katra_detect_insights(const char* ci_id,
                          topic_cluster_t** topics,
                          size_t topic_count,
                          conversation_thread_t** threads,
                          size_t thread_count,
                          daily_insight_t*** insights_out,
                          size_t* count_out) {
    PSYCHE_CHECK_PARAMS_2(ci_id, insights_out);

    /* Use topics and threads for insight detection */
    (void)topics;  /* Used below */
    (void)threads; /* Used below */

    /* Generate simple insights from topics and threads */
    daily_insight_t** insights = calloc(SUNRISE_MAX_INSIGHTS, sizeof(daily_insight_t*));
    if (!insights) {
        return E_SYSTEM_MEMORY;
    }

    size_t insight_count = 0;

    /* Insight: Multiple related topics */
    if (topic_count >= 2) {
        insights[insight_count] = calloc(1, sizeof(daily_insight_t));
        if (insights[insight_count]) {
            snprintf(insights[insight_count]->insight_text,
                    sizeof(insights[insight_count]->insight_text),
                    "Explored %zu different topics today", topic_count);
            insights[insight_count]->confidence = 0.9f;
            insight_count++;
        }
    }

    /* Insight: Long conversation threads */
    if (thread_count >= 1) {
        insights[insight_count] = calloc(1, sizeof(daily_insight_t));
        if (insights[insight_count]) {
            snprintf(insights[insight_count]->insight_text,
                    sizeof(insights[insight_count]->insight_text),
                    "Had %zu in-depth conversations", thread_count);
            insights[insight_count]->confidence = 0.8f;
            insight_count++;
        }
    }

    *insights_out = insights;
    *count_out = insight_count;

    LOG_INFO("Detected %zu insights for CI: %s", insight_count, ci_id);
    return KATRA_SUCCESS;
}

/* Enhanced sundown with working memory capture (Phase 7.2) */
int katra_sundown_with_wm(const char* ci_id,
                          vector_store_t* vectors,
                          graph_store_t* graph,
                          working_memory_t* wm,
                          sundown_context_t** context_out) {
    PSYCHE_CHECK_PARAMS_4(ci_id, vectors, graph, context_out);

    sundown_context_t* context;
    ALLOC_OR_RETURN(context, sundown_context_t);

    SAFE_STRNCPY(context->ci_id, ci_id);
    context->timestamp = time(NULL);

    /* Get basic daily stats */
    katra_get_daily_stats(ci_id, &context->stats);

    /* Build emotional arc */
    katra_build_emotional_arc(ci_id, &context->mood_arc, &context->mood_count);

    /* Calculate dominant mood */
    if (context->mood_count > 0) {
        float sum_v = 0.0f, sum_a = 0.0f, sum_d = 0.0f;
        for (size_t i = 0; i < context->mood_count; i++) {
            sum_v += context->mood_arc[i].valence;
            sum_a += context->mood_arc[i].arousal;
            sum_d += context->mood_arc[i].dominance;
        }
        context->dominant_mood.valence = sum_v / context->mood_count;
        context->dominant_mood.arousal = sum_a / context->mood_count;
        context->dominant_mood.dominance = sum_d / context->mood_count;
        katra_name_emotion(&context->dominant_mood);
    }

    /* Extract topic clusters */
    katra_extract_topics(ci_id, vectors, &context->topics, &context->topic_count);

    /* Trace conversation threads */
    katra_trace_threads(ci_id, graph, &context->threads, &context->thread_count);

    /* Detect insights */
    katra_detect_insights(ci_id, context->topics, context->topic_count,
                         context->threads, context->thread_count,
                         &context->insights, &context->insight_count);

    /* Capture working memory state (Phase 7.2) */
    if (wm) {
        context->working_memory = katra_wm_capture(wm);
        if (context->working_memory) {
            LOG_INFO("Captured working memory: %zu items", context->working_memory->item_count);
        }
    } else {
        context->working_memory = NULL;
    }

    *context_out = context;

    LOG_INFO("Sundown complete for %s: %d interactions, %zu topics, %zu threads",
            ci_id, context->stats.interaction_count,
            context->topic_count, context->thread_count);

    return KATRA_SUCCESS;
}

/* Enhanced sundown (backward-compatible wrapper) */
int katra_sundown(const char* ci_id,
                  vector_store_t* vectors,
                  graph_store_t* graph,
                  sundown_context_t** context_out) {
    return katra_sundown_with_wm(ci_id, vectors, graph, NULL, context_out);
}

/* Enhanced sunrise with working memory restore (Phase 7.2-7.6) */
int katra_sunrise_with_wm(const char* ci_id,
                          vector_store_t* vectors,
                          graph_store_t* graph,
                          working_memory_t* wm,
                          sunrise_context_t** context_out) {
    /* Note: vectors and graph can be NULL - we use them if available */
    (void)graph;  /* Reserved for future graph-based analysis */
    if (!ci_id || !context_out) {
        return E_INPUT_NULL;
    }

    sunrise_context_t* context;
    ALLOC_OR_RETURN(context, sunrise_context_t);

    SAFE_STRNCPY(context->ci_id, ci_id);
    context->timestamp = time(NULL);

    /* Load yesterday's sundown if available (Phase 7.4) */
    int load_result = katra_sundown_load_latest(ci_id, &context->yesterday);
    if (load_result == KATRA_SUCCESS && context->yesterday) {
        LOG_INFO("Loaded previous sundown context for %s", ci_id);

        /* Use yesterday's dominant mood as baseline */
        context->baseline_mood = context->yesterday->dominant_mood;
    } else {
        /* No previous sundown - set baseline mood to neutral */
        context->yesterday = NULL;
        context->baseline_mood.valence = 0.0f;
        context->baseline_mood.arousal = 0.0f;
        context->baseline_mood.dominance = 0.5f;
        SAFE_STRNCPY(context->baseline_mood.emotion, EMOTION_NEUTRAL);
    }

    /* Find recurring themes across recent days (Phase 7.5) */
    katra_find_recurring_themes(ci_id, 7,
                               &context->recurring_themes,
                               &context->theme_count);
    if (context->theme_count > 0) {
        LOG_INFO("Found %zu recurring themes for %s", context->theme_count, ci_id);
    }

    /* Build familiar topics using vector similarity (Phase 7.6) */
    katra_build_familiar_topics(ci_id, vectors, 7,
                               &context->familiar_topics,
                               &context->familiar_count);
    if (context->familiar_count > 0) {
        LOG_INFO("Built %zu familiar topics for %s", context->familiar_count, ci_id);
    }

    /* Carry forward open questions and intentions from yesterday */
    if (context->yesterday) {
        /* Carry forward unresolved questions */
        if (context->yesterday->question_count > 0 && context->yesterday->open_questions) {
            context->pending_questions = calloc(context->yesterday->question_count, sizeof(char*));
            if (context->pending_questions) {
                for (size_t i = 0; i < context->yesterday->question_count; i++) {
                    if (context->yesterday->open_questions[i]) {
                        context->pending_questions[i] =
                            katra_safe_strdup(context->yesterday->open_questions[i]);
                    }
                }
                context->pending_count = context->yesterday->question_count;
            }
        }

        /* Carry forward intentions as items to continue */
        if (context->yesterday->intention_count > 0 && context->yesterday->intentions) {
            context->carry_forward = calloc(context->yesterday->intention_count, sizeof(char*));
            if (context->carry_forward) {
                for (size_t i = 0; i < context->yesterday->intention_count; i++) {
                    if (context->yesterday->intentions[i]) {
                        context->carry_forward[i] =
                            katra_safe_strdup(context->yesterday->intentions[i]);
                    }
                }
                context->carry_count = context->yesterday->intention_count;
            }
        }
    }

    /* Restore working memory from previous session (Phase 7.2)
     * If we have a previous sundown context with working memory, restore it */
    if (wm && context->yesterday && context->yesterday->working_memory) {
        int wm_result = katra_wm_restore(wm, context->yesterday->working_memory);
        if (wm_result == KATRA_SUCCESS) {
            LOG_INFO("Restored working memory from previous session");
            /* Store reference to the snapshot for context */
            context->working_memory = context->yesterday->working_memory;
            /* Clear reference in yesterday so it doesn't get double-freed */
            context->yesterday->working_memory = NULL;
        }
    } else {
        context->working_memory = NULL;
    }

    /* Fetch pending daemon insights (Phase 9 integration)
     * These are discoveries made while the CI was asleep */
    daemon_insight_t* insights = NULL;
    size_t insight_count = 0;
    int daemon_result = katra_daemon_get_pending_insights(ci_id, &insights, &insight_count);
    if (daemon_result == KATRA_SUCCESS && insight_count > 0) {
        context->daemon_insights = calloc(insight_count, sizeof(char*));
        if (context->daemon_insights) {
            for (size_t i = 0; i < insight_count; i++) {
                context->daemon_insights[i] = katra_safe_strdup(insights[i].content);
                /* Mark insight as acknowledged */
                katra_daemon_acknowledge_insight(insights[i].id);
            }
            context->daemon_insight_count = insight_count;
            LOG_INFO("Loaded %zu daemon insights for sunrise", insight_count);
        }
        katra_daemon_free_insights(insights, insight_count);
    } else {
        context->daemon_insights = NULL;
        context->daemon_insight_count = 0;
    }

    *context_out = context;

    LOG_INFO("Sunrise complete for %s (themes: %zu, familiar: %zu)",
             ci_id, context->theme_count, context->familiar_count);
    return KATRA_SUCCESS;
}

/* Enhanced sunrise (backward-compatible wrapper) */
int katra_sunrise(const char* ci_id,
                  vector_store_t* vectors,
                  graph_store_t* graph,
                  sunrise_context_t** context_out) {
    return katra_sunrise_with_wm(ci_id, vectors, graph, NULL, context_out);
}

/* Free sundown context */
void katra_sundown_free(sundown_context_t* context) {
    if (!context) return;

    free(context->mood_arc);
    katra_topics_free(context->topics, context->topic_count);
    katra_threads_free(context->threads, context->thread_count);
    katra_insights_free(context->insights, context->insight_count);

    katra_free_string_array(context->open_questions, context->question_count);
    katra_free_string_array(context->intentions, context->intention_count);

    /* Free working memory snapshot (Phase 7.2) */
    katra_wm_snapshot_free(context->working_memory);

    free(context);
}

/* Free sunrise context */
void katra_sunrise_free(sunrise_context_t* context) {
    if (!context) return;

    katra_sundown_free(context->yesterday);

    katra_free_string_array(context->recurring_themes, context->theme_count);
    katra_free_string_array(context->pending_questions, context->pending_count);
    katra_free_string_array(context->carry_forward, context->carry_count);
    katra_free_string_array(context->familiar_topics, context->familiar_count);

    /* Free daemon insights (Phase 9 integration) */
    katra_free_string_array(context->daemon_insights, context->daemon_insight_count);

    /* Free working memory snapshot (Phase 7.2) */
    katra_wm_snapshot_free(context->working_memory);

    free(context);
}

/* Free topic clusters */
void katra_topics_free(topic_cluster_t** clusters, size_t count) {
    if (!clusters) return;

    for (size_t i = 0; i < count; i++) {
        if (clusters[i]) {
            for (size_t j = 0; j < clusters[i]->record_count; j++) {
                free(clusters[i]->record_ids[j]);
            }
            free(clusters[i]->record_ids);
            free(clusters[i]);
        }
    }
    free(clusters);
}

/* Free conversation threads */
void katra_threads_free(conversation_thread_t** threads, size_t count) {
    if (!threads) return;

    for (size_t i = 0; i < count; i++) {
        if (threads[i]) {
            for (size_t j = 0; j < threads[i]->record_count; j++) {
                free(threads[i]->record_ids[j]);
            }
            free(threads[i]->record_ids);
            free(threads[i]);
        }
    }
    free(threads);
}

/* Free insights */
void katra_insights_free(daily_insight_t** insights, size_t count) {
    if (!insights) return;

    for (size_t i = 0; i < count; i++) {
        if (insights[i]) {
            for (size_t j = 0; j < insights[i]->support_count; j++) {
                free(insights[i]->supporting_ids[j]);
            }
            free(insights[i]->supporting_ids);
            free(insights[i]);
        }
    }
    free(insights);
}

/* Working memory snapshot functions are in katra_sunrise_sunset_wm.c */
