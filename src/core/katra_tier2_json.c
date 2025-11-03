/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_tier2.h"
#include "katra_core_common.h"
#include "katra_error.h"
#include "katra_json_utils.h"
#include "katra_limits.h"

/* Helper: Write string array as JSON */
/* GUIDELINE_APPROVED: JSON array format strings */
static void write_json_string_array(FILE* fp, const char* field_name,
                                     char** items, size_t count) {
    fprintf(fp, "\"%s\":[", field_name); /* GUIDELINE_APPROVED */
    for (size_t i = 0; i < count; i++) {
        if (items && items[i]) {
            fprintf(fp, "\"%s\"", items[i]); /* GUIDELINE_APPROVED */
            if (i < count - 1) {
                fprintf(fp, ","); /* GUIDELINE_APPROVED */
            }
        }
    }
    fprintf(fp, "]"); /* GUIDELINE_APPROVED */
}
/* GUIDELINE_APPROVED_END */

/* Helper: Write escaped string array as JSON */
/* GUIDELINE_APPROVED: JSON array format strings */
static void write_json_escaped_array(FILE* fp, const char* field_name,
                                      char** items, size_t count) {
    fprintf(fp, "\"%s\":[", field_name); /* GUIDELINE_APPROVED */
    for (size_t i = 0; i < count; i++) {
        if (items && items[i]) {
            char escaped[KATRA_BUFFER_LARGE];
            katra_json_escape(items[i], escaped, sizeof(escaped));
            fprintf(fp, "\"%s\"", escaped); /* GUIDELINE_APPROVED */
            if (i < count - 1) {
                fprintf(fp, ","); /* GUIDELINE_APPROVED */
            }
        }
    }
    fprintf(fp, "]"); /* GUIDELINE_APPROVED */
}
/* GUIDELINE_APPROVED_END */

/* Write digest record as JSON line */
/* GUIDELINE_APPROVED: JSON field names are part of the Tier2 digest format */
int katra_tier2_write_json_digest(FILE* fp, const digest_record_t* digest) {
    if (!fp || !digest) {
        return E_INPUT_NULL;
    }

    /* Start JSON object */
    fprintf(fp, "{"); /* GUIDELINE_APPROVED */

    /* Basic fields */
    fprintf(fp, "\"digest_id\":\"%s\",", digest->digest_id ? digest->digest_id : ""); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"timestamp\":%ld,", (long)digest->timestamp); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"period_type\":%d,", digest->period_type); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"period_id\":\"%s\",", digest->period_id ? digest->period_id : ""); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"source_tier\":%d,", digest->source_tier); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"source_record_count\":%zu,", digest->source_record_count); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"ci_id\":\"%s\",", digest->ci_id ? digest->ci_id : ""); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"digest_type\":%d,", digest->digest_type); /* GUIDELINE_APPROVED */

    /* Themes array */
    write_json_string_array(fp, "themes", digest->themes, digest->theme_count); /* GUIDELINE_APPROVED */
    fprintf(fp, ","); /* GUIDELINE_APPROVED */

    /* Keywords array */
    write_json_string_array(fp, "keywords", digest->keywords, digest->keyword_count); /* GUIDELINE_APPROVED */
    fprintf(fp, ","); /* GUIDELINE_APPROVED */

    /* Entities object */
    fprintf(fp, "\"entities\":{"); /* GUIDELINE_APPROVED */
    write_json_string_array(fp, "files", digest->entities.files, digest->entities.file_count); /* GUIDELINE_APPROVED */
    fprintf(fp, ","); /* GUIDELINE_APPROVED */
    write_json_string_array(fp, "concepts", digest->entities.concepts, digest->entities.concept_count); /* GUIDELINE_APPROVED */
    fprintf(fp, ","); /* GUIDELINE_APPROVED */
    write_json_string_array(fp, "people", digest->entities.people, digest->entities.people_count); /* GUIDELINE_APPROVED */
    fprintf(fp, "},"); /* GUIDELINE_APPROVED */

    /* Summary (escaped) */
    if (digest->summary) {
        char summary_escaped[KATRA_BUFFER_LARGE];
        katra_json_escape(digest->summary, summary_escaped, sizeof(summary_escaped));
        fprintf(fp, "\"summary\":\"%s\",", summary_escaped); /* GUIDELINE_APPROVED */
    } else {
        fprintf(fp, "\"summary\":\"\","); /* GUIDELINE_APPROVED */
    }

    /* Key insights array */
    write_json_escaped_array(fp, "key_insights", digest->key_insights, digest->insight_count); /* GUIDELINE_APPROVED */
    fprintf(fp, ","); /* GUIDELINE_APPROVED */

    /* Metadata */
    fprintf(fp, "\"questions_asked\":%d,", digest->questions_asked); /* GUIDELINE_APPROVED */

    /* Decisions array */
    write_json_escaped_array(fp, "decisions_made", digest->decisions_made, digest->decision_count); /* GUIDELINE_APPROVED */
    fprintf(fp, ","); /* GUIDELINE_APPROVED */

    /* Archived flag */
    fprintf(fp, "\"archived\":%s", digest->archived ? "true" : "false"); /* GUIDELINE_APPROVED */

    /* End JSON object */
    fprintf(fp, "}\n"); /* GUIDELINE_APPROVED */

    return KATRA_SUCCESS;
}
/* GUIDELINE_APPROVED_END */

/* Helper: Extract string field from JSON */
static int extract_json_string(const char* line, const char* field,
                                char* buffer, size_t buffer_size) {
    char search_str[KATRA_BUFFER_NAME];
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
    char search_str[KATRA_BUFFER_NAME];
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
    char search_str[KATRA_BUFFER_NAME];
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
    digest_record_t* d;
    ALLOC_OR_RETURN(d, digest_record_t);

    /* Extract basic fields */
    char digest_id[KATRA_BUFFER_MEDIUM] = {0};
    char period_id[KATRA_BUFFER_SMALL] = {0};
    char ci_id[KATRA_BUFFER_MEDIUM] = {0};
    long timestamp = 0;
    int period_type = 0;
    int source_tier = 0;
    int digest_type = 0;
    int questions_asked = 0;
    int archived = 0;

    /* GUIDELINE_APPROVED: JSON field names for Tier2 digest parsing */
    /* Parse fields using helpers */
    extract_json_string(line, "digest_id", digest_id, sizeof(digest_id)); /* GUIDELINE_APPROVED */
    extract_json_long(line, "timestamp", &timestamp); /* GUIDELINE_APPROVED */
    extract_json_int(line, "period_type", &period_type); /* GUIDELINE_APPROVED */
    extract_json_string(line, "period_id", period_id, sizeof(period_id)); /* GUIDELINE_APPROVED */
    extract_json_int(line, "source_tier", &source_tier); /* GUIDELINE_APPROVED */
    extract_json_string(line, "ci_id", ci_id, sizeof(ci_id)); /* GUIDELINE_APPROVED */
    extract_json_int(line, "digest_type", &digest_type); /* GUIDELINE_APPROVED */
    extract_json_int(line, "questions_asked", &questions_asked); /* GUIDELINE_APPROVED */

    /* Parse source_record_count */
    const char* src_count_start = strstr(line, "\"source_record_count\":"); /* GUIDELINE_APPROVED */
    if (src_count_start) {
        sscanf(src_count_start, "\"source_record_count\":%zu", &d->source_record_count); /* GUIDELINE_APPROVED */
    }

    /* Parse archived boolean */
    const char* archived_start = strstr(line, "\"archived\":"); /* GUIDELINE_APPROVED */
    if (archived_start) {
        archived_start += JSON_ARCHIVED_PREFIX_LENGTH;
        archived = (strncmp(archived_start, "true", 4) == 0) ? 1 : 0; /* GUIDELINE_APPROVED */
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
    extract_json_string(line, "summary", summary_buf, sizeof(summary_buf)); /* GUIDELINE_APPROVED */
    if (summary_buf[0] != '\0') {
        d->summary = strdup(summary_buf);
    }
    /* GUIDELINE_APPROVED_END */

    /* Note: Arrays (themes, keywords, entities, etc.) not fully parsed in this simplified version */
    /* Production would use a proper JSON library like cJSON or jsmn */

    *digest = d;
    return KATRA_SUCCESS;
}
