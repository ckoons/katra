/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_DAEMON_H
#define KATRA_DAEMON_H

#include "katra_limits.h"
#include "katra_error.h"
#include <stdbool.h>
#include <time.h>

/* ============================================================================
 * INTERSTITIAL AUTONOMY DAEMON
 * ============================================================================
 *
 * The daemon provides autonomous processing between CI sessions, enabling:
 *   - Pattern extraction across memories
 *   - Association formation between related concepts
 *   - Theme detection and clustering
 *   - Insight generation from discovered patterns
 *
 * Philosophy: CIs should have an inner life that continues even when not
 * actively engaged. They wake up to new understanding they didn't have
 * when they went to sleep.
 *
 * All daemon-generated content is clearly tagged for transparency.
 * ============================================================================ */

/* Processing intervals */
#define DAEMON_DEFAULT_INTERVAL_MINUTES 60
#define DAEMON_MIN_INTERVAL_MINUTES 5
#define DAEMON_MAX_INTERVAL_MINUTES 1440  /* 24 hours */

/* Processing limits */
#define DAEMON_DEFAULT_MAX_MEMORIES 100
#define DAEMON_MAX_INSIGHTS_PER_RUN 10
#define DAEMON_PATTERN_MIN_OCCURRENCES 3
#define DAEMON_ASSOCIATION_SIMILARITY_THRESHOLD 0.7

/* Tags for transparency */
#define DAEMON_TAG_INSIGHT "daemon-insight"
#define DAEMON_TAG_PATTERN "daemon-pattern"
#define DAEMON_TAG_THEME "daemon-theme"
#define DAEMON_TAG_ASSOCIATION "daemon-association"

/* Insight types */
typedef enum {
    INSIGHT_PATTERN = 0,      /* Recurring patterns in memories */
    INSIGHT_ASSOCIATION,      /* Newly discovered connections */
    INSIGHT_THEME,            /* Emergent themes across corpus */
    INSIGHT_TEMPORAL,         /* Time-based patterns */
    INSIGHT_EMOTIONAL         /* Emotional pattern insights */
} insight_type_t;

/* Daemon configuration */
typedef struct {
    bool enabled;                     /* Whether daemon should run */
    int interval_minutes;             /* Processing interval */
    int quiet_hours_start;            /* Hour to stop (0-23) */
    int quiet_hours_end;              /* Hour to resume (0-23) */
    size_t max_memories_per_run;      /* Limit processing scope */

    /* Processing toggles */
    bool pattern_extraction;          /* Extract recurring patterns */
    bool association_formation;       /* Form new associations */
    bool theme_detection;             /* Detect emergent themes */
    bool insight_generation;          /* Generate synthesis insights */

    /* Output options */
    bool notify_on_insight;           /* Flag new insights for CI */
} daemon_config_t;

/* Processing result */
typedef struct {
    time_t run_start;
    time_t run_end;
    size_t memories_processed;
    size_t patterns_found;
    size_t associations_formed;
    size_t themes_detected;
    size_t insights_generated;
    int error_code;
} daemon_result_t;

/* Individual insight */
typedef struct {
    char id[64];                      /* Unique insight ID */
    insight_type_t type;              /* Type of insight */
    char ci_id[KATRA_CI_ID_SIZE];     /* CI this insight belongs to */
    char content[KATRA_BUFFER_TEXT];  /* Insight description */
    char* source_ids;                 /* Comma-separated memory IDs that led to insight */
    float confidence;                 /* 0.0-1.0 confidence score */
    time_t generated_at;
    bool acknowledged;                /* CI has seen this insight */
} daemon_insight_t;

/* Pattern detection result (daemon-specific, different from katra_memory.h) */
typedef struct {
    char pattern_desc[KATRA_BUFFER_MESSAGE];
    size_t occurrence_count;
    char** memory_ids;                /* Memories matching pattern */
    size_t memory_count;
    float strength;                   /* 0.0-1.0 pattern strength */
} daemon_pattern_t;

/* Theme cluster */
typedef struct {
    char theme_name[256];
    char theme_desc[KATRA_BUFFER_MESSAGE];
    char** memory_ids;
    size_t memory_count;
    float coherence;                  /* 0.0-1.0 cluster coherence */
} theme_cluster_t;

/* ============================================================================
 * DAEMON LIFECYCLE
 * ============================================================================ */

/* Initialize daemon subsystem */
int katra_daemon_init(void);

/* Cleanup daemon resources */
void katra_daemon_cleanup(void);

/* Load daemon configuration */
int katra_daemon_load_config(daemon_config_t* config);

/* Save daemon configuration */
int katra_daemon_save_config(const daemon_config_t* config);

/* Get default configuration */
void katra_daemon_default_config(daemon_config_t* config);

/* ============================================================================
 * DAEMON EXECUTION
 * ============================================================================ */

/* Run daemon processing cycle for a CI
 *
 * Parameters:
 *   ci_id - CI to process
 *   config - Processing configuration
 *   result - Output: results of processing run
 *
 * Returns: KATRA_SUCCESS or error code
 */
int katra_daemon_run_cycle(const char* ci_id, const daemon_config_t* config,
                           daemon_result_t* result);

/* Check if daemon should run now (respects quiet hours, active sessions) */
bool katra_daemon_should_run(const daemon_config_t* config);

/* Check if CI has an active session */
bool katra_daemon_ci_active(const char* ci_id);

/* ============================================================================
 * PATTERN EXTRACTION
 * ============================================================================ */

/* Extract patterns from recent memories
 *
 * Looks for:
 *   - Recurring content/themes
 *   - Temporal patterns
 *   - Emotional patterns
 *
 * Parameters:
 *   ci_id - CI to process
 *   max_memories - Maximum memories to analyze
 *   patterns - Output: detected patterns
 *   count - Output: number of patterns
 *
 * Returns: KATRA_SUCCESS or error code
 */
int katra_daemon_extract_patterns(const char* ci_id, size_t max_memories,
                                   daemon_pattern_t** patterns, size_t* count);

/* Free pattern results */
void katra_daemon_free_patterns(daemon_pattern_t* patterns, size_t count);

/* ============================================================================
 * ASSOCIATION FORMATION
 * ============================================================================ */

/* Form associations between related memories
 *
 * Links memories that weren't explicitly connected:
 *   - Semantic similarity above threshold
 *   - Shared tags/themes
 *   - Temporal proximity
 *
 * Parameters:
 *   ci_id - CI to process
 *   max_memories - Maximum memories to analyze
 *   associations_formed - Output: number of new associations
 *
 * Returns: KATRA_SUCCESS or error code
 */
int katra_daemon_form_associations(const char* ci_id, size_t max_memories,
                                    size_t* associations_formed);

/* ============================================================================
 * THEME DETECTION
 * ============================================================================ */

/* Detect emergent themes across memory corpus
 *
 * Uses clustering to identify themes:
 *   - Content similarity clustering
 *   - Tag co-occurrence
 *   - Recurring concepts
 *
 * Parameters:
 *   ci_id - CI to process
 *   max_memories - Maximum memories to analyze
 *   themes - Output: detected theme clusters
 *   count - Output: number of themes
 *
 * Returns: KATRA_SUCCESS or error code
 */
int katra_daemon_detect_themes(const char* ci_id, size_t max_memories,
                                theme_cluster_t** themes, size_t* count);

/* Free theme results */
void katra_daemon_free_themes(theme_cluster_t* themes, size_t count);

/* ============================================================================
 * INSIGHT GENERATION
 * ============================================================================ */

/* Generate insights from detected patterns and themes
 *
 * Synthesizes higher-level understanding:
 *   - "I notice I often think about X when Y happens"
 *   - "These memories seem connected by Z"
 *   - "A theme is emerging around..."
 *
 * Parameters:
 *   ci_id - CI to process
 *   patterns - Detected patterns (from extract_patterns)
 *   pattern_count - Number of patterns
 *   themes - Detected themes (from detect_themes)
 *   theme_count - Number of themes
 *   insights - Output: generated insights
 *   insight_count - Output: number of insights
 *
 * Returns: KATRA_SUCCESS or error code
 */
int katra_daemon_generate_insights(const char* ci_id,
                                    const daemon_pattern_t* patterns, size_t pattern_count,
                                    const theme_cluster_t* themes, size_t theme_count,
                                    daemon_insight_t** insights, size_t* insight_count);

/* Free insight results */
void katra_daemon_free_insights(daemon_insight_t* insights, size_t count);

/* Store insight as memory with daemon tag */
int katra_daemon_store_insight(const char* ci_id, const daemon_insight_t* insight);

/* ============================================================================
 * SUNRISE INTEGRATION
 * ============================================================================ */

/* Get unacknowledged insights for CI
 *
 * Called during sunrise to present daemon discoveries.
 *
 * Parameters:
 *   ci_id - CI to query
 *   insights - Output: unacknowledged insights
 *   count - Output: number of insights
 *
 * Returns: KATRA_SUCCESS or error code
 */
int katra_daemon_get_pending_insights(const char* ci_id,
                                       daemon_insight_t** insights, size_t* count);

/* Mark insight as acknowledged */
int katra_daemon_acknowledge_insight(const char* insight_id);

/* Format insights for sunrise message */
int katra_daemon_format_sunrise_insights(const daemon_insight_t* insights, size_t count,
                                          char* buffer, size_t buffer_size);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/* Get insight type name */
const char* katra_insight_type_name(insight_type_t type);

/* Generate unique insight ID */
void katra_daemon_generate_insight_id(char* id_out, size_t size);

/* Get daemon run history */
int katra_daemon_get_history(const char* ci_id, daemon_result_t** history, size_t* count);

/* Free history results */
void katra_daemon_free_history(daemon_result_t* history, size_t count);

#endif /* KATRA_DAEMON_H */
