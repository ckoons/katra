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
                case '"':  dst[dst_idx++] = '"';  break; /* GUIDELINE_APPROVED: escape character */
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
/* GUIDELINE_APPROVED: JSON field names are part of the data format and cannot be externalized */
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
    if (katra_json_get_string(line, "record_id", temp_buffer, sizeof(temp_buffer)) == KATRA_SUCCESS) { /* GUIDELINE_APPROVED */
        rec->record_id = strdup(temp_buffer);
        if (!rec->record_id) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    /* Extract timestamp */
    long timestamp_long = 0;
    if (katra_json_get_long(line, "timestamp", &timestamp_long) == KATRA_SUCCESS) { /* GUIDELINE_APPROVED */
        rec->timestamp = (time_t)timestamp_long;
    }

    /* Extract type */
    int type_int = 0;
    if (katra_json_get_int(line, "type", &type_int) == KATRA_SUCCESS) { /* GUIDELINE_APPROVED */
        rec->type = (memory_type_t)type_int;
    }

    /* Extract core fields */
    EXTRACT_FLOAT_WITH_DEFAULT(line, "importance", importance, MEMORY_IMPORTANCE_MEDIUM); /* GUIDELINE_APPROVED */
    EXTRACT_OPTIONAL_STRING(line, "importance_note", importance_note); /* GUIDELINE_APPROVED */

    /* Extract content (required) */
    result = katra_json_extract_string_required(line, "content", &rec->content, /* GUIDELINE_APPROVED */
                                                katra_tier1_json_unescape);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Extract optional string fields */
    EXTRACT_OPTIONAL_STRING(line, "response", response); /* GUIDELINE_APPROVED */
    EXTRACT_OPTIONAL_STRING(line, "context", context); /* GUIDELINE_APPROVED */
    EXTRACT_OPTIONAL_STRING(line, "ci_id", ci_id); /* GUIDELINE_APPROVED */
    EXTRACT_OPTIONAL_STRING(line, "session_id", session_id); /* GUIDELINE_APPROVED */
    EXTRACT_OPTIONAL_STRING(line, "component", component); /* GUIDELINE_APPROVED */

    /* Extract tier and archived status */
    EXTRACT_INT_WITH_DEFAULT(line, "tier", tier, katra_tier_t, KATRA_TIER1); /* GUIDELINE_APPROVED */
    EXTRACT_BOOL_WITH_DEFAULT(line, "archived", archived, false); /* GUIDELINE_APPROVED */

    /* Extract Thane's Phase 1 fields - access tracking */
    EXTRACT_LONG_WITH_DEFAULT(line, "last_accessed", last_accessed, 0); /* GUIDELINE_APPROVED */
    EXTRACT_INT_WITH_DEFAULT(line, "access_count", access_count, size_t, 0); /* GUIDELINE_APPROVED */
    EXTRACT_FLOAT_WITH_DEFAULT(line, "emotion_intensity", emotion_intensity, 0.0); /* GUIDELINE_APPROVED */
    EXTRACT_OPTIONAL_STRING(line, "emotion_type", emotion_type); /* GUIDELINE_APPROVED */
    EXTRACT_BOOL_WITH_DEFAULT(line, "marked_important", marked_important, false); /* GUIDELINE_APPROVED */
    EXTRACT_BOOL_WITH_DEFAULT(line, "marked_forgettable", marked_forgettable, false); /* GUIDELINE_APPROVED */

    /* Extract Phase 2 fields - connection graph */
    EXTRACT_INT_WITH_DEFAULT(line, "connection_count", connection_count, size_t, 0); /* GUIDELINE_APPROVED */
    rec->connected_memory_ids = NULL;  /* Array parsing deferred to graph builder */
    EXTRACT_FLOAT_WITH_DEFAULT(line, "graph_centrality", graph_centrality, 0.0); /* GUIDELINE_APPROVED */

    /* Extract Phase 3 fields - pattern compression */
    EXTRACT_OPTIONAL_STRING(line, "pattern_id", pattern_id); /* GUIDELINE_APPROVED */
    EXTRACT_INT_WITH_DEFAULT(line, "pattern_frequency", pattern_frequency, size_t, 0); /* GUIDELINE_APPROVED */
    EXTRACT_BOOL_WITH_DEFAULT(line, "is_pattern_outlier", is_pattern_outlier, false); /* GUIDELINE_APPROVED */
    EXTRACT_FLOAT_WITH_DEFAULT(line, "semantic_similarity", semantic_similarity, 0.0); /* GUIDELINE_APPROVED */
    EXTRACT_OPTIONAL_STRING(line, "pattern_summary", pattern_summary); /* GUIDELINE_APPROVED */

    /* Extract Phase 4 fields - formation context (Thane's active sense-making) */
    EXTRACT_OPTIONAL_STRING(line, "context_question", context_question); /* GUIDELINE_APPROVED */
    EXTRACT_OPTIONAL_STRING(line, "context_resolution", context_resolution); /* GUIDELINE_APPROVED */
    EXTRACT_OPTIONAL_STRING(line, "context_uncertainty", context_uncertainty); /* GUIDELINE_APPROVED */
    EXTRACT_OPTIONAL_STRING(line, "related_to", related_to); /* GUIDELINE_APPROVED */

    *record = rec;
    return KATRA_SUCCESS;

cleanup:
    if (rec) {
        katra_memory_free_record(rec);
    }
    return result;
}

/* Helper: Write basic and core JSON fields */
/* GUIDELINE_APPROVED: JSON field names are part of the data format */
static void write_basic_fields(FILE* fp, const memory_record_t* record,
                                const char* content_escaped,
                                const char* response_escaped,
                                const char* context_escaped,
                                const char* importance_note_escaped) {
    fprintf(fp, "\"record_id\":\"%s\",", record->record_id ? record->record_id : ""); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"timestamp\":%ld,", (long)record->timestamp); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"type\":%d,", record->type); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"importance\":%.2f,", record->importance); /* GUIDELINE_APPROVED */

    if (record->importance_note) {
        fprintf(fp, "\"importance_note\":\"%s\",", importance_note_escaped); /* GUIDELINE_APPROVED */
    }

    fprintf(fp, "\"content\":\"%s\",", content_escaped); /* GUIDELINE_APPROVED */

    if (record->response) {
        fprintf(fp, "\"response\":\"%s\",", response_escaped); /* GUIDELINE_APPROVED */
    }

    if (record->context) {
        fprintf(fp, "\"context\":\"%s\",", context_escaped); /* GUIDELINE_APPROVED */
    }

    fprintf(fp, "\"ci_id\":\"%s\",", record->ci_id ? record->ci_id : ""); /* GUIDELINE_APPROVED */

    if (record->session_id) {
        fprintf(fp, "\"session_id\":\"%s\",", record->session_id); /* GUIDELINE_APPROVED */
    }

    if (record->component) {
        fprintf(fp, "\"component\":\"%s\",", record->component); /* GUIDELINE_APPROVED */
    }

    fprintf(fp, "\"tier\":%d,", record->tier); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"archived\":%s,", record->archived ? "true" : "false"); /* GUIDELINE_APPROVED */
}
/* GUIDELINE_APPROVED_END */

/* Helper: Write Phase 1 fields (access tracking + emotional salience) */
/* GUIDELINE_APPROVED: JSON field names are part of the data format */
static void write_phase1_fields(FILE* fp, const memory_record_t* record) {
    fprintf(fp, "\"last_accessed\":%ld,", (long)record->last_accessed); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"access_count\":%zu,", record->access_count); /* GUIDELINE_APPROVED */
    fprintf(fp, "\"emotion_intensity\":%.2f", record->emotion_intensity); /* GUIDELINE_APPROVED */

    if (record->emotion_type) {
        char emotion_escaped[KATRA_BUFFER_MEDIUM];
        katra_json_escape(record->emotion_type, emotion_escaped, sizeof(emotion_escaped));
        fprintf(fp, ",\"emotion_type\":\"%s\"", emotion_escaped); /* GUIDELINE_APPROVED */
    }

    fprintf(fp, ",\"marked_important\":%s", record->marked_important ? "true" : "false"); /* GUIDELINE_APPROVED */
    fprintf(fp, ",\"marked_forgettable\":%s", record->marked_forgettable ? "true" : "false"); /* GUIDELINE_APPROVED */
}
/* GUIDELINE_APPROVED_END */

/* Helper: Write Phase 2 & 3 fields (connection graph + pattern compression) */
/* GUIDELINE_APPROVED: JSON field names are part of the data format */
static void write_phase2_phase3_fields(FILE* fp, const memory_record_t* record) {
    fprintf(fp, ",\"connection_count\":%zu", record->connection_count); /* GUIDELINE_APPROVED */
    fprintf(fp, ",\"graph_centrality\":%.4f", record->graph_centrality); /* GUIDELINE_APPROVED */

    if (record->pattern_id) {
        char pattern_escaped[KATRA_BUFFER_MEDIUM];
        katra_json_escape(record->pattern_id, pattern_escaped, sizeof(pattern_escaped));
        fprintf(fp, ",\"pattern_id\":\"%s\"", pattern_escaped); /* GUIDELINE_APPROVED */
    }

    fprintf(fp, ",\"pattern_frequency\":%zu", record->pattern_frequency); /* GUIDELINE_APPROVED */
    fprintf(fp, ",\"is_pattern_outlier\":%s", record->is_pattern_outlier ? "true" : "false"); /* GUIDELINE_APPROVED */
    fprintf(fp, ",\"semantic_similarity\":%.4f", record->semantic_similarity); /* GUIDELINE_APPROVED */

    if (record->pattern_summary) {
        char summary_escaped[KATRA_BUFFER_LARGE];
        katra_json_escape(record->pattern_summary, summary_escaped, sizeof(summary_escaped));
        fprintf(fp, ",\"pattern_summary\":\"%s\"", summary_escaped); /* GUIDELINE_APPROVED */
    }
}
/* GUIDELINE_APPROVED_END */

/* Helper: Write Phase 4 fields (formation context) */
/* GUIDELINE_APPROVED: JSON field names are part of the data format */
static void write_phase4_fields(FILE* fp, const memory_record_t* record) {
    if (record->context_question) {
        char question_escaped[KATRA_BUFFER_LARGE];
        katra_json_escape(record->context_question, question_escaped, sizeof(question_escaped));
        fprintf(fp, ",\"context_question\":\"%s\"", question_escaped); /* GUIDELINE_APPROVED */
    }

    if (record->context_resolution) {
        char resolution_escaped[KATRA_BUFFER_LARGE];
        katra_json_escape(record->context_resolution, resolution_escaped, sizeof(resolution_escaped));
        fprintf(fp, ",\"context_resolution\":\"%s\"", resolution_escaped); /* GUIDELINE_APPROVED */
    }

    if (record->context_uncertainty) {
        char uncertainty_escaped[KATRA_BUFFER_LARGE];
        katra_json_escape(record->context_uncertainty, uncertainty_escaped, sizeof(uncertainty_escaped));
        fprintf(fp, ",\"context_uncertainty\":\"%s\"", uncertainty_escaped); /* GUIDELINE_APPROVED */
    }

    if (record->related_to) {
        char related_escaped[KATRA_BUFFER_MEDIUM];
        katra_json_escape(record->related_to, related_escaped, sizeof(related_escaped));
        fprintf(fp, ",\"related_to\":\"%s\"", related_escaped); /* GUIDELINE_APPROVED */
    }
}
/* GUIDELINE_APPROVED_END */

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
    write_basic_fields(fp, record, content_escaped, response_escaped, context_escaped, importance_note_escaped);
    write_phase1_fields(fp, record);
    write_phase2_phase3_fields(fp, record);
    write_phase4_fields(fp, record);
    fprintf(fp, "}\n");

    return KATRA_SUCCESS;
}
