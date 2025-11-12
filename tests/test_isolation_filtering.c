/* © 2025 Casey Koons All rights reserved */

/*
 * test_isolation_filtering.c - Tests for namespace isolation filtering (Phase 7)
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Project includes */
#include "katra_memory.h"
#include "katra_tier1.h"
#include "katra_team.h"
#include "katra_error.h"

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    printf("\n[TEST] %s\n", #name); \
    if (run_test_##name()) { \
        printf("[PASS] %s\n", #name); \
        tests_passed++; \
    } else { \
        printf("[FAIL] %s\n", #name); \
        tests_failed++; \
    }

/* Helper: print result status */
static void print_result(int result) {
    if (result == KATRA_SUCCESS) {
        printf("  -> KATRA_SUCCESS\n");
    } else {
        printf("  -> ERROR: %s\n", katra_error_string(result));
    }
}

/* ============================================================================
 * TEST 1: Access Control Function - Owner Access
 * ============================================================================ */
static int run_test_owner_access(void) {
    memory_record_t* record = katra_memory_create_record("ci-001", MEMORY_TYPE_EXPERIENCE,
                                                           "Test content", 0.5f);
    if (!record) {
        printf("  Failed to create record\n");
        return 0;
    }

    record->isolation = ISOLATION_PRIVATE;

    /* Owner should have access */
    printf("  Checking owner access to PRIVATE memory...\n");
    int has_access = katra_memory_check_access(record, "ci-001");
    printf("  -> %s\n", has_access ? "granted" : "denied");

    /* NULL requesting_ci_id means owner */
    int has_access_null = katra_memory_check_access(record, NULL);
    printf("  -> NULL requesting_ci (owner): %s\n", has_access_null ? "granted" : "denied");

    katra_memory_free_record(record);
    return (has_access && has_access_null);
}

/* ============================================================================
 * TEST 2: Access Control Function - Private Isolation
 * ============================================================================ */
static int run_test_private_isolation(void) {
    memory_record_t* record = katra_memory_create_record("ci-001", MEMORY_TYPE_EXPERIENCE,
                                                           "Test content", 0.5f);
    if (!record) {
        printf("  Failed to create record\n");
        return 0;
    }

    record->isolation = ISOLATION_PRIVATE;

    /* Other CI should NOT have access */
    printf("  Checking other CI access to PRIVATE memory...\n");
    int has_access = katra_memory_check_access(record, "ci-002");
    printf("  -> %s (should be denied)\n", has_access ? "granted" : "denied");

    katra_memory_free_record(record);
    return (!has_access);
}

/* ============================================================================
 * TEST 3: Access Control Function - Public Isolation
 * ============================================================================ */
static int run_test_public_isolation(void) {
    memory_record_t* record = katra_memory_create_record("ci-001", MEMORY_TYPE_EXPERIENCE,
                                                           "Test content", 0.5f);
    if (!record) {
        printf("  Failed to create record\n");
        return 0;
    }

    record->isolation = ISOLATION_PUBLIC;

    /* Any CI should have access */
    printf("  Checking other CI access to PUBLIC memory...\n");
    int has_access = katra_memory_check_access(record, "ci-002");
    printf("  -> %s (should be granted)\n", has_access ? "granted" : "denied");

    katra_memory_free_record(record);
    return (has_access);
}

/* ============================================================================
 * TEST 4: Access Control Function - Team Isolation
 * ============================================================================ */
static int run_test_team_isolation(void) {
    int result = KATRA_SUCCESS;

    /* Initialize team system */
    printf("  Initializing team system...\n");
    result = katra_team_init();
    print_result(result);
    if (result != KATRA_SUCCESS) {
        return 0;
    }

    /* Create team */
    printf("  Creating team 'alpha'...\n");
    result = katra_team_create("alpha", "ci-001");
    print_result(result);
    if (result != KATRA_SUCCESS && result != E_DUPLICATE) {
        return 0;
    }

    /* Add member */
    printf("  Adding ci-002 to team...\n");
    result = katra_team_join("alpha", "ci-002", "ci-001");
    if (result != KATRA_SUCCESS && result != E_DUPLICATE) {
        print_result(result);
        return 0;
    }

    /* Create TEAM memory */
    memory_record_t* record = katra_memory_create_record("ci-001", MEMORY_TYPE_EXPERIENCE,
                                                           "Test content", 0.5f);
    if (!record) {
        printf("  Failed to create record\n");
        return 0;
    }

    record->isolation = ISOLATION_TEAM;
    record->team_name = strdup("alpha");

    /* Team member should have access */
    printf("  Checking team member (ci-002) access to TEAM memory...\n");
    int member_access = katra_memory_check_access(record, "ci-002");
    printf("  -> %s (should be granted)\n", member_access ? "granted" : "denied");

    /* Non-member should NOT have access */
    printf("  Checking non-member (ci-003) access to TEAM memory...\n");
    int nonmember_access = katra_memory_check_access(record, "ci-003");
    printf("  -> %s (should be denied)\n", nonmember_access ? "granted" : "denied");

    katra_memory_free_record(record);
    return (member_access && !nonmember_access);
}

/* ============================================================================
 * TEST 5: Access Control Function - Explicit Sharing
 * ============================================================================ */
static int run_test_explicit_sharing(void) {
    memory_record_t* record = katra_memory_create_record("ci-001", MEMORY_TYPE_EXPERIENCE,
                                                           "Test content", 0.5f);
    if (!record) {
        printf("  Failed to create record\n");
        return 0;
    }

    record->isolation = ISOLATION_PRIVATE;

    /* Add explicit sharing */
    record->shared_with = malloc(2 * sizeof(char*));
    record->shared_with[0] = strdup("ci-002");
    record->shared_with[1] = strdup("ci-003");
    record->shared_with_count = 2;

    /* Explicitly shared CI should have access */
    printf("  Checking explicitly shared CI (ci-002) access...\n");
    int shared_access = katra_memory_check_access(record, "ci-002");
    printf("  -> %s (should be granted)\n", shared_access ? "granted" : "denied");

    /* Non-shared CI should NOT have access */
    printf("  Checking non-shared CI (ci-004) access...\n");
    int nonshared_access = katra_memory_check_access(record, "ci-004");
    printf("  -> %s (should be denied)\n", nonshared_access ? "granted" : "denied");

    katra_memory_free_record(record);
    return (shared_access && !nonshared_access);
}

/* ============================================================================
 * TEST 6: Tier1 Query Filtering
 * ============================================================================ */
static int run_test_tier1_filtering(void) {
    int result = KATRA_SUCCESS;

    /* Initialize memory system for ci-001 */
    printf("  Initializing memory system for ci-001...\n");
    result = katra_memory_init("ci-001");
    if (result != KATRA_SUCCESS && result != E_ALREADY_INITIALIZED) {
        print_result(result);
        return 0;
    }

    /* Create and store PRIVATE memory */
    printf("  Storing PRIVATE memory...\n");
    memory_record_t* private_rec = katra_memory_create_record("ci-001", MEMORY_TYPE_EXPERIENCE,
                                                                "Private test memory", 0.5f);
    private_rec->isolation = ISOLATION_PRIVATE;
    result = tier1_store(private_rec);
    print_result(result);
    katra_memory_free_record(private_rec);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    /* Create and store PUBLIC memory */
    printf("  Storing PUBLIC memory...\n");
    memory_record_t* public_rec = katra_memory_create_record("ci-001", MEMORY_TYPE_EXPERIENCE,
                                                               "Public test memory", 0.5f);
    public_rec->isolation = ISOLATION_PUBLIC;
    result = tier1_store(public_rec);
    print_result(result);
    katra_memory_free_record(public_rec);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    /* Query as owner (should get both) */
    printf("  Querying as owner (ci-001)...\n");
    memory_query_t owner_query = {
        .ci_id = "ci-001",
        .requesting_ci_id = NULL,  /* NULL = owner */
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0f,
        .tier = 0,
        .limit = 0,
        .filter_personal = false,
        .collection_prefix = NULL,
        .filter_not_to_archive = false
    };

    memory_record_t** owner_results = NULL;
    size_t owner_count = 0;
    result = tier1_query(&owner_query, &owner_results, &owner_count);
    print_result(result);
    printf("  -> Owner got %zu results (should be 2)\n", owner_count);

    /* Free owner results */
    if (owner_results) {
        for (size_t i = 0; i < owner_count; i++) {
            katra_memory_free_record(owner_results[i]);
        }
        free(owner_results);
    }

    /* Query as other CI (should get only PUBLIC) */
    printf("  Querying as other CI (ci-002)...\n");
    memory_query_t other_query = {
        .ci_id = "ci-001",
        .requesting_ci_id = "ci-002",
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0f,
        .tier = 0,
        .limit = 0,
        .filter_personal = false,
        .collection_prefix = NULL,
        .filter_not_to_archive = false
    };

    memory_record_t** other_results = NULL;
    size_t other_count = 0;
    result = tier1_query(&other_query, &other_results, &other_count);
    print_result(result);
    printf("  -> Other CI got %zu results (should be 1)\n", other_count);

    /* Free other results */
    if (other_results) {
        for (size_t i = 0; i < other_count; i++) {
            katra_memory_free_record(other_results[i]);
        }
        free(other_results);
    }

    return (owner_count == 2 && other_count == 1);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */
int main(void) {
    printf("========================================\n");
    printf("Namespace Isolation Filtering Test Suite\n");
    printf("========================================\n");

    /* Run tests */
    TEST(owner_access);
    TEST(private_isolation);
    TEST(public_isolation);
    TEST(team_isolation);
    TEST(explicit_sharing);
    TEST(tier1_filtering);

    /* Cleanup */
    katra_team_cleanup();

    /* Summary */
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);
    printf("========================================\n");

    return (tests_failed == 0) ? 0 : 1;
}
