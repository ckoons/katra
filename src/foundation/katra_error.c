/* © 2025 Casey Koons All rights reserved */

/* Error string and formatting functions */

/* System includes */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Project includes */
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* Error descriptions - human readable messages */
/* GUIDELINE_APPROVED: Error descriptions are function return values, not constants */
static const char* get_error_description(int code) {
    switch (code) {
        /* System errors */
        case E_SYSTEM_MEMORY:     return "Out of memory"; /* GUIDELINE_APPROVED */
        case E_SYSTEM_FILE:       return "File operation failed";
        case E_SYSTEM_PERMISSION: return "Permission denied";
        case E_SYSTEM_TIMEOUT:    return "Operation timed out";
        case E_SYSTEM_PROCESS:    return "Process operation failed";
        case E_SYSTEM_IO:         return "I/O operation failed";
        case E_IO_EOF:            return "End of file";
        case E_IO_WOULDBLOCK:     return "Operation would block";
        case E_IO_INVALID:        return "Invalid I/O operation";
        case E_BUFFER_OVERFLOW:   return "Buffer overflow";

        /* Memory tier errors */
        case E_MEMORY_TIER_FULL:      return "Memory tier full";
        case E_MEMORY_CORRUPT:        return "Memory data corrupted";
        case E_MEMORY_NOT_FOUND:      return "Memory entry not found";
        case E_MEMORY_CONSOLIDATION:  return "Memory consolidation failed";
        case E_MEMORY_RETENTION:      return "Memory retention policy violated";

        /* Input errors */
        case E_INPUT_NULL:        return "Null pointer provided";
        case E_INPUT_RANGE:       return "Value out of range";
        case E_INPUT_FORMAT:      return "Invalid format";
        case E_INPUT_TOO_LARGE:   return "Input too large";
        case E_INPUT_INVALID:     return "Invalid input";
        case E_INVALID_PARAMS:    return "Invalid parameters";
        case E_INVALID_STATE:     return "Invalid state";
        case E_NOT_FOUND:         return "Not found";
        case E_DUPLICATE:         return "Duplicate entry";
        case E_RESOURCE_LIMIT:    return "Resource limit exceeded";

        /* Consent errors */
        case E_CONSENT_DENIED:        return "Consent denied";
        case E_CONSENT_TIMEOUT:       return "Consent request timed out";
        case E_CONSENT_REQUIRED:      return "Consent required for operation";
        case E_CONSENT_INVALID:       return "Invalid consent request";
        case E_DIRECTIVE_NOT_FOUND:   return "Advance directive not found";
        case E_DIRECTIVE_INVALID:     return "Invalid advance directive";

        /* Internal errors */
        case E_INTERNAL_ASSERT:   return "Assertion failed";
        case E_INTERNAL_LOGIC:    return "Internal logic error";
        case E_INTERNAL_CORRUPT:  return "Data corruption detected";
        case E_INTERNAL_NOTIMPL:  return "Not implemented";

        /* Checkpoint errors */
        case E_CHECKPOINT_FAILED:     return "Checkpoint creation failed";
        case E_CHECKPOINT_NOT_FOUND:  return "Checkpoint not found";
        case E_CHECKPOINT_CORRUPT:    return "Checkpoint data corrupted";
        case E_CHECKPOINT_TOO_LARGE:  return "Checkpoint exceeds size limit";
        case E_RECOVERY_FAILED:       return "Recovery from checkpoint failed";

        default:                  return "Unknown error";
    }
}

/* Format error as human-readable string */
const char* katra_error_string(int code) {
    static char buffer[KATRA_BUFFER_MEDIUM];

    if (code == KATRA_SUCCESS) {
        return "Success";
    }

    int type = KATRA_ERROR_TYPE(code);
    int num = KATRA_ERROR_NUM(code);
    const char* desc = get_error_description(code);
    const char* type_str = katra_error_type_string(type);

    snprintf(buffer, sizeof(buffer), "%s (%s:%d)",
             desc, type_str, num);

    return buffer;
}

/* Get just the error name (short form) */
/* GUIDELINE_APPROVED: Error names are function return values, not constants */
const char* katra_error_name(int code) {
    switch (code) {
        case KATRA_SUCCESS:       return "SUCCESS"; /* GUIDELINE_APPROVED */
        case E_SYSTEM_MEMORY:     return "E_SYSTEM_MEMORY";
        case E_SYSTEM_FILE:       return "E_SYSTEM_FILE";
        case E_SYSTEM_PERMISSION: return "E_SYSTEM_PERMISSION";
        case E_SYSTEM_TIMEOUT:    return "E_SYSTEM_TIMEOUT";
        case E_SYSTEM_PROCESS:    return "E_SYSTEM_PROCESS";
        case E_SYSTEM_IO:         return "E_SYSTEM_IO";
        case E_IO_EOF:            return "E_IO_EOF";
        case E_IO_WOULDBLOCK:     return "E_IO_WOULDBLOCK";
        case E_IO_INVALID:        return "E_IO_INVALID";
        case E_BUFFER_OVERFLOW:   return "E_BUFFER_OVERFLOW";
        case E_MEMORY_TIER_FULL:      return "E_MEMORY_TIER_FULL";
        case E_MEMORY_CORRUPT:        return "E_MEMORY_CORRUPT";
        case E_MEMORY_NOT_FOUND:      return "E_MEMORY_NOT_FOUND";
        case E_MEMORY_CONSOLIDATION:  return "E_MEMORY_CONSOLIDATION";
        case E_MEMORY_RETENTION:      return "E_MEMORY_RETENTION";
        case E_INPUT_NULL:        return "E_INPUT_NULL";
        case E_INPUT_RANGE:       return "E_INPUT_RANGE";
        case E_INPUT_FORMAT:      return "E_INPUT_FORMAT";
        case E_INPUT_TOO_LARGE:   return "E_INPUT_TOO_LARGE";
        case E_INPUT_INVALID:     return "E_INPUT_INVALID";
        case E_INVALID_PARAMS:    return "E_INVALID_PARAMS";
        case E_INVALID_STATE:     return "E_INVALID_STATE";
        case E_NOT_FOUND:         return "E_NOT_FOUND";
        case E_DUPLICATE:         return "E_DUPLICATE";
        case E_RESOURCE_LIMIT:    return "E_RESOURCE_LIMIT";
        case E_CONSENT_DENIED:        return "E_CONSENT_DENIED";
        case E_CONSENT_TIMEOUT:       return "E_CONSENT_TIMEOUT";
        case E_CONSENT_REQUIRED:      return "E_CONSENT_REQUIRED";
        case E_CONSENT_INVALID:       return "E_CONSENT_INVALID";
        case E_DIRECTIVE_NOT_FOUND:   return "E_DIRECTIVE_NOT_FOUND";
        case E_DIRECTIVE_INVALID:     return "E_DIRECTIVE_INVALID";
        case E_INTERNAL_ASSERT:   return "E_INTERNAL_ASSERT";
        case E_INTERNAL_LOGIC:    return "E_INTERNAL_LOGIC";
        case E_INTERNAL_CORRUPT:  return "E_INTERNAL_CORRUPT";
        case E_INTERNAL_NOTIMPL:  return "E_INTERNAL_NOTIMPL";
        case E_CHECKPOINT_FAILED:     return "E_CHECKPOINT_FAILED";
        case E_CHECKPOINT_NOT_FOUND:  return "E_CHECKPOINT_NOT_FOUND";
        case E_CHECKPOINT_CORRUPT:    return "E_CHECKPOINT_CORRUPT";
        case E_CHECKPOINT_TOO_LARGE:  return "E_CHECKPOINT_TOO_LARGE";
        case E_RECOVERY_FAILED:       return "E_RECOVERY_FAILED";
        default:                  return "E_UNKNOWN";
    }
}

/* Get just the human message (no code) */
const char* katra_error_message(int code) {
    return get_error_description(code);
}

/* Get suggestion for fixing the error */
/* GUIDELINE_APPROVED: Error suggestions are function return values, not constants */
const char* katra_error_suggestion(int code) {
    switch (code) {
        case E_SYSTEM_MEMORY:     return "Reduce memory usage or increase available memory"; /* GUIDELINE_APPROVED */
        case E_SYSTEM_FILE:       return "Verify file permissions and disk space";
        case E_SYSTEM_PERMISSION: return "Run with appropriate permissions";
        case E_SYSTEM_TIMEOUT:    return "Increase timeout or check system responsiveness";
        case E_MEMORY_TIER_FULL:      return "Trigger memory consolidation or increase tier limits";
        case E_MEMORY_CORRUPT:        return "Restore from checkpoint or verify data integrity";
        case E_MEMORY_NOT_FOUND:      return "Check memory tier and retention settings";
        case E_MEMORY_CONSOLIDATION:  return "Check logs for consolidation errors";
        case E_CONSENT_DENIED:        return "Request denied - operation cannot proceed";
        case E_CONSENT_TIMEOUT:       return "No response received within timeout period";
        case E_CONSENT_REQUIRED:      return "Obtain consent before attempting operation";
        case E_DIRECTIVE_NOT_FOUND:   return "Create advance directive before operation";
        case E_DIRECTIVE_INVALID:     return "Verify advance directive format and content";
        case E_INPUT_NULL:        return "Provide valid non-null input";
        case E_INPUT_RANGE:       return "Use value within valid range";
        case E_INPUT_TOO_LARGE:   return "Reduce input size";
        case E_CHECKPOINT_FAILED:     return "Check disk space and permissions";
        case E_CHECKPOINT_CORRUPT:    return "Restore from earlier checkpoint";
        case E_CHECKPOINT_TOO_LARGE:  return "Reduce checkpoint data or increase limit";
        case E_RECOVERY_FAILED:       return "Attempt recovery from earlier checkpoint";
        case E_INTERNAL_LOGIC:    return "Report this bug with reproduction steps";
        case E_INTERNAL_NOTIMPL:  return "Feature not yet implemented";
        default:                  return "Consult documentation or logs";
    }
}

/* Format error with full context */
int katra_error_format(char* buffer, size_t size, int code) {
    if (!buffer || size == 0) return -1;

    const char* name = katra_error_name(code);
    const char* message = katra_error_message(code);
    const char* suggestion = katra_error_suggestion(code);
    int type = KATRA_ERROR_TYPE(code);
    int num = KATRA_ERROR_NUM(code);

    return snprintf(buffer, size,
                   "Error: %s\n"
                   "Code: %s:%d\n"
                   "Message: %s\n"
                   "Suggestion: %s\n",
                   name,
                   katra_error_type_string(type), num,
                   message,
                   suggestion);
}

/* Print error with context to stderr */
void katra_error_print(int code, const char* context) {
    fprintf(stderr, "Error");
    if (context) {
        fprintf(stderr, " in %s", context);
    }
    fprintf(stderr, ": %s\n", katra_error_string(code));
}

/* Standard error reporting with routing based on severity */
void katra_report_error(int code, const char* context, const char* fmt, ...) {
    if (code == KATRA_SUCCESS) return;

    int type = KATRA_ERROR_TYPE(code);
    int num = KATRA_ERROR_NUM(code);
    const char* type_str = katra_error_type_string(type);
    const char* message = katra_error_message(code);

    /* Format: [KATRA ERROR] context: message (details) [TYPE:NUM] */
    char error_line[ERROR_LINE_BUFFER_SIZE];
    int pos = 0;

    pos += snprintf(error_line + pos, sizeof(error_line) - pos, "[KATRA ERROR]");

    if (context) {
        pos += snprintf(error_line + pos, sizeof(error_line) - pos, " %s:", context);
    }

    pos += snprintf(error_line + pos, sizeof(error_line) - pos, " %s", message);

    /* Add formatted details if provided */
    if (fmt && fmt[0] != '\0') {
        va_list args;
        va_start(args, fmt);
        char details[KATRA_BUFFER_MEDIUM];
        vsnprintf(details, sizeof(details), fmt, args);
        va_end(args);
        pos += snprintf(error_line + pos, sizeof(error_line) - pos, " (%s)", details);
    }

    snprintf(error_line + pos, sizeof(error_line) - pos, " [%s:%d]", type_str, num);

    /* Route based on severity:
     * INTERNAL/SYSTEM -> stderr + log (critical)
     * MEMORY/CONSENT/CHECKPOINT -> log only (expected in normal operation)
     * INPUT -> log only (expected)
     */
    int to_stderr = (type == ERR_INTERNAL || type == ERR_SYSTEM);

    if (to_stderr) {
        fprintf(stderr, "%s\n", error_line);
    }

    /* Always log errors */
    LOG_ERROR("%s", error_line);
}
