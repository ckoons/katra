/* © 2025 Casey Koons All rights reserved */

/*
 * katra_chat_internal.h - Internal definitions for chat subsystem
 *
 * Private header shared among chat implementation files.
 * Not exposed to external clients.
 */

#ifndef KATRA_CHAT_INTERNAL_H
#define KATRA_CHAT_INTERNAL_H

#include <stdbool.h>
#include <sqlite3.h>
#include <pthread.h>
#include "katra_limits.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define CHAT_DB_FILENAME "chat.db"
#define CHAT_SQL_BUFFER_SIZE 4096
/* SECONDS_PER_HOUR defined in katra_limits.h */

/* Phase 4.5.1: Registry heartbeat timeout (5 minutes) */
#define STALE_REGISTRATION_TIMEOUT_MINUTES 5

/* ============================================================================
 * GLOBAL STATE (defined in katra_chat_registry.c)
 * ============================================================================ */

extern sqlite3* g_chat_db;
extern bool g_chat_initialized;
extern pthread_mutex_t g_chat_lock;

/* ============================================================================
 * SQL SCHEMA (defined in katra_chat_registry.c)
 * ============================================================================ */

extern const char* CHAT_SCHEMA_MESSAGES;
extern const char* CHAT_SCHEMA_QUEUES;
extern const char* CHAT_SCHEMA_REGISTRY;

/* ============================================================================
 * HELPER FUNCTIONS (implemented in katra_chat_helpers.c)
 * ============================================================================ */

/**
 * get_caller_ci_id() - Get calling CI's identity
 */
int get_caller_ci_id(char* ci_id_out, size_t size);

/**
 * get_caller_name() - Get calling CI's name from MCP session
 */
void get_caller_name(char* name_out, size_t size);

/**
 * case_insensitive_compare() - Compare strings case-insensitively
 */
int case_insensitive_compare(const char* s1, const char* s2);

/**
 * is_broadcast() - Check if recipients string means broadcast
 */
bool is_broadcast(const char* recipients);

/**
 * resolve_ci_name_to_id() - Resolve CI name to ci_id (case-insensitive)
 */
int resolve_ci_name_to_id(const char* name, char* ci_id_out, size_t size);

/**
 * get_active_ci_ids() - Get array of all active CI IDs
 */
int get_active_ci_ids(char*** ci_ids_out, size_t* count_out);

/**
 * parse_recipients() - Parse comma-separated recipient list
 *
 * Returns array of ci_ids. Unknown names are logged and skipped.
 */
int parse_recipients(const char* recipients_str, const char* sender_ci_id,
                    char*** ci_ids_out, size_t* count_out);

#endif /* KATRA_CHAT_INTERNAL_H */
