/* © 2025 Casey Koons All rights reserved */

/**
 * katra_promise.c - Promise Operations for Async Memory Recall (Phase 10)
 *
 * Implements promise creation, awaiting, cancellation, and result retrieval.
 * Thread pool management is in katra_promise_pool.c.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

/* Project includes */
#include "katra_promise.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_psyche_common.h"
#include "katra_core_common.h"

/* Use E_SYSTEM_TIMEOUT for timeout errors */
#define E_TIMEOUT E_SYSTEM_TIMEOUT

/* ============================================================================
 * POOL INTERFACE (implemented in katra_promise_pool.c)
 * ============================================================================ */

/* Promise creation and queue */
extern katra_promise_t* katra_pool_create_promise(promise_op_type_t op_type);
extern int katra_pool_enqueue_promise(katra_promise_t* promise);
extern void katra_pool_free_internal(void* internal_ptr);

/* Internal structure access */
extern void* katra_pool_get_internal(katra_promise_t* promise);
extern int katra_pool_set_ci_id(void* internal_ptr, const char* ci_id);
extern int katra_pool_set_query(void* internal_ptr, const char* query);
extern void katra_pool_set_limit(void* internal_ptr, size_t limit);
extern int katra_pool_set_options(void* internal_ptr, const recall_options_t* options);
extern int katra_pool_set_mem_query(void* internal_ptr, const memory_query_t* query);

/* ============================================================================
 * INTERNAL PROMISE STRUCTURE (matches pool definition)
 * ============================================================================ */

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool cancelled;
    /* Other fields managed by pool */
} promise_internal_t;

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static double get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

/* ============================================================================
 * ASYNC OPERATIONS - Create and enqueue promises
 * ============================================================================ */

katra_promise_t* katra_recall_async(const char* ci_id,
                                     const char* topic,
                                     size_t limit,
                                     promise_callback_fn callback,
                                     void* user_data) {
    if (!ci_id || !topic) {
        katra_report_error(E_INPUT_NULL, "katra_recall_async", "NULL parameter");
        return NULL;
    }

    katra_promise_t* promise = katra_pool_create_promise(PROMISE_OP_RECALL);
    if (!promise) return NULL;

    void* internal = katra_pool_get_internal(promise);
    if (katra_pool_set_ci_id(internal, ci_id) != KATRA_SUCCESS ||
        katra_pool_set_query(internal, topic) != KATRA_SUCCESS) {
        katra_promise_free(promise);
        return NULL;
    }
    katra_pool_set_limit(internal, limit);

    promise->on_complete = callback;
    promise->user_data = user_data;

    if (katra_pool_enqueue_promise(promise) != KATRA_SUCCESS) {
        katra_promise_free(promise);
        return NULL;
    }

    return promise;
}

katra_promise_t* katra_recall_synthesized_async(const char* ci_id,
                                                 const char* query,
                                                 const recall_options_t* options,
                                                 promise_callback_fn callback,
                                                 void* user_data) {
    if (!ci_id || !query) {
        katra_report_error(E_INPUT_NULL, "katra_recall_synthesized_async",
                          "NULL parameter");
        return NULL;
    }

    katra_promise_t* promise = katra_pool_create_promise(PROMISE_OP_RECALL_SYNTHESIZED);
    if (!promise) return NULL;

    void* internal = katra_pool_get_internal(promise);
    if (katra_pool_set_ci_id(internal, ci_id) != KATRA_SUCCESS ||
        katra_pool_set_query(internal, query) != KATRA_SUCCESS) {
        katra_promise_free(promise);
        return NULL;
    }

    /* Copy options if provided */
    if (options) {
        if (katra_pool_set_options(internal, options) != KATRA_SUCCESS) {
            katra_promise_free(promise);
            return NULL;
        }
    }

    promise->on_complete = callback;
    promise->user_data = user_data;

    if (katra_pool_enqueue_promise(promise) != KATRA_SUCCESS) {
        katra_promise_free(promise);
        return NULL;
    }

    return promise;
}

katra_promise_t* katra_query_async(const memory_query_t* query,
                                    promise_callback_fn callback,
                                    void* user_data) {
    if (!query || !query->ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_query_async", "NULL parameter");
        return NULL;
    }

    katra_promise_t* promise = katra_pool_create_promise(PROMISE_OP_QUERY);
    if (!promise) return NULL;

    void* internal = katra_pool_get_internal(promise);
    if (katra_pool_set_mem_query(internal, query) != KATRA_SUCCESS) {
        katra_promise_free(promise);
        return NULL;
    }

    promise->on_complete = callback;
    promise->user_data = user_data;

    if (katra_pool_enqueue_promise(promise) != KATRA_SUCCESS) {
        katra_promise_free(promise);
        return NULL;
    }

    return promise;
}

/* ============================================================================
 * PROMISE AWAIT OPERATIONS
 * ============================================================================ */

int katra_promise_await(katra_promise_t* promise, int timeout_ms) {
    if (!promise) return E_INPUT_NULL;

    promise_internal_t* internal = (promise_internal_t*)promise->_internal;
    if (!internal) return E_INVALID_STATE;

    pthread_mutex_lock(&internal->mutex);

    if (timeout_ms == 0) {
        /* Infinite wait */
        while (promise->state == PROMISE_PENDING ||
               promise->state == PROMISE_RUNNING) {
            pthread_cond_wait(&internal->cond, &internal->mutex);
        }
    } else {
        /* Timed wait */
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += timeout_ms / MILLISECONDS_PER_SECOND;
        timeout.tv_nsec += (timeout_ms % MILLISECONDS_PER_SECOND) * MICROSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;
        if (timeout.tv_nsec >= NANOSECONDS_PER_SECOND) {
            timeout.tv_sec++;
            timeout.tv_nsec -= NANOSECONDS_PER_SECOND;
        }

        while (promise->state == PROMISE_PENDING ||
               promise->state == PROMISE_RUNNING) {
            int rc = pthread_cond_timedwait(&internal->cond, &internal->mutex,
                                            &timeout);
            if (rc == ETIMEDOUT) {
                pthread_mutex_unlock(&internal->mutex);
                return E_TIMEOUT;
            }
        }
    }

    int result = KATRA_SUCCESS;
    if (promise->state == PROMISE_REJECTED) {
        result = promise->error_code;
    } else if (promise->state == PROMISE_CANCELLED) {
        result = E_PROMISE_CANCELLED;
    }

    pthread_mutex_unlock(&internal->mutex);
    return result;
}

int katra_promise_await_any(katra_promise_t** promises, size_t count,
                            int timeout_ms, size_t* completed_index) {
    if (!promises || count == 0) return E_INPUT_NULL;

    double start = get_current_time_ms();
    double deadline = timeout_ms > 0 ? start + timeout_ms : 0;

    while (1) {
        /* Check each promise */
        for (size_t i = 0; i < count; i++) {
            if (katra_promise_is_done(promises[i])) {
                if (completed_index) *completed_index = i;
                return KATRA_SUCCESS;
            }
        }

        /* Check timeout */
        if (deadline > 0 && get_current_time_ms() >= deadline) {
            return E_TIMEOUT;
        }

        /* Brief sleep to avoid busy-waiting */
        usleep(1000);  /* 1ms */
    }
}

int katra_promise_await_all(katra_promise_t** promises, size_t count,
                            int timeout_ms) {
    if (!promises || count == 0) return E_INPUT_NULL;

    for (size_t i = 0; i < count; i++) {
        int rc = katra_promise_await(promises[i], timeout_ms);
        if (rc != KATRA_SUCCESS && rc != E_PROMISE_CANCELLED) {
            return rc;
        }
    }
    return KATRA_SUCCESS;
}

/* ============================================================================
 * PROMISE STATE AND CANCELLATION
 * ============================================================================ */

int katra_promise_cancel(katra_promise_t* promise) {
    if (!promise) return E_INPUT_NULL;

    promise_internal_t* internal = (promise_internal_t*)promise->_internal;
    if (!internal) return E_INVALID_STATE;

    pthread_mutex_lock(&internal->mutex);
    if (promise->state == PROMISE_FULFILLED ||
        promise->state == PROMISE_REJECTED ||
        promise->state == PROMISE_CANCELLED) {
        pthread_mutex_unlock(&internal->mutex);
        return E_INVALID_STATE;
    }

    internal->cancelled = true;
    if (promise->state == PROMISE_PENDING) {
        promise->state = PROMISE_CANCELLED;
        promise->completed_at = time(NULL);
        pthread_cond_broadcast(&internal->cond);
    }
    pthread_mutex_unlock(&internal->mutex);

    return KATRA_SUCCESS;
}

promise_state_t katra_promise_get_state(const katra_promise_t* promise) {
    if (!promise) return PROMISE_REJECTED;
    return promise->state;
}

bool katra_promise_is_done(const katra_promise_t* promise) {
    if (!promise) return true;
    return promise->state == PROMISE_FULFILLED ||
           promise->state == PROMISE_REJECTED ||
           promise->state == PROMISE_CANCELLED;
}

/* ============================================================================
 * RESULT RETRIEVAL
 * ============================================================================ */

int katra_promise_get_recall_result(katra_promise_t* promise,
                                     memory_record_t*** records_out,
                                     size_t* count_out) {
    if (!promise || !records_out || !count_out) return E_INPUT_NULL;

    if (promise->state != PROMISE_FULFILLED) {
        return E_INVALID_STATE;
    }

    if (promise->op_type != PROMISE_OP_RECALL &&
        promise->op_type != PROMISE_OP_QUERY) {
        return E_INVALID_STATE;
    }

    *records_out = promise->result.recall.records;
    *count_out = promise->result.recall.count;

    /* Transfer ownership - clear from promise */
    promise->result.recall.records = NULL;
    promise->result.recall.count = 0;

    return KATRA_SUCCESS;
}

int katra_promise_get_synthesis_result(katra_promise_t* promise,
                                        synthesis_result_set_t** result_set_out) {
    if (!promise || !result_set_out) return E_INPUT_NULL;

    if (promise->state != PROMISE_FULFILLED) {
        return E_INVALID_STATE;
    }

    if (promise->op_type != PROMISE_OP_RECALL_SYNTHESIZED) {
        return E_INVALID_STATE;
    }

    *result_set_out = promise->result.synthesis.result_set;

    /* Transfer ownership */
    promise->result.synthesis.result_set = NULL;

    return KATRA_SUCCESS;
}

/* ============================================================================
 * PROMISE CLEANUP
 * ============================================================================ */

void katra_promise_free(katra_promise_t* promise) {
    if (!promise) return;

    /* Cancel if still running */
    if (!katra_promise_is_done(promise)) {
        katra_promise_cancel(promise);
        katra_promise_await(promise, 1000);  /* 1 second timeout */
    }

    if (promise->_internal) {
        katra_pool_free_internal(promise->_internal);
    }

    /* Free any unretrieved results */
    if (promise->op_type == PROMISE_OP_RECALL ||
        promise->op_type == PROMISE_OP_QUERY) {
        if (promise->result.recall.records) {
            katra_memory_free_results(promise->result.recall.records,
                                       promise->result.recall.count);
        }
    } else if (promise->op_type == PROMISE_OP_RECALL_SYNTHESIZED) {
        if (promise->result.synthesis.result_set) {
            katra_synthesis_free_results(promise->result.synthesis.result_set);
        }
    }

    free(promise);
}
