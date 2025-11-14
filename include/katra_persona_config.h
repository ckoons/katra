/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_PERSONA_CONFIG_H
#define KATRA_PERSONA_CONFIG_H

/*
 * katra_persona_config.h - Per-persona configuration management
 *
 * Extends katra_config.h with persona-specific settings.
 * Uses three-tier precedence: Session > Persona > Global
 *
 * Storage:
 *   - Global:  ~/.katra/config/settings (KEY=VALUE format)
 *   - Persona: ~/.katra/config/{persona_name}/settings (KEY=VALUE format)
 *   - Session: In-memory overrides (not persisted)
 *   - Last persona: ~/.katra/k_last_persona (single line with persona name)
 */

#include <stdbool.h>
#include "katra_limits.h"

/* Initialize persona configuration subsystem */
int katra_persona_config_init(void);

/* Cleanup persona configuration subsystem */
void katra_persona_config_cleanup(void);

/* Get configuration value with persona-specific override
 *
 * Resolution order:
 *   1. Persona-specific config (~/.katra/config/{persona_name}/settings)
 *   2. Global config (~/.katra/config/settings)
 *   3. NULL if not found
 *
 * Parameters:
 *   persona_name - Persona to load config for (NULL = global only)
 *   key - Configuration key
 *
 * Returns:
 *   Value string if found, NULL if not found
 */
const char* katra_persona_config_get(const char* persona_name, const char* key);

/* Set configuration value for persona
 *
 * Parameters:
 *   persona_name - Persona to set config for (NULL = global)
 *   key - Configuration key
 *   value - Configuration value
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if key or value is NULL
 *   E_SYSTEM_FILE if write fails
 */
int katra_persona_config_set(const char* persona_name, const char* key, const char* value);

/* Get last used persona name
 *
 * Reads from ~/.katra/k_last_persona
 *
 * Parameters:
 *   persona_name - Buffer to store persona name
 *   size - Size of buffer
 *
 * Returns:
 *   KATRA_SUCCESS if found
 *   E_NOT_FOUND if file doesn't exist
 *   E_SYSTEM_FILE if read fails
 */
int katra_get_last_persona(char* persona_name, size_t size);

/* Set last used persona name
 *
 * Writes to ~/.katra/k_last_persona atomically
 *
 * Parameters:
 *   persona_name - Persona name to save
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if persona_name is NULL
 *   E_SYSTEM_FILE if write fails
 */
int katra_set_last_persona(const char* persona_name);

/* List all configured personas
 *
 * Scans ~/.katra/config/ for subdirectories with settings files
 *
 * Parameters:
 *   personas - Output array of persona names (caller must free with katra_free_string_array)
 *   count - Output count of personas found
 *
 * Returns:
 *   KATRA_SUCCESS on success (even if count=0)
 *   E_INPUT_NULL if parameters are NULL
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_list_personas(char*** personas, size_t* count);

/* Delete persona configuration
 *
 * Removes ~/.katra/config/{persona_name}/ directory
 *
 * Parameters:
 *   persona_name - Persona to delete
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if persona_name is NULL
 *   E_SYSTEM_FILE if deletion fails
 */
int katra_delete_persona_config(const char* persona_name);

#endif /* KATRA_PERSONA_CONFIG_H */
