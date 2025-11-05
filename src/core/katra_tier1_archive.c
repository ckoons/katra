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
#include "katra_core_common.h"

/* External helper from tier1.c */
extern int tier1_get_dir(const char* ci_id, char* buffer, size_t size);
extern int tier1_collect_jsonl_files(const char* tier1_dir, char*** filenames, size_t* count);

/* Thane's consolidation thresholds (neuroscience-aligned) */
#define RECENT_ACCESS_DAYS 7              /* Keep if accessed < 7 days ago */
#define HIGH_EMOTION_THRESHOLD 0.7f       /* High arousal/intensity (0.7+ = flashbulb) */
#define HIGH_CENTRALITY_THRESHOLD 0.5f    /* Graph centrality (0.5 = moderate connectors) */
#define PRESERVATION_SCORE_THRESHOLD 25.0f  /* Preserve if score >= 25 (multi-factor) */

/* Thane's Phase 4: Multi-factor scoring weights */
#define WEIGHT_RECENT_ACCESS 30.0f        /* Recent access contribution (0-30 points) */
#define WEIGHT_EMOTION 25.0f              /* Emotional salience (0-25 points) */
#define WEIGHT_CENTRALITY 20.0f           /* Graph centrality (0-20 points) */
#define WEIGHT_PATTERN_OUTLIER 15.0f      /* Pattern outlier bonus (15 points) */
#define WEIGHT_IMPORTANCE 10.0f           /* Base importance (0-10 points) */
#define AGE_PENALTY_START_DAYS 14         /* Start age penalty after 14 days */
#define AGE_PENALTY_PER_DAY 1.0f          /* Subtract 1 point per day > 14 days */

/* Helper: Get emotion type multiplier (Thane's Phase 4 Priority 2)
 *
 * Neuroscience rationale:
 * - Surprise: Novelty detection signal (enhanced consolidation)
 * - Fear: Threat relevance (survival priority)
 * - Satisfaction: Goal achieved (reduced need to remember)
 */
static float get_emotion_multiplier(const char* emotion_type) {
    if (!emotion_type) {
        return 1.0f;  /* Neutral */
    }

    /* Case-insensitive matching */
    char lower[KATRA_BUFFER_SMALL];
    size_t i;
    for (i = 0; emotion_type[i] && i < sizeof(lower) - 1; i++) {
        lower[i] = tolower((unsigned char)emotion_type[i]);
    }
    lower[i] = '\0';

    /* Emotion-specific multipliers */
    if (strcmp(lower, "surprise") == 0) {
        return 1.3f;  /* Novelty bonus */
    } else if (strcmp(lower, "fear") == 0) {
        return 1.5f;  /* Threat-relevance boost */
    } else if (strcmp(lower, "satisfaction") == 0) {
        return 0.8f;  /* Less consolidation needed */
    }

    return 1.0f;  /* Default for other emotions */
}

/* Helper: Calculate preservation score (Thane's Phase 4 Priority 3)
 *
 * Multi-factor scoring system replaces sequential if-else logic.
 * Memories "warm on multiple dimensions" can survive even if no single
 * factor is strong.
 *
 * Score breakdown:
 * - Voluntary marking: Absolute (±100)
 * - Recent access: 0-30 points (scaled by access count - Priority 6)
 * - Emotional salience: 0-25 points (type-weighted - Priority 2)
 * - Graph centrality: 0-20 points
 * - Pattern outlier: 15 points
 * - Base importance: 0-10 points
 * - Age penalty: -1 per day after 14 days
 */
static float calculate_preservation_score(memory_record_t* rec, time_t now) {
    float score = 0.0f;

    /* Voluntary marking (absolute) */
    if (rec->marked_important) {
        return PRESERVATION_SCORE_ABSOLUTE;  /* Always preserve */
    }
    if (rec->marked_forgettable) {
        return ARCHIVAL_SCORE_ABSOLUTE;  /* Always archive (consent requirement) */
    }

    /* Recent access (0-30 points) with frequency scaling (Priority 6) */
    if (rec->last_accessed > 0) {
        float days_since = (float)(now - rec->last_accessed) / (float)SECONDS_PER_DAY;

        /* Access-count weighting: frequent access extends "warm" period */
        float access_threshold = RECENT_ACCESS_DAYS + (rec->access_count * 2);
        if (access_threshold > RECENT_ACCESS_THRESHOLD_DAYS) {
            access_threshold = RECENT_ACCESS_THRESHOLD_DAYS;  /* Cap at 21 days */
        }

        /* Linear decay from max points to 0 over threshold period */
        if (days_since < access_threshold) {
            score += WEIGHT_RECENT_ACCESS * (1.0f - (days_since / access_threshold));
        }
    }

    /* Emotional salience (0-25 points) with type weighting (Priority 2) */
    if (rec->emotion_intensity > 0.0f) {
        float emotion_multiplier = get_emotion_multiplier(rec->emotion_type);
        float adjusted_intensity = rec->emotion_intensity * emotion_multiplier;

        /* Cap at 1.0 to prevent overflow */
        if (adjusted_intensity > 1.0f) {
            adjusted_intensity = 1.0f;
        }

        score += adjusted_intensity * WEIGHT_EMOTION;
    }

    /* Graph centrality (0-20 points) */
    score += rec->graph_centrality * WEIGHT_CENTRALITY;

    /* Pattern outlier (15 points) */
    if (rec->is_pattern_outlier) {
        score += WEIGHT_PATTERN_OUTLIER;
    }

    /* Base importance (0-10 points) */
    score += rec->importance * WEIGHT_IMPORTANCE;

    /* Age penalty (subtract 1 per day older than 14 days) */
    float age_days = (float)(now - rec->timestamp) / (float)SECONDS_PER_DAY;
    if (age_days > AGE_PENALTY_START_DAYS) {
        score -= (age_days - AGE_PENALTY_START_DAYS) * AGE_PENALTY_PER_DAY;
    }

    return score;
}

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

        /* Respect personal collection and CI curation (conscious consent)
         * Personal memories: Part of CI's working self, should not be archived
         * not_to_archive: CI explicitly holding onto this memory */
        if (record->personal) {
            LOG_DEBUG("Preserving personal memory (collection='%s'): %.50s...",
                     record->collection ? record->collection : KATRA_DEFAULT_NONE,
                     record->content);
            katra_memory_free_record(record);
            continue;
        }

        if (record->not_to_archive) {
            LOG_DEBUG("Preserving memory marked not_to_archive: %.50s...",
                     record->content);
            katra_memory_free_record(record);
            continue;
        }

        /* Calculate age for decision logic */
        float age_days = (float)(now - record->timestamp) / (float)SECONDS_PER_DAY;

        /* Safety: NEVER archive memories less than 1 day old
         * Fresh memories haven't had time to accumulate access/connection metrics */
        if (age_days < 1.0f) {
            LOG_DEBUG("Preserving recent memory (%.1f hours old): %.50s...",
                     age_days * 24.0f, record->content);
            katra_memory_free_record(record);
            continue;
        }

        /* Preserve memories with importance >= 0.5 until they're genuinely old
         * User requirement: "allow any memories with a score of 0.5 to be recalled"
         * Cutoff represents the age threshold (typically 7 days) */
        if (record->importance >= MEMORY_IMPORTANCE_MEDIUM && record->timestamp >= cutoff) {
            LOG_DEBUG("Preserving important memory (importance=%.2f, age=%.1f days): %.50s...",
                     record->importance, age_days, record->content);
            katra_memory_free_record(record);
            continue;
        }

        /* Thane's Phase 4: Multi-factor scoring system
         *
         * Replaces sequential if-else logic with weighted scoring.
         * Memories "warm on multiple dimensions" can survive even if
         * no single factor is strong.
         */
        float score = calculate_preservation_score(record, now);

        /* Decision threshold */
        if (score >= PRESERVATION_SCORE_THRESHOLD) {
            /* Preserve this memory */
            LOG_DEBUG("Preserving memory (score=%.1f): %.50s...", score, record->content);
            katra_memory_free_record(record);
            continue;
        }

        /* Archive if below threshold AND old enough (respects max_age_days cutoff) */
        if (score < PRESERVATION_SCORE_THRESHOLD && record->timestamp < cutoff) {
            LOG_DEBUG("Archiving memory (score=%.1f, age=%ld days): %.50s...",
                     score,
                     (now - record->timestamp) / SECONDS_PER_DAY,
                     record->content);
            /* Will be added to archive array below */
        } else {
            /* Edge case: preserve borderline memories that are not very old */
            katra_memory_free_record(record);
            continue;
        }

        /* Add to array */
        if (*count >= *capacity) {
            size_t new_cap = *capacity == 0 ? KATRA_INITIAL_CAPACITY_LARGE : *capacity * 2;
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
    int week_num = (day_of_year / DAYS_PER_WEEK) + 1;

    snprintf(week_id, size, "%04d-W%02d",
            tm_info->tm_year + TM_YEAR_OFFSET,
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
            size_t new_cap = arr_capacity == 0 ? KATRA_INITIAL_CAPACITY_LARGE : arr_capacity * 2;
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
        katra_free_string_array(filenames, file_count);
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

    katra_free_string_array(filenames, file_count);
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

/* Helper: Process archivable records and store to Tier 2 */
static int process_and_store_archivable_records(const char* ci_id,
                                                 const char* tier1_dir,
                                                 memory_record_t** records,
                                                 size_t* record_count) {
    int result = KATRA_SUCCESS;

    /* Thane's Phase 2: Calculate graph centrality */
    LOG_DEBUG("Calculating graph centrality for %zu records", *record_count);
    result = katra_memory_calculate_centrality_for_records(records, *record_count);
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Failed to calculate centrality, continuing without it");
    }

    /* Thane's Phase 3: Detect patterns and filter outliers */
    LOG_DEBUG("Detecting patterns in %zu archivable records", *record_count);
    katra_tier1_detect_patterns(records, *record_count);
    *record_count = katra_tier1_filter_pattern_outliers(records, *record_count);

    if (*record_count == 0) {
        return 0;  /* All records were outliers */
    }

    /* Prepare record IDs array */
    const char** record_ids = malloc(*record_count * sizeof(char*));
    if (!record_ids) {
        return E_SYSTEM_MEMORY;
    }
    for (size_t i = 0; i < *record_count; i++) {
        record_ids[i] = records[i]->record_id;
    }

    /* Get week_id and create digest */
    char week_id[KATRA_BUFFER_TINY];
    get_week_id(records[0]->timestamp, week_id, sizeof(week_id));
    digest_record_t* digest = create_digest_from_records(ci_id, week_id,
                                                          records, *record_count);
    if (!digest) {
        free(record_ids);
        return E_SYSTEM_MEMORY;
    }

    /* Store digest to Tier 2 */
    result = tier2_store_digest(digest);
    katra_digest_free(digest);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "process_and_store_archivable_records",
                          "Failed to store digest to Tier 2"); /* GUIDELINE_APPROVED: error context */
        free(record_ids);
        return result;
    }

    /* Mark records as archived in Tier 1 */
    result = mark_records_as_archived(tier1_dir, record_ids, *record_count);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "process_and_store_archivable_records",
                          "Failed to mark records as archived"); /* GUIDELINE_APPROVED: error context */
        free(record_ids);
        return result;
    }

    LOG_INFO("Archived %zu Tier 1 records to Tier 2 digest %s", *record_count, week_id);
    free(record_ids);
    return (int)*record_count;
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
    time_t cutoff = now - (max_age_days * SECONDS_PER_DAY);

    /* Collect filenames */
    result = tier1_collect_jsonl_files(tier1_dir, &filenames, &file_count);
    if (result != KATRA_SUCCESS || file_count == 0) {
        katra_free_string_array(filenames, file_count);
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

    /* Process records and store to Tier 2 */
    result = process_and_store_archivable_records(ci_id, tier1_dir, records, &record_count);

cleanup:
    if (records) {
        for (size_t i = 0; i < record_count; i++) {
            katra_memory_free_record(records[i]);
        }
        free(records);
    }
    katra_free_string_array(filenames, file_count);

    return result;
}
