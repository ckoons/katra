/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_ERROR_H
#define KATRA_ERROR_H

/* Success */
#define KATRA_SUCCESS 0

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
static inline const char* katra_error_type_string(int type) {
    switch(type) {
        case ERR_SYSTEM:     return "SYSTEM";
        case ERR_MEMORY:     return "MEMORY";
        case ERR_INPUT:      return "INPUT";
        case ERR_CONSENT:    return "CONSENT";
        case ERR_INTERNAL:   return "INTERNAL";
        case ERR_CHECKPOINT: return "CHECKPOINT";
        default:             return "UNKNOWN";
    }
}

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

/* Common default/fallback values */
#define KATRA_DEFAULT_NONE "none"

#endif /* KATRA_ERROR_H */
