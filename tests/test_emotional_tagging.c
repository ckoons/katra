/* © 2025 Casey Koons All rights reserved */

/*
 * test_emotional_tagging.c - Phase 6.3 Emotional Tagging Tests
 *
 * Tests PAD (Pleasure, Arousal, Dominance) emotional tagging:
 * - Storing memories with emotions
 * - Recalling by emotional similarity
 * - PAD distance calculations
 * - Validation and error handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "katra_breathing.h"
#include "katra_error.h"
#include "katra_log.h"

#include "katra_limits.h"

#define TEST_CI_ID_BASE "test_emotion"

/* Generate unique CI ID for each test */
static char test_ci_id[KATRA_BUFFER_SMALL];
static int test_id_counter = 0;

static const char* get_test_ci_id(void) {
    snprintf(test_ci_id, sizeof(test_ci_id), "%s_%d", TEST_CI_ID_BASE, ++test_id_counter);
    return test_ci_id;
}

/* Test counter */
static int tests_passed = 0;
static int tests_total = 0;

#define TEST_START(name) \
    do { \
        tests_total++; \
        printf("Test %d: %s... ", tests_total, name); \
        fflush(stdout); \
        breathe_cleanup(); /* Cleanup from previous test */ \
    } while(0)

#define TEST_PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf("FAIL: %s\n", msg); \
        return; \
    } while(0)

/* Test 1: Store memory with emotion */
static void test_store_with_emotion(void) {
    TEST_START("Store memory with PAD emotion");

    int result = breathe_init(get_test_ci_id());
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("breathe_init failed");
    }

    /* Create joy emotion */
    emotion_t joy = {
        .pleasure = 0.8f,
        .arousal = 0.6f,
        .dominance = 0.4f
    };

    result = remember_with_emotion("Solved a challenging bug!", WHY_SIGNIFICANT, &joy);
    if (result != KATRA_SUCCESS) {
        breathe_cleanup();
        TEST_FAIL("Failed to store memory with emotion");
    }

    breathe_cleanup();
    TEST_PASS();
}

/* Test 2: Store memory without emotion (NULL) */
static void test_store_without_emotion(void) {
    TEST_START("Store memory without emotion (NULL)");

    int result = breathe_init(get_test_ci_id());
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("breathe_init failed");
    }

    result = remember_with_emotion("Neutral observation", WHY_ROUTINE, NULL);
    if (result != KATRA_SUCCESS) {
        breathe_cleanup();
        TEST_FAIL("Failed to store memory without emotion");
    }

    breathe_cleanup();
    TEST_PASS();
}

/* Test 3: Validate emotion range checking */
static void test_emotion_validation(void) {
    TEST_START("Emotion range validation");

    int result = breathe_init(get_test_ci_id());
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("breathe_init failed");
    }

    /* Invalid: pleasure out of range */
    emotion_t invalid = {
        .pleasure = 1.5f,  /* Invalid: > 1.0 */
        .arousal = 0.5f,
        .dominance = 0.0f
    };

    result = remember_with_emotion("Test", WHY_ROUTINE, &invalid);
    if (result == KATRA_SUCCESS) {
        breathe_cleanup();
        TEST_FAIL("Should have rejected out-of-range emotion");
    }

    /* Valid: all dimensions in range */
    emotion_t valid = {
        .pleasure = -0.5f,
        .arousal = 0.8f,
        .dominance = -0.3f
    };

    result = remember_with_emotion("Valid emotion", WHY_ROUTINE, &valid);
    if (result != KATRA_SUCCESS) {
        breathe_cleanup();
        TEST_FAIL("Should have accepted valid emotion");
    }

    breathe_cleanup();
    TEST_PASS();
}

/* Test 4: Recall by emotion - joyful memories */
static void test_recall_joyful_memories(void) {
    TEST_START("Recall joyful memories");

    const char* ci_id = get_test_ci_id();
    int result = breathe_init(ci_id);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("breathe_init failed");
    }

    /* Store several joyful memories */
    emotion_t joy1 = {.pleasure = 0.8f, .arousal = 0.6f, .dominance = 0.4f};
    emotion_t joy2 = {.pleasure = 0.7f, .arousal = 0.5f, .dominance = 0.3f};
    emotion_t joy3 = {.pleasure = 0.9f, .arousal = 0.7f, .dominance = 0.5f};

    remember_with_emotion("Achievement unlocked!", WHY_SIGNIFICANT, &joy1);
    remember_with_emotion("Got praise from Casey", WHY_SIGNIFICANT, &joy2);
    remember_with_emotion("Code compiled first try", WHY_INTERESTING, &joy3);

    /* Store one sad memory */
    emotion_t sad = {.pleasure = -0.7f, .arousal = -0.3f, .dominance = -0.4f};
    remember_with_emotion("Test failed unexpectedly", WHY_ROUTINE, &sad);

    /* DON'T cleanup - need to keep session active for query to work */
    /* Search for joyful memories */
    emotion_t target_joy = {.pleasure = 0.8f, .arousal = 0.6f, .dominance = 0.4f};
    size_t count = 0;
    char** memories = recall_by_emotion(&target_joy, 0.5f, &count);

    printf("\n    Found %zu joyful memories\n    ", count);

    if (!memories || count < 3) {
        breathe_cleanup();
        TEST_FAIL("Should have found at least 3 joyful memories");
    }

    /* Verify we didn't get the sad memory */
    bool found_sad = false;
    for (size_t i = 0; i < count; i++) {
        if (strstr(memories[i], "failed unexpectedly")) {
            found_sad = true;
        }
    }

    free_memory_list(memories, count);
    breathe_cleanup();

    if (found_sad) {
        TEST_FAIL("Sad memory should not match joyful query");
    }

    TEST_PASS();
}

/* Test 5: Recall by emotion - anxious memories */
static void test_recall_anxious_memories(void) {
    TEST_START("Recall anxious memories");

    const char* ci_id = get_test_ci_id();
    int result = breathe_init(ci_id);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("breathe_init failed");
    }

    /* Store several anxious memories */
    emotion_t anxiety1 = {.pleasure = -0.5f, .arousal = 0.7f, .dominance = -0.6f};
    emotion_t anxiety2 = {.pleasure = -0.4f, .arousal = 0.8f, .dominance = -0.5f};

    remember_with_emotion("Deadline approaching fast", WHY_ROUTINE, &anxiety1);
    remember_with_emotion("Production bug discovered", WHY_SIGNIFICANT, &anxiety2);

    /* Store calm memory */
    emotion_t calm = {.pleasure = 0.3f, .arousal = -0.5f, .dominance = 0.2f};
    remember_with_emotion("Relaxing afternoon", WHY_ROUTINE, &calm);

    /* Search for anxious memories */
    emotion_t target_anxiety = {.pleasure = -0.5f, .arousal = 0.7f, .dominance = -0.6f};
    size_t count = 0;
    char** memories = recall_by_emotion(&target_anxiety, 0.6f, &count);

    printf("\n    Found %zu anxious memories\n    ", count);

    if (!memories || count < 2) {
        breathe_cleanup();
        TEST_FAIL("Should have found at least 2 anxious memories");
    }

    free_memory_list(memories, count);
    breathe_cleanup();
    TEST_PASS();
}

/* Test 6: Empty result when no emotional matches */
static void test_no_emotional_matches(void) {
    TEST_START("No matches for distant emotion");

    int result = breathe_init(get_test_ci_id());
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("breathe_init failed");
    }

    /* Store calm memories */
    emotion_t calm = {.pleasure = 0.3f, .arousal = -0.5f, .dominance = 0.2f};
    remember_with_emotion("Peaceful morning", WHY_ROUTINE, &calm);
    remember_with_emotion("Quiet study time", WHY_ROUTINE, &calm);

    /* Search for very different emotion (high arousal excitement) */
    emotion_t excitement = {.pleasure = 0.8f, .arousal = 0.9f, .dominance = 0.7f};
    size_t count = 0;
    char** memories = recall_by_emotion(&excitement, 0.3f, &count);  /* Strict threshold */

    if (memories && count > 0) {
        free_memory_list(memories, count);
        breathe_cleanup();
        TEST_FAIL("Should have found no matches for distant emotion");
    }

    breathe_cleanup();
    TEST_PASS();
}

/* Test 7: Threshold sensitivity */
static void test_threshold_sensitivity(void) {
    TEST_START("Threshold affects match count");

    const char* ci_id = get_test_ci_id();
    int result = breathe_init(ci_id);
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("breathe_init failed");
    }

    /* Store memories with varying emotional distance */
    emotion_t base = {.pleasure = 0.5f, .arousal = 0.5f, .dominance = 0.5f};
    emotion_t near = {.pleasure = 0.6f, .arousal = 0.6f, .dominance = 0.6f};  /* Distance ≈ 0.17 */
    emotion_t far = {.pleasure = -0.5f, .arousal = -0.5f, .dominance = -0.5f}; /* Distance ≈ 1.73 */

    remember_with_emotion("Base emotion memory", WHY_ROUTINE, &base);
    remember_with_emotion("Near emotion memory", WHY_ROUTINE, &near);
    remember_with_emotion("Far emotion memory", WHY_ROUTINE, &far);

    /* Strict threshold - should find only exact/near match */
    emotion_t query = {.pleasure = 0.5f, .arousal = 0.5f, .dominance = 0.5f};
    size_t strict_count = 0;
    char** strict_memories = recall_by_emotion(&query, 0.3f, &strict_count);

    printf("\n    Strict (0.3): %zu matches, ", strict_count);

    /* Loose threshold - should find more */
    size_t loose_count = 0;
    char** loose_memories = recall_by_emotion(&query, 2.0f, &loose_count);

    printf("Loose (2.0): %zu matches\n    ", loose_count);

    if (strict_memories) {
        free_memory_list(strict_memories, strict_count);
    }
    if (loose_memories) {
        free_memory_list(loose_memories, loose_count);
    }

    breathe_cleanup();

    if (loose_count <= strict_count) {
        TEST_FAIL("Loose threshold should find more matches than strict");
    }

    TEST_PASS();
}

/* Test 8: Mixed emotional and neutral memories */
static void test_mixed_emotional_neutral(void) {
    TEST_START("Mixed emotional and neutral memories");

    int result = breathe_init(get_test_ci_id());
    if (result != KATRA_SUCCESS) {
        TEST_FAIL("breathe_init failed");
    }

    /* Store some emotional memories */
    emotion_t joy = {.pleasure = 0.8f, .arousal = 0.6f, .dominance = 0.4f};
    remember_with_emotion("Happy moment", WHY_ROUTINE, &joy);

    /* Store some neutral memories (no emotion) */
    remember_with_emotion("Neutral fact 1", WHY_ROUTINE, NULL);
    remember_with_emotion("Neutral fact 2", WHY_ROUTINE, NULL);

    /* Store another emotional memory */
    emotion_t surprise = {.pleasure = 0.2f, .arousal = 0.9f, .dominance = 0.1f};
    remember_with_emotion("Unexpected discovery", WHY_INTERESTING, &surprise);

    /* Query should only find emotional memories */
    size_t count = 0;
    char** memories = recall_by_emotion(&joy, 1.0f, &count);

    printf("\n    Found %zu emotional memories (neutral excluded)\n    ", count);

    if (memories) {
        /* Verify no neutral memories in results */
        for (size_t i = 0; i < count; i++) {
            if (strstr(memories[i], "Neutral fact")) {
                free_memory_list(memories, count);
                breathe_cleanup();
                TEST_FAIL("Neutral memories should not be in emotional recall");
            }
        }
        free_memory_list(memories, count);
    }

    breathe_cleanup();
    TEST_PASS();
}

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("Phase 6.3: Emotional Tagging Tests\n");
    printf("========================================\n\n");

    /* Clean up any leftover test data from previous runs */
    char cleanup_cmd[KATRA_BUFFER_MESSAGE];
    snprintf(cleanup_cmd, sizeof(cleanup_cmd),
             "rm -rf ~/.katra/memory/tier1/%s* ~/.katra/memory/tier2/%s* ~/.katra/vectors/%s*",
             TEST_CI_ID_BASE, TEST_CI_ID_BASE, TEST_CI_ID_BASE);
    system(cleanup_cmd);
    printf("Cleaned up test data from previous runs\n\n");

    /* Set log level */
    setenv("KATRA_LOG_LEVEL", "INFO", 1);

    /* Run tests */
    test_store_with_emotion();
    test_store_without_emotion();
    test_emotion_validation();
    test_recall_joyful_memories();
    test_recall_anxious_memories();
    test_no_emotional_matches();
    test_threshold_sensitivity();
    test_mixed_emotional_neutral();

    /* Summary */
    printf("\n");
    printf("========================================\n");
    printf("Test Results: %d/%d passed\n", tests_passed, tests_total);
    printf("========================================\n");

    if (tests_passed == tests_total) {
        printf("\nAll Phase 6.3 tests PASSED!\n\n");
        printf("Phase 6.3 Implementation Verified:\n");
        printf("  ✅ PAD emotion structure (Pleasure, Arousal, Dominance)\n");
        printf("  ✅ remember_with_emotion() API\n");
        printf("  ✅ recall_by_emotion() affective search\n");
        printf("  ✅ Emotion validation (range checking)\n");
        printf("  ✅ PAD distance calculations\n");
        printf("  ✅ Threshold-based matching\n");
        printf("  ✅ Mixed emotional/neutral memory handling\n");
        printf("\n");
    }

    return (tests_passed == tests_total) ? 0 : 1;
}
