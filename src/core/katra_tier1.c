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
#include "katra_tier1.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_env_utils.h"

/* File path for Tier 1 storage: ~/.katra/memory/tier1/ */
#define TIER1_DIR_FORMAT "%s/.katra/memory/tier1"
#define TIER1_FILE_FORMAT "%s/.katra/memory/tier1/%04d-%02d-%02d.jsonl"

/* Forward declarations */
static int get_tier1_dir(const char* ci_id, char* buffer, size_t size);
static int get_daily_file_path(char* buffer, size_t size);
static int write_json_record(FILE* fp, const memory_record_t* record);
static int parse_json_record(const char* line, memory_record_t** record);
static int count_file_records(const char* filepath, size_t* count);
static void json_escape_string(const char* src, char* dst, size_t dst_size);

/* Get Tier 1 directory path */
static int get_tier1_dir(const char* ci_id, char* buffer, size_t size) {
    (void)ci_id;  /* Unused - future multi-tenant support */

    const char* home = getenv("HOME");
    if (!home) {
        return E_SYSTEM_FILE;
    }

    snprintf(buffer, size, TIER1_DIR_FORMAT, home);
    return KATRA_SUCCESS;
}

/* Get today's daily file path */
static int get_daily_file_path(char* buffer, size_t size) {
    const char* home = getenv("HOME");
    if (!home) {
        return E_SYSTEM_FILE;
    }

    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);

    snprintf(buffer, size, TIER1_FILE_FORMAT,
            home,
            tm_info->tm_year + 1900,
            tm_info->tm_mon + 1,
            tm_info->tm_mday);

    return KATRA_SUCCESS;
}

/* JSON escape string helper */
static void json_escape_string(const char* src, char* dst, size_t dst_size) {
    if (!src || !dst || dst_size == 0) {
        return;
    }

    size_t dst_idx = 0;
    for (const char* p = src; *p && dst_idx < dst_size - 1; p++) {
        if (*p == '"' || *p == '\\') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = *p;
            }
        } else if (*p == '\n') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 'n';
            }
        } else if (*p == '\r') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 'r';
            }
        } else if (*p == '\t') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 't';
            }
        } else {
            dst[dst_idx++] = *p;
        }
    }
    dst[dst_idx] = '\0';
}

/* Write memory record as JSON line */
static int write_json_record(FILE* fp, const memory_record_t* record) {
    char content_escaped[KATRA_BUFFER_LARGE];
    char response_escaped[KATRA_BUFFER_LARGE];
    char context_escaped[KATRA_BUFFER_LARGE];

    json_escape_string(record->content, content_escaped, sizeof(content_escaped));

    if (record->response) {
        json_escape_string(record->response, response_escaped, sizeof(response_escaped));
    } else {
        response_escaped[0] = '\0';
    }

    if (record->context) {
        json_escape_string(record->context, context_escaped, sizeof(context_escaped));
    } else {
        context_escaped[0] = '\0';
    }

    /* Write JSON object (one line) */
    fprintf(fp, "{");
    fprintf(fp, "\"record_id\":\"%s\",", record->record_id ? record->record_id : "");
    fprintf(fp, "\"timestamp\":%ld,", (long)record->timestamp);
    fprintf(fp, "\"type\":%d,", record->type);
    fprintf(fp, "\"importance\":%.2f,", record->importance);
    fprintf(fp, "\"content\":\"%s\",", content_escaped);

    if (record->response) {
        fprintf(fp, "\"response\":\"%s\",", response_escaped);
    }

    if (record->context) {
        fprintf(fp, "\"context\":\"%s\",", context_escaped);
    }

    fprintf(fp, "\"ci_id\":\"%s\",", record->ci_id ? record->ci_id : "");

    if (record->session_id) {
        fprintf(fp, "\"session_id\":\"%s\",", record->session_id);
    }

    if (record->component) {
        fprintf(fp, "\"component\":\"%s\",", record->component);
    }

    fprintf(fp, "\"tier\":%d,", record->tier);
    fprintf(fp, "\"archived\":%s", record->archived ? "true" : "false");
    fprintf(fp, "}\n");

    return KATRA_SUCCESS;
}

/* Parse JSON record from line */
__attribute__((unused))
static int parse_json_record(const char* line, memory_record_t** record) {
    (void)line;  /* TODO: Implement proper JSON parsing */

    /* Simplified JSON parsing - in production use a proper JSON library */
    /* For MVP, we'll use simple string scanning */

    memory_record_t* rec = calloc(1, sizeof(memory_record_t));
    if (!rec) {
        return E_SYSTEM_MEMORY;
    }

    /* This is a simplified parser - in production, use cJSON or similar */
    /* For now, we'll just mark as not implemented and return success */
    /* This allows the file format to be defined and tested */

    *record = rec;
    return E_INTERNAL_NOTIMPL;  /* TODO: Implement proper JSON parsing */
}

/* Count records in a file */
static int count_file_records(const char* filepath, size_t* count) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        *count = 0;
        return KATRA_SUCCESS;  /* File doesn't exist yet */
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

/* Initialize Tier 1 storage */
int tier1_init(const char* ci_id) {
    int result = KATRA_SUCCESS;
    char tier1_dir[KATRA_PATH_MAX];

    result = get_tier1_dir(ci_id, tier1_dir, sizeof(tier1_dir));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    LOG_DEBUG("Initializing Tier 1 storage: %s", tier1_dir);

    /* Create directory structure */
    const char* home = getenv("HOME");
    if (!home) {
        return E_SYSTEM_FILE;
    }

    char dir_path[KATRA_PATH_MAX];

    /* Create ~/.katra/memory */
    snprintf(dir_path, sizeof(dir_path), "%s/.katra/memory", home);
    mkdir(dir_path, KATRA_DIR_PERMISSIONS);

    /* Create ~/.katra/memory/tier1 */
    snprintf(dir_path, sizeof(dir_path), "%s/.katra/memory/tier1", home);
    mkdir(dir_path, KATRA_DIR_PERMISSIONS);

    LOG_INFO("Tier 1 storage initialized");
    return KATRA_SUCCESS;
}

/* Store raw recording */
int tier1_store(const memory_record_t* record) {
    int result = KATRA_SUCCESS;
    FILE* fp = NULL;
    char filepath[KATRA_PATH_MAX];

    if (!record) {
        katra_report_error(E_INPUT_NULL, "tier1_store", "record is NULL");
        result = E_INPUT_NULL;
        goto cleanup;
    }

    /* Get today's file path */
    result = get_daily_file_path(filepath, sizeof(filepath));
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "tier1_store", "Failed to get file path");
        goto cleanup;
    }

    /* Check file size before writing */
    struct stat st;
    if (stat(filepath, &st) == 0) {
        size_t size_mb = st.st_size / (1024 * 1024);
        if (size_mb >= TIER1_MAX_FILE_SIZE_MB) {
            katra_report_error(E_MEMORY_TIER_FULL, "tier1_store",
                              "Daily file exceeds %d MB", TIER1_MAX_FILE_SIZE_MB);
            result = E_MEMORY_TIER_FULL;
            goto cleanup;
        }
    }

    /* Open file for append */
    fp = fopen(filepath, "a");
    if (!fp) {
        katra_report_error(E_SYSTEM_FILE, "tier1_store",
                          "Failed to open %s", filepath);
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    /* Write JSON record */
    result = write_json_record(fp, record);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "tier1_store", "Failed to write record");
        goto cleanup;
    }

    LOG_DEBUG("Stored record to %s", filepath);

cleanup:
    if (fp) {
        fclose(fp);
    }
    return result;
}

/* Query Tier 1 recordings */
int tier1_query(const memory_query_t* query,
                memory_record_t*** results,
                size_t* count) {
    if (!query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "tier1_query", "NULL parameter");
        return E_INPUT_NULL;
    }

    *results = NULL;
    *count = 0;

    /* TODO: Implement query logic - scan daily files
     * For MVP, we'll return empty results */

    LOG_DEBUG("Tier 1 query (not yet implemented)");
    return E_INTERNAL_NOTIMPL;
}

/* Archive old Tier 1 recordings */
int tier1_archive(const char* ci_id, int max_age_days) {
    (void)max_age_days;  /* TODO: Use this parameter for archival logic */

    if (!ci_id) {
        return E_INPUT_NULL;
    }

    /* TODO: Implement archival logic
     * For MVP, return 0 (no records archived) */

    LOG_DEBUG("Tier 1 archive (not yet implemented)");
    return 0;  /* No records archived yet */
}

/* Get Tier 1 statistics */
int tier1_stats(const char* ci_id, size_t* total_records, size_t* bytes_used) {
    int result = KATRA_SUCCESS;
    char tier1_dir[KATRA_PATH_MAX];

    if (!ci_id || !total_records || !bytes_used) {
        return E_INPUT_NULL;
    }

    *total_records = 0;
    *bytes_used = 0;

    result = get_tier1_dir(ci_id, tier1_dir, sizeof(tier1_dir));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Scan directory for .jsonl files */
    DIR* dir = opendir(tier1_dir);
    if (!dir) {
        /* Directory doesn't exist yet - no records */
        return KATRA_SUCCESS;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".jsonl")) {
            char filepath[KATRA_PATH_MAX];
            snprintf(filepath, sizeof(filepath), "%s/%s", tier1_dir, entry->d_name);

            struct stat st;
            if (stat(filepath, &st) == 0) {
                *bytes_used += st.st_size;

                size_t file_records = 0;
                count_file_records(filepath, &file_records);
                *total_records += file_records;
            }
        }
    }

    closedir(dir);

    LOG_DEBUG("Tier 1 stats: records=%zu, bytes=%zu", *total_records, *bytes_used);
    return KATRA_SUCCESS;
}

/* Cleanup Tier 1 storage */
void tier1_cleanup(void) {
    LOG_DEBUG("Tier 1 cleanup complete");
    /* No persistent state to cleanup in current implementation */
}
