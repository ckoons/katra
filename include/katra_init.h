/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_INIT_H
#define KATRA_INIT_H

/* Initialize Katra library
 *
 * Initializes all Katra subsystems in correct order:
 * 1. Environment loading (.env.katra files)
 * 2. Configuration loading (.katra/config/ directories)
 * 3. Logging system initialization
 *
 * Must be called before using any other Katra functions.
 * Safe to call multiple times (subsequent calls are no-ops).
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if .env.katra not found
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_init(void);

/* Cleanup Katra library
 *
 * Cleans up all Katra subsystems in reverse order:
 * 1. Configuration cleanup
 * 2. Environment cleanup
 * 3. Logging cleanup
 *
 * Safe to call multiple times.
 * After calling this, katra_init() must be called again before
 * using other Katra functions.
 */
void katra_exit(void);

#endif /* KATRA_INIT_H */
