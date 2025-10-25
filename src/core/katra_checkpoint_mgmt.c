/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

/* Project includes */
#include "katra_checkpoint.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_file_utils.h"
#include "katra_strings.h"

/* External functions from katra_checkpoint.c */
extern int katra_checkpoint_get_path_internal(const char* checkpoint_id, char* buffer, size_t size);
extern int katra_checkpoint_get_dir_internal(char* buffer, size_t size);

/* List checkpoints */
int katra_checkpoint_list(const char* ci_id,
                          checkpoint_info_t** checkpoints,
                          size_t* count) {
    if (!checkpoints || !count) {
        return E_INPUT_NULL;
    }

    *checkpoints = NULL;
    *count = 0;

    char checkpoint_dir[KATRA_PATH_MAX];
    int result = katra_checkpoint_get_dir_internal(checkpoint_dir, sizeof(checkpoint_dir));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Open directory */
    DIR* dir = opendir(checkpoint_dir);
    if (!dir) {
        /* Directory doesn't exist yet - no checkpoints */
        return KATRA_SUCCESS;
    }

    /* Count matching checkpoints first */
    size_t checkpoint_count = 0;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, KATRA_CHECKPOINT_SUFFIX)) {
            checkpoint_count++;
        }
    }

    if (checkpoint_count == 0) {
        closedir(dir);
        return KATRA_SUCCESS;
    }

    /* Allocate result array */
    *checkpoints = calloc(checkpoint_count, sizeof(checkpoint_info_t));
    if (!*checkpoints) {
        closedir(dir);
        return E_SYSTEM_MEMORY;
    }

    /* Rewind and populate results */
    rewinddir(dir);
    size_t idx = 0;

    while ((entry = readdir(dir)) != NULL && idx < checkpoint_count) {
        if (strstr(entry->d_name, KATRA_CHECKPOINT_SUFFIX)) {
            /* Extract checkpoint ID from filename: checkpoint_CI_ID_TIMESTAMP.kcp */
            /* Remove prefix and suffix */
            size_t prefix_len = strlen(KATRA_CHECKPOINT_PREFIX);
            if (strncmp(entry->d_name, KATRA_CHECKPOINT_PREFIX, prefix_len) != 0) {
                continue;
            }

            const char* id_part = entry->d_name + prefix_len;
            size_t id_len = strlen(id_part);
            if (id_len < 5) continue;  /* Need at least ".kcp" */

            /* Remove suffix */
            char checkpoint_id[KATRA_BUFFER_MEDIUM];
            strncpy(checkpoint_id, id_part, sizeof(checkpoint_id) - 1);
            checkpoint_id[sizeof(checkpoint_id) - 1] = '\0';

            /* Remove extension */
            char* ext = strstr(checkpoint_id, KATRA_CHECKPOINT_SUFFIX);
            if (ext) {
                *ext = '\0';
            }

            /* Get metadata to extract CI ID for filtering */
            checkpoint_metadata_t metadata;
            if (katra_checkpoint_get_metadata(checkpoint_id, &metadata) == KATRA_SUCCESS) {
                /* Filter by CI if specified */
                if (ci_id && strcmp(metadata.ci_id, ci_id) != 0) {
                    continue;
                }

                strncpy((*checkpoints)[idx].checkpoint_id, checkpoint_id,
                       sizeof((*checkpoints)[idx].checkpoint_id) - 1);
                strncpy((*checkpoints)[idx].ci_id, metadata.ci_id,
                       sizeof((*checkpoints)[idx].ci_id) - 1);
                (*checkpoints)[idx].timestamp = metadata.timestamp;
                (*checkpoints)[idx].record_count = metadata.record_count;
                (*checkpoints)[idx].file_size = metadata.file_size;
                (*checkpoints)[idx].valid = (katra_checkpoint_validate(checkpoint_id) == KATRA_SUCCESS);
                idx++;
            }
        }
    }

    closedir(dir);
    *count = idx;

    return KATRA_SUCCESS;
}

/* Delete checkpoint */
int katra_checkpoint_delete(const char* checkpoint_id) {
    if (!checkpoint_id) {
        return E_INPUT_NULL;
    }

    char filepath[KATRA_PATH_MAX];
    int result = katra_checkpoint_get_path_internal(checkpoint_id, filepath, sizeof(filepath));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check if file exists */
    if (access(filepath, F_OK) != 0) {
        return E_CHECKPOINT_NOT_FOUND;
    }

    /* Delete file */
    if (unlink(filepath) != 0) {
        katra_report_error(E_SYSTEM_FILE, "katra_checkpoint_delete",
                          "Failed to delete %s", filepath);
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Checkpoint deleted: %s", checkpoint_id);
    return KATRA_SUCCESS;
}
