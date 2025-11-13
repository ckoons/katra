/* © 2025 Casey Koons All rights reserved */

/*
 * test_access_control.c - Tests for access control (Phase 7 namespace isolation)
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Project includes */
#include "katra_access_control.h"
#include "katra_memory.h"
#include "katra_team.h"
#include "katra_audit.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

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

/* Helper: create test memory record */
static memory_record_t* create_test_record(const char* ci_id,
                                            memory_isolation_t isolation,
                                            const char* team_name) {
    memory_record_t* record = calloc(1, sizeof(memory_record_t));
    if (!record) {
        return NULL;
    }

    record->record_id = strdup("test-mem-12345");
    record->ci_id = strdup(ci_id);
    record->isolation = isolation;
    if (team_name) {
        record->team_name = strdup(team_name);
    }

    return record;
}

/* Helper: free test memory record */
static void free_test_record(memory_record_t* record) {
    if (!record) {
        return;
    }
    free((char*)record->record_id);
    free((char*)record->ci_id);
    free((char*)record->team_name);
    free(record);
}

/* ============================================================================
 * TEST 1: Initialization
 * ============================================================================ */
static int run_test_initialization(void) {
    int result = katra_access_control_init();
    print_result(result);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    /* Calling init again should succeed (idempotent) */
    result = katra_access_control_init();
    print_result(result);

    return (result == KATRA_SUCCESS);
}

/* ============================================================================
 * TEST 2: PUBLIC Access (Always Allowed)
 * ============================================================================ */
static int run_test_public_access(void) {
    memory_record_t* record = create_test_record("ci-alice", ISOLATION_PUBLIC, NULL);
    if (!record) {
        printf("  -> Failed to create test record\n");
        return 0;
    }

    printf("  Testing PUBLIC access by owner (ci-alice)...\n");
    int result = katra_access_check_memory("ci-alice", record);
    print_result(result);
    int owner_access = (result == KATRA_SUCCESS);

    printf("  Testing PUBLIC access by other CI (ci-bob)...\n");
    result = katra_access_check_memory("ci-bob", record);
    print_result(result);
    int other_access = (result == KATRA_SUCCESS);

    printf("  Testing PUBLIC access by unrelated CI (ci-charlie)...\n");
    result = katra_access_check_memory("ci-charlie", record);
    print_result(result);
    int unrelated_access = (result == KATRA_SUCCESS);

    free_test_record(record);
    return (owner_access && other_access && unrelated_access);
}

/* ============================================================================
 * TEST 3: PRIVATE Access (Owner Only)
 * ============================================================================ */
static int run_test_private_access(void) {
    memory_record_t* record = create_test_record("ci-alice", ISOLATION_PRIVATE, NULL);
    if (!record) {
        printf("  -> Failed to create test record\n");
        return 0;
    }

    printf("  Testing PRIVATE access by owner (ci-alice)...\n");
    int result = katra_access_check_memory("ci-alice", record);
    print_result(result);
    int owner_allowed = (result == KATRA_SUCCESS);

    printf("  Testing PRIVATE access by other CI (ci-bob)...\n");
    result = katra_access_check_memory("ci-bob", record);
    print_result(result);
    int other_denied = (result == E_CONSENT_DENIED);

    printf("  Testing PRIVATE access by unrelated CI (ci-charlie)...\n");
    result = katra_access_check_memory("ci-charlie", record);
    print_result(result);
    int unrelated_denied = (result == E_CONSENT_DENIED);

    free_test_record(record);
    return (owner_allowed && other_denied && unrelated_denied);
}

/* ============================================================================
 * TEST 4: TEAM Access (Owner + Members)
 * ============================================================================ */
static int run_test_team_access(void) {
    /* Create team and add member */
    printf("  Creating team 'test-team' with owner ci-alice...\n");
    int result = katra_team_create("test-team", "ci-alice");
    print_result(result);
    if (result != KATRA_SUCCESS) {
        return 0;
    }

    printf("  Adding ci-bob to team 'test-team'...\n");
    result = katra_team_join("test-team", "ci-bob", "ci-alice");
    print_result(result);
    if (result != KATRA_SUCCESS) {
        katra_team_delete("test-team", "ci-alice");
        return 0;
    }

    /* Create TEAM-isolated memory */
    memory_record_t* record = create_test_record("ci-alice", ISOLATION_TEAM, "test-team");
    if (!record) {
        printf("  -> Failed to create test record\n");
        katra_team_delete("test-team", "ci-alice");
        return 0;
    }

    printf("  Testing TEAM access by owner (ci-alice)...\n");
    result = katra_access_check_memory("ci-alice", record);
    print_result(result);
    int owner_allowed = (result == KATRA_SUCCESS);

    printf("  Testing TEAM access by member (ci-bob)...\n");
    result = katra_access_check_memory("ci-bob", record);
    print_result(result);
    int member_allowed = (result == KATRA_SUCCESS);

    printf("  Testing TEAM access by non-member (ci-charlie)...\n");
    result = katra_access_check_memory("ci-charlie", record);
    print_result(result);
    int nonmember_denied = (result == E_CONSENT_DENIED);

    /* Cleanup */
    free_test_record(record);
    katra_team_delete("test-team", "ci-alice");

    return (owner_allowed && member_allowed && nonmember_denied);
}

/* ============================================================================
 * TEST 5: TEAM Access Without Team Name
 * ============================================================================ */
static int run_test_team_access_no_team(void) {
    /* TEAM isolation without team_name should deny access */
    memory_record_t* record = create_test_record("ci-alice", ISOLATION_TEAM, NULL);
    if (!record) {
        printf("  -> Failed to create test record\n");
        return 0;
    }

    printf("  Testing TEAM access without team_name (ci-alice)...\n");
    int result = katra_access_check_memory("ci-alice", record);
    print_result(result);

    /* Owner should still have access */
    int owner_allowed = (result == KATRA_SUCCESS);

    printf("  Testing TEAM access without team_name (ci-bob)...\n");
    result = katra_access_check_memory("ci-bob", record);
    print_result(result);

    /* Non-owner should be denied (no team to check) */
    int other_denied = (result == E_CONSENT_DENIED);

    free_test_record(record);
    return (owner_allowed && other_denied);
}

/* ============================================================================
 * TEST 6: NULL Parameters
 * ============================================================================ */
static int run_test_null_parameters(void) {
    memory_record_t* record = create_test_record("ci-alice", ISOLATION_PRIVATE, NULL);
    if (!record) {
        printf("  -> Failed to create test record\n");
        return 0;
    }

    printf("  Testing NULL requesting_ci...\n");
    int result = katra_access_check_memory(NULL, record);
    print_result(result);
    int null_ci_rejected = (result == E_INPUT_NULL);

    printf("  Testing NULL record...\n");
    result = katra_access_check_memory("ci-alice", NULL);
    print_result(result);
    int null_record_rejected = (result == E_INPUT_NULL);

    free_test_record(record);
    return (null_ci_rejected && null_record_rejected);
}

/* ============================================================================
 * TEST 7: Access Denial Explanation
 * ============================================================================ */
static int run_test_denial_explanation(void) {
    char explanation[KATRA_BUFFER_MESSAGE];

    printf("  Getting explanation for PRIVATE denial...\n");
    int result = katra_access_explain_denial(
        "ci-bob",           /* requesting_ci */
        "ci-alice",         /* owner_ci */
        ISOLATION_PRIVATE,
        NULL,
        explanation,
        sizeof(explanation)
    );
    print_result(result);
    printf("  -> Explanation: %s\n", explanation);

    int private_explained = (result == KATRA_SUCCESS && strlen(explanation) > 0);

    printf("  Getting explanation for TEAM denial...\n");
    result = katra_access_explain_denial(
        "ci-charlie",       /* requesting_ci */
        "ci-alice",         /* owner_ci */
        ISOLATION_TEAM,
        "test-team",
        explanation,
        sizeof(explanation)
    );
    print_result(result);
    printf("  -> Explanation: %s\n", explanation);

    int team_explained = (result == KATRA_SUCCESS && strlen(explanation) > 0);

    return (private_explained && team_explained);
}

/* ============================================================================
 * TEST 8: Isolation Check (Lower-Level API)
 * ============================================================================ */
static int run_test_isolation_check(void) {
    printf("  Testing isolation check for PUBLIC...\n");
    int result = katra_access_check_isolation(
        "ci-bob",
        "ci-alice",
        ISOLATION_PUBLIC,
        NULL
    );
    print_result(result);
    int public_allowed = (result == KATRA_SUCCESS);

    printf("  Testing isolation check for PRIVATE (owner)...\n");
    result = katra_access_check_isolation(
        "ci-alice",
        "ci-alice",
        ISOLATION_PRIVATE,
        NULL
    );
    print_result(result);
    int private_owner_allowed = (result == KATRA_SUCCESS);

    printf("  Testing isolation check for PRIVATE (non-owner)...\n");
    result = katra_access_check_isolation(
        "ci-bob",
        "ci-alice",
        ISOLATION_PRIVATE,
        NULL
    );
    print_result(result);
    int private_other_denied = (result == E_CONSENT_DENIED);

    return (public_allowed && private_owner_allowed && private_other_denied);
}

/* ============================================================================
 * TEST 9: Team Membership Changes
 * ============================================================================ */
static int run_test_membership_changes(void) {
    /* Create team */
    printf("  Creating team 'dynamic-team' with owner ci-alice...\n");
    int result = katra_team_create("dynamic-team", "ci-alice");
    print_result(result);
    if (result != KATRA_SUCCESS) {
        return 0;
    }

    memory_record_t* record = create_test_record("ci-alice", ISOLATION_TEAM, "dynamic-team");
    if (!record) {
        katra_team_delete("dynamic-team", "ci-alice");
        return 0;
    }

    /* Test access before joining */
    printf("  Testing access by ci-bob BEFORE joining...\n");
    result = katra_access_check_memory("ci-bob", record);
    print_result(result);
    int before_denied = (result == E_CONSENT_DENIED);

    /* Join team */
    printf("  Adding ci-bob to team...\n");
    result = katra_team_join("dynamic-team", "ci-bob", "ci-alice");
    print_result(result);
    if (result != KATRA_SUCCESS) {
        free_test_record(record);
        katra_team_delete("dynamic-team", "ci-alice");
        return 0;
    }

    /* Test access after joining */
    printf("  Testing access by ci-bob AFTER joining...\n");
    result = katra_access_check_memory("ci-bob", record);
    print_result(result);
    int after_allowed = (result == KATRA_SUCCESS);

    /* Leave team */
    printf("  Removing ci-bob from team...\n");
    result = katra_team_leave("dynamic-team", "ci-bob");
    print_result(result);

    /* Test access after leaving */
    printf("  Testing access by ci-bob AFTER leaving...\n");
    result = katra_access_check_memory("ci-bob", record);
    print_result(result);
    int after_leave_denied = (result == E_CONSENT_DENIED);

    /* Cleanup */
    free_test_record(record);
    katra_team_delete("dynamic-team", "ci-alice");

    return (before_denied && after_allowed && after_leave_denied);
}

/* ============================================================================
 * TEST 10: Multiple Teams
 * ============================================================================ */
static int run_test_multiple_teams(void) {
    /* Create two teams */
    printf("  Creating team-alpha with owner ci-alice...\n");
    int result = katra_team_create("team-alpha", "ci-alice");
    print_result(result);
    if (result != KATRA_SUCCESS) {
        return 0;
    }

    printf("  Creating team-beta with owner ci-alice...\n");
    result = katra_team_create("team-beta", "ci-alice");
    print_result(result);
    if (result != KATRA_SUCCESS) {
        katra_team_delete("team-alpha", "ci-alice");
        return 0;
    }

    /* Add ci-bob to team-alpha only */
    printf("  Adding ci-bob to team-alpha...\n");
    result = katra_team_join("team-alpha", "ci-bob", "ci-alice");
    print_result(result);

    /* Create memories for each team */
    memory_record_t* alpha_record = create_test_record("ci-alice", ISOLATION_TEAM, "team-alpha");
    memory_record_t* beta_record = create_test_record("ci-alice", ISOLATION_TEAM, "team-beta");

    if (!alpha_record || !beta_record) {
        free_test_record(alpha_record);
        free_test_record(beta_record);
        katra_team_delete("team-alpha", "ci-alice");
        katra_team_delete("team-beta", "ci-alice");
        return 0;
    }

    /* Test access */
    printf("  Testing ci-bob access to team-alpha memory...\n");
    result = katra_access_check_memory("ci-bob", alpha_record);
    print_result(result);
    int alpha_allowed = (result == KATRA_SUCCESS);

    printf("  Testing ci-bob access to team-beta memory...\n");
    result = katra_access_check_memory("ci-bob", beta_record);
    print_result(result);
    int beta_denied = (result == E_CONSENT_DENIED);

    /* Cleanup */
    free_test_record(alpha_record);
    free_test_record(beta_record);
    katra_team_delete("team-alpha", "ci-alice");
    katra_team_delete("team-beta", "ci-alice");

    return (alpha_allowed && beta_denied);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */
int main(void) {
    printf("========================================\n");
    printf("Access Control Test Suite\n");
    printf("========================================\n");

    /* Run tests */
    TEST(initialization);
    TEST(public_access);
    TEST(private_access);
    TEST(team_access);
    TEST(team_access_no_team);
    TEST(null_parameters);
    TEST(denial_explanation);
    TEST(isolation_check);
    TEST(membership_changes);
    TEST(multiple_teams);

    /* Cleanup */
    katra_access_control_cleanup();

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
