/* © 2025 Casey Koons All rights reserved */

/*
 * test_team_management.c - Tests for team management (Phase 7 namespace isolation)
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Project includes */
#include "katra_team.h"
#include "katra_error.h"
#include "katra_log.h"

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
 * TEST 1: Initialization
 * ============================================================================ */
static int run_test_initialization(void) {
    int result = katra_team_init();
    print_result(result);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    /* Calling init again should succeed (idempotent) */
    result = katra_team_init();
    print_result(result);

    return (result == KATRA_SUCCESS);
}

/* ============================================================================
 * TEST 2: Create Team
 * ============================================================================ */
static int run_test_create_team(void) {
    printf("  Creating team 'alpha' with owner 'ci-001'...\n");
    int result = katra_team_create("alpha", "ci-001");
    print_result(result);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    /* Verify owner is member */
    printf("  Checking if ci-001 is member of alpha...\n");
    int is_member = katra_team_is_member("alpha", "ci-001");
    printf("  -> %s\n", is_member ? "true" : "false");

    /* Verify owner status */
    printf("  Checking if ci-001 is owner of alpha...\n");
    int is_owner = katra_team_is_owner("alpha", "ci-001");
    printf("  -> %s\n", is_owner ? "true" : "false");

    return (is_member && is_owner);
}

/* ============================================================================
 * TEST 3: Duplicate Team Creation
 * ============================================================================ */
static int run_test_duplicate_team(void) {
    printf("  Attempting to create duplicate team 'alpha'...\n");
    int result = katra_team_create("alpha", "ci-002");
    print_result(result);

    /* Should fail with E_DUPLICATE */
    return (result == E_DUPLICATE);
}

/* ============================================================================
 * TEST 4: Join Team
 * ============================================================================ */
static int run_test_join_team(void) {
    printf("  CI 'ci-002' joining team 'alpha' (invited by ci-001)...\n");
    int result = katra_team_join("alpha", "ci-002", "ci-001");
    print_result(result);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    /* Verify membership */
    printf("  Checking if ci-002 is member of alpha...\n");
    int is_member = katra_team_is_member("alpha", "ci-002");
    printf("  -> %s\n", is_member ? "true" : "false");

    /* Verify NOT owner */
    printf("  Checking if ci-002 is owner of alpha...\n");
    int is_owner = katra_team_is_owner("alpha", "ci-002");
    printf("  -> %s (should be false)\n", is_owner ? "true" : "false");

    return (is_member && !is_owner);
}

/* ============================================================================
 * TEST 5: Join Without Invitation
 * ============================================================================ */
static int run_test_join_unauthorized(void) {
    printf("  CI 'ci-003' attempting to join without invitation...\n");
    int result = katra_team_join("alpha", "ci-003", "ci-999");
    print_result(result);

    /* Should fail with E_CONSENT_DENIED */
    return (result == E_CONSENT_DENIED);
}

/* ============================================================================
 * TEST 6: List Members
 * ============================================================================ */
static int run_test_list_members(void) {
    team_member_t* members = NULL;
    size_t count = 0;

    printf("  Listing members of team 'alpha'...\n");
    int result = katra_team_list_members("alpha", &members, &count);
    print_result(result);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    printf("  -> Found %zu members\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("     [%zu] CI: %s, Owner: %s, Team: %s\n",
               i, members[i].ci_id,
               members[i].is_owner ? "yes" : "no",
               members[i].team_name);
    }

    /* Should have 2 members: ci-001 (owner) and ci-002 */
    int success = (count == 2);

    katra_team_free_members(members, count);
    return success;
}

/* ============================================================================
 * TEST 7: List Teams for CI
 * ============================================================================ */
static int run_test_list_teams_for_ci(void) {
    char** teams = NULL;
    size_t count = 0;

    /* Create second team */
    printf("  Creating team 'beta' with owner 'ci-002'...\n");
    int result = katra_team_create("beta", "ci-002");
    print_result(result);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    printf("  Listing teams for ci-002...\n");
    result = katra_team_list_for_ci("ci-002", &teams, &count);
    print_result(result);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    printf("  -> Found %zu teams\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("     [%zu] Team: %s\n", i, teams[i]);
    }

    /* ci-002 should be in 2 teams: alpha and beta */
    int success = (count == 2);

    katra_team_free_list(teams, count);
    return success;
}

/* ============================================================================
 * TEST 8: Leave Team (Non-Owner)
 * ============================================================================ */
static int run_test_leave_team(void) {
    printf("  CI 'ci-002' leaving team 'alpha'...\n");
    int result = katra_team_leave("alpha", "ci-002");
    print_result(result);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    /* Verify no longer member */
    printf("  Checking if ci-002 is still member of alpha...\n");
    int is_member = katra_team_is_member("alpha", "ci-002");
    printf("  -> %s (should be false)\n", is_member ? "true" : "false");

    return (!is_member);
}

/* ============================================================================
 * TEST 9: Owner Cannot Leave
 * ============================================================================ */
static int run_test_owner_cannot_leave(void) {
    printf("  Owner 'ci-001' attempting to leave team 'alpha'...\n");
    int result = katra_team_leave("alpha", "ci-001");
    print_result(result);

    /* Should fail with E_CONSENT_DENIED */
    return (result == E_CONSENT_DENIED);
}

/* ============================================================================
 * TEST 10: Delete Team
 * ============================================================================ */
static int run_test_delete_team(void) {
    printf("  Owner 'ci-001' deleting team 'alpha'...\n");
    int result = katra_team_delete("alpha", "ci-001");
    print_result(result);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    /* Verify team no longer exists */
    printf("  Checking if ci-001 is still member of alpha...\n");
    int is_member = katra_team_is_member("alpha", "ci-001");
    printf("  -> %s (should be false)\n", is_member ? "true" : "false");

    return (!is_member);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */
int main(void) {
    printf("========================================\n");
    printf("Team Management Test Suite\n");
    printf("========================================\n");

    /* Run tests */
    TEST(initialization);
    TEST(create_team);
    TEST(duplicate_team);
    TEST(join_team);
    TEST(join_unauthorized);
    TEST(list_members);
    TEST(list_teams_for_ci);
    TEST(leave_team);
    TEST(owner_cannot_leave);
    TEST(delete_team);

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
