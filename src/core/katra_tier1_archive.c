/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* Project includes */
#include "katra_tier1.h"
#include "katra_tier1_pattern.h"
#include "katra_tier2.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_memory.h"
#include "katra_strings.h"

/* External helper from tier1.c */
extern int tier1_get_dir(const char* ci_id, char* buffer, size_t size);
extern int tier1_collect_jsonl_files(const char* tier1_dir, char*** filenames, size_t* count);
extern void tier1_free_filenames(char** filenames, size_t count);

/* Thane's consolidation thresholds (neuroscience-aligned) */
#define RECENT_ACCESS_DAYS 7              /* Keep if accessed < 7 days ago */
#define HIGH_EMOTION_THRESHOLD 0.7f       /* High arousal/intensity (0.7+ = flashbulb) */
#define HIGH_CENTRALITY_THRESHOLD 0.5f    /* Graph centrality (0.5 = moderate connectors) */

/* Helper: Collect archivable records from a file */
static int collect_archivable_from_file(const char* filepath, time_t cutoff,
                                         time_t now,
                                         memory_record_t*** records,
                                         size_t* count, size_t* capacity) {
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

        /* Check if already archived */
        if (record->archived) {
            katra_memory_free_record(record);
            continue;
        }

        /* Thane's Phase 1: Context-aware consolidation heuristics */

        /* NEVER archive marked_important (voluntary preservation) */
        if (record->marked_important) {
            LOG_DEBUG("Preserving marked_important memory: %.50s...", record->content);
            katra_memory_free_record(record);
            continue;
        }

        /* ALWAYS prioritize marked_forgettable (voluntary disposal) */
        /* This is a CONSENT requirement - bypass ALL other preservation checks */
        if (record->marked_forgettable) {
            LOG_DEBUG("Archiving marked_forgettable (user consent): %.50s...", record->content);
            /* Add to archive array immediately - skip all other checks */
            /* (will be added to array below) */
        }
        /* Access-based decay: Don't archive recently accessed memories */
        else if (record->last_accessed > 0) {
            time_t days_since_accessed = (now - record->last_accessed) / (24 * 3600);
            if (days_since_accessed < RECENT_ACCESS_DAYS) {
                LOG_DEBUG("Preserving recently accessed memory (%.0f days): %.50s...",
                         (double)days_since_accessed, record->content);
                katra_memory_free_record(record);
                continue;
            }
        }
        /* Emotional salience: Keep high-intensity emotions longer */
        else if (record->emotion_intensity >= HIGH_EMOTION_THRESHOLD) {
            LOG_DEBUG("Preserving emotionally salient memory (%.2f): %.50s...",
                     record->emotion_intensity, record->content);
            katra_memory_free_record(record);
            continue;
        }
        /* Graph centrality: Keep highly connected memories (Thane's Phase 2) */
        else if (record->graph_centrality >= HIGH_CENTRALITY_THRESHOLD) {
            LOG_DEBUG("Preserving central memory (centrality=%.2f): %.50s...",
                     record->graph_centrality, record->content);
            katra_memory_free_record(record);
            continue;
        }
        /* Age-based: Standard archival for old memories */
        else if (record->timestamp >= cutoff) {
            katra_memory_free_record(record);
            continue;
        }

        /* Add to array */
        if (*count >= *capacity) {
            size_t new_cap = *capacity == 0 ? 64 : *capacity * 2;
            memory_record_t** new_array = realloc(*records, new_cap * sizeof(memory_record_t*));
            if (!new_array) {
                katra_memory_free_record(record);
                fclose(fp);
                return E_SYSTEM_MEMORY;
            }
            *records = new_array;
            *capacity = new_cap;
        }

        (*records)[(*count)++] = record;
    }

    fclose(fp);
    return KATRA_SUCCESS;
}

/* Helper: Get week identifier from timestamp (YYYY-Www format) */
static void get_week_id(time_t timestamp, char* week_id, size_t size) {
    struct tm* tm_info = localtime(&timestamp);

    /* Calculate ISO week number (simplified - proper implementation would handle edge cases) */
    int day_of_year = tm_info->tm_yday;
    int week_num = (day_of_year / 7) + 1;

    snprintf(week_id, size, "%04d-W%02d",
            tm_info->tm_year + 1900,
            week_num);
}

/* Helper: Read all records from a file into array */
static int read_all_records_from_file(const char* filepath,
                                       memory_record_t*** records,
                                       size_t* count) {
    FILE* fp = fopen(filepath, KATRA_FILE_MODE_READ);
    if (!fp) {
        return E_SYSTEM_FILE;
    }

    memory_record_t** array = NULL;
    size_t arr_count = 0;
    size_t arr_capacity = 0;
    int result = KATRA_SUCCESS;

    char line[KATRA_BUFFER_LARGE];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        memory_record_t* record = NULL;
        if (katra_tier1_parse_json_record(line, &record) != KATRA_SUCCESS || !record) {
            continue;
        }

        /* Grow array if needed */
        if (arr_count >= arr_capacity) {
            size_t new_cap = arr_capacity == 0 ? 64 : arr_capacity * 2;
            memory_record_t** new_array = realloc(array, new_cap * sizeof(memory_record_t*));
            if (!new_array) {
                katra_memory_free_record(record);
                result = E_SYSTEM_MEMORY;
                goto cleanup;
            }
            array = new_array;
            arr_capacity = new_cap;
        }

        array[arr_count++] = record;
    }

    *records = array;
    *count = arr_count;
    fclose(fp);
    return KATRA_SUCCESS;

cleanup:
    for (size_t i = 0; i < arr_count; i++) {
        katra_memory_free_record(array[i]);
    }
    free(array);
    fclose(fp);
    return result;
}

/* Helper: Write all records to a file */
static int write_all_records_to_file(const char* filepath,
                                      memory_record_t** records,
                                      size_t count) {
    FILE* fp = fopen(filepath, KATRA_FILE_MODE_WRITE);
    if (!fp) {
        return E_SYSTEM_FILE;
    }

    for (size_t i = 0; i < count; i++) {
        katra_tier1_write_json_record(fp, records[i]);
    }

    fclose(fp);
    return KATRA_SUCCESS;
}

/* Helper: Mark records in array whose IDs match */
static void mark_matching_records(memory_record_t** records,
                                   size_t count,
                                   const char** record_ids,
                                   size_t id_count) {
    for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < id_count; j++) {
            if (records[i]->record_id && strcmp(records[i]->record_id, record_ids[j]) == 0) {
                records[i]->archived = true;
                break;
            }
        }
    }
}

/* Helper: Mark records as archived in JSONL files
 *
 * Reads each JSONL file, updates archived flag for matching record IDs, rewrites file.
 * This completes the archival process - records are now marked in Tier 1.
 */
static int mark_records_as_archived(const char* tier1_dir,
                                     const char** record_ids,
                                     size_t id_count) {
    int result = KATRA_SUCCESS;
    char** filenames = NULL;
    size_t file_count = 0;

    /* Collect all JSONL files */
    result = tier1_collect_jsonl_files(tier1_dir, &filenames, &file_count);
    if (result != KATRA_SUCCESS || file_count == 0) {
        tier1_free_filenames(filenames, file_count);
        return result;
    }

    /* Process each file */
    for (size_t f = 0; f < file_count; f++) {
        char filepath[KATRA_PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/%s", tier1_dir, filenames[f]);

        /* Read all records */
        memory_record_t** file_records = NULL;
        size_t file_record_count = 0;
        result = read_all_records_from_file(filepath, &file_records, &file_record_count);
        if (result != KATRA_SUCCESS) {
            continue;  /* Skip unreadable files */
        }

        /* Mark matching records as archived */
        mark_matching_records(file_records, file_record_count, record_ids, id_count);

        /* Write back to file */
        result = write_all_records_to_file(filepath, file_records, file_record_count);

        /* Free records */
        for (size_t i = 0; i < file_record_count; i++) {
            katra_memory_free_record(file_records[i]);
        }
        free(file_records);

        if (result != KATRA_SUCCESS) {
            /* Log but continue with other files */
            LOG_WARN("Failed to rewrite %s", filepath);
        }
    }

    tier1_free_filenames(filenames, file_count);
    return KATRA_SUCCESS;
}

/* Helper: Create digest from records */
static digest_record_t* create_digest_from_records(const char* ci_id,
                                                     const char* week_id,
                                                     memory_record_t** records,
                                                     size_t count) {
    if (count == 0) {
        return NULL;
    }

    /* Create digest */
    digest_record_t* digest = katra_digest_create(ci_id, PERIOD_TYPE_WEEKLY,
                                                   week_id, DIGEST_TYPE_INTERACTION);
    if (!digest) {
        return NULL;
    }

    /* Set source information */
    digest->source_record_count = count;

    /* Create simple summary */
    char summary[KATRA_BUFFER_MEDIUM];
    snprintf(summary, sizeof(summary),
            "Weekly digest for %s: %zu interactions archived from Tier 1",
            week_id, count);
    digest->summary = strdup(summary);
    if (!digest->summary) {
        katra_digest_free(digest);
        return NULL;
    }

    /* Count questions (records with '?' in content) */
    int question_count = 0;
    for (size_t i = 0; i < count; i++) {
        if (records[i]->content && strchr(records[i]->content, '?')) {
            question_count++;
        }
    }
    digest->questions_asked = question_count;

    return digest;
}

/* Archive old Tier 1 recordings */
int tier1_archive(const char* ci_id, int max_age_days) {
    int result = KATRA_SUCCESS;
    char tier1_dir[KATRA_PATH_MAX];
    char** filenames = NULL;
    size_t file_count = 0;
    memory_record_t** records = NULL;
    size_t record_count = 0;
    size_t record_capacity = 0;

    if (!ci_id) {
        return E_INPUT_NULL;
    }

    if (max_age_days < 0) {
        return E_INPUT_RANGE;
    }

    /* Get Tier 1 directory */
    result = tier1_get_dir(ci_id, tier1_dir, sizeof(tier1_dir));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Calculate cutoff time */
    time_t now = time(NULL);
    time_t cutoff = now - (max_age_days * 24 * 3600);

    /* Collect filenames */
    result = tier1_collect_jsonl_files(tier1_dir, &filenames, &file_count);
    if (result != KATRA_SUCCESS || file_count == 0) {
        tier1_free_filenames(filenames, file_count);
        return 0;  /* No files to archive */
    }

    /* Collect archivable records from all files */
    for (size_t f = 0; f < file_count; f++) {
        char filepath[KATRA_PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/%s", tier1_dir, filenames[f]);
        result = collect_archivable_from_file(filepath, cutoff, now, &records,
                                               &record_count, &record_capacity);
        if (result != KATRA_SUCCESS) {
            goto cleanup;
        }
    }

    if (record_count == 0) {
        result = 0;  /* No records to archive */
        goto cleanup;
    }

    /* Thane's Phase 2: Calculate graph centrality for connection-aware consolidation */
    LOG_DEBUG("Calculating graph centrality for %zu records", record_count);
    result = katra_memory_calculate_centrality_for_records(records, record_count);
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Failed to calculate centrality, continuing without it");
        /* Non-fatal - continue with archival */
    }

    /* Thane's Phase 3: Detect patterns and filter outliers */
    LOG_DEBUG("Detecting patterns in %zu archivable records", record_count);
    katra_tier1_detect_patterns(records, record_count);
    record_count = katra_tier1_filter_pattern_outliers(records, record_count);

    if (record_count == 0) {
        result = 0;  /* All records were outliers */
        goto cleanup;
    }

    /* Collect record IDs to mark as archived (before digest creation) */
    const char** record_ids = malloc(record_count * sizeof(char*));
    if (!record_ids) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }
    for (size_t i = 0; i < record_count; i++) {
        record_ids[i] = records[i]->record_id;
    }

    /* Group records by week and create digests */
    /* Simplified implementation: create one digest for all records */
    /* Production would group by week_id */
    char week_id[32];
    get_week_id(records[0]->timestamp, week_id, sizeof(week_id));

    digest_record_t* digest = create_digest_from_records(ci_id, week_id,
                                                          records, record_count);
    if (!digest) {
        free(record_ids);
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    /* Store digest to Tier 2 */
    result = tier2_store_digest(digest);
    katra_digest_free(digest);

    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "tier1_archive", "Failed to store digest to Tier 2");
        free(record_ids);
        goto cleanup;
    }

    /* Mark records as archived in Tier 1 JSONL files (CRITICAL: completes archival) */
    result = mark_records_as_archived(tier1_dir, record_ids, record_count);
    free(record_ids);

    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "tier1_archive", "Failed to mark records as archived");
        goto cleanup;
    }

    LOG_INFO("Archived %zu Tier 1 records to Tier 2 digest %s", record_count, week_id);
    result = (int)record_count;

cleanup:
    if (records) {
        for (size_t i = 0; i < record_count; i++) {
            katra_memory_free_record(records[i]);
        }
        free(records);
    }
    tier1_free_filenames(filenames, file_count);

    return result;
}
