/* © 2025 Casey Koons All rights reserved */

/**
 * katra_promise_pool.c - Thread Pool Implementation for Async Promises
 *
 * Manages worker threads, work queue, and promise execution.
 * Split from katra_promise.c for maintainability.
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

/* Global thread pool - shared with katra_promise.c via extern */
thread_pool_t* g_pool = NULL;

/* Promise ID counter */
static unsigned long g_promise_counter = 0;
static pthread_mutex_t g_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * EXTERNAL DECLARATIONS
 * ============================================================================ */

/* Promise execution - implemented in katra_promise_exec.c */
extern void katra_execute_promise(katra_promise_t* promise);

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static void* worker_thread(void* arg);
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
            timeout.tv_sec += g_pool->idle_timeout_ms / MILLISECONDS_PER_SECOND;
            timeout.tv_nsec += (g_pool->idle_timeout_ms % MILLISECONDS_PER_SECOND) * MICROSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;
            if (timeout.tv_nsec >= NANOSECONDS_PER_SECOND) {
                timeout.tv_sec++;
                timeout.tv_nsec -= NANOSECONDS_PER_SECOND;
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
        katra_execute_promise(promise);
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
 * QUEUE MANAGEMENT
 * ============================================================================ */

int katra_pool_enqueue_promise(katra_promise_t* promise) {
    pthread_mutex_lock(&g_pool->mutex);

    if (g_pool->queue_size >= g_pool->queue_capacity) {
        pthread_mutex_unlock(&g_pool->mutex);
        katra_report_error(E_PROMISE_QUEUE_FULL, "katra_pool_enqueue_promise",
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
 * PROMISE INTERNAL FREE (for use by katra_promise.c)
 * ============================================================================ */

void katra_pool_free_internal(void* internal_ptr) {
    promise_internal_t* internal = (promise_internal_t*)internal_ptr;
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
 * PROMISE CREATION (for use by katra_promise.c)
 * ============================================================================ */

katra_promise_t* katra_pool_create_promise(promise_op_type_t op_type) {
    if (!g_pool || !g_pool->initialized) {
        katra_report_error(E_INVALID_STATE, "katra_pool_create_promise",
                          "Promise system not initialized");
        return NULL;
    }

    katra_promise_t* promise = calloc(1, sizeof(katra_promise_t));
    if (!promise) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_pool_create_promise",
                          "Failed to allocate promise");
        return NULL;
    }

    promise_internal_t* internal = calloc(1, sizeof(promise_internal_t));
    if (!internal) {
        free(promise);
        katra_report_error(E_SYSTEM_MEMORY, "katra_pool_create_promise",
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

/* Get internal structure for setting operation-specific fields */
void* katra_pool_get_internal(katra_promise_t* promise) {
    return promise ? promise->_internal : NULL;
}

/* Set internal string fields */
int katra_pool_set_ci_id(void* internal_ptr, const char* ci_id) {
    promise_internal_t* internal = (promise_internal_t*)internal_ptr;
    if (!internal) return E_INPUT_NULL;
    internal->ci_id = katra_safe_strdup(ci_id);
    return internal->ci_id ? KATRA_SUCCESS : E_SYSTEM_MEMORY;
}

int katra_pool_set_query(void* internal_ptr, const char* query) {
    promise_internal_t* internal = (promise_internal_t*)internal_ptr;
    if (!internal) return E_INPUT_NULL;
    internal->query = katra_safe_strdup(query);
    return internal->query ? KATRA_SUCCESS : E_SYSTEM_MEMORY;
}

void katra_pool_set_limit(void* internal_ptr, size_t limit) {
    promise_internal_t* internal = (promise_internal_t*)internal_ptr;
    if (internal) internal->limit = limit;
}

int katra_pool_set_options(void* internal_ptr, const recall_options_t* options) {
    promise_internal_t* internal = (promise_internal_t*)internal_ptr;
    if (!internal || !options) return E_INPUT_NULL;
    internal->options = calloc(1, sizeof(recall_options_t));
    if (!internal->options) return E_SYSTEM_MEMORY;
    memcpy(internal->options, options, sizeof(recall_options_t));
    return KATRA_SUCCESS;
}

int katra_pool_set_mem_query(void* internal_ptr, const memory_query_t* query) {
    promise_internal_t* internal = (promise_internal_t*)internal_ptr;
    if (!internal || !query) return E_INPUT_NULL;
    internal->mem_query = calloc(1, sizeof(memory_query_t));
    if (!internal->mem_query) return E_SYSTEM_MEMORY;
    memcpy(internal->mem_query, query, sizeof(memory_query_t));
    internal->mem_query->ci_id = katra_safe_strdup(query->ci_id);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static double get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}
