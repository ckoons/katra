/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_tier1.h"
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
        if (record->marked_forgettable) {
            LOG_DEBUG("Archiving marked_forgettable memory: %.50s...", record->content);
            /* Skip age check - archive immediately */
        }
        /* Access-based decay: Don't archive recently accessed memories */
        else if (record->last_accessed > 0) {
            time_t days_since_accessed = (now - record->last_accessed) / (24 * 3600);
            if (days_since_accessed < 7) {  /* Keep if accessed within a week */
                LOG_DEBUG("Preserving recently accessed memory (%.0f days): %.50s...",
                         (double)days_since_accessed, record->content);
                katra_memory_free_record(record);
                continue;
            }
        }
        /* Emotional salience: Keep high-intensity emotions longer */
        else if (record->emotion_intensity >= 0.7f) {  /* High arousal threshold */
            LOG_DEBUG("Preserving emotionally salient memory (%.2f): %.50s...",
                     record->emotion_intensity, record->content);
            katra_memory_free_record(record);
            continue;
        }
        /* Graph centrality: Keep highly connected memories (Thane's Phase 2) */
        else if (record->graph_centrality >= 0.6f) {  /* High centrality threshold */
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

/* Helper: Calculate semantic similarity between two memories (Phase 3)
 *
 * Simple keyword-based similarity for pattern detection.
 * Returns score 0.0-1.0 based on shared keywords.
 */
static float calculate_similarity(const char* content1, const char* content2) {
    if (!content1 || !content2) {
        return 0.0f;
    }

    /* Convert to lowercase and count shared words (simplified) */
    size_t shared = 0;
    size_t total = 0;

    /* Very simple keyword matching - count character overlap */
    size_t len1 = strlen(content1);
    size_t len2 = strlen(content2);

    if (len1 == 0 || len2 == 0) {
        return 0.0f;
    }

    /* Count matching substrings (3+ chars) */
    for (size_t i = 0; i < len1 - 2; i++) {
        for (size_t j = 0; j < len2 - 2; j++) {
            if (strncasecmp(content1 + i, content2 + j, 3) == 0) {
                shared++;
                break;
            }
        }
        total++;
    }

    return total > 0 ? (float)shared / (float)total : 0.0f;
}

/* Helper: Detect patterns in memory set (Phase 3)
 *
 * Thane's insight: "I debugged 50 times" → pattern + count + outliers
 * Groups similar memories, marks patterns, preserves outliers.
 */
static void detect_patterns(memory_record_t** records, size_t count) {
    const float similarity_threshold = 0.3f;  /* 30% similarity = pattern */
    const size_t min_pattern_size = 3;        /* Need 3+ to be a pattern */

    for (size_t i = 0; i < count; i++) {
        if (records[i]->pattern_id) {
            continue;  /* Already assigned to pattern */
        }

        /* Find similar memories */
        size_t pattern_members[256];  /* Simplified: max 256 pattern members */
        size_t member_count = 0;
        pattern_members[member_count++] = i;

        for (size_t j = i + 1; j < count && member_count < 256; j++) {
            if (records[j]->pattern_id) {
                continue;
            }

            float similarity = calculate_similarity(records[i]->content, records[j]->content);
            if (similarity >= similarity_threshold) {
                pattern_members[member_count++] = j;
            }
        }

        /* If enough similar memories, create pattern */
        if (member_count >= min_pattern_size) {
            /* Generate pattern ID */
            char pattern_id[64];
            snprintf(pattern_id, sizeof(pattern_id), "pattern_%zu_%ld",
                    i, (long)records[i]->timestamp);

            /* Assign all members to pattern */
            for (size_t m = 0; m < member_count; m++) {
                size_t idx = pattern_members[m];
                records[idx]->pattern_id = strdup(pattern_id);
                records[idx]->pattern_frequency = member_count;
                records[idx]->semantic_similarity = 1.0f;  /* Simplified */
            }

            /* Mark outliers: first, last, and highest importance */
            records[pattern_members[0]]->is_pattern_outlier = true;  /* First */
            records[pattern_members[member_count - 1]]->is_pattern_outlier = true;  /* Last */

            /* Find highest importance */
            size_t max_importance_idx = 0;
            float max_importance = 0.0f;
            for (size_t m = 0; m < member_count; m++) {
                size_t idx = pattern_members[m];
                if (records[idx]->importance > max_importance) {
                    max_importance = records[idx]->importance;
                    max_importance_idx = idx;
                }
            }
            records[max_importance_idx]->is_pattern_outlier = true;  /* Most important */

            LOG_DEBUG("Detected pattern %s with %zu members", pattern_id, member_count);
        }
    }
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

    /* Thane's Phase 3: Detect patterns before archiving */
    LOG_DEBUG("Detecting patterns in %zu archivable records", record_count);
    detect_patterns(records, record_count);

    /* Filter out pattern outliers (preserve them, archive the rest) */
    size_t final_count = 0;
    for (size_t i = 0; i < record_count; i++) {
        if (records[i]->pattern_id && !records[i]->is_pattern_outlier) {
            /* Part of pattern, not an outlier - archive it */
            LOG_DEBUG("Archiving pattern member (pattern=%s, freq=%zu): %.50s...",
                     records[i]->pattern_id, records[i]->pattern_frequency,
                     records[i]->content);
            records[final_count++] = records[i];
        } else if (records[i]->is_pattern_outlier) {
            /* Pattern outlier - preserve it */
            LOG_DEBUG("Preserving pattern outlier (pattern=%s): %.50s...",
                     records[i]->pattern_id, records[i]->content);
            /* Note: In production, we'd write it back to tier1 with updated metadata */
            /* For now, just don't archive it */
            katra_memory_free_record(records[i]);
        } else {
            /* Not part of pattern - archive normally */
            records[final_count++] = records[i];
        }
    }

    record_count = final_count;

    if (record_count == 0) {
        result = 0;  /* All records were outliers */
        goto cleanup;
    }

    /* Group records by week and create digests */
    /* Simplified implementation: create one digest for all records */
    /* Production would group by week_id */
    char week_id[32];
    get_week_id(records[0]->timestamp, week_id, sizeof(week_id));

    digest_record_t* digest = create_digest_from_records(ci_id, week_id,
                                                          records, record_count);
    if (!digest) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

    /* Store digest to Tier 2 */
    result = tier2_store_digest(digest);
    katra_digest_free(digest);

    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "tier1_archive", "Failed to store digest to Tier 2");
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
