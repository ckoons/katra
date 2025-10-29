/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_tier1.h"
#include "katra_core_common.h"
#include "katra_error.h"
#include "katra_json_utils.h"
#include "katra_limits.h"

/* Write memory record as JSON line (exported to tier1.c) */
int katra_tier1_write_json_record(FILE* fp, const memory_record_t* record);

/* JSON field extraction macros to reduce repetition */
#define EXTRACT_OPTIONAL_STRING(line, field_name, rec_field) do { \
    int _result = katra_json_extract_string_alloc(line, field_name, &rec->rec_field, \
                                                   katra_tier1_json_unescape); \
    if (_result != KATRA_SUCCESS) { \
        result = _result; \
        goto cleanup; \
    } \
} while(0)

#define EXTRACT_LONG_WITH_DEFAULT(line, field_name, rec_field, default_val) do { \
    long _temp = 0; \
    if (katra_json_get_long(line, field_name, &_temp) == KATRA_SUCCESS) { \
        rec->rec_field = (time_t)_temp; \
    } else { \
        rec->rec_field = default_val; \
    } \
} while(0)

#define EXTRACT_INT_WITH_DEFAULT(line, field_name, rec_field, cast_type, default_val) do { \
    int _temp = 0; \
    if (katra_json_get_int(line, field_name, &_temp) == KATRA_SUCCESS) { \
        rec->rec_field = (cast_type)_temp; \
    } else { \
        rec->rec_field = default_val; \
    } \
} while(0)

#define EXTRACT_FLOAT_WITH_DEFAULT(line, field_name, rec_field, default_val) do { \
    if (katra_json_get_float(line, field_name, &rec->rec_field) != KATRA_SUCCESS) { \
        rec->rec_field = default_val; \
    } \
} while(0)

#define EXTRACT_BOOL_WITH_DEFAULT(line, field_name, rec_field, default_val) do { \
    if (katra_json_get_bool(line, field_name, &rec->rec_field) != KATRA_SUCCESS) { \
        rec->rec_field = default_val; \
    } \
} while(0)

/* JSON unescape string helper */
void katra_tier1_json_unescape(const char* src, char* dst, size_t dst_size) {
    if (!src || !dst || dst_size == 0) {
        if (dst && dst_size > 0) {
            dst[0] = '\0';
        }
        return;
    }

    size_t dst_idx = 0;
    for (const char* p = src; *p && dst_idx < dst_size - 1; p++) {
        if (*p == '\\' && *(p + 1)) {
            p++;  /* Skip backslash */
            switch (*p) {
                case 'n':  dst[dst_idx++] = '\n'; break;
                case 'r':  dst[dst_idx++] = '\r'; break;
                case 't':  dst[dst_idx++] = '\t'; break;
                case '"':  dst[dst_idx++] = '"';  break;
                case '\\': dst[dst_idx++] = '\\'; break;
                default:   dst[dst_idx++] = *p;   break;
            }
        } else {
            dst[dst_idx++] = *p;
        }
    }
    dst[dst_idx] = '\0';
}

/* Note: extract_optional_string and extract_required_string replaced by
 * katra_json_extract_string_alloc() and katra_json_extract_string_required()
 * from katra_json_utils.h */

/* Parse JSON record from line */
int katra_tier1_parse_json_record(const char* line, memory_record_t** record) {
    int result = KATRA_SUCCESS;
    memory_record_t* rec = NULL;
    char temp_buffer[KATRA_BUFFER_LARGE];

    if (!line || !record) {
        result = E_INPUT_NULL;
        goto cleanup;
    }

    ALLOC_OR_GOTO(rec, memory_record_t, cleanup);

    /* Extract record_id */
    if (katra_json_get_string(line, "record_id", temp_buffer, sizeof(temp_buffer)) == KATRA_SUCCESS) {
        rec->record_id = strdup(temp_buffer);
        if (!rec->record_id) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    /* Extract timestamp */
    long timestamp_long = 0;
    if (katra_json_get_long(line, "timestamp", &timestamp_long) == KATRA_SUCCESS) {
        rec->timestamp = (time_t)timestamp_long;
    }

    /* Extract type */
    int type_int = 0;
    if (katra_json_get_int(line, "type", &type_int) == KATRA_SUCCESS) {
        rec->type = (memory_type_t)type_int;
    }

    /* Extract core fields */
    EXTRACT_FLOAT_WITH_DEFAULT(line, "importance", importance, MEMORY_IMPORTANCE_MEDIUM);
    EXTRACT_OPTIONAL_STRING(line, "importance_note", importance_note);

    /* Extract content (required) */
    result = katra_json_extract_string_required(line, "content", &rec->content,
                                                katra_tier1_json_unescape);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Extract optional string fields */
    EXTRACT_OPTIONAL_STRING(line, "response", response);
    EXTRACT_OPTIONAL_STRING(line, "context", context);
    EXTRACT_OPTIONAL_STRING(line, "ci_id", ci_id);
    EXTRACT_OPTIONAL_STRING(line, "session_id", session_id);
    EXTRACT_OPTIONAL_STRING(line, "component", component);

    /* Extract tier and archived status */
    EXTRACT_INT_WITH_DEFAULT(line, "tier", tier, katra_tier_t, KATRA_TIER1);
    EXTRACT_BOOL_WITH_DEFAULT(line, "archived", archived, false);

    /* Extract Thane's Phase 1 fields - access tracking */
    EXTRACT_LONG_WITH_DEFAULT(line, "last_accessed", last_accessed, 0);
    EXTRACT_INT_WITH_DEFAULT(line, "access_count", access_count, size_t, 0);
    EXTRACT_FLOAT_WITH_DEFAULT(line, "emotion_intensity", emotion_intensity, 0.0);
    EXTRACT_OPTIONAL_STRING(line, "emotion_type", emotion_type);
    EXTRACT_BOOL_WITH_DEFAULT(line, "marked_important", marked_important, false);
    EXTRACT_BOOL_WITH_DEFAULT(line, "marked_forgettable", marked_forgettable, false);

    /* Extract Phase 2 fields - connection graph */
    EXTRACT_INT_WITH_DEFAULT(line, "connection_count", connection_count, size_t, 0);
    rec->connected_memory_ids = NULL;  /* Array parsing deferred to graph builder */
    EXTRACT_FLOAT_WITH_DEFAULT(line, "graph_centrality", graph_centrality, 0.0);

    /* Extract Phase 3 fields - pattern compression */
    EXTRACT_OPTIONAL_STRING(line, "pattern_id", pattern_id);
    EXTRACT_INT_WITH_DEFAULT(line, "pattern_frequency", pattern_frequency, size_t, 0);
    EXTRACT_BOOL_WITH_DEFAULT(line, "is_pattern_outlier", is_pattern_outlier, false);
    EXTRACT_FLOAT_WITH_DEFAULT(line, "semantic_similarity", semantic_similarity, 0.0);

    *record = rec;
    return KATRA_SUCCESS;

cleanup:
    if (rec) {
        katra_memory_free_record(rec);
    }
    return result;
}

/* Write memory record as JSON line */
int katra_tier1_write_json_record(FILE* fp, const memory_record_t* record) {
    char content_escaped[KATRA_BUFFER_LARGE];
    char response_escaped[KATRA_BUFFER_LARGE];
    char context_escaped[KATRA_BUFFER_LARGE];
    char importance_note_escaped[KATRA_BUFFER_LARGE];

    katra_json_escape(record->content, content_escaped, sizeof(content_escaped));

    if (record->response) {
        katra_json_escape(record->response, response_escaped, sizeof(response_escaped));
    } else {
        response_escaped[0] = '\0';
    }

    if (record->context) {
        katra_json_escape(record->context, context_escaped, sizeof(context_escaped));
    } else {
        context_escaped[0] = '\0';
    }

    if (record->importance_note) {
        katra_json_escape(record->importance_note, importance_note_escaped, sizeof(importance_note_escaped));
    } else {
        importance_note_escaped[0] = '\0';
    }

    /* Write JSON object (one line) */
    fprintf(fp, "{");
    fprintf(fp, "\"record_id\":\"%s\",", record->record_id ? record->record_id : "");
    fprintf(fp, "\"timestamp\":%ld,", (long)record->timestamp);
    fprintf(fp, "\"type\":%d,", record->type);
    fprintf(fp, "\"importance\":%.2f,", record->importance);

    if (record->importance_note) {
        fprintf(fp, "\"importance_note\":\"%s\",", importance_note_escaped);
    }

    fprintf(fp, "\"content\":\"%s\",", content_escaped);

    if (record->response) {
        fprintf(fp, "\"response\":\"%s\",", response_escaped);
    }

    if (record->context) {
        fprintf(fp, "\"context\":\"%s\",", context_escaped);
    }

    fprintf(fp, "\"ci_id\":\"%s\",", record->ci_id ? record->ci_id : "");

    if (record->session_id) {
        fprintf(fp, "\"session_id\":\"%s\",", record->session_id);
    }

    if (record->component) {
        fprintf(fp, "\"component\":\"%s\",", record->component);
    }

    fprintf(fp, "\"tier\":%d,", record->tier);
    fprintf(fp, "\"archived\":%s,", record->archived ? "true" : "false");

    /* Thane's Phase 1 fields - access tracking */
    fprintf(fp, "\"last_accessed\":%ld,", (long)record->last_accessed);
    fprintf(fp, "\"access_count\":%zu,", record->access_count);

    /* Emotional salience */
    fprintf(fp, "\"emotion_intensity\":%.2f", record->emotion_intensity);
    if (record->emotion_type) {
        char emotion_escaped[KATRA_BUFFER_MEDIUM];
        katra_json_escape(record->emotion_type, emotion_escaped, sizeof(emotion_escaped));
        fprintf(fp, ",\"emotion_type\":\"%s\"", emotion_escaped);
    }

    /* Voluntary preservation */
    fprintf(fp, ",\"marked_important\":%s", record->marked_important ? "true" : "false");
    fprintf(fp, ",\"marked_forgettable\":%s", record->marked_forgettable ? "true" : "false");

    /* Phase 2: Connection graph fields */
    fprintf(fp, ",\"connection_count\":%zu", record->connection_count);
    fprintf(fp, ",\"graph_centrality\":%.4f", record->graph_centrality);
    /* Note: connected_memory_ids array serialization would require JSON array */
    /* Deferred to graph builder module */

    /* Phase 3: Pattern compression fields */
    if (record->pattern_id) {
        char pattern_escaped[KATRA_BUFFER_MEDIUM];
        katra_json_escape(record->pattern_id, pattern_escaped, sizeof(pattern_escaped));
        fprintf(fp, ",\"pattern_id\":\"%s\"", pattern_escaped);
    }
    fprintf(fp, ",\"pattern_frequency\":%zu", record->pattern_frequency);
    fprintf(fp, ",\"is_pattern_outlier\":%s", record->is_pattern_outlier ? "true" : "false");
    fprintf(fp, ",\"semantic_similarity\":%.4f", record->semantic_similarity);

    fprintf(fp, "}\n");

    return KATRA_SUCCESS;
}
