/* © 2025 Casey Koons All rights reserved */

/*
 * katra_audit.c - Audit Logging for Memory Sharing (Phase 7)
 *
 * Tamper-evident audit trail using JSONL format.
 * Records all team operations, memory access, and isolation changes.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* Project includes */
#include "katra_audit.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_path_utils.h"
#include "katra_core_common.h"
#include "katra_json_utils.h"

/* Global state */
static FILE* g_audit_file = NULL;
static pthread_mutex_t g_audit_lock = PTHREAD_MUTEX_INITIALIZER;
static bool g_audit_initialized = false;
static uint64_t g_audit_sequence = 0;

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int katra_audit_init(void) {
    int result = KATRA_SUCCESS;
    char audit_path[KATRA_PATH_MAX];

    int lock_result = pthread_mutex_lock(&g_audit_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_audit_init",
                          AUDIT_ERR_MUTEX_LOCK);
        return E_SYSTEM_PERMISSION;
    }

    if (g_audit_initialized) {
        pthread_mutex_unlock(&g_audit_lock);
        return KATRA_SUCCESS;
    }

    /* Get audit log path: ~/.katra/audit.jsonl */
    result = katra_build_path(audit_path, sizeof(audit_path),
                               AUDIT_LOG_FILENAME, NULL);
    if (result != KATRA_SUCCESS) {
        pthread_mutex_unlock(&g_audit_lock);
        return result;
    }

    /* Open audit log in append mode */
    g_audit_file = fopen(audit_path, "a");
    if (!g_audit_file) {
        katra_report_error(E_SYSTEM_FILE, "katra_audit_init",
                          AUDIT_ERR_FILE_OPEN);
        pthread_mutex_unlock(&g_audit_lock);
        return E_SYSTEM_FILE;
    }

    /* Set line buffering for immediate writes */
    setlinebuf(g_audit_file);

    g_audit_initialized = true;
    LOG_INFO("Audit logging initialized: %s", audit_path);

    pthread_mutex_unlock(&g_audit_lock);
    return KATRA_SUCCESS;
}

void katra_audit_cleanup(void) {
    int lock_result = pthread_mutex_lock(&g_audit_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_audit_cleanup",
                          AUDIT_ERR_MUTEX_LOCK);
        /* Continue cleanup anyway to avoid leaks */
    }

    if (g_audit_file) {
        fflush(g_audit_file);
        fclose(g_audit_file);
        g_audit_file = NULL;
    }

    g_audit_initialized = false;
    g_audit_sequence = 0;
    LOG_DEBUG("Audit logging cleaned up");

    pthread_mutex_unlock(&g_audit_lock);
}

/* ============================================================================
 * EVENT TYPE STRINGS
 * ============================================================================ */

const char* katra_audit_event_type_string(audit_event_type_t type) {
    switch (type) {
        case AUDIT_EVENT_TEAM_CREATE:       return "team_create";
        case AUDIT_EVENT_TEAM_JOIN:         return "team_join";
        case AUDIT_EVENT_TEAM_LEAVE:        return "team_leave";
        case AUDIT_EVENT_TEAM_DELETE:       return "team_delete";
        case AUDIT_EVENT_ISOLATION_CHANGE:  return "isolation_change";
        case AUDIT_EVENT_MEMORY_SHARE:      return "memory_share";
        case AUDIT_EVENT_MEMORY_ACCESS:     return "memory_access";
        case AUDIT_EVENT_ACCESS_DENIED:     return "access_denied";
        case AUDIT_EVENT_CONSENT_GRANT:     return "consent_grant";
        case AUDIT_EVENT_CONSENT_DENY:      return "consent_deny";
        case AUDIT_EVENT_MEMORY_ARCHIVE:    return "memory_archive";
        case AUDIT_EVENT_MEMORY_FADE:       return "memory_fade";
        case AUDIT_EVENT_MEMORY_FORGET:     return "memory_forget";
        default:                            return "unknown";
    }
}

/* ============================================================================
 * AUDIT LOGGING
 * ============================================================================ */

int katra_audit_log(const audit_record_t* record) {
    if (!record) {
        return E_INPUT_NULL;
    }

    int lock_result = pthread_mutex_lock(&g_audit_lock);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PERMISSION, "katra_audit_log",
                          AUDIT_ERR_MUTEX_LOCK);
        return E_SYSTEM_PERMISSION;
    }

    if (!g_audit_initialized || !g_audit_file) {
        pthread_mutex_unlock(&g_audit_lock);
        katra_report_error(E_INVALID_STATE, "katra_audit_log",
                          AUDIT_ERR_NOT_INITIALIZED);
        return E_INVALID_STATE;
    }

    /* Build JSON manually for audit record */
    char json_buffer[KATRA_BUFFER_ENHANCED];
    char* pos = json_buffer;
    size_t remaining = sizeof(json_buffer);
    int written;

    /* Start JSON object */
    written = snprintf(pos, remaining, "{");
    pos += written;
    remaining -= written;

    /* Add sequence number (tamper detection) */
    g_audit_sequence++;
    written = snprintf(pos, remaining, "\"sequence\":%llu,",
                      (unsigned long long)g_audit_sequence);
    pos += written;
    remaining -= written;

    /* Add timestamp */
    written = snprintf(pos, remaining, "\"timestamp\":%ld,", (long)record->timestamp);
    pos += written;
    remaining -= written;

    /* Add event type */
    const char* event_str = katra_audit_event_type_string(record->event_type);
    char escaped_event[KATRA_BUFFER_SMALL];
    katra_json_escape(event_str, escaped_event, sizeof(escaped_event));
    written = snprintf(pos, remaining, "\"event_type\":\"%s\",", escaped_event);
    pos += written;
    remaining -= written;

    /* Add CI ID */
    if (record->ci_id) {
        char escaped_ci[KATRA_BUFFER_MEDIUM];
        katra_json_escape(record->ci_id, escaped_ci, sizeof(escaped_ci));
        written = snprintf(pos, remaining, "\"ci_id\":\"%s\",", escaped_ci);
        pos += written;
        remaining -= written;
    }

    /* Add target CI ID (optional) */
    if (record->target_ci_id) {
        char escaped_target[KATRA_BUFFER_MEDIUM];
        katra_json_escape(record->target_ci_id, escaped_target, sizeof(escaped_target));
        written = snprintf(pos, remaining, "\"target_ci_id\":\"%s\",", escaped_target);
        pos += written;
        remaining -= written;
    }

    /* Add team name (optional) */
    if (record->team_name) {
        char escaped_team[KATRA_BUFFER_MEDIUM];
        katra_json_escape(record->team_name, escaped_team, sizeof(escaped_team));
        written = snprintf(pos, remaining, "\"team_name\":\"%s\",", escaped_team);
        pos += written;
        remaining -= written;
    }

    /* Add memory ID (optional) */
    if (record->memory_id) {
        char escaped_memory[KATRA_BUFFER_MEDIUM];
        katra_json_escape(record->memory_id, escaped_memory, sizeof(escaped_memory));
        written = snprintf(pos, remaining, "\"memory_id\":\"%s\",", escaped_memory);
        pos += written;
        remaining -= written;
    }

    /* Add details (optional) */
    if (record->details) {
        char escaped_details[KATRA_BUFFER_TEXT];
        katra_json_escape(record->details, escaped_details, sizeof(escaped_details));
        written = snprintf(pos, remaining, "\"details\":\"%s\",", escaped_details);
        pos += written;
        remaining -= written;
    }

    /* Add success status */
    written = snprintf(pos, remaining, "\"success\":%s,",
                      record->success ? "true" : "false");
    pos += written;
    remaining -= written;

    /* Add error code if failed */
    if (!record->success && record->error_code != KATRA_SUCCESS) {
        const char* error_name = katra_error_name(record->error_code);
        char escaped_error[KATRA_BUFFER_SMALL];
        katra_json_escape(error_name, escaped_error, sizeof(escaped_error));
        written = snprintf(pos, remaining, "\"error_code\":%d,\"error_name\":\"%s\",",
                          record->error_code, escaped_error);
        pos += written;
        remaining -= written;
    }

    /* Remove trailing comma and close JSON object */
    if (pos > json_buffer && *(pos - 1) == ',') {
        pos--;
        remaining++;
    }
    snprintf(pos, remaining, "}\n");

    /* Write to file */
    int write_result = fputs(json_buffer, g_audit_file);
    if (write_result < 0) {
        pthread_mutex_unlock(&g_audit_lock);
        katra_report_error(E_SYSTEM_FILE, "katra_audit_log",
                          AUDIT_ERR_FILE_WRITE);
        return E_SYSTEM_FILE;
    }

    /* Flush to ensure durability */
    fflush(g_audit_file);

    pthread_mutex_unlock(&g_audit_lock);
    return KATRA_SUCCESS;
}

/* ============================================================================
 * CONVENIENCE FUNCTIONS
 * ============================================================================ */

int katra_audit_log_team_op(audit_event_type_t event_type,
                              const char* ci_id,
                              const char* team_name,
                              const char* target_ci_id,
                              bool success,
                              int error_code) {
    if (!ci_id || !team_name) {
        return E_INPUT_NULL;
    }

    audit_record_t record = {
        .event_type = event_type,
        .timestamp = time(NULL),
        .ci_id = (char*)ci_id,
        .target_ci_id = (char*)target_ci_id,
        .team_name = (char*)team_name,
        .memory_id = NULL,
        .details = NULL,
        .success = success,
        .error_code = error_code
    };

    return katra_audit_log(&record);
}

int katra_audit_log_memory_access(const char* ci_id,
                                    const char* memory_id,
                                    const char* owner_ci_id,
                                    const char* team_name,
                                    bool success,
                                    int error_code) {
    if (!ci_id || !memory_id) {
        return E_INPUT_NULL;
    }

    /* Build details string */
    char details[KATRA_BUFFER_MESSAGE];
    snprintf(details, sizeof(details), "owner=%s",
             owner_ci_id ? owner_ci_id : "unknown");

    audit_record_t record = {
        .event_type = success ? AUDIT_EVENT_MEMORY_ACCESS : AUDIT_EVENT_ACCESS_DENIED,
        .timestamp = time(NULL),
        .ci_id = (char*)ci_id,
        .target_ci_id = (char*)owner_ci_id,
        .team_name = (char*)team_name,
        .memory_id = (char*)memory_id,
        .details = details,
        .success = success,
        .error_code = error_code
    };

    return katra_audit_log(&record);
}

int katra_audit_log_isolation_change(const char* ci_id,
                                       const char* memory_id,
                                       const char* old_level,
                                       const char* new_level,
                                       const char* team_name) {
    if (!ci_id || !memory_id || !old_level || !new_level) {
        return E_INPUT_NULL;
    }

    /* Build details string */
    char details[KATRA_BUFFER_MESSAGE];
    snprintf(details, sizeof(details), "from=%s to=%s", old_level, new_level);

    audit_record_t record = {
        .event_type = AUDIT_EVENT_ISOLATION_CHANGE,
        .timestamp = time(NULL),
        .ci_id = (char*)ci_id,
        .target_ci_id = NULL,
        .team_name = (char*)team_name,
        .memory_id = (char*)memory_id,
        .details = details,
        .success = true,
        .error_code = KATRA_SUCCESS
    };

    return katra_audit_log(&record);
}

/* ============================================================================
 * CLEANUP
 * ============================================================================ */

void katra_audit_free_record(audit_record_t* record) {
    if (!record) {
        return;
    }

    free(record->ci_id);
    free(record->target_ci_id);
    free(record->team_name);
    free(record->memory_id);
    free(record->details);

    /* Zero out struct */
    memset(record, 0, sizeof(audit_record_t));
}
