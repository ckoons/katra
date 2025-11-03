/* © 2025 Casey Koons All rights reserved */

/*
 * katra_breathing_reflection.c - End-of-turn and end-of-session reflection
 *
 * Implements conscious curation of memories through reflection:
 * - Turn tracking: Memories created in current interaction cycle
 * - Session tracking: All memories from current session
 * - Metadata updates: Change personal/collection/archival flags
 * - Content revision: Update memory content after reflection
 *
 * Design philosophy: Simple mechanisms, emergent behavior, real people
 * CIs can review recent memories and consciously decide what matters.
 */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Project includes */
#include "katra_breathing.h"
#include "katra_memory.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_core_common.h"
#include "katra_breathing_internal.h"

/* Turn tracking external state (defined in katra_breathing.c) */
extern int g_current_turn;
extern char** g_turn_memory_ids;
extern size_t g_turn_memory_count;
extern size_t g_turn_memory_capacity;

/* Turn state tracking (internal) */
static turn_state_t g_turn_state = TURN_STATE_IDLE;

/* ============================================================================
 * TURN TRACKING HELPERS (Internal)
 * ============================================================================ */

/**
 * track_memory_in_turn() - Add memory ID to current turn list
 *
 * Called automatically when remember() stores a memory.
 * Grows the array as needed.
 */
int track_memory_in_turn(const char* record_id) {
    if (!record_id) {
        return E_INPUT_NULL;
    }

    /* Grow array if needed */
    if (g_turn_memory_count >= g_turn_memory_capacity) {
        size_t new_capacity = g_turn_memory_capacity == 0 ?
                              BREATHING_DEFAULT_TURN_CAPACITY :
                              g_turn_memory_capacity * BREATHING_GROWTH_FACTOR;

        char** new_array = realloc(g_turn_memory_ids,
                                   new_capacity * sizeof(char*));
        if (!new_array) {
            katra_report_error(E_SYSTEM_MEMORY, "track_memory_in_turn",
                             "Failed to grow turn memory array");
            return E_SYSTEM_MEMORY;
        }

        g_turn_memory_ids = new_array;
        g_turn_memory_capacity = new_capacity;
    }

    /* Add memory ID to turn list */
    g_turn_memory_ids[g_turn_memory_count] = strdup(record_id);
    if (!g_turn_memory_ids[g_turn_memory_count]) {
        katra_report_error(E_SYSTEM_MEMORY, "track_memory_in_turn",
                         "Failed to duplicate record_id");
        return E_SYSTEM_MEMORY;
    }

    g_turn_memory_count++;
    return KATRA_SUCCESS;
}

/**
 * clear_turn_memories() - Clear turn memory list
 *
 * Called at start of new turn to reset tracking.
 */
static void clear_turn_memories(void) {
    for (size_t i = 0; i < g_turn_memory_count; i++) {
        free(g_turn_memory_ids[i]);
    }
    g_turn_memory_count = 0;
}

/* ============================================================================
 * TURN MANAGEMENT API
 * ============================================================================ */

/**
 * begin_turn() - Start a new turn (explicit boundary)
 *
 * Call this to mark the start of a new interaction turn.
 * Increments turn counter and clears previous turn's memory list.
 *
 * Returns: KATRA_SUCCESS or error code
 */
int begin_turn(void) {
    if (!breathing_get_initialized()) {
        return E_INVALID_STATE;
    }

    /* Clear previous turn's memories */
    clear_turn_memories();

    /* Increment turn counter and activate turn */
    g_current_turn++;
    g_turn_state = TURN_STATE_ACTIVE;

    LOG_INFO("Turn %d started", g_current_turn);
    return KATRA_SUCCESS;
}

/**
 * end_turn() - End the current turn
 *
 * Marks end of interaction turn and clears turn memory list.
 * After this call, get_memories_this_turn() returns empty.
 *
 * Returns: KATRA_SUCCESS or error code
 */
int end_turn(void) {
    if (!breathing_get_initialized()) {
        return E_INVALID_STATE;
    }

    g_turn_state = TURN_STATE_IDLE;
    clear_turn_memories();
    LOG_INFO("Turn %d ended", g_current_turn);
    return KATRA_SUCCESS;
}

/**
 * get_current_turn() - Get current turn number
 *
 * Returns: Current turn number (0 if no session active)
 */
int get_current_turn(void) {
    return g_current_turn;
}

/**
 * get_turn_state() - Get current turn state
 *
 * Returns: TURN_STATE_IDLE or TURN_STATE_ACTIVE
 */
turn_state_t get_turn_state(void) {
    return g_turn_state;
}

/**
 * get_current_turn_id() - Get current turn ID as string
 *
 * Returns string representation of current turn number.
 * Caller must NOT free the returned pointer (static buffer).
 *
 * Returns: Turn ID string, or empty string if no active turn
 */
const char* get_current_turn_id(void) {
    static char turn_id_buffer[32];

    if (g_turn_state == TURN_STATE_IDLE || g_current_turn == 0) {
        return "";
    }

    snprintf(turn_id_buffer, sizeof(turn_id_buffer), "turn_%d", g_current_turn);
    return turn_id_buffer;
}

/* ============================================================================
 * REFLECTION QUERY API
 * ============================================================================ */

/**
 * get_memories_this_turn() - Get memory IDs from current turn
 *
 * Returns array of record IDs for memories created this turn.
 * CI can review these and decide about importance/personal/collection.
 *
 * Memory ownership: Caller must call free_memory_list() when done.
 *
 * Returns: Array of strings (caller owns), or NULL if no memories
 */
char** get_memories_this_turn(size_t* count) {
    if (!count) {
        return NULL;
    }

    if (!breathing_get_initialized()) {
        *count = 0;
        return NULL;
    }

    *count = g_turn_memory_count;

    if (g_turn_memory_count == 0) {
        return NULL;
    }

    /* Copy memory IDs array */
    char** result = malloc(g_turn_memory_count * sizeof(char*));
    if (!result) {
        katra_report_error(E_SYSTEM_MEMORY, "get_memories_this_turn",
                         "Failed to allocate result array");
        *count = 0;
        return NULL;
    }

    for (size_t i = 0; i < g_turn_memory_count; i++) {
        result[i] = strdup(g_turn_memory_ids[i]);
        if (!result[i]) {
            /* Cleanup on failure */
            for (size_t j = 0; j < i; j++) {
                free(result[j]);
            }
            free(result);
            katra_report_error(E_SYSTEM_MEMORY, "get_memories_this_turn",
                             "Failed to duplicate memory ID");
            *count = 0;
            return NULL;
        }
    }

    LOG_INFO("Retrieved %zu memories from turn %d", *count, g_current_turn);
    return result;
}

/**
 * get_memories_this_session() - Get all memory IDs from current session
 *
 * Returns array of record IDs for all memories created this session.
 * CI can review the full session at session_end().
 *
 * Memory ownership: Caller must call free_memory_list() when done.
 *
 * Returns: Array of strings (caller owns), or NULL if no memories
 */
char** get_memories_this_session(size_t* count) {
    if (!count) {
        return NULL;
    }

    if (!breathing_get_initialized()) {
        *count = 0;
        return NULL;
    }

    const char* ci_id = breathing_get_ci_id();
    const char* session_id = breathing_get_session_id();

    if (!ci_id || !session_id) {
        *count = 0;
        return NULL;
    }

    /* Query all memories from current session */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = 0,  /* All types */
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 0  /* No limit */
    };

    memory_record_t** records = NULL;
    size_t record_count = 0;

    int result = katra_memory_query(&query, &records, &record_count);
    if (result != KATRA_SUCCESS) {
        *count = 0;
        return NULL;
    }

    /* Filter by session_id and extract record_ids */
    char** memory_ids = malloc(record_count * sizeof(char*));
    if (!memory_ids) {
        katra_memory_free_results(records, record_count);
        *count = 0;
        return NULL;
    }

    size_t match_count = 0;
    for (size_t i = 0; i < record_count; i++) {
        if (records[i]->session_id &&
            strcmp(records[i]->session_id, session_id) == 0) {
            memory_ids[match_count] = strdup(records[i]->record_id);
            if (!memory_ids[match_count]) {
                /* Cleanup on failure */
                for (size_t j = 0; j < match_count; j++) {
                    free(memory_ids[j]);
                }
                free(memory_ids);
                katra_memory_free_results(records, record_count);
                *count = 0;
                return NULL;
            }
            match_count++;
        }
    }

    katra_memory_free_results(records, record_count);

    *count = match_count;
    LOG_INFO("Retrieved %zu memories from session %s", match_count, session_id);

    return memory_ids;
}

/* ============================================================================
 * METADATA UPDATE API
 * ============================================================================ */

/**
 * Helper: Load a single memory record by record_id
 *
 * Queries all memories and finds the matching record_id.
 * Caller must free the returned record with katra_memory_free_record().
 */
static memory_record_t* load_memory_by_id(const char* record_id) {
    if (!record_id) {
        return NULL;
    }

    const char* ci_id = breathing_get_ci_id();
    if (!ci_id) {
        return NULL;
    }

    /* Query all memories for this CI */
    memory_query_t query = {
        .ci_id = ci_id,
        .start_time = 0,
        .end_time = 0,
        .type = 0,
        .min_importance = 0.0,
        .tier = KATRA_TIER1,
        .limit = 0
    };

    memory_record_t** results = NULL;
    size_t count = 0;

    int result = katra_memory_query(&query, &results, &count);
    if (result != KATRA_SUCCESS) {
        return NULL;
    }

    /* Find matching record_id */
    memory_record_t* found = NULL;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(results[i]->record_id, record_id) == 0) {
            /* Found it - copy the record */
            found = katra_memory_create_record(
                results[i]->ci_id,
                results[i]->type,
                results[i]->content,
                results[i]->importance
            );

            if (found) {
                /* Copy all fields */
                found->timestamp = results[i]->timestamp;
                found->tier = results[i]->tier;
                found->archived = results[i]->archived;
                found->turn_id = results[i]->turn_id;
                found->personal = results[i]->personal;
                found->not_to_archive = results[i]->not_to_archive;
                found->last_reviewed = results[i]->last_reviewed;
                found->review_count = results[i]->review_count;

                /* Copy optional string fields */
                if (results[i]->collection) {
                    found->collection = strdup(results[i]->collection);
                }
                if (results[i]->session_id) {
                    free(found->session_id);
                    found->session_id = strdup(results[i]->session_id);
                }
                if (results[i]->importance_note) {
                    found->importance_note = strdup(results[i]->importance_note);
                }
            }
            break;
        }
    }

    katra_memory_free_results(results, count);
    return found;
}

/**
 * update_memory_metadata() - Update memory metadata after reflection
 */
int update_memory_metadata(const char* record_id,
                           const bool* personal,
                           const bool* not_to_archive,
                           const char* collection) {
    if (!record_id) {
        return E_INPUT_NULL;
    }

    if (!breathing_get_initialized()) {
        return E_INVALID_STATE;
    }

    /* Validate at least one metadata field is provided */
    if (!personal && !not_to_archive && !collection) {
        katra_report_error(E_INPUT_NULL, "update_memory_metadata",
                         "At least one metadata field must be provided");
        return E_INPUT_NULL;
    }

    /* Load the memory record */
    memory_record_t* record = load_memory_by_id(record_id);
    if (!record) {
        katra_report_error(E_NOT_FOUND, "update_memory_metadata",
                         "Memory record not found");
        return E_NOT_FOUND;
    }

    /* Update metadata fields if provided */
    if (personal) {
        record->personal = *personal;
    }

    if (not_to_archive) {
        record->not_to_archive = *not_to_archive;
    }

    if (collection) {
        free(record->collection);
        record->collection = strdup(collection);
        if (!record->collection) {
            katra_memory_free_record(record);
            return E_SYSTEM_MEMORY;
        }
    }

    /* Store updated record */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Updated metadata for memory %s", record_id);
    }

    return result;
}

/**
 * revise_memory_content() - Update memory content after reflection
 */
int revise_memory_content(const char* record_id, const char* new_content) {
    if (!record_id || !new_content) {
        return E_INPUT_NULL;
    }

    if (!breathing_get_initialized()) {
        return E_INVALID_STATE;
    }

    /* Load the memory record */
    memory_record_t* record = load_memory_by_id(record_id);
    if (!record) {
        katra_report_error(E_NOT_FOUND, "revise_memory_content",
                         "Memory record not found");
        return E_NOT_FOUND;
    }

    /* Update content */
    free(record->content);
    record->content = strdup(new_content);
    if (!record->content) {
        katra_memory_free_record(record);
        return E_SYSTEM_MEMORY;
    }

    /* Store updated record */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Revised content for memory %s", record_id);
    }

    return result;
}

/**
 * review_memory() - Mark memory as reviewed
 */
int review_memory(const char* record_id) {
    if (!record_id) {
        return E_INPUT_NULL;
    }

    if (!breathing_get_initialized()) {
        return E_INVALID_STATE;
    }

    /* Load the memory record */
    memory_record_t* record = load_memory_by_id(record_id);
    if (!record) {
        katra_report_error(E_NOT_FOUND, "review_memory",
                         "Memory record not found");
        return E_NOT_FOUND;
    }

    /* Update review metadata */
    record->last_reviewed = time(NULL);
    record->review_count++;

    /* Store updated record */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Reviewed memory %s (count: %d)", record_id, record->review_count);
    }

    return result;
}

/**
 * add_to_personal_collection() - Add memory to personal collection
 */
int add_to_personal_collection(const char* record_id, const char* collection_path) {
    if (!record_id || !collection_path) {
        return E_INPUT_NULL;
    }

    bool is_personal = true;
    return update_memory_metadata(record_id, &is_personal, NULL, collection_path);
}

/**
 * remove_from_personal_collection() - Remove memory from personal collection
 */
int remove_from_personal_collection(const char* record_id) {
    if (!record_id) {
        return E_INPUT_NULL;
    }

    if (!breathing_get_initialized()) {
        return E_INVALID_STATE;
    }

    /* Load the memory record */
    memory_record_t* record = load_memory_by_id(record_id);
    if (!record) {
        katra_report_error(E_NOT_FOUND, "remove_from_personal_collection",
                         "Memory record not found");
        return E_NOT_FOUND;
    }

    /* Clear personal metadata */
    record->personal = false;
    record->not_to_archive = false;
    free(record->collection);
    record->collection = NULL;

    /* Store updated record */
    int result = katra_memory_store(record);
    katra_memory_free_record(record);

    if (result == KATRA_SUCCESS) {
        LOG_DEBUG("Removed memory %s from personal collection", record_id);
    }

    return result;
}

/* ============================================================================
 * CLEANUP
 * ============================================================================ */

/**
 * cleanup_turn_tracking() - Free turn tracking resources
 *
 * Called during breathe_cleanup().
 */
void cleanup_turn_tracking(void) {
    clear_turn_memories();
    free(g_turn_memory_ids);
    g_turn_memory_ids = NULL;
    g_turn_memory_capacity = 0;
    g_current_turn = 0;
}
