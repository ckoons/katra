/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_FILE_UTILS_H
#define KATRA_FILE_UTILS_H

#include <stddef.h>

/* File utility functions for Katra
 *
 * Common file operations used across multiple modules.
 */

/* Count lines in a file
 *
 * Counts newline-terminated lines in a text file.
 * Used for statistics and record counting.
 *
 * Parameters:
 *   filepath - Path to file
 *   count - Pointer to store line count
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if filepath or count is NULL
 *   Note: Returns success with count=0 if file doesn't exist
 */
int katra_file_count_lines(const char* filepath, size_t* count);

/* Get file size in bytes
 *
 * Parameters:
 *   filepath - Path to file
 *   size - Pointer to store file size
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if filepath or size is NULL
 *   E_SYSTEM_FILE if file doesn't exist or stat fails
 */
int katra_file_get_size(const char* filepath, size_t* size);

/* File visitor callback function type
 *
 * Called for each file found during directory iteration.
 *
 * Parameters:
 *   filepath - Full path to file
 *   userdata - User-provided context data
 *
 * Returns:
 *   KATRA_SUCCESS to continue iteration
 *   Any other value stops iteration
 */
typedef int (*katra_file_visitor_fn)(const char* filepath, void* userdata);

/* Iterate over files in directory
 *
 * Calls visitor function for each file matching the extension filter.
 *
 * Parameters:
 *   dir_path - Directory to scan
 *   extension - File extension filter (e.g., ".jsonl"), NULL for all files
 *   visitor - Callback function for each file
 *   userdata - Context data passed to visitor
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if dir_path or visitor is NULL
 *   E_SYSTEM_FILE if directory doesn't exist
 *   Visitor return code if visitor stops iteration
 */
int katra_dir_foreach(const char* dir_path, const char* extension,
                      katra_file_visitor_fn visitor, void* userdata);

#endif /* KATRA_FILE_UTILS_H */
