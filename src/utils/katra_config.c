/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* Project includes */
#include "katra_config.h"
#include "katra_env_utils.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"

/* Config entry structure */
typedef struct config_entry {
    char* key;
    char* value;
    struct config_entry* next;
} config_entry_t;

/* Global configuration state */
static config_entry_t* config_head = NULL;
static bool config_initialized = false;

/* Forward declarations */
static int load_config_directory(const char* dir_path);
static int load_config_file(const char* file_path);
static int parse_config_line(const char* line, char** key, char** value);
static void set_config_internal(const char* key, const char* value);
static config_entry_t* find_config_entry(const char* key);
static void free_config_entries(void);
static void create_directory_structure(void);
static void trim_whitespace(char* str);
static void strip_quotes(char* str);

/* Create .katra directory structure */
static void create_directory_structure(void) {
    char dir_path[KATRA_PATH_MAX];

    /* Create ~/.katra/config */
    if (katra_build_and_ensure_dir(dir_path, sizeof(dir_path), KATRA_DIR_CONFIG, NULL) != KATRA_SUCCESS) {
        LOG_WARN("Failed to create config directory");
    }

    /* Create ~/.katra/logs */
    if (katra_build_and_ensure_dir(dir_path, sizeof(dir_path), KATRA_DIR_LOGS, NULL) != KATRA_SUCCESS) {
        LOG_WARN("Failed to create logs directory");
    }

    /* Create ~/.katra/memory */
    if (katra_build_and_ensure_dir(dir_path, sizeof(dir_path), KATRA_DIR_MEMORY, NULL) != KATRA_SUCCESS) {
        LOG_WARN("Failed to create memory directory");
    }

    /* Create ~/.katra/checkpoints */
    if (katra_build_and_ensure_dir(dir_path, sizeof(dir_path), KATRA_DIR_CHECKPOINTS, NULL) != KATRA_SUCCESS) {
        LOG_WARN("Failed to create checkpoints directory");
    }

    /* Create ~/.katra/audit */
    if (katra_build_and_ensure_dir(dir_path, sizeof(dir_path), KATRA_DIR_AUDIT, NULL) != KATRA_SUCCESS) {
        LOG_WARN("Failed to create audit directory");
    }

    /* If we have KATRA_ROOT, create project directories */
    const char* katra_root = katra_getenv("KATRA_ROOT");
    if (katra_root && katra_root[0]) {
        /* Create <project>/.katra */
        snprintf(dir_path, sizeof(dir_path), "%s/.katra", katra_root);
        if (katra_ensure_dir(dir_path) != KATRA_SUCCESS) {
            LOG_WARN("Failed to create project .katra directory");
        }

        /* Create <project>/.katra/config */
        snprintf(dir_path, sizeof(dir_path), "%s/.katra/config", katra_root);
        if (katra_ensure_dir(dir_path) != KATRA_SUCCESS) {
            LOG_WARN("Failed to create project config directory");
        }
    }
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

/* Parse config line into key/value */
static int parse_config_line(const char* line, char** key, char** value) {
    *key = NULL;
    *value = NULL;

    char* buffer = strdup(line);
    if (!buffer) return E_SYSTEM_MEMORY;

    trim_whitespace(buffer);

    /* Skip empty lines and comments */
    if (buffer[0] == '\0' || buffer[0] == '#') {
        free(buffer);
        return KATRA_SUCCESS;
    }

    /* Find = separator */
    char* eq = strchr(buffer, '=');
    if (!eq) {
        free(buffer);
        return KATRA_SUCCESS;  /* Not an error, just skip */
    }

    *eq = '\0';
    char* key_str = buffer;
    char* val_str = eq + 1;

    trim_whitespace(key_str);
    trim_whitespace(val_str);
    strip_quotes(val_str);

    if (key_str[0] == '\0') {
        free(buffer);
        return KATRA_SUCCESS;
    }

    /* GUIDELINE_APPROVED - Aggregate NULL check pattern */
    *key = strdup(key_str);
    *value = strdup(val_str);

    free(buffer);

    if (!*key || !*value) {
    /* GUIDELINE_APPROVED_END */
        free(*key);
        free(*value);
        *key = *value = NULL;
        return E_SYSTEM_MEMORY;
    }

    return KATRA_SUCCESS;
}

/* Find config entry by key */
static config_entry_t* find_config_entry(const char* key) {
    if (!key) return NULL;

    config_entry_t* entry = config_head;
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/* Set config value (internal, creates or updates) */
static void set_config_internal(const char* key, const char* value) {
    if (!key || !value) return;

    /* Check if key exists */
    config_entry_t* existing = find_config_entry(key);
    if (existing) {
        /* Update existing */
        free(existing->value);
        existing->value = strdup(value);
        if (!existing->value) {
            LOG_ERROR("Failed to allocate memory for config value");
            existing->value = NULL;
        }
        return;
    }

    /* Create new entry */
    config_entry_t* entry = calloc(1, sizeof(config_entry_t));
    if (!entry) return;

    /* GUIDELINE_APPROVED - Aggregate NULL check pattern */
    entry->key = strdup(key);
    entry->value = strdup(value);

    if (!entry->key || !entry->value) {
    /* GUIDELINE_APPROVED_END */
        free(entry->key);
        free(entry->value);
        free(entry);
        return;
    }

    /* Add to head of list */
    entry->next = config_head;
    config_head = entry;
}

/* Load single config file */
static int load_config_file(const char* file_path) {
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        /* Not an error - config files are optional */
        return KATRA_SUCCESS;
    }

    LOG_DEBUG("Loading config file: %s", file_path);

    char line[KATRA_BUFFER_STANDARD];
    int loaded = 0;

    while (fgets(line, sizeof(line), fp)) { /* GUIDELINE_APPROVED: fgets in while condition */
        /* Remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        char* key = NULL;
        char* value = NULL;
        int result = parse_config_line(line, &key, &value);

        if (result == KATRA_SUCCESS && key && value) {
            set_config_internal(key, value);
            loaded++;
        }

        free(key);
        free(value);
    }

    /* Check for read errors */
    if (ferror(fp)) {
        LOG_WARN("Error reading config file: %s", file_path);
    }

    fclose(fp);

    if (loaded > 0) {
        LOG_INFO("Loaded %d config values from %s", loaded, file_path);
    }

    return KATRA_SUCCESS;
}

/* Load all config files from directory */
static int load_config_directory(const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        /* Not an error - directories are optional */
        return KATRA_SUCCESS;
    }

    LOG_DEBUG("Scanning config directory: %s", dir_path);

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Build full path */
        char file_path[KATRA_PATH_MAX];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

        /* Check if it's a regular file */
        struct stat st;
        if (stat(file_path, &st) == 0 && S_ISREG(st.st_mode)) {
            load_config_file(file_path);
        }
    }

    closedir(dir);
    return KATRA_SUCCESS;
}

/* Free all config entries */
static void free_config_entries(void) {
    config_entry_t* entry = config_head;
    while (entry) {
        config_entry_t* next = entry->next;
        free(entry->key);
        free(entry->value);
        free(entry);
        entry = next;
    }
    config_head = NULL;
}

/* Load Katra configuration */
int katra_config(void) {
    if (config_initialized) {
        LOG_DEBUG("Config already initialized");
        return KATRA_SUCCESS;
    }

    LOG_INFO("Loading Katra configuration");

    /* Create directory structure */
    create_directory_structure();

    /* Load config in precedence order (later overrides earlier) */
    char config_dir[KATRA_PATH_MAX];

    /* 1. Load ~/.katra/config/ directory */
    if (katra_build_path(config_dir, sizeof(config_dir), KATRA_DIR_CONFIG, NULL) == KATRA_SUCCESS) {
        load_config_directory(config_dir);
    }

    /* 2. Load <project>/.katra/config/ directory */
    const char* katra_root = katra_getenv("KATRA_ROOT");
    if (katra_root && katra_root[0]) {
        char config_dir[KATRA_PATH_MAX];

        snprintf(config_dir, sizeof(config_dir), "%s/.katra/config", katra_root);
        load_config_directory(config_dir);
    }

    config_initialized = true;

    int count = 0;
    config_entry_t* entry = config_head;
    while (entry) {
        count++;
        entry = entry->next;
    }

    LOG_INFO("Configuration loaded: %d values", count);
    return KATRA_SUCCESS;
}

/* Get configuration value */
const char* katra_config_get(const char* key) {
    if (!key) return NULL;

    config_entry_t* entry = find_config_entry(key);
    return entry ? entry->value : NULL;
}

/* Reload configuration */
int katra_config_reload(void) {
    LOG_INFO("Reloading configuration");

    /* Clear existing config */
    free_config_entries();
    config_initialized = false;

    /* Reload */
    return katra_config();
}

/* Cleanup configuration subsystem */
void katra_config_cleanup(void) {
    if (!config_initialized) return;

    LOG_DEBUG("Cleaning up configuration");

    free_config_entries();
    config_initialized = false;
}
