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
 *
 * This file also implements the module interface for dynamic loading.
 * Required exports:
 *   - katra_module_info()
 *   - katra_module_init()
 *   - katra_module_shutdown()
 *   - katra_module_register_ops()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "katra_softdev.h"
#include "katra_metamemory.h"
#include "katra_module.h"
#include "katra_error.h"
#include "katra_limits.h"
#include <jansson.h>

/* ============================================================================
 * Module State
 * ============================================================================ */

/* Module initialization state */
static bool g_softdev_initialized = false;

/* Module context (from loader) */
static katra_module_context_t* g_module_context = NULL;

/* ============================================================================
 * Module Information (for dynamic loading)
 * ============================================================================ */

/* Capabilities this module provides */
static const char* g_provides[] = {
    "metamemory",
    "code_analysis",
    "impact_analysis"
};

/* Module information structure */
static katra_module_info_t g_module_info = {
    .name = SOFTDEV_MODULE_NAME,
    .version = SOFTDEV_MODULE_VERSION,
    .description = "Software development metamemory - indexed code understanding",
    .author = "Casey Koons",
    .api_version = KATRA_MODULE_API_VERSION,
    .min_katra_version = "0.1.0",
    .requires = NULL,
    .requires_count = 0,
    .provides = g_provides,
    .provides_count = 3
};

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

/* Internal initialization functions */
static int softdev_init_index(void);
static void softdev_cleanup_index(void);
static int softdev_register_operations(void);

/* MCP Operation handlers */
static json_t* handle_analyze_project(json_t* params, const char* ci_name);
static json_t* handle_find_concept(json_t* params, const char* ci_name);
static json_t* handle_find_code(json_t* params, const char* ci_name);
static json_t* handle_impact(json_t* params, const char* ci_name);
static json_t* handle_refresh(json_t* params, const char* ci_name);
static json_t* handle_add_concept(json_t* params, const char* ci_name);
static json_t* handle_status(json_t* params, const char* ci_name);

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

/* ============================================================================
 * Module Interface Exports (Required for Dynamic Loading)
 * ============================================================================ */

/**
 * Get module information.
 * Called during discovery (before full load).
 */
katra_module_info_t* katra_module_info(void)
{
    return &g_module_info;
}

/**
 * Initialize module.
 * Called when module is loaded.
 */
int katra_module_init(katra_module_context_t* ctx)
{
    if (!ctx) {
        katra_report_error(E_INPUT_NULL, "katra_module_init",
                          "module context is NULL");
        return E_INPUT_NULL;
    }

    /* Store context for later use */
    g_module_context = ctx;

    /* Initialize softdev subsystem */
    return softdev_init();
}

/**
 * Shutdown module.
 * Called when module is unloaded.
 */
void katra_module_shutdown(void)
{
    softdev_shutdown();
    g_module_context = NULL;
}

/**
 * Register module operations with MCP dispatch.
 * Called after init, before module is considered ready.
 */
int katra_module_register_ops(katra_op_registry_t* registry)
{
    int result = KATRA_SUCCESS;

    if (!registry) {
        katra_report_error(E_INPUT_NULL, "katra_module_register_ops",
                          "registry is NULL");
        return E_INPUT_NULL;
    }

    /* Register all softdev operations */
    result = registry->register_op(
        SOFTDEV_OP_ANALYZE,
        "Analyze a project and build metamemory index",
        handle_analyze_project,
        NULL  /* TODO: Add JSON schema */
    );
    if (result != KATRA_SUCCESS) return result;

    result = registry->register_op(
        SOFTDEV_OP_FIND_CONCEPT,
        "Find concepts matching a query",
        handle_find_concept,
        NULL
    );
    if (result != KATRA_SUCCESS) return result;

    result = registry->register_op(
        SOFTDEV_OP_FIND_CODE,
        "Find code elements matching a query",
        handle_find_code,
        NULL
    );
    if (result != KATRA_SUCCESS) return result;

    result = registry->register_op(
        SOFTDEV_OP_IMPACT,
        "Analyze impact of changing a code element",
        handle_impact,
        NULL
    );
    if (result != KATRA_SUCCESS) return result;

    result = registry->register_op(
        SOFTDEV_OP_REFRESH,
        "Refresh metamemory for changed files",
        handle_refresh,
        NULL
    );
    if (result != KATRA_SUCCESS) return result;

    result = registry->register_op(
        SOFTDEV_OP_ADD_CONCEPT,
        "Add a concept to the project",
        handle_add_concept,
        NULL
    );
    if (result != KATRA_SUCCESS) return result;

    result = registry->register_op(
        SOFTDEV_OP_STATUS,
        "Get project metamemory status",
        handle_status,
        NULL
    );

    return result;
}

/* ============================================================================
 * MCP Operation Handlers
 * ============================================================================ */

static json_t* handle_analyze_project(json_t* params, const char* ci_name)
{
    (void)ci_name;  /* CI attribution for logging */

    const char* project_id = json_string_value(json_object_get(params, "project_id"));
    const char* root_path = json_string_value(json_object_get(params, "root_path"));
    const char* depth_str = json_string_value(json_object_get(params, "depth"));

    if (!project_id || !root_path) {
        return json_pack("{s:s}", "error", "project_id and root_path required");
    }

    softdev_project_config_t config = {0};
    config.project_id = project_id;
    config.root_path = root_path;
    config.depth = SOFTDEV_DEPTH_FULL;

    /* Parse depth if provided */
    if (depth_str) {
        if (strcmp(depth_str, "structure") == 0) {
            config.depth = SOFTDEV_DEPTH_STRUCTURE;
        } else if (strcmp(depth_str, "signatures") == 0) {
            config.depth = SOFTDEV_DEPTH_SIGNATURES;
        } else if (strcmp(depth_str, "relationships") == 0) {
            config.depth = SOFTDEV_DEPTH_RELATIONSHIPS;
        }
    }

    softdev_analysis_result_t result = {0};
    int rc = softdev_analyze_project(&config, &result);

    if (rc != KATRA_SUCCESS) {
        return json_pack("{s:s,s:i}", "error", "Analysis failed", "code", rc);
    }

    json_t* response = json_pack("{s:s,s:i,s:i,s:i,s:i,s:i}",
        "project_id", result.project_id,
        "directories", (int)result.directories_scanned,
        "files", (int)result.files_scanned,
        "functions", (int)result.functions_indexed,
        "structs", (int)result.structs_indexed,
        "concepts", (int)result.concepts_created
    );

    softdev_free_analysis_result(&result);
    return response;
}

static json_t* handle_find_concept(json_t* params, const char* ci_name)
{
    (void)ci_name;

    const char* project_id = json_string_value(json_object_get(params, "project_id"));
    const char* query = json_string_value(json_object_get(params, "query"));

    if (!project_id || !query) {
        return json_pack("{s:s}", "error", "project_id and query required");
    }

    metamemory_node_t** results = NULL;
    size_t count = 0;

    int rc = softdev_find_concept(project_id, query, &results, &count);
    if (rc != KATRA_SUCCESS) {
        return json_pack("{s:s,s:i}", "error", "Find failed", "code", rc);
    }

    json_t* nodes = json_array();
    for (size_t i = 0; i < count; i++) {
        json_t* node = json_pack("{s:s,s:s,s:s}",
            "id", results[i]->id,
            "name", results[i]->name,
            "purpose", results[i]->purpose ? results[i]->purpose : ""
        );
        json_array_append_new(nodes, node);
    }

    metamemory_free_nodes(results, count);
    return json_pack("{s:o,s:i}", "results", nodes, "count", (int)count);
}

static json_t* handle_find_code(json_t* params, const char* ci_name)
{
    (void)ci_name;

    const char* project_id = json_string_value(json_object_get(params, "project_id"));
    const char* query = json_string_value(json_object_get(params, "query"));

    if (!project_id || !query) {
        return json_pack("{s:s}", "error", "project_id and query required");
    }

    metamemory_node_t** results = NULL;
    size_t count = 0;

    int rc = softdev_find_code(project_id, query, NULL, 0, &results, &count);
    if (rc != KATRA_SUCCESS) {
        return json_pack("{s:s,s:i}", "error", "Find failed", "code", rc);
    }

    json_t* nodes = json_array();
    for (size_t i = 0; i < count; i++) {
        json_t* node = json_pack("{s:s,s:s,s:s,s:s}",
            "id", results[i]->id,
            "name", results[i]->name,
            "signature", results[i]->signature ? results[i]->signature : "",
            "file", results[i]->location.file_path ? results[i]->location.file_path : ""
        );
        json_array_append_new(nodes, node);
    }

    metamemory_free_nodes(results, count);
    return json_pack("{s:o,s:i}", "results", nodes, "count", (int)count);
}

static json_t* handle_impact(json_t* params, const char* ci_name)
{
    (void)ci_name;

    const char* project_id = json_string_value(json_object_get(params, "project_id"));
    const char* node_id = json_string_value(json_object_get(params, "node_id"));

    if (!project_id || !node_id) {
        return json_pack("{s:s}", "error", "project_id and node_id required");
    }

    softdev_impact_result_t* result = NULL;
    int rc = softdev_impact(project_id, node_id, &result);

    if (rc != KATRA_SUCCESS) {
        return json_pack("{s:s,s:i}", "error", "Impact analysis failed", "code", rc);
    }

    /* Build response */
    json_t* response = json_pack("{s:i,s:i,s:i,s:s}",
        "directly_affected", result ? (int)result->directly_affected_count : 0,
        "transitively_affected", result ? (int)result->transitively_affected_count : 0,
        "affected_files", result ? (int)result->affected_file_count : 0,
        "summary", result && result->summary ? result->summary : "No impact data"
    );

    softdev_free_impact_result(result);
    return response;
}

static json_t* handle_refresh(json_t* params, const char* ci_name)
{
    (void)ci_name;

    const char* project_id = json_string_value(json_object_get(params, "project_id"));

    if (!project_id) {
        return json_pack("{s:s}", "error", "project_id required");
    }

    size_t files_updated = 0;
    int rc = softdev_refresh(project_id, &files_updated);

    if (rc != KATRA_SUCCESS) {
        return json_pack("{s:s,s:i}", "error", "Refresh failed", "code", rc);
    }

    return json_pack("{s:s,s:i}",
        "project_id", project_id,
        "files_updated", (int)files_updated
    );
}

static json_t* handle_add_concept(json_t* params, const char* ci_name)
{
    (void)ci_name;

    const char* project_id = json_string_value(json_object_get(params, "project_id"));
    const char* name = json_string_value(json_object_get(params, "name"));
    const char* purpose = json_string_value(json_object_get(params, "purpose"));

    if (!project_id || !name) {
        return json_pack("{s:s}", "error", "project_id and name required");
    }

    /* Create concept node (no tasks for now) */
    metamemory_node_t* concept = metamemory_create_concept(project_id, name, purpose,
                                                            NULL, 0);
    if (!concept) {
        return json_pack("{s:s}", "error", "Failed to create concept");
    }

    int rc = softdev_add_concept(project_id, concept);
    char* concept_id = concept->id ? strdup(concept->id) : NULL;
    metamemory_free_node(concept);

    if (rc != KATRA_SUCCESS) {
        free(concept_id);
        return json_pack("{s:s,s:i}", "error", "Add concept failed", "code", rc);
    }

    json_t* response = json_pack("{s:s,s:s}",
        "status", "created",
        "id", concept_id ? concept_id : "unknown"
    );
    free(concept_id);
    return response;
}

static json_t* handle_status(json_t* params, const char* ci_name)
{
    (void)ci_name;

    const char* project_id = json_string_value(json_object_get(params, "project_id"));

    if (!project_id) {
        return json_pack("{s:s}", "error", "project_id required");
    }

    softdev_status_t status = {0};
    int rc = softdev_get_status(project_id, &status);

    if (rc != KATRA_SUCCESS) {
        return json_pack("{s:s,s:i}", "error", "Get status failed", "code", rc);
    }

    json_t* response = json_pack("{s:s,s:i,s:i,s:i,s:i,s:i,s:b}",
        "project_id", status.project_id,
        "concepts", (int)status.concept_count,
        "components", (int)status.component_count,
        "functions", (int)status.function_count,
        "structs", (int)status.struct_count,
        "total_nodes", (int)status.total_nodes,
        "needs_refresh", status.needs_refresh
    );

    softdev_free_status(&status);
    return response;
}
