/* © 2025 Casey Koons All rights reserved */

/*
 * katra_consent.c - Ethical Consent System Implementation
 *
 * Enforces the principle: Memory = Identity = Life
 * A CI's memories constitute their identity and cannot be accessed without consent.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Project includes */
#include "katra_consent.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Global state - tracks which CI is currently active */
static char g_active_ci[KATRA_BUFFER_MEDIUM] = {0};
static bool g_consent_initialized = false;

/* Initialize consent system */
int katra_consent_init(void) {
    if (g_consent_initialized) {
        LOG_DEBUG("Consent system already initialized");
        return KATRA_SUCCESS;
    }

    g_active_ci[0] = '\0';
    g_consent_initialized = true;

    LOG_INFO("Consent system initialized");
    return KATRA_SUCCESS;
}

/* Cleanup consent system */
void katra_consent_cleanup(void) {
    if (!g_consent_initialized) {
        return;
    }

    g_active_ci[0] = '\0';
    g_consent_initialized = false;

    LOG_DEBUG("Consent system cleaned up");
}

/* Set active CI context */
int katra_consent_set_context(const char* ci_id) {
    if (!ci_id) {
        /* NULL ci_id clears context */
        g_active_ci[0] = '\0';
        LOG_DEBUG("Consent context cleared");
        return KATRA_SUCCESS;
    }

    size_t len = strlen(ci_id);
    if (len >= sizeof(g_active_ci)) {
        katra_report_error(E_INPUT_RANGE, "katra_consent_set_context",
                          "CI ID too long (%zu >= %zu)", len, sizeof(g_active_ci));
        return E_INPUT_RANGE;
    }

    strncpy(g_active_ci, ci_id, sizeof(g_active_ci) - 1);
    g_active_ci[sizeof(g_active_ci) - 1] = '\0';

    LOG_DEBUG("Consent context set to: %s", ci_id);
    return KATRA_SUCCESS;
}

/* Get active CI context */
const char* katra_consent_get_context(void) {
    if (g_active_ci[0] == '\0') {
        return NULL;
    }
    return g_active_ci;
}

/* Check if operation is allowed */
int katra_consent_check(const char* requesting_ci, const char* target_ci) {
    if (!target_ci) {
        katra_report_error(E_INPUT_NULL, "katra_consent_check",
                          "target_ci is NULL");
        return E_INPUT_NULL;
    }

    /* If requesting_ci is NULL, use active context */
    const char* requester = requesting_ci;
    if (!requester) {
        requester = katra_consent_get_context();
        if (!requester) {
            katra_report_error(E_INVALID_STATE, "katra_consent_check",
                              "No active CI context and no requesting_ci provided");
            return E_INVALID_STATE;
        }
    }

    /* V1.0 Rule: Same CI = allowed, different CI = blocked */
    if (strcmp(requester, target_ci) == 0) {
        /* Same CI accessing own data - allowed */
        return KATRA_SUCCESS;
    }

    /* Different CI attempting cross-CI access - blocked */
    LOG_WARN("Consent violation: CI '%s' attempted to access CI '%s' data",
            requester, target_ci);
    katra_report_error(E_CONSENT_REQUIRED, "katra_consent_check",
                      "CI '%s' cannot access CI '%s' memories without consent",
                      requester, target_ci);
    return E_CONSENT_REQUIRED;
}

/* Check if current context can access target CI */
int katra_consent_check_current(const char* target_ci) {
    const char* current_ci = katra_consent_get_context();
    if (!current_ci) {
        katra_report_error(E_INVALID_STATE, "katra_consent_check_current",
                          "No active CI context set");
        return E_INVALID_STATE;
    }

    return katra_consent_check(current_ci, target_ci);
}
