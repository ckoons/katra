# Katra Module Architecture

© 2025 Casey Koons. All rights reserved.

## Overview

Katra supports **loadable capability modules** that extend its core functionality. Modules integrate seamlessly with Katra's memory system while providing specialized features for different domains.

The module architecture follows these principles:
- **Memory = Identity**: Module knowledge persists as specialized memory
- **Lean Integration**: Modules share core infrastructure (error handling, SQLite patterns)
- **Unified Interface**: CIs interact through one consistent API
- **Domain-Specific**: Each module focuses on one capability area

## Module vs. Memory

| Aspect | Regular Memory | Module Memory (Metamemory) |
|--------|----------------|---------------------------|
| Nature | Experiential, permanent | Indexed, mutable |
| Content | "I decided to use goto cleanup" | "load_catalog() is at line 45" |
| Lifespan | Persists indefinitely | Updates when source changes |
| Purpose | Identity continuity | Current-state understanding |

## Module Structure

Modules live in `src/modules/<name>/` and follow a consistent structure:

```
src/modules/<name>/
├── katra_<name>.c           # Module init, lifecycle, MCP operations
├── katra_<name>_types.c     # Domain-specific types (if needed)
├── katra_<name>_index.c     # SQLite persistence
└── katra_<name>_*.c         # Additional implementation files

include/
├── katra_<name>.h           # Public API
└── katra_<name>_types.h     # Public types (if separate)
```

## Module Lifecycle

### Initialization

```c
int <name>_init(void);
```

Called during Katra startup or on first use. Responsibilities:
- Initialize SQLite database for module data
- Register MCP operations with unified dispatch
- Set module state to initialized

### Shutdown

```c
void <name>_shutdown(void);
```

Called during Katra shutdown. Responsibilities:
- Close database connections
- Free allocated resources
- Reset module state

### Status Check

```c
bool <name>_is_initialized(void);
```

Returns whether the module is ready for use.

## MCP Operation Registration

Modules register their operations with the unified dispatch system:

```c
static int <name>_register_operations(void)
{
    /* Register operations:
     *   - <name>_operation_one
     *   - <name>_operation_two
     *   - etc.
     */
    return KATRA_SUCCESS;
}
```

Operations become available as MCP tools:
- `<name>_operation_one` → callable from CI via MCP
- Results returned in standard Katra JSON-RPC format

## Build Integration

### Makefile.config

Add module objects:

```makefile
# <Name> module object files
<NAME>_OBJS := $(BUILD_DIR)/katra_<name>.o \
               $(BUILD_DIR)/katra_<name>_types.o \
               $(BUILD_DIR)/katra_<name>_index.o
```

Add to ALL_OBJS:

```makefile
ALL_OBJS := ... $(<NAME>_OBJS) ...
```

### Makefile.build

Add pattern rule:

```makefile
# Compile <name> module sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/modules/<name>/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS_DEBUG) -c $< -o $@
```

Add to library:

```makefile
$(LIBKATRA_UTILS): ... $(<NAME>_OBJS)
```

## Data Storage

Modules store data separately from core Katra memories:

```
~/.katra/
├── memory/              # Core Katra memories
│   └── tier1/
├── <name>/              # Module-specific data
│   ├── <project_id>/    # Per-project (if applicable)
│   │   └── <name>.db    # SQLite database
│   └── config.json      # Module configuration
```

## Error Handling

Modules use Katra's centralized error reporting:

```c
if (!param) {
    katra_report_error(E_INPUT_NULL, "function_name",
                      "param is NULL");
    return E_INPUT_NULL;
}
```

Key patterns:
- Always call `katra_report_error()` before returning error
- Use standard error codes from `katra_error.h`
- Provide context (function name) and details

## Memory Safety

Modules follow Katra's memory safety patterns:

```c
int module_operation(void)
{
    int result = KATRA_SUCCESS;
    char* buffer = NULL;

    buffer = malloc(SIZE);
    if (!buffer) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    /* ... work ... */

cleanup:
    free(buffer);
    return result;
}
```

## Current Modules

### softdev (Software Development)

The first Katra module. Provides metamemory for understanding codebases:

- **Purpose**: Help CIs understand, navigate, and safely modify code
- **Key Feature**: Three-layer architecture (concept → component → code)
- **Operations**: analyze_project, find_concept, find_code, impact

See [SOFTDEV_MODULE.md](SOFTDEV_MODULE.md) for detailed documentation.

## Creating a New Module

1. Create directory structure:
   ```bash
   mkdir -p src/modules/<name>
   ```

2. Create header in `include/katra_<name>.h`:
   - Module constants
   - Public types
   - Function declarations

3. Create implementation files:
   - `katra_<name>.c` - lifecycle and operations
   - Additional files as needed

4. Update build system:
   - Add to Makefile.config
   - Add pattern rule to Makefile.build

5. Write tests in `tests/test_<name>*.c`

6. Document in `docs/guide/<NAME>_MODULE.md`

## Design Guidelines

### Do

- Follow existing Katra patterns (error handling, memory safety)
- Use SQLite for persistence (proven pattern)
- Keep modules focused (one domain per module)
- Integrate with unified dispatch for consistent CI interface
- Store module data separately from core memories

### Don't

- Duplicate core Katra functionality
- Create tight coupling between modules
- Store volatile data as permanent memories
- Skip error reporting
- Ignore memory safety patterns

## Dynamic Module Loading

Katra now supports **dynamic module loading** - modules can be loaded, unloaded, and reloaded at runtime without restarting the daemon.

**Implemented Features:**
- ✅ **Dynamic loading**: Load modules on demand via `modules_load`
- ✅ **Module discovery**: List available modules via `modules_list`
- ✅ **Hot reload**: Reload modules during development via `modules_reload`
- ✅ **Module info**: Query module metadata via `modules_info`

**API Operations:**
- `modules_list` - List all discovered modules and their status
- `modules_load` - Load a module by name
- `modules_unload` - Unload a loaded module
- `modules_reload` - Reload a module (for development)
- `modules_info` - Get detailed module information

**See:** [MODULE_DEVELOPMENT.md](MODULE_DEVELOPMENT.md) for the complete developer guide.

## Future Considerations

Planned enhancements:
- **Inter-module communication**: Modules sharing data
- **Module versioning**: API compatibility across versions
- **Module dependencies**: Automatic dependency resolution
