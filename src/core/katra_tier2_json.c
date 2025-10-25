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
    fprintf(fp, "\"themes\":[");
    for (size_t i = 0; i < digest->theme_count; i++) {
        if (digest->themes && digest->themes[i]) {
            fprintf(fp, "\"%s\"", digest->themes[i]);
            if (i < digest->theme_count - 1) {
                fprintf(fp, ",");
            }
        }
    }
    fprintf(fp, "],");

    /* Keywords array */
    fprintf(fp, "\"keywords\":[");
    for (size_t i = 0; i < digest->keyword_count; i++) {
        if (digest->keywords && digest->keywords[i]) {
            fprintf(fp, "\"%s\"", digest->keywords[i]);
            if (i < digest->keyword_count - 1) {
                fprintf(fp, ",");
            }
        }
    }
    fprintf(fp, "],");

    /* Entities object */
    fprintf(fp, "\"entities\":{");
    fprintf(fp, "\"files\":[");
    for (size_t i = 0; i < digest->entities.file_count; i++) {
        if (digest->entities.files && digest->entities.files[i]) {
            fprintf(fp, "\"%s\"", digest->entities.files[i]);
            if (i < digest->entities.file_count - 1) {
                fprintf(fp, ",");
            }
        }
    }
    fprintf(fp, "],");
    fprintf(fp, "\"concepts\":[");
    for (size_t i = 0; i < digest->entities.concept_count; i++) {
        if (digest->entities.concepts && digest->entities.concepts[i]) {
            fprintf(fp, "\"%s\"", digest->entities.concepts[i]);
            if (i < digest->entities.concept_count - 1) {
                fprintf(fp, ",");
            }
        }
    }
    fprintf(fp, "],");
    fprintf(fp, "\"people\":[");
    for (size_t i = 0; i < digest->entities.people_count; i++) {
        if (digest->entities.people && digest->entities.people[i]) {
            fprintf(fp, "\"%s\"", digest->entities.people[i]);
            if (i < digest->entities.people_count - 1) {
                fprintf(fp, ",");
            }
        }
    }
    fprintf(fp, "]");
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
    fprintf(fp, "\"key_insights\":[");
    for (size_t i = 0; i < digest->insight_count; i++) {
        if (digest->key_insights && digest->key_insights[i]) {
            char insight_escaped[KATRA_BUFFER_LARGE];
            katra_json_escape(digest->key_insights[i], insight_escaped, sizeof(insight_escaped));
            fprintf(fp, "\"%s\"", insight_escaped);
            if (i < digest->insight_count - 1) {
                fprintf(fp, ",");
            }
        }
    }
    fprintf(fp, "],");

    /* Metadata */
    fprintf(fp, "\"questions_asked\":%d,", digest->questions_asked);

    /* Decisions array */
    fprintf(fp, "\"decisions_made\":[");
    for (size_t i = 0; i < digest->decision_count; i++) {
        if (digest->decisions_made && digest->decisions_made[i]) {
            char decision_escaped[KATRA_BUFFER_LARGE];
            katra_json_escape(digest->decisions_made[i], decision_escaped, sizeof(decision_escaped));
            fprintf(fp, "\"%s\"", decision_escaped);
            if (i < digest->decision_count - 1) {
                fprintf(fp, ",");
            }
        }
    }
    fprintf(fp, "],");

    /* Archived flag */
    fprintf(fp, "\"archived\":%s", digest->archived ? "true" : "false");

    /* End JSON object */
    fprintf(fp, "}\n");

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

    /* Extract basic fields using sscanf */
    char digest_id[256] = {0};
    char period_id[64] = {0};
    char ci_id[256] = {0};
    long timestamp = 0;
    int period_type = 0;
    int source_tier = 0;
    int digest_type = 0;
    int questions_asked = 0;
    int archived = 0;

    /* Parse main fields - simplified parsing, production would use JSON library */
    const char* ptr = line;

    /* digest_id */
    const char* digest_id_start = strstr(ptr, "\"digest_id\":\"");
    if (digest_id_start) {
        digest_id_start += 13;
        const char* digest_id_end = strchr(digest_id_start, '"');
        if (digest_id_end) {
            size_t len = digest_id_end - digest_id_start;
            if (len < sizeof(digest_id)) {
                strncpy(digest_id, digest_id_start, len);
                digest_id[len] = '\0';
            }
        }
    }

    /* timestamp */
    const char* timestamp_start = strstr(ptr, "\"timestamp\":");
    if (timestamp_start) {
        sscanf(timestamp_start, "\"timestamp\":%ld", &timestamp);
    }

    /* period_type */
    const char* period_type_start = strstr(ptr, "\"period_type\":");
    if (period_type_start) {
        sscanf(period_type_start, "\"period_type\":%d", &period_type);
    }

    /* period_id */
    const char* period_id_start = strstr(ptr, "\"period_id\":\"");
    if (period_id_start) {
        period_id_start += 13;
        const char* period_id_end = strchr(period_id_start, '"');
        if (period_id_end) {
            size_t len = period_id_end - period_id_start;
            if (len < sizeof(period_id)) {
                strncpy(period_id, period_id_start, len);
                period_id[len] = '\0';
            }
        }
    }

    /* source_tier */
    const char* source_tier_start = strstr(ptr, "\"source_tier\":");
    if (source_tier_start) {
        sscanf(source_tier_start, "\"source_tier\":%d", &source_tier);
    }

    /* source_record_count */
    const char* src_count_start = strstr(ptr, "\"source_record_count\":");
    if (src_count_start) {
        sscanf(src_count_start, "\"source_record_count\":%zu", &d->source_record_count);
    }

    /* ci_id */
    const char* ci_id_start = strstr(ptr, "\"ci_id\":\"");
    if (ci_id_start) {
        ci_id_start += 9;
        const char* ci_id_end = strchr(ci_id_start, '"');
        if (ci_id_end) {
            size_t len = ci_id_end - ci_id_start;
            if (len < sizeof(ci_id)) {
                strncpy(ci_id, ci_id_start, len);
                ci_id[len] = '\0';
            }
        }
    }

    /* digest_type */
    const char* digest_type_start = strstr(ptr, "\"digest_type\":");
    if (digest_type_start) {
        sscanf(digest_type_start, "\"digest_type\":%d", &digest_type);
    }

    /* questions_asked */
    const char* questions_start = strstr(ptr, "\"questions_asked\":");
    if (questions_start) {
        sscanf(questions_start, "\"questions_asked\":%d", &questions_asked);
    }

    /* archived */
    const char* archived_start = strstr(ptr, "\"archived\":");
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

    /* Note: Arrays (themes, keywords, entities, etc.) not parsed in this simplified version */
    /* Production would use a proper JSON library like cJSON or jsmn */

    *digest = d;
    return KATRA_SUCCESS;
}
