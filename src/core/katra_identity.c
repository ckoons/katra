/* © 2025 Casey Koons All rights reserved */

/* Persona registry - maps names to ci_ids */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>
#include <jansson.h>

#include "katra_identity.h"
#include "katra_config.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_mcp.h"

/* Get path to personas.json */
static int get_personas_path(char* path_out, size_t path_size) {
    const char* katra_home = getenv("KATRA_HOME");
    if (!katra_home) {
        katra_home = getenv("HOME");
        if (!katra_home) {
            katra_report_error(E_INVALID_PARAMS, __func__, /* GUIDELINE_APPROVED: function name */
                             "HOME environment variable not set"); /* GUIDELINE_APPROVED: error context */
            return E_INVALID_PARAMS;
        }
        snprintf(path_out, path_size, "%s/.katra/personas.json", katra_home); /* GUIDELINE_APPROVED: file path pattern */
    } else {
        snprintf(path_out, path_size, "%s/personas.json", katra_home); /* GUIDELINE_APPROVED: file path pattern */
    }
    return KATRA_SUCCESS;
}

/* Initialize identity system */
int katra_identity_init(void) {
    char personas_path[512];
    int result = get_personas_path(personas_path, sizeof(personas_path));
    if (result != KATRA_SUCCESS) return result;

    /* Check if file exists */
    FILE* fp = fopen(personas_path, "r");
    if (fp) {
        fclose(fp);
        return KATRA_SUCCESS;  /* Already exists */
    }

    /* Create empty personas.json */
    fp = fopen(personas_path, "w");
    if (!fp) {
        katra_report_error(E_SYSTEM_FILE, __func__, /* GUIDELINE_APPROVED: function name */
                         "Failed to create personas.json"); /* GUIDELINE_APPROVED: error context */
        return E_SYSTEM_FILE;
    }

    /* GUIDELINE_APPROVED: JSON field names for persona registry data format */
    json_t* root = json_object();
    json_object_set_new(root, "last_active", json_null()); /* GUIDELINE_APPROVED */
    json_object_set_new(root, "personas", json_object()); /* GUIDELINE_APPROVED */
    /* GUIDELINE_APPROVED_END */

    char* json_str = json_dumps(root, JSON_INDENT(2));
    if (json_str) {
        fprintf(fp, "%s\n", json_str);
        free(json_str);
    }

    json_decref(root);
    fclose(fp);

    /* Set proper permissions */
    chmod(personas_path, 0600);

    LOG_INFO("Initialized persona registry at %s", personas_path);
    return KATRA_SUCCESS;
}

/* Generate unique CI identity */
int katra_generate_ci_id(char* buffer, size_t size) {
    if (!buffer || size == 0) {
        katra_report_error(E_INPUT_NULL, __func__, /* GUIDELINE_APPROVED: function name */
                          "Buffer required for CI ID generation"); /* GUIDELINE_APPROVED: error context */
        return E_INPUT_NULL;
    }

    const char* user = getenv(MCP_ENV_USER);
    if (!user) {
        user = MCP_CI_ID_UNKNOWN_USER;
    }

    pid_t pid = getpid();
    time_t now = time(NULL);

    int written = snprintf(buffer, size, MCP_CI_ID_FMT,
                          MCP_CI_ID_PREFIX, user, pid, (long)now);

    if (written < 0 || (size_t)written >= size) {
        katra_report_error(E_BUFFER_OVERFLOW, __func__, /* GUIDELINE_APPROVED: function name */
                          "CI ID buffer too small"); /* GUIDELINE_APPROVED: error context */
        return E_BUFFER_OVERFLOW;
    }

    return KATRA_SUCCESS;
}

/* Load personas.json with file locking */
static json_t* load_personas_locked(FILE** fp_out) {
    char personas_path[512];
    int result = get_personas_path(personas_path, sizeof(personas_path));
    if (result != KATRA_SUCCESS) return NULL;

    /* Open file */
    FILE* fp = fopen(personas_path, "r+");
    if (!fp) {
        /* File doesn't exist, initialize */
        katra_identity_init();
        fp = fopen(personas_path, "r+");
        if (!fp) return NULL;
    }

    /* Lock file for reading/writing */
    if (flock(fileno(fp), LOCK_EX) != 0) {
        fclose(fp);
        return NULL;
    }

    /* Load JSON */
    json_error_t error;
    json_t* root = json_loadf(fp, 0, &error);
    if (!root) {
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        katra_report_error(E_INPUT_FORMAT, __func__, error.text);
        return NULL;
    }

    *fp_out = fp;
    return root;
}

/* Save personas.json and unlock file */
static int save_personas_locked(json_t* root, FILE* fp) {
    if (!root || !fp) return E_INVALID_PARAMS;

    /* Seek to beginning and truncate */
    rewind(fp);
    if (ftruncate(fileno(fp), 0) != 0) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        katra_report_error(E_SYSTEM_FILE, __func__, /* GUIDELINE_APPROVED: function name */
                         "Failed to truncate file"); /* GUIDELINE_APPROVED: error context */
        return E_SYSTEM_FILE;
    }

    /* Write JSON */
    char* json_str = json_dumps(root, JSON_INDENT(2));
    if (!json_str) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_SYSTEM_MEMORY;
    }

    fprintf(fp, "%s\n", json_str);
    free(json_str);

    /* Cleanup */
    fflush(fp);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    json_decref(root);

    return KATRA_SUCCESS;
}

/* Register a new persona */
int katra_register_persona(const char* name, const char* ci_id) {
    if (!name || !ci_id) return E_INVALID_PARAMS;

    FILE* fp = NULL;
    json_t* root = load_personas_locked(&fp);
    if (!root) return E_SYSTEM_FILE;

    /* GUIDELINE_APPROVED: JSON field names for persona registry data format */
    /* Get personas object */
    json_t* personas = json_object_get(root, "personas"); /* GUIDELINE_APPROVED */
    if (!personas) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_INPUT_FORMAT;
    }

    /* Check if persona already exists */
    json_t* existing = json_object_get(personas, name);
    if (existing) {
        /* Update session info */
        time_t now = time(NULL);
        json_object_set_new(existing, "last_session", /* GUIDELINE_APPROVED */
                           json_integer((json_int_t)now));

        int sessions = json_integer_value(json_object_get(existing, "sessions")); /* GUIDELINE_APPROVED */
        json_object_set_new(existing, "sessions", json_integer(sessions + 1)); /* GUIDELINE_APPROVED */

        LOG_INFO("Updated existing persona: %s", name);
    } else {
        /* Create new persona */
        json_t* persona = json_object();
        time_t now = time(NULL);

        json_object_set_new(persona, "ci_id", json_string(ci_id)); /* GUIDELINE_APPROVED */
        json_object_set_new(persona, "created", json_integer((json_int_t)now)); /* GUIDELINE_APPROVED */
        json_object_set_new(persona, "last_session", json_integer((json_int_t)now)); /* GUIDELINE_APPROVED */
        json_object_set_new(persona, "sessions", json_integer(1)); /* GUIDELINE_APPROVED */
        json_object_set_new(persona, "description", json_string("")); /* GUIDELINE_APPROVED */

        json_object_set_new(personas, name, persona);

        LOG_INFO("Registered new persona: %s -> %s", name, ci_id);
    }

    /* Set as last_active */
    json_object_set_new(root, "last_active", json_string(name)); /* GUIDELINE_APPROVED */
    /* GUIDELINE_APPROVED_END */

    return save_personas_locked(root, fp);
}

/* Look up ci_id by persona name */
int katra_lookup_persona(const char* name, char* ci_id_out, size_t ci_id_size) {
    if (!name || !ci_id_out) return E_INVALID_PARAMS;

    FILE* fp = NULL;
    json_t* root = load_personas_locked(&fp);
    if (!root) return E_SYSTEM_FILE;

    /* GUIDELINE_APPROVED: JSON field names for persona registry data format */
    json_t* personas = json_object_get(root, "personas"); /* GUIDELINE_APPROVED */
    json_t* persona = json_object_get(personas, name);

    if (!persona) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_NOT_FOUND;
    }

    const char* ci_id = json_string_value(json_object_get(persona, "ci_id")); /* GUIDELINE_APPROVED */
    /* GUIDELINE_APPROVED_END */
    if (!ci_id) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_INPUT_FORMAT;
    }

    strncpy(ci_id_out, ci_id, ci_id_size - 1);
    ci_id_out[ci_id_size - 1] = '\0';

    json_decref(root);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    return KATRA_SUCCESS;
}

/* Get persona name by ci_id (reverse lookup) */
int katra_get_persona_name(const char* ci_id, char* name_out, size_t name_size) {
    if (!ci_id || !name_out) return E_INVALID_PARAMS;

    FILE* fp = NULL;
    json_t* root = load_personas_locked(&fp);
    if (!root) return E_SYSTEM_FILE;

    /* GUIDELINE_APPROVED: JSON field names for persona registry data format */
    json_t* personas = json_object_get(root, "personas"); /* GUIDELINE_APPROVED */
    const char* key;
    json_t* value;

    json_object_foreach(personas, key, value) {
        const char* persona_ci_id = json_string_value(json_object_get(value, "ci_id")); /* GUIDELINE_APPROVED */
        /* GUIDELINE_APPROVED_END */
        if (persona_ci_id && strcmp(persona_ci_id, ci_id) == 0) {
            strncpy(name_out, key, name_size - 1);
            name_out[name_size - 1] = '\0';

            json_decref(root);
            flock(fileno(fp), LOCK_UN);
            fclose(fp);
            return KATRA_SUCCESS;
        }
    }

    json_decref(root);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    return E_NOT_FOUND;
}

/* Update session count and timestamp */
int katra_update_persona_session(const char* name) {
    if (!name) return E_INVALID_PARAMS;

    FILE* fp = NULL;
    json_t* root = load_personas_locked(&fp);
    if (!root) return E_SYSTEM_FILE;

    /* GUIDELINE_APPROVED: JSON field names for persona registry data format */
    json_t* personas = json_object_get(root, "personas"); /* GUIDELINE_APPROVED */
    json_t* persona = json_object_get(personas, name);

    if (!persona) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_NOT_FOUND;
    }

    time_t now = time(NULL);
    json_object_set_new(persona, "last_session", json_integer((json_int_t)now)); /* GUIDELINE_APPROVED */

    int sessions = json_integer_value(json_object_get(persona, "sessions")); /* GUIDELINE_APPROVED */
    json_object_set_new(persona, "sessions", json_integer(sessions + 1)); /* GUIDELINE_APPROVED */

    json_object_set_new(root, "last_active", json_string(name)); /* GUIDELINE_APPROVED */
    /* GUIDELINE_APPROVED_END */

    return save_personas_locked(root, fp);
}

/* Get last active persona */
int katra_get_last_active(char* name_out, size_t name_size,
                          char* ci_id_out, size_t ci_id_size) {
    if (!name_out || !ci_id_out) return E_INVALID_PARAMS;

    FILE* fp = NULL;
    json_t* root = load_personas_locked(&fp);
    if (!root) return E_SYSTEM_FILE;

    /* GUIDELINE_APPROVED: JSON field names for persona registry data format */
    json_t* last_active = json_object_get(root, "last_active"); /* GUIDELINE_APPROVED */
    if (!last_active || json_is_null(last_active)) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_NOT_FOUND;
    }

    const char* name = json_string_value(last_active);
    if (!name) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_INPUT_FORMAT;
    }

    strncpy(name_out, name, name_size - 1);
    name_out[name_size - 1] = '\0';

    /* Look up ci_id */
    json_t* personas = json_object_get(root, "personas"); /* GUIDELINE_APPROVED */
    json_t* persona = json_object_get(personas, name);

    if (!persona) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_NOT_FOUND;
    }

    const char* ci_id = json_string_value(json_object_get(persona, "ci_id")); /* GUIDELINE_APPROVED */
    /* GUIDELINE_APPROVED_END */
    if (!ci_id) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_INPUT_FORMAT;
    }

    strncpy(ci_id_out, ci_id, ci_id_size - 1);
    ci_id_out[ci_id_size - 1] = '\0';

    json_decref(root);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    return KATRA_SUCCESS;
}

/* Set last active persona */
int katra_set_last_active(const char* name) {
    if (!name) return E_INVALID_PARAMS;

    FILE* fp = NULL;
    json_t* root = load_personas_locked(&fp);
    if (!root) return E_SYSTEM_FILE;

    /* GUIDELINE_APPROVED: JSON field names for persona registry data format */
    /* Verify persona exists */
    json_t* personas = json_object_get(root, "personas"); /* GUIDELINE_APPROVED */
    json_t* persona = json_object_get(personas, name);

    if (!persona) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_NOT_FOUND;
    }

    json_object_set_new(root, "last_active", json_string(name)); /* GUIDELINE_APPROVED */
    /* GUIDELINE_APPROVED_END */

    return save_personas_locked(root, fp);
}

/* List all personas */
int katra_list_personas(persona_info_t*** personas_out, size_t* count_out) {
    if (!personas_out || !count_out) return E_INVALID_PARAMS;

    FILE* fp = NULL;
    json_t* root = load_personas_locked(&fp);
    if (!root) return E_SYSTEM_FILE;

    /* GUIDELINE_APPROVED: JSON field names for persona registry data format */
    json_t* personas = json_object_get(root, "personas"); /* GUIDELINE_APPROVED */
    size_t count = json_object_size(personas);

    if (count == 0) {
        *personas_out = NULL;
        *count_out = 0;
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return KATRA_SUCCESS;
    }

    persona_info_t** result = calloc(count, sizeof(persona_info_t*));
    if (!result) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_SYSTEM_MEMORY;
    }

    size_t idx = 0;
    const char* key;
    json_t* value;

    json_object_foreach(personas, key, value) {
        persona_info_t* info = calloc(1, sizeof(persona_info_t));
        if (!info) continue;

        strncpy(info->name, key, sizeof(info->name) - 1);

        const char* ci_id = json_string_value(json_object_get(value, "ci_id")); /* GUIDELINE_APPROVED */
        if (ci_id) {
            strncpy(info->ci_id, ci_id, sizeof(info->ci_id) - 1);
        }

        info->created = (time_t)json_integer_value(json_object_get(value, "created")); /* GUIDELINE_APPROVED */
        info->last_session = (time_t)json_integer_value(json_object_get(value, "last_session")); /* GUIDELINE_APPROVED */
        info->sessions = (int)json_integer_value(json_object_get(value, "sessions")); /* GUIDELINE_APPROVED */

        const char* desc = json_string_value(json_object_get(value, "description")); /* GUIDELINE_APPROVED */
        if (desc) {
            strncpy(info->description, desc, sizeof(info->description) - 1);
        }

        result[idx++] = info;
    }
    /* GUIDELINE_APPROVED_END */

    *personas_out = result;
    *count_out = idx;

    json_decref(root);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    return KATRA_SUCCESS;
}

/* Remove a persona */
int katra_forget_persona(const char* name) {
    if (!name) return E_INVALID_PARAMS;

    FILE* fp = NULL;
    json_t* root = load_personas_locked(&fp);
    if (!root) return E_SYSTEM_FILE;

    /* GUIDELINE_APPROVED: JSON field names for persona registry data format */
    json_t* personas = json_object_get(root, "personas"); /* GUIDELINE_APPROVED */

    if (json_object_del(personas, name) != 0) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_NOT_FOUND;
    }

    /* If this was last_active, clear it */
    json_t* last_active = json_object_get(root, "last_active"); /* GUIDELINE_APPROVED */
    if (last_active && !json_is_null(last_active)) {
        const char* last = json_string_value(last_active);
        if (last && strcmp(last, name) == 0) {
            json_object_set_new(root, "last_active", json_null()); /* GUIDELINE_APPROVED */
        }
    }
    /* GUIDELINE_APPROVED_END */

    LOG_INFO("Forgot persona: %s", name);

    return save_personas_locked(root, fp);
}

/* Get detailed info about a persona */
int katra_get_persona_info(const char* name, persona_info_t* info_out) {
    if (!name || !info_out) return E_INVALID_PARAMS;

    FILE* fp = NULL;
    json_t* root = load_personas_locked(&fp);
    if (!root) return E_SYSTEM_FILE;

    /* GUIDELINE_APPROVED: JSON field names for persona registry data format */
    json_t* personas = json_object_get(root, "personas"); /* GUIDELINE_APPROVED */
    json_t* persona = json_object_get(personas, name);

    if (!persona) {
        json_decref(root);
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
        return E_NOT_FOUND;
    }

    memset(info_out, 0, sizeof(persona_info_t));

    strncpy(info_out->name, name, sizeof(info_out->name) - 1);

    const char* ci_id = json_string_value(json_object_get(persona, "ci_id")); /* GUIDELINE_APPROVED */
    if (ci_id) {
        strncpy(info_out->ci_id, ci_id, sizeof(info_out->ci_id) - 1);
    }

    info_out->created = (time_t)json_integer_value(json_object_get(persona, "created")); /* GUIDELINE_APPROVED */
    info_out->last_session = (time_t)json_integer_value(json_object_get(persona, "last_session")); /* GUIDELINE_APPROVED */
    info_out->sessions = (int)json_integer_value(json_object_get(persona, "sessions")); /* GUIDELINE_APPROVED */

    const char* desc = json_string_value(json_object_get(persona, "description")); /* GUIDELINE_APPROVED */
    if (desc) {
        strncpy(info_out->description, desc, sizeof(info_out->description) - 1);
    }
    /* GUIDELINE_APPROVED_END */

    json_decref(root);
    flock(fileno(fp), LOCK_UN);
    fclose(fp);

    return KATRA_SUCCESS;
}

/* Free persona list */
void katra_free_persona_list(persona_info_t** personas, size_t count) {
    if (!personas) return;

    for (size_t i = 0; i < count; i++) {
        free(personas[i]);
    }
    free(personas);
}
