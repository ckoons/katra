/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_SOFTDEV_H
#define KATRA_SOFTDEV_H

/**
 * @file katra_softdev.h
 * @brief Software Development Module for Katra
 *
 * Softdev extends Katra with "metamemory" - mutable, indexed understanding
 * of codebases. Unlike regular memories (permanent, experiential), metamemory
 * represents the current state of code and updates as code changes.
 *
 * Three-layer architecture:
 *   - Concept layer: Big picture domains, purposes ("catalog manipulation")
 *   - Component layer: Modules, files, their roles ("src/catalogs/ handles catalogs")
 *   - Code layer: Functions, structs, signatures ("load_catalog() at line 45")
 *
 * Entry points allow natural queries:
 *   - "add Hipparcos catalog" -> finds concept:catalog_manipulation -> src/catalogs/
 *   - "what handles viewing?" -> finds concept:viewing_planning -> src/planning/
 *
 * Usage:
 *   1. softdev_init() - Initialize module
 *   2. softdev_analyze_project() - Scan and index a codebase
 *   3. softdev_find_*() - Query metamemory
 *   4. softdev_refresh() - Update index after code changes
 */

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#include "katra_metamemory.h"

/* ============================================================================
 * Constants
 * ============================================================================ */

/* Module identification */
#define SOFTDEV_MODULE_NAME       "softdev"
#define SOFTDEV_MODULE_VERSION    "0.1.0"

/* Operation names for MCP */
#define SOFTDEV_OP_ANALYZE        "softdev_analyze_project"
#define SOFTDEV_OP_FIND_CONCEPT   "softdev_find_concept"
#define SOFTDEV_OP_FIND_CODE      "softdev_find_code"
#define SOFTDEV_OP_IMPACT         "softdev_impact"
#define SOFTDEV_OP_REFRESH        "softdev_refresh"
#define SOFTDEV_OP_ADD_CONCEPT    "softdev_add_concept"
#define SOFTDEV_OP_STATUS         "softdev_status"

/* Analysis depth levels */
typedef enum {
    SOFTDEV_DEPTH_STRUCTURE = 0,  /* Directory structure only */
    SOFTDEV_DEPTH_SIGNATURES,     /* + function signatures, struct definitions */
    SOFTDEV_DEPTH_RELATIONSHIPS,  /* + call graphs, dependencies */
    SOFTDEV_DEPTH_FULL            /* + pattern extraction, purpose inference */
} softdev_depth_t;

/* Supported languages */
typedef enum {
    SOFTDEV_LANG_UNKNOWN = 0,
    SOFTDEV_LANG_C,
    SOFTDEV_LANG_PYTHON,
    SOFTDEV_LANG_JAVASCRIPT,
    SOFTDEV_LANG_GENERIC         /* Fallback: structure only */
} softdev_language_t;

/* ============================================================================
 * Project Configuration
 * ============================================================================ */

/**
 * Project configuration for analysis.
 */
typedef struct {
    const char* project_id;       /* Unique identifier for this project */
    const char* root_path;        /* Absolute path to project root */
    const char* name;             /* Human-readable project name */

    /* Analysis options */
    softdev_depth_t depth;        /* How deep to analyze */
    softdev_language_t primary_language;  /* Primary language (for parsing) */

    /* Exclusions */
    const char** exclude_dirs;    /* Directories to skip (e.g., "node_modules") */
    size_t exclude_dir_count;
    const char** exclude_patterns; /* File patterns to skip (e.g., "*.min.js") */
    size_t exclude_pattern_count;

    /* Options */
    bool incremental;             /* Only scan changed files */
    bool extract_concepts;        /* Attempt to infer concepts from structure */
} softdev_project_config_t;

/* ============================================================================
 * Analysis Results
 * ============================================================================ */

/**
 * Results from project analysis.
 */
typedef struct {
    const char* project_id;
    time_t analyzed_at;

    /* Counts */
    size_t directories_scanned;
    size_t files_scanned;
    size_t functions_indexed;
    size_t structs_indexed;
    size_t concepts_created;

    /* Timing */
    double scan_duration_ms;
    double parse_duration_ms;
    double index_duration_ms;

    /* Status */
    int errors_encountered;
    char* error_summary;          /* NULL if no errors */
} softdev_analysis_result_t;

/**
 * Impact analysis result.
 */
typedef struct {
    metamemory_node_t* target;    /* The node being analyzed */

    metamemory_node_t** directly_affected;  /* Direct dependents */
    size_t directly_affected_count;

    metamemory_node_t** transitively_affected;  /* Transitive dependents */
    size_t transitively_affected_count;

    char** affected_files;        /* Files that would need changes */
    size_t affected_file_count;

    char* summary;                /* Human-readable impact summary */
} softdev_impact_result_t;

/* ============================================================================
 * Module Lifecycle
 * ============================================================================ */

/**
 * Initialize the softdev module.
 *
 * Must be called before any other softdev functions.
 * Registers MCP operations with unified dispatch.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_* on initialization failure
 */
int softdev_init(void);

/**
 * Shutdown the softdev module.
 *
 * Closes database connections, frees resources.
 */
void softdev_shutdown(void);

/**
 * Check if module is initialized.
 */
bool softdev_is_initialized(void);

/* ============================================================================
 * Project Analysis
 * ============================================================================ */

/**
 * Analyze a project and build metamemory index.
 *
 * Scans the project according to config, extracts:
 *   - Directory structure
 *   - Function signatures (language-dependent)
 *   - Data structure definitions
 *   - Dependencies and relationships
 *   - Concepts (if extract_concepts=true)
 *
 * Parameters:
 *   config - Project configuration
 *   result - Analysis results (caller should check, can be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if config is NULL
 *   E_SYSTEM_FILE if project path doesn't exist
 */
int softdev_analyze_project(const softdev_project_config_t* config,
                            softdev_analysis_result_t* result);

/**
 * Refresh metamemory for changed files.
 *
 * Detects which files have changed since last analysis
 * and updates only those entries.
 *
 * Parameters:
 *   project_id - Project to refresh
 *   files_updated - Output: number of files re-indexed (can be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_NOT_FOUND if project hasn't been analyzed
 */
int softdev_refresh(const char* project_id, size_t* files_updated);

/* ============================================================================
 * Query Operations
 * ============================================================================ */

/**
 * Find concepts matching a query.
 *
 * Searches concept names, purposes, and typical_tasks.
 *
 * Parameters:
 *   project_id - Project to search
 *   query - Natural language query ("catalog", "viewing planning")
 *   results - Array of matching nodes (caller must free)
 *   count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int softdev_find_concept(const char* project_id,
                         const char* query,
                         metamemory_node_t*** results,
                         size_t* count);

/**
 * Find code elements matching a query.
 *
 * Searches function names, signatures, struct names.
 *
 * Parameters:
 *   project_id - Project to search
 *   query - Search query ("load_catalog", "position_3d_t")
 *   types - Filter by types (NULL = all types)
 *   type_count - Number of type filters
 *   results - Array of matching nodes (caller must free)
 *   count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int softdev_find_code(const char* project_id,
                      const char* query,
                      const metamemory_type_t* types,
                      size_t type_count,
                      metamemory_node_t*** results,
                      size_t* count);

/**
 * Find what implements a concept.
 *
 * Given a concept, returns all code that implements it.
 *
 * Parameters:
 *   project_id - Project to search
 *   concept_id - Concept to look up
 *   results - Array of implementing nodes (caller must free)
 *   count - Number of results
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_NOT_FOUND if concept doesn't exist
 */
int softdev_what_implements(const char* project_id,
                            const char* concept_id,
                            metamemory_node_t*** results,
                            size_t* count);

/**
 * Analyze impact of changing a code element.
 *
 * Traces dependencies to find what would be affected
 * by modifying the target.
 *
 * Parameters:
 *   project_id - Project to analyze
 *   node_id - Node to analyze ("func:load_catalog")
 *   result - Impact analysis result (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_NOT_FOUND if node doesn't exist
 */
int softdev_impact(const char* project_id,
                   const char* node_id,
                   softdev_impact_result_t** result);

/* ============================================================================
 * Concept Management
 * ============================================================================ */

/**
 * Add a concept to the project.
 *
 * Concepts can be added by:
 *   - CI inference during analysis
 *   - Explicit CI curation during work
 *   - User explanation
 *
 * Parameters:
 *   project_id - Project to add concept to
 *   concept - Concept node to add
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_ALREADY_EXISTS if concept ID already exists
 */
int softdev_add_concept(const char* project_id,
                        const metamemory_node_t* concept);

/**
 * Link code to a concept.
 *
 * Establishes that a code element implements a concept.
 *
 * Parameters:
 *   project_id - Project
 *   code_id - Code node ID ("func:load_catalog")
 *   concept_id - Concept ID ("concept:catalog_manipulation")
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_NOT_FOUND if either node doesn't exist
 */
int softdev_link_to_concept(const char* project_id,
                            const char* code_id,
                            const char* concept_id);

/* ============================================================================
 * Status and Utilities
 * ============================================================================ */

/**
 * Get status of a project's metamemory.
 */
typedef struct {
    const char* project_id;
    const char* project_name;
    const char* root_path;

    time_t last_analyzed;
    time_t last_refreshed;

    size_t concept_count;
    size_t component_count;
    size_t function_count;
    size_t struct_count;
    size_t total_nodes;

    bool needs_refresh;           /* Files changed since last analysis */
    size_t stale_file_count;      /* Number of changed files */
} softdev_status_t;

/**
 * Get project metamemory status.
 */
int softdev_get_status(const char* project_id, softdev_status_t* status);

/**
 * List all analyzed projects.
 */
int softdev_list_projects(char*** project_ids, size_t* count);

/**
 * Free analysis result.
 */
void softdev_free_analysis_result(softdev_analysis_result_t* result);

/**
 * Free impact result.
 */
void softdev_free_impact_result(softdev_impact_result_t* result);

/**
 * Free status.
 */
void softdev_free_status(softdev_status_t* status);

#endif /* KATRA_SOFTDEV_H */
