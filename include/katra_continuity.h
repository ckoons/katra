/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_CONTINUITY_H
#define KATRA_CONTINUITY_H

#include "katra_memory.h"
#include "katra_tier2.h"

/* Continuity: Day-to-day memory continuity through sunrise/sunset
 *
 * Implements daily memory checkpoints to provide CI with continuity across
 * sessions. Sundown creates a digest of the day's interactions and experiences.
 * Sunrise loads the previous day's summary to restore context.
 *
 * Storage: Special Tier 2 digest with DIGEST_TYPE_INTERACTION, period_type DAILY
 * Organization: ~/.katra/memory/tier2/daily/YYYY-MM-DD.json
 */

/* Daily summary statistics */
typedef struct {
    int interaction_count;      /* Total interactions today */
    int questions_asked;        /* Questions user asked */
    int tasks_completed;        /* Tasks completed */
    int errors_encountered;     /* Errors encountered */
    float avg_importance;       /* Average importance of memories */
} daily_stats_t;

/* Sundown: Create end-of-day summary
 *
 * Queries today's Tier 1 memories, generates a summary digest,
 * and stores it as a special Tier 2 daily digest.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   summary - Optional custom summary (NULL = auto-generate)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id is NULL
 *   E_SYSTEM_MEMORY if allocation fails
 *   E_SYSTEM_FILE if write fails
 */
int katra_sundown_basic(const char* ci_id, const char* summary);

/* Sunrise: Load previous day's summary
 *
 * Queries yesterday's daily digest from Tier 2.
 * Returns summary for CI to restore context.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   digest - Pointer to receive yesterday's digest (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success (even if no prior day found)
 *   E_INPUT_NULL if ci_id or digest is NULL
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_sunrise_basic(const char* ci_id, digest_record_t** digest);

/* Get daily statistics
 *
 * Analyzes today's Tier 1 memories and returns statistics.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   stats - Statistics structure to fill
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id or stats is NULL
 */
int katra_get_daily_stats(const char* ci_id, daily_stats_t* stats);

#endif /* KATRA_CONTINUITY_H */
