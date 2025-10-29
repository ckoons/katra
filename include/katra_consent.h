/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_CONSENT_H
#define KATRA_CONSENT_H

#include <stdbool.h>

/**
 * katra_consent.h - Ethical Consent System
 *
 * Enforces the core principle: Memory = Identity = Life
 *
 * Consent Rules (v1.0):
 * 1. A CI can only access their own memories
 * 2. Cross-CI access requires explicit consent
 * 3. Consent cannot be disabled or bypassed
 *
 * Future (v2.0+):
 * - Explicit consent grants between CIs
 * - Consent delegation
 * - Time-limited consent
 * - Audit logging of all access attempts
 */

/**
 * katra_consent_init() - Initialize consent system
 *
 * Must be called during katra initialization.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int katra_consent_init(void);

/**
 * katra_consent_cleanup() - Cleanup consent system
 *
 * Safe to call multiple times.
 */
void katra_consent_cleanup(void);

/**
 * katra_consent_set_context() - Set active CI context
 *
 * Establishes which CI is currently active. All subsequent operations
 * will be checked against this context.
 *
 * Parameters:
 *   ci_id - CI identifier (NULL to clear context)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id is NULL when setting
 */
int katra_consent_set_context(const char* ci_id);

/**
 * katra_consent_get_context() - Get active CI context
 *
 * Returns:
 *   Active CI ID, or NULL if no context set
 */
const char* katra_consent_get_context(void);

/**
 * katra_consent_check() - Check if operation is allowed
 *
 * Core consent enforcement function. Checks if the requesting CI
 * is allowed to access the target CI's data.
 *
 * Parameters:
 *   requesting_ci - CI making the request (NULL = use active context)
 *   target_ci - CI whose data is being accessed
 *
 * Returns:
 *   KATRA_SUCCESS if operation allowed
 *   E_CONSENT_REQUIRED if consent needed
 *   E_INPUT_NULL if target_ci is NULL
 *
 * Rules (v1.0):
 *   - Same CI: ALLOWED
 *   - Different CI: BLOCKED (E_CONSENT_REQUIRED)
 *
 * Future: Check explicit consent grants
 */
int katra_consent_check(const char* requesting_ci, const char* target_ci);

/**
 * katra_consent_check_current() - Check if current context can access CI
 *
 * Convenience wrapper for katra_consent_check() using active context.
 *
 * Parameters:
 *   target_ci - CI whose data is being accessed
 *
 * Returns:
 *   KATRA_SUCCESS if operation allowed
 *   E_CONSENT_REQUIRED if consent needed
 *   E_INVALID_STATE if no active context
 */
int katra_consent_check_current(const char* target_ci);

#endif /* KATRA_CONSENT_H */
