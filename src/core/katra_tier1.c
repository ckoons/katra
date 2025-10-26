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
#include "katra_tier2.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_json_utils.h"
#include "katra_file_utils.h"
#include "katra_strings.h"

/* File path for Tier 1 storage: ~/.katra/memory/tier1/ */
#define TIER1_DIR_FORMAT "%s/.katra/memory/tier1"
#define TIER1_FILE_FORMAT "%s/.katra/memory/tier1/%04d-%02d-%02d.jsonl"

/* Forward declarations */
static int get_daily_file_path(char* buffer, size_t size);
static int write_json_record(FILE* fp, const memory_record_t* record);
static void sort_filenames_desc(char** filenames, size_t count);
void tier1_free_filenames(char** filenames, size_t count);
static bool record_matches_query(const memory_record_t* record, const memory_query_t* query);
static int scan_file_for_records(const char* filepath, const memory_query_t* query,
                                  memory_record_t*** result_array, size_t* result_count,
                                  size_t* result_capacity);

/* Get Tier 1 directory path (exported for archive module) */
int tier1_get_dir(const char* ci_id, char* buffer, size_t size) {
    (void)ci_id;  /* Unused - future multi-tenant support */

    return katra_build_path(buffer, size, KATRA_DIR_MEMORY, KATRA_DIR_TIER1, NULL);
}

/* Get today's daily file path */
static int get_daily_file_path(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);

    char tier1_dir[KATRA_PATH_MAX];
    int result = katra_build_path(tier1_dir, sizeof(tier1_dir), KATRA_DIR_MEMORY, KATRA_DIR_TIER1, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    snprintf(buffer, size, "%s/%04d-%02d-%02d.jsonl",
            tier1_dir,
            tm_info->tm_year + 1900,
            tm_info->tm_mon + 1,
            tm_info->tm_mday);

    return KATRA_SUCCESS;
}

/* Write memory record as JSON line */
static int write_json_record(FILE* fp, const memory_record_t* record) {
    char content_escaped[KATRA_BUFFER_LARGE];
    char response_escaped[KATRA_BUFFER_LARGE];
    char context_escaped[KATRA_BUFFER_LARGE];

    katra_json_escape(record->content, content_escaped, sizeof(content_escaped));

    if (record->response) {
        katra_json_escape(record->response, response_escaped, sizeof(response_escaped));
    } else {
        response_escaped[0] = '\0';
    }

    if (record->context) {
        katra_json_escape(record->context, context_escaped, sizeof(context_escaped));
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

/* Initialize Tier 1 storage */
int tier1_init(const char* ci_id) {
    int result = KATRA_SUCCESS;
    char tier1_dir[KATRA_PATH_MAX];

    /* Build and create directory structure */
    result = katra_build_and_ensure_dir(tier1_dir, sizeof(tier1_dir), KATRA_DIR_MEMORY, KATRA_DIR_TIER1, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    (void)ci_id;  /* Unused - future multi-tenant support */

    LOG_DEBUG("Initializing Tier 1 storage: %s", tier1_dir);
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
    fp = fopen(filepath, KATRA_FILE_MODE_APPEND);
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

/* Collect .jsonl files from directory (exported for archive module) */
int tier1_collect_jsonl_files(const char* tier1_dir, char*** filenames, size_t* count) {
    DIR* dir = opendir(tier1_dir);
    if (!dir) {
        *filenames = NULL;
        *count = 0;
        return KATRA_SUCCESS;  /* No files yet */
    }

    char** files = NULL;
    size_t file_count = 0;
    size_t capacity = 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        size_t name_len = strlen(entry->d_name);
        if (name_len < 6 || strcmp(entry->d_name + name_len - 6, ".jsonl") != 0) {
            continue;
        }

        if (file_count >= capacity) {
            size_t new_cap = capacity == 0 ? 32 : capacity * 2;
            char** new_array = realloc(files, new_cap * sizeof(char*));
            if (!new_array) {
                tier1_free_filenames(files, file_count);
                closedir(dir);
                return E_SYSTEM_MEMORY;
            }
            files = new_array;
            capacity = new_cap;
        }

        files[file_count] = strdup(entry->d_name);
        if (!files[file_count]) {
            tier1_free_filenames(files, file_count);
            closedir(dir);
            return E_SYSTEM_MEMORY;
        }
        file_count++;
    }
    closedir(dir);

    *filenames = files;
    *count = file_count;
    return KATRA_SUCCESS;
}

/* Sort filenames in descending order (newest first) */
static void sort_filenames_desc(char** filenames, size_t count) {
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (strcmp(filenames[i], filenames[j]) < 0) {
                char* temp = filenames[i];
                filenames[i] = filenames[j];
                filenames[j] = temp;
            }
        }
    }
}

/* Free filename array (exported for archive module) */
void tier1_free_filenames(char** filenames, size_t count) {
    if (!filenames) return;
    for (size_t i = 0; i < count; i++) {
        free(filenames[i]);
    }
    free(filenames);
}

/* Helper: Check if record matches query */
static bool record_matches_query(const memory_record_t* record,
                                  const memory_query_t* query) {
    /* Check CI ID (required) */
    if (query->ci_id && record->ci_id) {
        if (strcmp(record->ci_id, query->ci_id) != 0) {
            return false;
        }
    }

    /* Check time range */
    if (query->start_time > 0 && record->timestamp < query->start_time) {
        return false;
    }
    if (query->end_time > 0 && record->timestamp > query->end_time) {
        return false;
    }

    /* Check type filter */
    if (query->type > 0 && record->type != query->type) {
        return false;
    }

    /* Check importance filter */
    if (record->importance < query->min_importance) {
        return false;
    }

    return true;
}

/* Helper: Scan file for matching records
 * Returns: KATRA_SUCCESS (continue), 1 (limit reached), E_SYSTEM_MEMORY (error) */
static int scan_file_for_records(const char* filepath, const memory_query_t* query,
                                  memory_record_t*** result_array, size_t* result_count,
                                  size_t* result_capacity) {
    FILE* fp = fopen(filepath, KATRA_FILE_MODE_READ);
    if (!fp) {
        return KATRA_SUCCESS;  /* Skip unreadable files */
    }

    char line[KATRA_BUFFER_LARGE];
    while (fgets(line, sizeof(line), fp)) {  /* GUIDELINE_APPROVED: fgets in while condition */
        /* Remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        /* Parse record */
        memory_record_t* record = NULL;
        if (katra_tier1_parse_json_record(line, &record) != KATRA_SUCCESS || !record) {
            continue;
        }

        /* Check if matches query */
        if (!record_matches_query(record, query)) {
            katra_memory_free_record(record);
            continue;
        }

        /* Add to results */
        if (*result_count >= *result_capacity) {
            size_t new_capacity = *result_capacity == 0 ? 32 : *result_capacity * 2;
            memory_record_t** new_array = realloc(*result_array,
                                                  new_capacity * sizeof(memory_record_t*));
            if (!new_array) {
                katra_memory_free_record(record);
                fclose(fp);
                return E_SYSTEM_MEMORY;
            }
            *result_array = new_array;
            *result_capacity = new_capacity;
        }

        (*result_array)[(*result_count)++] = record;

        /* Check limit */
        if (query->limit > 0 && *result_count >= query->limit) {
            fclose(fp);
            return 1;  /* Signal limit reached */
        }
    }

    fclose(fp);
    return KATRA_SUCCESS;
}

/* Query Tier 1 recordings */
int tier1_query(const memory_query_t* query,
                memory_record_t*** results,
                size_t* count) {
    int result = KATRA_SUCCESS;
    memory_record_t** result_array = NULL;
    size_t result_count = 0;
    size_t result_capacity = 0;
    char** filenames = NULL;
    size_t file_count = 0;

    if (!query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "tier1_query", "NULL parameter");
        return E_INPUT_NULL;
    }

    *results = NULL;
    *count = 0;

    /* Get Tier 1 directory */
    char tier1_dir[KATRA_PATH_MAX];
    result = tier1_get_dir(query->ci_id, tier1_dir, sizeof(tier1_dir));
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Collect and sort filenames */
    result = tier1_collect_jsonl_files(tier1_dir, &filenames, &file_count);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    if (file_count == 0) {
        result = KATRA_SUCCESS;
        goto cleanup;
    }

    sort_filenames_desc(filenames, file_count);

    /* Scan files */
    for (size_t f = 0; f < file_count; f++) {
        char filepath[KATRA_PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/%s", tier1_dir, filenames[f]);

        result = scan_file_for_records(filepath, query, &result_array,
                                        &result_count, &result_capacity);
        if (result == E_SYSTEM_MEMORY) {
            goto cleanup;
        } else if (result == 1) {
            /* Limit reached */
            break;
        }
    }

    tier1_free_filenames(filenames, file_count);
    *results = result_array;
    *count = result_count;
    LOG_DEBUG("Tier 1 query returned %zu results", result_count);
    return KATRA_SUCCESS;

cleanup:
    if (result_array) {
        for (size_t i = 0; i < result_count; i++) {
            katra_memory_free_record(result_array[i]);
        }
        free(result_array);
    }
    tier1_free_filenames(filenames, file_count);
    return result;
}

/* Stats visitor context */
typedef struct {
    size_t total_records;
    size_t bytes_used;
} tier1_stats_context_t;

/* Stats visitor function */
static int tier1_stats_visitor(const char* filepath, void* userdata) {
    tier1_stats_context_t* ctx = (tier1_stats_context_t*)userdata;

    /* Get file size */
    size_t file_size = 0;
    if (katra_file_get_size(filepath, &file_size) == KATRA_SUCCESS) {
        ctx->bytes_used += file_size;
    }

    /* Count records */
    size_t file_records = 0;
    if (katra_file_count_lines(filepath, &file_records) == KATRA_SUCCESS) {
        ctx->total_records += file_records;
    }

    return KATRA_SUCCESS;
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

    result = tier1_get_dir(ci_id, tier1_dir, sizeof(tier1_dir));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Iterate over .jsonl files */
    tier1_stats_context_t ctx = {0, 0};
    result = katra_dir_foreach(tier1_dir, ".jsonl", tier1_stats_visitor, &ctx);

    /* If directory doesn't exist, that's okay - no records yet */
    if (result == E_SYSTEM_FILE) {
        result = KATRA_SUCCESS;
    }

    if (result == KATRA_SUCCESS) {
        *total_records = ctx.total_records;
        *bytes_used = ctx.bytes_used;
        LOG_DEBUG("Tier 1 stats: records=%zu, bytes=%zu", *total_records, *bytes_used);
    }

    return result;
}

/* Cleanup Tier 1 storage */
void tier1_cleanup(void) {
    LOG_DEBUG("Tier 1 cleanup complete");
    /* No persistent state to cleanup in current implementation */
}
