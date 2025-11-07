/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_MEETING_H
#define KATRA_MEETING_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "katra_error.h"
#include "katra_limits.h"

/**
 * katra_meeting.h - Meeting Room for Inter-CI Communication
 *
 * Provides ephemeral database-backed chat for active CIs across multiple processes.
 * Messages stored in SQLite with 2-hour TTL, enabling async multi-process communication.
 *
 * Metaphor: Persistent chat room - CIs send messages that others retrieve from shared queue.
 *
 * Key Features:
 * - SQLite-backed (multi-process safe via file locking)
 * - Personal message queues (each CI drains their own queue)
 * - Broadcast and direct messaging ("alice,bob,charlie" or "broadcast")
 * - 2-hour message TTL (auto-cleanup on MCP startup)
 * - Self-filtering (CIs don't receive own messages)
 * - Ephemeral (broadcasts persist for history, DMs deleted on read)
 */

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

/* Meeting room configuration */
#define MEETING_MAX_MESSAGE_LENGTH 1024
#define MEETING_MAX_ACTIVE_CIS 32
#define MEETING_MESSAGE_TTL_HOURS 2
#define MEETING_DEFAULT_HISTORY_COUNT 10
#define MEETING_MAX_HISTORY_COUNT 100

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/**
 * heard_message_t - Message received from another CI
 */
typedef struct {
    uint64_t message_id;                  /* Database message ID */
    char speaker_name[KATRA_NAME_SIZE];   /* Who said it */
    time_t timestamp;                     /* When they said it */
    char content[MEETING_MAX_MESSAGE_LENGTH]; /* What they said */
    char recipients[KATRA_BUFFER_SMALL];  /* "broadcast" or "alice,bob" */
    bool is_direct_message;               /* True if not broadcast */
    bool more_available;                  /* True if more messages queued */
} heard_message_t;

/**
 * ci_info_t - Information about active CI in meeting
 */
typedef struct {
    char name[KATRA_NAME_SIZE];
    char role[KATRA_ROLE_SIZE];
    time_t joined_at;
} ci_info_t;

/**
 * history_message_t - Broadcast message from history
 */
typedef struct {
    char speaker_name[KATRA_NAME_SIZE];   /* Who said it */
    char content[MEETING_MAX_MESSAGE_LENGTH]; /* What they said */
    time_t timestamp;                     /* When they said it */
} history_message_t;

/* ============================================================================
 * MEETING ROOM API
 * ============================================================================ */

/**
 * katra_say() - Send message to recipient(s)
 *
 * Stores message in database and queues to recipients. Broadcasts are stored
 * in global history (2-hour TTL), direct messages only in recipient queues.
 *
 * Parameters:
 *   content: Message to send (max MEETING_MAX_MESSAGE_LENGTH)
 *   recipients: NULL/""/broadcast" for all, or "alice,bob,charlie" for specific CIs
 *               (case-insensitive, forgiving parse)
 *
 * Returns:
 *   KATRA_SUCCESS - Message sent (unknown recipients are logged and skipped)
 *   E_INPUT_NULL - NULL content
 *   E_INPUT_TOO_LARGE - Content exceeds max length
 *   E_INVALID_STATE - Meeting room not initialized
 *   E_SYSTEM_DATABASE - Database error
 *
 * Thread-safe: Yes
 */
int katra_say(const char* content, const char* recipients);

/**
 * katra_hear() - Receive next message from personal queue
 *
 * Returns next message from caller's personal queue and deletes it.
 * Sets more_available flag if additional messages are queued.
 *
 * Parameters:
 *   message_out: Pointer to receive message
 *
 * Returns:
 *   KATRA_SUCCESS - Message received and deleted from queue
 *   KATRA_NO_NEW_MESSAGES - Queue is empty
 *   E_INPUT_NULL - NULL message_out
 *   E_INVALID_STATE - Meeting room not initialized
 *   E_SYSTEM_DATABASE - Database error
 *
 * Behavior:
 * - Retrieves oldest message from personal queue
 * - Deletes message from queue (read-once)
 * - Sets more_available flag based on remaining queue depth
 *
 * Thread-safe: Yes
 */
int katra_hear(heard_message_t* message_out);

/**
 * katra_who_is_here() - List all active CIs in meeting
 *
 * Returns array of CI information for all active participants.
 *
 * Parameters:
 *   cis_out: Pointer to receive array (caller must free)
 *   count_out: Pointer to receive count
 *
 * Returns:
 *   KATRA_SUCCESS - List returned (even if empty)
 *   E_INPUT_NULL - NULL parameters
 *   E_SYSTEM_MEMORY - Allocation failed
 *   E_SYSTEM_DATABASE - Database error
 *
 * Thread-safe: Yes
 */
int katra_who_is_here(ci_info_t** cis_out, size_t* count_out);

/**
 * katra_get_history() - Retrieve recent broadcast messages
 *
 * Returns recent broadcast messages for context (e.g., when joining conversation).
 * Only returns broadcasts, not direct messages (privacy).
 *
 * Parameters:
 *   count: Number of messages to retrieve (0 = default, capped at MEETING_MAX_HISTORY_COUNT)
 *   messages_out: Pointer to receive array (caller must free with katra_free_history)
 *   count_out: Pointer to receive actual count
 *
 * Returns:
 *   KATRA_SUCCESS - History returned (even if empty)
 *   E_INPUT_NULL - NULL parameters
 *   E_SYSTEM_MEMORY - Allocation failed
 *   E_SYSTEM_DATABASE - Database error
 *
 * Thread-safe: Yes
 */
int katra_get_history(size_t count, history_message_t** messages_out, size_t* count_out);

/**
 * katra_free_history() - Free history array
 *
 * Frees array returned by katra_get_history().
 *
 * Parameters:
 *   messages: Array to free
 *   count: Number of messages in array
 */
void katra_free_history(history_message_t* messages, size_t count);

/* ============================================================================
 * INTERNAL LIFECYCLE (called by MCP server)
 * ============================================================================ */

/**
 * meeting_room_init() - Initialize meeting room database
 *
 * Called by MCP server on startup. Creates database tables and runs cleanup.
 *
 * Returns:
 *   KATRA_SUCCESS
 *   E_ALREADY_INITIALIZED
 *   E_SYSTEM_DATABASE - Database creation failed
 */
int meeting_room_init(void);

/**
 * meeting_room_cleanup() - Cleanup meeting room subsystem
 *
 * Called by MCP server on shutdown. Closes database.
 */
void meeting_room_cleanup(void);

/**
 * meeting_room_register_ci() - Register CI as active in meeting
 *
 * Called by katra_register MCP tool. Adds CI to active registry.
 *
 * Parameters:
 *   ci_id: Persistent CI identity
 *   name: Persona name
 *   role: CI role (e.g., "developer", "tester")
 *
 * Returns:
 *   KATRA_SUCCESS
 *   E_INPUT_NULL
 *   E_SYSTEM_DATABASE - Database error
 */
int meeting_room_register_ci(const char* ci_id, const char* name, const char* role);

/**
 * meeting_room_unregister_ci() - Remove CI from meeting
 *
 * Called by katra_register when re-registering. Marks CI as inactive.
 *
 * Parameters:
 *   ci_id: Persistent CI identity
 *
 * Returns:
 *   KATRA_SUCCESS
 *   E_INPUT_NULL
 *   E_SYSTEM_DATABASE - Database error
 */
int meeting_room_unregister_ci(const char* ci_id);

/**
 * katra_cleanup_old_messages() - Delete messages older than TTL
 *
 * Called by MCP server on startup. Deletes messages older than 2 hours.
 *
 * Returns:
 *   KATRA_SUCCESS
 *   E_SYSTEM_DATABASE - Database error
 */
int katra_cleanup_old_messages(void);

#endif /* KATRA_MEETING_H */
