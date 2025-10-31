/* © 2025 Casey Koons All rights reserved */

/* Phase 5 Common Utilities
 *
 * Shared utilities used across all Phase 5 subcomponents.
 * Reduces code duplication and ensures consistent patterns.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "katra_phase5.h"
#include "katra_error.h"

/* Generate unique ID with prefix
 *
 * Creates IDs like "pattern_naming_123" or "query_456"
 * Caller must free returned string.
 */
char* phase5_generate_id(const char* prefix, size_t* counter) {
    if (!prefix || !counter) {
        return NULL;
    }

    char* id = malloc(PHASE5_QUERY_ID_SIZE);
    if (!id) {
        return NULL;
    }

    snprintf(id, PHASE5_QUERY_ID_SIZE, "%s_%zu", prefix, (*counter)++);
    return id;
}

/* Calculate weighted confidence from multiple factors
 *
 * Takes up to 5 factors with weights and computes weighted average.
 * All weights should sum to 1.0 for normalized output.
 */
float phase5_calculate_confidence(const phase5_confidence_calc_t* calc) {
    if (!calc || calc->factor_count == 0 || calc->factor_count > 5) {
        return 0.5f;  /* Default: unknown */
    }

    float confidence = 0.0f;
    for (size_t i = 0; i < calc->factor_count; i++) {
        confidence += calc->factors[i] * calc->weights[i];
    }

    return confidence;
}

/* Safe strdup with NULL check
 *
 * Duplicates string and checks for allocation failure.
 * Returns KATRA_SUCCESS or E_SYSTEM_MEMORY.
 * Sets *dest to NULL on failure.
 */
int phase5_safe_strdup(char** dest, const char* src) {
    if (!dest) {
        return E_INPUT_NULL;
    }

    if (!src) {
        *dest = NULL;
        return KATRA_SUCCESS;  /* NULL source is allowed */
    }

    *dest = strdup(src);
    if (!*dest) {
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}

/* Free string array utility
 *
 * Frees each string in array, then frees array itself.
 * Safe to call with NULL array or count=0.
 */
void phase5_free_string_array(char** array, size_t count) {
    if (!array) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(array[i]);
    }
    free(array);
}
