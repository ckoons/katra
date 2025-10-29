/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_health.c - Memory health monitoring and periodic maintenance
 *
 * Implements:
 * - Memory pressure awareness for graceful degradation
 * - Periodic consolidation to prevent tier1 overflow
 * - Health status reporting for long-running CIs
 */

/* System includes */
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_breathing_internal.h"

/* ============================================================================
 * MEMORY HEALTH MONITORING
 * ============================================================================ */

memory_health_t* get_memory_health(const char* ci_id) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "get_memory_health", "ci_id is NULL");
        return NULL;
    }

    /* Allocate health structure */
    memory_health_t* health = calloc(1, sizeof(memory_health_t));
    if (!health) {
        katra_report_error(E_SYSTEM_MEMORY, "get_memory_health",
                          "Failed to allocate health structure");
        return NULL;
    }

    /* Get tier1 statistics */
    memory_stats_t stats = {0};
    int result = katra_memory_stats(ci_id, &stats);
    if (result == KATRA_SUCCESS) {
        health->tier1_records = stats.tier1_records;
        health->tier1_bytes = stats.bytes_used;

        /* Calculate fill percentage */
        if (TIER1_MAX_ENTRIES > 0) {
            health->tier1_fill_percentage =
                ((float)health->tier1_records / (float)TIER1_MAX_ENTRIES) * 100.0f;
        } else {
            health->tier1_fill_percentage = 0.0f;
        }

        /* Determine memory pressure based on soft/hard limits */
        if (health->tier1_records >= BREATHING_TIER1_HARD_LIMIT) {
            health->memory_pressure = true;
            health->degraded_mode = true;  /* Critical - force consolidation */
            LOG_WARN("Memory pressure CRITICAL: %zu/%d records (%.1f%%)",
                    health->tier1_records, TIER1_MAX_ENTRIES,
                    health->tier1_fill_percentage);
        } else if (health->tier1_records >= BREATHING_TIER1_SOFT_LIMIT) {
            health->memory_pressure = true;
            health->degraded_mode = false;  /* Warning - suggest consolidation */
            LOG_INFO("Memory pressure WARNING: %zu/%d records (%.1f%%)",
                    health->tier1_records, TIER1_MAX_ENTRIES,
                    health->tier1_fill_percentage);
        } else {
            health->memory_pressure = false;
            health->degraded_mode = false;
        }
    }

    /* Get consolidation status from enhanced stats */
    enhanced_stats_t* stats_ptr = breathing_get_stats_ptr();
    if (stats_ptr) {
        health->last_consolidation = stats_ptr->last_consolidation;
        health->consolidation_count = stats_ptr->consolidation_count;
    }

    /* Check tier2 availability */
    bool tier2_status = katra_memory_tier2_enabled();
    health->tier2_available = tier2_status;
    health->tier2_enabled = tier2_status;

    return health;
}

/* ============================================================================
 * PERIODIC MAINTENANCE
 * ============================================================================ */

int breathe_periodic_maintenance(void) {
    if (!breathing_get_initialized()) {
        return E_INVALID_STATE;
    }

    const char* ci_id = breathing_get_ci_id();
    if (!ci_id) {
        return E_INVALID_STATE;
    }

    /* Get current time */
    time_t now = time(NULL);

    /* Get enhanced stats for last consolidation time */
    enhanced_stats_t* stats = breathing_get_stats_ptr();
    if (!stats) {
        return E_INVALID_STATE;
    }

    /* Check if consolidation is needed (6 hour interval) */
    time_t time_since_last = now - stats->last_consolidation;
    bool consolidation_needed = (time_since_last >= BREATHING_MAINTENANCE_INTERVAL_SECONDS);

    /* Also check memory pressure - force consolidation if critical */
    memory_health_t* health = get_memory_health(ci_id);
    if (health && health->degraded_mode) {
        LOG_WARN("Forcing consolidation due to critical memory pressure");
        consolidation_needed = true;
    }
    free(health);

    /* Run consolidation if needed */
    if (consolidation_needed) {
        LOG_INFO("Running periodic maintenance: consolidation due");

        int result = auto_consolidate();
        if (result == KATRA_SUCCESS) {
            /* Update consolidation timestamp */
            stats->last_consolidation = now;
            stats->consolidation_count++;
            LOG_INFO("Periodic consolidation completed (%zu total)",
                    stats->consolidation_count);
        } else {
            LOG_WARN("Periodic consolidation failed: %d", result);
            stats->failed_stores++;
        }

        /* Flush tier1 to disk for crash safety */
        int flush_result = tier1_flush(ci_id);
        if (flush_result == KATRA_SUCCESS) {
            LOG_DEBUG("Tier1 flushed to disk");
        } else {
            LOG_WARN("Tier1 flush failed: %d", flush_result);
        }

        return result;
    }

    /* No maintenance needed yet */
    LOG_DEBUG("Periodic maintenance check: no action needed "
             "(last consolidation %ld seconds ago)",
             (long)time_since_last);

    return KATRA_SUCCESS;
}
