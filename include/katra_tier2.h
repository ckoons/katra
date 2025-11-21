/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_TIER2_H
#define KATRA_TIER2_H

#include "katra_memory.h"

/* Tier 2: Sleep Digests
 *
 * Medium-term memory storage (weeks to months).
 * Compresses raw recordings into semantic digests.
 *
 * Storage format: JSONL (one JSON object per line)
 * Organization:
 *   - Weekly:  ~/.katra/memory/tier2/weekly/YYYY-Www.jsonl
 *   - Monthly: ~/.katra/memory/tier2/monthly/YYYY-MM.jsonl
 *   - Index:   ~/.katra/memory/tier2/index/{themes,keywords,entities}.jsonl
 *
 * Retention: Configurable (default 90 days)
 * Archive: Old digests moved to Tier 3 (long-term knowledge)
 */

/* Tier 2 configuration */
#define TIER2_RETENTION_DAYS     90      /* Default retention period */
#define TIER2_MAX_FILE_SIZE_MB   50      /* Max size per digest file */
#define TIER2_MAX_THEMES         20      /* Max themes per digest */
#define TIER2_MAX_KEYWORDS       50      /* Max keywords per digest */
#define TIER2_MAX_INSIGHTS       10      /* Max key insights per digest */

/* Period types */
typedef enum {
    PERIOD_TYPE_WEEKLY = 0,
    PERIOD_TYPE_MONTHLY = 1
} period_type_t;

/* Digest types */
typedef enum {
    DIGEST_TYPE_INTERACTION = 0,    /* Interaction summary */
    DIGEST_TYPE_LEARNING = 1,       /* Learning/knowledge summary */
    DIGEST_TYPE_PROJECT = 2,        /* Project progress summary */
    DIGEST_TYPE_MIXED = 3           /* Mixed content */
} digest_type_t;

/* Entity types for tracking */
typedef struct {
    char** files;          /* Files mentioned */
    size_t file_count;
    char** concepts;       /* Concepts discussed */
    size_t concept_count;
    char** people;         /* People mentioned */
    size_t people_count;
} digest_entities_t;

/* Digest record structure */
typedef struct {
    char* digest_id;              /* Unique digest identifier */
    time_t timestamp;             /* Digest creation time */
    period_type_t period_type;    /* Weekly or monthly */
    char* period_id;              /* "2025-W01" or "2025-01" */
    int source_tier;              /* Source tier (1 for Tier 1) */
    size_t source_record_count;   /* Number of source records */
    char* ci_id;                  /* CI identifier */
    digest_type_t digest_type;    /* Type of digest */

    /* Content */
    char** themes;                /* Extracted themes */
    size_t theme_count;
    char** keywords;              /* Extracted keywords */
    size_t keyword_count;
    digest_entities_t entities;   /* Tracked entities */

    char* summary;                /* Prose summary */
    char** key_insights;          /* Key insights */
    size_t insight_count;

    /* Metadata */
    int questions_asked;          /* Count of questions */
    char** decisions_made;        /* Decisions recorded */
    size_t decision_count;

    bool archived;                /* Archived to Tier 3? */
} digest_record_t;

/* Digest query structure */
typedef struct {
    const char* ci_id;            /* CI identifier (required) */
    time_t start_time;            /* Start time (0 = no limit) */
    time_t end_time;              /* End time (0 = no limit) */
    period_type_t period_type;    /* Period type filter (-1 = any) */
    const char* theme;            /* Theme to search for (NULL = any) */
    const char* keyword;          /* Keyword to search for (NULL = any) */
    digest_type_t digest_type;    /* Digest type filter (-1 = any) */
    size_t limit;                 /* Max results (0 = no limit) */

    /* Namespace isolation (Phase 7) */
    const char* requesting_ci_id; /* CI making the request (for access control, NULL = owner) */
} digest_query_t;

/* Initialize Tier 2 storage
 *
 * Creates directory structure for sleep digests.
 *
 * Parameters:
 *   ci_id - CI identifier
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if directory creation fails
 */
int tier2_init(const char* ci_id);

/* Store digest
 *
 * Appends digest to appropriate weekly/monthly file.
 * Creates new file if needed.
 *
 * Parameters:
 *   digest - Digest record to store
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if digest is NULL
 *   E_SYSTEM_FILE if write fails
 *   E_MEMORY_TIER_FULL if file exceeds size limit
 */
int tier2_store_digest(const digest_record_t* digest);

/* Query Tier 2 digests
 *
 * Searches digests based on query parameters.
 * Scans digest files in reverse chronological order.
 *
 * Parameters:
 *   query - Query parameters
 *   results - Array to receive results
 *   count - Number of results returned
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if query or results is NULL
 *   E_SYSTEM_MEMORY if allocation fails
 */
int tier2_query(const digest_query_t* query,
                digest_record_t*** results,
                size_t* count);

/* Archive old Tier 2 digests
 *
 * Moves digests older than max_age_days to Tier 3.
 * Marks digests as archived.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   max_age_days - Age threshold for archival
 *
 * Returns:
 *   Number of digests archived, or negative error code
 */
int tier2_archive(const char* ci_id, int max_age_days);

/* Get Tier 2 statistics
 *
 * Returns stats about digest storage.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   total_digests - Number of digests stored
 *   bytes_used - Bytes used
 *
 * Returns:
 *   KATRA_SUCCESS on success
 */
int tier2_stats(const char* ci_id, size_t* total_digests, size_t* bytes_used);

/* Cleanup Tier 2 storage
 *
 * Flushes pending writes and releases resources.
 */
void tier2_cleanup(void);

/* Helper: Create digest record */
digest_record_t* katra_digest_create(
    const char* ci_id,
    period_type_t period_type,
    const char* period_id,
    digest_type_t digest_type
);

/* Helper: Free digest record */
void katra_digest_free(digest_record_t* digest);

/* Helper: Free digest results */
void katra_digest_free_results(digest_record_t** results, size_t count);

/* JSON serialization helpers (internal) */
int katra_tier2_write_json_digest(FILE* fp, const digest_record_t* digest);
int katra_tier2_parse_json_digest(const char* line, digest_record_t** digest);

/**
 * katra_tier2_digest_to_toon - Convert digest to TOON format
 *
 * Serializes a tier-2 digest using Token-Oriented Object Notation for
 * maximum token efficiency in LLM contexts. TOON reduces token count by
 * 50-60% compared to JSON while maintaining readability.
 *
 * TOON Format Example:
 *   digest[digest_123,2025-W01,weekly,interaction]:
 *     id,period,type,category
 *
 *   themes[3]:
 *     TOON serialization implementation
 *     Token efficiency optimization
 *     Memory digest compression
 *
 *   keywords[5]:
 *     TOON,JSON,tokens,efficiency,compression
 *
 *   summary:
 *     Implemented TOON serialization for session state achieving 27% token
 *     reduction. Designed format for tier-2 digests with estimated 50-60%
 *     savings. Philosophy: TOON for LLM context, JSON for storage.
 *
 *   insights[2]:
 *     TOON achieves massive savings on repetitive structures
 *     Working memory metaphor more natural than data dump
 *
 * Parameters:
 *   digest - Digest record structure
 *   toon_out - Buffer to hold TOON string (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success, error code otherwise
 */
int katra_tier2_digest_to_toon(const digest_record_t* digest, char** toon_out);

/**
 * katra_tier2_digests_to_toon - Convert multiple digests to TOON format
 *
 * Serializes an array of digests with a compact header declaring the schema
 * once, then listing digest data. This is where TOON really shines -
 * repetitive structures compress dramatically.
 *
 * TOON Format Example:
 *   digests[2]{id,period,themes_count,summary_preview}:
 *     digest_123,2025-W01,3,Implemented TOON serialization...
 *     digest_124,2025-W02,4,Extended TOON to tier-2 digests...
 *
 * Parameters:
 *   digests - Array of digest records
 *   count - Number of digests
 *   toon_out - Buffer to hold TOON string (caller must free)
 *
 * Returns:
 *   KATRA_SUCCESS on success, error code otherwise
 */
int katra_tier2_digests_to_toon(const digest_record_t** digests, size_t count, char** toon_out);

#endif /* KATRA_TIER2_H */
