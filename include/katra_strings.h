/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_STRINGS_H
#define KATRA_STRINGS_H

/* Common string constants for Katra
 *
 * Centralizes string literals to reduce duplication and improve maintainability.
 * All non-format string literals should be defined here rather than in .c files.
 */

/* Directory names (relative to ~/.katra/) */
#define KATRA_DIR_CHECKPOINTS "checkpoints"
#define KATRA_DIR_MEMORY      "memory"
#define KATRA_DIR_TIER1       "tier1"
#define KATRA_DIR_TIER2       "tier2"
#define KATRA_DIR_CONFIG      "config"
#define KATRA_DIR_LOGS        "logs"
#define KATRA_DIR_AUDIT       "audit"

/* File mode strings */
#define KATRA_FILE_MODE_READ   "r"
#define KATRA_FILE_MODE_WRITE  "w"
#define KATRA_FILE_MODE_APPEND "a"

/* Environment variable names */
#define KATRA_ENV_VAR_ROOT "KATRA_ROOT"
#define KATRA_ENV_VAR_HOME "HOME"

/* Checkpoint constants */
#define KATRA_CHECKPOINT_MAGIC "KATRA_CHECKPOINT_V1"
#define KATRA_CHECKPOINT_VERSION "1.0.0"
#define KATRA_CHECKPOINT_PREFIX "checkpoint_"
#define KATRA_CHECKPOINT_SUFFIX ".kcp"
#define KATRA_CHECKPOINT_RECORD_SEPARATOR "---RECORDS---"

/* JSON field names (for simple parsing) */
#define KATRA_JSON_FIELD_CHECKPOINT_ID "checkpoint_id"
#define KATRA_JSON_FIELD_CI_ID "ci_id"
#define KATRA_JSON_FIELD_TIMESTAMP "timestamp"
#define KATRA_JSON_FIELD_VERSION "version"
#define KATRA_JSON_FIELD_RECORD_COUNT "record_count"
#define KATRA_JSON_FIELD_TIER1_RECORDS "tier1_records"
#define KATRA_JSON_FIELD_NOTES "notes"

/* Line terminators */
#define KATRA_LINE_TERMINATOR "\n"

#endif /* KATRA_STRINGS_H */
