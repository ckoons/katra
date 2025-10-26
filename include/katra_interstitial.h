/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_INTERSTITIAL_H
#define KATRA_INTERSTITIAL_H

#include "katra_working_memory.h"
#include "katra_experience.h"
#include <stdbool.h>
#include <time.h>

/* Cognitive boundary types
 *
 * Boundaries represent transitions in cognitive state that trigger
 * different consolidation strategies.
 */
typedef enum {
    BOUNDARY_TOPIC_SHIFT = 0,       /* Subject matter change */
    BOUNDARY_TEMPORAL_GAP = 1,      /* Time gap in interaction */
    BOUNDARY_CONTEXT_SWITCH = 2,    /* Mode/context change */
    BOUNDARY_EMOTIONAL_PEAK = 3,    /* Strong emotional transition */
    BOUNDARY_CAPACITY_LIMIT = 4,    /* Working memory full */
    BOUNDARY_SESSION_END = 5,       /* Explicit session termination */
    BOUNDARY_NONE = 6               /* No boundary detected */
} boundary_type_t;

/* Boundary detection thresholds */
#define TEMPORAL_GAP_SECONDS 30          /* 30 second gap */
#define EMOTIONAL_PEAK_DELTA 0.5f        /* 50% valence change */
#define TOPIC_SIMILARITY_THRESHOLD 0.3f  /* Low similarity = shift */

/* Boundary event
 *
 * Represents a detected cognitive boundary with context.
 */
typedef struct {
    boundary_type_t type;           /* Type of boundary */
    time_t timestamp;               /* When detected */
    char description[256];          /* Human-readable description */

    /* Context */
    experience_t* prev_experience;  /* Experience before boundary */
    experience_t* curr_experience;  /* Experience after boundary */

    /* Metrics */
    float confidence;               /* Detection confidence (0.0-1.0) */
    float topic_similarity;         /* Similarity score (0.0-1.0) */
    time_t time_gap;                /* Time gap in seconds */
    float emotional_delta;          /* Emotional change magnitude */
} boundary_event_t;

/* Interstitial processor context
 *
 * Manages boundary detection and consolidation strategies.
 */
typedef struct {
    char ci_id[256];                /* CI identifier */

    /* State tracking */
    experience_t* last_experience;  /* Last processed experience */
    time_t last_interaction;        /* Last interaction timestamp */
    boundary_event_t* last_boundary;/* Last detected boundary */

    /* Statistics */
    size_t total_boundaries;        /* Total boundaries detected */
    size_t boundaries_by_type[7];   /* Count per boundary type */
    size_t associations_formed;     /* Total associations created */
    size_t patterns_extracted;      /* Total patterns found */
} interstitial_processor_t;

/* Initialize interstitial processor
 *
 * Creates processor context for boundary detection and consolidation.
 *
 * Parameters:
 *   ci_id - CI identifier
 *
 * Returns:
 *   Processor context or NULL on failure
 */
interstitial_processor_t* katra_interstitial_init(const char* ci_id);

/* Detect boundary
 *
 * Analyzes current experience against previous state to detect
 * cognitive boundaries.
 *
 * Parameters:
 *   processor - Interstitial processor context
 *   experience - Current experience to analyze
 *
 * Returns:
 *   Boundary event (type=BOUNDARY_NONE if no boundary detected)
 */
boundary_event_t* katra_detect_boundary(interstitial_processor_t* processor,
                                        experience_t* experience);

/* Process boundary
 *
 * Executes consolidation strategy appropriate for boundary type.
 * Different boundaries trigger different consolidation behaviors:
 *
 * - TOPIC_SHIFT: Form associations, extract patterns
 * - TEMPORAL_GAP: Consolidate to long-term, reset working memory
 * - CONTEXT_SWITCH: Save context, load new context
 * - EMOTIONAL_PEAK: Mark high-importance, boost attention
 * - CAPACITY_LIMIT: Standard consolidation
 * - SESSION_END: Full consolidation, save state
 *
 * Parameters:
 *   processor - Interstitial processor context
 *   boundary - Detected boundary event
 *   wm - Working memory context
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 */
int katra_process_boundary(interstitial_processor_t* processor,
                           boundary_event_t* boundary,
                           working_memory_t* wm);

/* Detect topic shift
 *
 * Compares content similarity between experiences to detect topic changes.
 * Uses simple keyword-based similarity (future: semantic embeddings).
 *
 * Parameters:
 *   prev - Previous experience
 *   curr - Current experience
 *
 * Returns:
 *   Similarity score 0.0-1.0 (lower = more different)
 */
float katra_topic_similarity(const experience_t* prev, const experience_t* curr);

/* Detect emotional peak
 *
 * Checks for significant emotional state changes.
 *
 * Parameters:
 *   prev - Previous experience
 *   curr - Current experience
 *
 * Returns:
 *   Emotional delta magnitude (0.0-2.0)
 */
float katra_emotional_delta(const experience_t* prev, const experience_t* curr);

/* Form associations
 *
 * Creates associations between related experiences during consolidation.
 * Used during topic shift boundaries to link related thoughts.
 *
 * Parameters:
 *   processor - Interstitial processor context
 *   experiences - Array of experiences to associate
 *   count - Number of experiences
 *
 * Returns:
 *   Number of associations formed
 */
int katra_form_associations(interstitial_processor_t* processor,
                            experience_t** experiences,
                            size_t count);

/* Extract patterns
 *
 * Identifies recurring patterns in experiences during consolidation.
 * Looks for:
 * - Repeated questions
 * - Common themes
 * - Behavioral patterns
 *
 * Parameters:
 *   processor - Interstitial processor context
 *   experiences - Array of experiences
 *   count - Number of experiences
 *   patterns - Output array of pattern descriptions
 *   pattern_count - Number of patterns found
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 */
int katra_extract_patterns(interstitial_processor_t* processor,
                           experience_t** experiences,
                           size_t count,
                           char*** patterns,
                           size_t* pattern_count);

/* Get boundary type name */
const char* katra_boundary_type_name(boundary_type_t type);

/* Free boundary event */
void katra_boundary_free(boundary_event_t* boundary);

/* Cleanup interstitial processor */
void katra_interstitial_cleanup(interstitial_processor_t* processor);

#endif /* KATRA_INTERSTITIAL_H */
