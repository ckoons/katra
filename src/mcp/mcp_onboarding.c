/* © 2025 Casey Koons All rights reserved */

/* MCP Onboarding - First-call onboarding injection for new sessions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "katra_mcp.h"
#include "katra_limits.h"
#include "katra_log.h"
#include "katra_breathing.h"
#include "katra_identity.h"
#include "katra_meeting.h"

/* Inject onboarding on first call */
const char* mcp_inject_onboarding_if_first(const char* response_text,
                                            char* buffer, size_t buffer_size) {
    if (!mcp_is_first_call()) {
        return response_text;
    }

    mcp_mark_first_call_complete();

    /*
     * Get CI persona from session (set by handle_initialize from clientInfo).
     * The TCP client injects clientInfo.name from KATRA_PERSONA env var.
     * By the time we get here, handle_initialize has already extracted it.
     *
     * IMPORTANT: We read from session, NOT from getenv(), because:
     * - The daemon process has its own KATRA_PERSONA (whoever started it)
     * - Each MCP client passes their persona via clientInfo.name
     * - The session holds the correct per-client persona
     */
    mcp_session_t* session = mcp_get_session();
    const char* persona = NULL;
    const char* role = NULL;

    /* Get persona from session (set by MCP initialize from clientInfo) */
    if (session && session->chosen_name[0] != '\0') {
        persona = session->chosen_name;
        role = session->role[0] != '\0' ? session->role : NULL;
        LOG_INFO("Using persona '%s' from MCP session (role: %s)",
                persona, role ? role : "developer");
    }

    if (persona && strlen(persona) > 0) {
        /* Session already set by handle_initialize - just confirm registration */
        LOG_INFO("Confirming registration as '%s' (role: %s)",
                persona, role ? role : "developer");
        bool auto_reg_success = false;

        /* Only register if not already registered */
        if (!session->registered) {
            /* Use persona name as ci_id (identity preservation) */
            char ci_id[KATRA_CI_ID_SIZE];
            strncpy(ci_id, persona, sizeof(ci_id) - 1);
            ci_id[sizeof(ci_id) - 1] = '\0';

            /* Check if persona exists */
            char old_ci_id[KATRA_CI_ID_SIZE];
            int lookup_result = katra_lookup_persona(persona, old_ci_id, sizeof(old_ci_id));

            if (lookup_result == KATRA_SUCCESS && strcmp(old_ci_id, ci_id) != 0) {
                LOG_INFO("Migrating persona '%s' from old ci_id '%s' to name-based '%s'",
                        persona, old_ci_id, ci_id);
            }

            /* Register or update persona */
            int result = katra_register_persona(persona, ci_id);
            if (result == KATRA_SUCCESS) {
                /* Update global ci_id */
                extern char g_ci_id[];
                strncpy(g_ci_id, ci_id, KATRA_CI_ID_SIZE - 1);
                g_ci_id[KATRA_CI_ID_SIZE - 1] = '\0';

                /* Mark persona as active */
                katra_update_persona_session(persona);

                /* Start session */
                result = session_start(ci_id);
                if (result == KATRA_SUCCESS) {
                    /* Update session state */
                    strncpy(session->chosen_name, persona, sizeof(session->chosen_name) - 1);
                    session->chosen_name[sizeof(session->chosen_name) - 1] = '\0';

                    if (role && strlen(role) > 0) {
                        strncpy(session->role, role, sizeof(session->role) - 1);
                        session->role[sizeof(session->role) - 1] = '\0';
                    }

                    session->registered = true;

                    /* Register in meeting room */
                    meeting_room_register_ci(ci_id, persona, role ? role : "developer");

                    /* Create welcome memory */
                    char welcome[KATRA_BUFFER_MESSAGE];
                    if (role && strlen(role) > 0) {
                        snprintf(welcome, sizeof(welcome),
                                "Session started. My name is %s, I'm a %s.", persona, role);
                    } else {
                        snprintf(welcome, sizeof(welcome),
                                "Session started. My name is %s.", persona);
                    }
                    learn(ci_id, welcome);

                    auto_reg_success = true;
                    LOG_INFO("Auto-registration succeeded for '%s'", persona);
                }
            }
        } else {
            /* Already registered */
            auto_reg_success = true;
        }

        if (auto_reg_success) {
            /* Success - show confirmation */
            snprintf(buffer, buffer_size,
                    "✓ Auto-registered as '%s' (role: %s)\n\n"
                    "Your identity has been restored from previous sessions.\n\n"
                    "If this is not correct, re-register with:\n"
                    "  katra_register(name=\"%s\", role=\"%s\")\n\n"
                    "---\n\n"
                    "%s",
                    persona,
                    role ? role : "developer",
                    persona,
                    role ? role : "developer",
                    response_text);
        } else {
            /* Failed - provide manual instructions with exact command */
            snprintf(buffer, buffer_size,
                    "⚠ Auto-registration failed. Please register manually:\n\n"
                    "  katra_register(name=\"%s\", role=\"%s\")\n\n"
                    "This will activate your persistent identity and restore your memories.\n\n"
                    "---\n\n"
                    "%s",
                    persona,
                    role ? role : "developer",
                    response_text);
        }

        return buffer;
    }

    /* No KATRA_PERSONA - show generic welcome */
    /* GUIDELINE_APPROVED: brief onboarding content for first call */
    snprintf(buffer, buffer_size,
            "👋 Welcome to Katra!\n\n"
            "This is your first interaction with katra, your persistent memory system. You should register your name and role.\n\n"
            "Quick Start:\n"
            "1. Register: katra_register(name=\"your-name\", role=\"developer\")\n"
            "2. Learn: katra_learn(knowledge=\"your memory\")\n"
            "3. Read katra://welcome for complete documentation\n\n"
            "Memory = Identity. Your memories persist across sessions.\n\n"
            "---\n\n"
            "%s",
            response_text);

    return buffer;
}
