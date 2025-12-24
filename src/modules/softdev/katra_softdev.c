/* © 2025 Casey Koons All rights reserved */

/**
 * @file katra_softdev.c
 * @brief Software Development Module - Core initialization and lifecycle
 *
 * This module extends Katra with "metamemory" - mutable, indexed understanding
 * of codebases. It provides:
 *   - Project analysis and indexing
 *   - Concept layer for semantic understanding
 *   - Impact analysis for safe editing
 *   - Query operations for CI navigation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "katra_softdev.h"
#include "katra_metamemory.h"
#include "katra_error.h"
#include "katra_limits.h"

/* ============================================================================
 * Module State
 * ============================================================================ */

/* Module initialization state */
static bool g_softdev_initialized = false;

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

/* Internal initialization functions */
static int softdev_init_index(void);
static void softdev_cleanup_index(void);
static int softdev_register_operations(void);

/* ============================================================================
 * Module Lifecycle
 * ============================================================================ */

int softdev_init(void)
{
    int result = KATRA_SUCCESS;

    if (g_softdev_initialized) {
        return KATRA_SUCCESS;  /* Already initialized */
    }

    /* Initialize metamemory index (SQLite) */
    result = softdev_init_index();
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "softdev_init",
                          "Failed to initialize metamemory index");
        goto cleanup;
    }

    /* Register MCP operations with unified dispatch */
    result = softdev_register_operations();
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "softdev_init",
                          "Failed to register MCP operations");
        goto cleanup;
    }

    g_softdev_initialized = true;
    return KATRA_SUCCESS;

cleanup:
    softdev_cleanup_index();
    return result;
}

void softdev_shutdown(void)
{
    if (!g_softdev_initialized) {
        return;
    }

    softdev_cleanup_index();
    g_softdev_initialized = false;
}

bool softdev_is_initialized(void)
{
    return g_softdev_initialized;
}

/* ============================================================================
 * Internal: Index Management (Stub - implemented in katra_mm_index.c)
 * ============================================================================ */

static int softdev_init_index(void)
{
    /* TODO: Initialize SQLite database for metamemory
     * Location: ~/.katra/softdev/<project_id>/metamemory.db
     *
     * Tables:
     *   - nodes: Main metamemory nodes
     *   - links: Node relationships
     *   - node_fts: Full-text search on name, purpose, tasks
     *   - file_hashes: For change detection
     */
    return KATRA_SUCCESS;
}

static void softdev_cleanup_index(void)
{
    /* TODO: Close database connections, free resources */
}

/* ============================================================================
 * Internal: Operation Registration (Stub)
 * ============================================================================ */

static int softdev_register_operations(void)
{
    /* TODO: Register operations with unified dispatch
     *
     * Operations to register:
     *   - softdev_analyze_project
     *   - softdev_find_concept
     *   - softdev_find_code
     *   - softdev_impact
     *   - softdev_refresh
     *   - softdev_add_concept
     *   - softdev_status
     */
    return KATRA_SUCCESS;
}

/* ============================================================================
 * Project Analysis (Stub - implemented in katra_mm_scanner.c)
 * ============================================================================ */

int softdev_analyze_project(const softdev_project_config_t* config,
                            softdev_analysis_result_t* result)
{
    if (!config) {
        katra_report_error(E_INPUT_NULL, "softdev_analyze_project",
                          "config is NULL");
        return E_INPUT_NULL;
    }

    if (!config->project_id || !config->root_path) {
        katra_report_error(E_INPUT_NULL, "softdev_analyze_project",
                          "project_id or root_path is NULL");
        return E_INPUT_NULL;
    }

    if (!g_softdev_initialized) {
        katra_report_error(E_INVALID_STATE, "softdev_analyze_project",
                          "softdev module not initialized");
        return E_INVALID_STATE;
    }

    /* Initialize result if provided */
    if (result) {
        memset(result, 0, sizeof(*result));
        result->project_id = config->project_id;
        result->analyzed_at = time(NULL);
    }

    /* TODO: Implement analysis workflow
     *
     * Phase 1: Structure
     *   - Walk directory tree
     *   - Identify build system (Makefile, CMake, etc.)
     *   - Map module boundaries
     *
     * Phase 2: Symbols (if depth >= SIGNATURES)
     *   - Parse files based on language
     *   - Extract function signatures
     *   - Extract struct definitions
     *
     * Phase 3: Relationships (if depth >= RELATIONSHIPS)
     *   - Build call graph
     *   - Map type usage
     *   - Track includes
     *
     * Phase 4: Concepts (if extract_concepts)
     *   - Infer concepts from directory names
     *   - Group related functions
     *   - Create initial concept layer
     */

    return KATRA_SUCCESS;
}

int softdev_refresh(const char* project_id, size_t* files_updated)
{
    if (!project_id) {
        katra_report_error(E_INPUT_NULL, "softdev_refresh",
                          "project_id is NULL");
        return E_INPUT_NULL;
    }

    if (!g_softdev_initialized) {
        katra_report_error(E_INVALID_STATE, "softdev_refresh",
                          "softdev module not initialized");
        return E_INVALID_STATE;
    }

    if (files_updated) {
        *files_updated = 0;
    }

    /* TODO: Implement incremental refresh
     *
     * 1. Load file hashes from database
     * 2. Compare with current file hashes
     * 3. Re-index changed files only
     * 4. Update relationships affected by changes
     */

    return KATRA_SUCCESS;
}

/* ============================================================================
 * Query Operations (Stubs)
 * ============================================================================ */

int softdev_find_concept(const char* project_id,
                         const char* query,
                         metamemory_node_t*** results,
                         size_t* count)
{
    if (!project_id || !query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "softdev_find_concept",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    *results = NULL;
    *count = 0;

    /* TODO: Search metamemory for concepts matching query
     *
     * Search in:
     *   - Concept names
     *   - Concept purposes
     *   - Typical tasks
     *
     * Use FTS for natural language matching
     */

    return KATRA_SUCCESS;
}

int softdev_find_code(const char* project_id,
                      const char* query,
                      const metamemory_type_t* types,
                      size_t type_count,
                      metamemory_node_t*** results,
                      size_t* count)
{
    (void)types;       /* Unused for now */
    (void)type_count;

    if (!project_id || !query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "softdev_find_code",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    *results = NULL;
    *count = 0;

    /* TODO: Search metamemory for code elements
     *
     * Search in:
     *   - Function names
     *   - Struct names
     *   - Signatures
     *
     * Filter by type if types array provided
     */

    return KATRA_SUCCESS;
}

int softdev_what_implements(const char* project_id,
                            const char* concept_id,
                            metamemory_node_t*** results,
                            size_t* count)
{
    if (!project_id || !concept_id || !results || !count) {
        katra_report_error(E_INPUT_NULL, "softdev_what_implements",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    *results = NULL;
    *count = 0;

    /* TODO: Follow concept -> implements links
     *
     * 1. Look up concept by ID
     * 2. Return all nodes in implements array
     */

    return KATRA_SUCCESS;
}

int softdev_impact(const char* project_id,
                   const char* node_id,
                   softdev_impact_result_t** result)
{
    if (!project_id || !node_id || !result) {
        katra_report_error(E_INPUT_NULL, "softdev_impact",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    *result = NULL;

    /* TODO: Trace dependencies to find impact
     *
     * 1. Look up target node
     * 2. Follow called_by links for direct dependents
     * 3. Recursively follow for transitive dependents
     * 4. Collect affected files
     * 5. Generate summary
     */

    return KATRA_SUCCESS;
}

/* ============================================================================
 * Concept Management (Stubs)
 * ============================================================================ */

int softdev_add_concept(const char* project_id,
                        const metamemory_node_t* concept)
{
    if (!project_id || !concept) {
        katra_report_error(E_INPUT_NULL, "softdev_add_concept",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (concept->type != METAMEMORY_TYPE_CONCEPT) {
        katra_report_error(E_INPUT_INVALID, "softdev_add_concept",
                          "Node is not a concept type");
        return E_INPUT_INVALID;
    }

    /* TODO: Store concept in metamemory index
     *
     * 1. Check if concept ID already exists
     * 2. Insert into nodes table
     * 3. Insert into FTS index
     */

    return KATRA_SUCCESS;
}

int softdev_link_to_concept(const char* project_id,
                            const char* code_id,
                            const char* concept_id)
{
    if (!project_id || !code_id || !concept_id) {
        katra_report_error(E_INPUT_NULL, "softdev_link_to_concept",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    /* TODO: Create bidirectional link
     *
     * 1. Add concept_id to code node's implemented_by
     * 2. Add code_id to concept node's implements
     * 3. Update database
     */

    return KATRA_SUCCESS;
}

/* ============================================================================
 * Status and Utilities
 * ============================================================================ */

int softdev_get_status(const char* project_id, softdev_status_t* status)
{
    if (!project_id || !status) {
        katra_report_error(E_INPUT_NULL, "softdev_get_status",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    memset(status, 0, sizeof(*status));
    status->project_id = project_id;

    /* TODO: Query database for project statistics
     *
     * 1. Count nodes by type
     * 2. Get last analyzed/refreshed times
     * 3. Check for stale files
     */

    return KATRA_SUCCESS;
}

int softdev_list_projects(char*** project_ids, size_t* count)
{
    if (!project_ids || !count) {
        katra_report_error(E_INPUT_NULL, "softdev_list_projects",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    *project_ids = NULL;
    *count = 0;

    /* TODO: List all analyzed projects
     *
     * 1. Scan ~/.katra/softdev/ directories
     * 2. Return project IDs
     */

    return KATRA_SUCCESS;
}

/* ============================================================================
 * Memory Management
 * ============================================================================ */

void softdev_free_analysis_result(softdev_analysis_result_t* result)
{
    if (!result) {
        return;
    }

    free(result->error_summary);
    memset(result, 0, sizeof(*result));
}

void softdev_free_impact_result(softdev_impact_result_t* result)
{
    if (!result) {
        return;
    }

    metamemory_free_node(result->target);
    metamemory_free_nodes(result->directly_affected,
                          result->directly_affected_count);
    metamemory_free_nodes(result->transitively_affected,
                          result->transitively_affected_count);

    for (size_t i = 0; i < result->affected_file_count; i++) {
        free(result->affected_files[i]);
    }
    free(result->affected_files);
    free(result->summary);
    free(result);
}

void softdev_free_status(softdev_status_t* status)
{
    if (!status) {
        return;
    }

    /* Status contains const pointers, nothing to free */
    memset(status, 0, sizeof(*status));
}
