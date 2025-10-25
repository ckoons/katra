/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

/* Project includes */
#include "katra_tier2.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_json_utils.h"
#include "katra_strings.h"

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

    (void)ci_id;  /* Unused - future multi-tenant support */

    LOG_DEBUG("Initializing Tier 2 storage: %s", tier2_dir);
    LOG_INFO("Tier 2 storage initialized (weekly, monthly, index)");

    return KATRA_SUCCESS;
}

/* Store digest (placeholder) */
int tier2_store_digest(const digest_record_t* digest) {
    if (!digest) {
        katra_report_error(E_INPUT_NULL, "tier2_store_digest", "digest is NULL");
        return E_INPUT_NULL;
    }

    LOG_INFO("Tier 2 store_digest not yet implemented");
    return E_INTERNAL_NOTIMPL;
}

/* Query Tier 2 digests (placeholder) */
int tier2_query(const digest_query_t* query,
                digest_record_t*** results,
                size_t* count) {
    if (!query || !results || !count) {
        katra_report_error(E_INPUT_NULL, "tier2_query", "NULL parameter");
        return E_INPUT_NULL;
    }

    *results = NULL;
    *count = 0;

    LOG_INFO("Tier 2 query not yet implemented");
    return E_INTERNAL_NOTIMPL;
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
    const char* period_type_str = (period_type == PERIOD_TYPE_WEEKLY) ? "weekly" : "monthly";
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
    for (size_t i = 0; i < digest->theme_count; i++) {
        free(digest->themes[i]);
    }
    free(digest->themes);

    for (size_t i = 0; i < digest->keyword_count; i++) {
        free(digest->keywords[i]);
    }
    free(digest->keywords);

    for (size_t i = 0; i < digest->insight_count; i++) {
        free(digest->key_insights[i]);
    }
    free(digest->key_insights);

    for (size_t i = 0; i < digest->decision_count; i++) {
        free(digest->decisions_made[i]);
    }
    free(digest->decisions_made);

    /* Free entities */
    for (size_t i = 0; i < digest->entities.file_count; i++) {
        free(digest->entities.files[i]);
    }
    free(digest->entities.files);

    for (size_t i = 0; i < digest->entities.concept_count; i++) {
        free(digest->entities.concepts[i]);
    }
    free(digest->entities.concepts);

    for (size_t i = 0; i < digest->entities.people_count; i++) {
        free(digest->entities.people[i]);
    }
    free(digest->entities.people);

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
