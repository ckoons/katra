/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_continuity.h"
#include "katra_tier1.h"
#include "katra_tier2.h"
#include "katra_error.h"
#include "katra_log.h"

/* Constants */
#define SUMMARY_BUFFER_SIZE 2048
#define INSIGHT_BUFFER_SIZE 512
#define MAX_INSIGHTS 5

/* Get start of today (00:00:00) */
static time_t get_day_start(time_t when) {
    struct tm tm_time;
    localtime_r(&when, &tm_time);
    tm_time.tm_hour = 0;
    tm_time.tm_min = 0;
    tm_time.tm_sec = 0;
    return mktime(&tm_time);
}

/* Get end of today (23:59:59) */
static time_t get_day_end(time_t when) {
    struct tm tm_time;
    localtime_r(&when, &tm_time);
    tm_time.tm_hour = 23;
    tm_time.tm_min = 59;
    tm_time.tm_sec = 59;
    return mktime(&tm_time);
}

/* Format date as YYYY-MM-DD */
static void format_date(time_t timestamp, char* buffer, size_t size) {
    struct tm tm_time;
    localtime_r(&timestamp, &tm_time);
    snprintf(buffer, size, "%04d-%02d-%02d",
             tm_time.tm_year + 1900,
             tm_time.tm_mon + 1,
             tm_time.tm_mday);
}

/* Get daily statistics */
int katra_get_daily_stats(const char* ci_id, daily_stats_t* stats) {
    int result = KATRA_SUCCESS;
    memory_record_t** records = NULL;
    size_t count = 0;

    if (!ci_id || !stats) {
        katra_report_error(E_INPUT_NULL, "katra_get_daily_stats",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    /* Initialize stats */
    memset(stats, 0, sizeof(daily_stats_t));

    /* Query today's memories */
    time_t now = time(NULL);
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = get_day_start(now),
        .end_time = get_day_end(now),
        .type = 0,  /* All types */
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 0  /* No limit */
    };

    result = tier1_query(&query, &records, &count);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    if (count == 0) {
        LOG_DEBUG("No memories found for today");
        return KATRA_SUCCESS;
    }

    /* Calculate statistics */
    stats->interaction_count = count;
    float total_importance = 0.0;

    for (size_t i = 0; i < count; i++) {
        memory_record_t* rec = records[i];

        if (rec->type == MEMORY_TYPE_INTERACTION && rec->content) {
            /* Simple heuristic: count question marks */
            const char* p = rec->content;
            while ((p = strchr(p, '?')) != NULL) {
                stats->questions_asked++;
                p++;
            }
        }

        total_importance += rec->importance;
    }

    stats->avg_importance = total_importance / (float)count;

    katra_memory_free_results(records, count);

    LOG_DEBUG("Daily stats: %d interactions, %d questions, avg importance %.2f",
             stats->interaction_count, stats->questions_asked,
             stats->avg_importance);

    return KATRA_SUCCESS;
}

/* Sundown: Create end-of-day summary */
int katra_sundown_basic(const char* ci_id, const char* summary) {
    int result = KATRA_SUCCESS;
    digest_record_t* digest = NULL;
    daily_stats_t stats;
    char date_str[32];
    char auto_summary[SUMMARY_BUFFER_SIZE];

    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_sundown_basic",
                          "ci_id is NULL");
        return E_INPUT_NULL;
    }

    /* Get today's statistics */
    result = katra_get_daily_stats(ci_id, &stats);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_sundown_basic",
                          "Failed to get daily stats");
        return result;
    }

    /* Format today's date as period_id */
    time_t now = time(NULL);
    format_date(now, date_str, sizeof(date_str));

    /* Create digest */
    digest = katra_digest_create(ci_id, PERIOD_TYPE_WEEKLY,
                                 date_str, DIGEST_TYPE_INTERACTION);
    if (!digest) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_sundown_basic",
                          "Failed to create digest");
        return E_SYSTEM_MEMORY;
    }

    /* Generate or use provided summary */
    if (summary) {
        digest->summary = strdup(summary);
    } else {
        snprintf(auto_summary, sizeof(auto_summary),
                "Daily summary for %s: %d interactions, %d questions asked, "
                "average importance %.2f. ",
                date_str, stats.interaction_count, stats.questions_asked,
                stats.avg_importance);
        digest->summary = strdup(auto_summary);
    }

    if (!digest->summary) {
        katra_digest_free(digest);
        katra_report_error(E_SYSTEM_MEMORY, "katra_sundown_basic",
                          "Failed to allocate summary");
        return E_SYSTEM_MEMORY;
    }

    /* Store metadata */
    digest->source_record_count = stats.interaction_count;
    digest->questions_asked = stats.questions_asked;
    digest->source_tier = KATRA_TIER1;

    /* Store digest */
    result = tier2_store_digest(digest);
    if (result != KATRA_SUCCESS) {
        katra_digest_free(digest);
        katra_report_error(result, "katra_sundown_basic",
                          "Failed to store digest");
        return result;
    }

    LOG_INFO("Sundown complete: %s (%d interactions)",
             date_str, stats.interaction_count);

    katra_digest_free(digest);
    return KATRA_SUCCESS;
}

/* Sunrise: Load previous day's summary */
int katra_sunrise_basic(const char* ci_id, digest_record_t** digest) {
    int result = KATRA_SUCCESS;
    digest_record_t** results = NULL;
    size_t count = 0;
    time_t now = time(NULL);
    time_t yesterday = now - (24 * 60 * 60);

    if (!ci_id || !digest) {
        katra_report_error(E_INPUT_NULL, "katra_sunrise_basic",
                          "NULL parameter");
        return E_INPUT_NULL;
    }

    *digest = NULL;

    /* Query yesterday's digest */
    digest_query_t query = {
        .ci_id = ci_id,
        .start_time = get_day_start(yesterday),
        .end_time = get_day_end(yesterday),
        .period_type = (period_type_t)-1,  /* Any period type */
        .theme = NULL,
        .keyword = NULL,
        .digest_type = DIGEST_TYPE_INTERACTION,
        .limit = 1  /* Just need one */
    };

    result = tier2_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_sunrise_basic",
                          "Failed to query yesterday's digest");
        return result;
    }

    if (count == 0) {
        LOG_INFO("Sunrise: No previous day summary found (first day?)");
        return KATRA_SUCCESS;
    }

    /* Return the digest (caller must free) */
    *digest = results[0];

    /* Free the results array but NOT the digest itself */
    free(results);

    LOG_INFO("Sunrise: Loaded summary from %s (%zu interactions)",
             (*digest)->period_id, (*digest)->source_record_count);

    return KATRA_SUCCESS;
}
