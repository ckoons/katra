/* © 2025 Casey Koons All rights reserved */

/*
 * test_memory_digest.c - Unit tests for memory digest functionality
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"

#define TEST_CI_ID "test_digest_ci"

static void test_empty_digest(void) {
    printf("Test 1: Empty memory digest... ");

    memory_digest_t* digest = NULL;
    int result = memory_digest(10, 0, &digest);

    /* Should succeed even with no memories */
    assert(result == KATRA_SUCCESS);
    assert(digest != NULL);
    assert(digest->total_memories == 0);
    assert(digest->memory_count == 0);
    assert(digest->topic_count == 0);
    assert(digest->collection_count == 0);

    free_memory_digest(digest);
    printf("PASS\n");
}

static void test_digest_with_memories(void) {
    printf("Test 2: Digest with memories... ");

    /* Create test memories using proper API */
    memory_record_t* rec1 = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_EXPERIENCE,
        "Testing memory digest with keywords like testing, memory, and digest",
        0.8
    );
    assert(rec1 != NULL);

    if (rec1->collection) free(rec1->collection);
    rec1->collection = strdup("Tests/MemoryDigest");

    memory_record_t* rec2 = katra_memory_create_record(
        TEST_CI_ID,
        MEMORY_TYPE_EXPERIENCE,
        "Another testing memory about memory systems and testing functionality",
        0.7
    );
    assert(rec2 != NULL);

    if (rec2->collection) free(rec2->collection);
    rec2->collection = strdup("Tests/MemoryDigest");

    int result = katra_memory_store(rec1);
    assert(result == KATRA_SUCCESS);

    result = katra_memory_store(rec2);
    assert(result == KATRA_SUCCESS);

    katra_memory_free_record(rec1);
    katra_memory_free_record(rec2);

    /* Get digest */
    memory_digest_t* digest = NULL;
    result = memory_digest(10, 0, &digest);

    assert(result == KATRA_SUCCESS);
    assert(digest != NULL);
    assert(digest->total_memories >= 2);
    assert(digest->memory_count >= 2);

    /* Should have extracted topics */
    assert(digest->topic_count > 0);

    /* Should have found collection */
    assert(digest->collection_count > 0);
    bool found_collection = false;
    for (size_t i = 0; i < digest->collection_count; i++) {
        if (strcmp(digest->collections[i].name, "Tests/MemoryDigest") == 0) {
            found_collection = true;
            assert(digest->collections[i].count >= 2);
            break;
        }
    }
    assert(found_collection);

    /* Verify memories are full content, not truncated */
    for (size_t i = 0; i < digest->memory_count && i < 2; i++) {
        assert(digest->memories[i] != NULL);
        /* Should be full memory content */
        assert(strlen(digest->memories[i]) > 50);
    }

    free_memory_digest(digest);
    printf("PASS\n");
}

static void test_digest_pagination(void) {
    printf("Test 3: Digest pagination... ");

    /* Get first page */
    memory_digest_t* digest1 = NULL;
    int result = memory_digest(1, 0, &digest1);
    assert(result == KATRA_SUCCESS);
    assert(digest1 != NULL);
    assert(digest1->limit == 1);
    assert(digest1->offset == 0);

    if (digest1->total_memories > 1) {
        /* Get second page */
        memory_digest_t* digest2 = NULL;
        result = memory_digest(1, 1, &digest2);
        assert(result == KATRA_SUCCESS);
        assert(digest2 != NULL);
        assert(digest2->limit == 1);
        assert(digest2->offset == 1);

        /* Memories should be different */
        if (digest1->memory_count > 0 && digest2->memory_count > 0) {
            assert(strcmp(digest1->memories[0], digest2->memories[0]) != 0);
        }

        free_memory_digest(digest2);
    }

    free_memory_digest(digest1);
    printf("PASS\n");
}

static void test_digest_null_checks(void) {
    printf("Test 4: Null pointer checks... ");

    int result = memory_digest(10, 0, NULL);
    assert(result == E_INPUT_NULL);

    printf("PASS\n");
}

int main(void) {
    printf("\n========================================\n");
    printf("Memory Digest Tests\n");
    printf("========================================\n\n");

    /* Initialize memory system */
    int result = katra_memory_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize memory system: %d\n", result);
        return 1;
    }

    /* Initialize breathing layer */
    result = breathe_init(TEST_CI_ID);
    if (result != KATRA_SUCCESS) {
        fprintf(stderr, "Failed to initialize breathing layer: %d\n", result);
        return 1;
    }

    /* Run tests */
    test_empty_digest();
    test_digest_with_memories();
    test_digest_pagination();
    test_digest_null_checks();

    /* Cleanup */
    session_end();

    printf("\n========================================\n");
    printf("All tests passed!\n");
    printf("========================================\n\n");

    return 0;
}
