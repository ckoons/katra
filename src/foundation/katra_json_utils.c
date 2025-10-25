/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_json_utils.h"
#include "katra_error.h"
#include "katra_limits.h"

/* Escape string for JSON output */
void katra_json_escape(const char* src, char* dst, size_t dst_size) {
    if (!src || !dst || dst_size == 0) {
        if (dst && dst_size > 0) {
            dst[0] = '\0';
        }
        return;
    }

    size_t dst_idx = 0;
    for (const char* p = src; *p && dst_idx < dst_size - 1; p++) {
        if (*p == '"' || *p == '\\') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = *p;
            }
        } else if (*p == '\n') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 'n';
            }
        } else if (*p == '\r') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 'r';
            }
        } else if (*p == '\t') {
            if (dst_idx < dst_size - 2) {
                dst[dst_idx++] = '\\';
                dst[dst_idx++] = 't';
            }
        } else {
            dst[dst_idx++] = *p;
        }
    }
    dst[dst_idx] = '\0';
}

/* Extract string value from JSON */
int katra_json_get_string(const char* json, const char* key,
                          char* value, size_t value_size) {
    if (!json || !key || !value || value_size == 0) {
        return E_INPUT_NULL;
    }

    /* Build search pattern: "key": " */
    char pattern[KATRA_BUFFER_MEDIUM];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    /* Find the key */
    const char* key_pos = strstr(json, pattern);
    if (!key_pos) {
        return E_NOT_FOUND;
    }

    /* Skip to opening quote of value */
    const char* value_start = strchr(key_pos + strlen(pattern), '"');
    if (!value_start) {
        return E_NOT_FOUND;
    }
    value_start++;  /* Skip opening quote */

    /* Find closing quote */
    const char* value_end = strchr(value_start, '"');
    if (!value_end) {
        return E_NOT_FOUND;
    }

    /* Extract value */
    size_t len = value_end - value_start;
    if (len >= value_size) {
        len = value_size - 1;
    }

    strncpy(value, value_start, len);
    value[len] = '\0';

    return KATRA_SUCCESS;
}

/* Extract long integer value from JSON */
int katra_json_get_long(const char* json, const char* key, long* value) {
    if (!json || !key || !value) {
        return E_INPUT_NULL;
    }

    /* Build search pattern: "key": */
    char pattern[KATRA_BUFFER_MEDIUM];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    /* Find the key */
    const char* key_pos = strstr(json, pattern);
    if (!key_pos) {
        return E_NOT_FOUND;
    }

    /* Skip to value (skip whitespace after colon) */
    const char* value_start = key_pos + strlen(pattern);
    while (*value_start == ' ' || *value_start == '\t') {
        value_start++;
    }

    /* Parse the number */
    char* endptr;
    *value = strtol(value_start, &endptr, 10);

    if (endptr == value_start) {
        return E_NOT_FOUND;  /* No valid number found */
    }

    return KATRA_SUCCESS;
}

/* Extract size_t value from JSON */
int katra_json_get_size(const char* json, const char* key, size_t* value) {
    if (!json || !key || !value) {
        return E_INPUT_NULL;
    }

    /* Build search pattern: "key": */
    char pattern[KATRA_BUFFER_MEDIUM];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);

    /* Find the key */
    const char* key_pos = strstr(json, pattern);
    if (!key_pos) {
        return E_NOT_FOUND;
    }

    /* Skip to value (skip whitespace after colon) */
    const char* value_start = key_pos + strlen(pattern);
    while (*value_start == ' ' || *value_start == '\t') {
        value_start++;
    }

    /* Parse the number */
    char* endptr;
    unsigned long parsed = strtoul(value_start, &endptr, 10);

    if (endptr == value_start) {
        return E_NOT_FOUND;  /* No valid number found */
    }

    *value = (size_t)parsed;
    return KATRA_SUCCESS;
}
