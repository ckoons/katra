/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_SUNRISE_SUNSET_H
#define KATRA_SUNRISE_SUNSET_H

#include "katra_continuity.h"
#include "katra_experience.h"
#include "katra_cognitive.h"
#include "katra_vector.h"
#include "katra_graph.h"
#include "katra_working_memory.h"
#include "katra_limits.h"
#include <time.h>

/* Advanced Sunrise/Sunset Protocol with vector and graph integration */

/* Configuration constants */
#define SUNRISE_MEMORY_QUERY_LIMIT 1000        /* Max memories per query */
#define SUNRISE_MAX_CLUSTERS 10                /* Max topic clusters */
#define SUNRISE_MAX_THREADS 10                 /* Max conversation threads */
#define SUNRISE_MAX_INSIGHTS 5                 /* Max daily insights */
#define SUNRISE_EMOTIONAL_ARC_SAMPLES 10       /* Samples for emotional arc */
#define SUNRISE_MAX_RECORDS_TO_PROCESS 100     /* Max records to cluster */
#define SUNRISE_GRAPH_TRAVERSAL_DEPTH 10       /* Max depth for graph traversal */

/* Working memory snapshot for sunset/sunrise (Phase 7.2)
 *
 * Captures the state of working memory items for persistence across sessions.
 * The actual experience data is stored separately; this tracks attention state.
 */
typedef struct {
    char content[KATRA_BUFFER_TEXT];     /* Content summary */
    float attention_score;               /* Current attention weight */
    time_t added_time;                   /* When originally added */
    time_t last_accessed;                /* When last accessed */
} wm_item_snapshot_t;

/* Working memory state for sunset/sunrise (Phase 7.2) */
typedef struct {
    wm_item_snapshot_t* items;           /* Array of item snapshots */
    size_t item_count;                   /* Number of items */
    size_t capacity;                     /* Working memory capacity setting */
    time_t last_consolidation;           /* Last consolidation timestamp */
    size_t total_consolidations;         /* Total consolidations performed */
} wm_state_snapshot_t;

/* Topic cluster (from vector similarity) */
typedef struct {
    char topic_name[128];       /* Extracted topic name */
    char** record_ids;          /* Memory IDs in this cluster */
    size_t record_count;        /* Number of records */
    float coherence;            /* Cluster coherence score */
    emotional_tag_t avg_emotion; /* Average emotion for topic */
} topic_cluster_t;

/* Conversation thread (from graph traversal) */
typedef struct {
    char thread_id[64];         /* Thread identifier */
    char** record_ids;          /* Memory IDs in thread */
    size_t record_count;        /* Number of records */
    char start_topic[128];      /* How thread started */
    char end_topic[128];        /* How thread ended */
    bool resolved;              /* Thread reached resolution */
} conversation_thread_t;

/* Daily insight (pattern or realization) */
typedef struct {
    char insight_text[512];     /* The insight */
    char** supporting_ids;      /* Supporting memory IDs */
    size_t support_count;       /* Number of supporting memories */
    float confidence;           /* Confidence in insight */
} daily_insight_t;

/* Enhanced sundown context */
typedef struct {
    char ci_id[256];            /* CI identifier */
    time_t timestamp;           /* When sundown occurred */

    /* Basic statistics */
    daily_stats_t stats;        /* Basic daily stats */

    /* Emotional journey */
    emotional_tag_t* mood_arc;  /* Emotional arc throughout day */
    size_t mood_count;          /* Number of mood samples */
    emotional_tag_t dominant_mood; /* Overall mood */

    /* Topic clustering (from vector DB) */
    topic_cluster_t** topics;   /* Topic clusters */
    size_t topic_count;         /* Number of topics */

    /* Conversation threads (from graph DB) */
    conversation_thread_t** threads; /* Conversation threads */
    size_t thread_count;        /* Number of threads */

    /* Key insights */
    daily_insight_t** insights; /* Daily insights */
    size_t insight_count;       /* Number of insights */

    /* Open questions */
    char** open_questions;      /* Unresolved questions */
    size_t question_count;      /* Number of questions */

    /* Tomorrow's intentions */
    char** intentions;          /* Plans for tomorrow */
    size_t intention_count;     /* Number of intentions */

    /* Working memory state (Phase 7.2) */
    wm_state_snapshot_t* working_memory;  /* Current working memory snapshot */
} sundown_context_t;

/* Enhanced sunrise context */
typedef struct {
    char ci_id[256];            /* CI identifier */
    time_t timestamp;           /* When sunrise occurred */

    /* Yesterday's summary */
    sundown_context_t* yesterday; /* Previous day context */

    /* Week in review */
    char** recurring_themes;    /* Themes across days */
    size_t theme_count;         /* Number of themes */

    /* Emotional baseline */
    emotional_tag_t baseline_mood; /* Starting mood */

    /* Continuity elements */
    char** pending_questions;   /* Questions to revisit */
    size_t pending_count;       /* Number of pending */
    char** carry_forward;       /* Items to continue */
    size_t carry_count;         /* Number to carry forward */

    /* Familiar context (vector similarity) */
    char** familiar_topics;     /* Topics from recent days */
    size_t familiar_count;      /* Number of familiar topics */

    /* Working memory to restore (Phase 7.2) */
    wm_state_snapshot_t* working_memory;  /* Working memory from previous session */
} sunrise_context_t;

/* Enhanced sundown: Create comprehensive end-of-day summary */
int katra_sundown(const char* ci_id,
                  vector_store_t* vectors,
                  graph_store_t* graph,
                  sundown_context_t** context_out);

/* Enhanced sundown with working memory capture (Phase 7.2)
 *
 * Same as katra_sundown but also captures working memory state.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   vectors - Vector store for semantic search
 *   graph - Graph store for relationship traversal
 *   wm - Working memory to capture (can be NULL)
 *   context_out - Output context with captured state
 */
int katra_sundown_with_wm(const char* ci_id,
                          vector_store_t* vectors,
                          graph_store_t* graph,
                          working_memory_t* wm,
                          sundown_context_t** context_out);

/* Enhanced sunrise: Load context for new day */
int katra_sunrise(const char* ci_id,
                  vector_store_t* vectors,
                  graph_store_t* graph,
                  sunrise_context_t** context_out);

/* Enhanced sunrise with working memory restore (Phase 7.2)
 *
 * Same as katra_sunrise but also restores working memory state.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   vectors - Vector store for semantic search
 *   graph - Graph store for relationship traversal
 *   wm - Working memory to restore into (can be NULL)
 *   context_out - Output context with restored state
 */
int katra_sunrise_with_wm(const char* ci_id,
                          vector_store_t* vectors,
                          graph_store_t* graph,
                          working_memory_t* wm,
                          sunrise_context_t** context_out);

/* Extract topic clusters from day's memories */
int katra_extract_topics(const char* ci_id,
                         vector_store_t* vectors,
                         topic_cluster_t*** clusters_out,
                         size_t* count_out);

/* Trace conversation threads from day's memories */
int katra_trace_threads(const char* ci_id,
                        graph_store_t* graph,
                        conversation_thread_t*** threads_out,
                        size_t* count_out);

/* Build emotional arc for the day */
int katra_build_emotional_arc(const char* ci_id,
                              emotional_tag_t** arc_out,
                              size_t* count_out);

/* Detect insights from day's patterns */
int katra_detect_insights(const char* ci_id,
                          topic_cluster_t** topics,
                          size_t topic_count,
                          conversation_thread_t** threads,
                          size_t thread_count,
                          daily_insight_t*** insights_out,
                          size_t* count_out);

/* Find recurring themes across multiple days */
int katra_find_recurring_themes(const char* ci_id,
                                int days_back,
                                char*** themes_out,
                                size_t* count_out);

/* Free sundown context */
void katra_sundown_free(sundown_context_t* context);

/* Free sunrise context */
void katra_sunrise_free(sunrise_context_t* context);

/* Free topic clusters */
void katra_topics_free(topic_cluster_t** clusters, size_t count);

/* Free conversation threads */
void katra_threads_free(conversation_thread_t** threads, size_t count);

/* Free insights */
void katra_insights_free(daily_insight_t** insights, size_t count);

/* Working memory snapshot functions (Phase 7.2) */

/* Capture working memory state for sunset
 *
 * Creates a snapshot of current working memory state for persistence.
 *
 * Parameters:
 *   wm - Working memory context to capture
 *
 * Returns:
 *   Snapshot structure or NULL on failure
 */
wm_state_snapshot_t* katra_wm_capture(working_memory_t* wm);

/* Restore working memory from snapshot
 *
 * Populates working memory with items from snapshot.
 *
 * Parameters:
 *   wm - Working memory context to restore into
 *   snapshot - Snapshot to restore from
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error code on failure
 */
int katra_wm_restore(working_memory_t* wm, const wm_state_snapshot_t* snapshot);

/* Free working memory snapshot */
void katra_wm_snapshot_free(wm_state_snapshot_t* snapshot);

#endif /* KATRA_SUNRISE_SUNSET_H */
