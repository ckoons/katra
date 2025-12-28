# Katra Dynamic Module Loading

© 2025 Casey Koons. All rights reserved.

## Overview

Katra uses a dynamic module loading architecture where capability modules are separate shared libraries loaded on demand. This keeps the core daemon lean while allowing extensibility.

## Goals

1. **Lean Core**: Daemon contains only essential functionality
2. **On-Demand Loading**: Modules loaded when needed, not at startup
3. **Hot Reload**: Update modules without daemon restart
4. **Discovery**: List available modules without loading them
5. **CI Control**: CIs can request module loading via MCP

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Katra Daemon (lean)                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   Memory    │  │     MCP     │  │   Module Loader     │  │
│  │   System    │  │   Dispatch  │  │   (dlopen/dlsym)    │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
              ┌───────────────┼───────────────┐
              ▼               ▼               ▼
     ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
     │   softdev   │  │  analytics  │  │   future    │
     │   .dylib    │  │   .dylib    │  │   .dylib    │
     └─────────────┘  └─────────────┘  └─────────────┘
           ~/.katra/modules/
```

## Module Directory

Default location: `~/.katra/modules/`

Override via:
- Environment: `KATRA_MODULE_PATH`
- Config: `~/.katra/config.json` → `"module_path": "/path/to/modules"`

```
~/.katra/modules/
├── katra_softdev.dylib      # macOS
├── katra_softdev.so         # Linux
├── katra_analytics.dylib
└── ...
```

Naming convention: `katra_<name>.dylib` or `katra_<name>.so`

## Module Contract

Every module must export these symbols:

### Required Exports

```c
/* Module information - called to probe without full load */
katra_module_info_t* katra_module_info(void);

/* Initialize module - called when loading */
int katra_module_init(katra_module_context_t* ctx);

/* Shutdown module - called when unloading */
void katra_module_shutdown(void);

/* Register operations with MCP dispatch */
int katra_module_register_ops(katra_op_registry_t* registry);
```

### Module Info Structure

```c
#define KATRA_MODULE_API_VERSION 1

typedef struct {
    /* Identification */
    const char* name;              /* "softdev" */
    const char* version;           /* "0.1.0" */
    const char* description;       /* "Software development metamemory" */
    const char* author;            /* "Casey Koons" */

    /* Compatibility */
    int api_version;               /* KATRA_MODULE_API_VERSION */
    const char* min_katra_version; /* "0.1.0" */

    /* Dependencies (optional) */
    const char** requires;         /* ["other_module>=1.0"] */
    size_t requires_count;

    /* Capabilities (optional) */
    const char** provides;         /* ["metamemory", "code_analysis"] */
    size_t provides_count;
} katra_module_info_t;
```

### Module Context

Passed to `katra_module_init()`:

```c
typedef struct {
    /* Katra version info */
    const char* katra_version;
    int api_version;

    /* Paths */
    const char* data_dir;          /* ~/.katra */
    const char* module_data_dir;   /* ~/.katra/<module_name> */

    /* Services */
    katra_memory_api_t* memory;    /* Access to memory system */
    katra_log_api_t* log;          /* Logging */
    katra_db_api_t* db;            /* Database access */
} katra_module_context_t;
```

### Operation Registry

For registering MCP operations:

```c
typedef struct {
    int (*register_op)(
        const char* name,           /* "softdev_analyze_project" */
        const char* description,
        katra_op_handler_t handler,
        json_t* schema              /* JSON schema for params */
    );
} katra_op_registry_t;

/* Handler signature */
typedef json_t* (*katra_op_handler_t)(json_t* params, const char* ci_name);
```

## Loading Flow

### 1. Discovery (without loading)

```c
int katra_modules_discover(void) {
    const char* module_dir = get_module_directory();
    DIR* dir = opendir(module_dir);

    while ((entry = readdir(dir))) {
        if (!is_module_file(entry->d_name)) continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", module_dir, entry->d_name);

        /* Quick probe: load, get info, unload */
        void* handle = dlopen(path, RTLD_LAZY);
        if (!handle) continue;

        katra_module_info_fn info_fn = dlsym(handle, "katra_module_info");
        if (info_fn) {
            katra_module_info_t* info = info_fn();
            register_available_module(path, info);
        }

        dlclose(handle);
    }
}
```

### 2. Loading

```c
int katra_module_load(const char* name) {
    /* Check if already loaded */
    if (is_module_loaded(name)) {
        return KATRA_SUCCESS;
    }

    /* Find module path */
    const char* path = find_module_path(name);
    if (!path) {
        return E_NOT_FOUND;
    }

    /* Open shared library */
    void* handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        LOG_ERROR("dlopen failed: %s", dlerror());
        return E_SYSTEM_FILE;
    }

    /* Get entry points */
    katra_module_info_fn info_fn = dlsym(handle, "katra_module_info");
    katra_module_init_fn init_fn = dlsym(handle, "katra_module_init");
    katra_module_shutdown_fn shutdown_fn = dlsym(handle, "katra_module_shutdown");
    katra_module_register_fn register_fn = dlsym(handle, "katra_module_register_ops");

    if (!info_fn || !init_fn || !shutdown_fn || !register_fn) {
        LOG_ERROR("Module missing required exports");
        dlclose(handle);
        return E_INPUT_INVALID;
    }

    /* Check API compatibility */
    katra_module_info_t* info = info_fn();
    if (info->api_version > KATRA_MODULE_API_VERSION) {
        LOG_ERROR("Module requires newer API version");
        dlclose(handle);
        return E_INPUT_INVALID;
    }

    /* Initialize */
    katra_module_context_t ctx = build_module_context(name);
    int result = init_fn(&ctx);
    if (result != KATRA_SUCCESS) {
        dlclose(handle);
        return result;
    }

    /* Register operations */
    result = register_fn(&g_op_registry);
    if (result != KATRA_SUCCESS) {
        shutdown_fn();
        dlclose(handle);
        return result;
    }

    /* Track loaded module */
    add_loaded_module(name, handle, info, shutdown_fn);

    return KATRA_SUCCESS;
}
```

### 3. Unloading

```c
int katra_module_unload(const char* name) {
    loaded_module_t* module = find_loaded_module(name);
    if (!module) {
        return E_NOT_FOUND;
    }

    /* Call shutdown */
    module->shutdown_fn();

    /* Unregister operations */
    unregister_module_ops(name);

    /* Close library */
    dlclose(module->handle);

    /* Remove from loaded list */
    remove_loaded_module(name);

    return KATRA_SUCCESS;
}
```

### 4. Hot Reload

```c
int katra_module_reload(const char* name) {
    /* Unload current version */
    int result = katra_module_unload(name);
    if (result != KATRA_SUCCESS && result != E_NOT_FOUND) {
        return result;
    }

    /* Re-discover to get updated file */
    katra_modules_discover();

    /* Load new version */
    return katra_module_load(name);
}
```

## CLI Interface

### Commands

```bash
# List available modules
katra modules list
# Output:
# Available modules:
#   NAME        VERSION  DESCRIPTION                        STATUS
#   softdev     0.1.0    Software development metamemory    [loaded]
#   analytics   0.2.0    Usage analytics                    [available]

# Load a module
katra modules load softdev

# Load multiple modules
katra modules load softdev analytics

# Show loaded modules
katra modules loaded

# Unload a module
katra modules unload softdev

# Reload (for updates)
katra modules reload softdev

# Module info
katra modules info softdev
# Output:
# Module: softdev
# Version: 0.1.0
# Description: Software development metamemory
# Author: Casey Koons
# API Version: 1
# Status: loaded
# Operations: softdev_analyze_project, softdev_find_concept, ...
```

### Startup Options

```bash
# Start with specific modules
katra start --persona Ami --modules softdev,analytics

# Start with no modules (default)
katra start --persona Ami

# Remember module configuration
katra start --persona Ami --modules softdev --save-module-config

# Next time, auto-loads remembered modules
katra start --persona Ami
```

## MCP Interface

### Operations

```json
// List available modules
{
    "method": "modules_list",
    "params": {"ci_name": "Ami"}
}
// Returns: {"modules": [{"name": "softdev", "version": "0.1.0", "loaded": true}, ...]}

// Load module
{
    "method": "modules_load",
    "params": {"module": "softdev", "ci_name": "Ami"}
}

// Load multiple
{
    "method": "modules_load",
    "params": {"modules": ["softdev", "analytics"], "ci_name": "Ami"}
}

// Unload module
{
    "method": "modules_unload",
    "params": {"module": "softdev", "ci_name": "Ami"}
}

// Reload module
{
    "method": "modules_reload",
    "params": {"module": "softdev", "ci_name": "Ami"}
}

// Get module info
{
    "method": "modules_info",
    "params": {"module": "softdev", "ci_name": "Ami"}
}
```

## Building Modules

### Module Makefile Pattern

```makefile
# Module: softdev
MODULE_NAME := softdev
MODULE_VERSION := 0.1.0

# Sources
SOFTDEV_SRCS := $(wildcard src/modules/softdev/*.c)
SOFTDEV_OBJS := $(SOFTDEV_SRCS:.c=.o)

# Shared library output
ifeq ($(UNAME_S),Darwin)
    MODULE_EXT := .dylib
    MODULE_FLAGS := -dynamiclib -install_name @rpath/katra_$(MODULE_NAME)$(MODULE_EXT)
else
    MODULE_EXT := .so
    MODULE_FLAGS := -shared
endif

MODULE_LIB := $(LIB_DIR)/katra_$(MODULE_NAME)$(MODULE_EXT)

# Build module
$(MODULE_LIB): $(SOFTDEV_OBJS)
	$(CC) $(MODULE_FLAGS) -fPIC -o $@ $^ $(LDFLAGS) -lsqlite3

# Compile with position-independent code
src/modules/softdev/%.o: src/modules/softdev/%.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

# Install
install-softdev: $(MODULE_LIB)
	mkdir -p ~/.katra/modules
	cp $(MODULE_LIB) ~/.katra/modules/
```

### Module Source Template

```c
/* katra_<name>.c - Module entry points */

#include "katra_module.h"
#include "katra_<name>.h"

/* Module information */
static katra_module_info_t g_module_info = {
    .name = "<name>",
    .version = "0.1.0",
    .description = "Description here",
    .author = "Casey Koons",
    .api_version = KATRA_MODULE_API_VERSION,
    .min_katra_version = "0.1.0",
    .requires = NULL,
    .requires_count = 0,
    .provides = NULL,
    .provides_count = 0
};

/* Required: Module info */
katra_module_info_t* katra_module_info(void) {
    return &g_module_info;
}

/* Required: Initialize */
int katra_module_init(katra_module_context_t* ctx) {
    /* Store context for later use */
    /* Initialize module state */
    /* Open databases, etc. */
    return KATRA_SUCCESS;
}

/* Required: Shutdown */
void katra_module_shutdown(void) {
    /* Close databases */
    /* Free resources */
}

/* Required: Register operations */
int katra_module_register_ops(katra_op_registry_t* registry) {
    registry->register_op(
        "<name>_operation",
        "Description",
        handle_operation,
        build_schema()
    );
    return KATRA_SUCCESS;
}
```

## Configuration

### Module Configuration File

`~/.katra/modules.json`:

```json
{
    "module_path": "~/.katra/modules",
    "auto_load": ["softdev"],
    "disabled": [],
    "module_config": {
        "softdev": {
            "default_project": "/path/to/project"
        }
    }
}
```

### Per-Module Configuration

Modules can read their config from the context:
- `ctx->module_data_dir` points to `~/.katra/<module_name>/`
- Module can store `config.json` there

## Error Handling

| Error | Cause | Recovery |
|-------|-------|----------|
| `E_NOT_FOUND` | Module not in module directory | Check path, install module |
| `E_SYSTEM_FILE` | dlopen failed | Check library dependencies |
| `E_INPUT_INVALID` | Missing exports or bad API version | Update module or Katra |
| `E_ALREADY_EXISTS` | Module already loaded | Use reload if updating |

## Security Considerations

1. **Module Trust**: Only load modules from trusted locations
2. **Path Validation**: Prevent path traversal in module names
3. **Sandboxing**: Future consideration for untrusted modules
4. **Signature Verification**: Future consideration for signed modules

## Future Enhancements

1. **Module Dependencies**: Automatic loading of required modules
2. **Module Marketplace**: Discover and install modules
3. **Sandboxing**: Run untrusted modules in isolation
4. **Signed Modules**: Verify module authenticity
5. **Module Versioning**: Multiple versions side-by-side
