/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_MM_SCANNER_H
#define KATRA_MM_SCANNER_H

/**
 * @file katra_mm_scanner.h
 * @brief C language parser for metamemory
 *
 * Provides directory walking and C file parsing to extract
 * functions, structs, and other code elements.
 */

#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * Types
 * ============================================================================ */

/**
 * Scanner options.
 */
typedef struct {
    const char** exclude_dirs;      /* Directories to skip */
    size_t exclude_dir_count;
    const char** exclude_patterns;  /* File patterns to skip */
    size_t exclude_pattern_count;
    bool incremental;               /* Only scan changed files */
} mm_scanner_options_t;

/**
 * Scanner result.
 */
typedef struct {
    size_t directories_scanned;
    size_t files_scanned;
    size_t functions_found;
    size_t structs_found;
    size_t enums_found;
    size_t macros_found;
    size_t errors_encountered;
} mm_scanner_result_t;

/* ============================================================================
 * Scanner API
 * ============================================================================ */

/**
 * Scan a project directory.
 *
 * Walks the directory tree and parses C files to extract
 * functions, structs, and other code elements.
 *
 * Parameters:
 *   project_id - Project identifier
 *   root_path - Absolute path to project root
 *   options - Scanner options (can be NULL for defaults)
 *   result - Scan statistics (can be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int mm_scanner_scan_project(const char* project_id,
                            const char* root_path,
                            const mm_scanner_options_t* options,
                            mm_scanner_result_t* result);

/**
 * Scan a single file.
 *
 * Parameters:
 *   project_id - Project identifier
 *   file_path - Absolute path to file
 *   result - Scan statistics (can be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int mm_scanner_scan_file(const char* project_id,
                         const char* file_path,
                         mm_scanner_result_t* result);

/**
 * Check for changed files.
 *
 * Compares file hashes to detect changes since last scan.
 *
 * Parameters:
 *   project_id - Project identifier
 *   root_path - Absolute path to project root
 *   changed_files - Output array of changed file paths
 *   count - Number of changed files
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int mm_scanner_check_changes(const char* project_id,
                             const char* root_path,
                             char*** changed_files,
                             size_t* count);

/**
 * Free scanner result resources.
 */
void mm_scanner_free_result(mm_scanner_result_t* result);

#endif /* KATRA_MM_SCANNER_H */
