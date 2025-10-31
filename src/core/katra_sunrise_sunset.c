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
#include "katra_engram_common.h"
#include "katra_core_common.h"
#include "katra_error.h"
#include "katra_log.h"

/* Constants */
#define MIN_CLUSTER_SIZE 2
#define MIN_THREAD_LENGTH 2
#define SIMILARITY_THRESHOLD 0.6f
#define INSIGHT_CONFIDENCE_THRESHOLD 0.5f

/* Extract topic clusters from day's memories using vector similarity */
int katra_extract_topics(const char* ci_id,
                         vector_store_t* vectors,
                         topic_cluster_t*** clusters_out,
                         size_t* count_out) {
    ENGRAM_CHECK_PARAMS_3(ci_id, vectors, clusters_out);

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
        katra_report_error(E_SYSTEM_MEMORY, __func__, "Failed to allocate clusters array");
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
        if (best_similarity >= SIMILARITY_THRESHOLD) {
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
    ENGRAM_CHECK_PARAMS_3(ci_id, graph, threads_out);

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

        if (result == KATRA_SUCCESS && path_len >= MIN_THREAD_LENGTH) {
            conversation_thread_t* thread = calloc(1, sizeof(conversation_thread_t));
            if (thread) {
                snprintf(thread->thread_id, sizeof(thread->thread_id),
                        "thread_%zu", thread_count);

                thread->record_ids = calloc(path_len, sizeof(char*));
                thread->record_count = path_len;

                for (size_t j = 0; j < path_len; j++) {
                    thread->record_ids[j] = katra_safe_strdup(path[j]->record_id);
                }

                SAFE_STRNCPY(thread->start_topic, "Conversation");
                SAFE_STRNCPY(thread->end_topic, "Discussion");
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
    ENGRAM_CHECK_PARAMS_3(ci_id, arc_out, count_out);

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
    ENGRAM_CHECK_PARAMS_2(ci_id, insights_out);

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

/* Enhanced sundown */
int katra_sundown(const char* ci_id,
                  vector_store_t* vectors,
                  graph_store_t* graph,
                  sundown_context_t** context_out) {
    ENGRAM_CHECK_PARAMS_4(ci_id, vectors, graph, context_out);

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

    *context_out = context;

    LOG_INFO("Sundown complete for %s: %d interactions, %zu topics, %zu threads",
            ci_id, context->stats.interaction_count,
            context->topic_count, context->thread_count);

    return KATRA_SUCCESS;
}

/* Enhanced sunrise */
int katra_sunrise(const char* ci_id,
                  vector_store_t* vectors,
                  graph_store_t* graph,
                  sunrise_context_t** context_out) {
    ENGRAM_CHECK_PARAMS_4(ci_id, vectors, graph, context_out);

    sunrise_context_t* context;
    ALLOC_OR_RETURN(context, sunrise_context_t);

    SAFE_STRNCPY(context->ci_id, ci_id);
    context->timestamp = time(NULL);

    /* Load yesterday's sundown if available */
    /* For MVP, just create empty context */
    context->yesterday = NULL;

    /* Set baseline mood to neutral */
    context->baseline_mood.valence = 0.0f;
    context->baseline_mood.arousal = 0.0f;
    context->baseline_mood.dominance = 0.5f;
    SAFE_STRNCPY(context->baseline_mood.emotion, EMOTION_NEUTRAL);

    *context_out = context;

    LOG_INFO("Sunrise complete for %s", ci_id);
    return KATRA_SUCCESS;
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
