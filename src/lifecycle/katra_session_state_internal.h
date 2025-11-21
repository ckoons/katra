/* © 2025 Casey Koons All rights reserved */

/* Internal header for session state helpers (shared across split files) */

#ifndef KATRA_SESSION_STATE_INTERNAL_H
#define KATRA_SESSION_STATE_INTERNAL_H

#include "katra_session_state.h"

/* Enum to string conversions */
const char* cognitive_mode_to_string(session_cognitive_mode_t mode);
const char* emotional_state_to_string(session_emotional_state_t emotion);
const char* insight_impact_to_string(insight_impact_t impact);
const char* insight_type_to_string(insight_type_t type);

/* String to enum conversions */
session_cognitive_mode_t cognitive_mode_from_string(const char* str);
session_emotional_state_t emotional_state_from_string(const char* str);
insight_impact_t insight_impact_from_string(const char* str);
insight_type_t insight_type_from_string(const char* str);

#endif /* KATRA_SESSION_STATE_INTERNAL_H */
