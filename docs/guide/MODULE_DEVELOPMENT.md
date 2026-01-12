# Katra Module Development Guide

© 2025 Casey Koons. All rights reserved.

## Overview

Katra supports **dynamic loadable modules** - shared libraries that can be loaded, unloaded, and reloaded at runtime without restarting the daemon. This guide covers everything you need to create, build, and deploy custom modules.

## Quick Start

### 1. Create Your Module

```c
/* src/modules/mymodule/katra_mymodule.c */
/* © 2025 Casey Koons All rights reserved */

#include <stdio.h>
#include <string.h>
#include <jansson.h>
#include "katra_error.h"
#include "katra_log.h"
#include "katra_module.h"

/* Module metadata - REQUIRED */
static katra_module_info_t g_module_info = {
    .name = "mymodule",
    .version = "1.0.0",
    .description = "My custom Katra module",
    .author = "Your Name"
};

/* Module state */
static bool g_initialized = false;

/* ============================================================================
 * REQUIRED EXPORTS - These functions MUST be exported
 * ============================================================================ */

/* Return module metadata */
katra_module_info_t* katra_module_get_info(void)
{
    return &g_module_info;
}

/* Initialize module - called when module is loaded */
int katra_module_init(void)
{
    if (g_initialized) {
        return KATRA_SUCCESS;
    }

    LOG_INFO("Initializing module: %s v%s",
             g_module_info.name, g_module_info.version);

    /* Your initialization code here */

    g_initialized = true;
    return KATRA_SUCCESS;
}

/* Shutdown module - called when module is unloaded */
void katra_module_shutdown(void)
{
    if (!g_initialized) {
        return;
    }

    LOG_INFO("Shutting down module: %s", g_module_info.name);

    /* Your cleanup code here */

    g_initialized = false;
}

/* Register MCP operations - called after init to expose API */
int katra_module_register_ops(void)
{
    /* Register your operations with the dispatch system */
    /* This is optional - return success if no operations to register */
    return KATRA_SUCCESS;
}
```

### 2. Add to Build System

**Makefile.config:**
```makefile
# MyModule object files (compiled with -fPIC for shared library)
MYMODULE_MODULE := $(MODULE_DIR)/katra_mymodule$(MODULE_EXT)
MYMODULE_MODULE_OBJS := $(BUILD_DIR)/katra_mymodule_pic.o
```

**Makefile.build:**
```makefile
# Compile mymodule with -fPIC
$(BUILD_DIR)/katra_mymodule_pic.o: $(SRC_DIR)/modules/mymodule/katra_mymodule.c | $(BUILD_DIR)
	@echo "Compiling (PIC): $<"
	@$(CC) $(CFLAGS_DEBUG) $(JANSSON_CFLAGS) -fPIC -c $< -o $@

# Build mymodule shared library
$(MYMODULE_MODULE): $(MYMODULE_MODULE_OBJS) $(LIBKATRA_UTILS) | $(MODULE_DIR)
	@echo "Building module: $@"
	@$(CC) $(MODULE_LDFLAGS) -o $@ $(MYMODULE_MODULE_OBJS) \
		-L$(BUILD_DIR) -lkatra_utils $(JANSSON_LDFLAGS) -lsqlite3
	@echo "Module built: katra_mymodule$(MODULE_EXT)"
```

### 3. Build and Install

```bash
# Build the module
make modules

# Install to ~/.katra/modules/
make install-modules

# Or manually copy
cp bin/modules/katra_mymodule.dylib ~/.katra/modules/
```

### 4. Test Your Module

```bash
# Start daemon
./bin/katra_unified_daemon

# List modules (should show your module)
curl -X POST http://localhost:9742/operation \
  -d '{"method":"modules_list"}'

# Load your module
curl -X POST http://localhost:9742/operation \
  -d '{"method":"modules_load","params":{"name":"mymodule"}}'

# Check module info
curl -X POST http://localhost:9742/operation \
  -d '{"method":"modules_info","params":{"name":"mymodule"}}'
```

## Module Interface

### Required Exports

Every module MUST export these functions:

| Function | Signature | Purpose |
|----------|-----------|---------|
| `katra_module_get_info` | `katra_module_info_t* (void)` | Return module metadata |
| `katra_module_init` | `int (void)` | Initialize module resources |
| `katra_module_shutdown` | `void (void)` | Clean up module resources |

### Optional Exports

| Function | Signature | Purpose |
|----------|-----------|---------|
| `katra_module_register_ops` | `int (void)` | Register MCP operations |

### Module Info Structure

```c
typedef struct {
    const char* name;        /* Module name (used for loading) */
    const char* version;     /* Semantic version string */
    const char* description; /* Human-readable description */
    const char* author;      /* Module author */
} katra_module_info_t;
```

## Module Management API

### List Available Modules

```bash
curl -X POST http://localhost:9742/operation \
  -H 'Content-Type: application/json' \
  -d '{"method":"modules_list"}'
```

Response:
```json
{
  "result": {
    "modules": [
      {
        "name": "softdev",
        "version": "0.1.0",
        "description": "Software development metamemory",
        "loaded": true
      },
      {
        "name": "mymodule",
        "version": "1.0.0",
        "description": "My custom module",
        "loaded": false
      }
    ]
  }
}
```

### Load a Module

```bash
curl -X POST http://localhost:9742/operation \
  -d '{"method":"modules_load","params":{"name":"mymodule"}}'
```

Response:
```json
{
  "result": {
    "status": "loaded",
    "module": "mymodule"
  }
}
```

### Unload a Module

```bash
curl -X POST http://localhost:9742/operation \
  -d '{"method":"modules_unload","params":{"name":"mymodule"}}'
```

### Reload a Module

Useful during development - unloads and reloads the module:

```bash
curl -X POST http://localhost:9742/operation \
  -d '{"method":"modules_reload","params":{"name":"mymodule"}}'
```

### Get Module Info

```bash
curl -X POST http://localhost:9742/operation \
  -d '{"method":"modules_info","params":{"name":"mymodule"}}'
```

Response:
```json
{
  "result": {
    "name": "mymodule",
    "version": "1.0.0",
    "description": "My custom module",
    "loaded": true,
    "loaded_at": 1767115881
  }
}
```

## Module Discovery

The daemon automatically discovers modules at startup from:

1. `~/.katra/modules/` (user modules)
2. `$KATRA_MODULE_DIR` (if set)

Modules must have the correct extension:
- macOS: `.dylib`
- Linux: `.so`

## Development Workflow

### Iterative Development

```bash
# 1. Make code changes
vim src/modules/mymodule/katra_mymodule.c

# 2. Rebuild module
make modules

# 3. Reinstall
make install-modules

# 4. Reload in running daemon
curl -X POST http://localhost:9742/operation \
  -d '{"method":"modules_reload","params":{"name":"mymodule"}}'
```

### Testing with Test Script

Create a test script similar to `tests/test_module_workflow.sh`:

```bash
#!/bin/bash
# Test your module

# Start daemon
./bin/katra_unified_daemon --port 9999 &
DAEMON_PID=$!
sleep 2

# Load module
result=$(curl -s -X POST http://localhost:9999/operation \
  -d '{"method":"modules_load","params":{"name":"mymodule"}}')

echo "Load result: $result"

# Test your operations...

# Cleanup
kill $DAEMON_PID
```

## Best Practices

### 1. Memory Safety

Follow Katra's goto cleanup pattern:

```c
int my_operation(const char* param)
{
    int result = KATRA_SUCCESS;
    char* buffer = NULL;
    sqlite3* db = NULL;

    /* Validate input */
    if (!param) {
        katra_report_error(E_INPUT_NULL, "my_operation", "param is NULL");
        return E_INPUT_NULL;
    }

    /* Allocate resources */
    buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    /* Work... */

cleanup:
    free(buffer);
    if (db) sqlite3_close(db);
    return result;
}
```

### 2. Error Reporting

Always use `katra_report_error()`:

```c
if (error_condition) {
    katra_report_error(E_MODULE_ERROR, "function_name",
                      "Description of what went wrong");
    return E_MODULE_ERROR;
}
```

### 3. Logging

Use Katra's logging macros:

```c
LOG_DEBUG("Detailed debug info: %s", detail);
LOG_INFO("Module initialized successfully");
LOG_WARN("Non-fatal issue: %s", issue);
LOG_ERROR("Error occurred: %d", error_code);
```

### 4. Thread Safety

If your module is accessed from multiple threads:

```c
#include <pthread.h>

static pthread_mutex_t g_module_mutex = PTHREAD_MUTEX_INITIALIZER;

int my_operation(void)
{
    pthread_mutex_lock(&g_module_mutex);

    /* Critical section */

    pthread_mutex_unlock(&g_module_mutex);
    return KATRA_SUCCESS;
}
```

### 5. Data Storage

Store module data in a dedicated directory:

```c
/* Get module data directory */
char path[512];
snprintf(path, sizeof(path), "%s/.katra/mymodule/data.db",
         getenv("HOME"));
```

Recommended structure:
```
~/.katra/mymodule/
├── config.json      # Module configuration
├── data.db          # SQLite database
└── cache/           # Cached data
```

## Example: Complete Module

See `src/modules/softdev/` for a complete working example:

- `katra_softdev.c` - Module lifecycle and MCP operations
- `katra_metamemory.c` - Domain-specific functionality

Key patterns demonstrated:
- Module initialization and shutdown
- SQLite database integration
- MCP operation implementation
- Error handling and logging

## Troubleshooting

### Module Not Found

```
Error: Module 'mymodule' not found
```

Causes:
1. Module not installed to `~/.katra/modules/`
2. Wrong file extension (.dylib vs .so)
3. Module file permissions

Fix:
```bash
ls -la ~/.katra/modules/
make install-modules
```

### Module Load Failed

```
Error: Failed to load module: dlopen error
```

Causes:
1. Missing dependencies (libkatra_utils, jansson, etc.)
2. Symbol not found
3. Architecture mismatch

Fix:
```bash
# Check dependencies
otool -L ~/.katra/modules/katra_mymodule.dylib

# Verify exports
nm ~/.katra/modules/katra_mymodule.dylib | grep katra_module
```

### Module Init Failed

```
Error: Module init returned error
```

Check your `katra_module_init()` function:
- Returns KATRA_SUCCESS on success
- Handles already-initialized state
- Reports errors via katra_report_error()

## Platform Notes

### macOS

- Extension: `.dylib`
- Linker flags: `-dynamiclib`
- Check deps: `otool -L module.dylib`
- View symbols: `nm module.dylib`

### Linux

- Extension: `.so`
- Linker flags: `-shared`
- Check deps: `ldd module.so`
- View symbols: `nm module.so`

## See Also

- [KATRA_MODULES.md](KATRA_MODULES.md) - Module architecture overview
- [SOFTDEV_MODULE.md](SOFTDEV_MODULE.md) - Softdev module documentation
- `include/katra_module.h` - Module API header
- `src/core/katra_module_loader.c` - Module loader implementation
