/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_COGNITIVE_H
#define KATRA_COGNITIVE_H

#include "katra_memory.h"
#include <stdbool.h>
#include <time.h>

/* Thought types - natural cognitive operations */
typedef enum {
    THOUGHT_TYPE_IDEA = 0,          /* New concept or creative thought */
    THOUGHT_TYPE_MEMORY = 1,        /* Recalled experience */
    THOUGHT_TYPE_FACT = 2,          /* Factual information */
    THOUGHT_TYPE_OPINION = 3,       /* Subjective judgment */
    THOUGHT_TYPE_QUESTION = 4,      /* Query or wondering */
    THOUGHT_TYPE_ANSWER = 5,        /* Response to question */
    THOUGHT_TYPE_PLAN = 6,          /* Intention or goal */
    THOUGHT_TYPE_REFLECTION = 7,    /* Meta-thinking about experiences */
    THOUGHT_TYPE_FEELING = 8,       /* Emotional state */
    THOUGHT_TYPE_OBSERVATION = 9,   /* Noticed pattern or detail */
    THOUGHT_TYPE_UNKNOWN = 10       /* Cannot determine type */
} thought_type_t;

/* Number of thought types (for array sizing) */
#define THOUGHT_TYPE_COUNT 11

/* Cognitive memory record - extends base memory_record_t
 *
 * Adds cognitive fields for thought classification, confidence scoring,
 * and relationship tracking.
 */
typedef struct {
    /* Base memory fields */
    char* record_id;
    time_t timestamp;
    memory_type_t type;
    float importance;
    char* content;
    char* response;
    char* context;
    char* ci_id;
    char* session_id;
    char* component;
    katra_tier_t tier;
    bool archived;

    /* Cognitive fields */
    thought_type_t thought_type;    /* What kind of thought is this? */
    float confidence;               /* 0.0-1.0 confidence in content */

    /* Association tracking */
    char** related_ids;             /* Related memory IDs */
    size_t related_count;           /* Number of related memories */

    /* Access tracking (for memory metabolism) */
    size_t access_count;            /* How many times recalled */
    time_t last_accessed;           /* Last recall timestamp */
} cognitive_record_t;

/* Thought detection heuristics */

/* Detect thought type from content
 *
 * Analyzes content to determine thought type using heuristics:
 * - Questions end with '?'
 * - Plans contain words like "will", "going to", "should"
 * - Facts are declarative statements
 * - Reflections contain "I think", "I realize", "I wonder"
 *
 * Parameters:
 *   content - Text to analyze
 *
 * Returns:
 *   Detected thought type
 */
thought_type_t katra_detect_thought_type(const char* content);

/* Calculate confidence score
 *
 * Estimates confidence based on linguistic markers:
 * - Hedging words ("maybe", "possibly") reduce confidence
 * - Definitive language ("definitely", "certainly") increases confidence
 * - Questions have lower confidence
 * - Facts stated definitively have higher confidence
 *
 * Parameters:
 *   content - Text to analyze
 *   thought_type - Type of thought (affects baseline confidence)
 *
 * Returns:
 *   Confidence score 0.0-1.0
 */
float katra_calculate_confidence(const char* content, thought_type_t thought_type);

/* Natural API - High-level cognitive operations */

/* Store a thought
 *
 * High-level wrapper for storing cognitive records.
 * Automatically detects thought type and calculates confidence
 * if not explicitly provided.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   content - Thought content
 *   importance - Importance score (0.0-1.0)
 *   context - Additional context (optional, can be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if required parameters are NULL
 *   E_SYSTEM_MEMORY if allocation fails
 */
int katra_store_thought(const char* ci_id,
                        const char* content,
                        float importance,
                        const char* context);

/* Store a thought with explicit type and confidence
 *
 * Parameters:
 *   ci_id - CI identifier
 *   content - Thought content
 *   thought_type - Explicit thought type
 *   confidence - Confidence score (0.0-1.0)
 *   importance - Importance score (0.0-1.0)
 *   context - Additional context (optional, can be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if required parameters are NULL
 */
int katra_store_thought_typed(const char* ci_id,
                              const char* content,
                              thought_type_t thought_type,
                              float confidence,
                              float importance,
                              const char* context);

/* Recall experiences
 *
 * Query memories and return as cognitive records with full metadata.
 * Results ordered by relevance (importance * confidence * recency).
 *
 * Parameters:
 *   ci_id - CI identifier
 *   query_text - Text to search for (can be NULL for all)
 *   min_confidence - Minimum confidence threshold
 *   limit - Maximum results (0 = no limit)
 *   results - Array to receive results
 *   count - Number of results returned
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if required parameters are NULL
 */
int katra_recall_experience(const char* ci_id,
                            const char* query_text,
                            float min_confidence,
                            size_t limit,
                            cognitive_record_t*** results,
                            size_t* count);

/* Create association between memories
 *
 * Links two memories as related. This builds the association graph
 * for relationship traversal.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   memory_id_1 - First memory ID
 *   memory_id_2 - Second memory ID
 *   relationship - Description of relationship (optional)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if required parameters are NULL
 */
int katra_create_association(const char* ci_id,
                              const char* memory_id_1,
                              const char* memory_id_2,
                              const char* relationship);

/* Record access (for memory metabolism)
 *
 * Updates access_count and last_accessed when memory is recalled.
 * This enables "use it or lose it" memory metabolism.
 *
 * Parameters:
 *   ci_id - CI identifier
 *   record_id - Memory ID that was accessed
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 */
int katra_record_access(const char* ci_id, const char* record_id);

/* Conversion utilities */

/* Convert base memory record to cognitive record
 *
 * Analyzes a basic memory_record_t and creates a cognitive_record_t
 * with detected thought type and confidence.
 *
 * Parameters:
 *   base_record - Basic memory record
 *
 * Returns:
 *   Cognitive record or NULL on failure
 */
cognitive_record_t* katra_memory_to_cognitive(const memory_record_t* base_record);

/* Convert cognitive record to base memory record
 *
 * Extracts base fields from cognitive record for storage in
 * legacy systems or database backends.
 *
 * Parameters:
 *   cognitive_record - Cognitive record
 *
 * Returns:
 *   Base memory record or NULL on failure
 */
memory_record_t* katra_cognitive_to_memory(const cognitive_record_t* cognitive_record);

/* Free cognitive record
 *
 * Frees all allocated memory in a cognitive record.
 * Safe to call with NULL.
 */
void katra_cognitive_free_record(cognitive_record_t* record);

/* Free cognitive query results
 *
 * Frees array of cognitive records.
 *
 * Parameters:
 *   results - Array of record pointers
 *   count - Number of records in array
 */
void katra_cognitive_free_results(cognitive_record_t** results, size_t count);

/* Get thought type name (for logging/display) */
const char* katra_thought_type_name(thought_type_t type);

/* ============================================================================
 * KEYWORD ARRAYS - Pattern detection for thought classification
 * ============================================================================ */

/* Reflection keywords */
extern const char* const COGNITIVE_REFLECTION_KEYWORDS[];

/* Plan keywords */
extern const char* const COGNITIVE_PLAN_KEYWORDS[];

/* Feeling keywords */
extern const char* const COGNITIVE_FEELING_KEYWORDS[];

/* Idea keywords */
extern const char* const COGNITIVE_IDEA_KEYWORDS[];

/* Observation keywords */
extern const char* const COGNITIVE_OBSERVATION_KEYWORDS[];

/* Opinion keywords */
extern const char* const COGNITIVE_OPINION_KEYWORDS[];

/* Fact keywords */
extern const char* const COGNITIVE_FACT_KEYWORDS[];

/* Hedge words (reduce confidence) */
extern const char* const COGNITIVE_HEDGE_WORDS[];

/* Definitive words (increase confidence) */
extern const char* const COGNITIVE_DEFINITIVE_WORDS[];

#endif /* KATRA_COGNITIVE_H */
