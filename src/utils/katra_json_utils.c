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

    /* Find closing quote (handle escaped quotes) */
    const char* value_end = value_start;
    while (*value_end) {
        if (*value_end == '"') {
            /* Check if this quote is escaped */
            const char* check = value_end - 1;
            int backslash_count = 0;
            while (check >= value_start && *check == '\\') {
                backslash_count++;
                check--;
            }
            /* If even number of backslashes (including 0), quote is not escaped */
            if (backslash_count % 2 == 0) {
                break;  /* Found unescaped closing quote */
            }
        }
        value_end++;
    }

    if (*value_end != '"') {
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
    *value = strtol(value_start, &endptr, DECIMAL_BASE);

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
    unsigned long parsed = strtoul(value_start, &endptr, DECIMAL_BASE);

    if (endptr == value_start) {
        return E_NOT_FOUND;  /* No valid number found */
    }

    *value = (size_t)parsed;
    return KATRA_SUCCESS;
}

/* Extract integer value from JSON */
int katra_json_get_int(const char* json, const char* key, int* value) {
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
    long parsed = strtol(value_start, &endptr, DECIMAL_BASE);

    if (endptr == value_start) {
        return E_NOT_FOUND;  /* No valid number found */
    }

    *value = (int)parsed;
    return KATRA_SUCCESS;
}

/* Extract float value from JSON */
int katra_json_get_float(const char* json, const char* key, float* value) {
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
    float parsed = strtof(value_start, &endptr);

    if (endptr == value_start) {
        return E_NOT_FOUND;  /* No valid number found */
    }

    *value = parsed;
    return KATRA_SUCCESS;
}

/* Extract boolean value from JSON */
int katra_json_get_bool(const char* json, const char* key, bool* value) {
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

    /* Check for true/false */
    if (strncmp(value_start, "true", 4) == 0) {
        *value = true;
        return KATRA_SUCCESS;
    } else if (strncmp(value_start, "false", 5) == 0) {
        *value = false;
        return KATRA_SUCCESS;
    }

    return E_NOT_FOUND;
}

/* Extract optional JSON string field with allocation */
int katra_json_extract_string_alloc(const char* json, const char* field,
                                     char** dest, json_unescape_fn_t unescape_fn) {
    if (!json || !dest) {
        return E_INPUT_NULL;
    }

    *dest = NULL;  /* Default to NULL if not found */

    char temp_buffer[KATRA_BUFFER_LARGE];
    char unescaped[KATRA_BUFFER_LARGE];

    int result = katra_json_get_string(json, field, temp_buffer, sizeof(temp_buffer));
    if (result != KATRA_SUCCESS) {
        return KATRA_SUCCESS;  /* Not found is not an error for optional field */
    }

    /* Apply unescape if provided */
    if (unescape_fn) {
        unescape_fn(temp_buffer, unescaped, sizeof(unescaped));
        *dest = strdup(unescaped);
    } else {
        *dest = strdup(temp_buffer);
    }

    if (!*dest) {
        katra_report_error(E_SYSTEM_MEMORY, __func__, KATRA_ERR_ALLOC_FAILED);
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}

/* Extract required JSON string field with allocation */
int katra_json_extract_string_required(const char* json, const char* field,
                                        char** dest, json_unescape_fn_t unescape_fn) {
    if (!json || !dest) {
        return E_INPUT_NULL;
    }

    char temp_buffer[KATRA_BUFFER_LARGE];
    char unescaped[KATRA_BUFFER_LARGE];

    int result = katra_json_get_string(json, field, temp_buffer, sizeof(temp_buffer));
    if (result != KATRA_SUCCESS) {
        katra_report_error(E_NOT_FOUND, __func__, "Required field not found");
        return E_NOT_FOUND;
    }

    /* Apply unescape if provided */
    if (unescape_fn) {
        unescape_fn(temp_buffer, unescaped, sizeof(unescaped));
        *dest = strdup(unescaped);
    } else {
        *dest = strdup(temp_buffer);
    }

    if (!*dest) {
        katra_report_error(E_SYSTEM_MEMORY, __func__, KATRA_ERR_ALLOC_FAILED);
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}
