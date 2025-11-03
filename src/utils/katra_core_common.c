/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdlib.h>

/* Project includes */
#include "katra_core_common.h"

/* Free array of strings */
void katra_free_string_array(char** strings, size_t count) {
    if (!strings) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(strings[i]);
    }
    free(strings);
}

/* Free array with custom free function */
void katra_free_array(void** items, size_t count, katra_free_fn_t free_fn) {
    if (!items) {
        return;
    }

    if (free_fn) {
        for (size_t i = 0; i < count; i++) {
            if (items[i]) {
                free_fn(items[i]);
            }
        }
    }
    free(items);
}

/* ============================================================================
 * NLP STOP WORDS - Common words filtered in keyword extraction
 * ============================================================================ */

/* GUIDELINE_APPROVED: NLP stop words for keyword extraction */
const char* const KATRA_STOP_WORDS[] = {
    "the", "this", "that", "these", "those", /* GUIDELINE_APPROVED */
    "with", "from", "have", "has", "been", /* GUIDELINE_APPROVED */
    "will", "would", "could", "should", /* GUIDELINE_APPROVED */
    "what", "when", "where", "which", "while", /* GUIDELINE_APPROVED */
    "your", "their", "there", "here", /* GUIDELINE_APPROVED */
    NULL /* GUIDELINE_APPROVED */
};
/* GUIDELINE_APPROVED_END */
