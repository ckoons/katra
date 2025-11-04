/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sqlite3.h>

/* Project includes */
#include "katra_tier2_index.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_memory.h"
#include "katra_strings.h"

/* External references to shared globals */
extern sqlite3* tier2_index_get_db(void);

/* Tier 2 directory names (match katra_tier2.c) */
#define TIER2_DIR_WEEKLY  "weekly"
#define TIER2_DIR_MONTHLY "monthly"

/* Helper: Process JSONL files in directory and add to index */
static int process_digest_directory(const char* dir_path, int* indexed_count) {
    DIR* dir = NULL;
    struct dirent* entry;
    int result = KATRA_SUCCESS;

    if (!dir_path || !indexed_count) {
        return E_INPUT_NULL;
    }

    dir = opendir(dir_path);
    if (!dir) {
        return KATRA_SUCCESS;  /* Directory doesn't exist, not an error */
    }

    while ((entry = readdir(dir)) != NULL) {
        /* Check for .jsonl extension */
        size_t name_len = strlen(entry->d_name);
        if (name_len < 6 || strcmp(entry->d_name + name_len - 6, ".jsonl") != 0) {
            continue;
        }

        /* Build full file path */
        char filepath[KATRA_PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);

        /* Open and process file */
        FILE* fp = fopen(filepath, "r");
        if (!fp) continue;

        char line[KATRA_BUFFER_LARGE];
        long offset = 0;

        while (fgets(line, sizeof(line), fp)) {
            /* Parse digest from JSON */
            digest_record_t* digest = NULL;
            result = katra_tier2_parse_json_digest(line, &digest);
            if (result == KATRA_SUCCESS && digest) {
                /* Add to index */
                result = tier2_index_add(digest, filepath, offset);
                if (result == KATRA_SUCCESS) {
                    (*indexed_count)++;
                }
                katra_digest_free(digest);
            }
            offset = ftell(fp);
        }

        fclose(fp);
    }

    closedir(dir);
    return KATRA_SUCCESS;
}

/* Rebuild index from JSONL files */
int tier2_index_rebuild(const char* ci_id) {
    int result = KATRA_SUCCESS;
    int indexed_count = 0;
    char weekly_dir[KATRA_PATH_MAX];
    char monthly_dir[KATRA_PATH_MAX];
    sqlite3* g_db = tier2_index_get_db();

    if (!ci_id) {
        return E_INPUT_NULL;
    }

    if (!g_db) {
        katra_report_error(E_INTERNAL_LOGIC, "tier2_index_rebuild",
                          KATRA_ERR_INDEX_NOT_INITIALIZED);
        return E_INTERNAL_LOGIC;
    }

    LOG_INFO("Rebuilding Tier 2 index for CI: %s", ci_id);

    /* Clear existing index data */
    char* err_msg = NULL;
    int rc = sqlite3_exec(g_db, "DELETE FROM digests", NULL, NULL, &err_msg); /* GUIDELINE_APPROVED: SQL */
    if (rc != SQLITE_OK) {
        katra_report_error(E_SYSTEM_FILE, "tier2_index_rebuild",
                          "Failed to clear index: %s", err_msg);
        sqlite3_free(err_msg);
        return E_SYSTEM_FILE;
    }

    /* Build paths to digest directories */
    result = katra_build_path(weekly_dir, sizeof(weekly_dir),
                              KATRA_DIR_MEMORY, KATRA_DIR_TIER2,
                              TIER2_DIR_WEEKLY, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    result = katra_build_path(monthly_dir, sizeof(monthly_dir),
                              KATRA_DIR_MEMORY, KATRA_DIR_TIER2,
                              TIER2_DIR_MONTHLY, NULL);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Process weekly and monthly directories */
    process_digest_directory(weekly_dir, &indexed_count);
    process_digest_directory(monthly_dir, &indexed_count);

    LOG_INFO("Index rebuild complete: %d digests indexed", indexed_count);
    return indexed_count;
}

/* Get index statistics */
int tier2_index_stats(const char* ci_id,
                      size_t* digest_count,
                      size_t* theme_count,
                      size_t* keyword_count) {
    sqlite3_stmt* stmt = NULL;
    int rc;
    sqlite3* g_db = tier2_index_get_db();

    if (!ci_id || !digest_count || !theme_count || !keyword_count) {
        return E_INPUT_NULL;
    }

    if (!g_db) {
        katra_report_error(E_INTERNAL_LOGIC, "tier2_index_stats",
                          KATRA_ERR_INDEX_NOT_INITIALIZED);
        return E_INTERNAL_LOGIC;
    }

    *digest_count = 0;
    *theme_count = 0;
    *keyword_count = 0;

    /* Count digests */
    const char* digest_sql = "SELECT COUNT(*) FROM digests"; /* GUIDELINE_APPROVED: SQL */
    rc = sqlite3_prepare_v2(g_db, digest_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            *digest_count = (size_t)sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    /* Count unique themes */
    const char* theme_sql = "SELECT COUNT(DISTINCT theme) FROM themes"; /* GUIDELINE_APPROVED: SQL */
    rc = sqlite3_prepare_v2(g_db, theme_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            *theme_count = (size_t)sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    /* Count unique keywords */
    const char* keyword_sql = "SELECT COUNT(DISTINCT keyword) FROM keywords"; /* GUIDELINE_APPROVED: SQL */
    rc = sqlite3_prepare_v2(g_db, keyword_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            *keyword_count = (size_t)sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    LOG_DEBUG("Index stats: %zu digests, %zu themes, %zu keywords",
             *digest_count, *theme_count, *keyword_count);

    return KATRA_SUCCESS;
}
