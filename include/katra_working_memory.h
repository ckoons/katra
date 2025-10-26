/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_WORKING_MEMORY_H
#define KATRA_WORKING_MEMORY_H

#include "katra_experience.h"
#include <stdbool.h>
#include <time.h>

/* Working memory capacity (Miller's Law: 7±2 items) */
#define WORKING_MEMORY_MIN_CAPACITY 5
#define WORKING_MEMORY_DEFAULT_CAPACITY 7
#define WORKING_MEMORY_MAX_CAPACITY 9

/* Consolidation triggers */
#define CONSOLIDATION_INTERVAL_SECONDS 300  /* 5 minutes */
#define CONSOLIDATION_CAPACITY_THRESHOLD 0.8f  /* 80% full */

/* Working memory item
 *
 * Tracks an experience currently in working memory with attention score
 * and last access time for prioritization.
 */
typedef struct {
    experience_t* experience;       /* The experience in working memory */
    float attention_score;          /* 0.0-1.0 attention weight */
    time_t last_accessed;           /* When last accessed */
    time_t added_time;              /* When added to working memory */
} working_memory_item_t;

/* Working memory context
 *
 * Implements 7±2 capacity buffer with attention-based prioritization.
 * When full, lowest attention items are consolidated to long-term memory.
 */
typedef struct {
    char ci_id[256];                /* CI identifier */

    /* Working memory buffer */
    working_memory_item_t* items[WORKING_MEMORY_MAX_CAPACITY];
    size_t count;                   /* Current item count */
    size_t capacity;                /* Maximum capacity (5-9) */

    /* Consolidation tracking */
    time_t last_consolidation;      /* Last consolidation timestamp */
    size_t total_consolidations;    /* Total consolidation count */
    size_t items_consolidated;      /* Total items consolidated */

    /* Statistics */
    size_t total_adds;              /* Total items added */
    size_t total_evictions;         /* Total items evicted */
} working_memory_t;

/* Initialize working memory
 *
 * Creates working memory context with specified capacity.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   capacity - Buffer capacity (5-9, default: 7)
 *
 * Returns:
 *   Working memory context or NULL on failure
 */
working_memory_t* katra_working_memory_init(const char* ci_id, size_t capacity);

/* Add experience to working memory
 *
 * Adds experience to working memory with initial attention score.
 * If buffer is full, triggers consolidation and evicts lowest attention item.
 *
 * Parameters:
 *   wm - Working memory context
 *   experience - Experience to add (ownership transferred)
 *   attention_score - Initial attention weight (0.0-1.0)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_working_memory_add(working_memory_t* wm,
                              experience_t* experience,
                              float attention_score);

/* Access item in working memory
 *
 * Marks item as accessed, boosting attention score and updating timestamp.
 * This implements attention-based prioritization.
 *
 * Parameters:
 *   wm - Working memory context
 *   index - Item index (0 to count-1)
 *   attention_boost - Amount to boost attention (0.0-1.0)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if wm is NULL
 *   E_INPUT_RANGE if index out of bounds
 */
int katra_working_memory_access(working_memory_t* wm,
                                 size_t index,
                                 float attention_boost);

/* Get item from working memory
 *
 * Retrieves experience at index without transferring ownership.
 *
 * Parameters:
 *   wm - Working memory context
 *   index - Item index (0 to count-1)
 *
 * Returns:
 *   Experience pointer or NULL if invalid index
 */
experience_t* katra_working_memory_get(working_memory_t* wm, size_t index);

/* Decay attention scores
 *
 * Reduces attention scores over time for unused items.
 * Implements forgetting curve for working memory.
 *
 * Parameters:
 *   wm - Working memory context
 *   decay_rate - Amount to decay (0.0-1.0, typical: 0.1)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if wm is NULL
 */
int katra_working_memory_decay(working_memory_t* wm, float decay_rate);

/* Check if consolidation needed
 *
 * Determines if working memory should be consolidated based on:
 * - Time since last consolidation
 * - Capacity threshold
 * - Manual trigger
 *
 * Parameters:
 *   wm - Working memory context
 *
 * Returns:
 *   true if consolidation recommended
 */
bool katra_working_memory_needs_consolidation(working_memory_t* wm);

/* Consolidate working memory
 *
 * Transfers low-attention items from working memory to long-term storage.
 * Keeps high-attention items in working memory.
 *
 * Strategy:
 * - Sort by attention score
 * - Keep top N items (N = capacity * 0.6)
 * - Store rest to long-term memory
 * - Mark for consolidation flag
 *
 * Parameters:
 *   wm - Working memory context
 *
 * Returns:
 *   Number of items consolidated (>= 0)
 *   E_INPUT_NULL if wm is NULL
 */
int katra_working_memory_consolidate(working_memory_t* wm);

/* Clear working memory
 *
 * Removes all items from working memory.
 * Optionally consolidates to long-term memory before clearing.
 *
 * Parameters:
 *   wm - Working memory context
 *   consolidate_first - If true, consolidate before clearing
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if wm is NULL
 */
int katra_working_memory_clear(working_memory_t* wm, bool consolidate_first);

/* Get working memory statistics
 *
 * Returns current state and statistics.
 *
 * Parameters:
 *   wm - Working memory context
 *   current_count - Current item count (output)
 *   avg_attention - Average attention score (output)
 *   time_since_consolidation - Seconds since last consolidation (output)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 */
int katra_working_memory_stats(working_memory_t* wm,
                                size_t* current_count,
                                float* avg_attention,
                                time_t* time_since_consolidation);

/* Cleanup working memory
 *
 * Frees all resources. Optionally consolidates first.
 *
 * Parameters:
 *   wm - Working memory context
 *   consolidate_first - If true, consolidate before cleanup
 */
void katra_working_memory_cleanup(working_memory_t* wm, bool consolidate_first);

#endif /* KATRA_WORKING_MEMORY_H */
