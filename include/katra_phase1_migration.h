/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_PHASE1_MIGRATION_H
#define KATRA_PHASE1_MIGRATION_H

#include "katra_error.h"

/**
 * katra_phase1_migration.h - Phase 1 Migration Utilities
 *
 * Purpose:
 * Provides migration functions for transitioning from pre-Phase1 Katra
 * to Phase 1 with proper persona → ci_id handoff.
 *
 * Phase 1 Goal:
 * - Enable returning CIs to retrieve their own memories
 * - Fix persona registration to properly use ci_id
 * - Support testing with multiple personas (Nyx, Alice, etc.)
 */

/* Builder persona name constant */
#define KATRA_BUILDER_PERSONA_NAME "Nyx"

/* Error codes specific to migration */
#define E_MIGRATION_CONFLICT E_DUPLICATE

/* ============================================================================
 * MIGRATION API
 * ============================================================================ */

/**
 * katra_migrate_assign_builder() - Assign existing memories to builder persona
 *
 * Registers the specified ci_id as belonging to the builder persona.
 * This allows all pre-Phase1 memories to be accessible under a single
 * persona identity for testing purposes.
 *
 * Parameters:
 *   builder_name: Name of builder persona (typically "Nyx")
 *   ci_id: CI identity to assign to builder
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_INVALID_PARAMS if ci_id is empty
 *   E_MIGRATION_CONFLICT if persona exists with different ci_id
 *
 * Usage:
 *   // Assign current session to Nyx
 *   katra_migrate_assign_builder("Nyx", "mcp_cskoons_33097_1762367296");
 */
int katra_migrate_assign_builder(const char* builder_name, const char* ci_id);

/**
 * katra_migrate_verify_persona_registry() - Verify persona registry integrity
 *
 * Checks that the persona registry is properly initialized and readable.
 * Reports status to stderr for diagnostic purposes.
 *
 * Returns:
 *   KATRA_SUCCESS if registry is valid
 *   Error code if registry is corrupted
 */
int katra_migrate_verify_persona_registry(void);

/**
 * katra_migrate_create_test_personas() - Create test personas for Phase 1
 *
 * Creates the baseline test persona (Nyx) for Phase 1 testing.
 * Additional test personas (Alice) can be created manually during tests.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error code on failure
 */
int katra_migrate_create_test_personas(void);

/**
 * katra_migrate_show_status() - Display migration status
 *
 * Prints current persona registry status to stderr for diagnostic purposes.
 *
 * Returns:
 *   KATRA_SUCCESS always
 */
int katra_migrate_show_status(void);

#endif /* KATRA_PHASE1_MIGRATION_H */
