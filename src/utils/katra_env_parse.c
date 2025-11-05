/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Project includes */
#include "katra_env_utils.h"
#include "katra_env_internal.h"
#include "katra_error.h"
#include "katra_log.h"

/* External references to env state */
extern char** katra_env;
extern int katra_env_count;
extern int find_env_index(const char* name);

/* Trim leading and trailing whitespace */
void katra_env_trim_whitespace(char* str) {
    if (!str) return;

    char* start = str;
    while (*start && isspace(*start)) start++;

    char* end = start + strlen(start) - 1;
    while (end > start && isspace(*end)) end--;
    *(end + 1) = '\0';

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

/* GUIDELINE_APPROVED - Quote character detection in string parsing */
/* Strip surrounding quotes */
void katra_env_strip_quotes(char* str) {
    if (!str) return;

    size_t len = strlen(str);
    if (len >= 2 &&
        ((str[0] == '"' && str[len-1] == '"') ||
         (str[0] == '\'' && str[len-1] == '\''))) {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    }
}
/* GUIDELINE_APPROVED_END */

/* Parse environment file line */
int katra_env_parse_line(const char* line, char** key, char** value) {
    *key = NULL;
    *value = NULL;

    char* buffer = strdup(line);
    if (!buffer) return E_SYSTEM_MEMORY;

    katra_env_trim_whitespace(buffer);

    if (buffer[0] == '\0' || buffer[0] == '#') { free(buffer); return KATRA_SUCCESS; }

    char* parse_start = buffer;
    if (strncmp(buffer, KATRA_ENV_EXPORT_PREFIX, strlen(KATRA_ENV_EXPORT_PREFIX)) == 0) {
        parse_start = buffer + strlen(KATRA_ENV_EXPORT_PREFIX);
        katra_env_trim_whitespace(parse_start);
    }

    char* eq = strchr(parse_start, '=');
    if (!eq) { free(buffer); return KATRA_SUCCESS; }

    *eq = '\0';
    katra_env_trim_whitespace(parse_start);

    if (parse_start[0] == '\0') { free(buffer); return KATRA_SUCCESS; }

    /* GUIDELINE_APPROVED - Aggregate NULL check pattern */
    *key = strdup(parse_start);

    char* val = eq + 1;
    katra_env_trim_whitespace(val);
    katra_env_strip_quotes(val);
    *value = strdup(val);

    if (!*key || !*value) {
    /* GUIDELINE_APPROVED_END */
        free(*key);
        free(*value);
        *key = *value = NULL;
        return E_SYSTEM_MEMORY;
    }

    free(buffer);
    return KATRA_SUCCESS;
}

/* Expand ${VAR} references in value */
char* katra_env_expand_value(const char* value, int depth) {
    if (depth >= KATRA_ENV_MAX_EXPANSION_DEPTH) {
        LOG_WARN("Variable expansion depth limit reached");
        char* result = strdup(value);
        if (!result) {
            LOG_ERROR(KATRA_ERR_ALLOC_FAILED);
        }
        return result;
    }

    char var_name[KATRA_ENV_VAR_NAME_MAX];
    char result[KATRA_ENV_LINE_MAX];
    result[0] = '\0';

    const char* src = value;
    size_t result_len = 0;

    while (*src && result_len < sizeof(result) - 1) {
        if (src[0] == '$' && src[1] == '{') {
            const char* var_start = src + 2;
            const char* var_end = strchr(var_start, '}');

            if (var_end) {
                size_t var_len = var_end - var_start;
                if (var_len < sizeof(var_name)) {
                    strncpy(var_name, var_start, var_len);
                    var_name[var_len] = '\0';

                    int idx = find_env_index(var_name);
                    if (idx >= 0) {
                        char* eq = strchr(katra_env[idx], '=');
                        if (eq) {
                            const char* var_value = eq + 1;
                            char* expanded = katra_env_expand_value(var_value, depth + 1);
                            if (!expanded) {
                                LOG_ERROR("Failed to expand variable: %s", var_name);
                                continue;
                            }

                            size_t exp_len = strlen(expanded);
                            if (result_len + exp_len < sizeof(result) - 1) {
                                snprintf(result + result_len, sizeof(result) - result_len, "%s", expanded);
                                result_len += exp_len;
                            }
                            free(expanded);
                        }
                    }
                    src = var_end + 1;
                    continue;
                }
            }
        }

        result[result_len++] = *src++;
    }

    result[result_len] = '\0';
    char* final_result = strdup(result);
    if (!final_result) {
        LOG_ERROR(KATRA_ERR_ALLOC_FAILED);
    }
    return final_result;
}

/* Expand all variables in environment */
int katra_env_expand_all(void) {
    int result = KATRA_SUCCESS;

    for (int i = 0; i < katra_env_count; i++) {
        char* eq = strchr(katra_env[i], '=');
        if (!eq) continue;

        const char* value = eq + 1;
        if (!strstr(value, "${")) continue;

        size_t name_len = eq - katra_env[i];
        char* name = strndup(katra_env[i], name_len);
        if (!name) { result = E_SYSTEM_MEMORY; goto cleanup; }

        char* expanded = katra_env_expand_value(value, 0);
        if (!expanded) {
            free(name);
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }

        size_t total_len = name_len + strlen(expanded) + 2;
        char* new_entry = malloc(total_len);
        if (!new_entry) {
            free(name);
            free(expanded);
            result = E_SYSTEM_MEMORY;
            goto cleanup;
        }

        snprintf(new_entry, total_len, "%s=%s", name, expanded);

        free(katra_env[i]);
        katra_env[i] = new_entry;

        free(name);
        free(expanded);
    }

cleanup:
    return result;
}
