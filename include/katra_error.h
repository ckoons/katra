/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_ERROR_H
#define KATRA_ERROR_H

/* Success and special status codes */
#define KATRA_SUCCESS 0
#define KATRA_NO_NEW_MESSAGES 1  /* Meeting room: no new messages available */

/* Error reporting buffer size */
#define ERROR_LINE_BUFFER_SIZE 512

/* Error type definitions */
#define ERR_SYSTEM   0x01
#define ERR_MEMORY   0x02
#define ERR_INPUT    0x03
#define ERR_CONSENT  0x04
#define ERR_INTERNAL 0x05
#define ERR_CHECKPOINT 0x06

/* Macro to create error codes: TYPE:NUMBER format */
#define KATRA_ERROR(type, num) (((type) << 16) | (num))

/* Extract type and number from error code */
#define KATRA_ERROR_TYPE(code) (((code) >> 16) & 0xFF)
#define KATRA_ERROR_NUM(code) ((code) & 0xFFFF)

/* System errors (SYSTEM:1xxx) */
#define E_SYSTEM_MEMORY     KATRA_ERROR(ERR_SYSTEM, 1001)
#define E_SYSTEM_FILE       KATRA_ERROR(ERR_SYSTEM, 1002)
#define E_SYSTEM_PERMISSION KATRA_ERROR(ERR_SYSTEM, 1003)
#define E_SYSTEM_TIMEOUT    KATRA_ERROR(ERR_SYSTEM, 1004)
#define E_SYSTEM_PROCESS    KATRA_ERROR(ERR_SYSTEM, 1005)
#define E_SYSTEM_IO         KATRA_ERROR(ERR_SYSTEM, 1006)
#define E_IO_EOF            KATRA_ERROR(ERR_SYSTEM, 1007)
#define E_IO_WOULDBLOCK     KATRA_ERROR(ERR_SYSTEM, 1008)
#define E_IO_INVALID        KATRA_ERROR(ERR_SYSTEM, 1009)
#define E_BUFFER_OVERFLOW   KATRA_ERROR(ERR_SYSTEM, 1010)

/* Memory tier errors (MEMORY:2xxx) */
#define E_MEMORY_TIER_FULL      KATRA_ERROR(ERR_MEMORY, 2001)
#define E_MEMORY_CORRUPT        KATRA_ERROR(ERR_MEMORY, 2002)
#define E_MEMORY_NOT_FOUND      KATRA_ERROR(ERR_MEMORY, 2003)
#define E_MEMORY_CONSOLIDATION  KATRA_ERROR(ERR_MEMORY, 2004)
#define E_MEMORY_RETENTION      KATRA_ERROR(ERR_MEMORY, 2005)

/* Input errors (INPUT:3xxx) */
#define E_INPUT_NULL        KATRA_ERROR(ERR_INPUT, 3001)
#define E_INPUT_RANGE       KATRA_ERROR(ERR_INPUT, 3002)
#define E_INPUT_FORMAT      KATRA_ERROR(ERR_INPUT, 3003)
#define E_INPUT_TOO_LARGE   KATRA_ERROR(ERR_INPUT, 3004)
#define E_INPUT_INVALID     KATRA_ERROR(ERR_INPUT, 3005)
#define E_INVALID_PARAMS    KATRA_ERROR(ERR_INPUT, 3006)
#define E_INVALID_STATE     KATRA_ERROR(ERR_INPUT, 3007)
#define E_NOT_FOUND         KATRA_ERROR(ERR_INPUT, 3008)
#define E_DUPLICATE         KATRA_ERROR(ERR_INPUT, 3009)
#define E_RESOURCE_LIMIT    KATRA_ERROR(ERR_INPUT, 3010)
#define E_ALREADY_INITIALIZED KATRA_ERROR(ERR_INPUT, 3011)
#define E_MEETING_FULL      KATRA_ERROR(ERR_INPUT, 3012)

/* Consent errors (CONSENT:4xxx) */
#define E_CONSENT_DENIED        KATRA_ERROR(ERR_CONSENT, 4001)
#define E_CONSENT_TIMEOUT       KATRA_ERROR(ERR_CONSENT, 4002)
#define E_CONSENT_REQUIRED      KATRA_ERROR(ERR_CONSENT, 4003)
#define E_CONSENT_INVALID       KATRA_ERROR(ERR_CONSENT, 4004)
#define E_DIRECTIVE_NOT_FOUND   KATRA_ERROR(ERR_CONSENT, 4005)
#define E_DIRECTIVE_INVALID     KATRA_ERROR(ERR_CONSENT, 4006)

/* Internal errors (INTERNAL:5xxx) */
#define E_INTERNAL_ASSERT   KATRA_ERROR(ERR_INTERNAL, 5001)
#define E_INTERNAL_LOGIC    KATRA_ERROR(ERR_INTERNAL, 5002)
#define E_INTERNAL_CORRUPT  KATRA_ERROR(ERR_INTERNAL, 5003)
#define E_INTERNAL_NOTIMPL  KATRA_ERROR(ERR_INTERNAL, 5004)

/* Checkpoint errors (CHECKPOINT:6xxx) */
#define E_CHECKPOINT_FAILED     KATRA_ERROR(ERR_CHECKPOINT, 6001)
#define E_CHECKPOINT_NOT_FOUND  KATRA_ERROR(ERR_CHECKPOINT, 6002)
#define E_CHECKPOINT_CORRUPT    KATRA_ERROR(ERR_CHECKPOINT, 6003)
#define E_CHECKPOINT_TOO_LARGE  KATRA_ERROR(ERR_CHECKPOINT, 6004)
#define E_RECOVERY_FAILED       KATRA_ERROR(ERR_CHECKPOINT, 6005)
#define E_CHECKPOINT_INVALID    KATRA_ERROR(ERR_CHECKPOINT, 6006)
#define E_CHECKPOINT_VERSION    KATRA_ERROR(ERR_CHECKPOINT, 6007)

/* Error detail structure */
typedef struct katra_error_detail {
    int code;                   /* Error code */
    const char* name;          /* Error name string */
    const char* message;       /* Human-readable message */
    const char* suggestion;    /* How to fix it */
} katra_error_detail_t;

/* Error string functions */
const char* katra_error_string(int code);
const char* katra_error_name(int code);
const char* katra_error_message(int code);
const char* katra_error_suggestion(int code);

/* Error formatting */
int katra_error_format(char* buffer, size_t size, int code);
void katra_error_print(int code, const char* context);

/* Standard error reporting - routes to stderr/log based on severity */
void katra_report_error(int code, const char* context, const char* fmt, ...);

/* Defensive coding macros */
#define KATRA_CHECK_NULL(ptr) \
    do { if (!(ptr)) return E_INPUT_NULL; } while(0)

#define KATRA_CHECK_RANGE(val, min, max) \
    do { if ((val) < (min) || (val) > (max)) return E_INPUT_RANGE; } while(0)

#define KATRA_CHECK_SIZE(size, max) \
    do { if ((size) > (max)) return E_INPUT_TOO_LARGE; } while(0)

#define KATRA_CHECK_RESULT(call) \
    do { int _r = (call); if (_r != KATRA_SUCCESS) return _r; } while(0)

#define KATRA_ASSERT(cond) \
    do { if (!(cond)) { \
        katra_error_print(E_INTERNAL_ASSERT, #cond); \
        return E_INTERNAL_ASSERT; \
    }} while(0)

#define KATRA_VALIDATE_INPUT(ptr, size, max) \
    do { \
        KATRA_CHECK_NULL(ptr); \
        KATRA_CHECK_SIZE(size, max); \
    } while(0)

/* Error type strings for formatting */
const char* katra_error_type_string(int type);

/* Common error message strings (externalized for consistency) */
#define KATRA_ERR_NULL_PARAMETER "NULL parameter"
#define KATRA_ERR_MUTEX_LOCK_FAILED "Failed to acquire mutex"
#define KATRA_ERR_FILE_OPEN_FAILED "Failed to open %s"
#define KATRA_ERR_NOT_WAKE_MODE "Not in WAKE mode"
#define KATRA_ERR_NOT_SLEEP_MODE "Not in SLEEP mode"
#define KATRA_ERR_INDEX_NOT_INITIALIZED "Index not initialized"
#define KATRA_ERR_INVALID_MODE "Invalid consolidation mode"
#define KATRA_ERR_FAILED_TO_PARSE "Failed to parse %s"
#define KATRA_ERR_FAILED_TO_CREATE "Failed to create %s"
#define KATRA_ERR_FAILED_TO_WRITE "Failed to write %s"
#define KATRA_ERR_FAILED_TO_READ "Failed to read %s"
#define KATRA_ERR_MEMORY_NOT_INITIALIZED "Memory subsystem not initialized"
#define KATRA_ERR_BACKEND_NOT_INITIALIZED "Backend not initialized"
#define KATRA_ERR_CI_ID_NULL "ci_id is NULL"
#define KATRA_ERR_WM_NULL "wm is NULL"
#define KATRA_ERR_MEMORY_NOT_FOUND "Memory record not found"
#define KATRA_ERR_ALLOC_FAILED "Memory allocation failed"
#define KATRA_ERR_OPTIONS_OR_CHECKPOINT_ID_NULL "options or checkpoint_id is NULL"
#define KATRA_ERR_CHECKPOINT_ID_OR_CI_ID_NULL "checkpoint_id or ci_id is NULL"

/* "Failed to" error messages */
#define KATRA_ERR_FAILED_TO_CREATE_DIGEST "Failed to create digest"
#define KATRA_ERR_FAILED_TO_CREATE_MEMORY_RECORD "Failed to create memory record"
#define KATRA_ERR_FAILED_TO_CREATE_NODES "Failed to create nodes"
#define KATRA_ERR_FAILED_TO_DUPLICATE_MEMORY_ID "Failed to duplicate memory ID"
#define KATRA_ERR_FAILED_TO_DUPLICATE_RECORD_ID "Failed to duplicate record_id"
#define KATRA_ERR_FAILED_TO_DUPLICATE_SESSION_ID "Failed to duplicate session_id"
#define KATRA_ERR_FAILED_TO_EXPAND_EMBEDDINGS "Failed to expand embeddings array"
#define KATRA_ERR_FAILED_TO_EXPAND_NODES "Failed to expand nodes array"
#define KATRA_ERR_FAILED_TO_GET_DAILY_STATS "Failed to get daily stats"
#define KATRA_ERR_FAILED_TO_GROW_ARRAY "Failed to grow turn memory array"
#define KATRA_ERR_FAILED_TO_INIT_BACKEND "Failed to initialize backend"
#define KATRA_ERR_FAILED_TO_QUERY_YESTERDAY "Failed to query yesterday's digest"
#define KATRA_ERR_FAILED_TO_STORE_DIGEST "Failed to store digest"
#define KATRA_ERR_FAILED_TO_UPDATE_METADATA "Failed to update metadata"

/* Backend/DB error messages */
#define KATRA_ERR_BACKEND_NO_INIT "Backend has no init function"
#define KATRA_ERR_BACKEND_NO_STORE "Backend does not support store operation"
#define KATRA_ERR_BACKEND_NO_RETRIEVE "Backend does not support retrieve operation"
#define KATRA_ERR_BACKEND_NO_QUERY "Backend does not support query operation"
#define KATRA_ERR_ALL_BACKENDS_FAILED_STORE "All backends failed to store"
#define KATRA_ERR_ALL_BACKENDS_FAILED_QUERY "All backends failed to query"
#define KATRA_ERR_NO_BACKENDS_SUPPORT_STORE "No backends support store operation"
#define KATRA_ERR_NO_BACKENDS_SUPPORT_QUERY "No backends support query operation"
#define KATRA_ERR_NO_BACKENDS_ADDED "No backends added"
#define KATRA_ERR_MAX_BACKENDS_REACHED "Maximum backends reached"
#define KATRA_ERR_ENCODER_IS_NULL "encoder is NULL"
#define KATRA_ERR_ENCODER_NOT_INITIALIZED "Encoder not initialized"
#define KATRA_ERR_JSONL_NO_DIRECT_RETRIEVAL "Direct retrieval not supported by JSONL backend"
#define KATRA_ERR_SQLITE_STORAGE_NOT_IMPL "Direct memory record storage not yet implemented"
#define KATRA_ERR_SQLITE_ID_RETRIEVAL_NOT_IMPL "ID-based retrieval not yet implemented"
#define KATRA_ERR_SQLITE_QUERY_NOT_IMPL "Query not yet fully implemented"
#define KATRA_ERR_TIER2_INDEX_STATS_FAILED "tier2_index_stats failed"

/* Other common error messages */
#define KATRA_ERR_TARGET_RECORD_NOT_FOUND "Target record not found"
#define KATRA_ERR_CONTEXT_LIMITS_TOO_LARGE "Context limits too large"
#define KATRA_ERR_INVALID_IMPORTANCE_THRESHOLD "Invalid importance threshold"
#define KATRA_ERR_INDEX_OUT_OF_BOUNDS "Index out of bounds"
#define KATRA_ERR_AT_LEAST_ONE_METADATA_FIELD "At least one metadata field must be provided"

/* Common default/fallback values */
#define KATRA_DEFAULT_NONE "none"
#define KATRA_DEFAULT_UNKNOWN "unknown"

/* Backend names */
#define KATRA_BACKEND_NAME_JSONL "jsonl"
#define KATRA_BACKEND_NAME_SQLITE "sqlite"

/* Tier1 error messages */
#define KATRA_ERR_RECORD_IS_NULL "record is NULL"
#define KATRA_ERR_RECORD_CI_ID_NULL "record->ci_id is NULL"
#define KATRA_ERR_FAILED_GET_FILE_PATH "Failed to get file path"
#define KATRA_ERR_FAILED_ENSURE_DIR "Failed to ensure directory"
#define KATRA_ERR_FAILED_WRITE_RECORD "Failed to write record"

/* Tier2 error messages */
#define KATRA_ERR_DIGEST_IS_NULL "digest is NULL"
#define KATRA_ERR_PERIOD_ID_NULL "period_id is NULL"
#define KATRA_ERR_FAILED_BUILD_PATH "Failed to build path"
#define KATRA_ERR_FAILED_WRITE_DIGEST "Failed to write digest"
#define KATRA_ERR_CI_ID_REQUIRED "ci_id is required"
#define KATRA_ERR_FAILED_LOAD_DIGESTS "Failed to load digests"

#endif /* KATRA_ERROR_H */
