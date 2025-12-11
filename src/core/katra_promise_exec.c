/* © 2025 Casey Koons All rights reserved */

/**
 * katra_promise_exec.c - Promise Execution Implementation
 *
 * Handles execution of different promise operation types.
 * Split from katra_promise_pool.c for maintainability.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

/* Project includes */
#include "katra_promise.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_psyche_common.h"
#include "katra_core_common.h"
#include "katra_tier1.h"

/* Internal promise data structure (must match katra_promise_pool.c) */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool cancelled;

    /* Operation-specific data */
    char* ci_id;
    char* query;
    size_t limit;
    memory_query_t* mem_query;
    recall_options_t* options;
} promise_internal_t;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int katra_execute_recall(katra_promise_t* promise, void* internal_ptr);
static int katra_execute_recall_synthesized(katra_promise_t* promise, void* internal_ptr);
static int katra_execute_query(katra_promise_t* promise, void* internal_ptr);

/* ============================================================================
 * PROMISE EXECUTION
 * ============================================================================ */

void katra_execute_promise(katra_promise_t* promise) {
    promise_internal_t* internal = (promise_internal_t*)promise->_internal;
    int result = KATRA_SUCCESS;

    pthread_mutex_lock(&internal->mutex);
    if (internal->cancelled) {
        promise->state = PROMISE_CANCELLED;
        promise->completed_at = time(NULL);
        pthread_cond_broadcast(&internal->cond);
        pthread_mutex_unlock(&internal->mutex);
        return;
    }
    promise->state = PROMISE_RUNNING;
    promise->started_at = time(NULL);
    pthread_mutex_unlock(&internal->mutex);

    switch (promise->op_type) {
        case PROMISE_OP_RECALL: {
            result = katra_execute_recall(promise, internal);
            break;
        }

        case PROMISE_OP_RECALL_SYNTHESIZED: {
            result = katra_execute_recall_synthesized(promise, internal);
            break;
        }

        case PROMISE_OP_QUERY: {
            result = katra_execute_query(promise, internal);
            break;
        }

        default:
            result = E_INTERNAL_NOTIMPL;
            break;
    }

    /* Update promise state */
    pthread_mutex_lock(&internal->mutex);
    if (internal->cancelled) {
        promise->state = PROMISE_CANCELLED;
    } else if (result == KATRA_SUCCESS) {
        promise->state = PROMISE_FULFILLED;
    } else {
        promise->state = PROMISE_REJECTED;
        promise->error_code = result;
        snprintf(promise->error_message, sizeof(promise->error_message),
                 "Operation failed: %s", katra_error_name(result));
    }
    promise->completed_at = time(NULL);
    pthread_cond_broadcast(&internal->cond);
    pthread_mutex_unlock(&internal->mutex);

    /* Call completion callback */
    if (promise->on_complete) {
        promise->on_complete(promise, promise->user_data);
    }
}

/* Execute basic recall operation */
static int katra_execute_recall(katra_promise_t* promise, void* internal_ptr) {
    promise_internal_t* internal = (promise_internal_t*)internal_ptr;
    memory_record_t** records = NULL;
    size_t count = 0;
    int result = KATRA_SUCCESS;

    memory_query_t query = {0};
    query.ci_id = internal->ci_id;
    query.limit = internal->limit > 0 ? internal->limit : DEFAULT_MEMORY_QUERY_LIMIT;

    result = tier1_query(&query, &records, &count);

    if (result == KATRA_SUCCESS && records && count > 0) {
        /* Filter by topic if specified */
        if (internal->query && strlen(internal->query) > 0) {
            memory_record_t** filtered = NULL;
            size_t filtered_count = 0;

            filtered = calloc(count, sizeof(memory_record_t*));
            if (filtered) {
                for (size_t i = 0; i < count; i++) {
                    if (records[i] && records[i]->content &&
                        katra_str_contains(records[i]->content, internal->query)) {
                        filtered[filtered_count++] = records[i];
                        records[i] = NULL;  /* Transfer ownership */
                    }
                }
                /* Free unmatched records */
                katra_memory_free_results(records, count);
                promise->result.recall.records = filtered;
                promise->result.recall.count = filtered_count;
            } else {
                promise->result.recall.records = records;
                promise->result.recall.count = count;
            }
        } else {
            promise->result.recall.records = records;
            promise->result.recall.count = count;
        }
    }

    return result;
}

/* Execute synthesized recall operation */
static int katra_execute_recall_synthesized(katra_promise_t* promise, void* internal_ptr) {
    promise_internal_t* internal = (promise_internal_t*)internal_ptr;
    synthesis_result_set_t* result_set = NULL;
    int result = KATRA_SUCCESS;

    result = katra_recall_synthesized(
        internal->ci_id,
        internal->query,
        internal->options,
        &result_set
    );

    if (result == KATRA_SUCCESS) {
        promise->result.synthesis.result_set = result_set;
    }

    return result;
}

/* Execute memory query operation */
static int katra_execute_query(katra_promise_t* promise, void* internal_ptr) {
    promise_internal_t* internal = (promise_internal_t*)internal_ptr;
    memory_record_t** records = NULL;
    size_t count = 0;
    int result = KATRA_SUCCESS;

    result = katra_memory_query(internal->mem_query, &records, &count);

    if (result == KATRA_SUCCESS) {
        promise->result.recall.records = records;
        promise->result.recall.count = count;
    }

    return result;
}
