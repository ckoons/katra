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

/* Join directory and filename
 *
 * Simple path join: dir/filename
 * Handles trailing slashes in dir automatically.
 *
 * Parameters:
 *   dest - Destination buffer
 *   dest_size - Size of destination buffer
 *   dir - Directory path
 *   filename - Filename to append
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if any parameter is NULL
 *   E_INPUT_TOO_LARGE if path exceeds buffer
 */
int katra_path_join(char* dest, size_t dest_size,
                    const char* dir, const char* filename);

/* Join directory, filename, and extension
 *
 * Simple path join: dir/filename.ext
 * Handles trailing slashes in dir automatically.
 *
 * Parameters:
 *   dest - Destination buffer
 *   dest_size - Size of destination buffer
 *   dir - Directory path
 *   filename - Filename to append
 *   ext - Extension (without leading dot)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if any parameter is NULL
 *   E_INPUT_TOO_LARGE if path exceeds buffer
 */
int katra_path_join_with_ext(char* dest, size_t dest_size,
                              const char* dir, const char* filename, const char* ext);

/* Get project root directory
 *
 * Finds the Katra project root directory by searching upward from
 * current working directory for .git or Makefile.
 *
 * Parameters:
 *   buffer - Buffer to store project root path
 *   size - Size of buffer
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if project root not found
 *   E_INPUT_TOO_LARGE if path exceeds buffer
 */
int katra_get_project_root(char* buffer, size_t size);

/* Get shipped persona directory
 *
 * Returns path to shipped persona template in project repository.
 * Location: {project_root}/personas/{persona_name}
 *
 * These are Git-versioned persona templates that ship with the product.
 * They contain default configuration but NO user data (memory/chat).
 *
 * Parameters:
 *   buffer - Buffer to store shipped persona path
 *   size - Size of buffer
 *   persona_name - Name of persona
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if buffer or persona_name is NULL
 *   E_SYSTEM_FILE if project root not found
 *   E_INPUT_TOO_LARGE if path exceeds buffer
 */
int katra_get_shipped_persona_dir(char* buffer, size_t size, const char* persona_name);

/* Get user persona directory
 *
 * Returns path to user's persona runtime directory.
 * Location: ~/.katra/personas/{persona_name}
 *
 * This directory contains user-private data (memory, chat, checkpoints)
 * and is NEVER tracked in Git.
 *
 * Parameters:
 *   buffer - Buffer to store user persona path
 *   size - Size of buffer
 *   persona_name - Name of persona
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if buffer or persona_name is NULL
 *   E_SYSTEM_FILE if HOME not set
 *   E_INPUT_TOO_LARGE if path exceeds buffer
 */
int katra_get_user_persona_dir(char* buffer, size_t size, const char* persona_name);

/* Build path under user persona directory
 *
 * Constructs a path under user's persona runtime directory.
 * All paths are under ~/.katra/personas/{persona_name}/
 *
 * Example:
 *   katra_build_user_persona_path(buf, size, "Alice", "memory", "tier1", NULL)
 *   -> ~/.katra/personas/Alice/memory/tier1
 *
 * Parameters:
 *   buffer - Buffer to store constructed path
 *   size - Size of buffer
 *   persona_name - Name of persona
 *   ... - Variable number of path components (NULL-terminated)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if buffer or persona_name is NULL
 *   E_SYSTEM_FILE if HOME not set
 *   E_INPUT_TOO_LARGE if path exceeds buffer
 */
int katra_build_user_persona_path(char* buffer, size_t size, const char* persona_name, ...);

/* DEPRECATED: Use katra_get_user_persona_dir() instead */
int katra_get_persona_dir(char* buffer, size_t size, const char* persona_name);

/* DEPRECATED: Use katra_build_user_persona_path() instead */
int katra_build_persona_path(char* buffer, size_t size, const char* persona_name, ...);

#endif /* KATRA_PATH_UTILS_H */
