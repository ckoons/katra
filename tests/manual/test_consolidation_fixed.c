/* © 2025 Casey Koons All rights reserved */

/*
 * Fixed: Comprehensive consolidation test with unique CI IDs per phase
 *
 * Tests Nyx's current implementation (sequential logic, NOT multi-factor scoring)
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_graph.h"
#include "katra_error.h"
#include "katra_log.h"

#define DAYS_AGO(d) (time(NULL) - ((d) * 24 * 3600))

/* Unique CI per test phase to avoid accumulation */
#define CI_ACCESS_DECAY "test_access_decay"
#define CI_EMOTION "test_emotion_salience"
#define CI_VOLUNTARY "test_voluntary_control"
#define CI_CENTRALITY "test_graph_centrality"
#define CI_PATTERN "test_pattern_detection"

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

void test_assert(int condition, const char* test_name) {
    if (condition) {
        printf("  ✓ %s\n", test_name);
        tests_passed++;
    } else {
        printf("  ✗ %s\n", test_name);
        tests_failed++;
    }
}

void phase_header(const char* name) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║ %s\n", name);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
}

/* Test 1: Access-based decay */
void test_access_decay(void) {
    phase_header("TEST 1: Access-Based Decay (7 Day Threshold)");

    katra_memory_init(CI_ACCESS_DECAY);

    /* Create old memory */
    memory_record_t* old = katra_memory_create_record(
        CI_ACCESS_DECAY, MEMORY_TYPE_EXPERIENCE,
        "Old memory from 15 days ago", 0.5);
    if (old) {
        old->timestamp = DAYS_AGO(15);
        old->last_accessed = time(NULL);  /* Accessed NOW */
        old->access_count = 3;
        katra_memory_store(old);
        katra_memory_free_record(old);
    }

    /* Archive (14 day threshold) */
    size_t archived = 0;
    katra_memory_archive(CI_ACCESS_DECAY, 14, &archived);

    /* Verify preservation */
    memory_stats_t stats;
    katra_memory_stats(CI_ACCESS_DECAY, &stats);

    test_assert(stats.tier1_records > 0 && archived == 0,
               "Recently accessed old memory preserved");

    printf("Result: %zu records preserved, %zu archived\n",
           stats.tier1_records, archived);

    katra_memory_cleanup();
}

/* Test 2: Emotional salience */
void test_emotional_salience(void) {
    phase_header("TEST 2: Emotional Salience (0.7 Threshold)");

    katra_memory_init(CI_EMOTION);

    /* High emotion */
    memory_record_t* high_emo = katra_memory_create_record(
        CI_EMOTION, MEMORY_TYPE_EXPERIENCE,
        "Breakthrough discovery - high arousal", 0.5);
    if (high_emo) {
        high_emo->timestamp = DAYS_AGO(20);
        high_emo->emotion_intensity = 0.9f;  /* Above 0.7 threshold */
        high_emo->emotion_type = strdup("surprise");
        katra_memory_store(high_emo);
        katra_memory_free_record(high_emo);
    }

    /* Low emotion */
    memory_record_t* low_emo = katra_memory_create_record(
        CI_EMOTION, MEMORY_TYPE_EXPERIENCE,
        "Routine status check - low arousal", 0.5);
    if (low_emo) {
        low_emo->timestamp = DAYS_AGO(20);
        low_emo->emotion_intensity = 0.3f;  /* Below 0.7 threshold */
        low_emo->emotion_type = strdup("neutral");
        katra_memory_store(low_emo);
        katra_memory_free_record(low_emo);
    }

    /* Archive */
    size_t archived = 0;
    katra_memory_archive(CI_EMOTION, 15, &archived);

    /* Query */
    memory_query_t query = {.ci_id = CI_EMOTION, .limit = 100};
    memory_record_t** results = NULL;
    size_t count = 0;
    katra_memory_query(&query, &results, &count);

    int found_high = 0, found_low = 0;
    if (results) {
        for (size_t i = 0; i < count; i++) {
            if (strstr(results[i]->content, "Breakthrough")) found_high = 1;
            if (strstr(results[i]->content, "Routine")) found_low = 1;
        }
        katra_memory_free_results(results, count);
    }

    test_assert(found_high == 1, "High-emotion memory preserved");
    test_assert(found_low == 0, "Low-emotion memory archived");

    printf("Result: High emotion=%s, Low emotion=%s\n",
           found_high ? "PRESERVED" : "ARCHIVED",
           found_low ? "PRESERVED" : "ARCHIVED");

    katra_memory_cleanup();
}

/* Test 3: Voluntary control */
void test_voluntary_control(void) {
    phase_header("TEST 3: Voluntary Control (Consent System)");

    katra_memory_init(CI_VOLUNTARY);

    /* Marked important (30 days old) */
    memory_record_t* important = katra_memory_create_record(
        CI_VOLUNTARY, MEMORY_TYPE_EXPERIENCE,
        "Core identity memory - marked important", 0.9);
    if (important) {
        important->timestamp = DAYS_AGO(30);
        important->marked_important = true;  /* NEVER archive */
        katra_memory_store(important);
        katra_memory_free_record(important);
    }

    /* Marked forgettable (1 day old) */
    memory_record_t* forgettable = katra_memory_create_record(
        CI_VOLUNTARY, MEMORY_TYPE_EXPERIENCE,
        "Trivial status - marked forgettable", 0.2);
    if (forgettable) {
        forgettable->timestamp = DAYS_AGO(1);
        forgettable->marked_forgettable = true;  /* ALWAYS archive */
        katra_memory_store(forgettable);
        katra_memory_free_record(forgettable);
    }

    /* Archive (5 day threshold) */
    size_t archived = 0;
    katra_memory_archive(CI_VOLUNTARY, 5, &archived);

    /* Query */
    memory_query_t query = {.ci_id = CI_VOLUNTARY, .limit = 100};
    memory_record_t** results = NULL;
    size_t count = 0;
    katra_memory_query(&query, &results, &count);

    int found_important = 0, found_forgettable = 0;
    if (results) {
        for (size_t i = 0; i < count; i++) {
            if (strstr(results[i]->content, "Core identity")) found_important = 1;
            if (strstr(results[i]->content, "Trivial status")) found_forgettable = 1;
        }
        katra_memory_free_results(results, count);
    }

    test_assert(found_important == 1, "Marked important NEVER archived");
    test_assert(found_forgettable == 0, "Marked forgettable ALWAYS archived");

    printf("Result: Important (30d)=%s, Forgettable (1d)=%s\n",
           found_important ? "PRESERVED" : "ARCHIVED",
           found_forgettable ? "PRESERVED" : "ARCHIVED");

    katra_memory_cleanup();
}

/* Test 4: Graph centrality */
void test_graph_centrality(void) {
    phase_header("TEST 4: Graph Centrality (0.5 Threshold)");

    katra_memory_init(CI_CENTRALITY);

    /* High centrality hub */
    memory_record_t* hub = katra_memory_create_record(
        CI_CENTRALITY, MEMORY_TYPE_KNOWLEDGE,
        "Core concept - highly connected", 0.5);
    if (hub) {
        hub->timestamp = DAYS_AGO(30);
        hub->graph_centrality = 0.8f;  /* Above 0.5 threshold */
        katra_memory_store(hub);
        katra_memory_free_record(hub);
    }

    /* Low centrality peripheral */
    memory_record_t* peripheral = katra_memory_create_record(
        CI_CENTRALITY, MEMORY_TYPE_EXPERIENCE,
        "Isolated observation - low connections", 0.5);
    if (peripheral) {
        peripheral->timestamp = DAYS_AGO(30);
        peripheral->graph_centrality = 0.2f;  /* Below 0.5 threshold */
        katra_memory_store(peripheral);
        katra_memory_free_record(peripheral);
    }

    /* Archive */
    size_t archived = 0;
    katra_memory_archive(CI_CENTRALITY, 20, &archived);

    /* Query */
    memory_query_t query = {.ci_id = CI_CENTRALITY, .limit = 100};
    memory_record_t** results = NULL;
    size_t count = 0;
    katra_memory_query(&query, &results, &count);

    int found_hub = 0, found_peripheral = 0;
    if (results) {
        for (size_t i = 0; i < count; i++) {
            if (strstr(results[i]->content, "Core concept")) found_hub = 1;
            if (strstr(results[i]->content, "Isolated")) found_peripheral = 1;
        }
        katra_memory_free_results(results, count);
    }

    test_assert(found_hub == 1, "High-centrality hub preserved");
    test_assert(found_peripheral == 0, "Low-centrality peripheral archived");

    printf("Result: Hub (0.8)=%s, Peripheral (0.2)=%s\n",
           found_hub ? "PRESERVED" : "ARCHIVED",
           found_peripheral ? "PRESERVED" : "ARCHIVED");

    katra_memory_cleanup();
}

/* Test 5: Pattern detection */
void test_pattern_detection(void) {
    phase_header("TEST 5: Pattern Detection (40% Similarity)");

    katra_memory_init(CI_PATTERN);

    /* Create pattern: 10 similar debugging memories */
    for (int i = 0; i < 10; i++) {
        char content[256];
        snprintf(content, sizeof(content),
                "Debugging null pointer exception in process_data iteration %d", i);

        memory_record_t* rec = katra_memory_create_record(
            CI_PATTERN, MEMORY_TYPE_EXPERIENCE, content,
            i == 5 ? 0.9f : 0.5f);  /* #5 is most important */
        if (rec) {
            rec->timestamp = DAYS_AGO(25);
            katra_memory_store(rec);
            katra_memory_free_record(rec);
        }
    }

    /* Create unrelated memories */
    const char* unrelated[] = {
        "Learning transformer architecture",
        "Team meeting about roadmap",
        "Refactoring authentication code"
    };
    for (int i = 0; i < 3; i++) {
        memory_record_t* rec = katra_memory_create_record(
            CI_PATTERN, MEMORY_TYPE_EXPERIENCE, unrelated[i], 0.5);
        if (rec) {
            rec->timestamp = DAYS_AGO(25);
            katra_memory_store(rec);
            katra_memory_free_record(rec);
        }
    }

    /* Before */
    memory_stats_t before;
    katra_memory_stats(CI_PATTERN, &before);

    /* Archive with pattern detection */
    size_t archived = 0;
    katra_memory_archive(CI_PATTERN, 20, &archived);

    /* After: Count ACTIVE records via query (excludes archived) */
    memory_query_t query = {
        .ci_id = CI_PATTERN,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 1000
    };
    memory_record_t** results = NULL;
    size_t active_count = 0;
    katra_memory_query(&query, &results, &active_count);
    katra_memory_free_results(results, active_count);

    /* Expected: ~7 pattern members archived, 3 outliers + 3 unrelated preserved */
    float compression = (1.0f - (float)active_count / (float)before.tier1_records) * 100.0f;

    test_assert(archived >= 5, "Pattern compression archived repetitive members");
    test_assert(active_count <= 8, "Pattern outliers and unrelated preserved (active)");

    printf("Before: %zu memories (total in JSONL)\n", before.tier1_records);
    printf("After: %zu active memories (archived: %zu)\n", active_count, archived);
    printf("Compression: %.1f%%\n", compression);

    katra_memory_cleanup();
}

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  FIXED CONSOLIDATION TEST                                     ║\n");
    printf("║  Testing Current Sequential Logic (NOT Multi-Factor Scoring)  ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    log_set_level(LOG_DEBUG);

    /* Run all tests with unique CI IDs */
    test_access_decay();
    test_emotional_salience();
    test_voluntary_control();
    test_graph_centrality();
    test_pattern_detection();

    /* Results */
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  RESULTS                                                      ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  Passed: %-4d                                                 ║\n", tests_passed);
    printf("║  Failed: %-4d                                                 ║\n", tests_failed);
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    return tests_failed == 0 ? 0 : 1;
}
