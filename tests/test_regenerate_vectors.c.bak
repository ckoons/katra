/* © 2025 Casey Koons All rights reserved */

/* Test suite for vector regeneration functionality */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_vector.h"
#include "katra_error.h"

#define TEST_CI_ID "test_regen_ci"
#define VECTORS_DB_PATH "/.katra/memory/tier2/vectors/vectors.db"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST(name) printf("\n=== TEST: %s ===\n", name)
#define PASS() do { g_tests_passed++; printf("✓ PASS\n"); return; } while(0)
#define FAIL(msg) do { g_tests_failed++; printf("✗ FAIL: %s\n", msg); return; } while(0)

/* Helper: Count vectors in database */
static int count_vectors_in_db(void) {
    char db_path[512];
    snprintf(db_path, sizeof(db_path), "%s%s", getenv("HOME"), VECTORS_DB_PATH);

    sqlite3* db = NULL;
    if (sqlite3_open(db_path, &db) != SQLITE_OK) {
        return -1;
    }

    sqlite3_stmt* stmt = NULL;
    int count = 0;

    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM vectors", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return count;
}

/* Helper: Clear vector database and recreate table */
static void clear_vector_db(void) {
    char db_path[512];
    snprintf(db_path, sizeof(db_path), "%s%s", getenv("HOME"), VECTORS_DB_PATH);

    sqlite3* db = NULL;
    /* Create/open database */
    if (sqlite3_open(db_path, &db) == SQLITE_OK) {
        /* Drop and recreate table */
        sqlite3_exec(db, "DROP TABLE IF EXISTS vectors", NULL, NULL, NULL);
        sqlite3_exec(db,
            "CREATE TABLE vectors ("
            "  record_id TEXT PRIMARY KEY,"
            "  dimensions INTEGER NOT NULL,"
            "  embedding_values BLOB NOT NULL,"
            "  magnitude REAL NOT NULL,"
            "  created_at INTEGER DEFAULT (strftime('%s', 'now')))",
            NULL, NULL, NULL);
        sqlite3_close(db);
    }
}

/* Helper: Clear all test memories */
static void clear_test_memories(const char* ci_id) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf ~/.katra/memory/tier1/%s ~/.katra/memory/tier2/%s", ci_id, ci_id);
    system(cmd);
}

/* Helper: Create test memories using breathing layer */
static void create_test_memories(const char* ci_id __attribute__((unused)), int tier1_count, int tier2_count) {
    /* Create Tier 1 memories via remember() */
    for (int i = 0; i < tier1_count; i++) {
        char content[256];
        snprintf(content, sizeof(content), "Tier 1 test memory %d with semantic content", i);
        remember(content, WHY_SIGNIFICANT);
    }

    /* Create Tier 2 memories via learn() */
    for (int i = 0; i < tier2_count; i++) {
        char content[256];
        snprintf(content, sizeof(content), "Tier 2 test memory %d about dragons and unicorns", i);
        learn(content);
    }
}

/* Test 1: Regenerate with semantic search initially disabled */
static void test_regenerate_with_disabled_semantic_search(void) {
    TEST("test_regenerate_with_disabled_semantic_search");

    clear_test_memories(TEST_CI_ID);
    clear_vector_db();

    /* Start session (semantic search disabled by default) */
    if (session_start(TEST_CI_ID) != KATRA_SUCCESS) {
        FAIL("Session start failed");
    }

    /* Create some test memories */
    create_test_memories(TEST_CI_ID, 3, 2);

    /* Regenerate vectors - should auto-enable semantic search */
    int count = regenerate_vectors();

    session_end();

    if (count < 0) {
        FAIL("regenerate_vectors returned error");
    }

    if (count != 5) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected 5 vectors, got %d", count);
        FAIL(msg);
    }

    /* Verify vectors persisted to database */
    int db_count = count_vectors_in_db();
    if (db_count != 5) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected 5 vectors in DB, got %d", db_count);
        FAIL(msg);
    }

    clear_test_memories(TEST_CI_ID);
    PASS();
}

/* Test 2: Regenerate with semantic search already enabled */
static void test_regenerate_with_enabled_semantic_search(void) {
    TEST("test_regenerate_with_enabled_semantic_search");

    clear_test_memories(TEST_CI_ID);
    clear_vector_db();

    if (session_start(TEST_CI_ID) != KATRA_SUCCESS) {
        FAIL("Session start failed");
    }

    /* Enable semantic search manually via get_context_config */
    context_config_t* cfg = get_context_config();
    cfg->use_semantic_search = true;
    cfg->semantic_threshold = 0.3f;
    cfg->max_semantic_results = 20;
    cfg->embedding_method = 1;  /* TF-IDF */

    create_test_memories(TEST_CI_ID, 4, 3);

    int count = regenerate_vectors();

    session_end();

    if (count != 7) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected 7 vectors, got %d", count);
        FAIL(msg);
    }

    int db_count = count_vectors_in_db();
    if (db_count != 7) {
        FAIL("Vectors not persisted correctly");
    }

    PASS();
}

/* Test 3: Regenerate with no memories */
static void test_regenerate_with_no_memories(void) {
    TEST("test_regenerate_with_no_memories");

    clear_vector_db();

    /* Use a fresh CI with no memories */
    const char* empty_ci = "test_empty_ci";

    if (session_start(empty_ci) != KATRA_SUCCESS) {
        FAIL("Session start failed");
    }

    int count = regenerate_vectors();

    session_end();

    if (count != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected 0 vectors for empty CI, got %d", count);
        FAIL(msg);
    }

    PASS();
}

/* Test 4: Regenerate clears old vectors before creating new ones */
static void test_regenerate_clears_old_vectors(void) {
    TEST("test_regenerate_clears_old_vectors");

    clear_test_memories(TEST_CI_ID);
    clear_vector_db();

    if (session_start(TEST_CI_ID) != KATRA_SUCCESS) {
        FAIL("Session start failed");
    }

    /* First regeneration */
    create_test_memories(TEST_CI_ID, 5, 0);
    int count1 = regenerate_vectors();

    if (count1 != 5) {
        FAIL("First regeneration failed");
    }

    /* Second regeneration with different memories */
    create_test_memories(TEST_CI_ID, 3, 0);
    int count2 = regenerate_vectors();

    session_end();

    /* Should have 3 new vectors, not 5+3 */
    if (count2 != 8) {  /* 5 from first + 3 from second */
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected 8 total vectors, got %d", count2);
        FAIL(msg);
    }

    /* Database should reflect the total count */
    int db_count = count_vectors_in_db();
    if (db_count != 8) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected 8 vectors in DB after regeneration, got %d", db_count);
        FAIL(msg);
    }

    PASS();
}

/* Test 5: Regenerate processes both Tier 1 and Tier 2 */
static void test_regenerate_processes_both_tiers(void) {
    TEST("test_regenerate_processes_both_tiers");

    clear_test_memories(TEST_CI_ID);
    clear_vector_db();

    if (session_start(TEST_CI_ID) != KATRA_SUCCESS) {
        FAIL("Session start failed");
    }

    /* Create memories in both tiers */
    create_test_memories(TEST_CI_ID, 10, 5);

    int count = regenerate_vectors();

    session_end();

    if (count != 15) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected 15 vectors (10 tier1 + 5 tier2), got %d", count);
        FAIL(msg);
    }

    PASS();
}

/* Test 6: Vectors have non-zero magnitude after regeneration */
static void test_vectors_have_nonzero_magnitude(void) {
    TEST("test_vectors_have_nonzero_magnitude");

    clear_vector_db();

    if (session_start(TEST_CI_ID) != KATRA_SUCCESS) {
        FAIL("Session start failed");
    }

    create_test_memories(TEST_CI_ID, 3, 0);
    regenerate_vectors();

    session_end();

    /* Check database for non-zero magnitudes */
    char db_path[512];
    snprintf(db_path, sizeof(db_path), "%s%s", getenv("HOME"), VECTORS_DB_PATH);

    sqlite3* db = NULL;
    if (sqlite3_open(db_path, &db) != SQLITE_OK) {
        FAIL("Could not open vector database");
    }

    sqlite3_stmt* stmt = NULL;
    int zero_count = 0;

    if (sqlite3_prepare_v2(db, "SELECT magnitude FROM vectors", -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            double mag = sqlite3_column_double(stmt, 0);
            if (mag == 0.0) {
                zero_count++;
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (zero_count > 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Found %d vectors with zero magnitude", zero_count);
        FAIL(msg);
    }

    PASS();
}

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Vector Regeneration Test Suite\n");
    printf("========================================\n");

    /* Initialize environment */
    setenv("KATRA_LOG_LEVEL", "WARN", 1);

    /* Run tests */
    test_regenerate_with_disabled_semantic_search();
    test_regenerate_with_enabled_semantic_search();
    test_regenerate_with_no_memories();
    test_regenerate_clears_old_vectors();
    test_regenerate_processes_both_tiers();
    test_vectors_have_nonzero_magnitude();

    /* Results */
    printf("\n");
    printf("========================================\n");
    printf("Test Results\n");
    printf("========================================\n");
    printf("Passed: %d\n", g_tests_passed);
    printf("Failed: %d\n", g_tests_failed);
    printf("Total:  %d\n", g_tests_passed + g_tests_failed);
    printf("========================================\n");
    printf("\n");

    return (g_tests_failed > 0) ? 1 : 0;
}
