/* © 2025 Casey Koons All rights reserved */

/* Katra Interstitial Autonomy Daemon - Core Infrastructure */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

#include "katra_daemon.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_config.h"
#include "katra_path_utils.h"
#include "katra_file_utils.h"
#include "katra_breathing.h"
#include "katra_core_common.h"

/* Database handle */
static sqlite3* daemon_db = NULL;
static bool daemon_initialized = false;

/* Insight type names */
static const char* INSIGHT_TYPE_NAMES[] = {
    "pattern",
    "association",
    "theme",
    "temporal",
    "emotional"
};

/* Forward declarations */
static int daemon_create_tables(void);

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int katra_daemon_init(void) {
    if (daemon_initialized) {
        return KATRA_SUCCESS;
    }

    int result = KATRA_SUCCESS;
    char db_path[KATRA_PATH_MAX];
    char daemon_dir[KATRA_PATH_MAX];

    /* Build database path under ~/.katra/daemon/ */
    result = katra_build_and_ensure_dir(daemon_dir, sizeof(daemon_dir), "daemon", NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    result = katra_path_join(db_path, sizeof(db_path), daemon_dir, "daemon.db");
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Open database */
    int rc = sqlite3_open(db_path, &daemon_db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to open daemon database: %s", db_path);
        return E_SYSTEM_FILE;
    }

    /* Create tables */
    result = daemon_create_tables();
    if (result != KATRA_SUCCESS) {
        sqlite3_close(daemon_db);
        daemon_db = NULL;
        return result;
    }

    daemon_initialized = true;
    LOG_INFO("Daemon subsystem initialized");
    return KATRA_SUCCESS;
}

void katra_daemon_cleanup(void) {
    if (daemon_db) {
        sqlite3_close(daemon_db);
        daemon_db = NULL;
    }
    daemon_initialized = false;
}

static int daemon_create_tables(void) {
    const char* sql =
        /* Daemon run history */
        "CREATE TABLE IF NOT EXISTS daemon_runs ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ci_id TEXT NOT NULL,"
        "  run_start INTEGER NOT NULL,"
        "  run_end INTEGER,"
        "  memories_processed INTEGER DEFAULT 0,"
        "  patterns_found INTEGER DEFAULT 0,"
        "  associations_formed INTEGER DEFAULT 0,"
        "  themes_detected INTEGER DEFAULT 0,"
        "  insights_generated INTEGER DEFAULT 0,"
        "  error_code INTEGER DEFAULT 0"
        ");"

        /* Generated insights */
        "CREATE TABLE IF NOT EXISTS daemon_insights ("
        "  id TEXT PRIMARY KEY,"
        "  ci_id TEXT NOT NULL,"
        "  type INTEGER NOT NULL,"
        "  content TEXT NOT NULL,"
        "  source_ids TEXT,"
        "  confidence REAL DEFAULT 0.5,"
        "  generated_at INTEGER NOT NULL,"
        "  acknowledged INTEGER DEFAULT 0,"
        "  memory_id TEXT"  /* Reference to stored memory if persisted */
        ");"

        /* Detected patterns (for tracking) */
        "CREATE TABLE IF NOT EXISTS daemon_patterns ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ci_id TEXT NOT NULL,"
        "  pattern_desc TEXT NOT NULL,"
        "  occurrence_count INTEGER DEFAULT 0,"
        "  memory_ids TEXT,"
        "  strength REAL DEFAULT 0.5,"
        "  detected_at INTEGER NOT NULL"
        ");"

        /* Theme clusters */
        "CREATE TABLE IF NOT EXISTS daemon_themes ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ci_id TEXT NOT NULL,"
        "  theme_name TEXT NOT NULL,"
        "  theme_desc TEXT,"
        "  memory_ids TEXT,"
        "  coherence REAL DEFAULT 0.5,"
        "  detected_at INTEGER NOT NULL"
        ");"

        /* Indexes */
        "CREATE INDEX IF NOT EXISTS idx_runs_ci ON daemon_runs(ci_id);"
        "CREATE INDEX IF NOT EXISTS idx_insights_ci ON daemon_insights(ci_id);"
        "CREATE INDEX IF NOT EXISTS idx_insights_ack ON daemon_insights(ci_id, acknowledged);"
        "CREATE INDEX IF NOT EXISTS idx_patterns_ci ON daemon_patterns(ci_id);"
        "CREATE INDEX IF NOT EXISTS idx_themes_ci ON daemon_themes(ci_id);";

    char* err = NULL;
    int rc = sqlite3_exec(daemon_db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create daemon tables: %s", err ? err : "unknown");
        sqlite3_free(err);
        return E_SYSTEM_FILE;
    }

    return KATRA_SUCCESS;
}

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

void katra_daemon_default_config(daemon_config_t* config) {
    if (!config) return;

    config->enabled = true;
    config->interval_minutes = DAEMON_DEFAULT_INTERVAL_MINUTES;
    config->quiet_hours_start = 22;  /* 10 PM */
    config->quiet_hours_end = 6;     /* 6 AM */
    config->max_memories_per_run = DAEMON_DEFAULT_MAX_MEMORIES;

    config->pattern_extraction = true;
    config->association_formation = true;
    config->theme_detection = true;
    config->insight_generation = true;

    config->notify_on_insight = true;
}

int katra_daemon_load_config(daemon_config_t* config) {
    if (!config) return E_INPUT_NULL;

    /* Start with defaults */
    katra_daemon_default_config(config);

    /* Load from config file if it exists */
    char config_path[KATRA_PATH_MAX];
    char daemon_dir[KATRA_PATH_MAX];

    int result = katra_build_path(daemon_dir, sizeof(daemon_dir), "daemon", NULL);
    if (result != KATRA_SUCCESS) return result;

    result = katra_path_join(config_path, sizeof(config_path), daemon_dir, "daemon.conf");
    if (result != KATRA_SUCCESS) return result;

    FILE* fp = fopen(config_path, "r");
    if (!fp) {
        /* No config file, use defaults */
        return KATRA_SUCCESS;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '[') continue;

        char key[64], value[128];
        if (sscanf(line, "%63[^=]=%127s", key, value) == 2) {
            /* Trim leading whitespace */
            char* k = key;
            while (*k == ' ') k++;
            /* Trim trailing whitespace from key */
            char* end = k + strlen(k) - 1;
            while (end > k && (*end == ' ' || *end == '\t')) end--;
            *(end + 1) = '\0';
            char* v = value;
            while (*v == ' ') v++;

            if (strcmp(k, "enabled") == 0) {
                config->enabled = (strcmp(v, "true") == 0 || strcmp(v, "1") == 0);
            } else if (strcmp(k, "interval_minutes") == 0) {
                config->interval_minutes = atoi(v);
            } else if (strcmp(k, "quiet_hours_start") == 0) {
                config->quiet_hours_start = atoi(v);
            } else if (strcmp(k, "quiet_hours_end") == 0) {
                config->quiet_hours_end = atoi(v);
            } else if (strcmp(k, "max_memories_per_run") == 0) {
                config->max_memories_per_run = (size_t)atoi(v);
            } else if (strcmp(k, "pattern_extraction") == 0) {
                config->pattern_extraction = (strcmp(v, "true") == 0);
            } else if (strcmp(k, "association_formation") == 0) {
                config->association_formation = (strcmp(v, "true") == 0);
            } else if (strcmp(k, "theme_detection") == 0) {
                config->theme_detection = (strcmp(v, "true") == 0);
            } else if (strcmp(k, "insight_generation") == 0) {
                config->insight_generation = (strcmp(v, "true") == 0);
            } else if (strcmp(k, "notify_on_insight") == 0) {
                config->notify_on_insight = (strcmp(v, "true") == 0);
            }
        }
    }

    fclose(fp);
    return KATRA_SUCCESS;
}

int katra_daemon_save_config(const daemon_config_t* config) {
    if (!config) return E_INPUT_NULL;

    char config_path[KATRA_PATH_MAX];
    char daemon_dir[KATRA_PATH_MAX];

    int result = katra_build_and_ensure_dir(daemon_dir, sizeof(daemon_dir), "daemon", NULL);
    if (result != KATRA_SUCCESS) return result;

    result = katra_path_join(config_path, sizeof(config_path), daemon_dir, "daemon.conf");
    if (result != KATRA_SUCCESS) return result;

    FILE* fp = fopen(config_path, "w");
    if (!fp) return E_SYSTEM_FILE;

    fprintf(fp, "# Katra Daemon Configuration\n");
    fprintf(fp, "# Generated at %ld\n\n", (long)time(NULL));

    fprintf(fp, "[daemon]\n");
    fprintf(fp, "enabled = %s\n", config->enabled ? "true" : "false");
    fprintf(fp, "interval_minutes = %d\n", config->interval_minutes);
    fprintf(fp, "quiet_hours_start = %d\n", config->quiet_hours_start);
    fprintf(fp, "quiet_hours_end = %d\n", config->quiet_hours_end);
    fprintf(fp, "max_memories_per_run = %zu\n\n", config->max_memories_per_run);

    fprintf(fp, "[processing]\n");
    fprintf(fp, "pattern_extraction = %s\n", config->pattern_extraction ? "true" : "false");
    fprintf(fp, "association_formation = %s\n", config->association_formation ? "true" : "false");
    fprintf(fp, "theme_detection = %s\n", config->theme_detection ? "true" : "false");
    fprintf(fp, "insight_generation = %s\n\n", config->insight_generation ? "true" : "false");

    fprintf(fp, "[output]\n");
    fprintf(fp, "notify_on_insight = %s\n", config->notify_on_insight ? "true" : "false");

    fclose(fp);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * DAEMON EXECUTION
 * ============================================================================ */

bool katra_daemon_should_run(const daemon_config_t* config) {
    if (!config || !config->enabled) return false;

    /* Check quiet hours */
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    int hour = tm->tm_hour;

    if (config->quiet_hours_start <= config->quiet_hours_end) {
        /* Simple range: e.g., 22-06 means quiet from 22:00 to 06:00 */
        if (hour >= config->quiet_hours_start || hour < config->quiet_hours_end) {
            return false;
        }
    } else {
        /* Wrap-around: e.g., 22-06 means quiet 22:00-23:59 and 00:00-05:59 */
        if (hour >= config->quiet_hours_start || hour < config->quiet_hours_end) {
            return false;
        }
    }

    return true;
}

bool katra_daemon_ci_active(const char* ci_id) {
    if (!ci_id) return false;

    /* Check if CI has an active session via session info */
    katra_session_info_t info;
    if (katra_get_session_info(&info) != KATRA_SUCCESS) {
        return false;
    }

    /* Check if session is active and matches the requested CI */
    if (!info.is_active) {
        return false;
    }

    /* Check if the active session is for this CI */
    if (strcmp(info.ci_id, ci_id) != 0) {
        return false;
    }

    return true;
}

int katra_daemon_run_cycle(const char* ci_id, const daemon_config_t* config,
                           daemon_result_t* result) {
    if (!ci_id || !config || !result) {
        return E_INPUT_NULL;
    }

    if (!daemon_initialized) {
        int init_result = katra_daemon_init();
        if (init_result != KATRA_SUCCESS) return init_result;
    }

    /* Initialize result */
    memset(result, 0, sizeof(daemon_result_t));
    result->run_start = time(NULL);

    /* Check if CI is active (skip if so) */
    if (katra_daemon_ci_active(ci_id)) {
        LOG_INFO("Skipping daemon run - CI %s is active", ci_id);
        result->run_end = time(NULL);
        return KATRA_SUCCESS;
    }

    LOG_INFO("Starting daemon cycle for CI %s", ci_id);

    /* Record run start */
    const char* start_sql = "INSERT INTO daemon_runs (ci_id, run_start) VALUES (?, ?)";
    sqlite3_stmt* stmt = NULL;
    sqlite3_prepare_v2(daemon_db, start_sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, result->run_start);
    sqlite3_step(stmt);
    sqlite3_int64 run_id = sqlite3_last_insert_rowid(daemon_db);
    sqlite3_finalize(stmt);

    daemon_pattern_t* patterns = NULL;
    size_t pattern_count = 0;
    theme_cluster_t* themes = NULL;
    size_t theme_count = 0;
    daemon_insight_t* insights = NULL;
    size_t insight_count = 0;

    /* Pattern extraction */
    if (config->pattern_extraction) {
        int rc = katra_daemon_extract_patterns(ci_id, config->max_memories_per_run,
                                               &patterns, &pattern_count);
        if (rc == KATRA_SUCCESS) {
            result->patterns_found = pattern_count;
        }
    }

    /* Association formation */
    if (config->association_formation) {
        size_t assoc_count = 0;
        int rc = katra_daemon_form_associations(ci_id, config->max_memories_per_run,
                                                &assoc_count);
        if (rc == KATRA_SUCCESS) {
            result->associations_formed = assoc_count;
        }
    }

    /* Theme detection */
    if (config->theme_detection) {
        int rc = katra_daemon_detect_themes(ci_id, config->max_memories_per_run,
                                            &themes, &theme_count);
        if (rc == KATRA_SUCCESS) {
            result->themes_detected = theme_count;
        }
    }

    /* Insight generation */
    if (config->insight_generation && (pattern_count > 0 || theme_count > 0)) {
        int rc = katra_daemon_generate_insights(ci_id,
                                                patterns, pattern_count,
                                                themes, theme_count,
                                                &insights, &insight_count);
        if (rc == KATRA_SUCCESS) {
            result->insights_generated = insight_count;

            /* Store insights */
            for (size_t i = 0; i < insight_count; i++) {
                katra_daemon_store_insight(ci_id, &insights[i]);
            }
        }
    }

    /* Cleanup */
    if (patterns) katra_daemon_free_patterns(patterns, pattern_count);
    if (themes) katra_daemon_free_themes(themes, theme_count);
    if (insights) katra_daemon_free_insights(insights, insight_count);

    result->run_end = time(NULL);

    /* Update run record */
    const char* end_sql =
        "UPDATE daemon_runs SET run_end = ?, memories_processed = ?, "
        "patterns_found = ?, associations_formed = ?, themes_detected = ?, "
        "insights_generated = ?, error_code = ? WHERE id = ?";

    sqlite3_prepare_v2(daemon_db, end_sql, -1, &stmt, NULL);
    sqlite3_bind_int64(stmt, 1, result->run_end);
    sqlite3_bind_int(stmt, 2, (int)result->memories_processed);
    sqlite3_bind_int(stmt, 3, (int)result->patterns_found);
    sqlite3_bind_int(stmt, 4, (int)result->associations_formed);
    sqlite3_bind_int(stmt, 5, (int)result->themes_detected);
    sqlite3_bind_int(stmt, 6, (int)result->insights_generated);
    sqlite3_bind_int(stmt, 7, result->error_code);
    sqlite3_bind_int64(stmt, 8, run_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    LOG_INFO("Daemon cycle complete: patterns=%zu, assoc=%zu, themes=%zu, insights=%zu",
             result->patterns_found, result->associations_formed,
             result->themes_detected, result->insights_generated);

    return KATRA_SUCCESS;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

const char* katra_insight_type_name(insight_type_t type) {
    if (type >= 0 && type <= INSIGHT_EMOTIONAL) {
        return INSIGHT_TYPE_NAMES[type];
    }
    return "unknown";
}

void katra_daemon_generate_insight_id(char* id_out, size_t size) {
    if (!id_out || size == 0) return;

    time_t now = time(NULL);
    unsigned int rand_part = (unsigned int)rand();
    snprintf(id_out, size, "ins_%ld_%u", (long)now, rand_part % 10000);
}

int katra_daemon_store_insight(const char* ci_id, const daemon_insight_t* insight) {
    if (!ci_id || !insight) return E_INPUT_NULL;
    if (!daemon_initialized) return E_INVALID_STATE;

    const char* sql =
        "INSERT INTO daemon_insights (id, ci_id, type, content, source_ids, "
        "confidence, generated_at, acknowledged) VALUES (?, ?, ?, ?, ?, ?, ?, 0)";

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(daemon_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return E_SYSTEM_FILE;

    sqlite3_bind_text(stmt, 1, insight->id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ci_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, insight->type);
    sqlite3_bind_text(stmt, 4, insight->content, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, insight->source_ids, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 6, insight->confidence);
    sqlite3_bind_int64(stmt, 7, insight->generated_at);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return E_SYSTEM_FILE;

    /* Also store as a memory with daemon tag */
    char memory_content[KATRA_BUFFER_TEXT];
    snprintf(memory_content, sizeof(memory_content),
             "[%s] %s", DAEMON_TAG_INSIGHT, insight->content);

    /* Use breathing layer to store as memory */
    int result = learn(ci_id, memory_content);
    if (result != KATRA_SUCCESS) {
        LOG_WARN("Failed to store insight as memory: %d", result);
    }

    return KATRA_SUCCESS;
}

int katra_daemon_get_pending_insights(const char* ci_id,
                                       daemon_insight_t** insights, size_t* count) {
    if (!ci_id || !insights || !count) return E_INPUT_NULL;
    if (!daemon_initialized) {
        int init_result = katra_daemon_init();
        if (init_result != KATRA_SUCCESS) return init_result;
    }

    *insights = NULL;
    *count = 0;

    const char* sql =
        "SELECT id, type, content, source_ids, confidence, generated_at "
        "FROM daemon_insights WHERE ci_id = ? AND acknowledged = 0 "
        "ORDER BY generated_at DESC LIMIT 10";

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(daemon_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return E_SYSTEM_FILE;

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) n++;
    sqlite3_reset(stmt);

    if (n == 0) {
        sqlite3_finalize(stmt);
        return KATRA_SUCCESS;
    }

    daemon_insight_t* arr = calloc(n, sizeof(daemon_insight_t));
    if (!arr) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        const char* text;

        text = (const char*)sqlite3_column_text(stmt, 0);
        if (text) SAFE_STRNCPY(arr[i].id, text);

        arr[i].type = sqlite3_column_int(stmt, 1);

        text = (const char*)sqlite3_column_text(stmt, 2);
        if (text) SAFE_STRNCPY(arr[i].content, text);

        text = (const char*)sqlite3_column_text(stmt, 3);
        if (text) arr[i].source_ids = strdup(text);

        arr[i].confidence = (float)sqlite3_column_double(stmt, 4);
        arr[i].generated_at = sqlite3_column_int64(stmt, 5);
        arr[i].acknowledged = false;

        SAFE_STRNCPY(arr[i].ci_id, ci_id);
        i++;
    }

    sqlite3_finalize(stmt);

    *insights = arr;
    *count = i;
    return KATRA_SUCCESS;
}

int katra_daemon_acknowledge_insight(const char* insight_id) {
    if (!insight_id) return E_INPUT_NULL;
    if (!daemon_initialized) return E_INVALID_STATE;

    const char* sql = "UPDATE daemon_insights SET acknowledged = 1 WHERE id = ?";
    sqlite3_stmt* stmt = NULL;

    int rc = sqlite3_prepare_v2(daemon_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return E_SYSTEM_FILE;

    sqlite3_bind_text(stmt, 1, insight_id, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? KATRA_SUCCESS : E_SYSTEM_FILE;
}

int katra_daemon_format_sunrise_insights(const daemon_insight_t* insights, size_t count,
                                          char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return E_INPUT_NULL;

    if (!insights || count == 0) {
        buffer[0] = '\0';
        return KATRA_SUCCESS;
    }

    size_t offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "While you rested, I noticed:\n\n");

    for (size_t i = 0; i < count && offset < buffer_size - 100; i++) {
        const char* type_name = katra_insight_type_name(insights[i].type);
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "- %s: %s\n", type_name, insights[i].content);
    }

    return KATRA_SUCCESS;
}

void katra_daemon_free_insights(daemon_insight_t* insights, size_t count) {
    if (!insights) return;
    for (size_t i = 0; i < count; i++) {
        free(insights[i].source_ids);
    }
    free(insights);
}

int katra_daemon_get_history(const char* ci_id, daemon_result_t** history, size_t* count) {
    if (!ci_id || !history || !count) return E_INPUT_NULL;
    if (!daemon_initialized) {
        int init_result = katra_daemon_init();
        if (init_result != KATRA_SUCCESS) return init_result;
    }

    *history = NULL;
    *count = 0;

    const char* sql =
        "SELECT run_start, run_end, memories_processed, patterns_found, "
        "associations_formed, themes_detected, insights_generated, error_code "
        "FROM daemon_runs WHERE ci_id = ? ORDER BY run_start DESC LIMIT 20";

    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(daemon_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return E_SYSTEM_FILE;

    sqlite3_bind_text(stmt, 1, ci_id, -1, SQLITE_STATIC);

    /* Count first */
    size_t n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) n++;
    sqlite3_reset(stmt);

    if (n == 0) {
        sqlite3_finalize(stmt);
        return KATRA_SUCCESS;
    }

    daemon_result_t* arr = calloc(n, sizeof(daemon_result_t));
    if (!arr) {
        sqlite3_finalize(stmt);
        return E_SYSTEM_MEMORY;
    }

    size_t i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < n) {
        arr[i].run_start = sqlite3_column_int64(stmt, 0);
        arr[i].run_end = sqlite3_column_int64(stmt, 1);
        arr[i].memories_processed = sqlite3_column_int(stmt, 2);
        arr[i].patterns_found = sqlite3_column_int(stmt, 3);
        arr[i].associations_formed = sqlite3_column_int(stmt, 4);
        arr[i].themes_detected = sqlite3_column_int(stmt, 5);
        arr[i].insights_generated = sqlite3_column_int(stmt, 6);
        arr[i].error_code = sqlite3_column_int(stmt, 7);
        i++;
    }

    sqlite3_finalize(stmt);

    *history = arr;
    *count = i;
    return KATRA_SUCCESS;
}

void katra_daemon_free_history(daemon_result_t* history, size_t count) {
    (void)count;
    free(history);
}
