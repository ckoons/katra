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

/* ============================================================================
 * TOON (Token-Oriented Object Notation) Serialization
 * ============================================================================
 *
 * TOON provides 50-60% token reduction compared to JSON for tier-2 digests.
 * Perfect for LLM context loading in sunrise.md.
 */

/* Helper: Escape commas and newlines in TOON strings */
static void toon_escape_string(const char* input, char* output, size_t output_size) {
    size_t in_idx = 0;
    size_t out_idx = 0;

    while (input && input[in_idx] && out_idx < output_size - 1) {
        if (input[in_idx] == ',') {
            if (out_idx < output_size - 3) {
                output[out_idx++] = '\\';
                output[out_idx++] = ',';
            }
        } else if (input[in_idx] == '\n') {
            if (out_idx < output_size - 2) {
                output[out_idx++] = ' ';  /* Replace newlines with spaces */
            }
        } else {
            output[out_idx++] = input[in_idx];
        }
        in_idx++;
    }
    output[out_idx] = '\0';
}

/* Helper: Period type to string */
static const char* period_type_to_string(period_type_t type) {
    switch (type) {
        case PERIOD_TYPE_WEEKLY: return "weekly";
        case PERIOD_TYPE_MONTHLY: return "monthly";
        default: return "unknown";
    }
}

/* Helper: Digest type to string */
static const char* digest_type_to_string(digest_type_t type) {
    switch (type) {
        case DIGEST_TYPE_INTERACTION: return "interaction";
        case DIGEST_TYPE_LEARNING: return "learning";
        case DIGEST_TYPE_PROJECT: return "project";
        case DIGEST_TYPE_MIXED: return "mixed";
        default: return "unknown";
    }
}

int katra_tier2_digest_to_toon(const digest_record_t* digest, char** toon_out) {
    KATRA_CHECK_NULL(digest);
    KATRA_CHECK_NULL(toon_out);

    /* Allocate buffer - tier-2 digests can be larger than session state */
    size_t buffer_size = 16384;  /* 16KB should handle most digests */
    char* buffer = malloc(buffer_size);
    if (!buffer) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_tier2_digest_to_toon",
                          "Failed to allocate buffer");
        return E_SYSTEM_MEMORY;
    }

    size_t offset = 0;
    int written;
    char escaped[KATRA_BUFFER_LARGE];

    /* Digest header - compact metadata */
    written = snprintf(buffer + offset, buffer_size - offset,
                      "digest[%s,%s,%s,%s]:\n"
                      "  id,period,period_type,digest_type\n\n",
                      digest->digest_id ? digest->digest_id : "unknown",
                      digest->period_id ? digest->period_id : "unknown",
                      period_type_to_string(digest->period_type),
                      digest_type_to_string(digest->digest_type));
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    /* Source metadata */
    written = snprintf(buffer + offset, buffer_size - offset,
                      "source: tier_%d (%zu records)\n\n",
                      digest->source_tier,
                      digest->source_record_count);
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    /* Themes - simple list */
    if (digest->theme_count > 0 && digest->themes) {
        written = snprintf(buffer + offset, buffer_size - offset,
                          "themes[%zu]:\n", digest->theme_count);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;

        for (size_t i = 0; i < digest->theme_count && i < TIER2_MAX_THEMES; i++) {
            if (digest->themes[i]) {
                toon_escape_string(digest->themes[i], escaped, sizeof(escaped));
                written = snprintf(buffer + offset, buffer_size - offset,
                                  "  %s\n", escaped);
                if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
                offset += (size_t)written;
            }
        }
        written = snprintf(buffer + offset, buffer_size - offset, "\n");
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;
    }

    /* Keywords - comma-separated on one line for compactness */
    if (digest->keyword_count > 0 && digest->keywords) {
        written = snprintf(buffer + offset, buffer_size - offset,
                          "keywords[%zu]: ", digest->keyword_count);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;

        for (size_t i = 0; i < digest->keyword_count && i < TIER2_MAX_KEYWORDS; i++) {
            if (digest->keywords[i]) {
                written = snprintf(buffer + offset, buffer_size - offset,
                                  "%s%s",
                                  digest->keywords[i],
                                  (i < digest->keyword_count - 1) ? "," : "");
                if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
                offset += (size_t)written;
            }
        }
        written = snprintf(buffer + offset, buffer_size - offset, "\n\n");
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;
    }

    /* Summary - prose text */
    if (digest->summary) {
        toon_escape_string(digest->summary, escaped, sizeof(escaped));
        written = snprintf(buffer + offset, buffer_size - offset,
                          "summary:\n  %s\n\n", escaped);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;
    }

    /* Key insights - list */
    if (digest->insight_count > 0 && digest->key_insights) {
        written = snprintf(buffer + offset, buffer_size - offset,
                          "insights[%zu]:\n", digest->insight_count);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;

        for (size_t i = 0; i < digest->insight_count && i < TIER2_MAX_INSIGHTS; i++) {
            if (digest->key_insights[i]) {
                toon_escape_string(digest->key_insights[i], escaped, sizeof(escaped));
                written = snprintf(buffer + offset, buffer_size - offset,
                                  "  %s\n", escaped);
                if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
                offset += (size_t)written;
            }
        }
        written = snprintf(buffer + offset, buffer_size - offset, "\n");
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;
    }

    /* Questions and decisions */
    if (digest->questions_asked > 0) {
        written = snprintf(buffer + offset, buffer_size - offset,
                          "questions_asked: %d\n", digest->questions_asked);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;
    }

    if (digest->decision_count > 0 && digest->decisions_made) {
        written = snprintf(buffer + offset, buffer_size - offset,
                          "decisions[%zu]:\n", digest->decision_count);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;

        for (size_t i = 0; i < digest->decision_count; i++) {
            if (digest->decisions_made[i]) {
                toon_escape_string(digest->decisions_made[i], escaped, sizeof(escaped));
                written = snprintf(buffer + offset, buffer_size - offset,
                                  "  %s\n", escaped);
                if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
                offset += (size_t)written;
            }
        }
    }

    *toon_out = buffer;
    return KATRA_SUCCESS;

overflow:
    free(buffer);
    katra_report_error(E_RESOURCE_LIMIT, "katra_tier2_digest_to_toon",
                      "Buffer overflow during serialization");
    return E_RESOURCE_LIMIT;
}

int katra_tier2_digests_to_toon(const digest_record_t** digests, size_t count, char** toon_out) {
    KATRA_CHECK_NULL(digests);
    KATRA_CHECK_NULL(toon_out);

    if (count == 0) {
        *toon_out = strdup("");
        return KATRA_SUCCESS;
    }

    /* Allocate buffer - multiple digests need more space */
    size_t buffer_size = 32768;  /* 32KB for multiple digests */
    char* buffer = malloc(buffer_size);
    if (!buffer) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_tier2_digests_to_toon",
                          "Failed to allocate buffer");
        return E_SYSTEM_MEMORY;
    }

    size_t offset = 0;
    int written;
    char escaped[KATRA_BUFFER_LARGE];

    /* Compact header with schema declaration */
    written = snprintf(buffer + offset, buffer_size - offset,
                      "digests[%zu]{id,period,type,themes,summary_preview}:\n",
                      count);
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    /* Each digest as compact row */
    for (size_t i = 0; i < count; i++) {
        const digest_record_t* d = digests[i];
        if (!d) continue;

        /* Extract summary preview (first 60 chars) */
        char summary_preview[64] = {0};
        if (d->summary) {
            size_t len = strlen(d->summary);
            if (len > 60) {
                strncpy(summary_preview, d->summary, 60);
                summary_preview[60] = '\0';
                strncat(summary_preview, "...", sizeof(summary_preview) - strlen(summary_preview) - 1);
            } else {
                strncpy(summary_preview, d->summary, sizeof(summary_preview) - 1);
                summary_preview[sizeof(summary_preview) - 1] = '\0';
            }
            toon_escape_string(summary_preview, escaped, sizeof(escaped));
        } else {
            snprintf(escaped, sizeof(escaped), "no summary");
        }

        written = snprintf(buffer + offset, buffer_size - offset,
                          "  %s,%s,%s,%zu,%s\n",
                          d->digest_id ? d->digest_id : "unknown",
                          d->period_id ? d->period_id : "unknown",
                          digest_type_to_string(d->digest_type),
                          d->theme_count,
                          escaped);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;
    }

    *toon_out = buffer;
    return KATRA_SUCCESS;

overflow:
    free(buffer);
    katra_report_error(E_RESOURCE_LIMIT, "katra_tier2_digests_to_toon",
                      "Buffer overflow during serialization");
    return E_RESOURCE_LIMIT;
}
