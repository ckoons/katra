/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>

/* Project includes */
#include "katra_path_utils.h"
#include "katra_error.h"
#include "katra_limits.h"
#include "katra_string_literals.h"
#include "katra_env_utils.h"

/* Get home directory path */
int katra_get_home_dir(char* buffer, size_t size) {
    if (!buffer || size == 0) {
        return E_INPUT_NULL;
    }

    const char* home = getenv("HOME");
    if (!home) {
        return E_SYSTEM_FILE;
    }

    strncpy(buffer, home, size - 1);
    buffer[size - 1] = '\0';

    return KATRA_SUCCESS;
}

/* Build path under ~/.katra/ */
int katra_build_path(char* buffer, size_t size, ...) {
    if (!buffer || size == 0) {
        return E_INPUT_NULL;
    }

    /* Get home directory */
    char home[KATRA_PATH_MAX];
    int result = katra_get_home_dir(home, sizeof(home));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Start with ~/.katra */
    size_t offset = 0;
    offset += snprintf(buffer + offset, size - offset, "%s/.katra", home);

    /* Append each path component */
    va_list args;
    va_start(args, size);

    const char* component;
    while ((component = va_arg(args, const char*)) != NULL) {
        if (offset >= size - 1) {
            va_end(args);
            return E_INPUT_TOO_LARGE;
        }

        offset += snprintf(buffer + offset, size - offset, "/%s", component);
    }

    va_end(args);

    if (offset >= size) {
        return E_INPUT_TOO_LARGE;
    }

    return KATRA_SUCCESS;
}

/* Ensure directory exists (create if needed) */
int katra_ensure_dir(const char* path) {
    if (!path) {
        return E_INPUT_NULL;
    }

    /* Check if already exists */
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return KATRA_SUCCESS;  /* Already exists */
        }
        return E_SYSTEM_FILE;  /* Exists but not a directory */
    }

    /* Need to create - first ensure parent exists */
    char parent[KATRA_PATH_MAX];
    strncpy(parent, path, sizeof(parent) - 1);
    parent[sizeof(parent) - 1] = '\0';

    /* Find last slash */
    char* last_slash = strrchr(parent, '/');
    if (last_slash && last_slash != parent) {
        *last_slash = '\0';

        /* Recursively ensure parent exists */
        int result = katra_ensure_dir(parent);
        if (result != KATRA_SUCCESS) {
            return result;
        }
    }

    /* Create this directory */
    if (mkdir(path, KATRA_DIR_PERMISSIONS) != 0) {
        /* Ignore error if directory was created by another process */
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            return KATRA_SUCCESS;
        }
        return E_SYSTEM_FILE;
    }

    return KATRA_SUCCESS;
}

/* Build and ensure katra directory exists */
int katra_build_and_ensure_dir(char* buffer, size_t size, ...) {
    if (!buffer || size == 0) {
        return E_INPUT_NULL;
    }

    /* Get home directory */
    char home[KATRA_PATH_MAX];
    int result = katra_get_home_dir(home, sizeof(home));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Start with ~/.katra */
    size_t offset = 0;
    offset += snprintf(buffer + offset, size - offset, "%s/.katra", home);

    /* Append each path component */
    va_list args;
    va_start(args, size);

    const char* component;
    while ((component = va_arg(args, const char*)) != NULL) {
        if (offset >= size - 1) {
            va_end(args);
            return E_INPUT_TOO_LARGE;
        }

        offset += snprintf(buffer + offset, size - offset, "/%s", component);
    }

    va_end(args);

    if (offset >= size) {
        return E_INPUT_TOO_LARGE;
    }

    /* Ensure the directory exists */
    return katra_ensure_dir(buffer);
}

/* Join directory and filename */
int katra_path_join(char* dest, size_t dest_size,
                    const char* dir, const char* filename) {
    if (!dest || !dir || !filename) {
        return E_INPUT_NULL;
    }

    /* Check if dir has trailing slash */
    size_t dir_len = strlen(dir);
    bool has_slash = (dir_len > 0 && dir[dir_len - 1] == '/');

    /* Build path */
    int written = snprintf(dest, dest_size, "%s%s%s",
                          dir,
                          has_slash ? STR_EMPTY : STR_SLASH,
                          filename);

    if (written < 0 || (size_t)written >= dest_size) {
        return E_INPUT_TOO_LARGE;
    }

    return KATRA_SUCCESS;
}

/* Join directory, filename, and extension */
int katra_path_join_with_ext(char* dest, size_t dest_size,
                              const char* dir, const char* filename, const char* ext) {
    if (!dest || !dir || !filename || !ext) {
        return E_INPUT_NULL;
    }

    /* Check if dir has trailing slash */
    size_t dir_len = strlen(dir);
    bool has_slash = (dir_len > 0 && dir[dir_len - 1] == '/');

    /* Build path with extension */
    int written = snprintf(dest, dest_size, "%s%s%s.%s",
                          dir,
                          has_slash ? STR_EMPTY : STR_SLASH,
                          filename,
                          ext);

    if (written < 0 || (size_t)written >= dest_size) {
        return E_INPUT_TOO_LARGE;
    }

    return KATRA_SUCCESS;
}

/* Get persona home directory */
int katra_get_persona_dir(char* buffer, size_t size, const char* persona_name) {
    int result = KATRA_SUCCESS;

    if (!buffer || size == 0) {
        return E_INPUT_NULL;
    }

    /* Get home directory */
    char home[KATRA_PATH_MAX];
    result = katra_get_home_dir(home, sizeof(home));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check layout mode from environment */
    const char* layout = katra_getenv("KATRA_PERSONA_LAYOUT");
    bool unified = (layout && strcmp(layout, "unified") == 0);

    if (unified && persona_name) {
        /* Unified layout: ~/.katra/personas/{persona_name} */
        int written = snprintf(buffer, size, "%s/.katra/personas/%s",
                              home, persona_name);
        if (written < 0 || (size_t)written >= size) {
            return E_INPUT_TOO_LARGE;
        }
    } else {
        /* Scattered layout (default): ~/.katra */
        int written = snprintf(buffer, size, "%s/.katra", home);
        if (written < 0 || (size_t)written >= size) {
            return E_INPUT_TOO_LARGE;
        }
    }

    return KATRA_SUCCESS;
}

/* Build path under persona directory */
int katra_build_persona_path(char* buffer, size_t size, const char* persona_name, ...) {
    int result = KATRA_SUCCESS;

    if (!buffer || size == 0 || !persona_name) {
        return E_INPUT_NULL;
    }

    /* Get home directory */
    char home[KATRA_PATH_MAX];
    result = katra_get_home_dir(home, sizeof(home));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check layout mode from environment */
    const char* layout = katra_getenv("KATRA_PERSONA_LAYOUT");
    bool unified = (layout && strcmp(layout, "unified") == 0);

    size_t offset = 0;

    if (unified) {
        /* Unified layout: ~/.katra/personas/{persona_name}/{components...} */
        offset += snprintf(buffer + offset, size - offset,
                          "%s/.katra/personas/%s", home, persona_name);

        /* Append path components */
        va_list args;
        va_start(args, persona_name);

        const char* component;
        while ((component = va_arg(args, const char*)) != NULL) {
            if (offset >= size - 1) {
                va_end(args);
                return E_INPUT_TOO_LARGE;
            }

            offset += snprintf(buffer + offset, size - offset, "/%s", component);
        }

        va_end(args);
    } else {
        /* Scattered layout: ~/.katra/{components...}/{persona_name} */
        /* Note: This maintains backward compatibility with existing code */

        /* Start with ~/.katra */
        offset += snprintf(buffer + offset, size - offset, "%s/.katra", home);

        /* Append path components (except persona goes at end) */
        va_list args;
        va_start(args, persona_name);

        const char* component;
        while ((component = va_arg(args, const char*)) != NULL) {
            if (offset >= size - 1) {
                va_end(args);
                return E_INPUT_TOO_LARGE;
            }

            offset += snprintf(buffer + offset, size - offset, "/%s", component);
        }

        va_end(args);

        /* Append persona name at the end (scattered layout pattern) */
        if (offset >= size - 1) {
            return E_INPUT_TOO_LARGE;
        }

        offset += snprintf(buffer + offset, size - offset, "/%s", persona_name);
    }

    if (offset >= size) {
        return E_INPUT_TOO_LARGE;
    }

    return KATRA_SUCCESS;
}

/* Get project root directory */
int katra_get_project_root(char* buffer, size_t size) {
    if (!buffer || size == 0) {
        return E_INPUT_NULL;
    }

    char cwd[KATRA_PATH_MAX];
    char search_path[KATRA_PATH_MAX];

    /* Get current working directory */
    if (!getcwd(cwd, sizeof(cwd))) {
        return E_SYSTEM_FILE;
    }

    /* Search upward for .git or Makefile */
    strncpy(search_path, cwd, sizeof(search_path) - 1);
    search_path[sizeof(search_path) - 1] = '\0';

    while (true) {
        struct stat st;
        char git_path[KATRA_PATH_MAX];
        char makefile_path[KATRA_PATH_MAX];

        /* Check for .git directory */
        snprintf(git_path, sizeof(git_path), "%s/.git", search_path);
        if (stat(git_path, &st) == 0) {
            /* Found .git - this is project root */
            strncpy(buffer, search_path, size - 1);
            buffer[size - 1] = '\0';
            return KATRA_SUCCESS;
        }

        /* Check for Makefile */
        snprintf(makefile_path, sizeof(makefile_path), "%s/Makefile", search_path);
        if (stat(makefile_path, &st) == 0) {
            /* Found Makefile - this is project root */
            strncpy(buffer, search_path, size - 1);
            buffer[size - 1] = '\0';
            return KATRA_SUCCESS;
        }

        /* Move up one directory */
        char* last_slash = strrchr(search_path, '/');
        if (!last_slash || last_slash == search_path) {
            /* Reached filesystem root without finding project root */
            return E_SYSTEM_FILE;
        }

        *last_slash = '\0';
    }

    return E_SYSTEM_FILE;
}

/* Get shipped persona directory */
int katra_get_shipped_persona_dir(char* buffer, size_t size, const char* persona_name) {
    if (!buffer || size == 0 || !persona_name) {
        return E_INPUT_NULL;
    }

    /* Get project root */
    char project_root[KATRA_PATH_MAX];
    int result = katra_get_project_root(project_root, sizeof(project_root));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Build path: {project_root}/personas/{persona_name} */
    int written = snprintf(buffer, size, "%s/personas/%s",
                          project_root, persona_name);

    if (written < 0 || (size_t)written >= size) {
        return E_INPUT_TOO_LARGE;
    }

    return KATRA_SUCCESS;
}

/* Get user persona directory */
int katra_get_user_persona_dir(char* buffer, size_t size, const char* persona_name) {
    if (!buffer || size == 0 || !persona_name) {
        return E_INPUT_NULL;
    }

    /* Get home directory */
    char home[KATRA_PATH_MAX];
    int result = katra_get_home_dir(home, sizeof(home));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Build path: ~/.katra/personas/{persona_name} */
    int written = snprintf(buffer, size, "%s/.katra/personas/%s",
                          home, persona_name);

    if (written < 0 || (size_t)written >= size) {
        return E_INPUT_TOO_LARGE;
    }

    return KATRA_SUCCESS;
}

/* Build path under user persona directory */
int katra_build_user_persona_path(char* buffer, size_t size, const char* persona_name, ...) {
    if (!buffer || size == 0 || !persona_name) {
        return E_INPUT_NULL;
    }

    /* Get home directory */
    char home[KATRA_PATH_MAX];
    int result = katra_get_home_dir(home, sizeof(home));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Start with ~/.katra/personas/{persona_name} */
    size_t offset = 0;
    offset += snprintf(buffer + offset, size - offset,
                      "%s/.katra/personas/%s", home, persona_name);

    /* Append path components */
    va_list args;
    va_start(args, persona_name);

    const char* component;
    while ((component = va_arg(args, const char*)) != NULL) {
        if (offset >= size - 1) {
            va_end(args);
            return E_INPUT_TOO_LARGE;
        }

        offset += snprintf(buffer + offset, size - offset, "/%s", component);
    }

    va_end(args);

    if (offset >= size) {
        return E_INPUT_TOO_LARGE;
    }

    return KATRA_SUCCESS;
}
