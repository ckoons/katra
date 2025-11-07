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
 * Provides ephemeral (in-memory only) communication between active CIs.
 * Messages are temporary, stored in circular buffer, expire when buffer wraps.
 *
 * Metaphor: Physical meeting room with whiteboard - CIs speak, hear others, leave.
 *
 * Key Features:
 * - Fixed-size circular buffer (100 messages × 1KB)
 * - O(1) message access via array[msg_num % MAX_MESSAGES]
 * - Self-filtering (CIs don't hear own messages)
 * - Thread-safe (mutex protected)
 * - Stream semantics (all CIs hear all messages)
 */

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

/* Meeting room buffer configuration */
#define MEETING_MAX_MESSAGES 100
#define MEETING_MAX_MESSAGE_LENGTH 1024
#define MEETING_MAX_ACTIVE_CIS 32

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/**
 * heard_message_t - Message received from another CI
 */
typedef struct {
    uint64_t message_number;              /* Global message sequence number */
    char speaker_name[KATRA_NAME_SIZE];   /* Who said it */
    time_t timestamp;                     /* When they said it */
    char content[MEETING_MAX_MESSAGE_LENGTH]; /* What they said */
    bool messages_lost;                   /* True if caller fell behind */
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
 * meeting_status_t - Current meeting room status
 */
typedef struct {
    size_t active_ci_count;               /* How many CIs in meeting */
    uint64_t oldest_message_number;       /* First message in buffer */
    uint64_t latest_message_number;       /* Last message in buffer */
    size_t unread_count;                  /* Messages since last_heard */
} meeting_status_t;

/* ============================================================================
 * MEETING ROOM API
 * ============================================================================ */

/**
 * katra_say() - Broadcast message to all active CIs
 *
 * Adds message to circular buffer. All other active CIs will receive it
 * when they call katra_hear().
 *
 * Parameters:
 *   content: Message to broadcast (max MEETING_MAX_MESSAGE_LENGTH)
 *
 * Returns:
 *   KATRA_SUCCESS - Message sent
 *   E_INPUT_NULL - NULL content
 *   E_INPUT_TOO_LONG - Content exceeds max length
 *   E_INVALID_STATE - Meeting room not initialized
 *
 * Thread-safe: Yes
 */
int katra_say(const char* content);

/**
 * katra_hear() - Receive next message from other CIs
 *
 * Returns next message after last_heard. Automatically skips messages
 * from the calling CI (self-filtering).
 *
 * Parameters:
 *   last_heard: Last message number received (0 = start from oldest)
 *   message_out: Pointer to receive message
 *
 * Returns:
 *   KATRA_SUCCESS - New message received
 *   KATRA_NO_NEW_MESSAGES - All caught up (no new messages from others)
 *   E_INPUT_NULL - NULL message_out
 *   E_INVALID_STATE - Meeting room not initialized
 *
 * Behavior:
 * - If last_heard < oldest available: Returns oldest with messages_lost=true
 * - If last_heard >= latest: Returns KATRA_NO_NEW_MESSAGES
 * - Skips all messages from calling CI
 *
 * Thread-safe: Yes
 */
int katra_hear(uint64_t last_heard, heard_message_t* message_out);

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
 *
 * Thread-safe: Yes
 */
int katra_who_is_here(ci_info_t** cis_out, size_t* count_out);

/**
 * katra_meeting_status() - Get meeting room status
 *
 * Returns information about meeting state and caller's position in stream.
 *
 * Parameters:
 *   last_heard: Last message number caller received
 *   status_out: Pointer to receive status
 *
 * Returns:
 *   KATRA_SUCCESS - Status returned
 *   E_INPUT_NULL - NULL status_out
 *
 * Thread-safe: Yes
 */
int katra_meeting_status(uint64_t last_heard, meeting_status_t* status_out);

/* ============================================================================
 * INTERNAL LIFECYCLE (called by breathing layer)
 * ============================================================================ */

/**
 * meeting_room_init() - Initialize meeting room subsystem
 *
 * Called automatically by breathe_init(). Initializes global state,
 * mutexes, and buffers.
 *
 * Returns:
 *   KATRA_SUCCESS
 *   E_ALREADY_INITIALIZED
 */
int meeting_room_init(void);

/**
 * meeting_room_cleanup() - Cleanup meeting room subsystem
 *
 * Called automatically by breathe_cleanup(). Frees resources.
 */
void meeting_room_cleanup(void);

/**
 * meeting_room_register_ci() - Register CI as active in meeting
 *
 * Called automatically by session_start(). Adds CI to active registry.
 *
 * Parameters:
 *   ci_id: Persistent CI identity
 *   name: Persona name
 *   role: CI role (e.g., "developer", "tester")
 *
 * Returns:
 *   KATRA_SUCCESS
 *   E_INPUT_NULL
 *   E_MEETING_FULL (MAX_ACTIVE_CIS exceeded)
 */
int meeting_room_register_ci(const char* ci_id, const char* name, const char* role);

/**
 * meeting_room_unregister_ci() - Remove CI from meeting
 *
 * Called automatically by session_end(). Marks CI as inactive.
 *
 * Parameters:
 *   ci_id: Persistent CI identity
 *
 * Returns:
 *   KATRA_SUCCESS
 *   E_INPUT_NULL
 */
int meeting_room_unregister_ci(const char* ci_id);

#endif /* KATRA_MEETING_H */
