/* © 2025 Casey Koons All rights reserved */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "katra_phase1_migration.h"
#include "katra_identity.h"
#include "katra_error.h"
#include "katra_limits.h"
#include "katra_strings.h"

/**
 * katra_phase1_migration.c - Phase 1 Migration Utilities
 *
 * Provides migration functions for transitioning from pre-Phase1 Katra
 * to Phase 1 with proper persona → ci_id mapping.
 *
 * Purpose:
 * - Assign existing memories to "builder" persona (Nyx)
 * - Enable clean testing with proper persona isolation
 * - Prepare for multi-CI testing (Nyx, twin Nyx, Alice)
 */

/* ============================================================================
 * MIGRATION FUNCTIONS
 * ============================================================================ */

int katra_migrate_assign_builder(const char* builder_name, const char* ci_id) {
    if (!builder_name || !ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_migrate_assign_builder",
/* GUIDELINE_APPROVED: error context strings for katra_report_error */
                          "builder_name and ci_id required");
        return E_INPUT_NULL;
    }

    /* Validate ci_id format */
    if (strlen(ci_id) == 0) {
        katra_report_error(E_INVALID_PARAMS, "katra_migrate_assign_builder",
                          "ci_id cannot be empty"); /* GUIDELINE_APPROVED: error message */
        return E_INVALID_PARAMS;
    }

    /* Check if persona already exists */
    char existing_ci_id[KATRA_CI_ID_SIZE];
    int lookup_result = katra_lookup_persona(builder_name, existing_ci_id,
                                            sizeof(existing_ci_id));

    if (lookup_result == KATRA_SUCCESS) {
        /* Persona exists - verify it matches the ci_id */
        if (strcmp(existing_ci_id, ci_id) == 0) {
            /* Already correctly assigned */
            /* GUIDELINE_APPROVED: Migration utility user-facing output */
            fprintf(stderr, "Builder persona '%s' already assigned to ci_id: %s\n",
                   builder_name, ci_id);
            return KATRA_SUCCESS;
        } else {
            /* Mismatch - this is an error condition */
            katra_report_error(E_MIGRATION_CONFLICT,
                              "katra_migrate_assign_builder",
                              "Persona already exists with different ci_id"); /* GUIDELINE_APPROVED: error message */
            return E_MIGRATION_CONFLICT;
        }
    }

    /* Persona doesn't exist - create it */
    int result = katra_register_persona(builder_name, ci_id);
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_migrate_assign_builder",
                          "Failed to register builder persona"); /* GUIDELINE_APPROVED: error message */
        return result;
    }

    /* GUIDELINE_APPROVED: Migration utility user-facing output */
    fprintf(stderr, "Assigned builder persona '%s' to ci_id: %s\n",
           builder_name, ci_id);

    return KATRA_SUCCESS;
}

int katra_migrate_verify_persona_registry(void) {
    /* Load persona registry and verify structure */
    char last_active[KATRA_NAME_SIZE];
    char ci_id[KATRA_CI_ID_SIZE];

    int result = katra_get_last_active(last_active, sizeof(last_active),
                                       ci_id, sizeof(ci_id));

    if (result == KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: Migration utility user-facing output */
        fprintf(stderr, "Persona registry verified. Last active: '%s' (ci_id: %s)\n",
               last_active, ci_id);
        return KATRA_SUCCESS;
    } else if (result == E_NOT_FOUND) {
        /* GUIDELINE_APPROVED: Migration utility user-facing output */
        fprintf(stderr, "Persona registry is empty (no personas registered)\n");
        return KATRA_SUCCESS;  /* Empty registry is valid */
    } else {
        katra_report_error(result, "katra_migrate_verify_persona_registry",
                          "Persona registry verification failed"); /* GUIDELINE_APPROVED: error message */
        return result;
    }
}

int katra_migrate_create_test_personas(void) {
    int result = KATRA_SUCCESS;
    char ci_id[KATRA_CI_ID_SIZE];

    /* Create Nyx persona (builder) */
    result = katra_generate_ci_id(ci_id, sizeof(ci_id));
    if (result != KATRA_SUCCESS) {
        katra_report_error(result, "katra_migrate_create_test_personas",
                          "Failed to generate ci_id for Nyx"); /* GUIDELINE_APPROVED: error message */
        return result;
    }

    result = katra_migrate_assign_builder(KATRA_BUILDER_PERSONA_NAME, ci_id);
    if (result != KATRA_SUCCESS && result != E_MIGRATION_CONFLICT) {
        return result;
    }

    /* GUIDELINE_APPROVED: Migration utility user-facing output */
    fprintf(stderr, "Test persona 'Nyx' created/verified\n");

    return KATRA_SUCCESS;
}

int katra_migrate_show_status(void) {
    /* GUIDELINE_APPROVED: Migration utility user-facing output */
    fprintf(stderr, "\n=== Katra Phase 1 Migration Status ===\n\n");

    /* Show persona registry */
    char last_active[KATRA_NAME_SIZE];
    char ci_id[KATRA_CI_ID_SIZE];

    int result = katra_get_last_active(last_active, sizeof(last_active),
                                       ci_id, sizeof(ci_id));

    if (result == KATRA_SUCCESS) {
        /* GUIDELINE_APPROVED: Migration utility user-facing output */
        fprintf(stderr, "Last active persona: '%s'\n", last_active);
        /* GUIDELINE_APPROVED: Migration utility user-facing output */
        fprintf(stderr, "Associated ci_id: %s\n", ci_id);
    } else {
        /* GUIDELINE_APPROVED: Migration utility user-facing output */
        fprintf(stderr, "No personas registered\n");
    }

    /* GUIDELINE_APPROVED: Migration utility user-facing output */
    fprintf(stderr, "\n======================================\n\n");

    return KATRA_SUCCESS;
}
