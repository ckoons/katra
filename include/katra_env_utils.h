/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_ENV_UTILS_H
#define KATRA_ENV_UTILS_H

#include <stdbool.h>

/* Environment configuration constants */
#define KATRA_ENV_INITIAL_CAPACITY 256
#define KATRA_ENV_GROWTH_QUANTUM 64
#define KATRA_ENV_LINE_MAX 4096
#define KATRA_ENV_VAR_NAME_MAX 256
#define KATRA_ENV_MAX_EXPANSION_DEPTH 10
#define KATRA_ENV_EXPORT_PREFIX "export "

/* Environment file names */
#define KATRA_ENV_HOME_FILE ".env"
#define KATRA_ENV_KATRARC_FILE ".katrarc"
#define KATRA_ENV_PROJECT_FILE ".env.katra"
#define KATRA_ENV_LOCAL_FILE ".env.katra.local"

/* Environment variable for root directory */
#define KATRA_ROOT_VAR "KATRA_ROOT"

/* Load Katra environment
 *
 * Loads environment in this sequence:
 * 1. System environment (environ)
 * 2. ~/.env (optional)
 * 3. ~/.katrarc (optional)
 * 4. ${KATRA_ROOT}/.env.katra or ./.env.katra (required)
 * 5. ${KATRA_ROOT}/.env.katra.local or ./.env.katra.local (optional)
 * 6. Expand ${VAR} references
 *
 * Thread-safe. Can be called multiple times to reload configuration.
 * Frees previous environment if already loaded.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if .env.katra not found
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_loadenv(void);  /* LOCKS: katra_env_mutex */

/* Get environment variable from Katra environment
 *
 * Thread-safe. Never accesses system environment after katra_loadenv().
 *
 * Parameters:
 *   name - Variable name to look up
 *
 * Returns:
 *   Value string or NULL if not found
 */
const char* katra_getenv(const char* name);  /* LOCKS: katra_env_mutex */

/* Set environment variable in Katra environment
 *
 * Thread-safe. Creates new entry or overwrites existing.
 *
 * Parameters:
 *   name - Variable name
 *   value - Variable value
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if name or value is NULL
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_setenv(const char* name, const char* value);  /* LOCKS: katra_env_mutex */

/* Unset environment variable in Katra environment
 *
 * Thread-safe. No error if variable doesn't exist.
 *
 * Parameters:
 *   name - Variable name to remove
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if name is NULL
 */
int katra_unsetenv(const char* name);  /* LOCKS: katra_env_mutex */

/* Clear all variables from Katra environment
 *
 * Thread-safe. Removes all variables but keeps allocation.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_clearenv(void);  /* LOCKS: katra_env_mutex */

/* Free Katra environment and release resources
 *
 * Thread-safe. Frees all allocations and resets to empty state.
 * Safe to call multiple times.
 */
void katra_freeenv(void);  /* LOCKS: katra_env_mutex */

/* Get integer value from environment
 *
 * Thread-safe. Parses variable as base-10 integer with full validation.
 *
 * Parameters:
 *   name - Variable name
 *   value - Pointer to receive integer value (not modified on error)
 *
 * Returns:
 *   KATRA_SUCCESS on success and sets *value
 *   E_INPUT_NULL if name or value pointer is NULL
 *   E_INPUT_FORMAT if variable not found or not valid integer
 */
int katra_getenvint(const char* name, int* value);  /* LOCKS: katra_env_mutex */

/* Print Katra environment to stdout
 *
 * Thread-safe. Prints all variables in NAME=VALUE format, one per line.
 * Similar to 'env' command output.
 */
void katra_env_print(void);  /* LOCKS: katra_env_mutex */

/* Dump Katra environment to file
 *
 * Thread-safe. Writes all variables to specified file in NAME=VALUE format.
 * Useful for debugging and diagnostics.
 *
 * Parameters:
 *   filepath - Path to output file
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if filepath is NULL
 *   E_SYSTEM_FILE if file cannot be opened
 */
int katra_env_dump(const char* filepath);  /* LOCKS: katra_env_mutex */

#endif /* KATRA_ENV_UTILS_H */
