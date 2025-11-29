/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_AUDIT_H
#define KATRA_AUDIT_H

#include <time.h>
#include <stdbool.h>

/**
 * katra_audit.h - Audit Logging for Memory Sharing and Team Operations
 *
 * Provides tamper-evident audit trail for:
 * - Team operations (create, join, leave, delete)
 * - Memory isolation changes
 * - Cross-namespace memory access
 * - Permission grants/denials
 */

/* Audit event types */
typedef enum {
    AUDIT_EVENT_TEAM_CREATE = 1,
    AUDIT_EVENT_TEAM_JOIN,
    AUDIT_EVENT_TEAM_LEAVE,
    AUDIT_EVENT_TEAM_DELETE,
    AUDIT_EVENT_ISOLATION_CHANGE,
    AUDIT_EVENT_MEMORY_SHARE,
    AUDIT_EVENT_MEMORY_ACCESS,
    AUDIT_EVENT_ACCESS_DENIED,
    AUDIT_EVENT_CONSENT_GRANT,
    AUDIT_EVENT_CONSENT_DENY,
    AUDIT_EVENT_MEMORY_ARCHIVE,    /* Phase 7.1: Memory moved to cold storage */
    AUDIT_EVENT_MEMORY_FADE,       /* Phase 7.1: Memory importance reduced */
    AUDIT_EVENT_MEMORY_FORGET      /* Phase 7.1: Memory deleted with consent */
} audit_event_type_t;

/* Audit record structure */
typedef struct {
    audit_event_type_t event_type;
    time_t timestamp;
    char* ci_id;              /* CI performing the action */
    char* target_ci_id;       /* CI being acted upon (optional) */
    char* team_name;          /* Team involved (optional) */
    char* memory_id;          /* Memory record involved (optional) */
    char* details;            /* Additional context (JSON or plain text) */
    bool success;             /* Whether operation succeeded */
    int error_code;           /* Error code if failed */
} audit_record_t;

/**
 * katra_audit_init() - Initialize audit logging system
 *
 * Creates audit log directory and initializes storage.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if cannot create audit directory
 */
int katra_audit_init(void);

/**
 * katra_audit_cleanup() - Cleanup audit logging system
 *
 * Flushes any pending audit records and closes files.
 */
void katra_audit_cleanup(void);

/**
 * katra_audit_log() - Record an audit event
 *
 * Parameters:
 *   record - Audit record to log (ownership retained by caller)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if record is NULL
 *   E_SYSTEM_FILE if cannot write to audit log
 *
 * Note: This function is thread-safe and uses internal locking.
 */
int katra_audit_log(const audit_record_t* record);

/**
 * katra_audit_log_team_op() - Convenience function for team operations
 *
 * Parameters:
 *   event_type - Type of team operation
 *   ci_id - CI performing the operation
 *   team_name - Team being operated on
 *   target_ci_id - Optional: CI being added/removed (for join/leave)
 *   success - Whether operation succeeded
 *   error_code - Error code if failed (or KATRA_SUCCESS)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error code on failure
 */
int katra_audit_log_team_op(audit_event_type_t event_type,
                              const char* ci_id,
                              const char* team_name,
                              const char* target_ci_id,
                              bool success,
                              int error_code);

/**
 * katra_audit_log_memory_access() - Log memory access event
 *
 * Parameters:
 *   ci_id - CI accessing the memory
 *   memory_id - Memory record being accessed
 *   owner_ci_id - CI that owns the memory
 *   team_name - Team context (if TEAM isolation)
 *   success - Whether access was granted
 *   error_code - Error code if denied
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error code on failure
 */
int katra_audit_log_memory_access(const char* ci_id,
                                    const char* memory_id,
                                    const char* owner_ci_id,
                                    const char* team_name,
                                    bool success,
                                    int error_code);

/**
 * katra_audit_log_isolation_change() - Log isolation level change
 *
 * Parameters:
 *   ci_id - CI making the change
 *   memory_id - Memory record affected
 *   old_level - Previous isolation level (as string)
 *   new_level - New isolation level (as string)
 *   team_name - Team name (if TEAM isolation)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   Error code on failure
 */
int katra_audit_log_isolation_change(const char* ci_id,
                                       const char* memory_id,
                                       const char* old_level,
                                       const char* new_level,
                                       const char* team_name);

/**
 * katra_audit_free_record() - Free audit record fields
 *
 * Parameters:
 *   record - Audit record to free (caller must free the struct itself)
 *
 * Frees all dynamically allocated strings in the record.
 */
void katra_audit_free_record(audit_record_t* record);

/* Event type string conversion */
const char* katra_audit_event_type_string(audit_event_type_t type);

/* Audit log file location */
#define AUDIT_LOG_FILENAME "audit.jsonl"

/* Error messages */
#define AUDIT_ERR_MUTEX_LOCK "Failed to acquire audit mutex"
#define AUDIT_ERR_NOT_INITIALIZED "Audit system not initialized"
#define AUDIT_ERR_FILE_OPEN "Failed to open audit log file"
#define AUDIT_ERR_FILE_WRITE "Failed to write to audit log"

#endif /* KATRA_AUDIT_H */
