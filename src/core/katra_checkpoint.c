/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

/* Project includes */
#include "katra_checkpoint.h"
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_env_utils.h"
#include "katra_path_utils.h"
#include "katra_json_utils.h"
#include "katra_file_utils.h"

/* Checkpoint directory and file format */
#define CHECKPOINT_DIR_FORMAT "%s/.katra/checkpoints"
#define CHECKPOINT_FILE_FORMAT "%s/.katra/checkpoints/checkpoint_%s_%ld.kcp"
#define CHECKPOINT_VERSION "1.0.0"
#define CHECKPOINT_MAGIC "KATRA_CHECKPOINT_V1"

/* Forward declarations */
static int get_checkpoint_dir(char* buffer, size_t size);
static int get_checkpoint_path(const char* checkpoint_id, char* buffer, size_t size);
static int generate_checkpoint_id(const char* ci_id, char* buffer, size_t size);
static int write_checkpoint_header(FILE* fp, const checkpoint_metadata_t* metadata);
static int read_checkpoint_header(FILE* fp, checkpoint_metadata_t* metadata);
static int calculate_checksum(const char* filepath, char* checksum, size_t size);
static int compare_version(const char* v1, const char* v2);

/* Get checkpoint directory path */
static int get_checkpoint_dir(char* buffer, size_t size) {
    return katra_build_path(buffer, size, "checkpoints", NULL);
}

/* Get full path for checkpoint file */
static int get_checkpoint_path(const char* checkpoint_id, char* buffer, size_t size) {
    if (!checkpoint_id) {
        return E_INPUT_NULL;
    }

    /* Parse checkpoint_id to extract components: ci_id_timestamp */
    /* Find the LAST underscore to separate CI ID from timestamp */
    const char* last_underscore = strrchr(checkpoint_id, '_');
    if (!last_underscore) {
        return E_INPUT_FORMAT;
    }

    /* Extract CI ID (everything before last underscore) */
    size_t ci_id_len = last_underscore - checkpoint_id;
    if (ci_id_len == 0 || ci_id_len >= KATRA_BUFFER_MEDIUM) {
        return E_INPUT_FORMAT;
    }

    char ci_id[KATRA_BUFFER_MEDIUM];
    strncpy(ci_id, checkpoint_id, ci_id_len);
    ci_id[ci_id_len] = '\0';

    /* Extract timestamp (everything after last underscore) */
    long timestamp = atol(last_underscore + 1);
    if (timestamp == 0) {
        return E_INPUT_FORMAT;
    }

    /* Build path: ~/.katra/checkpoints/checkpoint_{ci_id}_{timestamp}.kcp */
    char checkpoint_dir[KATRA_PATH_MAX];
    int result = katra_build_path(checkpoint_dir, sizeof(checkpoint_dir), "checkpoints", NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    snprintf(buffer, size, "%s/checkpoint_%s_%ld.kcp", checkpoint_dir, ci_id, timestamp);
    return KATRA_SUCCESS;
}

/* Generate unique checkpoint ID */
static int generate_checkpoint_id(const char* ci_id, char* buffer, size_t size) {
    if (!ci_id || !buffer) {
        return E_INPUT_NULL;
    }

    time_t now = time(NULL);
    snprintf(buffer, size, "%s_%ld", ci_id, (long)now);
    return KATRA_SUCCESS;
}

/* Write checkpoint header as JSON */
static int write_checkpoint_header(FILE* fp, const checkpoint_metadata_t* metadata) {
    if (!fp || !metadata) {
        return E_INPUT_NULL;
    }

    /* Write magic string */
    fprintf(fp, "%s\n", CHECKPOINT_MAGIC);

    /* Write metadata as JSON */
    fprintf(fp, "{\n");
    fprintf(fp, "  \"checkpoint_id\": \"%s\",\n", metadata->checkpoint_id);
    fprintf(fp, "  \"ci_id\": \"%s\",\n", metadata->ci_id);
    fprintf(fp, "  \"timestamp\": %ld,\n", (long)metadata->timestamp);
    fprintf(fp, "  \"version\": \"%s\",\n", metadata->version);
    fprintf(fp, "  \"record_count\": %zu,\n", metadata->record_count);
    fprintf(fp, "  \"tier1_records\": %zu,\n", metadata->tier1_records);
    fprintf(fp, "  \"tier2_records\": %zu,\n", metadata->tier2_records);
    fprintf(fp, "  \"tier3_records\": %zu,\n", metadata->tier3_records);
    fprintf(fp, "  \"compressed\": %s,\n", metadata->compressed ? "true" : "false");
    fprintf(fp, "  \"notes\": \"%s\"\n", metadata->notes);
    fprintf(fp, "}\n");
    fprintf(fp, "---RECORDS---\n");

    return KATRA_SUCCESS;
}

/* Read checkpoint header */
static int read_checkpoint_header(FILE* fp, checkpoint_metadata_t* metadata) {
    if (!fp || !metadata) {
        return E_INPUT_NULL;
    }

    char line[KATRA_BUFFER_LARGE];

    /* Read and verify magic string */
    if (!fgets(line, sizeof(line), fp)) {
        return E_CHECKPOINT_INVALID;
    }

    /* Remove newline */
    line[strcspn(line, "\n")] = '\0';

    if (strcmp(line, CHECKPOINT_MAGIC) != 0) {
        return E_CHECKPOINT_INVALID;
    }

    /* Read metadata JSON (simplified parsing for MVP) */
    /* In production, use proper JSON parser */

    while (fgets(line, sizeof(line), fp)) {
        /* Remove newline */
        line[strcspn(line, "\n")] = '\0';

        /* Check for end of header */
        if (strcmp(line, "---RECORDS---") == 0) {
            break;
        }

        /* Parse key-value pairs (simplified) */
        if (strstr(line, "\"checkpoint_id\"")) {
            sscanf(line, "  \"checkpoint_id\": \"%255[^\"]\"", metadata->checkpoint_id);
        } else if (strstr(line, "\"ci_id\"")) {
            sscanf(line, "  \"ci_id\": \"%255[^\"]\"", metadata->ci_id);
        } else if (strstr(line, "\"timestamp\"")) {
            long ts;
            sscanf(line, "  \"timestamp\": %ld", &ts);
            metadata->timestamp = (time_t)ts;
        } else if (strstr(line, "\"version\"")) {
            sscanf(line, "  \"version\": \"%63[^\"]\"", metadata->version);
        } else if (strstr(line, "\"record_count\"")) {
            sscanf(line, "  \"record_count\": %zu", &metadata->record_count);
        } else if (strstr(line, "\"tier1_records\"")) {
            sscanf(line, "  \"tier1_records\": %zu", &metadata->tier1_records);
        } else if (strstr(line, "\"notes\"")) {
            sscanf(line, "  \"notes\": \"%511[^\"]\"", metadata->notes);
        }
    }

    return KATRA_SUCCESS;
}

/* Calculate simple checksum (TODO: Use SHA-256 for production) */
static int calculate_checksum(const char* filepath, char* checksum, size_t size) {
    if (!filepath || !checksum) {
        return E_INPUT_NULL;
    }

    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        return E_SYSTEM_FILE;
    }

    /* Simple checksum for MVP - sum of all bytes */
    unsigned long sum = 0;
    int c;
    while ((c = fgetc(fp)) != EOF) {
        sum += c;
    }

    fclose(fp);

    snprintf(checksum, size, "%016lx", sum);
    return KATRA_SUCCESS;
}

/* Compare version strings */
static int compare_version(const char* v1, const char* v2) {
    (void)v1;  /* TODO: Implement proper version comparison */
    (void)v2;

    /* For MVP, all versions compatible */
    return 0;  /* Equal */
}

/* Initialize checkpoint subsystem */
int katra_checkpoint_init(void) {
    int result = KATRA_SUCCESS;
    char checkpoint_dir[KATRA_PATH_MAX];

    /* Build and create directory structure */
    result = katra_build_and_ensure_dir(checkpoint_dir, sizeof(checkpoint_dir),
                                        "checkpoints", NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    LOG_DEBUG("Initializing checkpoint system: %s", checkpoint_dir);
    LOG_INFO("Checkpoint system initialized");
    return KATRA_SUCCESS;
}

/* Save checkpoint */
int katra_checkpoint_save(const checkpoint_save_options_t* options,
                          char** checkpoint_id) {
    int result = KATRA_SUCCESS;
    FILE* fp = NULL;
    char filepath[KATRA_PATH_MAX];
    char id_buffer[KATRA_BUFFER_MEDIUM];
    checkpoint_metadata_t metadata;

    if (!options || !checkpoint_id) {
        katra_report_error(E_INPUT_NULL, "katra_checkpoint_save",
                          "options or checkpoint_id is NULL");
        result = E_INPUT_NULL;
        goto cleanup;
    }

    if (!options->ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_checkpoint_save", "ci_id is NULL");
        result = E_INPUT_NULL;
        goto cleanup;
    }

    LOG_INFO("Creating checkpoint for CI: %s", options->ci_id);

    /* Generate checkpoint ID */
    result = generate_checkpoint_id(options->ci_id, id_buffer, sizeof(id_buffer));
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_checkpoint_save", "Failed to generate ID");
        goto cleanup;
    }

    /* Get file path */
    result = get_checkpoint_path(id_buffer, filepath, sizeof(filepath));
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_checkpoint_save", "Failed to get path");
        goto cleanup;
    }

    /* Initialize metadata */
    memset(&metadata, 0, sizeof(metadata));
    strncpy(metadata.checkpoint_id, id_buffer, sizeof(metadata.checkpoint_id) - 1);
    strncpy(metadata.ci_id, options->ci_id, sizeof(metadata.ci_id) - 1);
    strncpy(metadata.version, CHECKPOINT_VERSION, sizeof(metadata.version) - 1);
    metadata.timestamp = time(NULL);
    metadata.compressed = options->compress;

    if (options->notes) {
        strncpy(metadata.notes, options->notes, sizeof(metadata.notes) - 1);
    }

    /* Get memory statistics to fill record counts */
    memory_stats_t stats;
    result = katra_memory_stats(options->ci_id, &stats);
    if (result == KATRA_SUCCESS) {
        metadata.record_count = stats.total_records;
        metadata.tier1_records = stats.tier1_records;
        metadata.tier2_records = stats.tier2_records;
        metadata.tier3_records = stats.tier3_records;
    }

    /* Open checkpoint file */
    fp = fopen(filepath, "w");
    if (!fp) {
        katra_report_error(E_SYSTEM_FILE, "katra_checkpoint_save",
                          "Failed to open %s", filepath);
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    /* Write header */
    result = write_checkpoint_header(fp, &metadata);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_checkpoint_save", "Failed to write header");
        goto cleanup;
    }

    /* TODO: Write memory records (query Tier 1, write as JSONL) */
    /* For MVP, we just create the checkpoint structure */

    fclose(fp);
    fp = NULL;

    /* Calculate checksum */
    calculate_checksum(filepath, metadata.checksum, sizeof(metadata.checksum));

    /* Get file size */
    size_t file_size = 0;
    if (katra_file_get_size(filepath, &file_size) == KATRA_SUCCESS) {
        metadata.file_size = file_size;
    }

    /* Return checkpoint ID */
    *checkpoint_id = strdup(id_buffer);
    if (!*checkpoint_id) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    LOG_INFO("Checkpoint saved: %s (%zu bytes, %zu records)",
             id_buffer, metadata.file_size, metadata.record_count);

cleanup:
    if (fp) {
        fclose(fp);
    }
    return result;
}

/* Load checkpoint */
int katra_checkpoint_load(const char* checkpoint_id, const char* ci_id) {
    int result = KATRA_SUCCESS;
    FILE* fp = NULL;
    char filepath[KATRA_PATH_MAX];
    checkpoint_metadata_t metadata;

    if (!checkpoint_id || !ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_checkpoint_load",
                          "checkpoint_id or ci_id is NULL");
        return E_INPUT_NULL;
    }

    LOG_INFO("Loading checkpoint: %s for CI: %s", checkpoint_id, ci_id);

    /* Get file path */
    result = get_checkpoint_path(checkpoint_id, filepath, sizeof(filepath));
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_checkpoint_load", "Failed to get path");
        goto cleanup;
    }

    /* Check if file exists */
    if (access(filepath, F_OK) != 0) {
        katra_report_error(E_CHECKPOINT_NOT_FOUND, "katra_checkpoint_load",
                          "Checkpoint not found: %s", checkpoint_id);
        result = E_CHECKPOINT_NOT_FOUND;
        goto cleanup;
    }

    /* Validate checkpoint first */
    result = katra_checkpoint_validate(checkpoint_id);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Open checkpoint file */
    fp = fopen(filepath, "r");
    if (!fp) {
        katra_report_error(E_SYSTEM_FILE, "katra_checkpoint_load",
                          "Failed to open %s", filepath);
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    /* Read header */
    memset(&metadata, 0, sizeof(metadata));
    result = read_checkpoint_header(fp, &metadata);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_checkpoint_load", "Failed to read header");
        goto cleanup;
    }

    /* Verify CI ID matches */
    if (strcmp(metadata.ci_id, ci_id) != 0) {
        katra_report_error(E_INPUT_INVALID, "katra_checkpoint_load",
                          "CI ID mismatch: expected %s, got %s", ci_id, metadata.ci_id);
        result = E_INPUT_INVALID;
        goto cleanup;
    }

    /* TODO: Read and restore memory records */
    /* For MVP, we just validate the checkpoint can be read */

    LOG_INFO("Checkpoint loaded: %zu records", metadata.record_count);

cleanup:
    if (fp) {
        fclose(fp);
    }
    return result;
}

/* Validate checkpoint */
int katra_checkpoint_validate(const char* checkpoint_id) {
    int result = KATRA_SUCCESS;
    FILE* fp = NULL;
    char filepath[KATRA_PATH_MAX];
    checkpoint_metadata_t metadata;

    if (!checkpoint_id) {
        return E_INPUT_NULL;
    }

    /* Get file path */
    result = get_checkpoint_path(checkpoint_id, filepath, sizeof(filepath));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check if file exists */
    if (access(filepath, F_OK) != 0) {
        return E_CHECKPOINT_NOT_FOUND;
    }

    /* Open file */
    fp = fopen(filepath, "r");
    if (!fp) {
        return E_SYSTEM_FILE;
    }

    /* Read and validate header */
    memset(&metadata, 0, sizeof(metadata));
    result = read_checkpoint_header(fp, &metadata);
    fclose(fp);

    if (result != KATRA_SUCCESS) {
        return E_CHECKPOINT_INVALID;
    }

    /* Check version compatibility */
    if (compare_version(metadata.version, CHECKPOINT_VERSION) != 0) {
        LOG_WARN("Checkpoint version mismatch: %s vs %s",
                 metadata.version, CHECKPOINT_VERSION);
        /* For MVP, we'll accept it anyway */
    }

    /* TODO: Verify checksum */

    return KATRA_SUCCESS;
}

/* Get checkpoint metadata */
int katra_checkpoint_get_metadata(const char* checkpoint_id,
                                   checkpoint_metadata_t* metadata) {
    int result = KATRA_SUCCESS;
    FILE* fp = NULL;
    char filepath[KATRA_PATH_MAX];

    if (!checkpoint_id || !metadata) {
        return E_INPUT_NULL;
    }

    /* Get file path */
    result = get_checkpoint_path(checkpoint_id, filepath, sizeof(filepath));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check if file exists */
    if (access(filepath, F_OK) != 0) {
        return E_CHECKPOINT_NOT_FOUND;
    }

    /* Open file */
    fp = fopen(filepath, "r");
    if (!fp) {
        return E_SYSTEM_FILE;
    }

    /* Read header */
    memset(metadata, 0, sizeof(*metadata));
    result = read_checkpoint_header(fp, metadata);
    if (result != KATRA_SUCCESS) {
        fclose(fp);
        return result;
    }

    fclose(fp);

    /* Get file size */
    size_t file_size = 0;
    if (katra_file_get_size(filepath, &file_size) == KATRA_SUCCESS) {
        metadata->file_size = file_size;
    }

    return KATRA_SUCCESS;
}

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
    int result = get_checkpoint_dir(checkpoint_dir, sizeof(checkpoint_dir));
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
        if (strstr(entry->d_name, ".kcp")) {
            /* Count all .kcp files (we'll filter during population) */
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
        if (strstr(entry->d_name, ".kcp")) {
            /* Extract checkpoint ID from filename: checkpoint_CI_ID_TIMESTAMP.kcp */
            /* Remove "checkpoint_" prefix and ".kcp" suffix */
            if (strncmp(entry->d_name, "checkpoint_", 11) != 0) {
                continue;
            }

            const char* id_part = entry->d_name + 11;  /* Skip "checkpoint_" */
            size_t id_len = strlen(id_part);
            if (id_len < 5) continue;  /* Need at least ".kcp" */

            /* Remove ".kcp" suffix */
            char checkpoint_id[KATRA_BUFFER_MEDIUM];
            strncpy(checkpoint_id, id_part, sizeof(checkpoint_id) - 1);
            checkpoint_id[sizeof(checkpoint_id) - 1] = '\0';

            /* Remove .kcp extension */
            char* ext = strstr(checkpoint_id, ".kcp");
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
    int result = get_checkpoint_path(checkpoint_id, filepath, sizeof(filepath));
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

/* Cleanup checkpoint subsystem */
void katra_checkpoint_cleanup(void) {
    LOG_DEBUG("Checkpoint cleanup complete");
    /* No persistent state to cleanup in current implementation */
}
