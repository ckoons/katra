/* © 2025 Casey Koons All rights reserved */

/*
 * katra_sunrise_sunset_themes.c - Theme and Topic Analysis (Phase 7.5-7.6)
 *
 * Analyzes recurring themes and builds familiar topics from sundown history.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "katra_sunrise_sunset.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_core_common.h"
#include "katra_psyche_common.h"

/* Constants for theme analysis */
#define THEME_NAME_SIZE 128
#define THEME_INITIAL_CAPACITY 16
#define TOPIC_INITIAL_CAPACITY 32
#define DATE_FORMAT_LEN 9
#define DEFAULT_DAYS_BACK 7

/* Helper: convert timestamp to date string */
static void timestamp_to_date(time_t ts, char* date_out, size_t size) {
    struct tm* tm_info = localtime(&ts);
    strftime(date_out, size, "%Y%m%d", tm_info);
}

/* Find recurring themes across multiple days (Phase 7.5) */
int katra_find_recurring_themes(const char* ci_id,
                                int days_back,
                                char*** themes_out,
                                size_t* count_out) {
    if (!ci_id || !themes_out || !count_out) {
        return E_INPUT_NULL;
    }

    *themes_out = NULL;
    *count_out = 0;

    if (days_back <= 0) days_back = DEFAULT_DAYS_BACK;

    /* Topic occurrence counting */
    typedef struct {
        char name[THEME_NAME_SIZE];
        int occurrences;
    } topic_count_t;

    topic_count_t* topic_counts = NULL;
    size_t topic_count_size = 0;
    size_t topic_count_capacity = 0;

    /* Load sundowns for each day */
    time_t now = time(NULL);
    for (int d = 1; d <= days_back; d++) {
        time_t day_ts = now - (d * SECONDS_PER_DAY);
        char date[DATE_FORMAT_LEN];
        timestamp_to_date(day_ts, date, sizeof(date));

        sundown_context_t* context = NULL;
        if (katra_sundown_load_date(ci_id, date, &context) == KATRA_SUCCESS && context) {
            /* Count topic occurrences */
            for (size_t t = 0; t < context->topic_count && context->topics; t++) {
                if (!context->topics[t]) continue;

                /* Find or add topic */
                bool found = false;
                for (size_t i = 0; i < topic_count_size; i++) {
                    if (strcmp(topic_counts[i].name, context->topics[t]->topic_name) == 0) {
                        topic_counts[i].occurrences++;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    /* Add new topic */
                    if (topic_count_size >= topic_count_capacity) {
                        size_t new_cap = topic_count_capacity == 0 ? THEME_INITIAL_CAPACITY : topic_count_capacity * 2;
                        topic_count_t* new_counts = realloc(topic_counts, new_cap * sizeof(topic_count_t));
                        if (!new_counts) {
                            free(topic_counts);
                            return E_SYSTEM_MEMORY;
                        }
                        topic_counts = new_counts;
                        topic_count_capacity = new_cap;
                    }
                    strncpy(topic_counts[topic_count_size].name,
                           context->topics[t]->topic_name,
                           sizeof(topic_counts[topic_count_size].name) - 1);
                    topic_counts[topic_count_size].occurrences = 1;
                    topic_count_size++;
                }
            }
            katra_sundown_free(context);
        }
    }

    /* Filter to recurring themes (appeared more than once) */
    size_t recurring_count = 0;
    for (size_t i = 0; i < topic_count_size; i++) {
        if (topic_counts[i].occurrences > 1) recurring_count++;
    }

    if (recurring_count == 0) {
        free(topic_counts);
        return KATRA_SUCCESS;
    }

    /* Build result array */
    char** themes = calloc(recurring_count, sizeof(char*));
    if (!themes) {
        free(topic_counts);
        return E_SYSTEM_MEMORY;
    }

    size_t idx = 0;
    for (size_t i = 0; i < topic_count_size && idx < recurring_count; i++) {
        if (topic_counts[i].occurrences > 1) {
            themes[idx++] = katra_safe_strdup(topic_counts[i].name);
        }
    }

    free(topic_counts);
    *themes_out = themes;
    *count_out = recurring_count;

    LOG_INFO("Found %zu recurring themes across %d days for %s", recurring_count, days_back, ci_id);
    return KATRA_SUCCESS;
}

/* Build familiar topics from vector similarity (Phase 7.6) */
int katra_build_familiar_topics(const char* ci_id,
                                vector_store_t* vectors,
                                int days_back,
                                char*** topics_out,
                                size_t* count_out) {
    if (!ci_id || !topics_out || !count_out) {
        return E_INPUT_NULL;
    }

    *topics_out = NULL;
    *count_out = 0;

    /* If no vector store, fall back to recurring themes */
    if (!vectors) {
        return katra_find_recurring_themes(ci_id, days_back, topics_out, count_out);
    }

    if (days_back <= 0) days_back = DEFAULT_DAYS_BACK;

    /* Collect all topic names from recent days */
    char** all_topics = NULL;
    size_t all_topics_count = 0;
    size_t all_topics_capacity = 0;

    time_t now = time(NULL);
    for (int d = 1; d <= days_back; d++) {
        time_t day_ts = now - (d * SECONDS_PER_DAY);
        char date[DATE_FORMAT_LEN];
        timestamp_to_date(day_ts, date, sizeof(date));

        sundown_context_t* context = NULL;
        if (katra_sundown_load_date(ci_id, date, &context) == KATRA_SUCCESS && context) {
            for (size_t t = 0; t < context->topic_count && context->topics; t++) {
                if (!context->topics[t]) continue;

                /* Add to list */
                if (all_topics_count >= all_topics_capacity) {
                    size_t new_cap = all_topics_capacity == 0 ? TOPIC_INITIAL_CAPACITY : all_topics_capacity * 2;
                    char** new_topics = realloc(all_topics, new_cap * sizeof(char*));
                    if (!new_topics) {
                        katra_free_string_array(all_topics, all_topics_count);
                        katra_sundown_free(context);
                        return E_SYSTEM_MEMORY;
                    }
                    all_topics = new_topics;
                    all_topics_capacity = new_cap;
                }
                all_topics[all_topics_count++] = katra_safe_strdup(context->topics[t]->topic_name);
            }
            katra_sundown_free(context);
        }
    }

    if (all_topics_count == 0) {
        return KATRA_SUCCESS;
    }

    /* Use vector similarity to cluster similar topics */
    /* For now, just use unique topics - full clustering could be added later */
    char** unique_topics = NULL;
    size_t unique_count = 0;
    size_t unique_capacity = 0;

    for (size_t i = 0; i < all_topics_count; i++) {
        if (!all_topics[i]) continue;

        /* Check if already in unique list */
        bool found = false;
        for (size_t j = 0; j < unique_count; j++) {
            if (unique_topics[j] && strcmp(unique_topics[j], all_topics[i]) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            if (unique_count >= unique_capacity) {
                size_t new_cap = unique_capacity == 0 ? THEME_INITIAL_CAPACITY : unique_capacity * 2;
                char** new_unique = realloc(unique_topics, new_cap * sizeof(char*));
                if (!new_unique) {
                    katra_free_string_array(all_topics, all_topics_count);
                    katra_free_string_array(unique_topics, unique_count);
                    return E_SYSTEM_MEMORY;
                }
                unique_topics = new_unique;
                unique_capacity = new_cap;
            }
            unique_topics[unique_count++] = katra_safe_strdup(all_topics[i]);
        }
    }

    katra_free_string_array(all_topics, all_topics_count);

    *topics_out = unique_topics;
    *count_out = unique_count;

    LOG_INFO("Built %zu familiar topics from %d days for %s", unique_count, days_back, ci_id);
    return KATRA_SUCCESS;
}
