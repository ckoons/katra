/* © 2025 Casey Koons All rights reserved */

/*
 * Comprehensive test for Thane's three-phase consolidation recommendations
 *
 * Tests:
 * - Phase 1: Access-based decay, emotional salience, voluntary control
 * - Phase 2: Graph centrality preservation
 * - Phase 3: Pattern detection and compression
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Project includes */
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_graph.h"
#include "katra_error.h"
#include "katra_log.h"

#define TEST_CI_ID "thane_consolidation_test"
#define DAYS_AGO(d) (time(NULL) - ((d) * 24 * 3600))

/* Test result tracking */
typedef struct {
    int passed;
    int failed;
    const char* current_phase;
} test_results_t;

static test_results_t results = {0, 0, NULL};

void test_assert(int condition, const char* test_name) {
    if (condition) {
        printf("  ✓ %s\n", test_name);
        results.passed++;
    } else {
        printf("  ✗ %s\n", test_name);
        results.failed++;
    }
}

void phase_header(const char* name) {
    results.current_phase = name;
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║ %s\n", name);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
}

/* ============================================================================
 * PHASE 1: ACCESS-BASED DECAY, EMOTIONAL SALIENCE, VOLUNTARY CONTROL
 * ============================================================================ */

void test_phase1_access_decay(void) {
    phase_header("PHASE 1A: Access-Based Decay (Memory Warming)");

    printf("Creating old memory (15 days ago)...\n");
    memory_record_t* record = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_EXPERIENCE,
        "Old memory that will be accessed", 0.5);

    if (record) {
        record->timestamp = DAYS_AGO(15);  /* 15 days old */
        record->last_accessed = DAYS_AGO(15);  /* Not accessed since creation */
        record->access_count = 0;
        katra_memory_store(record);
        katra_memory_free_record(record);
    }

    /* Query the memory multiple times to "warm it up" */
    printf("Accessing the memory 3 times to warm it up...\n");
    memory_query_t query = {
        .ci_id = TEST_CI_ID,
        .limit = 10
    };

    for (int i = 0; i < 3; i++) {
        memory_record_t** results_arr = NULL;
        size_t count = 0;
        katra_memory_query(&query, &results_arr, &count);

        if (count > 0 && results_arr) {
            /* Update last_accessed */
            results_arr[0]->last_accessed = time(NULL);
            results_arr[0]->access_count++;
        }
        katra_memory_free_results(results_arr, count);
    }

    /* Run archival */
    printf("Running archival (14 day threshold)...\n");
    size_t archived = 0;
    katra_memory_archive(TEST_CI_ID, 14, &archived);

    /* Check if memory was preserved */
    memory_stats_t stats;
    katra_memory_stats(TEST_CI_ID, &stats);

    test_assert(stats.tier1_records > 0,
               "Recently accessed old memory should be preserved");

    printf("\nResult: Memory was %s despite being 15 days old\n",
           stats.tier1_records > 0 ? "PRESERVED (warm)" : "ARCHIVED (cold)");
}

void test_phase1_emotional_salience(void) {
    phase_header("PHASE 1B: Emotional Salience Preservation");

    /* Create memories with varying emotional intensity */
    printf("Creating memories with varying emotional intensity...\n");

    struct {
        const char* content;
        float emotion_intensity;
        const char* emotion_type;
        int should_preserve;
    } test_cases[] = {
        {"Low emotion routine task", 0.1f, "neutral", 0},
        {"Moderate interest finding", 0.5f, "interest", 0},
        {"High arousal breakthrough!", 0.9f, "surprise", 1},
        {"Intense frustration bug hunt", 0.8f, "frustration", 1},
        {"Calm reflection on progress", 0.3f, "satisfaction", 0}
    };

    for (size_t i = 0; i < 5; i++) {
        memory_record_t* rec = katra_memory_create_record(
            TEST_CI_ID, MEMORY_TYPE_EXPERIENCE,
            test_cases[i].content, 0.5);

        if (rec) {
            rec->timestamp = DAYS_AGO(20);  /* Old enough to archive */
            rec->emotion_intensity = test_cases[i].emotion_intensity;
            rec->emotion_type = strdup(test_cases[i].emotion_type);
            katra_memory_store(rec);
            katra_memory_free_record(rec);
        }
    }

    /* Archive old memories */
    printf("Running archival (15 day threshold)...\n");
    size_t archived = 0;
    katra_memory_archive(TEST_CI_ID, 15, &archived);

    /* Query remaining memories */
    memory_query_t query = {.ci_id = TEST_CI_ID, .limit = 100};
    memory_record_t** results = NULL;
    size_t count = 0;
    katra_memory_query(&query, &results, &count);

    int high_emotion_preserved = 0;

    if (results) {
        for (size_t i = 0; i < count; i++) {
            if (results[i]->emotion_intensity >= 0.7f) {
                high_emotion_preserved++;
            }
        }
        katra_memory_free_results(results, count);
    }

    test_assert(high_emotion_preserved >= 2,
               "High-intensity emotions should be preserved");

    printf("\nResult: %d high-emotion memories preserved (expected: 2+)\n",
           high_emotion_preserved);
}

void test_phase1_voluntary_control(void) {
    phase_header("PHASE 1C: Voluntary Control (Agency)");

    printf("Creating memories with voluntary marking...\n");

    /* Create important memory */
    memory_record_t* important = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_EXPERIENCE,
        "This defines who I am - core identity memory", 0.9);
    if (important) {
        important->timestamp = DAYS_AGO(30);  /* Very old */
        important->marked_important = true;  /* User wants to keep */
        katra_memory_store(important);
        katra_memory_free_record(important);
    }

    /* Create forgettable memory */
    memory_record_t* forgettable = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_EXPERIENCE,
        "Trivial status message - ok to forget", 0.2);
    if (forgettable) {
        forgettable->timestamp = DAYS_AGO(1);  /* Very recent */
        forgettable->marked_forgettable = true;  /* User wants to forget */
        katra_memory_store(forgettable);
        katra_memory_free_record(forgettable);
    }

    /* Create normal memory */
    memory_record_t* normal = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_EXPERIENCE,
        "Normal memory with no marking", 0.5);
    if (normal) {
        normal->timestamp = DAYS_AGO(10);
        katra_memory_store(normal);
        katra_memory_free_record(normal);
    }

    /* Archive */
    printf("Running archival (5 day threshold)...\n");
    size_t archived = 0;
    katra_memory_archive(TEST_CI_ID, 5, &archived);

    /* Query results */
    memory_query_t query = {.ci_id = TEST_CI_ID, .limit = 100};
    memory_record_t** results = NULL;
    size_t count = 0;
    katra_memory_query(&query, &results, &count);

    int found_important = 0, found_forgettable = 0;

    if (results) {
        for (size_t i = 0; i < count; i++) {
            if (strstr(results[i]->content, "defines who I am")) {
                found_important = 1;
            }
            if (strstr(results[i]->content, "Trivial status")) {
                found_forgettable = 1;
            }
        }
        katra_memory_free_results(results, count);
    }

    test_assert(found_important == 1,
               "Marked important memory should NEVER be archived");
    test_assert(found_forgettable == 0,
               "Marked forgettable memory should ALWAYS be archived");

    printf("\nVoluntary control results:\n");
    printf("  Important (30d old): %s\n", found_important ? "PRESERVED" : "ARCHIVED");
    printf("  Forgettable (1d old): %s\n", found_forgettable ? "PRESERVED" : "ARCHIVED");
}

/* ============================================================================
 * PHASE 2: GRAPH CENTRALITY
 * ============================================================================ */

void test_phase2_graph_centrality(void) {
    phase_header("PHASE 2: Graph Centrality Preservation");

    printf("Creating memory graph with high-centrality hub...\n");

    /* Create hub memory (will be referenced by many others) */
    memory_record_t* hub = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_KNOWLEDGE,
        "Core concept that many things reference", 0.5);
    if (hub) {
        hub->timestamp = DAYS_AGO(30);  /* Old */
        hub->connection_count = 10;  /* Highly connected */
        hub->graph_centrality = 0.8f;  /* High PageRank */
        katra_memory_store(hub);
        katra_memory_free_record(hub);
    }

    /* Create peripheral memory (low connections) */
    memory_record_t* peripheral = katra_memory_create_record(
        TEST_CI_ID, MEMORY_TYPE_EXPERIENCE,
        "Isolated observation with few connections", 0.5);
    if (peripheral) {
        peripheral->timestamp = DAYS_AGO(30);  /* Same age as hub */
        peripheral->connection_count = 1;
        peripheral->graph_centrality = 0.1f;  /* Low PageRank */
        katra_memory_store(peripheral);
        katra_memory_free_record(peripheral);
    }

    /* Archive */
    printf("Running archival (20 day threshold)...\n");
    size_t archived = 0;
    katra_memory_archive(TEST_CI_ID, 20, &archived);

    /* Query */
    memory_query_t query = {.ci_id = TEST_CI_ID, .limit = 100};
    memory_record_t** results = NULL;
    size_t count = 0;
    katra_memory_query(&query, &results, &count);

    int found_hub = 0, found_peripheral = 0;

    if (results) {
        for (size_t i = 0; i < count; i++) {
            if (strstr(results[i]->content, "Core concept")) {
                found_hub = 1;
            }
            if (strstr(results[i]->content, "Isolated observation")) {
                found_peripheral = 1;
            }
        }
        katra_memory_free_results(results, count);
    }

    test_assert(found_hub == 1,
               "High-centrality hub should be preserved despite age");
    test_assert(found_peripheral == 0,
               "Low-centrality peripheral memory should be archived");

    printf("\nGraph centrality results:\n");
    printf("  Hub (centrality=0.8): %s\n", found_hub ? "PRESERVED" : "ARCHIVED");
    printf("  Peripheral (centrality=0.1): %s\n", found_peripheral ? "PRESERVED" : "ARCHIVED");
}

/* ============================================================================
 * PHASE 3: PATTERN DETECTION & COMPRESSION
 * ============================================================================ */

void test_phase3_pattern_compression(void) {
    phase_header("PHASE 3: Pattern Detection & Compression");

    printf("Creating pattern: 10 similar debugging memories...\n");

    /* Create pattern members */
    for (int i = 0; i < 10; i++) {
        char content[256];
        snprintf(content, sizeof(content),
                "Debugging null pointer exception in module process_data iteration %d", i);

        memory_record_t* rec = katra_memory_create_record(
            TEST_CI_ID, MEMORY_TYPE_EXPERIENCE, content,
            i == 5 ? 0.9f : 0.5f);  /* Make #5 most important */

        if (rec) {
            rec->timestamp = DAYS_AGO(25);
            katra_memory_store(rec);
            katra_memory_free_record(rec);
        }
    }

    /* Create unrelated memories */
    printf("Creating 3 unrelated memories...\n");
    const char* unrelated[] = {
        "Learned about transformer architecture concepts",
        "Team meeting about quarterly roadmap planning",
        "Refactored authentication middleware code structure"
    };

    for (int i = 0; i < 3; i++) {
        memory_record_t* rec = katra_memory_create_record(
            TEST_CI_ID, MEMORY_TYPE_EXPERIENCE, unrelated[i], 0.5);
        if (rec) {
            rec->timestamp = DAYS_AGO(25);
            katra_memory_store(rec);
            katra_memory_free_record(rec);
        }
    }

    /* Get count before archival */
    memory_stats_t before;
    katra_memory_stats(TEST_CI_ID, &before);
    printf("\nBefore archival: %zu memories\n", before.tier1_records);

    /* Archive with pattern detection */
    printf("Running archival with pattern detection (20 day threshold)...\n");
    size_t archived = 0;
    katra_memory_archive(TEST_CI_ID, 20, &archived);

    /* Get count after */
    memory_stats_t after;
    katra_memory_stats(TEST_CI_ID, &after);
    printf("After archival: %zu memories (archived: %zu)\n",
           after.tier1_records, archived);

    /* Expected: ~7 pattern members archived, 3 outliers kept, 3 unrelated kept */
    size_t expected_remaining = 6;  /* 3 outliers + 3 unrelated */
    size_t expected_archived = 7;   /* 7 pattern members */

    test_assert(after.tier1_records <= expected_remaining + 2,
               "Pattern compression should archive ~7 members");
    test_assert(archived >= expected_archived - 2,
               "Pattern should detect and archive repetitive members");

    printf("\nPattern compression results:\n");
    printf("  Expected: ~6 remaining (3 outliers + 3 unrelated)\n");
    printf("  Actual: %zu remaining\n", after.tier1_records);
    printf("  Compression ratio: %.1f%%\n",
           (1.0f - (float)after.tier1_records / (float)before.tier1_records) * 100.0f);
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  COMPREHENSIVE CONSOLIDATION TEST                            ║\n");
    printf("║  Testing Thane's Three-Phase Recommendations                 ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    /* Initialize with DEBUG logging to see consolidation decisions */
    log_set_level(LOG_DEBUG);

    int result = katra_memory_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        printf("\nERROR: Failed to initialize: %d\n", result);
        return 1;
    }

    /* Run all tests */
    test_phase1_access_decay();
    test_phase1_emotional_salience();
    test_phase1_voluntary_control();
    test_phase2_graph_centrality();
    test_phase3_pattern_compression();

    /* Cleanup */
    katra_memory_cleanup();

    /* Results */
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST RESULTS                                                 ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  Passed: %-4d                                                 ║\n", results.passed);
    printf("║  Failed: %-4d                                                 ║\n", results.failed);
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return results.failed == 0 ? 0 : 1;
}
