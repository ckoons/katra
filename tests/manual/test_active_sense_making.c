/* © 2025 Casey Koons All rights reserved */

/*
 * Comprehensive test for Phase 1: Active Sense-Making
 *
 * Tests Thane's recommendations:
 * 1. Formation context (why did I remember this?)
 * 2. Metacognitive awareness (what do I know about my memory state?)
 * 3. JSON persistence of context fields
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_error.h"
#include "katra_log.h"

/* Unique CI IDs per test to avoid accumulation */
#define CI_FORMATION_BASIC "test_formation_basic"
#define CI_FORMATION_LINKED "test_formation_linked"
#define CI_FORMATION_PERSIST "test_formation_persist"
#define CI_HEALTH "test_metacog_health"
#define CI_AT_RISK "test_metacog_at_risk"
#define CI_PATTERNS "test_metacog_patterns"
#define CI_DEBUGGING "test_real_world_debug"

#define DAYS_AGO(d) (time(NULL) - ((d) * 24 * 3600))

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

/* Test 1: Formation Context - Basic Creation */
void test_formation_context_basic(void) {
    phase_header("TEST 1: Formation Context - Basic Creation");

    katra_memory_init(CI_FORMATION_BASIC);

    /* Create memory with full context */
    memory_record_t* rec = katra_memory_create_with_context(
        CI_FORMATION_BASIC,
        MEMORY_TYPE_EXPERIENCE,
        "Fixed archive completion bug - records now marked as archived",
        0.9,
        "Why wasn't consolidation working?",           /* question */
        "Actually two bugs: marking AND query filtering", /* resolution */
        "Thought it was threshold issue",              /* uncertainty */
        NULL                                           /* related_to */
    );

    test_assert(rec != NULL, "Create memory with context");

    if (rec) {
        test_assert(rec->context_question != NULL, "Context question stored");
        test_assert(rec->context_resolution != NULL, "Context resolution stored");
        test_assert(rec->context_uncertainty != NULL, "Context uncertainty stored");

        test_assert(strcmp(rec->context_question, "Why wasn't consolidation working?") == 0,
                   "Context question content correct");
        test_assert(strcmp(rec->context_resolution,
                          "Actually two bugs: marking AND query filtering") == 0,
                   "Context resolution content correct");
        test_assert(strcmp(rec->context_uncertainty, "Thought it was threshold issue") == 0,
                   "Context uncertainty content correct");

        /* Store and retrieve */
        int result = katra_memory_store(rec);
        test_assert(result == KATRA_SUCCESS, "Store memory with context");

        katra_memory_free_record(rec);
    }

    katra_memory_cleanup();
}

/* Test 2: Formation Context - Linked Memories */
void test_formation_context_linked(void) {
    phase_header("TEST 2: Formation Context - Linked Memories");

    katra_memory_init(CI_FORMATION_LINKED);

    /* First memory - initial discovery */
    memory_record_t* first = katra_memory_create_with_context(
        CI_FORMATION_LINKED,
        MEMORY_TYPE_EXPERIENCE,
        "Archive marks records but queries still return them",
        0.8,
        "Why does consolidation report success but show 0% compression?",
        "Records marked as archived in JSONL but queries don't filter them",
        "Suspected threshold was too strict",
        NULL
    );

    if (first) {
        katra_memory_store(first);

        /* Second memory - related resolution */
        memory_record_t* second = katra_memory_create_with_context(
            CI_FORMATION_LINKED,
            MEMORY_TYPE_EXPERIENCE,
            "Added archived filter to scan_file_for_records()",
            0.9,
            "How do we exclude archived records from queries?",
            "4-line fix in tier1.c:290-294 checks record->archived",
            "Wasn't sure if issue was in query or consolidation",
            first->record_id  /* Link to previous memory */
        );

        if (second) {
            test_assert(second->related_to != NULL, "Related memory link stored");
            test_assert(strcmp(second->related_to, first->record_id) == 0,
                       "Related memory ID matches first memory");

            katra_memory_store(second);
            katra_memory_free_record(second);
        }

        katra_memory_free_record(first);
    }

    katra_memory_cleanup();
}

/* Test 3: Formation Context - Persistence */
void test_formation_context_persistence(void) {
    phase_header("TEST 3: Formation Context - JSON Persistence");

    katra_memory_init(CI_FORMATION_PERSIST);

    /* Create and store */
    memory_record_t* original = katra_memory_create_with_context(
        CI_FORMATION_PERSIST,
        MEMORY_TYPE_REFLECTION,
        "Casey values consent-first design philosophy",
        0.9,
        "What are Casey's core values in AI design?",
        "Ethics before expedience, consent is mandatory",
        "Unsure if this applied to all features or just memory deletion",
        NULL
    );

    if (original) {
        char* original_id = strdup(original->record_id);
        katra_memory_store(original);
        katra_memory_free_record(original);

        /* Query it back */
        memory_query_t query = {
            .ci_id = CI_FORMATION_PERSIST,
            .limit = 100
        };
        memory_record_t** results = NULL;
        size_t count = 0;

        katra_memory_query(&query, &results, &count);

        int found = 0;
        if (results) {
            for (size_t i = 0; i < count; i++) {
                if (strcmp(results[i]->record_id, original_id) == 0) {
                    found = 1;

                    test_assert(results[i]->context_question != NULL,
                               "Context question persisted");
                    test_assert(results[i]->context_resolution != NULL,
                               "Context resolution persisted");
                    test_assert(results[i]->context_uncertainty != NULL,
                               "Context uncertainty persisted");

                    if (results[i]->context_question) {
                        test_assert(strstr(results[i]->context_question, "core values") != NULL,
                                   "Context question content survived round-trip");
                    }
                    if (results[i]->context_resolution) {
                        test_assert(strstr(results[i]->context_resolution, "Ethics before") != NULL,
                                   "Context resolution content survived round-trip");
                    }

                    break;
                }
            }
            katra_memory_free_results(results, count);
        }

        test_assert(found, "Memory with context retrieved from storage");
        free(original_id);
    }

    katra_memory_cleanup();
}

/* Test 4: Metacognitive Awareness - Consolidation Health */
void test_metacognitive_health(void) {
    phase_header("TEST 4: Metacognitive Awareness - Health Status");

    katra_memory_init(CI_HEALTH);

    /* Create some test memories */
    for (int i = 0; i < 10; i++) {
        char content[256];
        snprintf(content, sizeof(content), "Test memory %d for health tracking", i);

        memory_record_t* rec = katra_memory_create_record(
            CI_HEALTH,
            MEMORY_TYPE_EXPERIENCE,
            content,
            0.5
        );

        if (rec) {
            rec->timestamp = DAYS_AGO(i * 3);  /* Varying ages */
            katra_memory_store(rec);
            katra_memory_free_record(rec);
        }
    }

    /* Get health status */
    memory_consolidation_health_t health = {0};
    int result = katra_memory_get_consolidation_health(CI_HEALTH, &health);

    test_assert(result == KATRA_SUCCESS, "Get consolidation health");
    test_assert(health.total_memories >= 10, "Health reports correct total count");
    test_assert(health.active_memories >= 10, "Health reports active memories");
    test_assert(health.health_status != NULL, "Health status string provided");

    printf("\nHealth Report:\n");
    printf("  Total: %zu\n", health.total_memories);
    printf("  Active: %zu\n", health.active_memories);
    printf("  Archived: %zu\n", health.archived_memories);
    printf("  Compression: %.1f%%\n", health.compression_ratio * 100.0f);
    printf("  Status: %s\n", health.health_status);
    printf("  Consolidation recommended: %s\n",
           health.consolidation_recommended ? "YES" : "NO");

    katra_memory_cleanup();
}

/* Test 5: Metacognitive Awareness - At-Risk Memories */
void test_metacognitive_at_risk(void) {
    phase_header("TEST 5: Metacognitive Awareness - Memories At Risk");

    katra_memory_init(CI_AT_RISK);

    /* Create old memory (at risk) */
    memory_record_t* old = katra_memory_create_record(
        CI_AT_RISK,
        MEMORY_TYPE_EXPERIENCE,
        "Old routine status check from 25 days ago",
        0.3  /* Low importance */
    );
    if (old) {
        old->timestamp = DAYS_AGO(25);
        old->emotion_intensity = 0.2f;  /* Low emotion */
        katra_memory_store(old);
        katra_memory_free_record(old);
    }

    /* Create recent memory (safe) */
    memory_record_t* recent = katra_memory_create_record(
        CI_AT_RISK,
        MEMORY_TYPE_EXPERIENCE,
        "Recent important breakthrough discovery",
        0.9
    );
    if (recent) {
        recent->timestamp = DAYS_AGO(2);
        recent->emotion_intensity = 0.9f;  /* High emotion */
        katra_memory_store(recent);
        katra_memory_free_record(recent);
    }

    /* Get at-risk memories */
    memory_at_risk_t* at_risk = NULL;
    size_t count = 0;
    int result = katra_memory_get_at_risk(CI_AT_RISK, 20, &at_risk, &count);

    test_assert(result == KATRA_SUCCESS, "Get at-risk memories");
    test_assert(count > 0, "At least one memory identified as at risk");

    if (at_risk && count > 0) {
        printf("\nAt-Risk Memories:\n");
        for (size_t i = 0; i < count; i++) {
            printf("  [%.2f] %s\n", at_risk[i].risk_score, at_risk[i].content_preview);
            printf("      Reason: %s\n", at_risk[i].risk_reason);

            test_assert(at_risk[i].risk_score >= 0.0 && at_risk[i].risk_score <= 1.0,
                       "Risk score in valid range");
            test_assert(at_risk[i].risk_reason != NULL, "Risk reason provided");
        }

        katra_memory_free_at_risk(at_risk, count);
    }

    katra_memory_cleanup();
}

/* Test 6: Metacognitive Awareness - Pattern Detection */
void test_metacognitive_patterns(void) {
    phase_header("TEST 6: Metacognitive Awareness - Detected Patterns");

    katra_memory_init(CI_PATTERNS);

    /* Create pattern: similar debugging memories */
    for (int i = 0; i < 5; i++) {
        char content[256];
        snprintf(content, sizeof(content),
                "Debugging null pointer exception in module_%d", i);

        memory_record_t* rec = katra_memory_create_record(
            CI_PATTERNS,
            MEMORY_TYPE_EXPERIENCE,
            content,
            0.5
        );

        if (rec) {
            rec->timestamp = DAYS_AGO(15);
            katra_memory_store(rec);
            katra_memory_free_record(rec);
        }
    }

    /* Archive to trigger pattern detection */
    size_t archived = 0;
    katra_memory_archive(CI_PATTERNS, 10, &archived);

    /* Get detected patterns */
    detected_pattern_t* patterns = NULL;
    size_t count = 0;
    int result = katra_memory_get_patterns(CI_PATTERNS, &patterns, &count);

    test_assert(result == KATRA_SUCCESS, "Get detected patterns");

    if (count > 0 && patterns) {
        printf("\nDetected Patterns:\n");
        for (size_t i = 0; i < count; i++) {
            printf("  Pattern '%s': %zu members\n",
                   patterns[i].pattern_id, patterns[i].member_count);
            printf("    Example: %s\n", patterns[i].centroid_preview);
            printf("    Threshold: %.2f\n", patterns[i].similarity_threshold);

            test_assert(patterns[i].member_count >= 3, "Pattern has minimum members");
            test_assert(patterns[i].centroid_preview != NULL, "Centroid provided");
        }

        katra_memory_free_patterns(patterns, count);
    } else {
        printf("\n  (No patterns detected - may need more similar memories)\n");
    }

    katra_memory_cleanup();
}

/* Test 7: Real-World Scenario - Debugging Session */
void test_real_world_debugging_session(void) {
    phase_header("TEST 7: Real-World Scenario - Debugging Session");

    katra_memory_init(CI_DEBUGGING);

    /* Simulate debugging session with formation context */

    /* 1. Initial problem observation */
    memory_record_t* problem = katra_memory_create_with_context(
        CI_DEBUGGING,
        MEMORY_TYPE_EXPERIENCE,
        "Consolidation reports success but compression shows 0%",
        0.7,
        "Why isn't memory consolidation working?",
        NULL,  /* No resolution yet */
        "Maybe thresholds are too strict? Or pattern detection broken?",
        NULL
    );

    if (problem) {
        katra_memory_store(problem);
        char* problem_id = strdup(problem->record_id);

        /* 2. Investigation and hypothesis */
        memory_record_t* investigation = katra_memory_create_with_context(
            CI_DEBUGGING,
            MEMORY_TYPE_REFLECTION,
            "Checked archive function - it marks records as archived in JSONL",
            0.6,
            "Is the archive function actually marking records?",
            "Yes, archived:true appears in JSONL files",
            "Suspected the marking step was missing entirely",
            problem_id
        );

        if (investigation) {
            katra_memory_store(investigation);
            char* investigation_id = strdup(investigation->record_id);

            /* 3. Breakthrough - finding the real bug */
            memory_record_t* breakthrough = katra_memory_create_with_context(
                CI_DEBUGGING,
                MEMORY_TYPE_EXPERIENCE,
                "Found it! Query function doesn't filter archived records",
                0.9,
                "If marking works, why do queries return archived records?",
                "scan_file_for_records() never checks record->archived field",
                "Thought maybe query was checking but filtering wasn't working",
                investigation_id
            );

            if (breakthrough) {
                breakthrough->emotion_intensity = 0.9f;  /* High emotion - breakthrough! */
                breakthrough->emotion_type = strdup("surprise");
                katra_memory_store(breakthrough);

                /* 4. Implementation */
                memory_record_t* fix = katra_memory_create_with_context(
                    CI_DEBUGGING,
                    MEMORY_TYPE_EXPERIENCE,
                    "Added 4-line archived filter to tier1.c:290-294",
                    0.8,
                    "How do we exclude archived records from queries?",
                    "Check record->archived after parsing, skip if true",
                    "Wasn't sure if this would handle all query paths",
                    breakthrough->record_id
                );

                if (fix) {
                    katra_memory_store(fix);
                    katra_memory_free_record(fix);
                }

                katra_memory_free_record(breakthrough);
            }

            free(investigation_id);
            katra_memory_free_record(investigation);
        }

        free(problem_id);
        katra_memory_free_record(problem);
    }

    /* Now query to see the debugging trail */
    memory_query_t query = {
        .ci_id = CI_DEBUGGING,
        .limit = 100
    };
    memory_record_t** results = NULL;
    size_t count = 0;

    katra_memory_query(&query, &results, &count);

    int memories_with_context = 0;
    int linked_memories = 0;

    if (results) {
        printf("\nDebugging Session Memory Trail:\n");
        for (size_t i = 0; i < count; i++) {
            if (results[i]->context_question) {
                memories_with_context++;
                printf("  [%s]\n", results[i]->content);
                printf("    Q: %s\n", results[i]->context_question);
                if (results[i]->context_resolution) {
                    printf("    R: %s\n", results[i]->context_resolution);
                }
                if (results[i]->related_to) {
                    linked_memories++;
                    printf("    → Links to: %s\n", results[i]->related_to);
                }
                printf("\n");
            }
        }
        katra_memory_free_results(results, count);
    }

    test_assert(memories_with_context >= 4, "Debugging trail has formation context");
    test_assert(linked_memories >= 2, "Memories are linked in causal chain");

    printf("Summary:\n");
    printf("  Memories with context: %d\n", memories_with_context);
    printf("  Linked memories: %d\n", linked_memories);

    katra_memory_cleanup();
}

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  ACTIVE SENSE-MAKING TEST (Phase 1)                          ║\n");
    printf("║  Testing Formation Context + Metacognitive Awareness         ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    log_set_level(LOG_INFO);

    /* Run all tests */
    test_formation_context_basic();
    test_formation_context_linked();
    test_formation_context_persistence();
    test_metacognitive_health();
    test_metacognitive_at_risk();
    test_metacognitive_patterns();
    test_real_world_debugging_session();

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
