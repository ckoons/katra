/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>

/* Project includes */
#include "katra_tier1_index.h"
#include "katra_tier1.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_strings.h"
#include "katra_core_common.h"

/* Update memory metadata in index */
int tier1_index_update_metadata(const char* record_id,
                                float importance,
                                size_t access_count,
                                float centrality) {
    sqlite3_stmt* stmt = NULL;
    int result = KATRA_SUCCESS;

    if (!record_id) {
        return E_INPUT_NULL;
    }

    sqlite3* db = tier1_index_get_db();
    if (!db) {
        return E_INTERNAL_LOGIC;
    }

    const char* update_sql =
        "UPDATE memories SET importance = ?, access_count = ?, " /* GUIDELINE_APPROVED */
        "graph_centrality = ?, last_accessed = ? WHERE record_id = ?"; /* GUIDELINE_APPROVED */

    int rc = sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_double(stmt, 1, importance);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)access_count);
    sqlite3_bind_double(stmt, 3, centrality);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)time(NULL));
    sqlite3_bind_text(stmt, 5, record_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        result = E_SYSTEM_FILE;
    }

    sqlite3_finalize(stmt);
    return result;
}

/* Mark memory as archived */
int tier1_index_mark_archived(const char* record_id) {
    sqlite3_stmt* stmt = NULL;
    int result = KATRA_SUCCESS;

    if (!record_id) {
        return E_INPUT_NULL;
    }

    sqlite3* db = tier1_index_get_db();
    if (!db) {
        return E_INTERNAL_LOGIC;
    }

    const char* update_sql = "UPDATE memories SET archived = 1 WHERE record_id = ?"; /* GUIDELINE_APPROVED */

    int rc = sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return E_SYSTEM_FILE;
    }

    sqlite3_bind_text(stmt, 1, record_id, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        result = E_SYSTEM_FILE;
    }

    sqlite3_finalize(stmt);
    return result;
}

/* Load specific memories by locations */
int tier1_load_by_locations(const memory_location_t* locations,
                            size_t count,
                            memory_record_t*** results,
                            size_t* result_count) {
    int result = KATRA_SUCCESS;
    memory_record_t** memories = NULL;
    size_t loaded = 0;

    if (!locations || !results || !result_count) {
        return E_INPUT_NULL;
    }

    *results = NULL;
    *result_count = 0;

    if (count == 0) {
        return KATRA_SUCCESS;
    }

    /* Allocate results array */
    memories = calloc(count, sizeof(memory_record_t*));
    if (!memories) {
        return E_SYSTEM_MEMORY;
    }

    /* Load each memory from its file location */
    for (size_t i = 0; i < count; i++) {
        FILE* fp = fopen(locations[i].file_path, "r"); /* GUIDELINE_APPROVED: file mode */
        if (!fp) {
            continue;  /* Skip missing files */
        }

        /* Seek to offset */
        if (fseek(fp, locations[i].offset, SEEK_SET) != 0) {
            fclose(fp);
            continue;
        }

        /* Read line */
        char line[KATRA_BUFFER_LARGE];
        if (!fgets(line, sizeof(line), fp)) {
            fclose(fp);
            continue;
        }

        fclose(fp);

        /* Parse memory from JSON */
        memory_record_t* memory = NULL;
        result = katra_tier1_parse_json_record(line, &memory);
        if (result == KATRA_SUCCESS && memory) {
            memories[loaded++] = memory;
        }
    }

    *results = memories;
    *result_count = loaded;

    LOG_DEBUG("Loaded %zu memories from %zu locations", loaded, count);
    return KATRA_SUCCESS;
}

/* Rebuild index from JSONL files */
int tier1_index_rebuild(const char* ci_id) {
    int total_indexed = 0;
    int result = KATRA_SUCCESS;
    char tier1_dir[KATRA_PATH_MAX];
    char** filenames = NULL;
    size_t file_count = 0;

    if (!ci_id) {
        return E_INPUT_NULL;
    }

    sqlite3* db = tier1_index_get_db();
    if (!db) {
        katra_report_error(E_INTERNAL_LOGIC, "tier1_index_rebuild",
                          KATRA_ERR_INDEX_NOT_INITIALIZED);
        return E_INTERNAL_LOGIC;
    }

    /* Get tier1 directory */
    result = tier1_get_dir(ci_id, tier1_dir, sizeof(tier1_dir));
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Collect all JSONL files */
    result = tier1_collect_jsonl_files(tier1_dir, &filenames, &file_count);
    if (result != KATRA_SUCCESS || file_count == 0) {
        LOG_INFO("No JSONL files found for %s", ci_id);
        return 0;
    }

    LOG_INFO("Rebuilding index from %zu JSONL files for %s", file_count, ci_id);

    /* Clear existing index entries for this CI */
    const char* delete_sql = "DELETE FROM memories WHERE ci_id = ?";
    sqlite3_stmt* delete_stmt = NULL;
    if (sqlite3_prepare_v2(db, delete_sql, -1, &delete_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(delete_stmt, 1, ci_id, -1, SQLITE_STATIC);
        sqlite3_step(delete_stmt);
        sqlite3_finalize(delete_stmt);
    }

    /* Process each JSONL file */
    for (size_t i = 0; i < file_count; i++) {
        char filepath[KATRA_PATH_MAX];
        snprintf(filepath, sizeof(filepath), "%s/%s", tier1_dir, filenames[i]);

        FILE* fp = fopen(filepath, KATRA_FILE_MODE_READ);
        if (!fp) {
            LOG_WARN("Failed to open %s", filepath);
            continue;
        }

        /* Read each line (memory record) */
        long offset = 0;
        char line[KATRA_BUFFER_LARGE];

        while (fgets(line, sizeof(line), fp)) {
            long next_offset = ftell(fp);

            /* Parse JSON record */
            memory_record_t* record = NULL;
            result = katra_tier1_parse_json_record(line, &record);

            if (result == KATRA_SUCCESS && record) {
                /* Add to index */
                result = tier1_index_add(record, filepath, offset);
                if (result == KATRA_SUCCESS) {
                    total_indexed++;
                }
                katra_memory_free_record(record);
            }

            offset = next_offset;
        }

        fclose(fp);
        LOG_DEBUG("Indexed %s", filenames[i]);
    }

    /* Free filename array */
    for (size_t i = 0; i < file_count; i++) {
        free(filenames[i]);
    }
    free(filenames);

    LOG_INFO("Index rebuild complete: %d memories indexed for %s", total_indexed, ci_id);
    return total_indexed;
}

/* Get index statistics */
int tier1_index_stats(const char* ci_id,
                      size_t* memory_count,
                      size_t* theme_count,
                      size_t* connection_count) {
    sqlite3_stmt* stmt = NULL;

    if (!ci_id || !memory_count || !theme_count || !connection_count) {
        return E_INPUT_NULL;
    }

    sqlite3* db = tier1_index_get_db();
    if (!db) {
        return E_INTERNAL_LOGIC;
    }

    /* Count memories */
    const char* count_sql = "SELECT COUNT(*) FROM memories"; /* GUIDELINE_APPROVED */
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            *memory_count = (size_t)sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    /* Count themes */
    count_sql = "SELECT COUNT(DISTINCT theme) FROM memory_themes"; /* GUIDELINE_APPROVED */
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            *theme_count = (size_t)sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    /* Count connections */
    count_sql = "SELECT COUNT(*) FROM memory_connections"; /* GUIDELINE_APPROVED */
    if (sqlite3_prepare_v2(db, count_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            *connection_count = (size_t)sqlite3_column_int64(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return KATRA_SUCCESS;
}
