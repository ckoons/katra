/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_CONVERGENCE_H
#define KATRA_CONVERGENCE_H

#include "katra_memory.h"
#include "katra_graph.h"
#include "katra_vector.h"
#include "katra_tier1_index.h"
#include <stdbool.h>
#include <time.h>

/* Dual-Path Memory System: Convergence Detection
 *
 * This implements the user's requirement:
 * "when your active memories coincide then they should get an upvote
 *  or marked stronger because they were 'obviously important'"
 *
 * Two pathways to memory formation:
 * 1. Conscious: Explicit remember() calls, marked important
 * 2. Subconscious: Automatic analysis of conversation
 *
 * When both paths identify the same memory → Convergence → Strengthen
 */

/* Memory formation pathway */
typedef enum {
    PATHWAY_CONSCIOUS = 1,      /* Explicit user action */
    PATHWAY_SUBCONSCIOUS = 2,   /* Automatic analysis */
    PATHWAY_BOTH = 3            /* Convergence detected */
} memory_pathway_t;

/* Convergence signal */
typedef struct {
    char record_id[256];        /* Memory that converged */
    float conscious_strength;   /* Strength from conscious path (0.0-1.0) */
    float subconscious_strength;/* Strength from subconscious path (0.0-1.0) */
    float convergence_score;    /* Combined score (0.0-1.0) */

    /* Evidence */
    bool explicit_marker;       /* User said "remember this" */
    bool graph_hub;             /* High centrality in graph */
    bool semantic_match;        /* Multiple similar memories */
    bool fts_match;             /* FTS finds related content */

    time_t detected;            /* When convergence detected */
} convergence_signal_t;

/* Automatic memory candidate */
typedef struct {
    char* content;              /* What to remember */
    float importance;           /* Auto-calculated importance */
    memory_type_t type;         /* Memory type */

    /* Rationale */
    char* reason;               /* Why this is memorable */
    bool decision_made;         /* Contains a decision */
    bool question_asked;        /* Contains a question */
    bool knowledge_shared;      /* Contains new knowledge */
    bool pattern_detected;      /* Recurring pattern */

    time_t timestamp;           /* When identified */
} auto_memory_candidate_t;

/* Convergence detector context */
typedef struct {
    char ci_id[256];            /* CI identifier */

    /* Storage backends */
    graph_store_t* graph;       /* Graph storage */
    vector_store_t* vectors;    /* Vector similarity */

    /* Statistics */
    size_t conscious_memories;  /* Memories via conscious path */
    size_t subconscious_memories; /* Memories via subconscious path */
    size_t convergences_detected; /* Times convergence occurred */
    size_t memories_strengthened; /* Memories boosted */

    /* Thresholds */
    float convergence_threshold; /* Min score for convergence (default 0.7) */
    float importance_boost;      /* How much to boost (default 0.2) */
    int time_window_hours;       /* Look-back window (default 24) */
} convergence_detector_t;

/* Initialize convergence detector
 *
 * Creates detector with access to all storage backends.
 *
 * Parameters:
 *   ci_id - CI identifier
 *
 * Returns:
 *   Detector context or NULL on failure
 */
convergence_detector_t* katra_convergence_init(const char* ci_id);

/* Analyze conversation for automatic memories
 *
 * The "what should I remember" routine.
 * Runs between conversation turns to identify memorable content.
 *
 * Parameters:
 *   detector - Convergence detector context
 *   user_input - User's message
 *   ci_response - CI's response
 *   candidates - Output array of memory candidates
 *   count - Number of candidates identified
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 */
int katra_analyze_conversation(convergence_detector_t* detector,
                               const char* user_input,
                               const char* ci_response,
                               auto_memory_candidate_t*** candidates,
                               size_t* count);

/* Detect convergence
 *
 * Checks if a memory exists in multiple pathways/systems.
 * Uses graph, vector, and FTS to find related memories.
 *
 * Parameters:
 *   detector - Convergence detector context
 *   candidate - Automatic memory candidate
 *   signal - Output convergence signal (NULL if no convergence)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_NOT_FOUND if no convergence detected
 */
int katra_detect_convergence(convergence_detector_t* detector,
                             const auto_memory_candidate_t* candidate,
                             convergence_signal_t** signal);

/* Strengthen converged memory
 *
 * Boosts importance when convergence detected.
 * Updates all storage backends (graph centrality, importance score).
 *
 * Parameters:
 *   detector - Convergence detector context
 *   signal - Convergence signal
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_strengthen_converged(convergence_detector_t* detector,
                               const convergence_signal_t* signal);

/* Store automatic memory
 *
 * Stores memory from subconscious pathway.
 * Checks for convergence before storing.
 *
 * Parameters:
 *   detector - Convergence detector context
 *   candidate - Memory candidate
 *   convergence_detected - Output: true if convergence found
 *
 * Returns:
 *   Record ID of stored memory, or NULL on failure
 */
char* katra_store_automatic_memory(convergence_detector_t* detector,
                                   const auto_memory_candidate_t* candidate,
                                   bool* convergence_detected);

/* Get convergence statistics
 *
 * Returns stats about dual-path memory formation.
 *
 * Parameters:
 *   detector - Convergence detector context
 *   conscious - Conscious pathway count
 *   subconscious - Subconscious pathway count
 *   converged - Convergence count
 *   boost_ratio - Average importance boost (0.0-1.0)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_convergence_stats(convergence_detector_t* detector,
                            size_t* conscious,
                            size_t* subconscious,
                            size_t* converged,
                            float* boost_ratio);

/* Free memory candidate */
void katra_free_memory_candidate(auto_memory_candidate_t* candidate);

/* Free convergence signal */
void katra_free_convergence_signal(convergence_signal_t* signal);

/* Cleanup convergence detector */
void katra_convergence_cleanup(convergence_detector_t* detector);

#endif /* KATRA_CONVERGENCE_H */
