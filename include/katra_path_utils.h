/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_PATH_UTILS_H
#define KATRA_PATH_UTILS_H

#include <stddef.h>

/* Path utility functions for Katra directory management
 *
 * These utilities consolidate HOME directory access, path construction,
 * and directory creation patterns used throughout the codebase.
 */

/* Get home directory path
 *
 * Retrieves the HOME environment variable and validates it exists.
 *
 * Parameters:
 *   buffer - Buffer to store home directory path
 *   size - Size of buffer
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if HOME not set
 */
int katra_get_home_dir(char* buffer, size_t size);

/* Build path under ~/.katra/
 *
 * Constructs a path under the Katra home directory.
 * Example: katra_build_path(buf, size, "memory", "tier1", NULL)
 *          -> ~/.katra/memory/tier1
 *
 * Parameters:
 *   buffer - Buffer to store constructed path
 *   size - Size of buffer
 *   ... - Variable number of path components (NULL-terminated)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if HOME not set
 *   E_INPUT_TOO_LARGE if path exceeds buffer
 */
int katra_build_path(char* buffer, size_t size, ...);

/* Ensure directory exists (create if needed)
 *
 * Creates directory and any necessary parent directories.
 * Safe to call on existing directories (idempotent).
 *
 * Parameters:
 *   path - Directory path to create
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if path is NULL
 *   E_SYSTEM_FILE if creation fails
 */
int katra_ensure_dir(const char* path);

/* Build and ensure katra directory exists
 *
 * Combines katra_build_path() and katra_ensure_dir().
 * Convenience function for common initialization pattern.
 *
 * Parameters:
 *   buffer - Buffer to store constructed path
 *   size - Size of buffer
 *   ... - Variable number of path components (NULL-terminated)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error codes from katra_build_path() or katra_ensure_dir()
 */
int katra_build_and_ensure_dir(char* buffer, size_t size, ...);

#endif /* KATRA_PATH_UTILS_H */
