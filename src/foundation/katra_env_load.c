/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <pwd.h>

/* Project includes */
#include "katra_env_utils.h"
#include "katra_env_internal.h"
#include "katra_error.h"
#include "katra_log.h"

/* Global environment state (definitions) */
char** katra_env = NULL;
int katra_env_count = 0;
int katra_env_capacity = 0;
pthread_mutex_t katra_env_mutex = PTHREAD_MUTEX_INITIALIZER;
bool katra_env_initialized = false;

/* External system environment */
extern char** environ;

/* Static helper functions */
static int load_system_environ(void);
static int load_env_file(const char* filepath, bool required);
static int parse_env_line(const char* line, char** key, char** value);
static char* expand_value(const char* value, int depth);
static int expand_all_variables(void);
static char* get_home_dir(void);
static const char* find_env_katra_file(void);
static void trim_whitespace(char* str);
static void strip_quotes(char* str);
static void load_optional_home_env(void);
static int load_project_and_local_files(char** project_env, char** local_env, char** root_dir);

/* Grow environment array by quantum */
int grow_env_array(void) {
    int new_capacity = katra_env_capacity + KATRA_ENV_GROWTH_QUANTUM;
    char** new_env = realloc(katra_env, (new_capacity + 1) * sizeof(char*));
    if (!new_env) return E_SYSTEM_MEMORY;

    for (int i = katra_env_capacity; i <= new_capacity; i++) {
        new_env[i] = NULL;
    }

    katra_env = new_env;
    katra_env_capacity = new_capacity;
    return KATRA_SUCCESS;
}

/* Find index of variable by name */
int find_env_index(const char* name) {
    size_t name_len = strlen(name);
    for (int i = 0; i < katra_env_count; i++) {
        if (katra_env[i] && strncmp(katra_env[i], name, name_len) == 0 &&
            katra_env[i][name_len] == '=') {
            return i;
        }
    }
    return -1;
}

/* Set environment variable (internal, assumes mutex held) */
int set_env_internal(const char* name, const char* value) {
    size_t len = strlen(name) + strlen(value) + 2;
    char* entry = malloc(len);
    if (!entry) return E_SYSTEM_MEMORY;

    snprintf(entry, len, "%s=%s", name, value);

    int idx = find_env_index(name);
    if (idx >= 0) {
        free(katra_env[idx]);
        katra_env[idx] = entry;
    } else {
        if (katra_env_count >= katra_env_capacity) {
            int result = grow_env_array();
            if (result != KATRA_SUCCESS) {
                free(entry);
                return result;
            }
        }
        katra_env[katra_env_count++] = entry;
        katra_env[katra_env_count] = NULL;
    }
    return KATRA_SUCCESS;
}

/* Load system environment */
static int load_system_environ(void) {
    for (char** env = environ; *env != NULL; env++) {
        char* eq = strchr(*env, '=');
        if (!eq) continue;

        size_t name_len = eq - *env;
        /* GUIDELINE_APPROVED - Aggregate NULL check pattern */
        char* name = strndup(*env, name_len);
        char* value = strdup(eq + 1);

        if (!name || !value) {
        /* GUIDELINE_APPROVED_END */
            free(name);
            free(value);
            return E_SYSTEM_MEMORY;
        }

        int result = set_env_internal(name, value);
        free(name);
        free(value);
        if (result != KATRA_SUCCESS) return result;
    }

    LOG_DEBUG("Loaded %d variables from system environment", katra_env_count);
    return KATRA_SUCCESS;
}

/* Trim leading and trailing whitespace */
static void trim_whitespace(char* str) {
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
static void strip_quotes(char* str) {
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
static int parse_env_line(const char* line, char** key, char** value) {
    *key = NULL;
    *value = NULL;

    char* buffer = strdup(line);
    if (!buffer) return E_SYSTEM_MEMORY;

    trim_whitespace(buffer);

    if (buffer[0] == '\0' || buffer[0] == '#') { free(buffer); return KATRA_SUCCESS; }

    char* parse_start = buffer;
    if (strncmp(buffer, KATRA_ENV_EXPORT_PREFIX, strlen(KATRA_ENV_EXPORT_PREFIX)) == 0) {
        parse_start = buffer + strlen(KATRA_ENV_EXPORT_PREFIX);
        trim_whitespace(parse_start);
    }

    char* eq = strchr(parse_start, '=');
    if (!eq) { free(buffer); return KATRA_SUCCESS; }

    *eq = '\0';
    trim_whitespace(parse_start);

    if (parse_start[0] == '\0') { free(buffer); return KATRA_SUCCESS; }

    /* GUIDELINE_APPROVED - Aggregate NULL check pattern */
    *key = strdup(parse_start);

    char* val = eq + 1;
    trim_whitespace(val);
    strip_quotes(val);
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

/* Load environment file */
static int load_env_file(const char* filepath, bool required) {
    int result = KATRA_SUCCESS;
    FILE* fp = NULL;
    int vars_loaded = 0;

    fp = fopen(filepath, "r");
    if (!fp) {
        if (required) {
            katra_report_error(E_SYSTEM_FILE, "load_env_file", "Failed to open file");
            result = E_SYSTEM_FILE;
        } else {
            LOG_INFO("Optional environment file not found: %s", filepath);
            result = KATRA_SUCCESS;
        }
        goto cleanup;
    }

    LOG_INFO("Loading environment from: %s", filepath);

    char line[KATRA_ENV_LINE_MAX];
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) { /* GUIDELINE_APPROVED: fgets in while condition */
        line_num++;

        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';

        char* key = NULL;
        char* value = NULL;
        int parse_result = parse_env_line(line, &key, &value);

        if (parse_result != KATRA_SUCCESS) {
            LOG_WARN("Malformed line in %s:%d: %s", filepath, line_num, line);
            continue;
        }

        if (key && value) {
            parse_result = set_env_internal(key, value);
            if (parse_result == KATRA_SUCCESS) vars_loaded++;
            free(key);
            free(value);
        }
    }

    /* Check for read errors */
    if (ferror(fp)) {
        katra_report_error(E_SYSTEM_IO, "load_env_file",
                         "Error reading %s after line %d", filepath, line_num);
        result = E_SYSTEM_IO;
        goto cleanup;
    }

    LOG_INFO("Loaded %d variables from %s", vars_loaded, filepath);

cleanup:
    if (fp) fclose(fp);
    return result;
}

/* Get home directory */
static char* get_home_dir(void) {
    const char* home = getenv("HOME");
    if (home) {
        char* result = strdup(home);
        if (!result) {
            LOG_ERROR("Failed to allocate memory for home directory");
        }
        return result;
    }

    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        char* result = strdup(pw->pw_dir);
        if (!result) {
            LOG_ERROR("Failed to allocate memory for home directory");
        }
        return result;
    }

    return NULL;
}

/* Find .env.katra file by searching upward */
static const char* find_env_katra_file(void) {
    static char found_path[PATH_MAX];
    char cwd[PATH_MAX];

    if (!getcwd(cwd, sizeof(cwd))) return NULL;

    char search_dir[PATH_MAX];
    strncpy(search_dir, cwd, sizeof(search_dir) - 1);

    while (1) {
        snprintf(found_path, sizeof(found_path), "%s/%s",
                search_dir, KATRA_ENV_PROJECT_FILE);

        if (access(found_path, F_OK) == 0) {
            LOG_DEBUG("Found %s in %s", KATRA_ENV_PROJECT_FILE, search_dir);
            return found_path;
        }

        char* last_slash = strrchr(search_dir, '/');
        if (!last_slash || last_slash == search_dir) break;

        *last_slash = '\0';
    }

    return NULL;
}

/* Expand ${VAR} references in value */
static char* expand_value(const char* value, int depth) {
    if (depth >= KATRA_ENV_MAX_EXPANSION_DEPTH) {
        LOG_WARN("Variable expansion depth limit reached");
        char* result = strdup(value);
        if (!result) {
            LOG_ERROR("Failed to allocate memory for value expansion");
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
                            char* expanded = expand_value(var_value, depth + 1);
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
        LOG_ERROR("Failed to allocate memory for expanded value");
    }
    return final_result;
}

/* Expand all variables in environment */
static int expand_all_variables(void) {
    int result = KATRA_SUCCESS;

    for (int i = 0; i < katra_env_count; i++) {
        char* eq = strchr(katra_env[i], '=');
        if (!eq) continue;

        const char* value = eq + 1;
        if (!strstr(value, "${")) continue;

        size_t name_len = eq - katra_env[i];
        char* name = strndup(katra_env[i], name_len);
        if (!name) { result = E_SYSTEM_MEMORY; goto cleanup; }

        char* expanded = expand_value(value, 0);
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

/* Load optional home environment files */
static void load_optional_home_env(void) {
    char* home = get_home_dir();
    if (home) {
        char home_env[PATH_MAX];

        /* Load ~/.env */
        snprintf(home_env, sizeof(home_env), "%s/%s", home, KATRA_ENV_HOME_FILE);
        load_env_file(home_env, false);

        /* Load ~/.katrarc */
        snprintf(home_env, sizeof(home_env), "%s/%s", home, KATRA_ENV_KATRARC_FILE);
        load_env_file(home_env, false);

        free(home);
    }
}

/* Load project and local environment files */
static int load_project_and_local_files(char** project_env, char** local_env, char** root_dir) {
    int result = KATRA_SUCCESS;

    const char* katra_root = NULL;
    int idx = find_env_index(KATRA_ROOT_VAR);
    if (idx >= 0) {
        char* eq = strchr(katra_env[idx], '=');
        if (eq) katra_root = eq + 1;
    }

    *project_env = malloc(PATH_MAX);
    *local_env = malloc(PATH_MAX);
    if (!*project_env || !*local_env) { result = E_SYSTEM_MEMORY; goto cleanup; }

    if (katra_root && katra_root[0]) {
        snprintf(*project_env, PATH_MAX, "%s/%s", katra_root, KATRA_ENV_PROJECT_FILE);
        snprintf(*local_env, PATH_MAX, "%s/%s", katra_root, KATRA_ENV_LOCAL_FILE);
    } else {
        const char* found = find_env_katra_file();
        if (!found) {
            katra_report_error(E_SYSTEM_FILE, "katra_loadenv", "Failed to find .env.katra");
            result = E_SYSTEM_FILE;
            goto cleanup;
        }

        strncpy(*project_env, found, PATH_MAX - 1);
        (*project_env)[PATH_MAX - 1] = '\0';

        char* last_slash = strrchr(*project_env, '/');
        if (last_slash) {
            size_t dir_len = last_slash - *project_env;
            *root_dir = malloc(PATH_MAX);
            if (!*root_dir) { result = E_SYSTEM_MEMORY; goto cleanup; }

            strncpy(*root_dir, *project_env, dir_len);
            (*root_dir)[dir_len] = '\0';

            set_env_internal(KATRA_ROOT_VAR, *root_dir);

            strncpy(*local_env, *project_env, dir_len);
            (*local_env)[dir_len] = '\0';
            snprintf(*local_env + dir_len, PATH_MAX - dir_len, "/%s", KATRA_ENV_LOCAL_FILE);
        }
    }

    result = load_env_file(*project_env, true);
    if (result != KATRA_SUCCESS) goto cleanup;

    load_env_file(*local_env, false);

cleanup:
    return result;
}

/* Load Katra environment */
int katra_loadenv(void) {
    int result = KATRA_SUCCESS;
    char* project_env = NULL;
    char* local_env = NULL;
    char* root_dir = NULL;

    int lock_result = pthread_mutex_lock(&katra_env_mutex);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PROCESS, "katra_loadenv", "Failed to acquire mutex");
        return E_SYSTEM_PROCESS;
    }

    if (katra_env_initialized) {
        for (int i = 0; i < katra_env_count; i++) {
            free(katra_env[i]);
        }
        katra_env_count = 0;
    }

    if (!katra_env) {
        katra_env_capacity = KATRA_ENV_INITIAL_CAPACITY;
        katra_env = calloc(katra_env_capacity + 1, sizeof(char*));
        if (!katra_env) { result = E_SYSTEM_MEMORY; goto cleanup; }
    }

    LOG_INFO("Loading Katra environment");

    result = load_system_environ();
    if (result != KATRA_SUCCESS) goto cleanup;

    load_optional_home_env();

    result = load_project_and_local_files(&project_env, &local_env, &root_dir);
    if (result != KATRA_SUCCESS) goto cleanup;

    result = expand_all_variables();
    if (result != KATRA_SUCCESS) goto cleanup;

    katra_env_initialized = true;
    LOG_INFO("Katra environment loaded: %d variables", katra_env_count);

cleanup:
    free(project_env);
    free(local_env);
    free(root_dir);
    pthread_mutex_unlock(&katra_env_mutex);
    return result;
}

/* Clear environment */
int katra_clearenv(void) {
    int lock_result = pthread_mutex_lock(&katra_env_mutex);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PROCESS, "katra_clearenv", "Failed to acquire mutex");
        return E_SYSTEM_PROCESS;
    }

    for (int i = 0; i < katra_env_count; i++) {
        free(katra_env[i]);
        katra_env[i] = NULL;
    }

    katra_env_count = 0;

    pthread_mutex_unlock(&katra_env_mutex);
    return KATRA_SUCCESS;
}

/* Free environment */
void katra_freeenv(void) {
    int lock_result = pthread_mutex_lock(&katra_env_mutex);
    if (lock_result != 0) {
        /* Can't return error from void function, log and proceed */
        LOG_ERROR("Failed to acquire mutex in katra_freeenv: %d", lock_result);
        return;
    }

    for (int i = 0; i < katra_env_count; i++) {
        free(katra_env[i]);
    }

    free(katra_env);
    katra_env = NULL;
    katra_env_count = 0;
    katra_env_capacity = 0;
    katra_env_initialized = false;

    pthread_mutex_unlock(&katra_env_mutex);
}
