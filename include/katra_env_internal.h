/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_ENV_INTERNAL_H
#define KATRA_ENV_INTERNAL_H

#include <pthread.h>
#include <stdbool.h>

/* Environment subsystem - Thread-safe environment variable storage
 *
 * THREAD SAFETY:
 * - All access to katra_env, katra_env_count, katra_env_capacity MUST be protected by katra_env_mutex
 * - katra_env_initialized is accessed atomically (bool reads/writes are atomic)
 * - All public functions in katra_env_utils.h acquire katra_env_mutex
 * - Internal helpers assume caller holds lock (documented per-function)
 *
 * PROTECTED BY katra_env_mutex:
 * - katra_env           - Environment variable array
 * - katra_env_count     - Number of variables
 * - katra_env_capacity  - Array capacity
 */

/* Shared environment state */
extern char** katra_env;              /* PROTECTED BY katra_env_mutex */
extern int katra_env_count;           /* PROTECTED BY katra_env_mutex */
extern int katra_env_capacity;        /* PROTECTED BY katra_env_mutex */
extern pthread_mutex_t katra_env_mutex;  /* PROTECTS: katra_env, katra_env_count, katra_env_capacity */
extern bool katra_env_initialized;    /* Atomic flag (bool read/write is atomic) */

/* Internal helper functions - CALLER MUST HOLD katra_env_mutex */
int grow_env_array(void);            /* LOCKS: none (caller holds katra_env_mutex) */
int find_env_index(const char* name);  /* LOCKS: none (caller holds katra_env_mutex) */
int set_env_internal(const char* name, const char* value);  /* LOCKS: none (caller holds katra_env_mutex) */

#endif /* KATRA_ENV_INTERNAL_H */
