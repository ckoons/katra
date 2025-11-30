/* © 2025 Casey Koons All rights reserved */

/*
 * katra_sunrise_sunset_json.c - Sunrise/Sunset JSON Persistence (Phase 7.3-7.6)
 *
 * Provides JSON serialization and persistence for sunrise/sunset contexts,
 * enabling cross-session continuity for CIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <jansson.h>

#include "katra_sunrise_sunset.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_core_common.h"
#include "katra_psyche_common.h"

/* JSON field names */
#define JSON_CI_ID "ci_id"
#define JSON_TIMESTAMP "timestamp"
#define JSON_STATS "stats"
#define JSON_MOOD_ARC "mood_arc"
#define JSON_DOMINANT_MOOD "dominant_mood"
#define JSON_TOPICS "topics"
#define JSON_THREADS "threads"
#define JSON_INSIGHTS "insights"
#define JSON_OPEN_QUESTIONS "open_questions"
#define JSON_INTENTIONS "intentions"
#define JSON_WORKING_MEMORY "working_memory"

/* Emotional tag JSON fields */
#define JSON_VALENCE "valence"
#define JSON_AROUSAL "arousal"
#define JSON_DOMINANCE "dominance"
#define JSON_EMOTION "emotion"

/* Topic cluster fields */
#define JSON_TOPIC_NAME "topic_name"
#define JSON_RECORD_IDS "record_ids"
#define JSON_COHERENCE "coherence"
#define JSON_AVG_EMOTION "avg_emotion"

/* Working memory snapshot fields */
#define JSON_WM_ITEMS "items"
#define JSON_WM_CAPACITY "capacity"
#define JSON_WM_CONSOLIDATIONS "total_consolidations"
#define JSON_WM_LAST_CONSOLIDATION "last_consolidation"
#define JSON_WM_CONTENT "content"
#define JSON_WM_ATTENTION "attention_score"
#define JSON_WM_ADDED "added_time"
#define JSON_WM_ACCESSED "last_accessed"

/* Stats fields - match daily_stats_t in katra_continuity.h */
#define JSON_INTERACTION_COUNT "interaction_count"
#define JSON_QUESTIONS_ASKED "questions_asked"
#define JSON_TASKS_COMPLETED "tasks_completed"
#define JSON_ERRORS_ENCOUNTERED "errors_encountered"
#define JSON_AVG_IMPORTANCE "avg_importance"

/* Error codes from katra_error.h that might not be in scope */
#ifndef E_FILE_NOT_FOUND
#define E_FILE_NOT_FOUND 12
#endif
#ifndef E_JSON_PARSE
#define E_JSON_PARSE 20
#endif

/* Sundown file pattern */
#define SUNDOWN_FILE_PREFIX "sundown_"
#define SUNDOWN_FILE_SUFFIX ".json"
#define SUNDOWN_DIR "sundowns"
#define DATE_FORMAT_LEN 9  /* YYYYMMDD + null */

/* ============================================================================
 * JSON SERIALIZATION HELPERS
 * ============================================================================ */

/* Serialize emotional tag to JSON */
static json_t* emotional_tag_to_json(const emotional_tag_t* emotion) {
    if (!emotion) return json_null();

    json_t* obj = json_object();
    json_object_set_new(obj, JSON_VALENCE, json_real(emotion->valence));
    json_object_set_new(obj, JSON_AROUSAL, json_real(emotion->arousal));
    json_object_set_new(obj, JSON_DOMINANCE, json_real(emotion->dominance));
    json_object_set_new(obj, JSON_EMOTION, json_string(emotion->emotion));
    json_object_set_new(obj, JSON_TIMESTAMP, json_integer(emotion->timestamp));
    return obj;
}

/* Deserialize emotional tag from JSON */
static int json_to_emotional_tag(json_t* obj, emotional_tag_t* emotion) {
    if (!obj || !emotion) return E_INPUT_NULL;

    emotion->valence = (float)json_number_value(json_object_get(obj, JSON_VALENCE));
    emotion->arousal = (float)json_number_value(json_object_get(obj, JSON_AROUSAL));
    emotion->dominance = (float)json_number_value(json_object_get(obj, JSON_DOMINANCE));

    const char* em = json_string_value(json_object_get(obj, JSON_EMOTION));
    if (em) {
        strncpy(emotion->emotion, em, sizeof(emotion->emotion) - 1);
        emotion->emotion[sizeof(emotion->emotion) - 1] = '\0';
    }

    emotion->timestamp = json_integer_value(json_object_get(obj, JSON_TIMESTAMP));
    return KATRA_SUCCESS;
}

/* Serialize string array to JSON */
static json_t* string_array_to_json(char** strings, size_t count) {
    json_t* arr = json_array();
    for (size_t i = 0; i < count && strings && strings[i]; i++) {
        json_array_append_new(arr, json_string(strings[i]));
    }
    return arr;
}

/* Deserialize string array from JSON */
static int json_to_string_array(json_t* arr, char*** strings_out, size_t* count_out) {
    if (!arr || !strings_out || !count_out) return E_INPUT_NULL;

    size_t count = json_array_size(arr);
    if (count == 0) {
        *strings_out = NULL;
        *count_out = 0;
        return KATRA_SUCCESS;
    }

    char** strings = calloc(count, sizeof(char*));
    if (!strings) return E_SYSTEM_MEMORY;

    for (size_t i = 0; i < count; i++) {
        const char* s = json_string_value(json_array_get(arr, i));
        strings[i] = s ? katra_safe_strdup(s) : NULL;
    }

    *strings_out = strings;
    *count_out = count;
    return KATRA_SUCCESS;
}

/* Serialize working memory snapshot to JSON */
static json_t* wm_snapshot_to_json(const wm_state_snapshot_t* snapshot) {
    if (!snapshot) return json_null();

    json_t* obj = json_object();
    json_object_set_new(obj, JSON_WM_CAPACITY, json_integer(snapshot->capacity));
    json_object_set_new(obj, JSON_WM_CONSOLIDATIONS, json_integer(snapshot->total_consolidations));
    json_object_set_new(obj, JSON_WM_LAST_CONSOLIDATION, json_integer(snapshot->last_consolidation));

    json_t* items = json_array();
    for (size_t i = 0; i < snapshot->item_count; i++) {
        json_t* item = json_object();
        json_object_set_new(item, JSON_WM_CONTENT, json_string(snapshot->items[i].content));
        json_object_set_new(item, JSON_WM_ATTENTION, json_real(snapshot->items[i].attention_score));
        json_object_set_new(item, JSON_WM_ADDED, json_integer(snapshot->items[i].added_time));
        json_object_set_new(item, JSON_WM_ACCESSED, json_integer(snapshot->items[i].last_accessed));
        json_array_append_new(items, item);
    }
    json_object_set_new(obj, JSON_WM_ITEMS, items);

    return obj;
}

/* Deserialize working memory snapshot from JSON */
static wm_state_snapshot_t* json_to_wm_snapshot(json_t* obj) {
    if (!obj || json_is_null(obj)) return NULL;

    wm_state_snapshot_t* snapshot = calloc(1, sizeof(wm_state_snapshot_t));
    if (!snapshot) return NULL;

    snapshot->capacity = json_integer_value(json_object_get(obj, JSON_WM_CAPACITY));
    snapshot->total_consolidations = json_integer_value(json_object_get(obj, JSON_WM_CONSOLIDATIONS));
    snapshot->last_consolidation = json_integer_value(json_object_get(obj, JSON_WM_LAST_CONSOLIDATION));

    json_t* items = json_object_get(obj, JSON_WM_ITEMS);
    if (items && json_is_array(items)) {
        snapshot->item_count = json_array_size(items);
        if (snapshot->item_count > 0) {
            snapshot->items = calloc(snapshot->item_count, sizeof(wm_item_snapshot_t));
            if (snapshot->items) {
                for (size_t i = 0; i < snapshot->item_count; i++) {
                    json_t* item = json_array_get(items, i);
                    const char* content = json_string_value(json_object_get(item, JSON_WM_CONTENT));
                    if (content) {
                        strncpy(snapshot->items[i].content, content,
                                sizeof(snapshot->items[i].content) - 1);
                    }
                    snapshot->items[i].attention_score =
                        (float)json_number_value(json_object_get(item, JSON_WM_ATTENTION));
                    snapshot->items[i].added_time =
                        json_integer_value(json_object_get(item, JSON_WM_ADDED));
                    snapshot->items[i].last_accessed =
                        json_integer_value(json_object_get(item, JSON_WM_ACCESSED));
                }
            }
        }
    }

    return snapshot;
}

/* Serialize daily stats to JSON */
static json_t* daily_stats_to_json(const daily_stats_t* stats) {
    json_t* obj = json_object();
    json_object_set_new(obj, JSON_INTERACTION_COUNT, json_integer(stats->interaction_count));
    json_object_set_new(obj, JSON_QUESTIONS_ASKED, json_integer(stats->questions_asked));
    json_object_set_new(obj, JSON_TASKS_COMPLETED, json_integer(stats->tasks_completed));
    json_object_set_new(obj, JSON_ERRORS_ENCOUNTERED, json_integer(stats->errors_encountered));
    json_object_set_new(obj, JSON_AVG_IMPORTANCE, json_real(stats->avg_importance));
    return obj;
}

/* Deserialize daily stats from JSON */
static void json_to_daily_stats(json_t* obj, daily_stats_t* stats) {
    if (!obj || !stats) return;
    stats->interaction_count = (int)json_integer_value(json_object_get(obj, JSON_INTERACTION_COUNT));
    stats->questions_asked = (int)json_integer_value(json_object_get(obj, JSON_QUESTIONS_ASKED));
    stats->tasks_completed = (int)json_integer_value(json_object_get(obj, JSON_TASKS_COMPLETED));
    stats->errors_encountered = (int)json_integer_value(json_object_get(obj, JSON_ERRORS_ENCOUNTERED));
    stats->avg_importance = (float)json_number_value(json_object_get(obj, JSON_AVG_IMPORTANCE));
}

/* Serialize topic cluster to JSON */
static json_t* topic_cluster_to_json(const topic_cluster_t* topic) {
    if (!topic) return json_null();

    json_t* obj = json_object();
    json_object_set_new(obj, JSON_TOPIC_NAME, json_string(topic->topic_name));
    json_object_set_new(obj, JSON_COHERENCE, json_real(topic->coherence));
    json_object_set_new(obj, JSON_AVG_EMOTION, emotional_tag_to_json(&topic->avg_emotion));
    json_object_set_new(obj, JSON_RECORD_IDS, string_array_to_json(topic->record_ids, topic->record_count));

    return obj;
}

/* ============================================================================
 * SUNDOWN SERIALIZATION
 * ============================================================================ */

/* Serialize full sundown context to JSON */
static json_t* sundown_to_json(const sundown_context_t* context) {
    if (!context) return NULL;

    json_t* root = json_object();

    /* Basic fields */
    json_object_set_new(root, JSON_CI_ID, json_string(context->ci_id));
    json_object_set_new(root, JSON_TIMESTAMP, json_integer(context->timestamp));
    json_object_set_new(root, JSON_STATS, daily_stats_to_json(&context->stats));
    json_object_set_new(root, JSON_DOMINANT_MOOD, emotional_tag_to_json(&context->dominant_mood));

    /* Mood arc */
    json_t* mood_arr = json_array();
    for (size_t i = 0; i < context->mood_count && context->mood_arc; i++) {
        json_array_append_new(mood_arr, emotional_tag_to_json(&context->mood_arc[i]));
    }
    json_object_set_new(root, JSON_MOOD_ARC, mood_arr);

    /* Topics */
    json_t* topics_arr = json_array();
    for (size_t i = 0; i < context->topic_count && context->topics; i++) {
        json_array_append_new(topics_arr, topic_cluster_to_json(context->topics[i]));
    }
    json_object_set_new(root, JSON_TOPICS, topics_arr);

    /* String arrays */
    json_object_set_new(root, JSON_OPEN_QUESTIONS,
                       string_array_to_json(context->open_questions, context->question_count));
    json_object_set_new(root, JSON_INTENTIONS,
                       string_array_to_json(context->intentions, context->intention_count));

    /* Working memory */
    json_object_set_new(root, JSON_WORKING_MEMORY, wm_snapshot_to_json(context->working_memory));

    return root;
}

/* Deserialize sundown context from JSON */
static sundown_context_t* json_to_sundown(json_t* root) {
    if (!root) return NULL;

    sundown_context_t* context = calloc(1, sizeof(sundown_context_t));
    if (!context) return NULL;

    /* Basic fields */
    const char* ci_id = json_string_value(json_object_get(root, JSON_CI_ID));
    if (ci_id) {
        strncpy(context->ci_id, ci_id, sizeof(context->ci_id) - 1);
    }
    context->timestamp = json_integer_value(json_object_get(root, JSON_TIMESTAMP));

    /* Stats */
    json_to_daily_stats(json_object_get(root, JSON_STATS), &context->stats);

    /* Dominant mood */
    json_to_emotional_tag(json_object_get(root, JSON_DOMINANT_MOOD), &context->dominant_mood);

    /* Mood arc */
    json_t* mood_arr = json_object_get(root, JSON_MOOD_ARC);
    if (mood_arr && json_is_array(mood_arr)) {
        context->mood_count = json_array_size(mood_arr);
        if (context->mood_count > 0) {
            context->mood_arc = calloc(context->mood_count, sizeof(emotional_tag_t));
            if (context->mood_arc) {
                for (size_t i = 0; i < context->mood_count; i++) {
                    json_to_emotional_tag(json_array_get(mood_arr, i), &context->mood_arc[i]);
                }
            }
        }
    }

    /* String arrays */
    json_to_string_array(json_object_get(root, JSON_OPEN_QUESTIONS),
                        &context->open_questions, &context->question_count);
    json_to_string_array(json_object_get(root, JSON_INTENTIONS),
                        &context->intentions, &context->intention_count);

    /* Working memory */
    context->working_memory = json_to_wm_snapshot(json_object_get(root, JSON_WORKING_MEMORY));

    return context;
}

/* ============================================================================
 * FILE OPERATIONS
 * ============================================================================ */

/* Build sundown file path for a given CI and date */
static int build_sundown_path(const char* ci_id, const char* date, char* path_out, size_t path_size) {
    char base_path[KATRA_PATH_MAX];

    /* Get user data directory ~/.katra/{ci_id}/sundowns/ */
    int result = katra_build_path(base_path, sizeof(base_path), ci_id, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Create sundowns subdirectory path */
    char sundown_dir[KATRA_PATH_MAX];
    snprintf(sundown_dir, sizeof(sundown_dir), "%s/%s", base_path, SUNDOWN_DIR);

    /* Ensure directory exists */
    result = katra_ensure_dir(sundown_dir);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Build full file path */
    snprintf(path_out, path_size, "%s/%s%s%s", sundown_dir, SUNDOWN_FILE_PREFIX, date, SUNDOWN_FILE_SUFFIX);

    return KATRA_SUCCESS;
}

/* Get date string from timestamp */
static void timestamp_to_date(time_t ts, char* date_out, size_t size) {
    struct tm* tm_info = localtime(&ts);
    strftime(date_out, size, "%Y%m%d", tm_info);
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

/* Save sundown context to disk (Phase 7.3) */
int katra_sundown_save(const sundown_context_t* context) {
    if (!context) {
        return E_INPUT_NULL;
    }

    /* Get date from timestamp */
    char date[DATE_FORMAT_LEN];
    timestamp_to_date(context->timestamp, date, sizeof(date));

    /* Build file path */
    char path[KATRA_PATH_MAX];
    int result = build_sundown_path(context->ci_id, date, path, sizeof(path));
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, __func__, "Failed to build sundown path");
        return result;
    }

    /* Serialize to JSON */
    json_t* root = sundown_to_json(context);
    if (!root) {
        katra_report_error(E_SYSTEM_MEMORY, __func__, "Failed to serialize sundown");
        return E_SYSTEM_MEMORY;
    }

    /* Write to file */
    result = json_dump_file(root, path, JSON_INDENT(2));
    json_decref(root);

    if (result != 0) {
        katra_report_error(E_SYSTEM_FILE, __func__, "Failed to write sundown file");
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Saved sundown context for %s to %s", context->ci_id, path);
    return KATRA_SUCCESS;
}

/* Load sundown context for specific date */
int katra_sundown_load_date(const char* ci_id, const char* date, sundown_context_t** context_out) {
    if (!ci_id || !date || !context_out) {
        return E_INPUT_NULL;
    }

    /* Build file path */
    char path[KATRA_PATH_MAX];
    int result = build_sundown_path(ci_id, date, path, sizeof(path));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Check if file exists */
    struct stat st;
    if (stat(path, &st) != 0) {
        return E_FILE_NOT_FOUND;
    }

    /* Load JSON */
    json_error_t error;
    json_t* root = json_load_file(path, 0, &error);
    if (!root) {
        LOG_ERROR("Failed to load sundown JSON: %s", error.text);
        return E_JSON_PARSE;
    }

    /* Deserialize */
    sundown_context_t* context = json_to_sundown(root);
    json_decref(root);

    if (!context) {
        return E_SYSTEM_MEMORY;
    }

    *context_out = context;
    LOG_INFO("Loaded sundown context for %s from %s", ci_id, path);
    return KATRA_SUCCESS;
}

/* Load most recent sundown context (Phase 7.4) */
int katra_sundown_load_latest(const char* ci_id, sundown_context_t** context_out) {
    if (!ci_id || !context_out) {
        return E_INPUT_NULL;
    }

    /* Build sundowns directory path */
    char base_path[KATRA_PATH_MAX];
    int result = katra_build_path(base_path, sizeof(base_path), ci_id, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    char sundown_dir[KATRA_PATH_MAX];
    snprintf(sundown_dir, sizeof(sundown_dir), "%s/%s", base_path, SUNDOWN_DIR);

    /* Open directory */
    DIR* dir = opendir(sundown_dir);
    if (!dir) {
        return E_FILE_NOT_FOUND;
    }

    /* Find most recent sundown file */
    char latest_date[DATE_FORMAT_LEN] = "";
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        /* Check if file matches sundown pattern */
        if (strncmp(entry->d_name, SUNDOWN_FILE_PREFIX, strlen(SUNDOWN_FILE_PREFIX)) == 0) {
            /* Extract date from filename */
            const char* date_start = entry->d_name + strlen(SUNDOWN_FILE_PREFIX);
            char date[DATE_FORMAT_LEN];
            strncpy(date, date_start, DATE_FORMAT_LEN - 1);
            date[DATE_FORMAT_LEN - 1] = '\0';

            /* Remove .json suffix */
            char* dot = strchr(date, '.');
            if (dot) *dot = '\0';

            /* Compare dates (lexicographic comparison works for YYYYMMDD) */
            if (strcmp(date, latest_date) > 0) {
                strncpy(latest_date, date, DATE_FORMAT_LEN);
            }
        }
    }
    closedir(dir);

    if (strlen(latest_date) == 0) {
        return E_FILE_NOT_FOUND;
    }

    /* Load the latest sundown */
    return katra_sundown_load_date(ci_id, latest_date, context_out);
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

    if (days_back <= 0) days_back = 7;  /* Default to 7 days */

    /* Topic occurrence counting */
    typedef struct {
        char name[128];
        int occurrences;
    } topic_count_t;

    topic_count_t* topic_counts = NULL;
    size_t topic_count_size = 0;
    size_t topic_count_capacity = 0;

    /* Load sundowns for each day */
    time_t now = time(NULL);
    for (int d = 1; d <= days_back; d++) {
        time_t day_ts = now - (d * 86400);  /* 86400 seconds per day */
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
                        size_t new_cap = topic_count_capacity == 0 ? 16 : topic_count_capacity * 2;
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

    if (days_back <= 0) days_back = 7;

    /* Collect all topic names from recent days */
    char** all_topics = NULL;
    size_t all_topics_count = 0;
    size_t all_topics_capacity = 0;

    time_t now = time(NULL);
    for (int d = 1; d <= days_back; d++) {
        time_t day_ts = now - (d * 86400);
        char date[DATE_FORMAT_LEN];
        timestamp_to_date(day_ts, date, sizeof(date));

        sundown_context_t* context = NULL;
        if (katra_sundown_load_date(ci_id, date, &context) == KATRA_SUCCESS && context) {
            for (size_t t = 0; t < context->topic_count && context->topics; t++) {
                if (!context->topics[t]) continue;

                /* Add to list */
                if (all_topics_count >= all_topics_capacity) {
                    size_t new_cap = all_topics_capacity == 0 ? 32 : all_topics_capacity * 2;
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
                size_t new_cap = unique_capacity == 0 ? 16 : unique_capacity * 2;
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
