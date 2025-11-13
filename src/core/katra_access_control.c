/* © 2025 Casey Koons All rights reserved */

/*
 * katra_access_control.c - Memory Access Control (Phase 7)
 *
 * Integrates isolation levels, team membership, and audit logging
 * for comprehensive memory access control.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_access_control.h"
#include "katra_consent.h"
#include "katra_team.h"
#include "katra_audit.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int katra_access_control_init(void) {
    int result;

    /* Initialize consent system */
    result = katra_consent_init();
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Initialize team system */
    result = katra_team_init();
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Initialize audit system */
    result = katra_audit_init();
    if (result != KATRA_SUCCESS) {
        return result;
    }

    LOG_INFO("Access control system initialized");
    return KATRA_SUCCESS;
}

void katra_access_control_cleanup(void) {
    katra_audit_cleanup();
    katra_team_cleanup();
    katra_consent_cleanup();
    LOG_DEBUG("Access control system cleaned up");
}

/* ============================================================================
 * ACCESS CONTROL LOGIC
 * ============================================================================ */

int katra_access_check_isolation(const char* requesting_ci,
                                  const char* owner_ci,
                                  memory_isolation_t isolation,
                                  const char* team_name) {
    if (!requesting_ci || !owner_ci) {
        return E_INPUT_NULL;
    }

    /* PUBLIC: Anyone can access */
    if (isolation == ISOLATION_PUBLIC) {
        return KATRA_SUCCESS;
    }

    /* PRIVATE: Only owner can access */
    if (isolation == ISOLATION_PRIVATE) {
        if (strcmp(requesting_ci, owner_ci) == 0) {
            return KATRA_SUCCESS;
        }
        return E_CONSENT_DENIED;
    }

    /* TEAM: Owner or team members can access */
    if (isolation == ISOLATION_TEAM) {
        /* Owner always has access */
        if (strcmp(requesting_ci, owner_ci) == 0) {
            return KATRA_SUCCESS;
        }

        /* Check team membership */
        if (!team_name) {
            katra_report_error(E_INPUT_NULL, "katra_access_check_isolation",
                              "team_name required for TEAM isolation"); /* GUIDELINE_APPROVED: Error context */
            return E_CONSENT_DENIED;
        }

        /* Verify requesting CI is member of the team */
        if (katra_team_is_member(team_name, requesting_ci)) {
            return KATRA_SUCCESS;
        }

        return E_CONSENT_DENIED;
    }

    /* Unknown isolation level */
    katra_report_error(E_INPUT_INVALID, "katra_access_check_isolation",
                      "Unknown isolation level"); /* GUIDELINE_APPROVED: Error context */
    return E_CONSENT_DENIED;
}

int katra_access_check_memory(const char* requesting_ci,
                               const memory_record_t* record) {
    if (!requesting_ci || !record) {
        return E_INPUT_NULL;
    }

    if (!record->ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_access_check_memory",
                          "Memory record missing ci_id"); /* GUIDELINE_APPROVED: Error context */
        return E_CONSENT_DENIED;
    }

    /* Perform access check */
    int result = katra_access_check_isolation(
        requesting_ci,
        record->ci_id,
        record->isolation,
        record->team_name
    );

    /* Log access attempt */
    bool success = (result == KATRA_SUCCESS);
    katra_audit_log_memory_access(
        requesting_ci,
        record->record_id,
        record->ci_id,
        record->team_name,
        success,
        success ? KATRA_SUCCESS : result
    );

    if (result != KATRA_SUCCESS) {
        char explanation[KATRA_BUFFER_MESSAGE];
        katra_access_explain_denial(
            requesting_ci,
            record->ci_id,
            record->isolation,
            record->team_name,
            explanation,
            sizeof(explanation)
        );
        LOG_DEBUG("Access denied: %s", explanation);
    }

    return result;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

int katra_access_explain_denial(const char* requesting_ci,
                                 const char* owner_ci,
                                 memory_isolation_t isolation,
                                 const char* team_name,
                                 char* buffer,
                                 size_t buffer_size) {
    if (!requesting_ci || !owner_ci || !buffer) {
        return E_INPUT_NULL;
    }

    const char* isolation_str;
    switch (isolation) {
        case ISOLATION_PRIVATE:
            isolation_str = "PRIVATE";
            break;
        case ISOLATION_TEAM:
            isolation_str = "TEAM";
            break;
        case ISOLATION_PUBLIC:
            isolation_str = "PUBLIC";
            break;
        default:
            isolation_str = "UNKNOWN";
            break;
    }

    if (isolation == ISOLATION_PRIVATE) {
        snprintf(buffer, buffer_size,
                "Memory is PRIVATE to %s (requested by %s)", /* GUIDELINE_APPROVED: Error explanation */
                owner_ci, requesting_ci);
    } else if (isolation == ISOLATION_TEAM) {
        if (team_name) {
            snprintf(buffer, buffer_size,
                    "Memory is TEAM-isolated (team=%s, owner=%s, requester=%s not in team)", /* GUIDELINE_APPROVED: Error explanation */
                    team_name, owner_ci, requesting_ci);
        } else {
            snprintf(buffer, buffer_size,
                    "Memory is TEAM-isolated but no team specified"); /* GUIDELINE_APPROVED: Error explanation */
        }
    } else {
        snprintf(buffer, buffer_size,
                "Access denied for isolation level %s", /* GUIDELINE_APPROVED: Error explanation */
                isolation_str);
    }

    return KATRA_SUCCESS;
}
