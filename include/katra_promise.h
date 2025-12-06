/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_PROMISE_H
#define KATRA_PROMISE_H

/**
 * katra_promise.h - Async Memory Recall with Thread Pool (Phase 10)
 *
 * Provides asynchronous memory operations using a thread pool and
 * promise/future pattern. Enables non-blocking recall operations
 * for improved responsiveness in CI systems.
 *
 * Key concepts:
 * - Promise: Represents a pending async operation
 * - Future: Handle to retrieve the result when ready
 * - Callback: Optional notification when operation completes
 * - Thread Pool: Reusable worker threads for async execution
 */

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "katra_memory.h"
#include "katra_synthesis.h"

/* ============================================================================
 * PROMISE STATES AND TYPES
 * ============================================================================ */

/* Promise states */
typedef enum {
    PROMISE_PENDING = 0,    /* Operation not yet started */
    PROMISE_RUNNING = 1,    /* Operation in progress */
    PROMISE_FULFILLED = 2,  /* Operation completed successfully */
    PROMISE_REJECTED = 3,   /* Operation failed with error */
    PROMISE_CANCELLED = 4   /* Operation was cancelled */
} promise_state_t;

/* Promise operation types */
typedef enum {
    PROMISE_OP_RECALL = 1,          /* Basic memory recall */
    PROMISE_OP_RECALL_SYNTHESIZED,  /* Multi-backend synthesized recall */
    PROMISE_OP_RECALL_EMOTIONAL,    /* Emotion-based recall */
    PROMISE_OP_QUERY,               /* Memory query operation */
    PROMISE_OP_CUSTOM               /* Custom async operation */
} promise_op_type_t;

/* Promise priority levels */
typedef enum {
    PROMISE_PRIORITY_LOW = 0,
    PROMISE_PRIORITY_NORMAL = 1,
    PROMISE_PRIORITY_HIGH = 2,
    PROMISE_PRIORITY_URGENT = 3
} promise_priority_t;

/* ============================================================================
 * PROMISE RESULT STRUCTURES
 * ============================================================================ */

/* Result for basic recall operations */
typedef struct {
    memory_record_t** records;  /* Array of memory records */
    size_t count;               /* Number of records */
} promise_recall_result_t;

/* Result for synthesized recall operations */
typedef struct {
    synthesis_result_set_t* result_set;  /* Synthesized results */
} promise_synthesis_result_t;

/* Generic promise result union */
typedef union {
    promise_recall_result_t recall;
    promise_synthesis_result_t synthesis;
    void* custom;  /* For custom operations */
} promise_result_t;

/* ============================================================================
 * CALLBACK TYPES
 * ============================================================================ */

/* Forward declaration */
typedef struct katra_promise katra_promise_t;

/**
 * Callback function type for promise completion
 *
 * Parameters:
 *   promise - The completed promise
 *   user_data - User-provided context data
 */
typedef void (*promise_callback_fn)(katra_promise_t* promise, void* user_data);

/**
 * Progress callback function type
 *
 * Parameters:
 *   promise - The promise in progress
 *   progress - Progress percentage (0-100)
 *   user_data - User-provided context data
 */
typedef void (*promise_progress_fn)(katra_promise_t* promise, int progress,
                                    void* user_data);

/* ============================================================================
 * PROMISE STRUCTURE
 * ============================================================================ */

/**
 * Promise structure - represents an async memory operation
 *
 * Thread-safe: All operations on promises are protected by internal locks.
 */
struct katra_promise {
    /* Identity */
    char id[64];                    /* Unique promise ID */
    promise_op_type_t op_type;      /* Type of operation */
    promise_state_t state;          /* Current state */
    promise_priority_t priority;    /* Execution priority */

    /* Timing */
    time_t created_at;              /* When promise was created */
    time_t started_at;              /* When execution started (0 if pending) */
    time_t completed_at;            /* When execution completed (0 if not done) */

    /* Result */
    promise_result_t result;        /* Operation result (valid if fulfilled) */
    int error_code;                 /* Error code (if rejected) */
    char error_message[256];        /* Error message (if rejected) */

    /* Callbacks */
    promise_callback_fn on_complete;  /* Called when promise completes */
    promise_progress_fn on_progress;  /* Called for progress updates */
    void* user_data;                  /* User context for callbacks */

    /* Internal - do not access directly */
    void* _internal;                /* Internal synchronization data */
};

/* ============================================================================
 * THREAD POOL CONFIGURATION
 * ============================================================================ */

/* Thread pool configuration */
typedef struct {
    size_t min_threads;       /* Minimum worker threads (default: 2) */
    size_t max_threads;       /* Maximum worker threads (default: 8) */
    size_t queue_capacity;    /* Maximum pending promises (default: 100) */
    int idle_timeout_ms;      /* Thread idle timeout in ms (default: 30000) */
} thread_pool_config_t;

/* Default configuration */
#define PROMISE_DEFAULT_MIN_THREADS 2
#define PROMISE_DEFAULT_MAX_THREADS 8
#define PROMISE_DEFAULT_QUEUE_CAPACITY 100
#define PROMISE_DEFAULT_IDLE_TIMEOUT_MS 30000

/* Thread pool statistics */
typedef struct {
    size_t active_threads;      /* Currently active worker threads */
    size_t idle_threads;        /* Currently idle worker threads */
    size_t pending_promises;    /* Promises waiting in queue */
    size_t completed_promises;  /* Total promises completed */
    size_t failed_promises;     /* Total promises that failed */
    size_t cancelled_promises;  /* Total promises cancelled */
    double avg_execution_ms;    /* Average execution time in ms */
} thread_pool_stats_t;

/* ============================================================================
 * INITIALIZATION AND CLEANUP
 * ============================================================================ */

/**
 * katra_promise_init() - Initialize the promise system and thread pool
 *
 * Must be called before using any promise functions.
 * Uses default configuration if config is NULL.
 *
 * Parameters:
 *   config - Thread pool configuration (NULL for defaults)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_MEMORY if allocation fails
 *   E_INVALID_STATE if already initialized
 */
int katra_promise_init(const thread_pool_config_t* config);

/**
 * katra_promise_cleanup() - Shutdown promise system and thread pool
 *
 * Waits for pending promises to complete or times out after 5 seconds.
 * All pending promises will be cancelled after timeout.
 */
void katra_promise_cleanup(void);

/**
 * katra_promise_is_initialized() - Check if promise system is ready
 *
 * Returns:
 *   true if initialized, false otherwise
 */
bool katra_promise_is_initialized(void);

/* ============================================================================
 * ASYNC RECALL OPERATIONS
 * ============================================================================ */

/**
 * katra_recall_async() - Asynchronous memory recall
 *
 * Queues a memory recall operation for async execution.
 * Returns immediately with a promise that can be awaited.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   topic - Search topic/query
 *   limit - Maximum results (0 for default)
 *   callback - Optional completion callback
 *   user_data - User context for callback
 *
 * Returns:
 *   Promise pointer on success (caller owns, free with katra_promise_free)
 *   NULL on failure (check katra_get_last_error)
 */
katra_promise_t* katra_recall_async(const char* ci_id,
                                     const char* topic,
                                     size_t limit,
                                     promise_callback_fn callback,
                                     void* user_data);

/**
 * katra_recall_synthesized_async() - Async multi-backend synthesized recall
 *
 * Queues a synthesized recall across Vector, Graph, SQL, and Working Memory.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   query - Search query
 *   options - Recall options (NULL for defaults)
 *   callback - Optional completion callback
 *   user_data - User context for callback
 *
 * Returns:
 *   Promise pointer on success
 *   NULL on failure
 */
katra_promise_t* katra_recall_synthesized_async(const char* ci_id,
                                                 const char* query,
                                                 const recall_options_t* options,
                                                 promise_callback_fn callback,
                                                 void* user_data);

/**
 * katra_query_async() - Asynchronous memory query
 *
 * Queues a memory query operation for async execution.
 *
 * Parameters:
 *   query - Query parameters
 *   callback - Optional completion callback
 *   user_data - User context for callback
 *
 * Returns:
 *   Promise pointer on success
 *   NULL on failure
 */
katra_promise_t* katra_query_async(const memory_query_t* query,
                                    promise_callback_fn callback,
                                    void* user_data);

/* ============================================================================
 * PROMISE OPERATIONS
 * ============================================================================ */

/**
 * katra_promise_await() - Wait for promise to complete
 *
 * Blocks until the promise is fulfilled, rejected, or cancelled.
 *
 * Parameters:
 *   promise - Promise to wait for
 *   timeout_ms - Maximum wait time in ms (0 = infinite)
 *
 * Returns:
 *   KATRA_SUCCESS if fulfilled
 *   E_TIMEOUT if timeout expired
 *   Error code if rejected
 */
int katra_promise_await(katra_promise_t* promise, int timeout_ms);

/**
 * katra_promise_await_any() - Wait for any promise to complete
 *
 * Blocks until at least one promise completes.
 *
 * Parameters:
 *   promises - Array of promises
 *   count - Number of promises
 *   timeout_ms - Maximum wait time in ms (0 = infinite)
 *   completed_index - Output: index of first completed promise
 *
 * Returns:
 *   KATRA_SUCCESS if at least one completed
 *   E_TIMEOUT if timeout expired
 */
int katra_promise_await_any(katra_promise_t** promises, size_t count,
                            int timeout_ms, size_t* completed_index);

/**
 * katra_promise_await_all() - Wait for all promises to complete
 *
 * Blocks until all promises complete.
 *
 * Parameters:
 *   promises - Array of promises
 *   count - Number of promises
 *   timeout_ms - Maximum wait time in ms (0 = infinite)
 *
 * Returns:
 *   KATRA_SUCCESS if all completed
 *   E_TIMEOUT if timeout expired
 */
int katra_promise_await_all(katra_promise_t** promises, size_t count,
                            int timeout_ms);

/**
 * katra_promise_cancel() - Cancel a pending promise
 *
 * Attempts to cancel a pending or running promise.
 * If already completed, has no effect.
 *
 * Parameters:
 *   promise - Promise to cancel
 *
 * Returns:
 *   KATRA_SUCCESS if cancelled
 *   E_INVALID_STATE if already completed
 */
int katra_promise_cancel(katra_promise_t* promise);

/**
 * katra_promise_get_state() - Get current promise state
 *
 * Parameters:
 *   promise - Promise to query
 *
 * Returns:
 *   Current state
 */
promise_state_t katra_promise_get_state(const katra_promise_t* promise);

/**
 * katra_promise_is_done() - Check if promise is complete
 *
 * Parameters:
 *   promise - Promise to check
 *
 * Returns:
 *   true if fulfilled, rejected, or cancelled
 */
bool katra_promise_is_done(const katra_promise_t* promise);

/**
 * katra_promise_get_recall_result() - Get recall result from fulfilled promise
 *
 * Only valid if promise is fulfilled and was a recall operation.
 *
 * Parameters:
 *   promise - Fulfilled promise
 *   records_out - Output: array of records (caller must free)
 *   count_out - Output: number of records
 *
 * Returns:
 *   KATRA_SUCCESS if valid
 *   E_INVALID_STATE if not fulfilled or wrong op type
 */
int katra_promise_get_recall_result(katra_promise_t* promise,
                                     memory_record_t*** records_out,
                                     size_t* count_out);

/**
 * katra_promise_get_synthesis_result() - Get synthesis result
 *
 * Only valid if promise is fulfilled and was a synthesized recall.
 *
 * Parameters:
 *   promise - Fulfilled promise
 *   result_set_out - Output: synthesis result set (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS if valid
 *   E_INVALID_STATE if not fulfilled or wrong op type
 */
int katra_promise_get_synthesis_result(katra_promise_t* promise,
                                        synthesis_result_set_t** result_set_out);

/**
 * katra_promise_free() - Free promise and associated resources
 *
 * Safe to call on NULL. If promise is still running, it will be
 * cancelled first.
 *
 * Parameters:
 *   promise - Promise to free
 */
void katra_promise_free(katra_promise_t* promise);

/* ============================================================================
 * THREAD POOL MANAGEMENT
 * ============================================================================ */

/**
 * katra_promise_get_stats() - Get thread pool statistics
 *
 * Parameters:
 *   stats - Statistics structure to fill
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INVALID_STATE if not initialized
 */
int katra_promise_get_stats(thread_pool_stats_t* stats);

/**
 * katra_promise_resize_pool() - Dynamically resize thread pool
 *
 * Parameters:
 *   min_threads - New minimum threads
 *   max_threads - New maximum threads
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_RANGE if min > max
 *   E_INVALID_STATE if not initialized
 */
int katra_promise_resize_pool(size_t min_threads, size_t max_threads);

/**
 * katra_promise_drain() - Wait for all pending promises to complete
 *
 * Blocks until queue is empty and all workers are idle.
 *
 * Parameters:
 *   timeout_ms - Maximum wait time (0 = infinite)
 *
 * Returns:
 *   KATRA_SUCCESS if drained
 *   E_TIMEOUT if timeout expired
 */
int katra_promise_drain(int timeout_ms);

/* ============================================================================
 * CONVENIENCE MACROS
 * ============================================================================ */

/* Await promise and extract recall result in one step */
#define KATRA_AWAIT_RECALL(promise, records, count, timeout) \
    do { \
        int _await_rc = katra_promise_await(promise, timeout); \
        if (_await_rc == KATRA_SUCCESS) { \
            _await_rc = katra_promise_get_recall_result(promise, &(records), &(count)); \
        } \
        if (_await_rc != KATRA_SUCCESS) { \
            katra_promise_free(promise); \
            return _await_rc; \
        } \
    } while(0)

/* Promise state name for debugging */
#define PROMISE_STATE_NAME(state) \
    ((state) == PROMISE_PENDING ? "pending" : \
     (state) == PROMISE_RUNNING ? "running" : \
     (state) == PROMISE_FULFILLED ? "fulfilled" : \
     (state) == PROMISE_REJECTED ? "rejected" : \
     (state) == PROMISE_CANCELLED ? "cancelled" : "unknown")

/* ============================================================================
 * ERROR CONSTANTS
 * ============================================================================ */

/* Promise-specific errors (add to katra_error.h if needed) */
#define E_PROMISE_QUEUE_FULL     (-500)  /* Promise queue at capacity */
#define E_PROMISE_CANCELLED      (-501)  /* Promise was cancelled */
#define E_PROMISE_TIMEOUT        (-502)  /* Promise operation timed out */

/* ============================================================================
 * SQL QUERY CONSTANTS
 * ============================================================================ */

/* Note: Async operations use existing memory query SQL from katra_memory.c */

#endif /* KATRA_PROMISE_H */
