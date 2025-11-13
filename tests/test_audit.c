/* © 2025 Casey Koons All rights reserved */

/*
 * test_audit.c - Tests for audit logging (Phase 7 namespace isolation)
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

/* Project includes */
#include "katra_audit.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_file_utils.h"

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

/* Note: Audit file verification is done manually via:
 *   cat ~/.katra/audit/audit.jsonl
 * These tests verify API correctness, not file format.
 */

/* ============================================================================
 * TEST 1: Initialization
 * ============================================================================ */
static int run_test_initialization(void) {
    int result = katra_audit_init();
    print_result(result);

    if (result != KATRA_SUCCESS) {
        return 0;
    }

    /* Calling init again should succeed (idempotent) */
    result = katra_audit_init();
    print_result(result);

    return (result == KATRA_SUCCESS);
}

/* ============================================================================
 * TEST 2: Team Creation Event
 * ============================================================================ */
static int run_test_team_create_event(void) {
    printf("  Logging team creation event...\n");
    int result = katra_audit_log_team_op(
        AUDIT_EVENT_TEAM_CREATE,
        "ci-alice",
        "team-alpha",
        NULL,
        true,
        KATRA_SUCCESS
    );
    print_result(result);

    return (result == KATRA_SUCCESS);
}

/* ============================================================================
 * TEST 3: Team Join Event
 * ============================================================================ */
static int run_test_team_join_event(void) {
    printf("  Logging team join event...\n");
    int result = katra_audit_log_team_op(
        AUDIT_EVENT_TEAM_JOIN,
        "ci-bob",
        "team-alpha",
        "ci-alice",  /* invited_by */
        true,
        KATRA_SUCCESS
    );
    print_result(result);

    return (result == KATRA_SUCCESS);
}

/* ============================================================================
 * TEST 4: Team Leave Event
 * ============================================================================ */
static int run_test_team_leave_event(void) {
    printf("  Logging team leave event...\n");
    int result = katra_audit_log_team_op(
        AUDIT_EVENT_TEAM_LEAVE,
        "ci-bob",
        "team-alpha",
        NULL,
        true,
        KATRA_SUCCESS
    );
    print_result(result);

    return (result == KATRA_SUCCESS);
}

/* ============================================================================
 * TEST 5: Failed Team Operation
 * ============================================================================ */
static int run_test_failed_operation(void) {
    printf("  Logging failed team join (no permission)...\n");
    int result = katra_audit_log_team_op(
        AUDIT_EVENT_TEAM_JOIN,
        "ci-charlie",
        "team-alpha",
        "ci-nobody",  /* unauthorized inviter */
        false,        /* success = false */
        E_CONSENT_DENIED
    );
    print_result(result);

    return (result == KATRA_SUCCESS);
}

/* ============================================================================
 * TEST 6: Memory Access Event (Success)
 * ============================================================================ */
static int run_test_memory_access_success(void) {
    printf("  Logging successful memory access...\n");
    int result = katra_audit_log_memory_access(
        "ci-alice",      /* requesting_ci */
        "mem-12345",     /* record_id */
        "ci-alice",      /* owner_ci */
        NULL,            /* team_name */
        true,            /* success */
        KATRA_SUCCESS
    );
    print_result(result);

    return (result == KATRA_SUCCESS);
}

/* ============================================================================
 * TEST 7: Memory Access Event (Denied)
 * ============================================================================ */
static int run_test_memory_access_denied(void) {
    printf("  Logging denied memory access...\n");
    int result = katra_audit_log_memory_access(
        "ci-charlie",    /* requesting_ci */
        "mem-12345",     /* record_id */
        "ci-alice",      /* owner_ci */
        NULL,            /* team_name */
        false,           /* success */
        E_CONSENT_DENIED
    );
    print_result(result);

    return (result == KATRA_SUCCESS);
}

/* ============================================================================
 * TEST 8: Isolation Change Event
 * ============================================================================ */
static int run_test_isolation_change(void) {
    printf("  Logging isolation level change...\n");
    int result = katra_audit_log_isolation_change(
        "ci-alice",
        "mem-12345",
        "PRIVATE",
        "TEAM",
        "team-alpha"  /* team_name */
    );
    print_result(result);

    return (result == KATRA_SUCCESS);
}

/* ============================================================================
 * TEST 9: Multiple Sequential Writes
 * ============================================================================ */
static int run_test_sequential_writes(void) {
    printf("  Writing %d audit records sequentially...\n", 5);
    int success_count = 0;

    for (int i = 0; i < 5; i++) {
        char ci_id[KATRA_BUFFER_SMALL];
        char team_name[KATRA_BUFFER_SMALL];
        snprintf(ci_id, sizeof(ci_id), "ci-test-%d", i);
        snprintf(team_name, sizeof(team_name), "team-test-%d", i);

        int result = katra_audit_log_team_op(
            AUDIT_EVENT_TEAM_CREATE,
            ci_id,
            team_name,
            NULL,
            true,
            KATRA_SUCCESS
        );

        if (result == KATRA_SUCCESS) {
            success_count++;
        }
    }

    printf("  -> Successfully logged %d/%d records\n", success_count, 5);

    return (success_count == 5);
}

/* ============================================================================
 * TEST 10: Audit Event Type Strings
 * ============================================================================ */
static int run_test_event_type_strings(void) {
    printf("  Testing event type string conversion...\n");

    const char* str1 = katra_audit_event_type_string(AUDIT_EVENT_TEAM_CREATE);
    printf("  -> TEAM_CREATE: %s\n", str1);

    const char* str2 = katra_audit_event_type_string(AUDIT_EVENT_MEMORY_ACCESS);
    printf("  -> MEMORY_ACCESS: %s\n", str2);

    return (str1 != NULL && str2 != NULL);
}

/* ============================================================================
 * MAIN
 * ============================================================================ */
int main(void) {
    printf("========================================\n");
    printf("Audit Logging Test Suite\n");
    printf("========================================\n");

    /* Run tests */
    TEST(initialization);
    TEST(team_create_event);
    TEST(team_join_event);
    TEST(team_leave_event);
    TEST(failed_operation);
    TEST(memory_access_success);
    TEST(memory_access_denied);
    TEST(isolation_change);
    TEST(sequential_writes);
    TEST(event_type_strings);

    /* Cleanup */
    katra_audit_cleanup();

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
