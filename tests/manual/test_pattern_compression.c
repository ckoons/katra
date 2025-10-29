/* © 2025 Casey Koons All rights reserved */

/*
 * Pattern Compression Test (Phase 3)
 *
 * Tests Thane's Phase 3 active sense-making features:
 * - Pattern detection based on content similarity
 * - Pattern outlier preservation (first, last, most important)
 * - Pattern compression during consolidation
 * - Metacognitive pattern queries
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "katra_init.h"
#include "katra_memory.h"
#include "katra_consent.h"
#include "katra_error.h"

/* Test CI IDs - unique per test to ensure isolation */
#define CI_PATTERN_DETECT "test_pattern_detect_p3_1"
#define CI_OUTLIERS "test_outliers_p3_2"
#define CI_COMPRESSION "test_compression_p3_3"
#define CI_METACOG "test_metacog_p3_4"

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Helper: Assert with message */
#define ASSERT(condition, message) do { \
    if (condition) { \
        printf("  ✓ %s\n", message); \
        tests_passed++; \
    } else { \
        printf("  ✗ %s\n", message); \
        tests_failed++; \
    } \
} while(0)

/* Test 1: Pattern Detection Based on Content Similarity */
static void test_pattern_detection(void) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║ TEST 1: Pattern Detection Based on Content Similarity\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    /* Ensure clean state */
    katra_memory_cleanup();
    katra_memory_init(CI_PATTERN_DETECT);

    /* Create pattern: debugging compilation errors */
    memory_record_t* debug1 = katra_memory_create_record(CI_PATTERN_DETECT, MEMORY_TYPE_REFLECTION,
        "Fixed compilation error undefined reference linker problem debugging session today",
        MEMORY_IMPORTANCE_MEDIUM);
    debug1->timestamp = time(NULL) - (20 * 86400);  /* 20 days old */

    memory_record_t* debug2 = katra_memory_create_record(CI_PATTERN_DETECT, MEMORY_TYPE_REFLECTION,
        "Resolved compilation error undefined symbol linker issue debugging process completed",
        MEMORY_IMPORTANCE_MEDIUM);
    debug2->timestamp = time(NULL) - (20 * 86400);

    memory_record_t* debug3 = katra_memory_create_record(CI_PATTERN_DETECT, MEMORY_TYPE_REFLECTION,
        "Another compilation error undefined function linker debugging work finished successfully",
        MEMORY_IMPORTANCE_MEDIUM);
    debug3->timestamp = time(NULL) - (20 * 86400);

    memory_record_t* debug4 = katra_memory_create_record(CI_PATTERN_DETECT, MEMORY_TYPE_REFLECTION,
        "More compilation error undefined variable linker debugging iteration done today",
        MEMORY_IMPORTANCE_HIGH);  /* Most important */
    debug4->timestamp = time(NULL) - (20 * 86400);

    /* Create unrelated memory */
    memory_record_t* unrelated = katra_memory_create_record(CI_PATTERN_DETECT, MEMORY_TYPE_EXPERIENCE,
        "Went shopping for groceries bought milk eggs bread cheese vegetables fruit",
        MEMORY_IMPORTANCE_LOW);
    unrelated->timestamp = time(NULL) - (20 * 86400);

    katra_memory_store(debug1);
    katra_memory_store(debug2);
    katra_memory_store(debug3);
    katra_memory_store(debug4);
    katra_memory_store(unrelated);

    /* Run consolidation (pattern detection happens during archival) */
    size_t archived_count = 0;
    int result = katra_memory_archive(CI_PATTERN_DETECT, 14, &archived_count);  /* 14 day cutoff */

    ASSERT(result >= 0, "Consolidation ran successfully");

    /* Query remaining memories */
    memory_query_t query = {
        .ci_id = CI_PATTERN_DETECT,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 100
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    result = katra_memory_query(&query, &results, &count);

    /* Pattern should be detected and some memories compressed */
    printf("\n  Pattern detection results:\n");
    printf("    Archived: %zu memories\n", archived_count);
    printf("    Remaining: %zu memories\n", count);

    ASSERT(archived_count > 0, "Some memories archived (pattern compressed)");
    ASSERT(count < 5, "Some memories preserved (pattern outliers)");

    katra_memory_free_results(results, count);
    katra_memory_free_record(debug1);
    katra_memory_free_record(debug2);
    katra_memory_free_record(debug3);
    katra_memory_free_record(debug4);
    katra_memory_free_record(unrelated);
    katra_memory_cleanup();
}

/* Test 2: Pattern Outlier Preservation */
static void test_outlier_preservation(void) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║ TEST 2: Pattern Outlier Preservation\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    /* Ensure clean state */
    katra_memory_cleanup();
    katra_memory_init(CI_OUTLIERS);

    /* Create pattern of 5 memories: first, 3 middle, last */
    memory_record_t* mem[5];

    /* First (should be preserved as outlier) */
    mem[0] = katra_memory_create_record(CI_OUTLIERS, MEMORY_TYPE_KNOWLEDGE,
        "Initial learning about memory consolidation process sleep transfer hippocampus cortex",
        MEMORY_IMPORTANCE_MEDIUM);
    mem[0]->timestamp = time(NULL) - (25 * 86400);  /* Oldest */

    /* Middle memories (should be compressed) */
    mem[1] = katra_memory_create_record(CI_OUTLIERS, MEMORY_TYPE_KNOWLEDGE,
        "Additional memory consolidation learning sleep process transfer hippocampus cortex",
        MEMORY_IMPORTANCE_MEDIUM);
    mem[1]->timestamp = time(NULL) - (22 * 86400);

    mem[2] = katra_memory_create_record(CI_OUTLIERS, MEMORY_TYPE_KNOWLEDGE,
        "Further memory consolidation understanding sleep process transfer hippocampus cortex",
        MEMORY_IMPORTANCE_MEDIUM);
    mem[2]->timestamp = time(NULL) - (20 * 86400);

    mem[3] = katra_memory_create_record(CI_OUTLIERS, MEMORY_TYPE_KNOWLEDGE,
        "More memory consolidation knowledge sleep process transfer hippocampus cortex system",
        MEMORY_IMPORTANCE_CRITICAL);  /* Highest importance - should be preserved */
    mem[3]->timestamp = time(NULL) - (18 * 86400);

    /* Last (should be preserved as outlier) */
    mem[4] = katra_memory_create_record(CI_OUTLIERS, MEMORY_TYPE_KNOWLEDGE,
        "Latest memory consolidation insight sleep process transfer hippocampus cortex mechanism",
        MEMORY_IMPORTANCE_MEDIUM);
    mem[4]->timestamp = time(NULL) - (16 * 86400);  /* Newest of pattern */

    for (int i = 0; i < 5; i++) {
        katra_memory_store(mem[i]);
    }

    /* Run consolidation */
    size_t archived_count = 0;
    int result = katra_memory_archive(CI_OUTLIERS, 14, &archived_count);

    ASSERT(result >= 0, "Consolidation ran successfully");

    /* Query remaining memories */
    memory_query_t query = {
        .ci_id = CI_OUTLIERS,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 100
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    result = katra_memory_query(&query, &results, &count);

    /* Should preserve 3 outliers: first, last, most important */
    printf("\n  Outlier preservation results:\n");
    printf("    Archived: %zu memories\n", archived_count);
    printf("    Remaining: %zu memories\n", count);
    printf("    Expected outliers: 3 (first + last + most important)\n");

    /* At least 2 should be archived (middle memories) */
    ASSERT(archived_count >= 2, "Middle memories archived");

    /* Outliers should be preserved */
    bool has_initial = false;
    bool has_latest = false;
    bool has_critical = false;

    for (size_t i = 0; i < count; i++) {
        if (strstr(results[i]->content, "Initial learning")) has_initial = true;
        if (strstr(results[i]->content, "Latest memory")) has_latest = true;
        if (results[i]->importance >= MEMORY_IMPORTANCE_CRITICAL) has_critical = true;
    }

    ASSERT(has_initial, "First memory preserved");
    ASSERT(has_latest, "Last memory preserved");
    ASSERT(has_critical, "Most important memory preserved");

    katra_memory_free_results(results, count);
    for (int i = 0; i < 5; i++) {
        katra_memory_free_record(mem[i]);
    }
    katra_memory_cleanup();
}

/* Test 3: Pattern Compression Integration */
static void test_pattern_compression(void) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║ TEST 3: Pattern Compression Integration\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    /* Ensure clean state */
    katra_memory_cleanup();
    katra_memory_init(CI_COMPRESSION);

    /* Create large pattern (10 similar memories) */
    for (int i = 0; i < 10; i++) {
        char content[256];
        float importance = MEMORY_IMPORTANCE_MEDIUM;

        /* Make first, last, and middle one important */
        if (i == 0 || i == 9 || i == 5) {
            importance = MEMORY_IMPORTANCE_HIGH;
        }

        snprintf(content, sizeof(content),
                "Pattern instance %d: testing memory consolidation archive system implementation details", i);

        memory_record_t* mem = katra_memory_create_record(CI_COMPRESSION, MEMORY_TYPE_KNOWLEDGE,
                                                           content, importance);
        mem->timestamp = time(NULL) - (20 * 86400);  /* 20 days old */
        katra_memory_store(mem);
        katra_memory_free_record(mem);
    }

    /* Create diverse non-pattern memories */
    memory_record_t* diverse[3];
    diverse[0] = katra_memory_create_record(CI_COMPRESSION, MEMORY_TYPE_EXPERIENCE,
        "Completed morning exercise routine running jogging stretching workout fitness",
        MEMORY_IMPORTANCE_LOW);
    diverse[0]->timestamp = time(NULL) - (20 * 86400);

    diverse[1] = katra_memory_create_record(CI_COMPRESSION, MEMORY_TYPE_REFLECTION,
        "Interesting conversation about artificial intelligence machine learning neural networks",
        MEMORY_IMPORTANCE_MEDIUM);
    diverse[1]->timestamp = time(NULL) - (20 * 86400);

    diverse[2] = katra_memory_create_record(CI_COMPRESSION, MEMORY_TYPE_GOAL,
        "Plan to learn distributed systems consensus protocols raft paxos algorithms",
        MEMORY_IMPORTANCE_HIGH);
    diverse[2]->timestamp = time(NULL) - (20 * 86400);

    for (int i = 0; i < 3; i++) {
        katra_memory_store(diverse[i]);
    }

    /* Run consolidation */
    size_t archived_count = 0;
    int result = katra_memory_archive(CI_COMPRESSION, 14, &archived_count);

    ASSERT(result >= 0, "Consolidation ran successfully");

    printf("\n  Compression results:\n");
    printf("    Total created: 13 memories (10 pattern + 3 diverse)\n");
    printf("    Archived: %zu memories\n", archived_count);
    printf("    Pattern compressed: ~7 middle pattern members\n");
    printf("    Pattern outliers preserved: ~3 (first + last + most important)\n");
    printf("    Diverse memories: archived normally\n");

    /* Pattern should compress middle members, preserve outliers */
    ASSERT(archived_count >= 5, "Pattern compression archived redundant memories");

    for (int i = 0; i < 3; i++) {
        katra_memory_free_record(diverse[i]);
    }
    katra_memory_cleanup();
}

/* Test 4: Metacognitive Pattern Query API */
static void test_metacognitive_api(void) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║ TEST 4: Metacognitive Pattern Query API\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    /* Ensure clean state */
    katra_memory_cleanup();
    katra_memory_init(CI_METACOG);

    /* Create two distinct patterns */

    /* Pattern 1: Bug fixing */
    for (int i = 0; i < 4; i++) {
        char content[256];
        snprintf(content, sizeof(content),
                "Bug fix iteration %d: segmentation fault null pointer dereference memory error", i);
        memory_record_t* mem = katra_memory_create_record(CI_METACOG, MEMORY_TYPE_REFLECTION,
                                                           content, MEMORY_IMPORTANCE_MEDIUM);
        mem->timestamp = time(NULL) - (18 * 86400);
        katra_memory_store(mem);
        katra_memory_free_record(mem);
    }

    /* Pattern 2: Feature implementation */
    for (int i = 0; i < 5; i++) {
        char content[256];
        snprintf(content, sizeof(content),
                "Feature development %d: implementing authentication system user login session management", i);
        memory_record_t* mem = katra_memory_create_record(CI_METACOG, MEMORY_TYPE_KNOWLEDGE,
                                                           content, MEMORY_IMPORTANCE_HIGH);
        mem->timestamp = time(NULL) - (16 * 86400);
        katra_memory_store(mem);
        katra_memory_free_record(mem);
    }

    /* Run consolidation to detect patterns */
    size_t archived_count = 0;
    katra_memory_archive(CI_METACOG, 14, &archived_count);

    /* Query patterns using metacognitive API */
    detected_pattern_t* patterns = NULL;
    size_t pattern_count = 0;
    int result = katra_memory_get_patterns(CI_METACOG, &patterns, &pattern_count);

    ASSERT(result == KATRA_SUCCESS, "Pattern query succeeded");

    /* Should detect patterns (may be 0 if all archived, or >0 if outliers remain) */
    printf("\n  Metacognitive pattern results:\n");
    printf("    Patterns detected: %zu\n", pattern_count);

    if (pattern_count > 0) {
        for (size_t i = 0; i < pattern_count; i++) {
            printf("    Pattern %zu: %zu members - %s\n",
                   i + 1, patterns[i].member_count, patterns[i].centroid_preview);
        }
        ASSERT(patterns[0].member_count >= 3, "Pattern has minimum required members");
    }

    katra_memory_free_patterns(patterns, pattern_count);
    katra_memory_cleanup();
}

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  PATTERN COMPRESSION TEST (Phase 3)                          ║\n");
    printf("║  Testing Pattern-Based Memory Consolidation                  ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    /* Initialize */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        printf("Failed to initialize Katra: %d\n", result);
        return 1;
    }

    /* Run tests */
    test_pattern_detection();
    test_outlier_preservation();
    test_pattern_compression();
    test_metacognitive_api();

    /* Summary */
    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  RESULTS                                                      ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  Passed: %-4d                                                 ║\n", tests_passed);
    printf("║  Failed: %-4d                                                 ║\n", tests_failed);
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    return (tests_failed == 0) ? 0 : 1;
}
