/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_LIFECYCLE_INTERNAL_H
#define KATRA_LIFECYCLE_INTERNAL_H

#include "katra_lifecycle.h"

/* Internal functions shared between katra_lifecycle.c and katra_lifecycle_session.c */

/* Get the global session state (internal access) */
session_state_t* katra_lifecycle_get_state(void);

/* Get the global session end state (internal access) */
session_end_state_t* katra_lifecycle_get_end_state(void);

/* Check if lifecycle is initialized (internal access) */
bool katra_lifecycle_is_initialized(void);

#endif /* KATRA_LIFECYCLE_INTERNAL_H */
