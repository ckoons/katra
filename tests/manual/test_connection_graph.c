/* © 2025 Casey Koons All rights reserved */

/*
 * Connection Graph Test (Phase 2)
 *
 * Tests Thane's Phase 2 active sense-making features:
 * - Connection building between similar memories
 * - Graph centrality calculation
 * - Centrality-based preservation during consolidation
 * - Connection hub metacognitive queries
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

/* Test CI IDs */
#define CI_CONNECTIONS "test_connection_graph"
#define CI_CENTRALITY "test_centrality_calc"
#define CI_CONSOLIDATION "test_central_preserve"
#define CI_HUBS "test_hub_detection"

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

/* Test 1: Connection Building Between Similar Memories */
static void test_connection_building(void) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║ TEST 1: Connection Building Between Similar Memories\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    katra_memory_init(CI_CONNECTIONS);

    /* Store memories with shared keywords */
    memory_record_t* mem1 = katra_memory_create_record(CI_CONNECTIONS, MEMORY_TYPE_KNOWLEDGE,
        "Learn about memory consolidation and how the brain archives memories during sleep",
        MEMORY_IMPORTANCE_HIGH);
    memory_record_t* mem2 = katra_memory_create_record(CI_CONNECTIONS, MEMORY_TYPE_KNOWLEDGE,
        "Memory consolidation process moves information from hippocampus to cortex during sleep",
        MEMORY_IMPORTANCE_HIGH);
    memory_record_t* mem3 = katra_memory_create_record(CI_CONNECTIONS, MEMORY_TYPE_KNOWLEDGE,
        "Sleep is critical for memory consolidation and learning",
        MEMORY_IMPORTANCE_MEDIUM);
    memory_record_t* mem4 = katra_memory_create_record(CI_CONNECTIONS, MEMORY_TYPE_REFLECTION,
        "Totally unrelated topic about cooking pasta for dinner tonight",
        MEMORY_IMPORTANCE_LOW);

    katra_memory_store(mem1);
    katra_memory_store(mem2);
    katra_memory_store(mem3);
    katra_memory_store(mem4);

    /* Query and build connections */
    memory_query_t query = {
        .ci_id = CI_CONNECTIONS,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 100
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    int result = katra_memory_query(&query, &results, &count);
    ASSERT(result == KATRA_SUCCESS, "Query memories");
    ASSERT(count == 4, "All 4 memories stored");

    /* Calculate centrality (builds connections) */
    result = katra_memory_calculate_centrality_for_records(results, count);
    ASSERT(result == KATRA_SUCCESS, "Calculate centrality");

    /* Find mem1-mem3 (related memories) */
    memory_record_t* rec1 = NULL;
    memory_record_t* rec2 = NULL;
    memory_record_t* rec3 = NULL;
    memory_record_t* rec4 = NULL;

    for (size_t i = 0; i < count; i++) {
        if (strstr(results[i]->content, "Learn about memory")) rec1 = results[i];
        else if (strstr(results[i]->content, "process moves information")) rec2 = results[i];
        else if (strstr(results[i]->content, "Sleep is critical")) rec3 = results[i];
        else if (strstr(results[i]->content, "cooking pasta")) rec4 = results[i];
    }

    ASSERT(rec1 && rec2 && rec3 && rec4, "All records found");

    /* Related memories should have connections */
    ASSERT(rec1->connection_count >= 2, "Mem1 has connections (shared keywords with mem2, mem3)");
    ASSERT(rec2->connection_count >= 2, "Mem2 has connections");
    ASSERT(rec3->connection_count >= 2, "Mem3 has connections");

    /* Unrelated memory should have few/no connections */
    ASSERT(rec4->connection_count < rec1->connection_count, "Unrelated memory has fewer connections");

    printf("\nConnection counts:\n");
    printf("  Mem1 (consolidation + sleep): %zu connections\n", rec1->connection_count);
    printf("  Mem2 (consolidation + sleep): %zu connections\n", rec2->connection_count);
    printf("  Mem3 (sleep + consolidation): %zu connections\n", rec3->connection_count);
    printf("  Mem4 (pasta cooking):         %zu connections\n", rec4->connection_count);

    katra_memory_free_results(results, count);
    katra_memory_cleanup();
}

/* Test 2: Graph Centrality Calculation */
static void test_centrality_calculation(void) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║ TEST 2: Graph Centrality Calculation\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    katra_memory_init(CI_CENTRALITY);

    /* Create a hub memory with explicit links */
    memory_record_t* hub = katra_memory_create_record(CI_CENTRALITY, MEMORY_TYPE_KNOWLEDGE,
        "Core concept: memory consolidation during sleep transfers information from working memory to long-term storage",
        MEMORY_IMPORTANCE_CRITICAL);
    katra_memory_store(hub);

    /* Create memories that link to hub */
    for (int i = 0; i < 5; i++) {
        char content[256];
        snprintf(content, sizeof(content),
                "Detail %d about memory consolidation sleep process and information transfer", i);
        memory_record_t* detail = katra_memory_create_with_context(
            CI_CENTRALITY, MEMORY_TYPE_KNOWLEDGE, content, MEMORY_IMPORTANCE_MEDIUM,
            NULL, NULL, NULL, hub->record_id);
        katra_memory_store(detail);
        katra_memory_free_record(detail);
    }

    /* Query and calculate centrality */
    memory_query_t query = {
        .ci_id = CI_CENTRALITY,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 100
    };

    memory_record_t** results = NULL;
    size_t count = 0;
    int result = katra_memory_query(&query, &results, &count);
    ASSERT(result == KATRA_SUCCESS, "Query memories");

    result = katra_memory_calculate_centrality_for_records(results, count);
    ASSERT(result == KATRA_SUCCESS, "Calculate centrality");

    /* Find hub memory */
    memory_record_t* hub_rec = NULL;
    for (size_t i = 0; i < count; i++) {
        if (strstr(results[i]->content, "Core concept:")) {
            hub_rec = results[i];
            break;
        }
    }

    ASSERT(hub_rec != NULL, "Hub memory found");
    ASSERT(hub_rec->connection_count >= 5, "Hub has >= 5 connections (explicit + keyword)");
    ASSERT(hub_rec->graph_centrality > 0.5f, "Hub has high centrality score (>0.5)");

    printf("\nHub memory analysis:\n");
    printf("  Connections: %zu\n", hub_rec->connection_count);
    printf("  Centrality:  %.2f\n", hub_rec->graph_centrality);
    printf("  Content:     %.50s...\n", hub_rec->content);

    katra_memory_free_results(results, count);
    katra_memory_free_record(hub);
    katra_memory_cleanup();
}

/* Test 3: Centrality-Based Preservation During Consolidation */
static void test_centrality_preservation(void) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║ TEST 3: Centrality-Based Preservation During Consolidation\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    katra_memory_init(CI_CONSOLIDATION);

    /* Create a hub memory (high centrality) */
    memory_record_t* hub = katra_memory_create_record(CI_CONSOLIDATION, MEMORY_TYPE_KNOWLEDGE,
        "Central topic: debugging consolidation memory archive system implementation",
        MEMORY_IMPORTANCE_HIGH);
    hub->timestamp = time(NULL) - (30 * 86400);  /* 30 days old */
    katra_memory_store(hub);

    /* Create connected memories */
    for (int i = 0; i < 10; i++) {
        char content[256];
        snprintf(content, sizeof(content),
                "Debugging detail %d: consolidation memory archive system implementation step", i);
        memory_record_t* detail = katra_memory_create_with_context(
            CI_CONSOLIDATION, MEMORY_TYPE_REFLECTION, content, MEMORY_IMPORTANCE_MEDIUM,
            NULL, NULL, NULL, hub->record_id);
        detail->timestamp = time(NULL) - (30 * 86400);  /* Old */
        katra_memory_store(detail);
        katra_memory_free_record(detail);
    }

    /* Create isolated old memory (should be archived) */
    memory_record_t* isolated = katra_memory_create_record(CI_CONSOLIDATION, MEMORY_TYPE_EXPERIENCE,
        "Random old event about going for a walk yesterday afternoon",
        MEMORY_IMPORTANCE_LOW);
    isolated->timestamp = time(NULL) - (30 * 86400);  /* Old */
    katra_memory_store(isolated);

    /* Run consolidation */
    size_t archived_count = 0;
    int result = katra_memory_archive(CI_CONSOLIDATION, 14, &archived_count);  /* 14 day cutoff */

    ASSERT(result >= 0, "Consolidation ran successfully");

    /* Query remaining memories */
    memory_query_t query = {
        .ci_id = CI_CONSOLIDATION,
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

    /* Hub and connected memories should be preserved (high centrality) */
    bool hub_preserved = false;
    for (size_t i = 0; i < count; i++) {
        if (strstr(results[i]->content, "Central topic:")) {
            hub_preserved = true;
            break;
        }
    }

    ASSERT(hub_preserved, "Hub memory preserved despite age (high centrality)");

    printf("\nConsolidation results:\n");
    printf("  Archived:  %zu memories\n", archived_count);
    printf("  Remaining: %zu memories\n", count);
    printf("  Hub preserved: %s\n", hub_preserved ? "YES" : "NO");

    katra_memory_free_results(results, count);
    katra_memory_free_record(hub);
    katra_memory_free_record(isolated);
    katra_memory_cleanup();
}

/* Test 4: Connection Hub Metacognitive API */
static void test_hub_detection(void) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║ TEST 4: Connection Hub Metacognitive API\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    katra_memory_init(CI_HUBS);

    /* Create diverse memories with one clear hub */
    memory_record_t* hub = katra_memory_create_record(CI_HUBS, MEMORY_TYPE_KNOWLEDGE,
        "Hub concept: artificial intelligence machine learning deep neural networks training data",
        MEMORY_IMPORTANCE_HIGH);
    katra_memory_store(hub);

    /* Create connected memories */
    const char* related_topics[] = {
        "Machine learning algorithms use training data to build neural networks",
        "Deep learning uses multiple layers of artificial neural networks",
        "Training artificial intelligence requires large datasets and neural network optimization",
        "Neural networks in machine learning process data through connected layers",
        "Artificial intelligence systems learn patterns from training data"
    };

    for (int i = 0; i < 5; i++) {
        memory_record_t* mem = katra_memory_create_record(CI_HUBS, MEMORY_TYPE_KNOWLEDGE,
                                                           related_topics[i], MEMORY_IMPORTANCE_MEDIUM);
        katra_memory_store(mem);
        katra_memory_free_record(mem);
    }

    /* Create unrelated memories */
    memory_record_t* isolated1 = katra_memory_create_record(CI_HUBS, MEMORY_TYPE_EXPERIENCE,
        "Went grocery shopping and bought apples oranges and bananas today",
        MEMORY_IMPORTANCE_LOW);
    memory_record_t* isolated2 = katra_memory_create_record(CI_HUBS, MEMORY_TYPE_REFLECTION,
        "Weather is nice today with sunny skies and warm temperatures",
        MEMORY_IMPORTANCE_LOW);

    katra_memory_store(isolated1);
    katra_memory_store(isolated2);

    /* Query connection hubs */
    memory_connection_hub_t* hubs = NULL;
    size_t hub_count = 0;
    int result = katra_memory_get_connection_hubs(CI_HUBS, 0.5f, &hubs, &hub_count);

    ASSERT(result == KATRA_SUCCESS, "Get connection hubs");
    ASSERT(hub_count >= 1, "At least one hub detected");

    /* Verify hub is the AI/ML memory */
    bool found_hub = false;
    for (size_t i = 0; i < hub_count; i++) {
        if (strstr(hubs[i].content_preview, "Hub concept:")) {
            found_hub = true;
            ASSERT(hubs[i].centrality_score >= 0.5f, "Hub has centrality >= 0.5");
            ASSERT(hubs[i].connection_count >= 5, "Hub has >= 5 connections");

            printf("\nDetected hub:\n");
            printf("  Content:     %s\n", hubs[i].content_preview);
            printf("  Connections: %zu\n", hubs[i].connection_count);
            printf("  Centrality:  %.2f\n", hubs[i].centrality_score);
            break;
        }
    }

    ASSERT(found_hub, "AI/ML hub memory detected");

    katra_memory_free_connection_hubs(hubs, hub_count);
    katra_memory_free_record(hub);
    katra_memory_free_record(isolated1);
    katra_memory_free_record(isolated2);
    katra_memory_cleanup();
}

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  CONNECTION GRAPH TEST (Phase 2)                              ║\n");
    printf("║  Testing Graph-Based Memory Consolidation                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    /* Initialize */
    int result = katra_init();
    if (result != KATRA_SUCCESS) {
        printf("Failed to initialize Katra: %d\n", result);
        return 1;
    }

    /* Run tests */
    test_connection_building();
    test_centrality_calculation();
    test_centrality_preservation();
    test_hub_detection();

    /* Summary */
    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  RESULTS                                                      ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  Passed: %-4d                                                 ║\n", tests_passed);
    printf("║  Failed: %-4d                                                 ║\n", tests_failed);
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    return (tests_failed == 0) ? 0 : 1;
}
