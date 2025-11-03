/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <dirent.h>

/* Project includes */
#include "katra_tier2.h"
#include "katra_tier2_index.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_json_utils.h"
#include "katra_strings.h"
#include "katra_core_common.h"

/* Tier 2 directory names */
#define TIER2_DIR_WEEKLY  "weekly"
#define TIER2_DIR_MONTHLY "monthly"
#define TIER2_DIR_INDEX   "index"

/* Initialize Tier 2 storage */
int tier2_init(const char* ci_id) {
    int result = KATRA_SUCCESS;
    char tier2_dir[KATRA_PATH_MAX];
    char weekly_dir[KATRA_PATH_MAX];
    char monthly_dir[KATRA_PATH_MAX];
    char index_dir[KATRA_PATH_MAX];

    /* Create main Tier 2 directory */
    result = katra_build_and_ensure_dir(tier2_dir, sizeof(tier2_dir),
                                        KATRA_DIR_MEMORY, KATRA_DIR_TIER2, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Create weekly subdirectory */
    result = katra_build_and_ensure_dir(weekly_dir, sizeof(weekly_dir),
                                        KATRA_DIR_MEMORY, KATRA_DIR_TIER2,
                                        TIER2_DIR_WEEKLY, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Create monthly subdirectory */
    result = katra_build_and_ensure_dir(monthly_dir, sizeof(monthly_dir),
                                        KATRA_DIR_MEMORY, KATRA_DIR_TIER2,
                                        TIER2_DIR_MONTHLY, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Create index subdirectory */
    result = katra_build_and_ensure_dir(index_dir, sizeof(index_dir),
                                        KATRA_DIR_MEMORY, KATRA_DIR_TIER2,
                                        TIER2_DIR_INDEX, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Initialize SQLite index */
    result = tier2_index_init(ci_id);
    if (result != KATRA_SUCCESS) {
        LOG_ERROR("Failed to initialize Tier 2 index");
        return result;
    }

    LOG_DEBUG("Initializing Tier 2 storage: %s", tier2_dir);
    LOG_INFO("Tier 2 storage initialized (weekly, monthly, index)");

    return KATRA_SUCCESS;
}

/* Store digest */
int tier2_store_digest(const digest_record_t* digest) {
    int result = KATRA_SUCCESS;
    FILE* fp = NULL;
    char filepath[KATRA_PATH_MAX];

    if (!digest) {
        katra_report_error(E_INPUT_NULL, "tier2_store_digest", "digest is NULL");
        result = E_INPUT_NULL;
        goto cleanup;
    }

    if (!digest->period_id) {
        katra_report_error(E_INPUT_NULL, "tier2_store_digest", "period_id is NULL");
        result = E_INPUT_NULL;
        goto cleanup;
    }

    /* Determine subdirectory based on period type */
    const char* subdir = (digest->period_type == PERIOD_TYPE_WEEKLY)
                         ? TIER2_DIR_WEEKLY : TIER2_DIR_MONTHLY;

    /* Build file path: ~/.katra/memory/tier2/{weekly|monthly}/PERIOD_ID.jsonl */
    char tier2_subdir[KATRA_PATH_MAX];
    result = katra_build_path(tier2_subdir, sizeof(tier2_subdir),
                              KATRA_DIR_MEMORY, KATRA_DIR_TIER2, subdir, NULL);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "tier2_store_digest", "Failed to build path");
        goto cleanup;
    }

    snprintf(filepath, sizeof(filepath), "%s/%s.jsonl", tier2_subdir, digest->period_id);

    /* Check file size before writing */
    struct stat st;
    if (stat(filepath, &st) == 0) {
        size_t size_mb = st.st_size / BYTES_PER_MEGABYTE;
        if (size_mb >= TIER2_MAX_FILE_SIZE_MB) {
            katra_report_error(E_MEMORY_TIER_FULL, "tier2_store_digest",
                              "Digest file exceeds %d MB", TIER2_MAX_FILE_SIZE_MB);
            result = E_MEMORY_TIER_FULL;
            goto cleanup;
        }
    }

    /* Open file for append and capture current offset */
    fp = fopen(filepath, "a");
    if (!fp) {
        katra_report_error(E_SYSTEM_FILE, "tier2_store_digest",
                          "Failed to open %s", filepath);
        result = E_SYSTEM_FILE;
        goto cleanup;
    }

    /* Get file offset before writing */
    long offset = ftell(fp);

    /* Write JSON digest */
    result = katra_tier2_write_json_digest(fp, digest);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "tier2_store_digest", "Failed to write digest");
        goto cleanup;
    }

    /* Add to index */
    int index_result = tier2_index_add(digest, filepath, offset);
    if (index_result != KATRA_SUCCESS) {
        LOG_WARN("Failed to add digest to index (digest stored, but not indexed)");
        /* Don't fail the whole operation if indexing fails */
    }

    LOG_DEBUG("Stored digest to %s", filepath);

cleanup:
    if (fp) {
        fclose(fp);
    }
    return result;
}

/* Helper: Check if digest matches query filters */
static bool digest_matches_query(const digest_record_t* digest,
                                  const digest_query_t* query) {
    /* Time range filter */
    if (query->start_time > 0 && digest->timestamp < query->start_time) {
        return false;
    }
    if (query->end_time > 0 && digest->timestamp > query->end_time) {
        return false;
    }

    /* Period type filter (-1 cast to period_type_t means "any") */
    if ((int)query->period_type != -1 && digest->period_type != query->period_type) {
        return false;
    }

    /* Digest type filter (-1 cast to digest_type_t means "any") */
    if ((int)query->digest_type != -1 && digest->digest_type != query->digest_type) {
        return false;
    }

    /* CI ID filter (required) */
    if (strcmp(digest->ci_id, query->ci_id) != 0) {
        return false;
    }

    /* Theme filter (if specified) */
    if (query->theme) {
        bool found = false;
        for (size_t i = 0; i < digest->theme_count; i++) {
            if (digest->themes[i] && strstr(digest->themes[i], query->theme)) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    /* Keyword filter (if specified) */
    if (query->keyword) {
        bool found = false;
        for (size_t i = 0; i < digest->keyword_count; i++) {
            if (digest->keywords[i] && strstr(digest->keywords[i], query->keyword)) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

/* Helper: Scan digest file and collect matching records */
static int scan_digest_file(const char* filepath,
                            const digest_query_t* query,
                            digest_record_t*** results,
                            size_t* count,
                            size_t* capacity) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        return KATRA_SUCCESS;  /* File doesn't exist, skip */
    }

    char line[KATRA_BUFFER_LARGE];
    while (fgets(line, sizeof(line), fp)) {
        /* Parse digest from JSON line */
        digest_record_t* digest = NULL;
        int result = katra_tier2_parse_json_digest(line, &digest);
        if (result != KATRA_SUCCESS || !digest) {
            continue;  /* Skip malformed lines */
        }

        /* Check if digest matches query */
        if (digest_matches_query(digest, query)) {
            /* Grow results array if needed */
            if (*count >= *capacity) {
                size_t new_capacity = (*capacity == 0) ? KATRA_INITIAL_CAPACITY_MEDIUM : (*capacity * 2);
                digest_record_t** new_results = realloc(*results,
                    new_capacity * sizeof(digest_record_t*));
                if (!new_results) {
                    katra_digest_free(digest);
                    fclose(fp);
                    return E_SYSTEM_MEMORY;
                }
                *results = new_results;
                *capacity = new_capacity;
            }

            /* Add to results */
            (*results)[*count] = digest;
            (*count)++;

            /* Check limit */
            if (query->limit > 0 && *count >= query->limit) {
                fclose(fp);
                return KATRA_SUCCESS;
            }
        } else {
            katra_digest_free(digest);
        }
    }

    fclose(fp);
    return KATRA_SUCCESS;
}

/* Helper: Scan all .jsonl files in directory */
static int scan_directory(const char* dir_path,
                          const digest_query_t* query,
                          digest_record_t*** results,
                          size_t* count,
                          size_t* capacity) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        return KATRA_SUCCESS;  /* Directory doesn't exist, skip */
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Check for .jsonl extension */
        size_t name_len = strlen(entry->d_name);
        if (name_len < 6 || strcmp(entry->d_name + name_len - 6, ".jsonl") != 0) {
            continue;
        }

        /* Build full file path */
        char filepath[KATRA_PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);

        /* Scan file */
        int result = scan_digest_file(filepath, query, results, count, capacity);
        if (result != KATRA_SUCCESS) {
            closedir(dir);
            return result;
        }

        /* Check if limit reached */
        if (query->limit > 0 && *count >= query->limit) {
            closedir(dir);
            return KATRA_SUCCESS;
        }
    }

    closedir(dir);
    return KATRA_SUCCESS;
}

/* Query Tier 2 digests (using SQLite index) */
int tier2_query(const digest_query_t* query,
                digest_record_t*** results,
                size_t* count) {
    int result = KATRA_SUCCESS;
    char** digest_ids = NULL;
    index_location_t* locations = NULL;
    size_t location_count = 0;
    digest_record_t** result_array = NULL;
    size_t result_count = 0;

    if (!query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "tier2_query", "NULL parameter");
        return E_INPUT_NULL;
    }

    if (!query->ci_id) {
        katra_report_error(E_INPUT_NULL, "tier2_query", "ci_id is required");
        return E_INPUT_NULL;
    }

    *results = NULL;
    *count = 0;

    /* Use SQLite index for fast lookup */
    result = tier2_index_query(query, &digest_ids, &locations, &location_count);
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Index query failed, falling back to file scan");
        goto fallback_scan;
    }

    if (location_count == 0) {
        LOG_DEBUG("Tier 2 query returned 0 results (from index)");
        return KATRA_SUCCESS;
    }

    /* Load digests from JSONL files using index locations */
    result = tier2_load_by_locations(locations, location_count,
                                      &result_array, &result_count);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "tier2_query", "Failed to load digests");
        goto cleanup;
    }

    *results = result_array;
    *count = result_count;
    LOG_DEBUG("Tier 2 query returned %zu results (indexed)", result_count);

    /* Free index query results */
    katra_free_string_array(digest_ids, location_count);
    free(locations);

    return KATRA_SUCCESS;

fallback_scan:
    /* Fallback to file scanning if index unavailable */
    {
        digest_record_t** scan_results = NULL;
        size_t scan_count = 0;
        size_t scan_capacity = 0;
        char weekly_dir[KATRA_PATH_MAX];
        char monthly_dir[KATRA_PATH_MAX];

        result = katra_build_path(weekly_dir, sizeof(weekly_dir),
                                  KATRA_DIR_MEMORY, KATRA_DIR_TIER2,
                                  TIER2_DIR_WEEKLY, NULL);
        if (result != KATRA_SUCCESS) {
            goto cleanup;
        }

        result = katra_build_path(monthly_dir, sizeof(monthly_dir),
                                  KATRA_DIR_MEMORY, KATRA_DIR_TIER2,
                                  TIER2_DIR_MONTHLY, NULL);
        if (result != KATRA_SUCCESS) {
            goto cleanup;
        }

        /* Scan weekly files */
        if ((int)query->period_type == -1 || query->period_type == PERIOD_TYPE_WEEKLY) {
            result = scan_directory(weekly_dir, query, &scan_results,
                                   &scan_count, &scan_capacity);
            if (result != KATRA_SUCCESS) {
                goto cleanup;
            }
        }

        /* Scan monthly files */
        if ((int)query->period_type == -1 || query->period_type == PERIOD_TYPE_MONTHLY) {
            result = scan_directory(monthly_dir, query, &scan_results,
                                   &scan_count, &scan_capacity);
            if (result != KATRA_SUCCESS) {
                goto cleanup;
            }
        }

        *results = scan_results;
        *count = scan_count;
        LOG_DEBUG("Tier 2 query returned %zu results (file scan)", scan_count);
        return KATRA_SUCCESS;
    }

cleanup:
    katra_free_string_array(digest_ids, location_count);
    free(locations);

    if (result_array) {
        for (size_t i = 0; i < result_count; i++) {
            katra_digest_free(result_array[i]);
        }
        free(result_array);
    }
    return result;
}

/* Archive old Tier 2 digests (placeholder) */
int tier2_archive(const char* ci_id, int max_age_days) {
    if (!ci_id) {
        return E_INPUT_NULL;
    }

    if (max_age_days < 0) {
        return E_INPUT_RANGE;
    }

    LOG_INFO("Tier 2 archive not yet implemented (Tier 3 not ready)");
    return 0;  /* No digests archived */
}

/* Get Tier 2 statistics (placeholder) */
int tier2_stats(const char* ci_id, size_t* total_digests, size_t* bytes_used) {
    if (!ci_id || !total_digests || !bytes_used) {
        return E_INPUT_NULL;
    }

    *total_digests = 0;
    *bytes_used = 0;

    LOG_DEBUG("Tier 2 stats: digests=%zu, bytes=%zu", *total_digests, *bytes_used);
    return KATRA_SUCCESS;
}

/* Cleanup Tier 2 storage */
void tier2_cleanup(void) {
    tier2_index_cleanup();
    LOG_DEBUG("Tier 2 cleanup complete");
}

/* Create digest record */
digest_record_t* katra_digest_create(
    const char* ci_id,
    period_type_t period_type,
    const char* period_id,
    digest_type_t digest_type) {

    if (!ci_id || !period_id) {
        return NULL;
    }

    digest_record_t* digest = calloc(1, sizeof(digest_record_t));
    if (!digest) {
        return NULL;
    }

    /* Set basic fields */
    digest->ci_id = strdup(ci_id);
    digest->period_id = strdup(period_id);
    digest->period_type = period_type;
    digest->digest_type = digest_type;
    digest->timestamp = time(NULL);
    digest->source_tier = KATRA_TIER1;
    digest->archived = false;

    /* Check allocations */
    if (!digest->ci_id || !digest->period_id) {
        katra_digest_free(digest);
        return NULL;
    }

    /* Generate digest_id */
    char digest_id[KATRA_BUFFER_MEDIUM];
    const char* period_type_str = (period_type == PERIOD_TYPE_WEEKLY) ? TIER2_DIR_WEEKLY : TIER2_DIR_MONTHLY;
    snprintf(digest_id, sizeof(digest_id), "%s-%s-digest", period_id, period_type_str);
    digest->digest_id = strdup(digest_id);

    if (!digest->digest_id) {
        katra_digest_free(digest);
        return NULL;
    }

    return digest;
}

/* Free digest record */
void katra_digest_free(digest_record_t* digest) {
    if (!digest) {
        return;
    }

    free(digest->digest_id);
    free(digest->period_id);
    free(digest->ci_id);
    free(digest->summary);

    /* Free arrays */
    katra_free_string_array(digest->themes, digest->theme_count);
    katra_free_string_array(digest->keywords, digest->keyword_count);
    katra_free_string_array(digest->key_insights, digest->insight_count);
    katra_free_string_array(digest->decisions_made, digest->decision_count);

    /* Free entities */
    katra_free_string_array(digest->entities.files, digest->entities.file_count);
    katra_free_string_array(digest->entities.concepts, digest->entities.concept_count);
    katra_free_string_array(digest->entities.people, digest->entities.people_count);

    free(digest);
}

/* Free digest results */
void katra_digest_free_results(digest_record_t** results, size_t count) {
    if (!results) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        katra_digest_free(results[i]);
    }
    free(results);
}
