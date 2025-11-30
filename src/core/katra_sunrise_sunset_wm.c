/* © 2025 Casey Koons All rights reserved */

/* Katra Working Memory Snapshot - Capture/Restore for sunrise/sunset */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_sunrise_sunset.h"
#include "katra_experience.h"
#include "katra_working_memory.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_core_common.h"
#include "katra_psyche_common.h"

/* ============================================================================
 * WORKING MEMORY SNAPSHOT FUNCTIONS (Phase 7.2)
 * ============================================================================ */

/* Helper: Create experience from content for restore */
static experience_t* create_experience_from_content(const char* ci_id, const char* content) {
    if (!content) {
        return NULL;
    }

    /* Allocate experience */
    experience_t* experience = calloc(1, sizeof(experience_t));
    if (!experience) {
        return NULL;
    }

    /* Allocate cognitive record */
    cognitive_record_t* record = calloc(1, sizeof(cognitive_record_t));
    if (!record) {
        free(experience);
        return NULL;
    }

    /* Generate unique record ID */
    char record_id[KATRA_BUFFER_SMALL];
    snprintf(record_id, sizeof(record_id), "wm_restore_%ld_%d",
             (long)time(NULL), rand() % SUNRISE_RAND_MODULO);
    record->record_id = katra_safe_strdup(record_id);
    record->timestamp = time(NULL);
    record->type = MEMORY_TYPE_EXPERIENCE;
    record->importance = SUNRISE_DEFAULT_IMPORTANCE;
    record->content = katra_safe_strdup(content);
    record->ci_id = katra_safe_strdup(ci_id);
    record->thought_type = THOUGHT_TYPE_OBSERVATION;
    record->confidence = SUNRISE_DEFAULT_CONFIDENCE;
    record->access_count = 0;
    record->last_accessed = time(NULL);

    experience->record = record;

    /* Detect emotion from content */
    katra_detect_emotion(content, &experience->emotion);
    experience->in_working_memory = false;
    experience->needs_consolidation = false;

    return experience;
}

/* Capture working memory state for sunset */
wm_state_snapshot_t* katra_wm_capture(working_memory_t* wm) {
    if (!wm) {
        return NULL;
    }

    wm_state_snapshot_t* snapshot = calloc(1, sizeof(wm_state_snapshot_t));
    if (!snapshot) {
        return NULL;
    }

    snapshot->capacity = wm->capacity;
    snapshot->last_consolidation = wm->last_consolidation;
    snapshot->total_consolidations = wm->total_consolidations;
    snapshot->item_count = wm->count;

    if (wm->count == 0) {
        snapshot->items = NULL;
        return snapshot;
    }

    snapshot->items = calloc(wm->count, sizeof(wm_item_snapshot_t));
    if (!snapshot->items) {
        free(snapshot);
        return NULL;
    }

    for (size_t i = 0; i < wm->count; i++) {
        if (wm->items[i] && wm->items[i]->experience && wm->items[i]->experience->record) {
            /* Copy content summary - content is in the cognitive record */
            const char* content = wm->items[i]->experience->record->content;
            if (content) {
                strncpy(snapshot->items[i].content, content,
                        sizeof(snapshot->items[i].content) - 1);
                snapshot->items[i].content[sizeof(snapshot->items[i].content) - 1] = '\0';
            }
            snapshot->items[i].attention_score = wm->items[i]->attention_score;
            snapshot->items[i].added_time = wm->items[i]->added_time;
            snapshot->items[i].last_accessed = wm->items[i]->last_accessed;
        }
    }

    LOG_INFO("Captured working memory snapshot: %zu items", wm->count);
    return snapshot;
}

/* Restore working memory from snapshot */
int katra_wm_restore(working_memory_t* wm, const wm_state_snapshot_t* snapshot) {
    if (!wm || !snapshot) {
        return E_INPUT_NULL;
    }

    /* Clear current working memory first */
    katra_working_memory_clear(wm, false);

    /* Restore metadata */
    wm->last_consolidation = snapshot->last_consolidation;
    wm->total_consolidations = snapshot->total_consolidations;

    /* Restore items */
    for (size_t i = 0; i < snapshot->item_count && i < wm->capacity; i++) {
        if (strlen(snapshot->items[i].content) > 0) {
            /* Create experience from snapshot content */
            experience_t* exp = create_experience_from_content(
                wm->ci_id,
                snapshot->items[i].content
            );
            if (exp) {
                katra_working_memory_add(wm, exp, snapshot->items[i].attention_score);
                /* Note: attention score and timestamps may not be fully restored
                 * due to katra_working_memory_add resetting some fields */
            }
        }
    }

    LOG_INFO("Restored working memory: %zu items (from %zu in snapshot)",
             wm->count, snapshot->item_count);
    return KATRA_SUCCESS;
}

/* Free working memory snapshot */
void katra_wm_snapshot_free(wm_state_snapshot_t* snapshot) {
    if (!snapshot) {
        return;
    }
    free(snapshot->items);
    free(snapshot);
}
