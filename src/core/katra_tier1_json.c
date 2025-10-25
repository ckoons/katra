/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_tier1.h"
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

/* Parse JSON record from line */
int katra_tier1_parse_json_record(const char* line, memory_record_t** record) {
    int result = KATRA_SUCCESS;
    memory_record_t* rec = NULL;
    char temp_buffer[KATRA_BUFFER_LARGE];

    if (!line || !record) {
        result = E_INPUT_NULL;
        goto cleanup;
    }

    rec = calloc(1, sizeof(memory_record_t));
    if (!rec) {
        result = E_SYSTEM_MEMORY;
        goto cleanup;
    }

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

    /* Extract content (required) */
    if (katra_json_get_string(line, "content", temp_buffer, sizeof(temp_buffer)) == KATRA_SUCCESS) {
        char unescaped[KATRA_BUFFER_LARGE];
        katra_tier1_json_unescape(temp_buffer, unescaped, sizeof(unescaped));
        rec->content = strdup(unescaped);
        if (!rec->content) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    /* Extract response (optional) */
    if (katra_json_get_string(line, "response", temp_buffer, sizeof(temp_buffer)) == KATRA_SUCCESS) {
        char unescaped[KATRA_BUFFER_LARGE];
        katra_tier1_json_unescape(temp_buffer, unescaped, sizeof(unescaped));
        rec->response = strdup(unescaped);
        if (!rec->response) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    /* Extract context (optional) */
    if (katra_json_get_string(line, "context", temp_buffer, sizeof(temp_buffer)) == KATRA_SUCCESS) {
        char unescaped[KATRA_BUFFER_LARGE];
        katra_tier1_json_unescape(temp_buffer, unescaped, sizeof(unescaped));
        rec->context = strdup(unescaped);
        if (!rec->context) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    /* Extract ci_id */
    if (katra_json_get_string(line, "ci_id", temp_buffer, sizeof(temp_buffer)) == KATRA_SUCCESS) {
        rec->ci_id = strdup(temp_buffer);
        if (!rec->ci_id) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    /* Extract session_id (optional) */
    if (katra_json_get_string(line, "session_id", temp_buffer, sizeof(temp_buffer)) == KATRA_SUCCESS) {
        rec->session_id = strdup(temp_buffer);
        if (!rec->session_id) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
    }

    /* Extract component (optional) */
    if (katra_json_get_string(line, "component", temp_buffer, sizeof(temp_buffer)) == KATRA_SUCCESS) {
        rec->component = strdup(temp_buffer);
        if (!rec->component) {
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }
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

    *record = rec;
    return KATRA_SUCCESS;

cleanup:
    if (rec) {
        katra_memory_free_record(rec);
    }
    return result;
}
