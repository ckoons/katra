/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_MEMORY_H
#define KATRA_MEMORY_H

#include <stdbool.h>
#include <time.h>

/* Memory tier identifiers */
typedef enum {
    KATRA_TIER1 = 1,    /* Raw recordings (days to weeks) */
    KATRA_TIER2 = 2,    /* Sleep digests (weeks to months) */
    KATRA_TIER3 = 3     /* Pattern summaries (months to years) */
} katra_tier_t;

/* Memory record types
 *
 * These categories align with how CIs naturally organize thoughts:
 * - EXPERIENCE: What happened (events, interactions, observations)
 * - KNOWLEDGE: What I learned (facts, skills, understanding)
 * - REFLECTION: What I think about it (analysis, insights, meaning)
 * - PATTERN: What I've noticed (recurring themes, connections)
 * - GOAL: What I want to do (intentions, plans, aspirations)
 * - DECISION: What I decided and why (choices with reasoning)
 */
typedef enum {
    MEMORY_TYPE_EXPERIENCE = 1,    /* What happened */
    MEMORY_TYPE_KNOWLEDGE = 2,     /* What I learned */
    MEMORY_TYPE_REFLECTION = 3,    /* What I think about it */
    MEMORY_TYPE_PATTERN = 4,       /* What I've noticed */
    MEMORY_TYPE_GOAL = 5,          /* What I want to do */
    MEMORY_TYPE_DECISION = 6       /* What I decided and why */
} memory_type_t;

/* Memory importance levels (0.0 = trivial, 1.0 = critical) */
#define MEMORY_IMPORTANCE_TRIVIAL    0.0
#define MEMORY_IMPORTANCE_LOW        0.25
#define MEMORY_IMPORTANCE_MEDIUM     0.50
#define MEMORY_IMPORTANCE_HIGH       0.75
#define MEMORY_IMPORTANCE_CRITICAL   1.0

/* Memory record structure
 *
 * This is the fundamental unit of persistent memory in Katra.
 * Every interaction, experience, and thought is captured as a memory record.
 */
typedef struct {
    char* record_id;           /* Unique identifier */
    time_t timestamp;          /* When memory was created */

    memory_type_t type;        /* Type of memory */
    float importance;          /* 0.0-1.0 importance score */
    char* importance_note;     /* Why this importance level? (optional) */

    char* content;             /* Memory content (user input or experience) */
    char* response;            /* CI response (if interaction) */
    char* context;             /* Additional context (JSON format) */

    char* ci_id;               /* Which CI this memory belongs to */
    char* session_id;          /* Session identifier */
    char* component;           /* Which Tekton component created this */

    katra_tier_t tier;         /* Which tier this memory is stored in */
    bool archived;             /* Has this been moved to higher tier? */

    /* Thane's recommendations - Phase 1: Context-aware consolidation */
    time_t last_accessed;      /* Access-based decay: when last queried */
    size_t access_count;       /* Access-based decay: query frequency */
    float emotion_intensity;   /* Emotional salience: 0.0-1.0 */
    char* emotion_type;        /* Emotional salience: joy/surprise/confusion */
    bool marked_important;     /* Voluntary consent: "remember forever" */
    bool marked_forgettable;   /* Voluntary consent: "okay to forget" */

    /* Thane's recommendations - Phase 2: Connection graph */
    char** connected_memory_ids; /* Array of record IDs this memory references */
    size_t connection_count;     /* Number of connections */
    float graph_centrality;      /* Graph centrality score (0.0-1.0) */
} memory_record_t;

/* Memory query parameters */
typedef struct {
    const char* ci_id;         /* Filter by CI (required) */
    time_t start_time;         /* Start of time range (0 = no limit) */
    time_t end_time;           /* End of time range (0 = no limit) */
    memory_type_t type;        /* Filter by type (0 = all types) */
    float min_importance;      /* Minimum importance (0.0 = all) */
    katra_tier_t tier;         /* Which tier to search (0 = all tiers) */
    size_t limit;              /* Maximum results (0 = no limit) */
} memory_query_t;

/* Memory statistics */
typedef struct {
    size_t total_records;      /* Total memory records */
    size_t tier1_records;      /* Raw recordings */
    size_t tier2_records;      /* Sleep digests */
    size_t tier3_records;      /* Pattern summaries */

    size_t bytes_used;         /* Total storage used */
    time_t oldest_memory;      /* Oldest memory timestamp */
    time_t newest_memory;      /* Newest memory timestamp */
} memory_stats_t;

/* Initialize memory subsystem
 *
 * Must be called after katra_init() and before any memory operations.
 * Creates directory structure and loads configuration.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if directories cannot be created
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_memory_init(const char* ci_id);

/* Cleanup memory subsystem
 *
 * Flushes pending writes and releases resources.
 * Safe to call multiple times.
 */
void katra_memory_cleanup(void);

/* Store memory record
 *
 * Stores a memory record in the appropriate tier.
 * Records are written immediately (no buffering by default).
 *
 * Parameters:
 *   record - Memory record to store (must be fully initialized)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if record is NULL
 *   E_SYSTEM_FILE if write fails
 *   E_MEMORY_TIER_FULL if tier is full
 */
int katra_memory_store(const memory_record_t* record);

/* Query memory records
 *
 * Searches memory tiers based on query parameters.
 * Results are returned in reverse chronological order (newest first).
 *
 * Parameters:
 *   query - Query parameters
 *   results - Array to receive results (caller must free each record)
 *   count - Number of results returned
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if query or results is NULL
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_memory_query(const memory_query_t* query,
                       memory_record_t*** results,
                       size_t* count);

/* Get memory statistics
 *
 * Returns statistics about memory usage for a CI.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   stats - Statistics structure to fill
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id or stats is NULL
 */
int katra_memory_stats(const char* ci_id, memory_stats_t* stats);

/* Archive old memories
 *
 * Moves old Tier 1 memories to Tier 2 based on age and importance.
 * This is part of the memory consolidation process.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   max_age_days - Archive memories older than this many days
 *   archived_count - (output) Number of records archived (may be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id is NULL
 *   E_INVALID_STATE if memory subsystem not initialized
 *   E_INTERNAL_NOTIMPL if archival not yet implemented
 */
int katra_memory_archive(const char* ci_id, int max_age_days, size_t* archived_count);

/* Create memory record (helper)
 *
 * Allocates and initializes a memory record.
 * Caller must free with katra_memory_free_record().
 *
 * Parameters:
 *   ci_id - CI identifier
 *   type - Memory type
 *   content - Memory content
 *   importance - Importance score (0.0-1.0)
 *
 * Returns:
 *   Allocated memory record or NULL on failure
 */
memory_record_t* katra_memory_create_record(const char* ci_id,
                                             memory_type_t type,
                                             const char* content,
                                             float importance);

/* Free memory record
 *
 * Frees all allocated memory in a record.
 * Safe to call with NULL.
 */
void katra_memory_free_record(memory_record_t* record);

/* Free memory query results
 *
 * Frees array of memory records returned by katra_memory_query().
 *
 * Parameters:
 *   results - Array of record pointers
 *   count - Number of records in array
 */
void katra_memory_free_results(memory_record_t** results, size_t count);

/* Check if tier2 is enabled
 *
 * Returns true if tier2 initialized successfully, false otherwise.
 * Used by health monitoring to report tier2 availability.
 */
bool katra_memory_tier2_enabled(void);

#endif /* KATRA_MEMORY_H */
