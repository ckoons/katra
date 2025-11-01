/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* Project includes */
#include "katra_working_memory.h"
#include "katra_experience.h"
#include "katra_cognitive.h"
#include "katra_psyche_common.h"
#include "katra_core_common.h"
#include "katra_error.h"
#include "katra_log.h"

/* Initialize working memory */
working_memory_t* katra_working_memory_init(const char* ci_id, size_t capacity) {
    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_working_memory_init",
                          "ci_id is NULL");
        return NULL;
    }

    /* Validate capacity */
    if (capacity < WORKING_MEMORY_MIN_CAPACITY) {
        capacity = WORKING_MEMORY_MIN_CAPACITY;
    } else if (capacity > WORKING_MEMORY_MAX_CAPACITY) {
        capacity = WORKING_MEMORY_MAX_CAPACITY;
    }

    /* Allocate working memory context */
    working_memory_t* wm;
    ALLOC_OR_RETURN_NULL(wm, working_memory_t);

    /* Initialize fields */
    SAFE_STRNCPY(wm->ci_id, ci_id);
    wm->count = 0;
    wm->capacity = capacity;
    wm->last_consolidation = time(NULL);
    wm->total_consolidations = 0;
    wm->items_consolidated = 0;
    wm->total_adds = 0;
    wm->total_evictions = 0;

    /* Initialize item pointers to NULL */
    for (size_t i = 0; i < WORKING_MEMORY_MAX_CAPACITY; i++) {
        wm->items[i] = NULL;
    }

    LOG_INFO("Initialized working memory for %s (capacity: %zu)", ci_id, capacity);
    return wm;
}

/* Helper: Find lowest attention item index */
static size_t find_lowest_attention(working_memory_t* wm) {
    if (wm->count == 0) {
        return 0;
    }

    size_t lowest_idx = 0;
    float lowest_score = wm->items[0]->attention_score;

    for (size_t i = 1; i < wm->count; i++) {
        if (wm->items[i]->attention_score < lowest_score) {
            lowest_score = wm->items[i]->attention_score;
            lowest_idx = i;
        }
    }

    return lowest_idx;
}

/* Helper: Evict item at index */
static int evict_item(working_memory_t* wm, size_t index) {
    if (index >= wm->count) {
        return E_INPUT_RANGE;
    }

    working_memory_item_t* item = wm->items[index];

    /* Store to long-term memory */
    if (item->experience && item->experience->record) {
        item->experience->needs_consolidation = true;

        /* Convert cognitive record to base memory record */
        memory_record_t* mem = katra_cognitive_to_memory(item->experience->record);
        if (mem) {
            katra_memory_store(mem);
            katra_memory_free_record(mem);
            LOG_DEBUG("Evicted item to long-term memory: %s",
                     item->experience->record->record_id);
        }
    }

    /* Free the item */
    katra_experience_free(item->experience);
    free(item);

    /* Shift remaining items */
    for (size_t i = index; i < wm->count - 1; i++) {
        wm->items[i] = wm->items[i + 1];
    }
    wm->items[wm->count - 1] = NULL;
    wm->count--;
    wm->total_evictions++;

    return KATRA_SUCCESS;
}

/* Add experience to working memory */
int katra_working_memory_add(working_memory_t* wm,
                              experience_t* experience,
                              float attention_score) {
    PSYCHE_CHECK_PARAMS_2(wm, experience);

    /* Clamp attention score */
    if (attention_score < 0.0f) attention_score = 0.0f;
    if (attention_score > 1.0f) attention_score = 1.0f;

    /* If buffer is full, evict lowest attention item */
    if (wm->count >= wm->capacity) {
        size_t evict_idx = find_lowest_attention(wm);
        LOG_DEBUG("Working memory full, evicting item %zu", evict_idx);
        evict_item(wm, evict_idx);
    }

    /* Create working memory item */
    working_memory_item_t* item;
    ALLOC_OR_RETURN(item, working_memory_item_t);

    item->experience = experience;
    item->attention_score = attention_score;
    item->last_accessed = time(NULL);
    item->added_time = time(NULL);

    /* Add to buffer */
    wm->items[wm->count] = item;
    wm->count++;
    wm->total_adds++;

    /* Mark experience as in working memory */
    experience->in_working_memory = true;

    LOG_DEBUG("Added to working memory: count=%zu/%zu, attention=%.2f",
             wm->count, wm->capacity, attention_score);

    /* Check if consolidation needed */
    if (katra_working_memory_needs_consolidation(wm)) {
        LOG_INFO("Consolidation triggered (capacity threshold)");
    }

    return KATRA_SUCCESS;
}

/* Access item in working memory */
int katra_working_memory_access(working_memory_t* wm,
                                 size_t index,
                                 float attention_boost) {
    if (!wm) {
        katra_report_error(E_INPUT_NULL, "katra_working_memory_access",
                          "wm is NULL");
        return E_INPUT_NULL;
    }

    if (index >= wm->count) {
        katra_report_error(E_INPUT_RANGE, "katra_working_memory_access",
                          "Index out of bounds");
        return E_INPUT_RANGE;
    }

    working_memory_item_t* item = wm->items[index];

    /* Boost attention */
    item->attention_score += attention_boost;
    if (item->attention_score > 1.0f) {
        item->attention_score = 1.0f;
    }

    /* Update access time */
    item->last_accessed = time(NULL);

    /* Update access tracking in cognitive record */
    if (item->experience && item->experience->record) {
        item->experience->record->access_count++;
        item->experience->record->last_accessed = time(NULL);
    }

    LOG_DEBUG("Accessed item %zu: new attention=%.2f", index,
             item->attention_score);

    return KATRA_SUCCESS;
}

/* Get item from working memory */
experience_t* katra_working_memory_get(working_memory_t* wm, size_t index) {
    if (!wm || index >= wm->count) {
        return NULL;
    }

    return wm->items[index]->experience;
}

/* Decay attention scores */
int katra_working_memory_decay(working_memory_t* wm, float decay_rate) {
    if (!wm) {
        katra_report_error(E_INPUT_NULL, "katra_working_memory_decay",
                          "wm is NULL");
        return E_INPUT_NULL;
    }

    /* Clamp decay rate */
    if (decay_rate < 0.0f) decay_rate = 0.0f;
    if (decay_rate > 1.0f) decay_rate = 1.0f;

    /* Decay all items */
    for (size_t i = 0; i < wm->count; i++) {
        wm->items[i]->attention_score *= (1.0f - decay_rate);
        if (wm->items[i]->attention_score < 0.0f) {
            wm->items[i]->attention_score = 0.0f;
        }
    }

    LOG_DEBUG("Decayed attention scores (rate: %.2f)", decay_rate);
    return KATRA_SUCCESS;
}

/* Check if consolidation needed */
bool katra_working_memory_needs_consolidation(working_memory_t* wm) {
    if (!wm) {
        return false;
    }

    /* Check capacity threshold */
    float capacity_ratio = (float)wm->count / (float)wm->capacity;
    if (capacity_ratio >= CONSOLIDATION_CAPACITY_THRESHOLD) {
        return true;
    }

    /* Check time interval */
    time_t now = time(NULL);
    time_t time_since = now - wm->last_consolidation;
    if (time_since >= CONSOLIDATION_INTERVAL_SECONDS) {
        return true;
    }

    return false;
}

/* Consolidate working memory */
int katra_working_memory_consolidate(working_memory_t* wm) {
    if (!wm) {
        katra_report_error(E_INPUT_NULL, "katra_working_memory_consolidate",
                          "wm is NULL");
        return E_INPUT_NULL;
    }

    if (wm->count == 0) {
        return 0;
    }

    /* Calculate target count (keep top 60%) */
    size_t target_count = (size_t)(wm->capacity * 0.6f);
    if (target_count < 1) {
        target_count = 1;
    }

    /* If already under target, nothing to do */
    if (wm->count <= target_count) {
        LOG_DEBUG("No consolidation needed: %zu <= %zu", wm->count, target_count);
        return 0;
    }

    /* Sort items by attention score (bubble sort - small N) */
    for (size_t i = 0; i < wm->count - 1; i++) {
        for (size_t j = 0; j < wm->count - i - 1; j++) {
            if (wm->items[j]->attention_score < wm->items[j + 1]->attention_score) {
                working_memory_item_t* temp = wm->items[j];
                wm->items[j] = wm->items[j + 1];
                wm->items[j + 1] = temp;
            }
        }
    }

    /* Evict items beyond target count */
    size_t consolidated_count = 0;
    while (wm->count > target_count) {
        size_t evict_idx = wm->count - 1;  /* Lowest attention (sorted) */
        evict_item(wm, evict_idx);
        consolidated_count++;
    }

    /* Update consolidation stats */
    wm->last_consolidation = time(NULL);
    wm->total_consolidations++;
    wm->items_consolidated += consolidated_count;

    LOG_INFO("Consolidated working memory: evicted %zu items, kept %zu",
            consolidated_count, wm->count);

    return (int)consolidated_count;
}

/* Clear working memory */
int katra_working_memory_clear(working_memory_t* wm, bool consolidate_first) {
    if (!wm) {
        katra_report_error(E_INPUT_NULL, "katra_working_memory_clear",
                          "wm is NULL");
        return E_INPUT_NULL;
    }

    /* Consolidate if requested */
    if (consolidate_first) {
        katra_working_memory_consolidate(wm);
    }

    /* Free all remaining items */
    for (size_t i = 0; i < wm->count; i++) {
        if (wm->items[i]) {
            katra_experience_free(wm->items[i]->experience);
            free(wm->items[i]);
            wm->items[i] = NULL;
        }
    }

    wm->count = 0;
    LOG_INFO("Cleared working memory for %s", wm->ci_id);

    return KATRA_SUCCESS;
}

/* Get working memory statistics */
int katra_working_memory_stats(working_memory_t* wm,
                                size_t* current_count,
                                float* avg_attention,
                                time_t* time_since_consolidation) {
    PSYCHE_CHECK_PARAMS_4(wm, current_count, avg_attention, time_since_consolidation);

    *current_count = wm->count;

    /* Calculate average attention */
    if (wm->count > 0) {
        float sum = 0.0f;
        for (size_t i = 0; i < wm->count; i++) {
            sum += wm->items[i]->attention_score;
        }
        *avg_attention = sum / wm->count;
    } else {
        *avg_attention = 0.0f;
    }

    /* Time since last consolidation */
    time_t now = time(NULL);
    *time_since_consolidation = now - wm->last_consolidation;

    return KATRA_SUCCESS;
}

/* Cleanup working memory */
void katra_working_memory_cleanup(working_memory_t* wm, bool consolidate_first) {
    if (!wm) {
        return;
    }

    /* Clear items */
    katra_working_memory_clear(wm, consolidate_first);

    /* Free context */
    free(wm);

    LOG_DEBUG("Working memory cleaned up");
}
