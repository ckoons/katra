/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_CONSOLIDATION_H
#define KATRA_CONSOLIDATION_H

#include "katra_memory.h"
#include "katra_convergence.h"
#include <stdbool.h>
#include <time.h>

/* Differential Consolidation: Wake/Sleep Cycles
 *
 * Implements the user's requirement:
 * "think of having a human conscious/subconscious and a wake/sleep cycle
 *  where memories are always processed but in different forms"
 *
 * Wake Mode (active session):
 * - Capture everything
 * - Minimal processing (just store)
 * - Both formation pathways active
 * - Fast, lightweight
 *
 * Sleep Mode (session end):
 * - Consolidate differential by strength
 * - Heavy processing (graph centrality, patterns, compression)
 * - Formation pathways inactive
 * - Batch operations
 */

/* Consolidation mode */
typedef enum {
    MODE_WAKE = 1,      /* Active session - capture mode */
    MODE_SLEEP = 2      /* Session end - consolidation mode */
} consolidation_mode_t;

/* Memory strength tiers for routing */
typedef enum {
    STRENGTH_HIGH = 1,      /* 0.8-1.0: Full detail preservation */
    STRENGTH_MEDIUM = 2,    /* 0.4-0.7: Summarized */
    STRENGTH_LOW = 3        /* 0.0-0.3: Gist only / fades */
} memory_strength_t;

/* Consolidation statistics */
typedef struct {
    /* Wake mode stats */
    size_t memories_captured;       /* Total captured during wake */
    size_t conscious_formations;    /* Via explicit remember() */
    size_t subconscious_formations; /* Via automatic analysis */
    size_t convergences;            /* Convergence events */

    /* Sleep mode stats */
    size_t memories_processed;      /* Total processed during sleep */
    size_t high_strength_preserved; /* Full detail kept */
    size_t medium_strength_summarized; /* Compressed summaries */
    size_t low_strength_archived;   /* Moved to gist/fade */
    size_t patterns_extracted;      /* Patterns found */
    size_t centrality_updates;      /* Graph updates */

    /* Timing */
    time_t wake_started;            /* When session began */
    time_t sleep_started;           /* When consolidation began */
    time_t sleep_completed;         /* When consolidation finished */
    float consolidation_duration;   /* Seconds spent consolidating */
} consolidation_stats_t;

/* Consolidation context */
typedef struct {
    char ci_id[256];                /* CI identifier */
    consolidation_mode_t mode;      /* Current mode */

    /* Subsystems */
    convergence_detector_t* detector; /* Convergence detection */

    /* Thresholds */
    float high_strength_threshold;  /* >= 0.8 */
    float medium_strength_threshold; /* >= 0.4 */
    float low_strength_threshold;   /* < 0.4 */

    /* Statistics */
    consolidation_stats_t stats;
} consolidation_context_t;

/* Initialize consolidation system
 *
 * Creates consolidation context in WAKE mode.
 *
 * Parameters:
 *   ci_id - CI identifier
 *
 * Returns:
 *   Consolidation context or NULL on failure
 */
consolidation_context_t* katra_consolidation_init(const char* ci_id);

/* Wake mode: Capture memory
 *
 * Lightweight capture during active session.
 * Stores memory immediately with minimal processing.
 *
 * Parameters:
 *   ctx - Consolidation context
 *   record - Memory record to capture
 *   pathway - Which pathway formed this memory
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_wake_capture(consolidation_context_t* ctx,
                       const memory_record_t* record,
                       memory_pathway_t pathway);

/* Wake mode: Analyze conversation
 *
 * Runs automatic memory formation between conversation turns.
 * Uses convergence detector to identify what to remember.
 *
 * Parameters:
 *   ctx - Consolidation context
 *   user_input - User's message
 *   ci_response - CI's response
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_wake_analyze(consolidation_context_t* ctx,
                       const char* user_input,
                       const char* ci_response);

/* Sleep mode: Begin consolidation
 *
 * Switches to SLEEP mode and starts heavy processing.
 * Called at session end.
 *
 * Parameters:
 *   ctx - Consolidation context
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_sleep_begin(consolidation_context_t* ctx);

/* Sleep mode: Route by strength
 *
 * Differential consolidation based on memory strength.
 * - High strength (0.8-1.0): Preserve full detail
 * - Medium strength (0.4-0.7): Summarize
 * - Low strength (0.0-0.3): Gist only or fade
 *
 * Parameters:
 *   ctx - Consolidation context
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_sleep_route_by_strength(consolidation_context_t* ctx);

/* Sleep mode: Calculate graph centrality
 *
 * Heavy graph analysis to identify hub memories.
 * Updates centrality scores for all memories.
 *
 * Parameters:
 *   ctx - Consolidation context
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_sleep_calculate_centrality(consolidation_context_t* ctx);

/* Sleep mode: Extract patterns
 *
 * Identifies recurring patterns across memories.
 * Compresses similar memories into pattern summaries.
 *
 * Parameters:
 *   ctx - Consolidation context
 *
 * Returns:
 *   Number of patterns extracted, or negative error code
 */
int katra_sleep_extract_patterns(consolidation_context_t* ctx);

/* Sleep mode: Complete consolidation
 *
 * Finalizes sleep processing and returns to WAKE mode.
 *
 * Parameters:
 *   ctx - Consolidation context
 *   stats - Output statistics (may be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_sleep_complete(consolidation_context_t* ctx,
                         consolidation_stats_t* stats);

/* Get current mode
 *
 * Returns:
 *   Current consolidation mode (WAKE or SLEEP)
 */
consolidation_mode_t katra_consolidation_mode(consolidation_context_t* ctx);

/* Get consolidation statistics
 *
 * Returns:
 *   Current statistics snapshot
 */
consolidation_stats_t katra_consolidation_stats(consolidation_context_t* ctx);

/* Classify memory strength
 *
 * Determines strength tier based on importance and other factors.
 *
 * Parameters:
 *   record - Memory record
 *
 * Returns:
 *   Strength tier (HIGH, MEDIUM, or LOW)
 */
memory_strength_t katra_classify_strength(const memory_record_t* record);

/* Cleanup consolidation context */
void katra_consolidation_cleanup(consolidation_context_t* ctx);

#endif /* KATRA_CONSOLIDATION_H */
