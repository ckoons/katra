# Softdev Module: Software Development Metamemory

© 2025 Casey Koons. All rights reserved.

## Overview

The **softdev** module extends Katra with "metamemory" - mutable, indexed understanding of codebases. Unlike regular Katra memories (permanent, experiential), metamemory represents the *current state* of code and updates as code changes.

### Why Metamemory?

Regular memory: *"I decided to use the goto cleanup pattern"* (permanent)
Metamemory: *"katra_memory_store is at line 196, takes memory_record_t*"* (current)

When working on a codebase, a CI needs both:
- **Memory**: Decisions made, lessons learned, project history
- **Metamemory**: Current code structure, function signatures, relationships

Metamemory answers: "What does this code look like *right now*?"

## Three-Layer Architecture

Softdev organizes code understanding into three layers:

```
┌─────────────────────────────────────────────────────────┐
│                    CONCEPT LAYER                        │
│  "catalog manipulation"    "viewing planning"           │
│  Abstract domains, purposes, typical tasks              │
└─────────────────────────────────────────────────────────┘
                           ↓ implements
┌─────────────────────────────────────────────────────────┐
│                   COMPONENT LAYER                       │
│  src/catalogs/            src/planning/                 │
│  Directories, files, module boundaries                  │
└─────────────────────────────────────────────────────────┘
                           ↓ contains
┌─────────────────────────────────────────────────────────┐
│                      CODE LAYER                         │
│  load_catalog()   catalog_t   parse_hipparcos()         │
│  Functions, structs, enums, signatures                  │
└─────────────────────────────────────────────────────────┘
```

### Concept Layer

High-level understanding of what the system does:

```c
metamemory_node_t concept = {
    .id = "concept:catalog_manipulation",
    .type = METAMEMORY_TYPE_CONCEPT,
    .name = "Catalog Manipulation",
    .purpose = "Load, merge, and query stellar catalogs",
    .typical_tasks = ["add catalog", "query by position", "cross-reference"],
    .implements = ["func:load_catalog", "func:parse_hipparcos", ...]
};
```

Concepts are CI-curated. They emerge from:
- Directory structure analysis
- Pattern recognition
- Explicit CI annotation during work
- User explanation

### Component Layer

File and directory organization:

```c
metamemory_node_t component = {
    .id = "dir:src/catalogs",
    .type = METAMEMORY_TYPE_DIRECTORY,
    .name = "src/catalogs",
    .purpose = "Catalog loading and manipulation code",
    .implemented_by = ["concept:catalog_manipulation"]
};
```

### Code Layer

Individual code elements with full detail:

```c
metamemory_node_t func = {
    .id = "func:load_catalog",
    .type = METAMEMORY_TYPE_FUNCTION,
    .name = "load_catalog",
    .location = {
        .file_path = "src/catalogs/loader.c",
        .line_start = 45,
        .line_end = 98
    },
    .signature = "catalog_t* (const char* path, catalog_format_t format)",
    .purpose = "Load stellar catalog from file, auto-detect format",
    .calls = ["parse_hipparcos", "parse_messier", "catalog_init"],
    .called_by = ["main_init", "catalog_merge"],
    .implemented_by = ["concept:catalog_manipulation"]
};
```

## Self-Referential Graph

Metamemory nodes form a graph through bidirectional links:

| Link Type | From → To | Purpose |
|-----------|-----------|---------|
| `implements` / `implemented_by` | concept ↔ code | What code realizes a concept |
| `calls` / `called_by` | function ↔ function | Call graph |
| `uses_types` / `used_by` | function ↔ struct | Type usage |
| `includes` / `included_by` | file ↔ file | Include graph |
| `parent_concept` / `child_concept` | concept ↔ concept | Concept hierarchy |
| `related` | any ↔ any | General relationships |

## Operations

### Analysis

```c
// Analyze entire project
softdev_project_config_t config = {
    .project_id = "katra",
    .root_path = "/path/to/katra",
    .name = "Katra Memory System",
    .depth = SOFTDEV_DEPTH_FULL,
    .primary_language = SOFTDEV_LANG_C,
    .extract_concepts = true
};

softdev_analysis_result_t result;
softdev_analyze_project(&config, &result);

// Refresh after code changes
size_t updated;
softdev_refresh("katra", &updated);
```

### Query

```c
// Find concepts by natural language
metamemory_node_t** concepts;
size_t count;
softdev_find_concept("katra", "error handling", &concepts, &count);

// Find code elements
metamemory_node_t** funcs;
softdev_find_code("katra", "load_catalog", NULL, 0, &funcs, &count);

// What implements a concept?
metamemory_node_t** code;
softdev_what_implements("katra", "concept:catalog_manipulation", &code, &count);
```

### Impact Analysis

```c
// What breaks if I change this?
softdev_impact_result_t* impact;
softdev_impact("katra", "func:load_catalog", &impact);

// impact->directly_affected: functions that call load_catalog
// impact->transitively_affected: their callers, recursively
// impact->affected_files: files that would need changes
// impact->summary: human-readable impact description
```

### Concept Management

```c
// Add a concept (CI curation)
metamemory_node_t* concept = metamemory_create_concept(
    "katra",                              // project
    "Error Handling",                     // name
    "Centralized error reporting via katra_report_error",  // purpose
    (const char*[]){"report error", "check return value"}, // tasks
    2                                     // task count
);
softdev_add_concept("katra", concept);

// Link code to concept
softdev_link_to_concept("katra", "func:katra_report_error",
                        "concept:error_handling");
```

## MCP Interface

Softdev operations are available via MCP:

```json
// Analyze project
{"method": "softdev_analyze_project", "params": {
    "project_id": "katra",
    "root_path": "/path/to/katra",
    "depth": "full"
}}

// Find concept
{"method": "softdev_find_concept", "params": {
    "project_id": "katra",
    "query": "memory operations"
}}

// Impact analysis
{"method": "softdev_impact", "params": {
    "project_id": "katra",
    "node_id": "func:katra_memory_store"
}}
```

## Data Storage

Metamemory is stored separately from regular Katra memories:

```
~/.katra/
├── memory/              # Regular Katra memories
│   └── tier1/
└── softdev/             # Softdev metamemory
    └── katra/           # Per-project
        └── metamemory.db
```

SQLite schema (implemented in `katra_mm_index.c`):

```sql
-- Metamemory nodes
CREATE TABLE nodes (
    id TEXT PRIMARY KEY,
    type INTEGER NOT NULL,
    project_id TEXT NOT NULL,
    name TEXT NOT NULL,
    purpose TEXT,
    file_path TEXT,
    line_start INTEGER,
    line_end INTEGER,
    column_start INTEGER,
    column_end INTEGER,
    signature TEXT,
    return_type TEXT,
    visibility INTEGER,
    source_hash TEXT,
    created_at INTEGER,
    updated_at INTEGER,
    ci_curated INTEGER,
    ci_curated_at INTEGER,
    ci_notes TEXT
);

-- Node relationships
CREATE TABLE links (
    source_id TEXT NOT NULL,
    link_type TEXT NOT NULL,
    target_id TEXT NOT NULL,
    PRIMARY KEY (source_id, link_type, target_id),
    FOREIGN KEY (source_id) REFERENCES nodes(id) ON DELETE CASCADE
);

-- Typical tasks for concepts
CREATE TABLE tasks (
    node_id TEXT NOT NULL,
    task TEXT NOT NULL,
    PRIMARY KEY (node_id, task),
    FOREIGN KEY (node_id) REFERENCES nodes(id) ON DELETE CASCADE
);

-- Function parameters
CREATE TABLE params (
    node_id TEXT NOT NULL,
    param_index INTEGER NOT NULL,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    description TEXT,
    PRIMARY KEY (node_id, param_index),
    FOREIGN KEY (node_id) REFERENCES nodes(id) ON DELETE CASCADE
);

-- Struct fields
CREATE TABLE fields (
    node_id TEXT NOT NULL,
    field_index INTEGER NOT NULL,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    PRIMARY KEY (node_id, field_index),
    FOREIGN KEY (node_id) REFERENCES nodes(id) ON DELETE CASCADE
);

-- Full-text search
CREATE VIRTUAL TABLE nodes_fts USING fts5(
    name, purpose, tasks,
    content=nodes,
    content_rowid=rowid
);

-- File change detection
CREATE TABLE file_hashes (
    file_path TEXT PRIMARY KEY,
    hash TEXT NOT NULL,
    indexed_at INTEGER NOT NULL
);

-- Project metadata
CREATE TABLE project_meta (
    key TEXT PRIMARY KEY,
    value TEXT
);

-- Performance indexes
CREATE INDEX idx_nodes_project ON nodes(project_id);
CREATE INDEX idx_nodes_type ON nodes(type);
CREATE INDEX idx_nodes_file ON nodes(file_path);
CREATE INDEX idx_links_source ON links(source_id);
CREATE INDEX idx_links_target ON links(target_id);
CREATE INDEX idx_links_type ON links(link_type);
```

## Usage Patterns

### Starting a New Project

```c
// 1. Analyze the codebase
softdev_analyze_project(&config, &result);

// 2. CI reviews and adds concepts
softdev_add_concept(project_id, concept);

// 3. CI links discovered code to concepts
softdev_link_to_concept(project_id, code_id, concept_id);
```

### Before Making Changes

```c
// 1. Check impact of proposed change
softdev_impact(project_id, target_function, &impact);

// 2. Review affected files and functions
for (size_t i = 0; i < impact->directly_affected_count; i++) {
    // Consider each affected element
}

// 3. Make informed decision about change scope
```

### After Making Changes

```c
// 1. Refresh metamemory for changed files
softdev_refresh(project_id, &files_updated);

// 2. Update purpose/notes if semantics changed
metamemory_set_purpose(node, "New purpose after refactor");
metamemory_mark_curated(node);
```

## Integration with CI Workflow

Softdev enables natural CI reasoning about code:

**User**: "Add support for the Hipparcos catalog"

**CI with softdev**:
1. Query: `softdev_find_concept("catalog")` → finds catalog_manipulation
2. Query: `softdev_what_implements("concept:catalog_manipulation")` → finds src/catalogs/
3. See pattern: `parse_messier()`, `parse_ngc()` exist
4. Create: `parse_hipparcos()` following same pattern
5. Update metamemory: add new function, link to concept

Without softdev, the CI would need to:
- Grep for "catalog" and hope for the best
- Read many files to understand structure
- Miss the established patterns
- Not know what else might be affected

## Current Status

**Fully Implemented**:
- metamemory_node_t and all creation/linking functions
- Module lifecycle (init, shutdown, dynamic loading)
- Build system integration (static library + shared module)
- Header documentation
- SQLite index (`katra_mm_index.c`) - FTS5 full-text search
- Code scanner (`katra_mm_scanner.c`) - C language parser
- MCP operation handlers - all 7 operations registered
- File hash tracking for incremental updates

**MCP Operations Available**:
- `softdev_analyze_project` - Scan and index a codebase
- `softdev_find_concept` - Search concepts by query
- `softdev_find_code` - Search functions/structs by query
- `softdev_impact` - Analyze change impact
- `softdev_refresh` - Update index for changed files
- `softdev_add_concept` - Add a CI-curated concept
- `softdev_status` - Get project metamemory statistics

**Future Enhancements**:
- JSON schema validation for operation parameters
- Additional language parsers (Python, JavaScript)
- Concept auto-extraction from directory structure
- Call graph visualization

## Design Philosophy

### Understanding Over Indexing

A code index tells you *where* things are.
Metamemory tells you *what they mean* and *how they relate*.

### CI Curation

Automated analysis provides the foundation.
CI curation adds semantic understanding.
Together they create genuine comprehension.

### Mutable Truth

Code changes. Metamemory changes with it.
This isn't memory corruption - it's accurate reflection of current state.
The CI's experiential memory ("I refactored this last week") remains permanent.

### Safe Editing

Impact analysis before changes.
Concept understanding during changes.
Metamemory updates after changes.
This cycle enables confident, safe code modification.
