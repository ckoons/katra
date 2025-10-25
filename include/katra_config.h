/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_CONFIG_H
#define KATRA_CONFIG_H

/* Configuration subsystem
 *
 * Loads and manages Katra configuration from config/ directory hierarchies.
 * Unlike .env files (for application config like log levels), config/ directories
 * hold katra-specific settings (memory tier config, checkpoint settings, etc.).
 *
 * Directory hierarchy (loaded in order, later overrides earlier):
 *   1. ~/.katra/config/           (user-wide katra settings)
 *   2. <project>/.katra/config/   (project katra settings)
 *
 * Configuration is loaded during katra_init() after environment is loaded,
 * so config files can reference environment variables.
 *
 * Format: Simple KEY=VALUE files (one per line, # for comments)
 */

/* Load Katra configuration
 *
 * Sequence:
 * 1. Create .katra/ directory structure if needed
 * 2. Scan and load config/ directories in precedence order
 * 3. Store config in memory for katra_config_get() access
 *
 * Uses KATRA_ROOT from environment to locate project config directories.
 * Silently skips missing directories/files (all are optional).
 *
 * Returns:
 *   KATRA_SUCCESS on success (even if no config found)
 *   E_SYSTEM_FILE if required directories cannot be created
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_config(void);

/* Get configuration value
 *
 * Retrieves config value by key. Returns NULL if key not found.
 *
 * Parameters:
 *   key - Configuration key to lookup
 *
 * Returns:
 *   Value string if found, NULL if not found
 *   Returned pointer is valid until katra_config_cleanup() is called
 */
const char* katra_config_get(const char* key);

/* Reload configuration
 *
 * Reloads config from disk, useful if config files changed.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error codes same as katra_config()
 */
int katra_config_reload(void);

/* Cleanup configuration subsystem
 *
 * Frees all configuration resources.
 * Safe to call multiple times.
 */
void katra_config_cleanup(void);

#endif /* KATRA_CONFIG_H */
