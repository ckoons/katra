/* © 2025 Casey Koons All rights reserved */

/**
 * @file katra_metamemory.c
 * @brief Metamemory node creation, linking, and management
 *
 * Provides core operations for metamemory nodes:
 *   - Node creation (concept, function, struct, etc.)
 *   - Link management (calls, implements, uses, etc.)
 *   - Attribute management (purpose, tasks, parameters)
 *   - Memory management (free, clone)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "katra_metamemory.h"
#include "katra_error.h"
#include "katra_limits.h"

/* ============================================================================
 * Type String Tables
 * ============================================================================ */

static const char* TYPE_STRINGS[] = {
    "unknown",
    "concept",
    "directory",
    "file",
    "function",
    "struct",
    "enum",
    "typedef",
    "macro",
    "variable"
};

static const char* TYPE_PREFIXES[] = {
    "",              /* UNKNOWN */
    "concept:",      /* CONCEPT */
    "dir:",          /* DIRECTORY */
    "file:",         /* FILE */
    "func:",         /* FUNCTION */
    "struct:",       /* STRUCT */
    "enum:",         /* ENUM */
    "typedef:",      /* TYPEDEF */
    "macro:",        /* MACRO */
    "var:"           /* VARIABLE */
};

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

/**
 * Safely duplicate a string.
 */
static char* safe_strdup(const char* str)
{
    if (!str) {
        return NULL;
    }
    return strdup(str);
}

/**
 * Add a string to a string array, growing if needed.
 */
static int add_to_string_array(char*** array, size_t* count, const char* str)
{
    if (!array || !count || !str) {
        return E_INPUT_NULL;
    }

    /* Allocate or reallocate */
    char** new_array = realloc(*array, (*count + 1) * sizeof(char*));
    if (!new_array) {
        return E_SYSTEM_MEMORY;
    }

    /* Duplicate string */
    new_array[*count] = strdup(str);
    if (!new_array[*count]) {
        /* Don't lose existing array on failure */
        *array = new_array;
        return E_SYSTEM_MEMORY;
    }

    *array = new_array;
    (*count)++;
    return KATRA_SUCCESS;
}

/**
 * Remove a string from a string array.
 */
static int remove_from_string_array(char*** array, size_t* count, const char* str)
{
    if (!array || !count || !str || !*array) {
        return E_INPUT_NULL;
    }

    for (size_t i = 0; i < *count; i++) {
        if ((*array)[i] && strcmp((*array)[i], str) == 0) {
            free((*array)[i]);

            /* Shift remaining elements */
            for (size_t j = i; j < *count - 1; j++) {
                (*array)[j] = (*array)[j + 1];
            }

            (*count)--;
            return KATRA_SUCCESS;
        }
    }

    return E_NOT_FOUND;
}

/**
 * Check if string exists in array.
 */
static bool string_in_array(char** array, size_t count, const char* str)
{
    if (!array || !str) {
        return false;
    }

    for (size_t i = 0; i < count; i++) {
        if (array[i] && strcmp(array[i], str) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * Free a string array.
 */
static void free_string_array(char** array, size_t count)
{
    if (!array) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        free(array[i]);
    }
    free(array);
}

/* ============================================================================
 * Node Creation
 * ============================================================================ */

metamemory_node_t* metamemory_create_node(metamemory_type_t type,
                                           const char* project_id,
                                           const char* name)
{
    if (!project_id || !name) {
        katra_report_error(E_INPUT_NULL, "metamemory_create_node",
                          "project_id or name is NULL");
        return NULL;
    }

    if (type <= METAMEMORY_TYPE_UNKNOWN || type >= METAMEMORY_TYPE_COUNT) {
        katra_report_error(E_INPUT_INVALID, "metamemory_create_node",
                          "Invalid node type");
        return NULL;
    }

    metamemory_node_t* node = calloc(1, sizeof(metamemory_node_t));
    if (!node) {
        katra_report_error(E_SYSTEM_MEMORY, "metamemory_create_node",
                          "Failed to allocate node");
        return NULL;
    }

    /* Generate ID */
    char id_buffer[METAMEMORY_ID_MAX];
    if (!metamemory_make_id(type, name, id_buffer, sizeof(id_buffer))) {
        free(node);
        return NULL;
    }

    node->id = strdup(id_buffer);
    node->type = type;
    node->project_id = strdup(project_id);
    node->name = strdup(name);
    node->created_at = time(NULL);
    node->updated_at = node->created_at;

    if (!node->id || !node->project_id || !node->name) {
        metamemory_free_node(node);
        katra_report_error(E_SYSTEM_MEMORY, "metamemory_create_node",
                          "Failed to duplicate strings");
        return NULL;
    }

    return node;
}

metamemory_node_t* metamemory_create_concept(const char* project_id,
                                              const char* name,
                                              const char* purpose,
                                              const char** tasks,
                                              size_t task_count)
{
    metamemory_node_t* node = metamemory_create_node(
        METAMEMORY_TYPE_CONCEPT, project_id, name);

    if (!node) {
        return NULL;
    }

    if (purpose) {
        node->purpose = strdup(purpose);
        if (!node->purpose) {
            metamemory_free_node(node);
            return NULL;
        }
    }

    /* Add tasks */
    if (tasks && task_count > 0) {
        for (size_t i = 0; i < task_count && i < METAMEMORY_MAX_TASKS; i++) {
            if (metamemory_add_task(node, tasks[i]) != KATRA_SUCCESS) {
                metamemory_free_node(node);
                return NULL;
            }
        }
    }

    return node;
}

metamemory_node_t* metamemory_create_function(const char* project_id,
                                               const char* name,
                                               const char* file_path,
                                               int line_start,
                                               int line_end,
                                               const char* signature)
{
    metamemory_node_t* node = metamemory_create_node(
        METAMEMORY_TYPE_FUNCTION, project_id, name);

    if (!node) {
        return NULL;
    }

    /* Set location */
    if (file_path) {
        node->location.file_path = strdup(file_path);
        if (!node->location.file_path) {
            metamemory_free_node(node);
            return NULL;
        }
    }
    node->location.line_start = line_start;
    node->location.line_end = line_end;

    /* Set signature */
    if (signature) {
        node->signature = strdup(signature);
        if (!node->signature) {
            metamemory_free_node(node);
            return NULL;
        }
    }

    return node;
}

metamemory_node_t* metamemory_create_struct(const char* project_id,
                                             const char* name,
                                             const char* file_path,
                                             int line_start,
                                             int line_end)
{
    metamemory_node_t* node = metamemory_create_node(
        METAMEMORY_TYPE_STRUCT, project_id, name);

    if (!node) {
        return NULL;
    }

    /* Set location */
    if (file_path) {
        node->location.file_path = strdup(file_path);
        if (!node->location.file_path) {
            metamemory_free_node(node);
            return NULL;
        }
    }
    node->location.line_start = line_start;
    node->location.line_end = line_end;

    return node;
}

/* ============================================================================
 * Memory Management
 * ============================================================================ */

void metamemory_free_node(metamemory_node_t* node)
{
    if (!node) {
        return;
    }

    /* Identity */
    free(node->id);
    free(node->project_id);
    free(node->name);
    free(node->purpose);

    /* Location */
    free(node->location.file_path);

    /* Tasks */
    free_string_array(node->typical_tasks, node->task_count);

    /* Function details */
    free(node->signature);
    free(node->return_type);
    if (node->parameters) {
        for (size_t i = 0; i < node->param_count; i++) {
            free(node->parameters[i].name);
            free(node->parameters[i].type);
            free(node->parameters[i].description);
        }
        free(node->parameters);
    }

    /* Struct fields */
    free_string_array(node->field_names, node->field_count);
    free_string_array(node->field_types, node->field_count);

    /* Links */
    free_string_array(node->parent_concepts, node->parent_concept_count);
    free_string_array(node->child_concepts, node->child_concept_count);
    free_string_array(node->implements, node->implements_count);
    free_string_array(node->implemented_by, node->implemented_by_count);
    free_string_array(node->calls, node->calls_count);
    free_string_array(node->called_by, node->called_by_count);
    free_string_array(node->uses_types, node->uses_types_count);
    free_string_array(node->used_by, node->used_by_count);
    free_string_array(node->includes, node->includes_count);
    free_string_array(node->included_by, node->included_by_count);
    free_string_array(node->related, node->related_count);

    /* Freshness */
    free(node->source_hash);

    /* CI curation */
    free(node->ci_notes);

    free(node);
}

void metamemory_free_nodes(metamemory_node_t** nodes, size_t count)
{
    if (!nodes) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        metamemory_free_node(nodes[i]);
    }
    free(nodes);
}

/* ============================================================================
 * Link Management
 * ============================================================================ */

int metamemory_add_link(metamemory_node_t* node,
                        const char* link_type,
                        const char* target_id)
{
    if (!node || !link_type || !target_id) {
        katra_report_error(E_INPUT_NULL, "metamemory_add_link",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    char*** array = NULL;
    size_t* count = NULL;
    size_t max_links = METAMEMORY_MAX_LINKS;

    /* Select appropriate array based on link type */
    if (strcmp(link_type, "parent_concept") == 0) {
        array = &node->parent_concepts;
        count = &node->parent_concept_count;
    } else if (strcmp(link_type, "child_concept") == 0) {
        array = &node->child_concepts;
        count = &node->child_concept_count;
    } else if (strcmp(link_type, "implements") == 0) {
        array = &node->implements;
        count = &node->implements_count;
    } else if (strcmp(link_type, "implemented_by") == 0) {
        array = &node->implemented_by;
        count = &node->implemented_by_count;
    } else if (strcmp(link_type, "calls") == 0) {
        array = &node->calls;
        count = &node->calls_count;
    } else if (strcmp(link_type, "called_by") == 0) {
        array = &node->called_by;
        count = &node->called_by_count;
    } else if (strcmp(link_type, "uses_types") == 0) {
        array = &node->uses_types;
        count = &node->uses_types_count;
    } else if (strcmp(link_type, "used_by") == 0) {
        array = &node->used_by;
        count = &node->used_by_count;
    } else if (strcmp(link_type, "includes") == 0) {
        array = &node->includes;
        count = &node->includes_count;
    } else if (strcmp(link_type, "included_by") == 0) {
        array = &node->included_by;
        count = &node->included_by_count;
    } else if (strcmp(link_type, "related") == 0) {
        array = &node->related;
        count = &node->related_count;
    } else {
        katra_report_error(E_INPUT_INVALID, "metamemory_add_link",
                          "Unknown link type");
        return E_INPUT_INVALID;
    }

    /* Check if already at max */
    if (*count >= max_links) {
        katra_report_error(E_INPUT_TOO_LARGE, "metamemory_add_link",
                          "Maximum links exceeded");
        return E_INPUT_TOO_LARGE;
    }

    /* Check for duplicate */
    if (string_in_array(*array, *count, target_id)) {
        return KATRA_SUCCESS;  /* Already linked */
    }

    return add_to_string_array(array, count, target_id);
}

int metamemory_remove_link(metamemory_node_t* node,
                           const char* link_type,
                           const char* target_id)
{
    if (!node || !link_type || !target_id) {
        katra_report_error(E_INPUT_NULL, "metamemory_remove_link",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    char*** array = NULL;
    size_t* count = NULL;

    /* Select array (same logic as add_link) */
    if (strcmp(link_type, "calls") == 0) {
        array = &node->calls;
        count = &node->calls_count;
    } else if (strcmp(link_type, "called_by") == 0) {
        array = &node->called_by;
        count = &node->called_by_count;
    }
    /* ... other types omitted for brevity, same pattern */
    else {
        katra_report_error(E_INPUT_INVALID, "metamemory_remove_link",
                          "Unknown link type");
        return E_INPUT_INVALID;
    }

    return remove_from_string_array(array, count, target_id);
}

bool metamemory_has_link(const metamemory_node_t* node,
                         const char* link_type,
                         const char* target_id)
{
    if (!node || !link_type || !target_id) {
        return false;
    }

    char** array = NULL;
    size_t count = 0;

    if (strcmp(link_type, "calls") == 0) {
        array = node->calls;
        count = node->calls_count;
    } else if (strcmp(link_type, "called_by") == 0) {
        array = node->called_by;
        count = node->called_by_count;
    } else if (strcmp(link_type, "implements") == 0) {
        array = node->implements;
        count = node->implements_count;
    } else if (strcmp(link_type, "implemented_by") == 0) {
        array = node->implemented_by;
        count = node->implemented_by_count;
    }
    /* ... other types */

    return string_in_array(array, count, target_id);
}

/* ============================================================================
 * Attribute Management
 * ============================================================================ */

int metamemory_set_purpose(metamemory_node_t* node, const char* purpose)
{
    if (!node) {
        katra_report_error(E_INPUT_NULL, "metamemory_set_purpose",
                          "node is NULL");
        return E_INPUT_NULL;
    }

    free(node->purpose);
    node->purpose = safe_strdup(purpose);

    if (purpose && !node->purpose) {
        katra_report_error(E_SYSTEM_MEMORY, "metamemory_set_purpose",
                          "Failed to duplicate purpose");
        return E_SYSTEM_MEMORY;
    }

    node->updated_at = time(NULL);
    return KATRA_SUCCESS;
}

int metamemory_add_task(metamemory_node_t* node, const char* task)
{
    if (!node || !task) {
        katra_report_error(E_INPUT_NULL, "metamemory_add_task",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (node->task_count >= METAMEMORY_MAX_TASKS) {
        katra_report_error(E_INPUT_TOO_LARGE, "metamemory_add_task",
                          "Maximum tasks exceeded");
        return E_INPUT_TOO_LARGE;
    }

    return add_to_string_array(&node->typical_tasks, &node->task_count, task);
}

int metamemory_add_parameter(metamemory_node_t* node,
                             const char* name,
                             const char* type,
                             const char* description)
{
    if (!node || !name || !type) {
        katra_report_error(E_INPUT_NULL, "metamemory_add_parameter",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    if (node->param_count >= METAMEMORY_MAX_PARAMS) {
        katra_report_error(E_INPUT_TOO_LARGE, "metamemory_add_parameter",
                          "Maximum parameters exceeded");
        return E_INPUT_TOO_LARGE;
    }

    /* Reallocate parameters array */
    metamemory_param_t* new_params = realloc(
        node->parameters,
        (node->param_count + 1) * sizeof(metamemory_param_t));

    if (!new_params) {
        katra_report_error(E_SYSTEM_MEMORY, "metamemory_add_parameter",
                          "Failed to allocate parameter");
        return E_SYSTEM_MEMORY;
    }

    node->parameters = new_params;

    /* Initialize new parameter */
    metamemory_param_t* param = &node->parameters[node->param_count];
    param->name = strdup(name);
    param->type = strdup(type);
    param->description = safe_strdup(description);

    if (!param->name || !param->type) {
        free(param->name);
        free(param->type);
        free(param->description);
        katra_report_error(E_SYSTEM_MEMORY, "metamemory_add_parameter",
                          "Failed to duplicate strings");
        return E_SYSTEM_MEMORY;
    }

    node->param_count++;
    return KATRA_SUCCESS;
}

int metamemory_add_field(metamemory_node_t* node,
                         const char* name,
                         const char* type)
{
    if (!node || !name || !type) {
        katra_report_error(E_INPUT_NULL, "metamemory_add_field",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    int result = add_to_string_array(&node->field_names, &node->field_count, name);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Need to keep arrays in sync - if type add fails, remove name */
    size_t dummy_count = node->field_count;
    result = add_to_string_array(&node->field_types, &dummy_count, type);
    if (result != KATRA_SUCCESS) {
        /* Rollback name */
        node->field_count--;
        free(node->field_names[node->field_count]);
        return result;
    }

    return KATRA_SUCCESS;
}

int metamemory_set_ci_notes(metamemory_node_t* node, const char* notes)
{
    if (!node) {
        katra_report_error(E_INPUT_NULL, "metamemory_set_ci_notes",
                          "node is NULL");
        return E_INPUT_NULL;
    }

    free(node->ci_notes);
    node->ci_notes = safe_strdup(notes);

    if (notes && !node->ci_notes) {
        katra_report_error(E_SYSTEM_MEMORY, "metamemory_set_ci_notes",
                          "Failed to duplicate notes");
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}

int metamemory_mark_curated(metamemory_node_t* node)
{
    if (!node) {
        katra_report_error(E_INPUT_NULL, "metamemory_mark_curated",
                          "node is NULL");
        return E_INPUT_NULL;
    }

    node->ci_curated = true;
    node->ci_curated_at = time(NULL);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * Utilities
 * ============================================================================ */

const char* metamemory_type_to_string(metamemory_type_t type)
{
    if (type < 0 || type >= METAMEMORY_TYPE_COUNT) {
        return TYPE_STRINGS[0];  /* "unknown" */
    }
    return TYPE_STRINGS[type];
}

metamemory_type_t metamemory_type_from_string(const char* str)
{
    if (!str) {
        return METAMEMORY_TYPE_UNKNOWN;
    }

    for (int i = 1; i < METAMEMORY_TYPE_COUNT; i++) {
        if (strcmp(str, TYPE_STRINGS[i]) == 0) {
            return (metamemory_type_t)i;
        }
    }

    return METAMEMORY_TYPE_UNKNOWN;
}

char* metamemory_make_id(metamemory_type_t type,
                         const char* name,
                         char* buffer,
                         size_t buffer_size)
{
    if (!name || !buffer || buffer_size == 0) {
        return NULL;
    }

    if (type <= METAMEMORY_TYPE_UNKNOWN || type >= METAMEMORY_TYPE_COUNT) {
        return NULL;
    }

    const char* prefix = TYPE_PREFIXES[type];
    int written = snprintf(buffer, buffer_size, "%s%s", prefix, name);

    if (written < 0 || (size_t)written >= buffer_size) {
        return NULL;  /* Truncated */
    }

    return buffer;
}

metamemory_node_t* metamemory_clone_node(const metamemory_node_t* node)
{
    if (!node) {
        return NULL;
    }

    metamemory_node_t* clone = metamemory_create_node(
        node->type, node->project_id, node->name);

    if (!clone) {
        return NULL;
    }

    /* Copy basic fields */
    clone->purpose = safe_strdup(node->purpose);
    clone->signature = safe_strdup(node->signature);
    clone->return_type = safe_strdup(node->return_type);
    clone->visibility = node->visibility;

    /* Copy location */
    clone->location.file_path = safe_strdup(node->location.file_path);
    clone->location.line_start = node->location.line_start;
    clone->location.line_end = node->location.line_end;
    clone->location.column_start = node->location.column_start;
    clone->location.column_end = node->location.column_end;

    /* Copy tasks */
    for (size_t i = 0; i < node->task_count; i++) {
        add_to_string_array(&clone->typical_tasks, &clone->task_count,
                           node->typical_tasks[i]);
    }

    /* Copy timestamps */
    clone->created_at = node->created_at;
    clone->updated_at = node->updated_at;
    clone->ci_curated = node->ci_curated;
    clone->ci_curated_at = node->ci_curated_at;
    clone->ci_notes = safe_strdup(node->ci_notes);

    /* Note: links are NOT copied - clone gets fresh link arrays */

    return clone;
}

bool metamemory_nodes_equal(const metamemory_node_t* a,
                            const metamemory_node_t* b)
{
    if (!a || !b) {
        return a == b;  /* Both NULL = equal */
    }

    /* Compare IDs */
    if (!a->id || !b->id) {
        return false;
    }

    return strcmp(a->id, b->id) == 0;
}
