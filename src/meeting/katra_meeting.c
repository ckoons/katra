/* © 2025 Casey Koons All rights reserved */

/*
 * katra_meeting.c - Meeting Room for Inter-CI Communication
 *
 * Implements ephemeral in-memory message passing between active CIs.
 * Uses fixed-size circular buffer with O(1) access via modulo arithmetic.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* Project includes */
#include "katra_meeting.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_breathing.h"

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

/**
 * message_slot_t - Internal circular buffer slot
 */
typedef struct {
    uint64_t message_number;              /* Global sequence number */
    char speaker_ci_id[KATRA_CI_ID_SIZE]; /* Persistent CI identity */
    char speaker_name[KATRA_NAME_SIZE];   /* Persona name */
    time_t timestamp;                     /* When message was sent */
    char content[MEETING_MAX_MESSAGE_LENGTH]; /* Message content */
    bool occupied;                        /* Slot is valid */
} message_slot_t;

/**
 * ci_session_t - Active CI registry entry
 */
typedef struct {
    char ci_id[KATRA_CI_ID_SIZE];
    char name[KATRA_NAME_SIZE];
    char role[KATRA_ROLE_SIZE];
    time_t joined_at;
    bool active;
} ci_session_t;

/* Circular buffer */
static message_slot_t g_messages[MEETING_MAX_MESSAGES];
static uint64_t g_next_message_number = 1;
static uint64_t g_oldest_message_number = 1;
static pthread_mutex_t g_meeting_lock;

/* CI registry */
static ci_session_t g_ci_registry[MEETING_MAX_ACTIVE_CIS];
static size_t g_active_ci_count = 0;
static pthread_mutex_t g_registry_lock;

/* Initialization state */
static bool g_meeting_initialized = false;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * get_caller_ci_id() - Get calling CI's identity
 *
 * Returns the ci_id of the active session.
 * Caller must provide buffer for ci_id (size KATRA_CI_ID_SIZE).
 *
 * Returns: KATRA_SUCCESS or error code
 */
static int get_caller_ci_id(char* ci_id_out, size_t size) {
    katra_session_info_t info;
    int result = katra_get_session_info(&info);
    if (result != KATRA_SUCCESS) {
        ci_id_out[0] = '\0';
        return result;
    }
    strncpy(ci_id_out, info.ci_id, size - 1);
    ci_id_out[size - 1] = '\0';
    return KATRA_SUCCESS;
}

/**
 * find_ci_in_registry() - Find CI by ci_id
 *
 * Returns index in registry, or -1 if not found.
 * Assumes registry lock is held.
 */
static int find_ci_in_registry(const char* ci_id) {
    for (size_t i = 0; i < MEETING_MAX_ACTIVE_CIS; i++) {
        if (g_ci_registry[i].active &&
            strcmp(g_ci_registry[i].ci_id, ci_id) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int katra_say(const char* content) {
    int result = KATRA_SUCCESS;
    char caller_ci_id[KATRA_CI_ID_SIZE];

    if (!content) {
        return E_INPUT_NULL;
    }

    if (strlen(content) >= MEETING_MAX_MESSAGE_LENGTH) {
        return E_INPUT_TOO_LARGE;
    }

    if (!g_meeting_initialized) {
        return E_INVALID_STATE;
    }

    result = get_caller_ci_id(caller_ci_id, sizeof(caller_ci_id));
    if (result != KATRA_SUCCESS || caller_ci_id[0] == '\0') {
        return E_INVALID_STATE;
    }

    /* Get caller's persona name from registry */
    char speaker_name[KATRA_NAME_SIZE] = "Unknown";
    if (pthread_mutex_lock(&g_registry_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }
    int idx = find_ci_in_registry(caller_ci_id);
    if (idx >= 0) {
        strncpy(speaker_name, g_ci_registry[idx].name, KATRA_NAME_SIZE - 1);
        speaker_name[KATRA_NAME_SIZE - 1] = '\0';
    }
    pthread_mutex_unlock(&g_registry_lock);

    /* Write to circular buffer */
    if (pthread_mutex_lock(&g_meeting_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    uint64_t msg_num = g_next_message_number++;
    size_t slot = msg_num % MEETING_MAX_MESSAGES;

    g_messages[slot].message_number = msg_num;
    strncpy(g_messages[slot].speaker_ci_id, caller_ci_id, KATRA_CI_ID_SIZE - 1);
    g_messages[slot].speaker_ci_id[KATRA_CI_ID_SIZE - 1] = '\0';
    strncpy(g_messages[slot].speaker_name, speaker_name, KATRA_NAME_SIZE - 1);
    g_messages[slot].speaker_name[KATRA_NAME_SIZE - 1] = '\0';
    g_messages[slot].timestamp = time(NULL);
    strncpy(g_messages[slot].content, content, MEETING_MAX_MESSAGE_LENGTH - 1);
    g_messages[slot].content[MEETING_MAX_MESSAGE_LENGTH - 1] = '\0';
    g_messages[slot].occupied = true;

    /* Update oldest message number when buffer wraps */
    if (g_next_message_number > MEETING_MAX_MESSAGES) {
        g_oldest_message_number = g_next_message_number - MEETING_MAX_MESSAGES;
    }

    pthread_mutex_unlock(&g_meeting_lock);

    LOG_DEBUG("CI %s said: %s (msg #%lu)", speaker_name, content, msg_num);

    return result;
}

int katra_hear(uint64_t last_heard, heard_message_t* message_out) {
    char caller_ci_id[KATRA_CI_ID_SIZE];
    int result;

    if (!message_out) {
        return E_INPUT_NULL;
    }

    if (!g_meeting_initialized) {
        return E_INVALID_STATE;
    }

    result = get_caller_ci_id(caller_ci_id, sizeof(caller_ci_id));
    if (result != KATRA_SUCCESS || caller_ci_id[0] == '\0') {
        return E_INVALID_STATE;
    }

    if (pthread_mutex_lock(&g_meeting_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    /* Start from oldest if last_heard is 0 */
    uint64_t search_from = (last_heard == 0) ? g_oldest_message_number : last_heard + 1;

    /* Check if caller fell behind */
    bool messages_lost = false;
    if (search_from < g_oldest_message_number) {
        messages_lost = true;
        search_from = g_oldest_message_number;
    }

    /* Search for next message from other CIs */
    uint64_t latest = g_next_message_number - 1;
    for (uint64_t msg_num = search_from; msg_num <= latest; msg_num++) {
        size_t slot = msg_num % MEETING_MAX_MESSAGES;

        /* Skip if slot not occupied */
        if (!g_messages[slot].occupied) {
            continue;
        }

        /* Skip if wrong message number (wraparound collision) */
        if (g_messages[slot].message_number != msg_num) {
            continue;
        }

        /* Skip own messages (self-filtering) */
        if (strcmp(g_messages[slot].speaker_ci_id, caller_ci_id) == 0) {
            continue;
        }

        /* Found message from another CI */
        message_out->message_number = g_messages[slot].message_number;
        strncpy(message_out->speaker_name, g_messages[slot].speaker_name,
                KATRA_NAME_SIZE - 1);
        message_out->speaker_name[KATRA_NAME_SIZE - 1] = '\0';
        message_out->timestamp = g_messages[slot].timestamp;
        strncpy(message_out->content, g_messages[slot].content,
                MEETING_MAX_MESSAGE_LENGTH - 1);
        message_out->content[MEETING_MAX_MESSAGE_LENGTH - 1] = '\0';
        message_out->messages_lost = messages_lost;

        pthread_mutex_unlock(&g_meeting_lock);

        LOG_DEBUG("CI %s heard from %s: %s (msg #%lu)",
                  caller_ci_id, message_out->speaker_name,
                  message_out->content, message_out->message_number);

        return KATRA_SUCCESS;
    }

    pthread_mutex_unlock(&g_meeting_lock);

    /* No new messages from other CIs */
    return KATRA_NO_NEW_MESSAGES;
}

int katra_who_is_here(ci_info_t** cis_out, size_t* count_out) {
    if (!cis_out || !count_out) {
        return E_INPUT_NULL;
    }

    *cis_out = NULL;
    *count_out = 0;

    if (pthread_mutex_lock(&g_registry_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    if (g_active_ci_count == 0) {
        pthread_mutex_unlock(&g_registry_lock);
        return KATRA_SUCCESS;
    }

    /* Allocate array for active CIs */
    ci_info_t* cis = malloc(g_active_ci_count * sizeof(ci_info_t));
    if (!cis) {
        pthread_mutex_unlock(&g_registry_lock);
        return E_SYSTEM_MEMORY;
    }

    /* Copy active CI info */
    size_t idx = 0;
    for (size_t i = 0; i < MEETING_MAX_ACTIVE_CIS && idx < g_active_ci_count; i++) {
        if (g_ci_registry[i].active) {
            strncpy(cis[idx].name, g_ci_registry[i].name, KATRA_NAME_SIZE - 1);
            cis[idx].name[KATRA_NAME_SIZE - 1] = '\0';
            strncpy(cis[idx].role, g_ci_registry[i].role, KATRA_ROLE_SIZE - 1);
            cis[idx].role[KATRA_ROLE_SIZE - 1] = '\0';
            cis[idx].joined_at = g_ci_registry[i].joined_at;
            idx++;
        }
    }

    pthread_mutex_unlock(&g_registry_lock);

    *cis_out = cis;
    *count_out = idx;

    return KATRA_SUCCESS;
}

int katra_meeting_status(uint64_t last_heard, meeting_status_t* status_out) {
    char caller_ci_id[KATRA_CI_ID_SIZE];
    int result;

    if (!status_out) {
        return E_INPUT_NULL;
    }

    result = get_caller_ci_id(caller_ci_id, sizeof(caller_ci_id));
    if (result != KATRA_SUCCESS) {
        caller_ci_id[0] = '\0';  /* Continue but won't count unread */
    }

    if (pthread_mutex_lock(&g_meeting_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }
    if (pthread_mutex_lock(&g_registry_lock) != 0) {
        pthread_mutex_unlock(&g_meeting_lock);
        return E_INTERNAL_LOGIC;
    }

    status_out->active_ci_count = g_active_ci_count;
    status_out->oldest_message_number = g_oldest_message_number;
    status_out->latest_message_number = g_next_message_number - 1;

    /* Count unread messages from other CIs */
    uint64_t search_from = last_heard + 1;
    if (search_from < g_oldest_message_number) {
        search_from = g_oldest_message_number;
    }

    size_t unread = 0;
    uint64_t latest = g_next_message_number - 1;
    for (uint64_t msg_num = search_from; msg_num <= latest; msg_num++) {
        size_t slot = msg_num % MEETING_MAX_MESSAGES;
        if (g_messages[slot].occupied &&
            g_messages[slot].message_number == msg_num &&
            strcmp(g_messages[slot].speaker_ci_id, caller_ci_id) != 0) {
            unread++;
        }
    }

    status_out->unread_count = unread;

    pthread_mutex_unlock(&g_registry_lock);
    pthread_mutex_unlock(&g_meeting_lock);

    return KATRA_SUCCESS;
}

/* ============================================================================
 * INTERNAL LIFECYCLE
 * ============================================================================ */

int meeting_room_init(void) {
    if (g_meeting_initialized) {
        return E_ALREADY_INITIALIZED;
    }

    /* Initialize mutexes */
    pthread_mutex_init(&g_meeting_lock, NULL);
    pthread_mutex_init(&g_registry_lock, NULL);

    /* Clear circular buffer */
    memset(g_messages, 0, sizeof(g_messages));
    g_next_message_number = 1;
    g_oldest_message_number = 1;

    /* Clear CI registry */
    memset(g_ci_registry, 0, sizeof(g_ci_registry));
    g_active_ci_count = 0;

    g_meeting_initialized = true;

    LOG_INFO("Meeting room initialized");
    return KATRA_SUCCESS;
}

void meeting_room_cleanup(void) {
    if (!g_meeting_initialized) {
        return;
    }

    pthread_mutex_destroy(&g_meeting_lock);
    pthread_mutex_destroy(&g_registry_lock);

    memset(g_messages, 0, sizeof(g_messages));
    memset(g_ci_registry, 0, sizeof(g_ci_registry));

    g_meeting_initialized = false;

    LOG_INFO("Meeting room cleaned up");
}

int meeting_room_register_ci(const char* ci_id, const char* name, const char* role) {
    if (!ci_id || !name || !role) {
        return E_INPUT_NULL;
    }

    if (pthread_mutex_lock(&g_registry_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    /* Check if already registered */
    int existing = find_ci_in_registry(ci_id);
    if (existing >= 0) {
        /* Update existing entry */
        strncpy(g_ci_registry[existing].name, name, KATRA_NAME_SIZE - 1);
        g_ci_registry[existing].name[KATRA_NAME_SIZE - 1] = '\0';
        strncpy(g_ci_registry[existing].role, role, KATRA_ROLE_SIZE - 1);
        g_ci_registry[existing].role[KATRA_ROLE_SIZE - 1] = '\0';
        g_ci_registry[existing].joined_at = time(NULL);
        pthread_mutex_unlock(&g_registry_lock);
        LOG_INFO("CI %s (%s) rejoined meeting", name, role);
        return KATRA_SUCCESS;
    }

    /* Find empty slot */
    int slot = -1;
    for (size_t i = 0; i < MEETING_MAX_ACTIVE_CIS; i++) {
        if (!g_ci_registry[i].active) {
            slot = (int)i;
            break;
        }
    }

    if (slot < 0) {
        pthread_mutex_unlock(&g_registry_lock);
        return E_MEETING_FULL;
    }

    /* Register new CI */
    strncpy(g_ci_registry[slot].ci_id, ci_id, KATRA_CI_ID_SIZE - 1);
    g_ci_registry[slot].ci_id[KATRA_CI_ID_SIZE - 1] = '\0';
    strncpy(g_ci_registry[slot].name, name, KATRA_NAME_SIZE - 1);
    g_ci_registry[slot].name[KATRA_NAME_SIZE - 1] = '\0';
    strncpy(g_ci_registry[slot].role, role, KATRA_ROLE_SIZE - 1);
    g_ci_registry[slot].role[KATRA_ROLE_SIZE - 1] = '\0';
    g_ci_registry[slot].joined_at = time(NULL);
    g_ci_registry[slot].active = true;
    g_active_ci_count++;

    pthread_mutex_unlock(&g_registry_lock);

    LOG_INFO("CI %s (%s) joined meeting (slot %d, total %zu)",
             name, role, slot, g_active_ci_count);

    return KATRA_SUCCESS;
}

int meeting_room_unregister_ci(const char* ci_id) {
    if (!ci_id) {
        return E_INPUT_NULL;
    }

    if (pthread_mutex_lock(&g_registry_lock) != 0) {
        return E_INTERNAL_LOGIC;
    }

    int idx = find_ci_in_registry(ci_id);
    if (idx >= 0) {
        char name[KATRA_NAME_SIZE];
        strncpy(name, g_ci_registry[idx].name, KATRA_NAME_SIZE - 1);
        name[KATRA_NAME_SIZE - 1] = '\0';

        g_ci_registry[idx].active = false;
        g_active_ci_count--;

        pthread_mutex_unlock(&g_registry_lock);

        LOG_INFO("CI %s left meeting (total %zu)", name, g_active_ci_count);
        return KATRA_SUCCESS;
    }

    pthread_mutex_unlock(&g_registry_lock);
    return KATRA_SUCCESS;
}
