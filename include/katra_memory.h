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

/* Memory system thresholds and limits */
#define MEMORY_ID_RANDOM_MAX         10000  /* Random component for record IDs */
#define MEMORY_HEALTH_THRESHOLD_LOW  50     /* Active memories for "healthy" status */
#define MEMORY_HEALTH_THRESHOLD_HIGH 200    /* Active memories for "critical" status */
#define MEMORY_CONSOLIDATION_THRESHOLD 100  /* Active memories to recommend consolidation */
#define MEMORY_QUERY_LIMIT_DEFAULT   10000  /* Default query limit for large batches */
#define MEMORY_PREVIEW_LENGTH        100    /* Content preview length in characters */
#define MEMORY_ACCESS_IGNORE_SECONDS 5      /* Ignore recent accesses (query side effect) */
/* Note: SECONDS_PER_DAY is defined in katra_limits.h */

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

    /* Personal memory collections - Active curation metadata */
    bool personal;             /* Part of personal collection (working self) */
    bool not_to_archive;       /* CI holding onto this, don't auto-archive */
    char* collection;          /* Collection path: "People/Casey", "Moments/Breakthrough" */
    time_t last_reviewed;      /* When CI last reflected on this memory */
    int review_count;          /* How many times CI has reviewed this */
    int turn_id;               /* Which turn this memory was created in (for reflection) */

    /* Thane's recommendations - Phase 2: Connection graph */
    char** connected_memory_ids; /* Array of record IDs this memory references */
    size_t connection_count;     /* Number of connections */
    float graph_centrality;      /* Graph centrality score (0.0-1.0) */

    /* Thane's recommendations - Phase 3: Pattern compression */
    char* pattern_id;            /* Pattern this memory belongs to (NULL if not part of pattern) */
    size_t pattern_frequency;    /* How many times this pattern occurs */
    bool is_pattern_outlier;     /* True if this is an outlier worth preserving */
    float semantic_similarity;   /* Similarity score to pattern centroid (0.0-1.0) */
    char* pattern_summary;       /* Context about archived pattern (NULL if not pattern outlier) */

    /* Thane's active sense-making - Phase 4: Formation context */
    char* context_question;      /* Why did I remember this? What problem/question? */
    char* context_resolution;    /* What did this resolve or clarify? */
    char* context_uncertainty;   /* What was I uncertain about before this? */
    char* related_to;            /* Record ID this connects to (NULL if standalone) */
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

    /* Personal collection filters */
    bool filter_personal;      /* If true, filter by personal field */
    bool personal_value;       /* Value to match if filter_personal=true */
    const char* collection_prefix; /* Filter by collection prefix (NULL = no filter) */
    bool filter_not_to_archive; /* If true, filter by not_to_archive field */
    bool not_to_archive_value; /* Value to match if filter_not_to_archive=true */
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

/* Create memory record with formation context (Thane's active sense-making)
 *
 * Extended version that captures WHY the memory formed, not just what happened.
 * Enables active sense-making by connecting memories to their reasoning context.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   type - Memory type
 *   content - Memory content
 *   importance - Importance score (0.0-1.0)
 *   context_question - Why remember this? (may be NULL)
 *   context_resolution - What did this resolve? (may be NULL)
 *   context_uncertainty - What was uncertain before? (may be NULL)
 *   related_to - Related memory record_id (may be NULL)
 *
 * Returns:
 *   Allocated memory record or NULL on failure
 */
memory_record_t* katra_memory_create_with_context(
    const char* ci_id,
    memory_type_t type,
    const char* content,
    float importance,
    const char* context_question,
    const char* context_resolution,
    const char* context_uncertainty,
    const char* related_to);

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

/* ============================================================================
 * Metacognitive Awareness API (Thane's Active Sense-Making)
 * ============================================================================
 * These functions enable CIs to understand their own memory state:
 * - What memories exist and their consolidation health
 * - What will be archived soon (memories at risk)
 * - What patterns have been detected
 */

/* Memory consolidation health information */
typedef struct {
    size_t total_memories;        /* Total memories (active + archived) */
    size_t active_memories;       /* Active memories (working set) */
    size_t archived_memories;     /* Archived memories (compressed) */
    float compression_ratio;      /* Archived / total (0.0-1.0) */
    bool consolidation_recommended; /* Should consolidation run? */
    const char* health_status;    /* "healthy", "degraded", "critical" */
} memory_consolidation_health_t;

/* Memory at risk information */
typedef struct {
    char* record_id;              /* Which memory */
    char* content_preview;        /* First 100 chars of content */
    const char* risk_reason;      /* Why it's at risk */
    float risk_score;             /* How likely to archive (0.0-1.0) */
} memory_at_risk_t;

/* Pattern detection information */
typedef struct {
    char* pattern_id;             /* Pattern identifier */
    size_t member_count;          /* How many memories in pattern */
    char* centroid_preview;       /* Representative example */
    float similarity_threshold;   /* Similarity threshold used */
} detected_pattern_t;

/* Connection hub information */
typedef struct {
    char* record_id;              /* Hub memory record ID */
    char* content_preview;        /* First 100 chars of content */
    size_t connection_count;      /* Number of connections */
    float centrality_score;       /* Normalized centrality (0.0-1.0) */
} memory_connection_hub_t;

/* Get memory consolidation health status
 *
 * Returns current state of memory system for a CI.
 * Enables CI to understand memory pressure and need for consolidation.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   health - Memory consolidation health structure to fill
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id or health is NULL
 *   E_INVALID_STATE if memory subsystem not initialized
 */
int katra_memory_get_consolidation_health(const char* ci_id, memory_consolidation_health_t* health);

/* Get memories at risk of archival
 *
 * Returns list of memories that would be archived on next consolidation.
 * Enables CI to understand what's about to be forgotten.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   max_age_days - Archival threshold (same as would be used in archive())
 *   at_risk - Array of at-risk memories (caller must free)
 *   count - Number of memories at risk
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id, at_risk, or count is NULL
 *   E_INVALID_STATE if memory subsystem not initialized
 */
int katra_memory_get_at_risk(const char* ci_id, int max_age_days,
                             memory_at_risk_t** at_risk, size_t* count);

/* Get detected patterns
 *
 * Returns list of memory patterns that have been identified.
 * Enables CI to understand what recurring themes exist.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   patterns - Array of detected patterns (caller must free)
 *   count - Number of patterns detected
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id, patterns, or count is NULL
 *   E_INVALID_STATE if memory subsystem not initialized
 */
int katra_memory_get_patterns(const char* ci_id,
                              detected_pattern_t** patterns,
                              size_t* count);

/* Free memory at risk array */
void katra_memory_free_at_risk(memory_at_risk_t* at_risk, size_t count);

/* Free detected patterns array */
void katra_memory_free_patterns(detected_pattern_t* patterns, size_t count);

/* Get connection graph hub memories
 *
 * Returns memories with high graph centrality scores (well-connected hubs).
 * These are important memories that connect many other memories together.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   min_centrality - Minimum centrality threshold (0.0-1.0, recommend 0.5)
 *   hubs - Array of hub memories (caller must free)
 *   count - Number of hub memories found
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id, hubs, or count is NULL
 *   E_INVALID_STATE if memory subsystem not initialized
 */
int katra_memory_get_connection_hubs(const char* ci_id, float min_centrality,
                                      memory_connection_hub_t** hubs, size_t* count);

/* Free connection hubs array */
void katra_memory_free_connection_hubs(memory_connection_hub_t* hubs, size_t count);

/* Related memory information (Quick Win #2) */
typedef struct {
    char* record_id;               /* Related memory ID */
    char* content_preview;         /* First 100 chars of content */
    float similarity_score;        /* Similarity to target (0.0-1.0) */
    bool explicit_link;            /* True if related_to link */
} related_memory_t;

/* Get memories related to a specific record
 *
 * Finds memories similar to a target memory based on:
 * - Keyword similarity (using Phase 2/3 keyword matching)
 * - Explicit related_to links
 *
 * Results are sorted by similarity score (highest first).
 *
 * Parameters:
 *   ci_id - CI identifier
 *   record_id - Target memory to find relations for
 *   max_results - Maximum number of results (0 = no limit)
 *   min_similarity - Minimum similarity threshold (0.0-1.0, recommend 0.3)
 *   related - Array of related memories (caller must free)
 *   count - Number of related memories found
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if ci_id, record_id, related, or count is NULL
 *   E_INVALID_STATE if memory subsystem not initialized
 *   E_NOT_FOUND if target record_id doesn't exist
 */
int katra_memory_get_related(const char* ci_id, const char* record_id,
                              size_t max_results, float min_similarity,
                              related_memory_t** related, size_t* count);

/* Free related memories array */
void katra_memory_free_related(related_memory_t* related, size_t count);

/* ============================================================================
 * Connection Graph API (Thane's Phase 2)
 * ============================================================================
 * These functions build and analyze memory connections:
 * - Build connection counts based on content similarity
 * - Calculate graph centrality to identify hub memories
 * - Use centrality in consolidation to preserve important connections
 */

/* Build connections for a single memory record
 *
 * Analyzes memory content and counts connections to other memories.
 * Updates the record's connection_count field (does not persist).
 *
 * Parameters:
 *   record - Memory record to analyze
 *   all_memories - Array of all memories to compare against
 *   memory_count - Number of memories in array
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if record or all_memories is NULL
 */
int katra_memory_build_connections_for_record(memory_record_t* record,
                                                memory_record_t** all_memories,
                                                size_t memory_count);

/* Calculate graph centrality for a set of memory records
 *
 * Builds connection graphs and calculates normalized centrality scores.
 * Updates each record's connection_count and graph_centrality fields.
 *
 * Parameters:
 *   memories - Array of memory records
 *   count - Number of records in array
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if memories is NULL
 */
int katra_memory_calculate_centrality_for_records(memory_record_t** memories,
                                                    size_t count);

#endif /* KATRA_MEMORY_H */
