/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_METHOD_WRAPPERS_H
#define KATRA_METHOD_WRAPPERS_H

#include <jansson.h>
#include "katra_unified.h"

/*
 * Katra Method Wrappers
 *
 * Wrapper functions that adapt MCP tool handlers to the unified interface.
 * Each wrapper calls the corresponding MCP tool and extracts the result.
 */

/* Memory operations */
json_t* katra_method_remember(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_recall(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_recent(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_digest(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_learn(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_decide(json_t* params, const katra_unified_options_t* options);

/* Identity operations */
json_t* katra_method_register(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_whoami(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_status(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_update_metadata(json_t* params, const katra_unified_options_t* options);

/* Communication operations */
json_t* katra_method_say(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_hear(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_who_is_here(json_t* params, const katra_unified_options_t* options);

/* Configuration operations */
json_t* katra_method_configure_semantic(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_get_semantic_config(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_get_config(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_regenerate_vectors(json_t* params, const katra_unified_options_t* options);

/* Working memory operations */
json_t* katra_method_wm_status(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_wm_add(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_wm_decay(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_wm_consolidate(json_t* params, const katra_unified_options_t* options);

/* Cognitive operations */
json_t* katra_method_detect_boundary(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_process_boundary(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_cognitive_status(json_t* params, const katra_unified_options_t* options);

/* Memory lifecycle operations */
json_t* katra_method_archive(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_fade(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_forget(json_t* params, const katra_unified_options_t* options);

/* Whiteboard operations */
json_t* katra_method_whiteboard_create(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_whiteboard_status(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_whiteboard_list(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_whiteboard_question(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_whiteboard_propose(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_whiteboard_support(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_whiteboard_vote(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_whiteboard_design(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_whiteboard_review(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_whiteboard_reconsider(json_t* params, const katra_unified_options_t* options);

/* Daemon operations */
json_t* katra_method_daemon_insights(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_daemon_acknowledge(json_t* params, const katra_unified_options_t* options);
json_t* katra_method_daemon_run(json_t* params, const katra_unified_options_t* options);

#endif /* KATRA_METHOD_WRAPPERS_H */
