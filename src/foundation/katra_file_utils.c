/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

/* Project includes */
#include "katra_file_utils.h"
#include "katra_error.h"
#include "katra_limits.h"

/* Count lines in a file */
int katra_file_count_lines(const char* filepath, size_t* count) {
    if (!filepath || !count) {
        return E_INPUT_NULL;
    }

    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        *count = 0;
        return KATRA_SUCCESS;  /* File doesn't exist - not an error */
    }

    size_t lines = 0;
    char buffer[KATRA_BUFFER_LARGE];

    while (fgets(buffer, sizeof(buffer), fp)) {
        lines++;
    }

    fclose(fp);
    *count = lines;

    return KATRA_SUCCESS;
}

/* Get file size in bytes */
int katra_file_get_size(const char* filepath, size_t* size) {
    if (!filepath || !size) {
        return E_INPUT_NULL;
    }

    struct stat st;
    if (stat(filepath, &st) != 0) {
        return E_SYSTEM_FILE;
    }

    *size = st.st_size;
    return KATRA_SUCCESS;
}

/* Iterate over files in directory */
int katra_dir_foreach(const char* dir_path, const char* extension,
                      katra_file_visitor_fn visitor, void* userdata) {
    if (!dir_path || !visitor) {
        return E_INPUT_NULL;
    }

    DIR* dir = opendir(dir_path);
    if (!dir) {
        return E_SYSTEM_FILE;
    }

    int result = KATRA_SUCCESS;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Filter by extension if provided */
        if (extension) {
            if (!strstr(entry->d_name, extension)) {
                continue;
            }
        }

        /* Build full path */
        char filepath[KATRA_PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);

        /* Call visitor */
        result = visitor(filepath, userdata);
        if (result != KATRA_SUCCESS) {
            break;  /* Visitor requested stop */
        }
    }

    closedir(dir);
    return result;
}
