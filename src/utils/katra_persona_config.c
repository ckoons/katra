/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

/* Project includes */
#include "katra_persona_config.h"
#include "katra_config.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_env_utils.h"
#include "katra_strings.h"
#include "katra_core_common.h"

/* Forward declarations */
static int ensure_persona_config_dir(const char* persona_name);
static int read_persona_config_file(const char* persona_name);
static int write_config_value(const char* filepath, const char* key, const char* value);

/* Persona-specific config cache (simple linked list, one per persona) */
typedef struct persona_config_entry {
    char* persona_name;
    char* key;
    char* value;
    struct persona_config_entry* next;
} persona_config_entry_t;

static persona_config_entry_t* g_persona_configs = NULL;
static bool g_persona_config_initialized = false;

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int katra_persona_config_init(void) {
    if (g_persona_config_initialized) {
        return KATRA_SUCCESS;
    }

    /* Ensure base config directory exists */
    char config_dir[KATRA_PATH_MAX];
    int result = katra_build_and_ensure_dir(config_dir, sizeof(config_dir),
                                           KATRA_DIR_CONFIG, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    g_persona_config_initialized = true;
    LOG_DEBUG("Persona configuration system initialized");
    return KATRA_SUCCESS;
}

void katra_persona_config_cleanup(void) {
    if (!g_persona_config_initialized) {
        return;
    }

    /* Free all cached persona configs */
    persona_config_entry_t* current = g_persona_configs;
    while (current) {
        persona_config_entry_t* next = current->next;
        free(current->persona_name);
        free(current->key);
        free(current->value);
        free(current);
        current = next;
    }

    g_persona_configs = NULL;
    g_persona_config_initialized = false;
    LOG_DEBUG("Persona configuration system cleaned up");
}

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/* Ensure persona config directory exists */
static int ensure_persona_config_dir(const char* persona_name) {
    char persona_dir[KATRA_PATH_MAX];
    int result = katra_build_and_ensure_dir(persona_dir, sizeof(persona_dir),
                                           KATRA_DIR_CONFIG, persona_name, NULL);
    return result;
}

/* Read persona config file into cache */
static int read_persona_config_file(const char* persona_name) {
    char config_file[KATRA_PATH_MAX];
    char config_dir[KATRA_PATH_MAX];

    /* Build path to persona config file */
    int result = katra_build_path(config_dir, sizeof(config_dir),
                                  KATRA_DIR_CONFIG, persona_name, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    snprintf(config_file, sizeof(config_file), "%s/settings", config_dir);

    /* Check if file exists */
    struct stat st;
    if (stat(config_file, &st) != 0) {
        /* File doesn't exist - that's okay, no persona-specific config */
        return KATRA_SUCCESS;
    }

    /* Read file line by line */
    FILE* fp = fopen(config_file, "r");
    if (!fp) {
        return E_SYSTEM_FILE;
    }

    char line[KATRA_BUFFER_LARGE];
    while (fgets(line, sizeof(line), fp)) {
        /* Remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        /* Skip empty lines and comments */
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        /* Parse KEY=VALUE */
        char* eq = strchr(line, '=');
        if (!eq) {
            continue;
        }

        *eq = '\0';
        char* key = line;
        char* value = eq + 1;

        /* Trim whitespace */
        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;

        /* Store in cache */
        persona_config_entry_t* entry = malloc(sizeof(persona_config_entry_t));
        if (!entry) {
            fclose(fp);
            return E_SYSTEM_MEMORY;
        }

        entry->persona_name = strdup(persona_name);
        entry->key = strdup(key);
        entry->value = strdup(value);
        entry->next = g_persona_configs;
        g_persona_configs = entry;

        if (!entry->persona_name || !entry->key || !entry->value) {
            fclose(fp);
            return E_SYSTEM_MEMORY;
        }
    }

    fclose(fp);
    return KATRA_SUCCESS;
}

/* Write config value to file atomically */
static int write_config_value(const char* filepath, const char* key, const char* value) {
    char temp_file[KATRA_PATH_MAX];
    snprintf(temp_file, sizeof(temp_file), "%s.tmp", filepath);

    /* Read existing config */
    FILE* in = fopen(filepath, "r");
    FILE* out = fopen(temp_file, "w");
    if (!out) {
        if (in) fclose(in);
        return E_SYSTEM_FILE;
    }

    bool key_written = false;

    /* Copy existing config, updating key if found */
    if (in) {
        char line[KATRA_BUFFER_LARGE];
        while (fgets(line, sizeof(line), in)) {
            /* Check if this is the key we're updating */
            char* eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char* line_key = line;
                while (*line_key == ' ' || *line_key == '\t') line_key++;

                if (strcmp(line_key, key) == 0) {
                    /* Write updated value */
                    fprintf(out, "%s=%s\n", key, value);
                    key_written = true;
                    continue;
                }
                *eq = '=';  /* Restore for writing */
            }

            /* Write original line */
            fputs(line, out);
        }
        fclose(in);
    }

    /* If key wasn't found, append it */
    if (!key_written) {
        fprintf(out, "%s=%s\n", key, value);
    }

    fclose(out);

    /* Atomic rename */
    if (rename(temp_file, filepath) != 0) {
        unlink(temp_file);
        return E_SYSTEM_FILE;
    }

    return KATRA_SUCCESS;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

const char* katra_persona_config_get(const char* persona_name, const char* key) {
    if (!key) {
        return NULL;
    }

    /* Check persona-specific config first */
    if (persona_name) {
        /* Load persona config if not cached */
        persona_config_entry_t* entry = g_persona_configs;
        bool found_persona = false;
        while (entry) {
            if (entry->persona_name && strcmp(entry->persona_name, persona_name) == 0) {
                found_persona = true;
                if (strcmp(entry->key, key) == 0) {
                    return entry->value;
                }
            }
            entry = entry->next;
        }

        /* If persona not cached, try to load it */
        if (!found_persona) {
            read_persona_config_file(persona_name);

            /* Search again */
            entry = g_persona_configs;
            while (entry) {
                if (entry->persona_name && strcmp(entry->persona_name, persona_name) == 0) {
                    if (strcmp(entry->key, key) == 0) {
                        return entry->value;
                    }
                }
                entry = entry->next;
            }
        }
    }

    /* Fall back to global config */
    return katra_config_get(key);
}

int katra_persona_config_set(const char* persona_name, const char* key, const char* value) {
    if (!key || !value) {
        return E_INPUT_NULL;
    }

    int result;

    if (persona_name) {
        /* Persona-specific config */
        result = ensure_persona_config_dir(persona_name);
        if (result != KATRA_SUCCESS) {
            return result;
        }

        char config_file[KATRA_PATH_MAX];
        char config_dir[KATRA_PATH_MAX];

        result = katra_build_path(config_dir, sizeof(config_dir),
                                  KATRA_DIR_CONFIG, persona_name, NULL);
        if (result != KATRA_SUCCESS) {
            return result;
        }

        snprintf(config_file, sizeof(config_file), "%s/settings", config_dir);

        result = write_config_value(config_file, key, value);
        if (result != KATRA_SUCCESS) {
            return result;
        }

        /* Update cache */
        persona_config_entry_t* entry = g_persona_configs;
        while (entry) {
            if (entry->persona_name && strcmp(entry->persona_name, persona_name) == 0) {
                if (strcmp(entry->key, key) == 0) {
                    free(entry->value);
                    entry->value = strdup(value);
                    if (!entry->value) {
                        return E_SYSTEM_MEMORY;
                    }
                    return KATRA_SUCCESS;
                }
            }
            entry = entry->next;
        }

        /* Not in cache, add it */
        entry = malloc(sizeof(persona_config_entry_t));
        if (!entry) {
            return E_SYSTEM_MEMORY;
        }

        entry->persona_name = strdup(persona_name);
        entry->key = strdup(key);
        entry->value = strdup(value);
        entry->next = g_persona_configs;
        g_persona_configs = entry;

        if (!entry->persona_name || !entry->key || !entry->value) {
            return E_SYSTEM_MEMORY;
        }

        return KATRA_SUCCESS;
    } else {
        /* Global config - would need to implement global config writing */
        /* For now, just update existing katra_config system */
        LOG_WARN("Global config writing not implemented yet");
        return E_SYSTEM_FILE;
    }
}

/* ============================================================================
 * LAST PERSONA TRACKING
 * ============================================================================ */

int katra_get_last_persona(char* persona_name, size_t size) {
    if (!persona_name || size == 0) {
        return E_INPUT_NULL;
    }

    char filepath[KATRA_PATH_MAX];
    const char* home = katra_getenv("HOME");
    if (!home) {
        return E_SYSTEM_FILE;
    }

    snprintf(filepath, sizeof(filepath), "%s/.katra/k_last_persona", home);

    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        return E_NOT_FOUND;
    }

    if (!fgets(persona_name, size, fp)) {
        fclose(fp);
        return E_SYSTEM_FILE;
    }

    fclose(fp);

    /* Remove trailing newline */
    size_t len = strlen(persona_name);
    if (len > 0 && persona_name[len-1] == '\n') {
        persona_name[len-1] = '\0';
    }

    return KATRA_SUCCESS;
}

int katra_set_last_persona(const char* persona_name) {
    if (!persona_name) {
        return E_INPUT_NULL;
    }

    char filepath[KATRA_PATH_MAX];
    char temp_file[KATRA_PATH_MAX];
    const char* home = katra_getenv("HOME");
    if (!home) {
        return E_SYSTEM_FILE;
    }

    snprintf(filepath, sizeof(filepath), "%s/.katra/k_last_persona", home);
    snprintf(temp_file, sizeof(temp_file), "%s/.katra/k_last_persona.tmp", home);

    /* Write atomically */
    FILE* fp = fopen(temp_file, "w");
    if (!fp) {
        return E_SYSTEM_FILE;
    }

    fprintf(fp, "%s\n", persona_name);
    fclose(fp);

    if (rename(temp_file, filepath) != 0) {
        unlink(temp_file);
        return E_SYSTEM_FILE;
    }

    return KATRA_SUCCESS;
}

/* ============================================================================
 * PERSONA MANAGEMENT
 * ============================================================================ */

int katra_list_personas(char*** personas, size_t* count) {
    if (!personas || !count) {
        return E_INPUT_NULL;
    }

    *personas = NULL;
    *count = 0;

    char config_dir[KATRA_PATH_MAX];
    int result = katra_build_path(config_dir, sizeof(config_dir),
                                  KATRA_DIR_CONFIG, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    DIR* dir = opendir(config_dir);
    if (!dir) {
        /* No config directory yet */
        return KATRA_SUCCESS;
    }

    char** persona_list = NULL;
    size_t persona_count = 0;
    size_t capacity = 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Check if it's a directory with a settings file */
        char persona_dir[KATRA_PATH_MAX];
        char settings_file[KATRA_PATH_MAX];
        snprintf(persona_dir, sizeof(persona_dir), "%s/%s", config_dir, entry->d_name);
        snprintf(settings_file, sizeof(settings_file), "%s/settings", persona_dir);

        struct stat st;
        if (stat(persona_dir, &st) == 0 && S_ISDIR(st.st_mode)) {
            /* It's a directory - add to list */
            if (persona_count >= capacity) {
                size_t new_capacity = capacity == 0 ? 8 : capacity * 2;
                char** new_list = realloc(persona_list, new_capacity * sizeof(char*));
                if (!new_list) {
                    katra_free_string_array(persona_list, persona_count);
                    closedir(dir);
                    return E_SYSTEM_MEMORY;
                }
                persona_list = new_list;
                capacity = new_capacity;
            }

            persona_list[persona_count] = strdup(entry->d_name);
            if (!persona_list[persona_count]) {
                katra_free_string_array(persona_list, persona_count);
                closedir(dir);
                return E_SYSTEM_MEMORY;
            }
            persona_count++;
        }
    }

    closedir(dir);

    *personas = persona_list;
    *count = persona_count;
    return KATRA_SUCCESS;
}

int katra_delete_persona_config(const char* persona_name) {
    if (!persona_name) {
        return E_INPUT_NULL;
    }

    char persona_dir[KATRA_PATH_MAX];
    int result = katra_build_path(persona_dir, sizeof(persona_dir),
                                  KATRA_DIR_CONFIG, persona_name, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Remove settings file */
    char settings_file[KATRA_PATH_MAX];
    snprintf(settings_file, sizeof(settings_file), "%s/settings", persona_dir);
    unlink(settings_file);

    /* Remove directory */
    if (rmdir(persona_dir) != 0) {
        return E_SYSTEM_FILE;
    }

    /* Remove from cache */
    persona_config_entry_t** current = &g_persona_configs;
    while (*current) {
        if ((*current)->persona_name && strcmp((*current)->persona_name, persona_name) == 0) {
            persona_config_entry_t* to_delete = *current;
            *current = (*current)->next;
            free(to_delete->persona_name);
            free(to_delete->key);
            free(to_delete->value);
            free(to_delete);
        } else {
            current = &(*current)->next;
        }
    }

    return KATRA_SUCCESS;
}
