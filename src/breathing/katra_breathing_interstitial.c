/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_interstitial.c - Automatic capture and consolidation
 *
 * Interstitial capture, auto-consolidation, context loading
 */

/* System includes */
#include <string.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_breathing_internal.h"

/* ============================================================================
 * INTERSTITIAL CAPTURE - Automatic thought extraction
 * ============================================================================ */

int capture_significant_thoughts(const char* text) {
    if (!breathing_get_initialized() || !text) {
        return E_INVALID_STATE;
    }

    /* GUIDELINE_APPROVED: Pattern detection keywords for significance detection */
    /* Simple heuristic: sentences with significance markers */
    const char* markers[] = {
        "important", "significant", "critical", "learned", /* GUIDELINE_APPROVED */
        "realized", "discovered", "insight", "pattern", /* GUIDELINE_APPROVED */
        "decided", "understand", NULL /* GUIDELINE_APPROVED */
    };
    /* GUIDELINE_APPROVED_END */

    /* TODO: More sophisticated natural language processing */
    /* For now, check if text contains significance markers */

    for (int i = 0; markers[i] != NULL; i++) {
        if (strstr(text, markers[i]) != NULL) {
            LOG_DEBUG("Captured significant thought: %.50s...", text);
            return remember(text, WHY_INTERESTING);
        }
    }

    return KATRA_SUCCESS;  /* Not significant enough to capture */
}

void mark_significant(void) {
    /* This function would need access to g_current_thought */
    /* For now, it's a placeholder for future implementation */
    LOG_DEBUG("mark_significant() called");
}

/* ============================================================================
 * INVISIBLE CONSOLIDATION
 * ============================================================================ */

int auto_consolidate(void) {
    if (!breathing_get_initialized()) {
        return E_INVALID_STATE;
    }

    const char* ci_id = breathing_get_ci_id();
    LOG_DEBUG("Auto-consolidating memories for %s", ci_id);

    /* Archive old memories (7+ days old) */
    size_t archived = 0;
    int result = katra_memory_archive(ci_id, 7, &archived);

    if (result == KATRA_SUCCESS) {
        LOG_INFO("Auto-consolidation: archived %zu memories", archived);
    }

    return result;
}

int load_context(void) {
    if (!breathing_get_initialized()) {
        return E_INVALID_STATE;
    }

    const char* ci_id = breathing_get_ci_id();
    LOG_DEBUG("Loading context for %s", ci_id);

    /* Get recent significant memories */
    size_t count = 0;
    char** memories = relevant_memories(&count);

    if (count > 0) {
        LOG_INFO("Loaded %zu relevant memories into context", count);
        free_memory_list(memories, count);

        /* Track stats */
        breathing_track_context_load(count);
    }

    return KATRA_SUCCESS;
}
