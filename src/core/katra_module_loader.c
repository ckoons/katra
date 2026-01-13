/* © 2025 Casey Koons All rights reserved */

/**
 * @file katra_module_loader.c
 * @brief Dynamic Module Loader Implementation
 *
 * Implements discovery, loading, and lifecycle management for
 * Katra capability modules as shared libraries.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>

#include "katra_module.h"
#include "katra_unified.h"
#include "katra_error.h"
#include "katra_limits.h"
#include "katra_log.h"
#include "katra_path_utils.h"
#include <jansson.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define MAX_MODULES 64
#define DEFAULT_MODULE_SUBDIR "modules"
#define MAX_MODULE_OPS 256

/* ============================================================================
 * Operation Registry for Module-to-Dispatcher Bridge
 * ============================================================================ */

/* Entry for a registered module operation */
typedef struct {
    char name[128];                  /* Operation name */
    katra_op_handler_t handler;      /* Module's handler */
    char module_name[64];            /* Owning module */
} module_op_entry_t;

/* Registry of module operations */
static module_op_entry_t g_module_ops[MAX_MODULE_OPS];
static size_t g_module_op_count = 0;
static pthread_mutex_t g_ops_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ============================================================================
 * Module Registry State
 * ============================================================================ */

/* Registry of discovered modules */
static katra_module_entry_t g_modules[MAX_MODULES];
static size_t g_module_count = 0;

/* Module directory path */
static char g_module_dir[KATRA_PATH_MAX] = {0};

/* Initialization state */
static bool g_loader_initialized = false;

/* Thread safety */
static pthread_mutex_t g_loader_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Global operation registry */
static katra_op_registry_t g_op_registry;

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

static int ensure_module_directory(void);
static bool is_module_file(const char* filename);
static int probe_module(const char* path, katra_module_entry_t* entry);
static int find_module_index(const char* name);
static katra_module_context_t* build_module_context(const char* name);
static void free_module_context(katra_module_context_t* ctx);

/* Operation registry callbacks */
static int registry_register_op(const char* name, const char* description,
                                katra_op_handler_t handler, json_t* schema);
static int registry_unregister_op(const char* name);

/* Adapter to bridge unified dispatcher to module handlers */
static json_t* module_op_adapter(json_t* params, const katra_unified_options_t* options);
static module_op_entry_t* find_module_op(const char* name);

/* ============================================================================
 * Loader Initialization
 * ============================================================================ */

int katra_module_loader_init(void)
{
    int result = KATRA_SUCCESS;

    pthread_mutex_lock(&g_loader_mutex);

    if (g_loader_initialized) {
        pthread_mutex_unlock(&g_loader_mutex);
        return KATRA_SUCCESS;
    }

    /* Set default module directory if not already set */
    if (g_module_dir[0] == '\0') {
        const char* home = getenv("HOME");
        if (!home) {
            home = "/tmp";
        }
        snprintf(g_module_dir, sizeof(g_module_dir),
                 "%s/.katra/%s", home, DEFAULT_MODULE_SUBDIR);
    }

    /* Ensure directory exists */
    result = ensure_module_directory();
    if (result != KATRA_SUCCESS) {
        pthread_mutex_unlock(&g_loader_mutex);
        return result;
    }

    /* Initialize operation registry */
    g_op_registry.register_op = registry_register_op;
    g_op_registry.unregister_op = registry_unregister_op;
    g_op_registry._module_name = NULL;

    /* Clear module list */
    memset(g_modules, 0, sizeof(g_modules));
    g_module_count = 0;

    g_loader_initialized = true;
    pthread_mutex_unlock(&g_loader_mutex);

    LOG_INFO("Module loader initialized, directory: %s", g_module_dir);
    return KATRA_SUCCESS;
}

void katra_module_loader_shutdown(void)
{
    pthread_mutex_lock(&g_loader_mutex);

    if (!g_loader_initialized) {
        pthread_mutex_unlock(&g_loader_mutex);
        return;
    }

    /* Unload all loaded modules */
    for (size_t i = 0; i < g_module_count; i++) {
        if (g_modules[i].state == KATRA_MODULE_STATE_LOADED) {
            /* Call shutdown */
            if (g_modules[i]._shutdown_fn) {
                g_modules[i]._shutdown_fn();
            }
            /* Close handle */
            if (g_modules[i]._handle) {
                dlclose(g_modules[i]._handle);
            }
        }
    }

    /* Clear state */
    memset(g_modules, 0, sizeof(g_modules));
    g_module_count = 0;
    g_loader_initialized = false;

    pthread_mutex_unlock(&g_loader_mutex);
    LOG_INFO("Module loader shutdown complete");
}

/* ============================================================================
 * Module Discovery
 * ============================================================================ */

int katra_module_loader_discover(void)
{
    if (!g_loader_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_module_loader_discover",
                          "Loader not initialized");
        return E_INVALID_STATE;
    }

    pthread_mutex_lock(&g_loader_mutex);

    DIR* dir = opendir(g_module_dir);
    if (!dir) {
        pthread_mutex_unlock(&g_loader_mutex);
        LOG_INFO("Module directory empty or inaccessible: %s", g_module_dir);
        return 0;  /* Not an error - just no modules */
    }

    int discovered = 0;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (!is_module_file(entry->d_name)) {
            continue;
        }

        if (g_module_count >= MAX_MODULES) {
            LOG_WARN("Maximum modules reached (%d), skipping: %s",
                     MAX_MODULES, entry->d_name);
            break;
        }

        /* Build full path */
        char path[KATRA_PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", g_module_dir, entry->d_name);

        /* Check if already discovered */
        int existing = -1;
        for (size_t i = 0; i < g_module_count; i++) {
            if (strcmp(g_modules[i].path, path) == 0) {
                existing = (int)i;
                break;
            }
        }

        katra_module_entry_t* target;
        if (existing >= 0) {
            /* Update existing entry */
            target = &g_modules[existing];
        } else {
            /* New entry */
            target = &g_modules[g_module_count];
        }

        /* Probe module */
        int result = probe_module(path, target);
        if (result == KATRA_SUCCESS) {
            if (existing < 0) {
                g_module_count++;
            }
            discovered++;
            LOG_INFO("Discovered module: %s v%s",
                     target->name, target->version);
        }
    }

    closedir(dir);
    pthread_mutex_unlock(&g_loader_mutex);

    return discovered;
}

int katra_module_loader_list(katra_module_entry_t** entries, size_t* count)
{
    if (!entries || !count) {
        katra_report_error(E_INPUT_NULL, "katra_module_loader_list",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    pthread_mutex_lock(&g_loader_mutex);

    if (g_module_count == 0) {
        *entries = NULL;
        *count = 0;
        pthread_mutex_unlock(&g_loader_mutex);
        return KATRA_SUCCESS;
    }

    /* Allocate copy */
    *entries = calloc(g_module_count, sizeof(katra_module_entry_t));
    if (!*entries) {
        pthread_mutex_unlock(&g_loader_mutex);
        katra_report_error(E_SYSTEM_MEMORY, "katra_module_loader_list",
                          "Failed to allocate entry array");
        return E_SYSTEM_MEMORY;
    }

    memcpy(*entries, g_modules, g_module_count * sizeof(katra_module_entry_t));
    *count = g_module_count;

    pthread_mutex_unlock(&g_loader_mutex);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * Module Loading
 * ============================================================================ */

int katra_module_load(const char* name)
{
    if (!name) {
        katra_report_error(E_INPUT_NULL, "katra_module_load",
                          "name is NULL");
        return E_INPUT_NULL;
    }

    if (!g_loader_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_module_load",
                          "Loader not initialized");
        return E_INVALID_STATE;
    }

    pthread_mutex_lock(&g_loader_mutex);

    /* Find module */
    int idx = find_module_index(name);
    if (idx < 0) {
        pthread_mutex_unlock(&g_loader_mutex);
        katra_report_error(E_NOT_FOUND, "katra_module_load",
                          "Module not found");
        return E_NOT_FOUND;
    }

    katra_module_entry_t* module = &g_modules[idx];

    /* Check if already loaded */
    if (module->state == KATRA_MODULE_STATE_LOADED) {
        pthread_mutex_unlock(&g_loader_mutex);
        return KATRA_SUCCESS;  /* Already loaded is success */
    }

    module->state = KATRA_MODULE_STATE_LOADING;

    /* Open shared library */
    void* handle = dlopen(module->path, RTLD_NOW);
    if (!handle) {
        module->state = KATRA_MODULE_STATE_FAILED;
        pthread_mutex_unlock(&g_loader_mutex);
        katra_report_error(E_SYSTEM_FILE, "katra_module_load",
                          "dlopen failed: %s", dlerror());
        return E_SYSTEM_FILE;
    }

    /* Get required symbols */
    katra_module_info_fn info_fn =
        (katra_module_info_fn)dlsym(handle, KATRA_MODULE_INFO_SYMBOL);
    katra_module_init_fn init_fn =
        (katra_module_init_fn)dlsym(handle, KATRA_MODULE_INIT_SYMBOL);
    katra_module_shutdown_fn shutdown_fn =
        (katra_module_shutdown_fn)dlsym(handle, KATRA_MODULE_SHUTDOWN_SYMBOL);
    katra_module_register_ops_fn register_fn =
        (katra_module_register_ops_fn)dlsym(handle, KATRA_MODULE_REGISTER_SYMBOL);

    if (!info_fn || !init_fn || !shutdown_fn || !register_fn) {
        dlclose(handle);
        module->state = KATRA_MODULE_STATE_FAILED;
        pthread_mutex_unlock(&g_loader_mutex);
        katra_report_error(E_INPUT_INVALID, "katra_module_load",
                          "Module missing required exports");
        return E_INPUT_INVALID;
    }

    /* Check API version */
    katra_module_info_t* info = info_fn();
    if (info->api_version > KATRA_MODULE_API_VERSION) {
        dlclose(handle);
        module->state = KATRA_MODULE_STATE_FAILED;
        pthread_mutex_unlock(&g_loader_mutex);
        katra_report_error(E_INPUT_INVALID, "katra_module_load",
                          "Module requires newer API version %d (have %d)",
                          info->api_version, KATRA_MODULE_API_VERSION);
        return E_INPUT_INVALID;
    }

    /* Build context */
    katra_module_context_t* ctx = build_module_context(name);
    if (!ctx) {
        dlclose(handle);
        module->state = KATRA_MODULE_STATE_FAILED;
        pthread_mutex_unlock(&g_loader_mutex);
        return E_SYSTEM_MEMORY;
    }

    /* Initialize module */
    int result = init_fn(ctx);
    if (result != KATRA_SUCCESS) {
        free_module_context(ctx);
        dlclose(handle);
        module->state = KATRA_MODULE_STATE_FAILED;
        pthread_mutex_unlock(&g_loader_mutex);
        katra_report_error(result, "katra_module_load",
                          "Module init failed");
        return result;
    }

    /* Register operations */
    g_op_registry._module_name = name;
    result = register_fn(&g_op_registry);
    g_op_registry._module_name = NULL;

    if (result != KATRA_SUCCESS) {
        shutdown_fn();
        free_module_context(ctx);
        dlclose(handle);
        module->state = KATRA_MODULE_STATE_FAILED;
        pthread_mutex_unlock(&g_loader_mutex);
        katra_report_error(result, "katra_module_load",
                          "Failed to register operations");
        return result;
    }

    /* Success - update module entry */
    module->_handle = handle;
    module->_shutdown_fn = shutdown_fn;
    module->state = KATRA_MODULE_STATE_LOADED;
    module->loaded_at = time(NULL);

    free_module_context(ctx);
    pthread_mutex_unlock(&g_loader_mutex);

    LOG_INFO("Loaded module: %s v%s", module->name, module->version);
    return KATRA_SUCCESS;
}

int katra_module_unload(const char* name)
{
    if (!name) {
        katra_report_error(E_INPUT_NULL, "katra_module_unload",
                          "name is NULL");
        return E_INPUT_NULL;
    }

    pthread_mutex_lock(&g_loader_mutex);

    int idx = find_module_index(name);
    if (idx < 0) {
        pthread_mutex_unlock(&g_loader_mutex);
        katra_report_error(E_NOT_FOUND, "katra_module_unload",
                          "Module not found");
        return E_NOT_FOUND;
    }

    katra_module_entry_t* module = &g_modules[idx];

    if (module->state != KATRA_MODULE_STATE_LOADED) {
        pthread_mutex_unlock(&g_loader_mutex);
        katra_report_error(E_INVALID_STATE, "katra_module_unload",
                          "Module not loaded");
        return E_INVALID_STATE;
    }

    module->state = KATRA_MODULE_STATE_UNLOADING;

    /* Call shutdown */
    if (module->_shutdown_fn) {
        module->_shutdown_fn();
    }

    /* Unregister all operations belonging to this module */
    pthread_mutex_lock(&g_ops_mutex);
    for (size_t i = 0; i < g_module_op_count; ) {
        if (strcmp(g_module_ops[i].module_name, name) == 0) {
            /* Unregister from unified dispatch */
            katra_unregister_method(g_module_ops[i].name);
            /* Remove from local registry */
            for (size_t j = i; j < g_module_op_count - 1; j++) {
                g_module_ops[j] = g_module_ops[j + 1];
            }
            g_module_op_count--;
            /* Don't increment i - next entry shifted into current position */
        } else {
            i++;
        }
    }
    pthread_mutex_unlock(&g_ops_mutex);

    /* Close library */
    if (module->_handle) {
        dlclose(module->_handle);
        module->_handle = NULL;
    }

    module->_shutdown_fn = NULL;
    module->state = KATRA_MODULE_STATE_AVAILABLE;
    module->loaded_at = 0;

    pthread_mutex_unlock(&g_loader_mutex);

    LOG_INFO("Unloaded module: %s", name);
    return KATRA_SUCCESS;
}

int katra_module_reload(const char* name)
{
    /* Unload if loaded */
    int result = katra_module_unload(name);
    if (result != KATRA_SUCCESS && result != E_INVALID_STATE) {
        return result;
    }

    /* Re-discover to pick up updated file */
    katra_module_loader_discover();

    /* Load */
    return katra_module_load(name);
}

/* ============================================================================
 * Module Queries
 * ============================================================================ */

bool katra_module_is_loaded(const char* name)
{
    if (!name) {
        return false;
    }

    pthread_mutex_lock(&g_loader_mutex);
    int idx = find_module_index(name);
    bool loaded = (idx >= 0 && g_modules[idx].state == KATRA_MODULE_STATE_LOADED);
    pthread_mutex_unlock(&g_loader_mutex);

    return loaded;
}

int katra_module_get_info(const char* name, katra_module_entry_t* entry)
{
    if (!name || !entry) {
        katra_report_error(E_INPUT_NULL, "katra_module_get_info",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    pthread_mutex_lock(&g_loader_mutex);
    int idx = find_module_index(name);
    if (idx < 0) {
        pthread_mutex_unlock(&g_loader_mutex);
        return E_NOT_FOUND;
    }

    memcpy(entry, &g_modules[idx], sizeof(katra_module_entry_t));
    pthread_mutex_unlock(&g_loader_mutex);

    return KATRA_SUCCESS;
}

const char* katra_module_get_directory(void)
{
    return g_module_dir;
}

int katra_module_set_directory(const char* path)
{
    if (!path) {
        katra_report_error(E_INPUT_NULL, "katra_module_set_directory",
                          "path is NULL");
        return E_INPUT_NULL;
    }

    if (g_loader_initialized) {
        katra_report_error(E_INVALID_STATE, "katra_module_set_directory",
                          "Cannot change directory after init");
        return E_INVALID_STATE;
    }

    strncpy(g_module_dir, path, sizeof(g_module_dir) - 1);
    g_module_dir[sizeof(g_module_dir) - 1] = '\0';

    return KATRA_SUCCESS;
}

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

static int ensure_module_directory(void)
{
    struct stat st;
    if (stat(g_module_dir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return KATRA_SUCCESS;
        }
        katra_report_error(E_SYSTEM_FILE, "ensure_module_directory",
                          "Path exists but is not a directory");
        return E_SYSTEM_FILE;
    }

    /* Create directory (and parents) */
    char cmd[KATRA_PATH_MAX + 16];
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", g_module_dir);
    int result = system(cmd);
    if (result != 0) {
        katra_report_error(E_SYSTEM_FILE, "ensure_module_directory",
                          "Failed to create directory");
        return E_SYSTEM_FILE;
    }

    return KATRA_SUCCESS;
}

static bool is_module_file(const char* filename)
{
    if (!filename) {
        return false;
    }

    /* Check prefix */
    if (strncmp(filename, KATRA_MODULE_PREFIX, strlen(KATRA_MODULE_PREFIX)) != 0) {
        return false;
    }

    /* Check extension */
    const char* ext = strrchr(filename, '.');
    if (!ext) {
        return false;
    }

    return strcmp(ext, KATRA_MODULE_EXT) == 0;
}

static int probe_module(const char* path, katra_module_entry_t* entry)
{
    /* Quick load to get info */
    void* handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        return E_SYSTEM_FILE;
    }

    katra_module_info_fn info_fn =
        (katra_module_info_fn)dlsym(handle, KATRA_MODULE_INFO_SYMBOL);

    if (!info_fn) {
        dlclose(handle);
        return E_INPUT_INVALID;
    }

    katra_module_info_t* info = info_fn();
    if (!info || !info->name) {
        dlclose(handle);
        return E_INPUT_INVALID;
    }

    /* Fill entry */
    strncpy(entry->name, info->name, sizeof(entry->name) - 1);
    strncpy(entry->version, info->version ? info->version : "0.0.0",
            sizeof(entry->version) - 1);
    strncpy(entry->description, info->description ? info->description : "",
            sizeof(entry->description) - 1);
    strncpy(entry->author, info->author ? info->author : "",
            sizeof(entry->author) - 1);
    strncpy(entry->path, path, sizeof(entry->path) - 1);
    entry->api_version = info->api_version;

    entry->state = KATRA_MODULE_STATE_AVAILABLE;
    entry->loaded_at = 0;
    entry->_handle = NULL;
    entry->_shutdown_fn = NULL;

    dlclose(handle);
    return KATRA_SUCCESS;
}

static int find_module_index(const char* name)
{
    for (size_t i = 0; i < g_module_count; i++) {
        if (strcmp(g_modules[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static katra_module_context_t* build_module_context(const char* name)
{
    katra_module_context_t* ctx = calloc(1, sizeof(katra_module_context_t));
    if (!ctx) {
        return NULL;
    }

    ctx->katra_version = KATRA_VERSION;
    ctx->api_version = KATRA_MODULE_API_VERSION;

    /* Build paths */
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";

    static char katra_dir[KATRA_PATH_MAX];
    static char module_data_dir[KATRA_PATH_MAX];

    snprintf(katra_dir, sizeof(katra_dir), "%s/.katra", home);
    snprintf(module_data_dir, sizeof(module_data_dir), "%s/.katra/%s", home, name);

    ctx->katra_dir = katra_dir;
    ctx->module_dir = g_module_dir;
    ctx->module_data_dir = module_data_dir;

    /* Create module data directory */
    char cmd[KATRA_PATH_MAX + 16];
    snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", module_data_dir);
    system(cmd);

    /* Service APIs - modules can access core Katra functionality through these.
     * Currently NULL as modules use direct function calls via linked library.
     * Future: Provide vtable APIs for true isolation/versioning. */
    ctx->memory = NULL;  /* Would provide: store, recall, recent, etc. */
    ctx->log = NULL;     /* Would provide: debug, info, warn, error */
    ctx->db = NULL;      /* Would provide: exec, query, prepare */

    return ctx;
}

static void free_module_context(katra_module_context_t* ctx)
{
    if (ctx) {
        free(ctx);
    }
}

/* ============================================================================
 * Operation Registry Implementation
 * ============================================================================ */

static int registry_register_op(const char* name, const char* description,
                                katra_op_handler_t handler, json_t* schema)
{
    (void)description;
    (void)schema;

    if (!name || !handler) {
        return E_INPUT_NULL;
    }

    pthread_mutex_lock(&g_ops_mutex);

    /* Check for duplicates */
    for (size_t i = 0; i < g_module_op_count; i++) {
        if (strcmp(g_module_ops[i].name, name) == 0) {
            pthread_mutex_unlock(&g_ops_mutex);
            LOG_WARN("Operation already registered: %s", name);
            return E_DUPLICATE;
        }
    }

    /* Check capacity */
    if (g_module_op_count >= MAX_MODULE_OPS) {
        pthread_mutex_unlock(&g_ops_mutex);
        katra_report_error(E_RESOURCE_LIMIT, "registry_register_op",
                          "Maximum module operations reached");
        return E_RESOURCE_LIMIT;
    }

    /* Store the operation */
    module_op_entry_t* entry = &g_module_ops[g_module_op_count];
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->handler = handler;
    if (g_op_registry._module_name) {
        strncpy(entry->module_name, g_op_registry._module_name,
                sizeof(entry->module_name) - 1);
        entry->module_name[sizeof(entry->module_name) - 1] = '\0';
    } else {
        entry->module_name[0] = '\0';
    }
    g_module_op_count++;

    pthread_mutex_unlock(&g_ops_mutex);

    /* Register with unified dispatcher using the adapter */
    int result = katra_register_method(name, module_op_adapter);
    if (result != KATRA_SUCCESS) {
        LOG_ERROR("Failed to register %s with unified dispatcher", name);
        return result;
    }

    LOG_INFO("Registered operation: %s (module: %s)",
             name, g_op_registry._module_name ? g_op_registry._module_name : "?");

    return KATRA_SUCCESS;
}

static int registry_unregister_op(const char* name)
{
    if (!name) {
        return E_INPUT_NULL;
    }

    /* Unregister from unified dispatch first */
    int result = katra_unregister_method(name);
    if (result != KATRA_SUCCESS && result != E_NOT_FOUND) {
        LOG_WARN("Failed to unregister %s from unified dispatch", name);
    }

    /* Remove from module operations registry */
    pthread_mutex_lock(&g_ops_mutex);

    for (size_t i = 0; i < g_module_op_count; i++) {
        if (strcmp(g_module_ops[i].name, name) == 0) {
            /* Shift remaining entries down */
            for (size_t j = i; j < g_module_op_count - 1; j++) {
                g_module_ops[j] = g_module_ops[j + 1];
            }
            g_module_op_count--;
            pthread_mutex_unlock(&g_ops_mutex);
            LOG_INFO("Unregistered operation: %s", name);
            return KATRA_SUCCESS;
        }
    }

    pthread_mutex_unlock(&g_ops_mutex);
    return E_NOT_FOUND;
}

/* Find a module operation by name */
static module_op_entry_t* find_module_op(const char* name)
{
    if (!name) return NULL;

    pthread_mutex_lock(&g_ops_mutex);
    for (size_t i = 0; i < g_module_op_count; i++) {
        if (strcmp(g_module_ops[i].name, name) == 0) {
            module_op_entry_t* entry = &g_module_ops[i];
            pthread_mutex_unlock(&g_ops_mutex);
            return entry;
        }
    }
    pthread_mutex_unlock(&g_ops_mutex);
    return NULL;
}

/* Adapter that bridges unified dispatcher to module handler */
static json_t* module_op_adapter(json_t* params, const katra_unified_options_t* options)
{
    (void)options;

    /* Get the method name from thread-local context */
    const char* method_name = katra_get_current_method();
    if (!method_name || method_name[0] == '\0') {
        return json_pack("{s:s}", "error", "No method name in dispatch context");
    }

    /* Find the module operation */
    module_op_entry_t* op = find_module_op(method_name);
    if (!op || !op->handler) {
        return json_pack("{s:s,s:s}", "error", "Module operation not found",
                        "method", method_name);
    }

    /* Extract ci_name from params */
    const char* ci_name = NULL;
    if (params) {
        ci_name = json_string_value(json_object_get(params, "ci_name"));
    }
    if (!ci_name) {
        ci_name = "anonymous";
    }

    /* Call the module's handler with the module signature */
    return op->handler(params, ci_name);
}

/* ============================================================================
 * MCP Operations
 * ============================================================================ */

json_t* katra_mcp_modules_list(json_t* params, const char* ci_name)
{
    (void)params;
    (void)ci_name;

    json_t* modules = json_array();

    for (size_t i = 0; i < g_module_count; i++) {
        json_t* mod = json_pack("{s:s,s:s,s:s,s:b}",
            "name", g_modules[i].name,
            "version", g_modules[i].version,
            "description", g_modules[i].description,
            "loaded", g_modules[i].state == KATRA_MODULE_STATE_LOADED
        );
        json_array_append_new(modules, mod);
    }

    return json_pack("{s:o}", "modules", modules);
}

json_t* katra_mcp_modules_load(json_t* params, const char* ci_name)
{
    (void)ci_name;

    /* Accept either "name" or "module" parameter */
    const char* name = json_string_value(json_object_get(params, "name"));
    if (!name) {
        name = json_string_value(json_object_get(params, "module"));
    }
    if (!name) {
        return json_pack("{s:s}", "error", "module name required");
    }

    int result = katra_module_load(name);
    if (result != KATRA_SUCCESS) {
        return json_pack("{s:s,s:i}", "error", "load failed", "code", result);
    }

    return json_pack("{s:s,s:s}", "status", "loaded", "module", name);
}

json_t* katra_mcp_modules_unload(json_t* params, const char* ci_name)
{
    (void)ci_name;

    /* Accept either "name" or "module" parameter */
    const char* name = json_string_value(json_object_get(params, "name"));
    if (!name) {
        name = json_string_value(json_object_get(params, "module"));
    }
    if (!name) {
        return json_pack("{s:s}", "error", "module name required");
    }

    int result = katra_module_unload(name);
    if (result != KATRA_SUCCESS) {
        return json_pack("{s:s,s:i}", "error", "unload failed", "code", result);
    }

    return json_pack("{s:s,s:s}", "status", "unloaded", "module", name);
}

json_t* katra_mcp_modules_reload(json_t* params, const char* ci_name)
{
    (void)ci_name;

    /* Accept either "name" or "module" parameter */
    const char* name = json_string_value(json_object_get(params, "name"));
    if (!name) {
        name = json_string_value(json_object_get(params, "module"));
    }
    if (!name) {
        return json_pack("{s:s}", "error", "module name required");
    }

    int result = katra_module_reload(name);
    if (result != KATRA_SUCCESS) {
        return json_pack("{s:s,s:i}", "error", "reload failed", "code", result);
    }

    return json_pack("{s:s,s:s}", "status", "reloaded", "module", name);
}

json_t* katra_mcp_modules_info(json_t* params, const char* ci_name)
{
    (void)ci_name;

    /* Accept either "name" or "module" parameter */
    const char* name = json_string_value(json_object_get(params, "name"));
    if (!name) {
        name = json_string_value(json_object_get(params, "module"));
    }
    if (!name) {
        return json_pack("{s:s}", "error", "module name required");
    }

    katra_module_entry_t entry;
    int result = katra_module_get_info(name, &entry);
    if (result != KATRA_SUCCESS) {
        return json_pack("{s:s,s:i}", "error", "module not found", "code", result);
    }

    return json_pack("{s:s,s:s,s:s,s:s,s:i,s:b,s:i}",
        "name", entry.name,
        "version", entry.version,
        "description", entry.description,
        "author", entry.author,
        "api_version", entry.api_version,
        "loaded", entry.state == KATRA_MODULE_STATE_LOADED,
        "loaded_at", (int)entry.loaded_at
    );
}
