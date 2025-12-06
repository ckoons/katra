/* © 2025 Casey Koons All rights reserved */

/**
 * katra_promise.c - Async Memory Recall with Thread Pool (Phase 10)
 *
 * Implements asynchronous memory operations using POSIX threads.
 * Thread pool manages worker threads for executing promises.
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
#include "katra_tier1.h"

/* Use E_SYSTEM_TIMEOUT for timeout errors */
#define E_TIMEOUT E_SYSTEM_TIMEOUT

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

/* Internal promise data */
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

/* Work queue item */
typedef struct work_item {
    katra_promise_t* promise;
    struct work_item* next;
} work_item_t;

/* Thread pool structure */
typedef struct {
    /* Configuration */
    size_t min_threads;
    size_t max_threads;
    size_t queue_capacity;
    int idle_timeout_ms;

    /* State */
    bool initialized;
    bool shutdown;
    size_t active_threads;
    size_t idle_threads;

    /* Work queue */
    work_item_t* queue_head;
    work_item_t* queue_tail;
    size_t queue_size;

    /* Statistics */
    size_t completed_count;
    size_t failed_count;
    size_t cancelled_count;
    double total_execution_ms;

    /* Synchronization */
    pthread_mutex_t mutex;
    pthread_cond_t work_available;
    pthread_cond_t worker_done;

    /* Worker threads */
    pthread_t* workers;
    size_t worker_count;
} thread_pool_t;

/* Global thread pool */
static thread_pool_t* g_pool = NULL;

/* Promise ID counter */
static unsigned long g_promise_counter = 0;
static pthread_mutex_t g_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static void* worker_thread(void* arg);
static void execute_promise(katra_promise_t* promise);
static void promise_internal_free(promise_internal_t* internal);
static int enqueue_promise(katra_promise_t* promise);
static katra_promise_t* dequeue_promise(void);
static double get_current_time_ms(void);

/* ============================================================================
 * INITIALIZATION AND CLEANUP
 * ============================================================================ */

int katra_promise_init(const thread_pool_config_t* config) {
    if (g_pool) {
        katra_report_error(E_INVALID_STATE, "katra_promise_init",
                          "Promise system already initialized");
        return E_INVALID_STATE;
    }

    g_pool = calloc(1, sizeof(thread_pool_t));
    if (!g_pool) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_promise_init",
                          "Failed to allocate thread pool");
        return E_SYSTEM_MEMORY;
    }

    /* Apply configuration */
    if (config) {
        g_pool->min_threads = config->min_threads > 0 ?
                              config->min_threads : PROMISE_DEFAULT_MIN_THREADS;
        g_pool->max_threads = config->max_threads > 0 ?
                              config->max_threads : PROMISE_DEFAULT_MAX_THREADS;
        g_pool->queue_capacity = config->queue_capacity > 0 ?
                                 config->queue_capacity : PROMISE_DEFAULT_QUEUE_CAPACITY;
        g_pool->idle_timeout_ms = config->idle_timeout_ms > 0 ?
                                  config->idle_timeout_ms : PROMISE_DEFAULT_IDLE_TIMEOUT_MS;
    } else {
        g_pool->min_threads = PROMISE_DEFAULT_MIN_THREADS;
        g_pool->max_threads = PROMISE_DEFAULT_MAX_THREADS;
        g_pool->queue_capacity = PROMISE_DEFAULT_QUEUE_CAPACITY;
        g_pool->idle_timeout_ms = PROMISE_DEFAULT_IDLE_TIMEOUT_MS;
    }

    /* Validate configuration */
    if (g_pool->min_threads > g_pool->max_threads) {
        free(g_pool);
        g_pool = NULL;
        katra_report_error(E_INPUT_RANGE, "katra_promise_init",
                          "min_threads > max_threads");
        return E_INPUT_RANGE;
    }

    /* Initialize synchronization */
    if (pthread_mutex_init(&g_pool->mutex, NULL) != 0) {
        free(g_pool);
        g_pool = NULL;
        return E_SYSTEM_MEMORY;
    }

    if (pthread_cond_init(&g_pool->work_available, NULL) != 0) {
        pthread_mutex_destroy(&g_pool->mutex);
        free(g_pool);
        g_pool = NULL;
        return E_SYSTEM_MEMORY;
    }

    if (pthread_cond_init(&g_pool->worker_done, NULL) != 0) {
        pthread_cond_destroy(&g_pool->work_available);
        pthread_mutex_destroy(&g_pool->mutex);
        free(g_pool);
        g_pool = NULL;
        return E_SYSTEM_MEMORY;
    }

    /* Allocate worker thread array */
    g_pool->workers = calloc(g_pool->max_threads, sizeof(pthread_t));
    if (!g_pool->workers) {
        pthread_cond_destroy(&g_pool->worker_done);
        pthread_cond_destroy(&g_pool->work_available);
        pthread_mutex_destroy(&g_pool->mutex);
        free(g_pool);
        g_pool = NULL;
        return E_SYSTEM_MEMORY;
    }

    /* Start minimum worker threads */
    g_pool->initialized = true;
    for (size_t i = 0; i < g_pool->min_threads; i++) {
        if (pthread_create(&g_pool->workers[i], NULL, worker_thread, NULL) == 0) {
            g_pool->worker_count++;
            g_pool->idle_threads++;
        }
    }

    LOG_INFO("Promise system initialized: %zu-%zu threads, queue capacity %zu",
             g_pool->min_threads, g_pool->max_threads, g_pool->queue_capacity);
    return KATRA_SUCCESS;
}

void katra_promise_cleanup(void) {
    if (!g_pool) return;

    pthread_mutex_lock(&g_pool->mutex);
    g_pool->shutdown = true;
    pthread_cond_broadcast(&g_pool->work_available);
    pthread_mutex_unlock(&g_pool->mutex);

    /* Wait for workers to finish (with timeout) */
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 5;  /* 5 second timeout */

    pthread_mutex_lock(&g_pool->mutex);
    while (g_pool->active_threads > 0) {
        int rc = pthread_cond_timedwait(&g_pool->worker_done, &g_pool->mutex, &timeout);
        if (rc == ETIMEDOUT) {
            LOG_WARN("Promise cleanup timed out with %zu active threads",
                     g_pool->active_threads);
            break;
        }
    }
    pthread_mutex_unlock(&g_pool->mutex);

    /* Join worker threads */
    for (size_t i = 0; i < g_pool->worker_count; i++) {
        pthread_join(g_pool->workers[i], NULL);
    }

    /* Free remaining queue items */
    work_item_t* item = g_pool->queue_head;
    while (item) {
        work_item_t* next = item->next;
        if (item->promise) {
            item->promise->state = PROMISE_CANCELLED;
            katra_promise_free(item->promise);
        }
        free(item);
        item = next;
    }

    /* Cleanup */
    free(g_pool->workers);
    pthread_cond_destroy(&g_pool->worker_done);
    pthread_cond_destroy(&g_pool->work_available);
    pthread_mutex_destroy(&g_pool->mutex);
    free(g_pool);
    g_pool = NULL;

    LOG_INFO("Promise system shutdown complete");
}

bool katra_promise_is_initialized(void) {
    return g_pool != NULL && g_pool->initialized;
}

/* ============================================================================
 * WORKER THREAD
 * ============================================================================ */

static void* worker_thread(void* arg) {
    (void)arg;

    while (1) {
        pthread_mutex_lock(&g_pool->mutex);

        /* Wait for work or shutdown */
        while (!g_pool->shutdown && g_pool->queue_head == NULL) {
            struct timespec timeout;
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += g_pool->idle_timeout_ms / 1000;
            timeout.tv_nsec += (g_pool->idle_timeout_ms % 1000) * 1000000;
            if (timeout.tv_nsec >= 1000000000) {
                timeout.tv_sec++;
                timeout.tv_nsec -= 1000000000;
            }

            int rc = pthread_cond_timedwait(&g_pool->work_available,
                                            &g_pool->mutex, &timeout);

            /* Check if we should exit due to idle timeout */
            if (rc == ETIMEDOUT && g_pool->worker_count > g_pool->min_threads) {
                g_pool->worker_count--;
                g_pool->idle_threads--;
                pthread_mutex_unlock(&g_pool->mutex);
                return NULL;
            }
        }

        if (g_pool->shutdown) {
            g_pool->idle_threads--;
            pthread_cond_signal(&g_pool->worker_done);
            pthread_mutex_unlock(&g_pool->mutex);
            return NULL;
        }

        /* Dequeue work */
        katra_promise_t* promise = dequeue_promise();
        if (!promise) {
            pthread_mutex_unlock(&g_pool->mutex);
            continue;
        }

        g_pool->idle_threads--;
        g_pool->active_threads++;
        pthread_mutex_unlock(&g_pool->mutex);

        /* Execute promise */
        double start_time = get_current_time_ms();
        execute_promise(promise);
        double elapsed = get_current_time_ms() - start_time;

        /* Update statistics */
        pthread_mutex_lock(&g_pool->mutex);
        g_pool->active_threads--;
        g_pool->idle_threads++;
        g_pool->total_execution_ms += elapsed;

        if (promise->state == PROMISE_FULFILLED) {
            g_pool->completed_count++;
        } else if (promise->state == PROMISE_REJECTED) {
            g_pool->failed_count++;
        } else if (promise->state == PROMISE_CANCELLED) {
            g_pool->cancelled_count++;
        }

        pthread_cond_signal(&g_pool->worker_done);
        pthread_mutex_unlock(&g_pool->mutex);
    }

    return NULL;
}

/* ============================================================================
 * PROMISE EXECUTION
 * ============================================================================ */

static void execute_promise(katra_promise_t* promise) {
    promise_internal_t* internal = (promise_internal_t*)promise->_internal;

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

    int result = KATRA_SUCCESS;

    switch (promise->op_type) {
        case PROMISE_OP_RECALL: {
            /* Basic recall - use tier1_query with topic matching */
            memory_record_t** records = NULL;
            size_t count = 0;

            memory_query_t query = {0};
            query.ci_id = internal->ci_id;
            query.limit = internal->limit > 0 ? internal->limit : 20;

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
            break;
        }

        case PROMISE_OP_RECALL_SYNTHESIZED: {
            /* Synthesized recall */
            synthesis_result_set_t* result_set = NULL;

            result = katra_recall_synthesized(
                internal->ci_id,
                internal->query,
                internal->options,
                &result_set
            );

            if (result == KATRA_SUCCESS) {
                promise->result.synthesis.result_set = result_set;
            }
            break;
        }

        case PROMISE_OP_QUERY: {
            /* Memory query */
            memory_record_t** records = NULL;
            size_t count = 0;

            result = katra_memory_query(internal->mem_query, &records, &count);

            if (result == KATRA_SUCCESS) {
                promise->result.recall.records = records;
                promise->result.recall.count = count;
            }
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

/* ============================================================================
 * ASYNC OPERATIONS
 * ============================================================================ */

static katra_promise_t* create_promise(promise_op_type_t op_type) {
    if (!g_pool || !g_pool->initialized) {
        katra_report_error(E_INVALID_STATE, "create_promise",
                          "Promise system not initialized");
        return NULL;
    }

    katra_promise_t* promise = calloc(1, sizeof(katra_promise_t));
    if (!promise) {
        katra_report_error(E_SYSTEM_MEMORY, "create_promise",
                          "Failed to allocate promise");
        return NULL;
    }

    promise_internal_t* internal = calloc(1, sizeof(promise_internal_t));
    if (!internal) {
        free(promise);
        katra_report_error(E_SYSTEM_MEMORY, "create_promise",
                          "Failed to allocate promise internal");
        return NULL;
    }

    pthread_mutex_init(&internal->mutex, NULL);
    pthread_cond_init(&internal->cond, NULL);

    /* Generate unique ID */
    pthread_mutex_lock(&g_counter_mutex);
    unsigned long id = ++g_promise_counter;
    pthread_mutex_unlock(&g_counter_mutex);

    snprintf(promise->id, sizeof(promise->id), "promise_%lu_%ld", id, (long)time(NULL));
    promise->op_type = op_type;
    promise->state = PROMISE_PENDING;
    promise->priority = PROMISE_PRIORITY_NORMAL;
    promise->created_at = time(NULL);
    promise->_internal = internal;

    return promise;
}

katra_promise_t* katra_recall_async(const char* ci_id,
                                     const char* topic,
                                     size_t limit,
                                     promise_callback_fn callback,
                                     void* user_data) {
    if (!ci_id || !topic) {
        katra_report_error(E_INPUT_NULL, "katra_recall_async", "NULL parameter");
        return NULL;
    }

    katra_promise_t* promise = create_promise(PROMISE_OP_RECALL);
    if (!promise) return NULL;

    promise_internal_t* internal = (promise_internal_t*)promise->_internal;
    internal->ci_id = katra_safe_strdup(ci_id);
    internal->query = katra_safe_strdup(topic);
    internal->limit = limit;

    if (!internal->ci_id || !internal->query) {
        katra_promise_free(promise);
        return NULL;
    }

    promise->on_complete = callback;
    promise->user_data = user_data;

    if (enqueue_promise(promise) != KATRA_SUCCESS) {
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

    katra_promise_t* promise = create_promise(PROMISE_OP_RECALL_SYNTHESIZED);
    if (!promise) return NULL;

    promise_internal_t* internal = (promise_internal_t*)promise->_internal;
    internal->ci_id = katra_safe_strdup(ci_id);
    internal->query = katra_safe_strdup(query);

    if (!internal->ci_id || !internal->query) {
        katra_promise_free(promise);
        return NULL;
    }

    /* Copy options if provided */
    if (options) {
        internal->options = calloc(1, sizeof(recall_options_t));
        if (internal->options) {
            memcpy(internal->options, options, sizeof(recall_options_t));
        }
    }

    promise->on_complete = callback;
    promise->user_data = user_data;

    if (enqueue_promise(promise) != KATRA_SUCCESS) {
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

    katra_promise_t* promise = create_promise(PROMISE_OP_QUERY);
    if (!promise) return NULL;

    promise_internal_t* internal = (promise_internal_t*)promise->_internal;

    /* Copy query */
    internal->mem_query = calloc(1, sizeof(memory_query_t));
    if (!internal->mem_query) {
        katra_promise_free(promise);
        return NULL;
    }
    memcpy(internal->mem_query, query, sizeof(memory_query_t));
    internal->mem_query->ci_id = katra_safe_strdup(query->ci_id);

    promise->on_complete = callback;
    promise->user_data = user_data;

    if (enqueue_promise(promise) != KATRA_SUCCESS) {
        katra_promise_free(promise);
        return NULL;
    }

    return promise;
}

/* ============================================================================
 * QUEUE MANAGEMENT
 * ============================================================================ */

static int enqueue_promise(katra_promise_t* promise) {
    pthread_mutex_lock(&g_pool->mutex);

    if (g_pool->queue_size >= g_pool->queue_capacity) {
        pthread_mutex_unlock(&g_pool->mutex);
        katra_report_error(E_PROMISE_QUEUE_FULL, "enqueue_promise",
                          "Promise queue at capacity");
        return E_PROMISE_QUEUE_FULL;
    }

    work_item_t* item = calloc(1, sizeof(work_item_t));
    if (!item) {
        pthread_mutex_unlock(&g_pool->mutex);
        return E_SYSTEM_MEMORY;
    }

    item->promise = promise;

    if (g_pool->queue_tail) {
        g_pool->queue_tail->next = item;
    } else {
        g_pool->queue_head = item;
    }
    g_pool->queue_tail = item;
    g_pool->queue_size++;

    /* Spawn additional worker if needed and possible */
    if (g_pool->idle_threads == 0 && g_pool->worker_count < g_pool->max_threads) {
        pthread_t new_worker;
        if (pthread_create(&new_worker, NULL, worker_thread, NULL) == 0) {
            g_pool->workers[g_pool->worker_count++] = new_worker;
            g_pool->idle_threads++;
        }
    }

    pthread_cond_signal(&g_pool->work_available);
    pthread_mutex_unlock(&g_pool->mutex);

    return KATRA_SUCCESS;
}

static katra_promise_t* dequeue_promise(void) {
    /* Called with mutex held */
    if (!g_pool->queue_head) return NULL;

    work_item_t* item = g_pool->queue_head;
    g_pool->queue_head = item->next;
    if (!g_pool->queue_head) {
        g_pool->queue_tail = NULL;
    }
    g_pool->queue_size--;

    katra_promise_t* promise = item->promise;
    free(item);
    return promise;
}

/* ============================================================================
 * PROMISE OPERATIONS
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
        timeout.tv_sec += timeout_ms / 1000;
        timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
        if (timeout.tv_nsec >= 1000000000) {
            timeout.tv_sec++;
            timeout.tv_nsec -= 1000000000;
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

void katra_promise_free(katra_promise_t* promise) {
    if (!promise) return;

    /* Cancel if still running */
    if (!katra_promise_is_done(promise)) {
        katra_promise_cancel(promise);
        katra_promise_await(promise, 1000);  /* 1 second timeout */
    }

    promise_internal_t* internal = (promise_internal_t*)promise->_internal;
    if (internal) {
        promise_internal_free(internal);
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

static void promise_internal_free(promise_internal_t* internal) {
    if (!internal) return;

    pthread_mutex_destroy(&internal->mutex);
    pthread_cond_destroy(&internal->cond);

    free(internal->ci_id);
    free(internal->query);
    free(internal->options);

    if (internal->mem_query) {
        free((char*)internal->mem_query->ci_id);
        free(internal->mem_query);
    }

    free(internal);
}

/* ============================================================================
 * THREAD POOL MANAGEMENT
 * ============================================================================ */

int katra_promise_get_stats(thread_pool_stats_t* stats) {
    if (!stats) return E_INPUT_NULL;
    if (!g_pool) return E_INVALID_STATE;

    pthread_mutex_lock(&g_pool->mutex);
    stats->active_threads = g_pool->active_threads;
    stats->idle_threads = g_pool->idle_threads;
    stats->pending_promises = g_pool->queue_size;
    stats->completed_promises = g_pool->completed_count;
    stats->failed_promises = g_pool->failed_count;
    stats->cancelled_promises = g_pool->cancelled_count;

    size_t total = g_pool->completed_count + g_pool->failed_count;
    stats->avg_execution_ms = total > 0 ?
                              g_pool->total_execution_ms / total : 0.0;
    pthread_mutex_unlock(&g_pool->mutex);

    return KATRA_SUCCESS;
}

int katra_promise_resize_pool(size_t min_threads, size_t max_threads) {
    if (!g_pool) return E_INVALID_STATE;
    if (min_threads > max_threads) return E_INPUT_RANGE;

    pthread_mutex_lock(&g_pool->mutex);
    g_pool->min_threads = min_threads;
    g_pool->max_threads = max_threads;
    pthread_mutex_unlock(&g_pool->mutex);

    return KATRA_SUCCESS;
}

int katra_promise_drain(int timeout_ms) {
    if (!g_pool) return E_INVALID_STATE;

    double start = get_current_time_ms();
    double deadline = timeout_ms > 0 ? start + timeout_ms : 0;

    while (1) {
        pthread_mutex_lock(&g_pool->mutex);
        bool empty = (g_pool->queue_size == 0 && g_pool->active_threads == 0);
        pthread_mutex_unlock(&g_pool->mutex);

        if (empty) return KATRA_SUCCESS;

        if (deadline > 0 && get_current_time_ms() >= deadline) {
            return E_TIMEOUT;
        }

        usleep(10000);  /* 10ms */
    }
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static double get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}
