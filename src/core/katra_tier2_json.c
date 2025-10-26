/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_tier2.h"
#include "katra_error.h"
#include "katra_json_utils.h"
#include "katra_limits.h"

/* Helper: Write string array as JSON */
static void write_json_string_array(FILE* fp, const char* field_name,
                                     char** items, size_t count) {
    fprintf(fp, "\"%s\":[", field_name);
    for (size_t i = 0; i < count; i++) {
        if (items && items[i]) {
            fprintf(fp, "\"%s\"", items[i]);
            if (i < count - 1) {
                fprintf(fp, ",");
            }
        }
    }
    fprintf(fp, "]");
}

/* Helper: Write escaped string array as JSON */
static void write_json_escaped_array(FILE* fp, const char* field_name,
                                      char** items, size_t count) {
    fprintf(fp, "\"%s\":[", field_name);
    for (size_t i = 0; i < count; i++) {
        if (items && items[i]) {
            char escaped[KATRA_BUFFER_LARGE];
            katra_json_escape(items[i], escaped, sizeof(escaped));
            fprintf(fp, "\"%s\"", escaped);
            if (i < count - 1) {
                fprintf(fp, ",");
            }
        }
    }
    fprintf(fp, "]");
}

/* Write digest record as JSON line */
int katra_tier2_write_json_digest(FILE* fp, const digest_record_t* digest) {
    if (!fp || !digest) {
        return E_INPUT_NULL;
    }

    /* Start JSON object */
    fprintf(fp, "{");

    /* Basic fields */
    fprintf(fp, "\"digest_id\":\"%s\",", digest->digest_id ? digest->digest_id : "");
    fprintf(fp, "\"timestamp\":%ld,", (long)digest->timestamp);
    fprintf(fp, "\"period_type\":%d,", digest->period_type);
    fprintf(fp, "\"period_id\":\"%s\",", digest->period_id ? digest->period_id : "");
    fprintf(fp, "\"source_tier\":%d,", digest->source_tier);
    fprintf(fp, "\"source_record_count\":%zu,", digest->source_record_count);
    fprintf(fp, "\"ci_id\":\"%s\",", digest->ci_id ? digest->ci_id : "");
    fprintf(fp, "\"digest_type\":%d,", digest->digest_type);

    /* Themes array */
    write_json_string_array(fp, "themes", digest->themes, digest->theme_count);
    fprintf(fp, ",");

    /* Keywords array */
    write_json_string_array(fp, "keywords", digest->keywords, digest->keyword_count);
    fprintf(fp, ",");

    /* Entities object */
    fprintf(fp, "\"entities\":{");
    write_json_string_array(fp, "files", digest->entities.files, digest->entities.file_count);
    fprintf(fp, ",");
    write_json_string_array(fp, "concepts", digest->entities.concepts, digest->entities.concept_count);
    fprintf(fp, ",");
    write_json_string_array(fp, "people", digest->entities.people, digest->entities.people_count);
    fprintf(fp, "},");

    /* Summary (escaped) */
    if (digest->summary) {
        char summary_escaped[KATRA_BUFFER_LARGE];
        katra_json_escape(digest->summary, summary_escaped, sizeof(summary_escaped));
        fprintf(fp, "\"summary\":\"%s\",", summary_escaped);
    } else {
        fprintf(fp, "\"summary\":\"\",");
    }

    /* Key insights array */
    write_json_escaped_array(fp, "key_insights", digest->key_insights, digest->insight_count);
    fprintf(fp, ",");

    /* Metadata */
    fprintf(fp, "\"questions_asked\":%d,", digest->questions_asked);

    /* Decisions array */
    write_json_escaped_array(fp, "decisions_made", digest->decisions_made, digest->decision_count);
    fprintf(fp, ",");

    /* Archived flag */
    fprintf(fp, "\"archived\":%s", digest->archived ? "true" : "false");

    /* End JSON object */
    fprintf(fp, "}\n");

    return KATRA_SUCCESS;
}

/* Helper: Extract string field from JSON */
static int extract_json_string(const char* line, const char* field,
                                char* buffer, size_t buffer_size) {
    char search_str[128];
    snprintf(search_str, sizeof(search_str), "\"%s\":\"", field);

    const char* start = strstr(line, search_str);
    if (!start) {
        buffer[0] = '\0';
        return KATRA_SUCCESS;
    }

    start += strlen(search_str);
    const char* end = strchr(start, '"');
    if (!end) {
        buffer[0] = '\0';
        return KATRA_SUCCESS;
    }

    size_t len = end - start;
    if (len >= buffer_size) {
        len = buffer_size - 1;
    }

    strncpy(buffer, start, len);
    buffer[len] = '\0';
    return KATRA_SUCCESS;
}

/* Helper: Extract int field from JSON */
static int extract_json_int(const char* line, const char* field, int* value) {
    char search_str[128];
    snprintf(search_str, sizeof(search_str), "\"%s\":", field);

    const char* start = strstr(line, search_str);
    if (!start) {
        return E_SYSTEM_FILE;
    }

    sscanf(start + strlen(search_str), "%d", value);
    return KATRA_SUCCESS;
}

/* Helper: Extract long field from JSON */
static int extract_json_long(const char* line, const char* field, long* value) {
    char search_str[128];
    snprintf(search_str, sizeof(search_str), "\"%s\":", field);

    const char* start = strstr(line, search_str);
    if (!start) {
        return E_SYSTEM_FILE;
    }

    sscanf(start + strlen(search_str), "%ld", value);
    return KATRA_SUCCESS;
}

/* Parse digest record from JSON line */
int katra_tier2_parse_json_digest(const char* line, digest_record_t** digest) {
    if (!line || !digest) {
        return E_INPUT_NULL;
    }

    /* Allocate digest structure */
    digest_record_t* d = calloc(1, sizeof(digest_record_t));
    if (!d) {
        return E_SYSTEM_MEMORY;
    }

    /* Extract basic fields */
    char digest_id[256] = {0};
    char period_id[64] = {0};
    char ci_id[256] = {0};
    long timestamp = 0;
    int period_type = 0;
    int source_tier = 0;
    int digest_type = 0;
    int questions_asked = 0;
    int archived = 0;

    /* Parse fields using helpers */
    extract_json_string(line, "digest_id", digest_id, sizeof(digest_id));
    extract_json_long(line, "timestamp", &timestamp);
    extract_json_int(line, "period_type", &period_type);
    extract_json_string(line, "period_id", period_id, sizeof(period_id));
    extract_json_int(line, "source_tier", &source_tier);
    extract_json_string(line, "ci_id", ci_id, sizeof(ci_id));
    extract_json_int(line, "digest_type", &digest_type);
    extract_json_int(line, "questions_asked", &questions_asked);

    /* Parse source_record_count */
    const char* src_count_start = strstr(line, "\"source_record_count\":");
    if (src_count_start) {
        sscanf(src_count_start, "\"source_record_count\":%zu", &d->source_record_count);
    }

    /* Parse archived boolean */
    const char* archived_start = strstr(line, "\"archived\":");
    if (archived_start) {
        archived_start += 11;
        archived = (strncmp(archived_start, "true", 4) == 0) ? 1 : 0;
    }

    /* Allocate and copy string fields */
    d->digest_id = strdup(digest_id);
    d->period_id = strdup(period_id);
    d->ci_id = strdup(ci_id);
    d->timestamp = (time_t)timestamp;
    d->period_type = (period_type_t)period_type;
    d->source_tier = source_tier;
    d->digest_type = (digest_type_t)digest_type;
    d->questions_asked = questions_asked;
    d->archived = (bool)archived;

    /* Check critical allocations */
    if (!d->digest_id || !d->period_id || !d->ci_id) {
        katra_digest_free(d);
        return E_SYSTEM_MEMORY;
    }

    /* Parse summary field */
    char summary_buf[KATRA_BUFFER_LARGE];
    extract_json_string(line, "summary", summary_buf, sizeof(summary_buf));
    if (summary_buf[0] != '\0') {
        d->summary = strdup(summary_buf);
    }

    /* Note: Arrays (themes, keywords, entities, etc.) not fully parsed in this simplified version */
    /* Production would use a proper JSON library like cJSON or jsmn */

    *digest = d;
    return KATRA_SUCCESS;
}
