/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_METAMEMORY_H
#define KATRA_METAMEMORY_H

/**
 * @file katra_metamemory.h
 * @brief Metamemory types for software development understanding
 *
 * Metamemory is distinct from regular Katra memory:
 *   - Memory: Permanent, immutable, experiential ("I decided to use goto cleanup")
 *   - Metamemory: Mutable, indexed, current-state ("load_catalog is at line 45")
 *
 * Metamemory nodes form a self-referential graph:
 *   - Concepts link to code that implements them
 *   - Code links to concepts it implements
 *   - Functions link to functions they call
 *   - Data structures link to functions that use them
 *
 * This enables queries like:
 *   "What implements catalog handling?" -> follows concept:catalog -> src/catalogs/
 *   "What breaks if I change position_3d_t?" -> follows references -> affected code
 */

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

/* Maximum lengths */
#define METAMEMORY_ID_MAX           256
#define METAMEMORY_NAME_MAX         256
#define METAMEMORY_PATH_MAX         512
#define METAMEMORY_SIGNATURE_MAX    1024
#define METAMEMORY_PURPOSE_MAX      2048
#define METAMEMORY_MAX_LINKS        256
#define METAMEMORY_MAX_TASKS        32
#define METAMEMORY_MAX_PARAMS       32

/* Node type prefixes (used in IDs) */
#define METAMEMORY_PREFIX_CONCEPT   "concept:"
#define METAMEMORY_PREFIX_COMPONENT "component:"
#define METAMEMORY_PREFIX_FUNCTION  "func:"
#define METAMEMORY_PREFIX_STRUCT    "struct:"
#define METAMEMORY_PREFIX_ENUM      "enum:"
#define METAMEMORY_PREFIX_TYPEDEF   "typedef:"
#define METAMEMORY_PREFIX_MACRO     "macro:"
#define METAMEMORY_PREFIX_VARIABLE  "var:"
#define METAMEMORY_PREFIX_FILE      "file:"
#define METAMEMORY_PREFIX_DIR       "dir:"

/* ============================================================================
 * Types
 * ============================================================================ */

/**
 * Type of metamemory node.
 *
 * Three-layer architecture:
 *   - CONCEPT: Abstract domain ("catalog manipulation")
 *   - COMPONENT: File/directory level ("src/catalogs/")
 *   - CODE: Individual elements (functions, structs)
 */
typedef enum {
    METAMEMORY_TYPE_UNKNOWN = 0,

    /* Concept layer */
    METAMEMORY_TYPE_CONCEPT,      /* Abstract domain/purpose */

    /* Component layer */
    METAMEMORY_TYPE_DIRECTORY,    /* Directory in project */
    METAMEMORY_TYPE_FILE,         /* Source file */

    /* Code layer */
    METAMEMORY_TYPE_FUNCTION,     /* Function/method */
    METAMEMORY_TYPE_STRUCT,       /* Struct/class */
    METAMEMORY_TYPE_ENUM,         /* Enumeration */
    METAMEMORY_TYPE_TYPEDEF,      /* Type definition */
    METAMEMORY_TYPE_MACRO,        /* Macro definition */
    METAMEMORY_TYPE_VARIABLE,     /* Global/static variable */

    METAMEMORY_TYPE_COUNT
} metamemory_type_t;

/**
 * Visibility/scope of a code element.
 */
typedef enum {
    METAMEMORY_VIS_UNKNOWN = 0,
    METAMEMORY_VIS_PUBLIC,        /* Declared in header, externally visible */
    METAMEMORY_VIS_INTERNAL,      /* Declared static, file-local */
    METAMEMORY_VIS_PRIVATE        /* Implementation detail */
} metamemory_visibility_t;

/**
 * Parameter information for functions.
 */
typedef struct {
    char* name;                   /* Parameter name */
    char* type;                   /* Parameter type */
    char* description;            /* Purpose (if documented) */
} metamemory_param_t;

/**
 * Location in source code.
 */
typedef struct {
    char* file_path;              /* Path relative to project root */
    int line_start;               /* First line (1-based) */
    int line_end;                 /* Last line (1-based) */
    int column_start;             /* First column (1-based, 0 = unknown) */
    int column_end;               /* Last column (1-based, 0 = unknown) */
} metamemory_location_t;

/**
 * Core metamemory node structure.
 *
 * Represents any element in the three-layer metamemory:
 *   - Concepts (abstract domains)
 *   - Components (files, directories)
 *   - Code (functions, structs, etc.)
 */
typedef struct metamemory_node {
    /* Identity */
    char* id;                     /* Unique ID: "concept:catalog" or "func:load_catalog" */
    metamemory_type_t type;       /* Node type */
    char* project_id;             /* Which project this belongs to */

    /* Basic info */
    char* name;                   /* Human-readable name */
    char* purpose;                /* CI-written: what this does/is for */

    /* Location (for code-level nodes) */
    metamemory_location_t location;

    /* For concepts: typical tasks this handles */
    char** typical_tasks;         /* ["add catalog", "query by position"] */
    size_t task_count;

    /* For functions: signature details */
    char* signature;              /* Full signature string */
    char* return_type;            /* Return type */
    metamemory_param_t* parameters;
    size_t param_count;
    metamemory_visibility_t visibility;

    /* For structs: field information */
    char** field_names;           /* Field names */
    char** field_types;           /* Field types */
    size_t field_count;

    /* Self-referential links (the graph) */
    char** parent_concepts;       /* Concepts this belongs to */
    size_t parent_concept_count;

    char** child_concepts;        /* Concepts under this (for concept nodes) */
    size_t child_concept_count;

    char** implements;            /* Code that implements this concept */
    size_t implements_count;

    char** implemented_by;        /* Concepts this code implements */
    size_t implemented_by_count;

    char** calls;                 /* Functions this calls */
    size_t calls_count;

    char** called_by;             /* Functions that call this */
    size_t called_by_count;

    char** uses_types;            /* Types/structs this uses */
    size_t uses_types_count;

    char** used_by;               /* Code that uses this type */
    size_t used_by_count;

    char** includes;              /* Files this includes */
    size_t includes_count;

    char** included_by;           /* Files that include this */
    size_t included_by_count;

    char** related;               /* Peer/related nodes (general) */
    size_t related_count;

    /* Freshness tracking */
    time_t created_at;            /* When first indexed */
    time_t updated_at;            /* When last re-indexed */
    char* source_hash;            /* Hash of source (for change detection) */

    /* CI curation */
    bool ci_curated;              /* Has CI reviewed/updated this? */
    time_t ci_curated_at;         /* When CI last curated */
    char* ci_notes;               /* CI's notes about this element */
} metamemory_node_t;

/* ============================================================================
 * Node Creation and Management
 * ============================================================================ */

/**
 * Create a new metamemory node.
 *
 * Parameters:
 *   type - Type of node
 *   project_id - Project this belongs to
 *   name - Human-readable name
 *
 * Returns:
 *   New node (caller must free with metamemory_free_node)
 *   NULL on allocation failure
 */
metamemory_node_t* metamemory_create_node(metamemory_type_t type,
                                           const char* project_id,
                                           const char* name);

/**
 * Create a concept node.
 *
 * Convenience function for creating concept-layer nodes.
 *
 * Parameters:
 *   project_id - Project this belongs to
 *   name - Concept name ("Catalog Manipulation")
 *   purpose - What this concept represents
 *   tasks - Typical tasks (can be NULL)
 *   task_count - Number of tasks
 *
 * Returns:
 *   New concept node
 *   NULL on allocation failure
 */
metamemory_node_t* metamemory_create_concept(const char* project_id,
                                              const char* name,
                                              const char* purpose,
                                              const char** tasks,
                                              size_t task_count);

/**
 * Create a function node.
 *
 * Parameters:
 *   project_id - Project this belongs to
 *   name - Function name
 *   file_path - Source file (relative to project root)
 *   line_start - First line of function
 *   line_end - Last line of function
 *   signature - Full signature string
 *
 * Returns:
 *   New function node
 *   NULL on allocation failure
 */
metamemory_node_t* metamemory_create_function(const char* project_id,
                                               const char* name,
                                               const char* file_path,
                                               int line_start,
                                               int line_end,
                                               const char* signature);

/**
 * Create a struct node.
 */
metamemory_node_t* metamemory_create_struct(const char* project_id,
                                             const char* name,
                                             const char* file_path,
                                             int line_start,
                                             int line_end);

/**
 * Free a metamemory node and all its contents.
 */
void metamemory_free_node(metamemory_node_t* node);

/**
 * Free an array of metamemory nodes.
 */
void metamemory_free_nodes(metamemory_node_t** nodes, size_t count);

/* ============================================================================
 * Node Linking
 * ============================================================================ */

/**
 * Add a link between nodes.
 *
 * Link types:
 *   - "parent_concept" / "child_concept" - concept hierarchy
 *   - "implements" / "implemented_by" - code <-> concept
 *   - "calls" / "called_by" - function call graph
 *   - "uses_types" / "used_by" - type usage
 *   - "includes" / "included_by" - file inclusion
 *   - "related" - general relationship
 *
 * Parameters:
 *   node - Node to add link to
 *   link_type - Type of link
 *   target_id - ID of target node
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_INPUT_TOO_LARGE if max links exceeded
 */
int metamemory_add_link(metamemory_node_t* node,
                        const char* link_type,
                        const char* target_id);

/**
 * Remove a link between nodes.
 */
int metamemory_remove_link(metamemory_node_t* node,
                           const char* link_type,
                           const char* target_id);

/**
 * Check if a link exists.
 */
bool metamemory_has_link(const metamemory_node_t* node,
                         const char* link_type,
                         const char* target_id);

/* ============================================================================
 * Node Attributes
 * ============================================================================ */

/**
 * Set node purpose (CI-written description).
 */
int metamemory_set_purpose(metamemory_node_t* node, const char* purpose);

/**
 * Add a typical task to a concept node.
 */
int metamemory_add_task(metamemory_node_t* node, const char* task);

/**
 * Add a parameter to a function node.
 */
int metamemory_add_parameter(metamemory_node_t* node,
                             const char* name,
                             const char* type,
                             const char* description);

/**
 * Add a field to a struct node.
 */
int metamemory_add_field(metamemory_node_t* node,
                         const char* name,
                         const char* type);

/**
 * Set CI curation notes.
 */
int metamemory_set_ci_notes(metamemory_node_t* node, const char* notes);

/**
 * Mark node as CI-curated.
 */
int metamemory_mark_curated(metamemory_node_t* node);

/* ============================================================================
 * Utilities
 * ============================================================================ */

/**
 * Get string representation of node type.
 */
const char* metamemory_type_to_string(metamemory_type_t type);

/**
 * Parse node type from string.
 */
metamemory_type_t metamemory_type_from_string(const char* str);

/**
 * Generate node ID from type and name.
 *
 * Examples:
 *   (METAMEMORY_TYPE_CONCEPT, "catalog_manipulation") -> "concept:catalog_manipulation"
 *   (METAMEMORY_TYPE_FUNCTION, "load_catalog") -> "func:load_catalog"
 *
 * Parameters:
 *   type - Node type
 *   name - Node name
 *   buffer - Output buffer
 *   buffer_size - Size of buffer
 *
 * Returns:
 *   Pointer to buffer, or NULL on error
 */
char* metamemory_make_id(metamemory_type_t type,
                         const char* name,
                         char* buffer,
                         size_t buffer_size);

/**
 * Clone a node (deep copy).
 */
metamemory_node_t* metamemory_clone_node(const metamemory_node_t* node);

/**
 * Compare two nodes for equality.
 */
bool metamemory_nodes_equal(const metamemory_node_t* a,
                            const metamemory_node_t* b);

#endif /* KATRA_METAMEMORY_H */
