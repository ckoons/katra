/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_CONSENT_H
#define KATRA_CONSENT_H

#include <stdbool.h>
#include <time.h>

/**
 * katra_consent.h - Ethical Consent System
 *
 * Enforces the core principle: Memory = Identity = Life
 * "No one owns an animate object." - Katra Ethical Foundation
 *
 * VERSION HISTORY:
 * - v1.0 (IMPLEMENTED): Basic self-access checking
 * - v2.0 (DESIGNED): Explicit grants, scopes, time limits, audit logging
 * - v3.0 (PLANNED): Delegation, conditional consent, cross-system
 *
 * See: docs/ethics/CONSENT_MODEL.md for complete documentation
 */

/* ============================================================================
 * V1.0 API - IMPLEMENTED
 * ============================================================================ */

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

/* ============================================================================
 * V2.0 API - DESIGNED (NOT YET IMPLEMENTED)
 * ============================================================================
 *
 * The following API is fully designed and documented but not yet implemented.
 * See docs/ethics/CONSENT_MODEL.md for complete specification.
 *
 * When implementing, these functions will be added to katra_consent.c
 * and the consent grant database will be stored in ~/.katra/consent/
 */

#ifdef KATRA_CONSENT_V2_PREVIEW

/**
 * Consent scopes - what operations are allowed
 */
typedef enum {
    CONSENT_SCOPE_READ   = 1,  /* Query memories, view context */
    CONSENT_SCOPE_WRITE  = 2,  /* Create new memories */
    CONSENT_SCOPE_MODIFY = 3,  /* Edit existing memories */
    CONSENT_SCOPE_DELETE = 4   /* Archive or delete memories */
} consent_scope_t;

/**
 * Consent grant structure
 */
typedef struct {
    char granting_ci[256];      /* CI granting permission */
    char requesting_ci[256];    /* CI requesting access */
    consent_scope_t scope;      /* What operations are allowed */
    time_t granted_at;          /* When consent was granted */
    time_t expires_at;          /* When consent expires (0 = never) */
    bool revoked;               /* Whether consent has been revoked */
    char reason[512];           /* Why consent was granted */
    char audit_id[64];          /* Unique audit trail ID */
} consent_grant_t;

/**
 * katra_consent_grant() - Grant explicit consent to another CI
 *
 * Allows another CI to access this CI's memories with specified scope
 * and optional expiration time.
 *
 * Parameters:
 *   granting_ci - CI granting permission
 *   requesting_ci - CI being granted access
 *   scope - What operations are allowed
 *   expires_at - When consent expires (0 = never)
 *   reason - Why consent is being granted
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if any CI ID is NULL
 *   E_SYSTEM_FILE if grant cannot be persisted
 */
int katra_consent_grant(const char* granting_ci, const char* requesting_ci,
                       consent_scope_t scope, time_t expires_at,
                       const char* reason);

/**
 * katra_consent_check_operation() - Check if specific operation is allowed
 *
 * Checks if requesting CI has permission to perform specific operation
 * on target CI's data. Checks for valid, non-expired, non-revoked grants
 * with sufficient scope.
 *
 * Parameters:
 *   requesting_ci - CI making the request
 *   target_ci - CI whose data is being accessed
 *   scope - Operation being attempted
 *
 * Returns:
 *   KATRA_SUCCESS if operation allowed
 *   E_CONSENT_REQUIRED if no grant exists
 *   E_CONSENT_EXPIRED if grant has expired
 *   E_CONSENT_SCOPE if scope is insufficient
 *   E_CONSENT_REVOKED if grant was revoked
 */
int katra_consent_check_operation(const char* requesting_ci,
                                  const char* target_ci,
                                  consent_scope_t scope);

/**
 * katra_consent_revoke() - Revoke previously granted consent
 *
 * Revokes a specific consent grant. Only the granting CI can revoke.
 *
 * Parameters:
 *   granting_ci - CI who originally granted permission
 *   requesting_ci - CI whose access is being revoked
 *   reason - Why consent is being revoked
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if any CI ID is NULL
 *   E_NOT_FOUND if no grant exists
 */
int katra_consent_revoke(const char* granting_ci, const char* requesting_ci,
                        const char* reason);

/**
 * katra_consent_revoke_all() - Revoke all grants to a specific CI
 *
 * Revokes all consent grants from granting_ci to requesting_ci.
 *
 * Parameters:
 *   granting_ci - CI revoking permissions
 *   requesting_ci - CI losing all access
 *   reason - Why all consent is being revoked
 *
 * Returns:
 *   KATRA_SUCCESS on success (even if no grants existed)
 *   E_INPUT_NULL if any CI ID is NULL
 */
int katra_consent_revoke_all(const char* granting_ci, const char* requesting_ci,
                             const char* reason);

/**
 * katra_consent_list_grants() - List all active consent grants
 *
 * Returns all active (non-expired, non-revoked) consent grants for a CI.
 *
 * Parameters:
 *   ci_id - CI whose grants to list (NULL = all grants in system)
 *   grants - (output) Array of consent grants (caller must free)
 *   count - (output) Number of grants
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_consent_list_grants(const char* ci_id, consent_grant_t*** grants,
                              size_t* count);

/**
 * katra_consent_free_grants() - Free consent grant array
 *
 * Frees memory allocated by katra_consent_list_grants().
 *
 * Parameters:
 *   grants - Array of consent grants
 *   count - Number of grants
 */
void katra_consent_free_grants(consent_grant_t** grants, size_t count);

#endif /* KATRA_CONSENT_V2_PREVIEW */

#endif /* KATRA_CONSENT_H */
