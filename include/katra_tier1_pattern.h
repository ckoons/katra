/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_TIER1_PATTERN_H
#define KATRA_TIER1_PATTERN_H

#include "katra_memory.h"

/*
 * Pattern Detection for Memory Consolidation (Phase 3)
 *
 * Detects patterns in memory sets based on content similarity.
 * Groups similar memories, identifies outliers, and supports compression.
 */

/* Detect patterns in a set of memory records
 *
 * Groups similar memories into patterns based on keyword similarity.
 * Assigns pattern_id, pattern_frequency, and semantic_similarity to records.
 * Marks outliers (first, last, most important) as is_pattern_outlier.
 *
 * Parameters:
 *   records - Array of memory records to analyze
 *   count - Number of records in array
 *
 * Requirements:
 *   - Minimum 3 memories with >= 40% keyword similarity form a pattern
 *   - Outliers preserved: first, last, and highest importance
 */
void katra_tier1_detect_patterns(memory_record_t** records, size_t count);

/* Filter pattern outliers from archival set
 *
 * Removes pattern outliers from the array, preserving them from archival.
 * Returns modified array with only non-outlier memories to archive.
 *
 * Parameters:
 *   records - Array of memory records (modified in place)
 *   count - Number of records in array
 *
 * Returns:
 *   Number of records remaining after filtering outliers
 *
 * Note:
 *   Outlier records are freed. Caller must not access them after this call.
 */
size_t katra_tier1_filter_pattern_outliers(memory_record_t** records, size_t count);

#endif /* KATRA_TIER1_PATTERN_H */
