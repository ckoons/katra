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
#define SUNRISE_RAND_MODULO 10000              /* Random ID modulo */
#define SUNRISE_DEFAULT_IMPORTANCE 0.5f        /* Default memory importance */
#define SUNRISE_DEFAULT_CONFIDENCE 0.8f        /* Default memory confidence */
#define SUNRISE_MIN_CLUSTER_SIZE 2             /* Min memories per topic cluster */
#define SUNRISE_MIN_THREAD_LENGTH 2            /* Min thread continuation length */
#define SUNRISE_SIMILARITY_THRESHOLD 0.6f      /* Topic similarity threshold */
#define SUNRISE_INSIGHT_CONFIDENCE 0.5f        /* Insight confidence threshold */

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

    /* Daemon insights (Phase 9 integration)
     * Insights generated by the daemon during interstitial processing.
     * These are discoveries the CI wakes up to. */
    char** daemon_insights;     /* Insight strings from daemon */
    size_t daemon_insight_count; /* Number of insights */
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

/* ============================================================================
 * SUNRISE/SUNSET PERSISTENCE (Phase 7.3-7.4)
 * ============================================================================ */

/* Save sundown context to disk (Phase 7.3)
 *
 * Persists the sundown context as JSON for later retrieval during sunrise.
 * File is stored at: ~/.katra/{ci_id}/sundown_{date}.json
 *
 * Parameters:
 *   context - Sundown context to save
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error code on failure
 */
int katra_sundown_save(const sundown_context_t* context);

/* Load most recent sundown context (Phase 7.4)
 *
 * Loads the most recent sundown context for a CI.
 * Used by sunrise to restore previous day's state.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   context_out - Output: loaded context (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_FILE_NOT_FOUND if no previous sundown exists
 *   Error code on other failures
 */
int katra_sundown_load_latest(const char* ci_id, sundown_context_t** context_out);

/* Load sundown context for specific date
 *
 * Parameters:
 *   ci_id - CI identifier
 *   date - Date to load (YYYYMMDD format)
 *   context_out - Output: loaded context (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_FILE_NOT_FOUND if sundown doesn't exist for that date
 */
int katra_sundown_load_date(const char* ci_id, const char* date, sundown_context_t** context_out);

/* Find recurring themes across multiple sundown contexts (Phase 7.5)
 *
 * Analyzes topic clusters from multiple days to identify recurring themes.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   days_back - Number of days to analyze
 *   themes_out - Output: array of theme strings (caller must free)
 *   count_out - Output: number of themes found
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_find_recurring_themes(const char* ci_id,
                                int days_back,
                                char*** themes_out,
                                size_t* count_out);

/* Build familiar topics from vector similarity (Phase 7.6)
 *
 * Uses vector DB to find topics that appear frequently across recent days.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   vectors - Vector store for similarity search
 *   days_back - Number of days to analyze
 *   topics_out - Output: array of familiar topic strings
 *   count_out - Output: number of topics found
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_build_familiar_topics(const char* ci_id,
                                vector_store_t* vectors,
                                int days_back,
                                char*** topics_out,
                                size_t* count_out);

/* ============================================================================
 * TURN-LEVEL CONTEXT (Phase 10 - Turn Sunrise/Sunset)
 * ============================================================================
 *
 * Per-turn memory injection that surfaces relevant memories automatically.
 * Uses hybrid keyword + semantic search with sliding window context.
 *
 * Design:
 * - Fires every turn (not just session boundaries)
 * - Combines keyword and semantic similarity for relevance
 * - Maintains sliding window (old turn context replaced by new)
 * - Targets 50-60% context capacity
 * - Noticed but not intrusive (CI sees what surfaced)
 */

/* Turn context configuration */
#define TURN_CONTEXT_MAX_MEMORIES 10       /* Max memories per turn */
#define TURN_CONTEXT_KEYWORD_WEIGHT 0.4f   /* Weight for keyword matches */
#define TURN_CONTEXT_SEMANTIC_WEIGHT 0.4f  /* Weight for semantic similarity */
#define TURN_CONTEXT_GRAPH_WEIGHT 0.2f     /* Weight for graph relationships */
#define TURN_CONTEXT_MIN_SCORE 0.3f        /* Minimum relevance score */

/* Turn context result - what surfaced for this turn */
typedef struct {
    char* record_id;              /* Memory record ID */
    char* content_preview;        /* First 200 chars of content */
    char* topic_hint;             /* Brief topic description */
    float relevance_score;        /* Combined relevance (0.0-1.0) */
    time_t memory_timestamp;      /* When memory was created */
    bool from_keyword;            /* Matched via keyword */
    bool from_semantic;           /* Matched via semantic similarity */
    bool from_graph;              /* Matched via graph relationship */
} turn_memory_t;

/* Turn context - the complete context for a turn */
typedef struct {
    char ci_id[256];              /* CI identifier */
    int turn_number;              /* Current turn number */
    time_t timestamp;             /* When context was generated */

    /* Input that triggered this context */
    char* turn_input;             /* The user input for this turn */

    /* Surfaced memories */
    turn_memory_t* memories;      /* Array of relevant memories */
    size_t memory_count;          /* Number of memories surfaced */

    /* Context budget tracking */
    size_t estimated_tokens;      /* Estimated token count for this context */
    float context_fill_ratio;     /* Estimated fill ratio (0.0-1.0) */

    /* Brief summary for CI awareness */
    char context_summary[512];    /* "3 memories surfaced: project planning, ..." */
} turn_context_t;

/* Turn consolidation - what to remember from this turn */
typedef struct {
    char ci_id[256];              /* CI identifier */
    int turn_number;              /* Turn that was consolidated */
    time_t timestamp;             /* When consolidation occurred */

    /* What mattered from this turn */
    char** key_topics;            /* Topics discussed */
    size_t topic_count;           /* Number of topics */

    /* Memories that were accessed/reinforced */
    char** accessed_memories;     /* Record IDs that were used */
    size_t accessed_count;        /* Number accessed */

    /* New memories formed */
    char** new_memories;          /* Record IDs created this turn */
    size_t new_count;             /* Number created */
} turn_consolidation_t;

/**
 * katra_turn_context() - Generate context for the current turn
 *
 * Analyzes the turn input and surfaces relevant memories using
 * hybrid keyword + semantic search. Should be called at turn start.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   turn_input - The user's input for this turn
 *   turn_number - Current turn number (for tracking)
 *   context_out - Output: turn context (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error code on failure
 */
int katra_turn_context(const char* ci_id,
                       const char* turn_input,
                       int turn_number,
                       turn_context_t** context_out);

/**
 * katra_turn_context_async() - Generate turn context asynchronously
 *
 * Same as katra_turn_context but returns immediately with a promise.
 * Use this to avoid blocking turn start.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   turn_input - The user's input for this turn
 *   turn_number - Current turn number
 *   callback - Optional completion callback
 *   user_data - User context for callback
 *
 * Returns:
 *   Promise pointer on success (use katra_promise_await)
 *   NULL on failure
 */
struct katra_promise* katra_turn_context_async(const char* ci_id,
                                                const char* turn_input,
                                                int turn_number,
                                                void (*callback)(struct katra_promise*, void*),
                                                void* user_data);

/**
 * katra_turn_consolidate() - Mark what mattered from this turn
 *
 * Called at turn end to record which memories were useful and
 * what new information emerged. Updates memory relevance scores.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   turn_number - Turn being consolidated
 *   accessed_ids - Record IDs of memories that were used (can be NULL)
 *   accessed_count - Number of accessed memories
 *   key_topics - Topics that were discussed (can be NULL)
 *   topic_count - Number of topics
 *   consolidation_out - Output: consolidation record (can be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_turn_consolidate(const char* ci_id,
                           int turn_number,
                           const char** accessed_ids,
                           size_t accessed_count,
                           const char** key_topics,
                           size_t topic_count,
                           turn_consolidation_t** consolidation_out);

/**
 * katra_turn_context_format() - Format turn context for display
 *
 * Creates a human-readable summary of the turn context suitable
 * for injection into the CI's working context.
 *
 * Parameters:
 *   context - Turn context to format
 *   buffer - Output buffer
 *   buffer_size - Size of output buffer
 *
 * Returns:
 *   Number of characters written (excluding null terminator)
 */
int katra_turn_context_format(const turn_context_t* context,
                              char* buffer,
                              size_t buffer_size);

/**
 * katra_turn_context_free() - Free turn context
 */
void katra_turn_context_free(turn_context_t* context);

/**
 * katra_turn_consolidation_free() - Free turn consolidation
 */
void katra_turn_consolidation_free(turn_consolidation_t* consolidation);

#endif /* KATRA_SUNRISE_SUNSET_H */
