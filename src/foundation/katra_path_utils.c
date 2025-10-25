/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

/* Project includes */
#include "katra_path_utils.h"
#include "katra_error.h"
#include "katra_limits.h"

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
