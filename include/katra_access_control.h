/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_ACCESS_CONTROL_H
#define KATRA_ACCESS_CONTROL_H

#include <stdbool.h>
#include "katra_memory.h"

/**
 * katra_access_control.h - Memory Access Control (Phase 7)
 *
 * Integrates isolation levels, team membership, and consent
 * to provide comprehensive access control for memory records.
 *
 * Access Rules:
 * - PRIVATE: Only owning CI can access
 * - TEAM: Owning CI + team members can access
 * - PUBLIC: Anyone can access
 */

/**
 * katra_access_control_init() - Initialize access control system
 *
 * Initializes dependencies (consent, team, audit systems).
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error code on failure
 */
int katra_access_control_init(void);

/**
 * katra_access_control_cleanup() - Cleanup access control system
 *
 * Cleans up all subsystems.
 */
void katra_access_control_cleanup(void);

/**
 * katra_access_check_memory() - Check if CI can access memory record
 *
 * Comprehensive access control check combining:
 * - Isolation level (PRIVATE/TEAM/PUBLIC)
 * - Team membership (for TEAM isolation)
 * - Ownership (for PRIVATE isolation)
 *
 * Parameters:
 *   requesting_ci - CI requesting access
 *   record - Memory record to access
 *
 * Returns:
 *   KATRA_SUCCESS if access allowed
 *   E_CONSENT_DENIED if access denied
 *   E_INPUT_NULL if parameters are NULL
 *
 * Side effects:
 *   - Logs access attempt to audit log (success or failure)
 */
int katra_access_check_memory(const char* requesting_ci,
                               const memory_record_t* record);

/**
 * katra_access_check_isolation() - Check access based on isolation level
 *
 * Lower-level function that checks access based on isolation settings.
 *
 * Parameters:
 *   requesting_ci - CI requesting access
 *   owner_ci - CI that owns the memory
 *   isolation - Isolation level
 *   team_name - Team name (required for TEAM isolation, NULL otherwise)
 *
 * Returns:
 *   KATRA_SUCCESS if access allowed
 *   E_CONSENT_DENIED if access denied
 *   E_INPUT_NULL if required parameters are NULL
 */
int katra_access_check_isolation(const char* requesting_ci,
                                  const char* owner_ci,
                                  memory_isolation_t isolation,
                                  const char* team_name);

/**
 * katra_access_explain_denial() - Get human-readable denial reason
 *
 * Provides explanation for why access was denied.
 *
 * Parameters:
 *   requesting_ci - CI that was denied
 *   owner_ci - CI that owns the memory
 *   isolation - Isolation level
 *   team_name - Team name (if applicable)
 *   buffer - Output buffer for explanation
 *   buffer_size - Size of output buffer
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 */
int katra_access_explain_denial(const char* requesting_ci,
                                 const char* owner_ci,
                                 memory_isolation_t isolation,
                                 const char* team_name,
                                 char* buffer,
                                 size_t buffer_size);

#endif /* KATRA_ACCESS_CONTROL_H */
