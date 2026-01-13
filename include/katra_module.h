/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_MODULE_H
#define KATRA_MODULE_H

/**
 * @file katra_module.h
 * @brief Dynamic Module Loading API
 *
 * Katra supports loadable capability modules as shared libraries.
 * This header defines:
 *   - The contract that modules must implement (exports)
 *   - The context provided to modules during initialization
 *   - The loader API for managing modules
 *
 * Module authors: Include this header and implement the required exports.
 * Daemon code: Use the loader API to discover, load, and manage modules.
 */

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* Forward declarations */
typedef struct json_t json_t;

/* ============================================================================
 * Module API Version
 * ============================================================================ */

/**
 * Current module API version.
 * Increment when making breaking changes to the module interface.
 */
#define KATRA_MODULE_API_VERSION 1

/**
 * Minimum supported API version for backwards compatibility.
 */
#define KATRA_MODULE_API_MIN_VERSION 1

/* ============================================================================
 * Module Constants
 * ============================================================================ */

/* Maximum lengths */
#define KATRA_MODULE_NAME_MAX        64
#define KATRA_MODULE_VERSION_MAX     32
#define KATRA_MODULE_DESC_MAX        256
#define KATRA_MODULE_AUTHOR_MAX      128
#define KATRA_MODULE_MAX_DEPS        16
#define KATRA_MODULE_MAX_PROVIDES    16
#define KATRA_MODULE_MAX_OPS         64

/* Module file extension */
#ifdef __APPLE__
    #define KATRA_MODULE_EXT ".dylib"
#else
    #define KATRA_MODULE_EXT ".so"
#endif

/* Module file prefix */
#define KATRA_MODULE_PREFIX "katra_"

/* ============================================================================
 * Module Information (exported by modules)
 * ============================================================================ */

/**
 * Module information structure.
 *
 * Every module must provide this via katra_module_info().
 * Used for discovery, compatibility checking, and display.
 */
typedef struct {
    /* Identification */
    const char* name;              /* Short name: "softdev" */
    const char* version;           /* Semantic version: "0.1.0" */
    const char* description;       /* Human readable description */
    const char* author;            /* Author name/email */

    /* Compatibility */
    int api_version;               /* KATRA_MODULE_API_VERSION when built */
    const char* min_katra_version; /* Minimum Katra version required */

    /* Dependencies (optional) */
    const char** requires;         /* Module dependencies: ["other>=1.0"] */
    size_t requires_count;

    /* Capabilities (optional) */
    const char** provides;         /* Capabilities provided: ["metamemory"] */
    size_t provides_count;
} katra_module_info_t;

/* ============================================================================
 * Module Context (provided to modules)
 * ============================================================================ */

/* Forward declarations for service APIs */
typedef struct katra_memory_api katra_memory_api_t;
typedef struct katra_log_api katra_log_api_t;
typedef struct katra_db_api katra_db_api_t;

/**
 * Context passed to modules during initialization.
 *
 * Provides:
 *   - Version information for compatibility
 *   - Paths for data storage
 *   - Service APIs for core functionality
 */
typedef struct {
    /* Version info */
    const char* katra_version;     /* Katra version string */
    int api_version;               /* Current API version */

    /* Paths */
    const char* katra_dir;         /* ~/.katra */
    const char* module_dir;        /* ~/.katra/modules */
    const char* module_data_dir;   /* ~/.katra/<module_name> */

    /* Service APIs (opaque, use through function pointers) */
    katra_memory_api_t* memory;    /* Memory system access */
    katra_log_api_t* log;          /* Logging */
    katra_db_api_t* db;            /* Database access */
} katra_module_context_t;

/* ============================================================================
 * Operation Registry (for MCP registration)
 * ============================================================================ */

/**
 * Operation handler function signature.
 *
 * Parameters:
 *   params - JSON object with operation parameters
 *   ci_name - Name of the CI making the request
 *
 * Returns:
 *   JSON result object (caller frees)
 */
typedef json_t* (*katra_op_handler_t)(json_t* params, const char* ci_name);

/**
 * Operation registry for modules to register MCP operations.
 */
typedef struct {
    /**
     * Register an operation.
     *
     * Parameters:
     *   name - Operation name ("softdev_analyze_project")
     *   description - Human readable description
     *   handler - Function to handle the operation
     *   input_schema - JSON schema for input validation (can be NULL)
     *
     * Returns:
     *   KATRA_SUCCESS on success
     *   E_ALREADY_EXISTS if operation name taken
     *   E_RESOURCE_LIMIT if max operations reached
     */
    int (*register_op)(
        const char* name,
        const char* description,
        katra_op_handler_t handler,
        json_t* input_schema
    );

    /**
     * Unregister an operation.
     */
    int (*unregister_op)(const char* name);

    /* Internal - module name for tracking */
    const char* _module_name;
} katra_op_registry_t;

/* ============================================================================
 * Module Export Function Types
 * ============================================================================ */

/**
 * Get module information.
 * Called during discovery (before full load).
 */
typedef katra_module_info_t* (*katra_module_info_fn)(void);

/**
 * Initialize module.
 * Called when module is loaded.
 */
typedef int (*katra_module_init_fn)(katra_module_context_t* ctx);

/**
 * Shutdown module.
 * Called when module is unloaded.
 */
typedef void (*katra_module_shutdown_fn)(void);

/**
 * Register module operations.
 * Called after init, before module is considered ready.
 */
typedef int (*katra_module_register_ops_fn)(katra_op_registry_t* registry);

/* ============================================================================
 * Module Export Symbol Names
 * ============================================================================ */

#define KATRA_MODULE_INFO_SYMBOL       "katra_module_info"
#define KATRA_MODULE_INIT_SYMBOL       "katra_module_init"
#define KATRA_MODULE_SHUTDOWN_SYMBOL   "katra_module_shutdown"
#define KATRA_MODULE_REGISTER_SYMBOL   "katra_module_register_ops"

/* ============================================================================
 * Loader API (used by daemon)
 * ============================================================================ */

/**
 * Module state.
 */
typedef enum {
    KATRA_MODULE_STATE_UNKNOWN = 0,
    KATRA_MODULE_STATE_AVAILABLE,    /* Discovered but not loaded */
    KATRA_MODULE_STATE_LOADING,      /* Currently being loaded */
    KATRA_MODULE_STATE_LOADED,       /* Loaded and ready */
    KATRA_MODULE_STATE_FAILED,       /* Load failed */
    KATRA_MODULE_STATE_UNLOADING     /* Currently being unloaded */
} katra_module_state_t;

/**
 * Module entry in registry.
 */
typedef struct {
    char name[KATRA_MODULE_NAME_MAX];
    char version[KATRA_MODULE_VERSION_MAX];
    char description[KATRA_MODULE_DESC_MAX];
    char author[KATRA_MODULE_AUTHOR_MAX];
    char path[512];                  /* Path to shared library */
    int api_version;                 /* Module API version */

    katra_module_state_t state;
    time_t loaded_at;                /* When loaded (0 if not loaded) */

    /* Internal - only valid when loaded */
    void* _handle;                   /* dlopen handle */
    katra_module_shutdown_fn _shutdown_fn;
} katra_module_entry_t;

/**
 * Initialize the module loader.
 *
 * Must be called before any other loader functions.
 * Creates module directory if needed.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if directory creation fails
 */
int katra_module_loader_init(void);

/**
 * Shutdown the module loader.
 *
 * Unloads all loaded modules and frees resources.
 */
void katra_module_loader_shutdown(void);

/**
 * Discover available modules.
 *
 * Scans module directory and probes each module for info.
 * Does not fully load modules.
 *
 * Returns:
 *   Number of modules discovered, or negative error code
 */
int katra_module_loader_discover(void);

/**
 * Get list of discovered modules.
 *
 * Parameters:
 *   entries - Output array (caller must free)
 *   count - Number of entries
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_module_loader_list(katra_module_entry_t** entries, size_t* count);

/**
 * Load a module by name.
 *
 * Parameters:
 *   name - Module name (e.g., "softdev")
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_NOT_FOUND if module not discovered
 *   E_ALREADY_EXISTS if already loaded
 *   E_SYSTEM_FILE if dlopen fails
 *   E_INPUT_INVALID if module invalid
 */
int katra_module_load(const char* name);

/**
 * Unload a module by name.
 *
 * Parameters:
 *   name - Module name
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_NOT_FOUND if not loaded
 */
int katra_module_unload(const char* name);

/**
 * Reload a module (unload + load).
 *
 * Parameters:
 *   name - Module name
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_module_reload(const char* name);

/**
 * Check if a module is loaded.
 *
 * Parameters:
 *   name - Module name
 *
 * Returns:
 *   true if loaded, false otherwise
 */
bool katra_module_is_loaded(const char* name);

/**
 * Get module info by name.
 *
 * Parameters:
 *   name - Module name
 *   entry - Output entry (filled if found)
 *
 * Returns:
 *   KATRA_SUCCESS if found
 *   E_NOT_FOUND if not discovered
 */
int katra_module_get_info(const char* name, katra_module_entry_t* entry);

/**
 * Get module directory path.
 *
 * Returns:
 *   Path to module directory (do not free)
 */
const char* katra_module_get_directory(void);

/**
 * Set module directory path.
 *
 * Must be called before katra_module_loader_init().
 *
 * Parameters:
 *   path - Directory path
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_module_set_directory(const char* path);

/* ============================================================================
 * MCP Operations for Module Management
 * ============================================================================ */

/**
 * MCP operation: List modules.
 * {"method": "modules_list", "params": {"ci_name": "Ami"}}
 */
json_t* katra_mcp_modules_list(json_t* params, const char* ci_name);

/**
 * MCP operation: Load module.
 * {"method": "modules_load", "params": {"module": "softdev", "ci_name": "Ami"}}
 */
json_t* katra_mcp_modules_load(json_t* params, const char* ci_name);

/**
 * MCP operation: Unload module.
 * {"method": "modules_unload", "params": {"module": "softdev", "ci_name": "Ami"}}
 */
json_t* katra_mcp_modules_unload(json_t* params, const char* ci_name);

/**
 * MCP operation: Reload module.
 * {"method": "modules_reload", "params": {"module": "softdev", "ci_name": "Ami"}}
 */
json_t* katra_mcp_modules_reload(json_t* params, const char* ci_name);

/**
 * MCP operation: Get module info.
 * {"method": "modules_info", "params": {"module": "softdev", "ci_name": "Ami"}}
 */
json_t* katra_mcp_modules_info(json_t* params, const char* ci_name);

#endif /* KATRA_MODULE_H */
