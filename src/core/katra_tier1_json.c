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

    /* Extract importance */
    if (katra_json_get_float(line, "importance", &rec->importance) != KATRA_SUCCESS) {
        rec->importance = MEMORY_IMPORTANCE_MEDIUM;
    }

    /* Extract importance_note (optional) */
    result = katra_json_extract_string_alloc(line, "importance_note", &rec->importance_note,
                                             katra_tier1_json_unescape);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Extract content (required) */
    result = katra_json_extract_string_required(line, "content", &rec->content,
                                                katra_tier1_json_unescape);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Extract optional string fields */
    result = katra_json_extract_string_alloc(line, "response", &rec->response,
                                             katra_tier1_json_unescape);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    result = katra_json_extract_string_alloc(line, "context", &rec->context,
                                             katra_tier1_json_unescape);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    result = katra_json_extract_string_alloc(line, "ci_id", &rec->ci_id,
                                             katra_tier1_json_unescape);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    result = katra_json_extract_string_alloc(line, "session_id", &rec->session_id,
                                             katra_tier1_json_unescape);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    result = katra_json_extract_string_alloc(line, "component", &rec->component,
                                             katra_tier1_json_unescape);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Extract tier */
    int tier_int = KATRA_TIER1;
    if (katra_json_get_int(line, "tier", &tier_int) == KATRA_SUCCESS) {
        rec->tier = (katra_tier_t)tier_int;
    }

    /* Extract archived */
    if (katra_json_get_bool(line, "archived", &rec->archived) != KATRA_SUCCESS) {
        rec->archived = false;
    }

    /* Extract Thane's Phase 1 fields - access tracking */
    long last_accessed_long = 0;
    if (katra_json_get_long(line, "last_accessed", &last_accessed_long) == KATRA_SUCCESS) {
        rec->last_accessed = (time_t)last_accessed_long;
    } else {
        rec->last_accessed = 0;
    }

    int access_count_int = 0;
    if (katra_json_get_int(line, "access_count", &access_count_int) == KATRA_SUCCESS) {
        rec->access_count = (size_t)access_count_int;
    } else {
        rec->access_count = 0;
    }

    /* Extract emotional salience fields */
    if (katra_json_get_float(line, "emotion_intensity", &rec->emotion_intensity) != KATRA_SUCCESS) {
        rec->emotion_intensity = 0.0;
    }

    result = katra_json_extract_string_alloc(line, "emotion_type", &rec->emotion_type,
                                             katra_tier1_json_unescape);
    if (result != KATRA_SUCCESS) {
        goto cleanup;
    }

    /* Extract voluntary preservation fields */
    if (katra_json_get_bool(line, "marked_important", &rec->marked_important) != KATRA_SUCCESS) {
        rec->marked_important = false;
    }

    if (katra_json_get_bool(line, "marked_forgettable", &rec->marked_forgettable) != KATRA_SUCCESS) {
        rec->marked_forgettable = false;
    }

    *record = rec;
    return KATRA_SUCCESS;

cleanup:
    if (rec) {
        katra_memory_free_record(rec);
    }
    return result;
}
