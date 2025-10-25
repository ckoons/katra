/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>

/* Project includes */
#include "katra_init.h"
#include "katra_env_utils.h"
#include "katra_config.h"
#include "katra_error.h"
#include "katra_log.h"

/* Initialize Katra library */
int katra_init(void) {
    int result = KATRA_SUCCESS;

    LOG_INFO("Initializing Katra library");

    /* Step 1: Load environment */
    result = katra_loadenv();
    if (result != KATRA_SUCCESS) {
        katra_exit();  /* Cleanup previously initialized subsystems */
        return result;
    }

    /* Step 2: Load configuration */
    result = katra_config();
    if (result != KATRA_SUCCESS) {
        katra_exit();  /* Cleanup previously initialized subsystems */
        return result;
    }

    /* Note: Logging is initialized on demand by first LOG_* call */
    /* Memory tier, checkpoint, and consent systems initialized separately */

    LOG_INFO("Katra initialization complete");
    return KATRA_SUCCESS;
}

/* Cleanup Katra library */
void katra_exit(void) {
    LOG_INFO("Shutting down Katra library");

    /* Cleanup in reverse order of initialization */

    /* Config subsystem */
    katra_config_cleanup();

    /* Environment subsystem */
    katra_freeenv();

    LOG_DEBUG("Katra shutdown complete");
}
