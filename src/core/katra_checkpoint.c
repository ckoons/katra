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
#include "katra_strings.h"

/* Checkpoint directory and file format */
#define CHECKPOINT_DIR_FORMAT "%s/.katra/checkpoints"
#define CHECKPOINT_FILE_FORMAT "%s/.katra/checkpoints/checkpoint_%s_%ld.kcp"

/* Forward declarations */
static int generate_checkpoint_id(const char* ci_id, char* buffer, size_t size);
static int write_checkpoint_header(FILE* fp, const checkpoint_metadata_t* metadata);
static int read_checkpoint_header(FILE* fp, checkpoint_metadata_t* metadata);
static int calculate_checksum(const char* filepath, char* checksum, size_t size);
static int compare_version(const char* v1, const char* v2);

/* Get checkpoint directory path (exposed for management module) */
int katra_checkpoint_get_dir_internal(char* buffer, size_t size) {
    return katra_build_path(buffer, size, KATRA_DIR_CHECKPOINTS, NULL);
}

/* Get full path for checkpoint file (exposed for management module) */
int katra_checkpoint_get_path_internal(const char* checkpoint_id, char* buffer, size_t size) {
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
    char* endptr;
    long timestamp = strtol(last_underscore + 1, &endptr, 10);
    if (endptr == last_underscore + 1 || *endptr != '\0' || timestamp == 0) {
        return E_INPUT_FORMAT;
    }

    /* Build path: ~/.katra/checkpoints/checkpoint_{ci_id}_{timestamp}.kcp */
    char checkpoint_dir[KATRA_PATH_MAX];
    int result = katra_build_path(checkpoint_dir, sizeof(checkpoint_dir), KATRA_DIR_CHECKPOINTS, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    snprintf(buffer, size, "%s/%s%s_%ld%s", checkpoint_dir,
            KATRA_CHECKPOINT_PREFIX, ci_id, timestamp, KATRA_CHECKPOINT_SUFFIX);
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
/* GUIDELINE_APPROVED: JSON field names are part of checkpoint file format */
static int write_checkpoint_header(FILE* fp, const checkpoint_metadata_t* metadata) {
    if (!fp || !metadata) {
        return E_INPUT_NULL;
    }

    /* Write magic string */
    fprintf(fp, "%s\n", KATRA_CHECKPOINT_MAGIC);

    /* Write metadata as JSON */
    fprintf(fp, "{\n"); /* GUIDELINE_APPROVED */
    fprintf(fp, "  \"checkpoint_id\": \"%s\",\n", metadata->checkpoint_id); /* GUIDELINE_APPROVED */
    fprintf(fp, "  \"ci_id\": \"%s\",\n", metadata->ci_id); /* GUIDELINE_APPROVED */
    fprintf(fp, "  \"timestamp\": %ld,\n", (long)metadata->timestamp); /* GUIDELINE_APPROVED */
    fprintf(fp, "  \"version\": \"%s\",\n", metadata->version); /* GUIDELINE_APPROVED */
    fprintf(fp, "  \"record_count\": %zu,\n", metadata->record_count); /* GUIDELINE_APPROVED */
    fprintf(fp, "  \"tier1_records\": %zu,\n", metadata->tier1_records); /* GUIDELINE_APPROVED */
    fprintf(fp, "  \"tier2_records\": %zu,\n", metadata->tier2_records); /* GUIDELINE_APPROVED */
    fprintf(fp, "  \"tier3_records\": %zu,\n", metadata->tier3_records); /* GUIDELINE_APPROVED */
    fprintf(fp, "  \"compressed\": %s,\n", metadata->compressed ? "true" : "false"); /* GUIDELINE_APPROVED */
    fprintf(fp, "  \"notes\": \"%s\"\n", metadata->notes); /* GUIDELINE_APPROVED */
    fprintf(fp, "}\n"); /* GUIDELINE_APPROVED */
    fprintf(fp, "%s\n", KATRA_CHECKPOINT_RECORD_SEPARATOR);

    return KATRA_SUCCESS;
}
/* GUIDELINE_APPROVED_END */

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
    line[strcspn(line, KATRA_LINE_TERMINATOR)] = '\0';

    if (strcmp(line, KATRA_CHECKPOINT_MAGIC) != 0) {
        return E_CHECKPOINT_INVALID;
    }

    /* Read metadata JSON (simplified parsing for MVP) */
    /* In production, use proper JSON parser */

    while (fgets(line, sizeof(line), fp)) {
        /* Remove newline */
        line[strcspn(line, KATRA_LINE_TERMINATOR)] = '\0';

        /* Check for end of header */
        if (strcmp(line, KATRA_CHECKPOINT_RECORD_SEPARATOR) == 0) {
            break;
        }

        /* Parse key-value pairs (simplified) */
        if (strstr(line, KATRA_JSON_FIELD_CHECKPOINT_ID)) {
            sscanf(line, "  \"checkpoint_id\": \"" SSCANF_FMT_MEDIUM "\"", metadata->checkpoint_id);
        } else if (strstr(line, KATRA_JSON_FIELD_CI_ID)) {
            sscanf(line, "  \"ci_id\": \"" SSCANF_FMT_MEDIUM "\"", metadata->ci_id);
        } else if (strstr(line, KATRA_JSON_FIELD_TIMESTAMP)) {
            long ts;
            sscanf(line, "  \"timestamp\": %ld", &ts);
            metadata->timestamp = (time_t)ts;
        } else if (strstr(line, KATRA_JSON_FIELD_VERSION)) {
            sscanf(line, "  \"version\": \"" SSCANF_FMT_SMALL "\"", metadata->version);
        } else if (strstr(line, KATRA_JSON_FIELD_RECORD_COUNT)) {
            sscanf(line, "  \"record_count\": %zu", &metadata->record_count);
        } else if (strstr(line, KATRA_JSON_FIELD_TIER1_RECORDS)) {
            sscanf(line, "  \"tier1_records\": %zu", &metadata->tier1_records);
        } else if (strstr(line, KATRA_JSON_FIELD_NOTES)) {
            sscanf(line, "  \"notes\": \"" SSCANF_FMT_NOTES "\"", metadata->notes);
        }
    }

    return KATRA_SUCCESS;
}

/* Calculate simple checksum (TODO: Use SHA-256 for production) */
static int calculate_checksum(const char* filepath, char* checksum, size_t size) {
    if (!filepath || !checksum) {
        return E_INPUT_NULL;
    }

    FILE* fp = fopen(filepath, KATRA_FILE_MODE_READ);
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
                                        KATRA_DIR_CHECKPOINTS, NULL);
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
    result = katra_checkpoint_get_path_internal(id_buffer, filepath, sizeof(filepath));
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_checkpoint_save", "Failed to get path");
        goto cleanup;
    }

    /* Initialize metadata */
    memset(&metadata, 0, sizeof(metadata));
    strncpy(metadata.checkpoint_id, id_buffer, sizeof(metadata.checkpoint_id) - 1);
    strncpy(metadata.ci_id, options->ci_id, sizeof(metadata.ci_id) - 1);
    strncpy(metadata.version, KATRA_CHECKPOINT_VERSION, sizeof(metadata.version) - 1);
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
    fp = fopen(filepath, KATRA_FILE_MODE_WRITE);
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
        katra_report_error(E_SYSTEM_MEMORY, "katra_checkpoint_save",
                          "Failed to allocate checkpoint ID");
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
    result = katra_checkpoint_get_path_internal(checkpoint_id, filepath, sizeof(filepath));
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
    fp = fopen(filepath, KATRA_FILE_MODE_READ);
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
    result = katra_checkpoint_get_path_internal(checkpoint_id, filepath, sizeof(filepath));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check if file exists */
    if (access(filepath, F_OK) != 0) {
        return E_CHECKPOINT_NOT_FOUND;
    }

    /* Open file */
    fp = fopen(filepath, KATRA_FILE_MODE_READ);
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
    if (compare_version(metadata.version, KATRA_CHECKPOINT_VERSION) != 0) {
        LOG_WARN("Checkpoint version mismatch: %s vs %s",
                 metadata.version, KATRA_CHECKPOINT_VERSION);
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
    result = katra_checkpoint_get_path_internal(checkpoint_id, filepath, sizeof(filepath));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check if file exists */
    if (access(filepath, F_OK) != 0) {
        return E_CHECKPOINT_NOT_FOUND;
    }

    /* Open file */
    fp = fopen(filepath, KATRA_FILE_MODE_READ);
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

/* Cleanup checkpoint subsystem */
void katra_checkpoint_cleanup(void) {
    LOG_DEBUG("Checkpoint cleanup complete");
    /* No persistent state to cleanup in current implementation */
}
