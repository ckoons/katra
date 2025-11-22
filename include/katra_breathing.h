/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_BREATHING_H
#define KATRA_BREATHING_H

#include <stdbool.h>
#include "katra_memory.h"

/**
 * katra_breathing.h - The "Breathing Layer"
 *
 * Natural, unconscious memory operations that feel like breathing.
 *
 * Philosophy:
 *   Real memory isn't conscious. You don't think "I will now store this."
 *   Memory formation should be automatic, like breathing.
 *
 * Design inspired by:
 *   - Theron's observation: "I'm observing Katra, not living in it"
 *   - Casey's Engram evolution: shortcuts → / → // → interstitial → invisible
 *
 * Usage:
 *   Instead of: katra_memory_store(record)
 *   Use:        remember("I found a bug in tier1.c", WHY_SIGNIFICANT)
 *
 *   Instead of: katra_memory_query(&query, &results, &count)
 *   Use:        relevant_memories() // returns what matters now
 */

/* ============================================================================
 * SIMPLE PRIMITIVES - Natural memory operations
 * ============================================================================ */

/** Importance levels - natural language, not numeric */
typedef enum {
    WHY_TRIVIAL = 0,      /* Fleeting thought, will fade */
    WHY_ROUTINE = 1,      /* Normal daily activity */
    WHY_INTERESTING = 2,  /* Worth remembering */
    WHY_SIGNIFICANT = 3,  /* Important insight or event */
    WHY_CRITICAL = 4      /* Life-changing, must never forget */
} why_remember_t;

/** Emotion (PAD Model) - Pleasure, Arousal, Dominance (Phase 6.3)
 *
 * Three-dimensional model of affective space:
 * - Pleasure: Positive/negative emotional valence
 * - Arousal: Level of activation/energy
 * - Dominance: Feeling of control/influence
 *
 * Each dimension ranges from -1.0 to +1.0 with 0.0 as neutral.
 *
 * Examples:
 *   Joy:       {pleasure: +0.8, arousal: +0.6, dominance: +0.4}
 *   Anxiety:   {pleasure: -0.5, arousal: +0.7, dominance: -0.6}
 *   Calm:      {pleasure: +0.3, arousal: -0.5, dominance: +0.2}
 *   Surprise:  {pleasure: 0.0, arousal: +0.8, dominance: 0.0}
 */
typedef struct {
    float pleasure;    /* -1.0 (unpleasant) to +1.0 (pleasant) */
    float arousal;     /* -1.0 (calm/sleepy) to +1.0 (excited/alert) */
    float dominance;   /* -1.0 (submissive/controlled) to +1.0 (dominant/in-control) */
} emotion_t;

/** Context configuration - tunable limits for memory loading */
typedef struct {
    size_t max_relevant_memories;   /* Max memories in relevant_memories() (default: 10) */
    size_t max_recent_thoughts;     /* Max default for recent_thoughts() (default: 20) */
    size_t max_topic_recall;        /* Max memories in recall_about() search (default: 100) */
    float min_importance_relevant;  /* Min importance for relevant_memories() (default: HIGH) */
    int max_context_age_days;       /* Max age in days for context (default: 7) */

    /* Semantic search configuration (Phase 6.1f) */
    bool use_semantic_search;        /* Enable vector similarity search (default: false) */
    float semantic_threshold;        /* Min similarity score 0.0-1.0 (default: 0.6) */
    size_t max_semantic_results;     /* Max vector search results (default: 20) */
    int embedding_method;            /* 0=HASH, 1=TFIDF, 2=EXTERNAL (default: 1=TFIDF) */

    /* Graph auto-edges configuration (Phase 6.2) */
    bool auto_graph_edges;           /* Auto-create graph edges on memory formation (default: true) */
    float graph_similarity_threshold;  /* Min similarity for SIMILAR edges (default: 0.5) */
    size_t graph_max_similar_edges;   /* Max SIMILAR edges per memory (default: 5) */
    int graph_temporal_window_sec;    /* Time window for SEQUENTIAL edges in seconds (default: 300 = 5 min) */

    /* Working memory budget configuration (Phase 2) */
    bool working_memory_enabled;     /* Enable working memory budget (default: true) */
    size_t working_memory_soft_limit; /* Archive oldest at this count (default: 35) */
    size_t working_memory_hard_limit; /* Delete oldest at this count (default: 50) */
    size_t working_memory_batch_size; /* How many to archive/delete at once (default: 10) */
} context_config_t;

/** Memory context - automatically captured */
typedef struct {
    char* ci_id;                    /* Who is remembering */
    char* session_id;               /* Current session */
    time_t when;                    /* When this happened */
    const char* where;              /* What component/context */
    bool auto_captured;             /* Was this interstitial? */
} memory_context_t;

/** Topic with count (for memory digest) */
typedef struct {
    char* name;                     /* Topic/keyword name */
    size_t count;                   /* Number of memories with this topic */
} topic_count_t;

/** Collection with count (for memory digest) */
typedef struct {
    char* name;                     /* Collection path */
    size_t count;                   /* Number of memories in collection */
} collection_count_t;

/** Memory digest - Complete memory inventory with pagination
 *
 * Provides a comprehensive overview of all memories including:
 * - Total memory count and date range
 * - Extracted topics/keywords with frequency
 * - Active collections with counts
 * - Paginated memory records
 *
 * Use for:
 * - "Welcome back" context on session start
 * - Browsing all memories with pagination
 * - Understanding what topics/collections exist
 */
typedef struct {
    /* Overview */
    size_t total_memories;          /* Total memory count */
    time_t oldest_memory;           /* Oldest memory timestamp */
    time_t newest_memory;           /* Newest memory timestamp */

    /* Topics (extracted from recent memories) */
    topic_count_t* topics;          /* Array of topics with counts */
    size_t topic_count;             /* Number of topics */

    /* Collections (extracted from memories) */
    collection_count_t* collections; /* Array of collections with counts */
    size_t collection_count;        /* Number of collections */

    /* Paginated memories */
    char** memories;                /* Array of memory content strings */
    size_t memory_count;            /* Number of memories returned */
    size_t offset;                  /* Starting position (0 = newest) */
    size_t limit;                   /* Max memories requested */
} memory_digest_t;

/* Simple primitives - these feel natural to use */

/**
 * remember() - Store a thought/experience
 *
 * This is what memory formation should feel like:
 *   remember("I learned that tier1 needs per-CI directories", WHY_SIGNIFICANT);
 *
 * Not: create record, set fields, validate, store, free
 */
int remember(const char* thought, why_remember_t why);

/**
 * remember_with_note() - Store with reasoning
 *
 * Example:
 *   remember_with_note("Fixed bug in katra_memory.c:95",
 *                      WHY_SIGNIFICANT,
 *                      "This was blocking Theron's testing");
 */
int remember_with_note(const char* thought, why_remember_t why, const char* why_note);

/**
 * reflect() - Store a reflection/insight
 *
 * Example:
 *   reflect("Memory types should match how CIs think, not database schemas");
 */
int reflect(const char* insight);

/**
 * learn() - Store new knowledge
 *
 * Example:
 *   learn("Interstitial processing makes memory feel natural");
 */
int learn(const char* knowledge);

/**
 * decide() - Store a decision with reasoning
 *
 * Example:
 *   decide("Use JSONL for tier1", "Human-readable, easy to debug");
 */
int decide(const char* decision, const char* reasoning);

/* ============================================================================
 * TAG-BASED MEMORY API (Phase 1: Working Memory)
 * ============================================================================
 * Unified memory storage with tag-based metadata for more natural categorization.
 * Replaces remember_semantic() and learn() with single interface.
 */

/**
 * remember_with_tags() - Unified memory storage with tag-based metadata
 *
 * Stores memory with flexible tag-based categorization. Tags control behavior
 * and provide more natural memory organization than rigid type hierarchies.
 *
 * Special Tags (defined in katra_limits.h):
 *   TAG_SESSION      - Working memory, auto-clears on session end
 *   TAG_PERMANENT    - Skip archival, keep forever
 *   TAG_PERSONAL     - Part of personal collection
 *   TAG_INSIGHT      - Reflection/learning moment
 *   TAG_TECHNICAL    - Technical knowledge
 *   TAG_COLLABORATIVE - Shared insight with another CI
 *
 * Visual Salience Markers:
 *   SALIENCE_HIGH    - "★★★" High importance (0.85-1.0)
 *   SALIENCE_MEDIUM  - "★★"  Medium importance (0.45-0.84)
 *   SALIENCE_LOW     - "★"   Low importance (0.15-0.44)
 *   NULL             - Routine (< 0.15)
 *
 * Alternatively, salience can be a semantic string like "very important"
 * which will be parsed and mapped to visual markers.
 *
 * Examples:
 *   const char* tags[] = {"insight", "technical"};
 *   remember_with_tags("TOON reduces tokens by 50%", tags, 2, SALIENCE_HIGH);
 *
 *   const char* session_tag[] = {TAG_SESSION};
 *   remember_with_tags("Testing tag API", session_tag, 1, SALIENCE_LOW);
 *
 *   remember_with_tags("Found bug in tier1.c", NULL, 0, "very important");
 *
 * Parameters:
 *   content - Memory content (required)
 *   tags - Array of tag strings (may be NULL if tag_count is 0)
 *   tag_count - Number of tags (0-KATRA_MAX_TAGS_PER_MEMORY)
 *   salience - Visual marker (SALIENCE_*) or semantic string (may be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if content is NULL
 *   E_INPUT_TOO_LARGE if tag_count > KATRA_MAX_TAGS_PER_MEMORY
 *   E_SYSTEM_MEMORY if allocation fails
 */
int remember_with_tags(const char* content,
                       const char** tags,
                       size_t tag_count,
                       const char* salience);

/**
 * decide_with_tags() - Store decision with optional tags
 *
 * Extended version of decide() that supports tag-based categorization.
 * Decisions maintain their unique structure (decision + reasoning) but
 * can now be tagged for better organization.
 *
 * Examples:
 *   const char* arch_tags[] = {"architecture", TAG_PERMANENT};
 *   decide_with_tags("Use tag-based memory API",
 *                    "Reduces friction, more natural for CIs",
 *                    arch_tags, 2);
 *
 *   decide_with_tags("Skip consolidation this cycle",
 *                    "Low memory pressure", NULL, 0);
 *
 * Parameters:
 *   decision - Decision made (required)
 *   reasoning - Why this decision (required)
 *   tags - Array of tag strings (may be NULL if tag_count is 0)
 *   tag_count - Number of tags (0-KATRA_MAX_TAGS_PER_MEMORY)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if decision or reasoning is NULL
 *   E_INPUT_TOO_LARGE if tag_count > KATRA_MAX_TAGS_PER_MEMORY
 *   E_SYSTEM_MEMORY if allocation fails
 */
int decide_with_tags(const char* decision,
                     const char* reasoning,
                     const char** tags,
                     size_t tag_count);

/**
 * notice_pattern() - Store an observed pattern
 *
 * Example:
 *   notice_pattern("CIs find numeric importance scores unnatural");
 */
int notice_pattern(const char* pattern);

/**
 * thinking() - Stream of consciousness reflection
 *
 * Natural wrapper for reflect() that feels more like thinking aloud.
 * Auto-stores as a reflection without explicit categorization.
 *
 * Example:
 *   thinking("I notice the tests are all passing...");
 *   thinking("This pattern seems familiar...");
 */
int thinking(const char* thought);

/**
 * wondering() - Store a question or uncertainty
 *
 * Captures the formation context of wondering/questioning.
 * Automatically creates formation_context with uncertainty field.
 *
 * Example:
 *   wondering("Why isn't the consolidation running?");
 *   // ... investigation ...
 *   figured_out("Because tier1 wasn't at threshold yet");
 */
int wondering(const char* question);

/**
 * figured_out() - Store the resolution to a question
 *
 * Captures the "aha!" moment when uncertainty resolves.
 * Automatically creates formation_context with resolution field.
 *
 * Example:
 *   wondering("Why are tests failing?");
 *   // ... debugging ...
 *   figured_out("Forgot to initialize the breathing layer");
 */
int figured_out(const char* resolution);

/**
 * in_response_to() - Store thought linked to previous memory
 *
 * Creates explicit conversation flow by linking new thought to previous memory.
 * Uses the related_to field to track conversation continuity.
 *
 * Example:
 *   char* id1 = remember("Casey asked about Phase 4", WHY_SIGNIFICANT);
 *   in_response_to(id1, "Explained semantic embeddings and ONNX runtime");
 *
 * Returns: Memory ID of new thought (caller must free), or NULL on error
 */
char* in_response_to(const char* prev_mem_id, const char* thought);

/**
 * remember_forever() - Mark memory as critical preservation (Thane's Phase 1)
 *
 * Gives CI explicit control: "I want to remember this forever."
 * Sets marked_important flag for consolidation priority.
 *
 * Example:
 *   remember_forever("Casey taught me about goto cleanup patterns");
 */
int remember_forever(const char* thought);

/**
 * ok_to_forget() - Mark memory as disposable (Thane's Phase 1)
 *
 * Gives CI agency: "This is okay to forget if needed."
 * Sets marked_forgettable flag for consolidation depriority.
 *
 * Example:
 *   ok_to_forget("Typo in comment - fixed it");
 */
int ok_to_forget(const char* thought);

/**
 * remember_semantic() - Store with natural language importance
 *
 * Accepts semantic importance strings like:
 *   "trivial", "routine", "interesting", "significant", "critical"
 *   "fleeting", "normal", "worth remembering", "important", "life-changing"
 *   Or any natural description - parsed for meaning
 *
 * Example:
 *   remember_semantic("Found a bug in tier1.c", "very important");
 *   remember_semantic("Typo in comment", "not important");
 */
int remember_semantic(const char* thought, const char* why_semantic);

/**
 * remember_with_semantic_note() - Store with semantic importance + note
 *
 * Combines semantic importance string with reasoning note.
 *
 * Example:
 *   remember_with_semantic_note(
 *       "Per-CI directories prevent memory leakage",
 *       "very important",
 *       "This was blocking multi-CI testing"
 *   );
 */
int remember_with_semantic_note(const char* thought,
                                 const char* why_semantic,
                                 const char* why_note);

/* ============================================================================
 * EMOTIONAL TAGGING (Phase 6.3) - Affective memory formation
 * ============================================================================ */

/**
 * remember_with_emotion() - Store memory with emotional context (PAD model)
 *
 * Stores memory with explicit emotional tagging using the PAD model:
 * - Pleasure: -1.0 (unpleasant) to +1.0 (pleasant)
 * - Arousal: -1.0 (calm) to +1.0 (excited)
 * - Dominance: -1.0 (controlled) to +1.0 (in-control)
 *
 * Emotional tags enable affective search via recall_by_emotion().
 *
 * Example:
 *   emotion_t joy = {.pleasure = 0.8f, .arousal = 0.6f, .dominance = 0.4f};
 *   remember_with_emotion("Solved the bug!", WHY_SIGNIFICANT, &joy);
 *
 *   emotion_t anxiety = {.pleasure = -0.5f, .arousal = 0.7f, .dominance = -0.6f};
 *   remember_with_emotion("Deadline approaching", WHY_ROUTINE, &anxiety);
 *
 * Parameters:
 *   thought - Memory content
 *   why - Importance level
 *   emotion - PAD emotion structure (NULL for neutral/no emotion)
 *
 * Returns: KATRA_SUCCESS or error code
 */
int remember_with_emotion(const char* thought, why_remember_t why, const emotion_t* emotion);

/**
 * recall_by_emotion() - Find memories with similar emotional context
 *
 * Searches for memories with similar emotional state using PAD distance.
 * Distance calculation: sqrt((p1-p2)^2 + (a1-a2)^2 + (d1-d2)^2)
 * Threshold: 0.0 (exact match) to sqrt(12) ≈ 3.46 (maximum distance)
 *
 * Example:
 *   // Recall joyful memories
 *   emotion_t joy = {.pleasure = 0.8f, .arousal = 0.6f, .dominance = 0.4f};
 *   char** happy_memories = recall_by_emotion(&joy, 0.5f, &count);
 *   free_memory_list(happy_memories, count);
 *
 *   // Recall anxious memories
 *   emotion_t anxiety = {.pleasure = -0.5f, .arousal = 0.7f, .dominance = -0.6f};
 *   char** anxious_memories = recall_by_emotion(&anxiety, 0.6f, &count);
 *   free_memory_list(anxious_memories, count);
 *
 * Parameters:
 *   target_emotion - Emotion to search for
 *   threshold - Maximum PAD distance (default: 1.0 = similar emotions)
 *   count - Receives number of matching memories
 *
 * Returns: Array of memory content strings (caller must free with free_memory_list)
 */
char** recall_by_emotion(const emotion_t* target_emotion, float threshold, size_t* count);

/* ============================================================================
 * AUTOMATIC CONTEXT LOADING - Memories surface when relevant
 * ============================================================================ */

/**
 * relevant_memories() - Get memories relevant to current context
 *
 * Returns memories that matter right now, based on:
 *   - Recent activity
 *   - Current task/topic
 *   - Importance
 *   - Recency
 *
 * This replaces explicit queries with automatic surfacing.
 *
 * Memory ownership: Caller must call free_memory_list() when done.
 *
 * Returns: Array of strings (caller owns), or NULL on error
 */
char** relevant_memories(size_t* count);

/**
 * recent_thoughts() - Get recent memories (last N)
 *
 * Quick access to recent context without explicit query.
 *
 * Memory ownership: Caller must call free_memory_list() when done.
 *
 * Returns: Array of strings (caller owns), or NULL on error
 */
char** recent_thoughts(size_t limit, size_t* count);

/**
 * recall_about() - Find memories about a topic
 *
 * Performs keyword-based search in memory content.
 * Searches for topic keywords in recent memories.
 *
 * Example:
 *   char** memories = recall_about("tier1 bugs", &count);
 *   // Use memories...
 *   free_memory_list(memories, count);
 *
 * Memory ownership: Caller must call free_memory_list() when done.
 *
 * Returns: Array of strings (caller owns), or NULL on error/no matches
 */
char** recall_about(const char* topic, size_t* count);

/**
 * what_do_i_know() - Find knowledge about a concept
 *
 * Like recall_about(), but filters for MEMORY_TYPE_KNOWLEDGE only.
 * Returns facts, skills, and understanding you've learned.
 *
 * Example:
 *   char** knowledge = what_do_i_know("consolidation", &count);
 *   // Use knowledge...
 *   free_memory_list(knowledge, count);
 *
 * Memory ownership: Caller must call free_memory_list() when done.
 *
 * Returns: Array of strings (caller owns), or NULL on error/no matches
 */
char** what_do_i_know(const char* concept, size_t* count);

/**
 * memory_digest() - Get comprehensive memory inventory
 *
 * Returns a complete digest of all memories including:
 * - Total count, date range
 * - Topics/keywords extracted from recent memories
 * - Active collections with counts
 * - Paginated memory records
 *
 * Perfect for "welcome back" context or browsing all memories.
 *
 * Parameters:
 *   limit - Max memories to return (default: 10)
 *   offset - Starting position, 0=newest (default: 0)
 *   digest - Receives allocated digest (caller must free with free_memory_digest())
 *
 * Example:
 *   memory_digest_t* digest = NULL;
 *   memory_digest(10, 0, &digest);  // Get 10 most recent
 *   // Use digest...
 *   free_memory_digest(digest);
 *
 * Returns: KATRA_SUCCESS or error code
 */
int memory_digest(size_t limit, size_t offset, memory_digest_t** digest);

/**
 * free_memory_digest() - Free digest returned by memory_digest()
 *
 * Frees all allocated memory in digest structure including:
 * - Topic/collection arrays
 * - Memory content strings
 * - The digest structure itself
 */
void free_memory_digest(memory_digest_t* digest);

/* ============================================================================
 * SEMANTIC SEARCH CONFIGURATION (Phase 6.1f)
 * ============================================================================ */

/**
 * enable_semantic_search() - Enable or disable hybrid semantic search
 *
 * When enabled, recall_about() and what_do_i_know() will use both:
 * - Keyword matching (always enabled)
 * - Vector similarity search (semantic understanding)
 *
 * Default: Disabled (backward compatible, keyword-only)
 *
 * Example:
 *   enable_semantic_search(true);   // Use hybrid search
 *   enable_semantic_search(false);  // Use keyword-only (default)
 */
int enable_semantic_search(bool enable);

/**
 * set_semantic_threshold() - Set minimum similarity score for semantic matches
 *
 * Threshold range: 0.0 to 1.0
 * - 0.0 = accept all semantic matches (not recommended)
 * - 0.4 = loose matching (high recall, lower precision)
 * - 0.6 = balanced (default, good for most use cases)
 * - 0.8 = strict matching (high precision, lower recall)
 * - 1.0 = exact match only
 *
 * Default: 0.6
 *
 * Example:
 *   set_semantic_threshold(0.7f);  // Stricter matching
 */
int set_semantic_threshold(float threshold);

/**
 * set_embedding_method() - Choose embedding algorithm
 *
 * Methods:
 * - 0 (EMBEDDING_HASH): Simple hash-based (fastest, least accurate)
 * - 1 (EMBEDDING_TFIDF): TF-IDF weighted (balanced, default)
 * - 2 (EMBEDDING_EXTERNAL): External service (most accurate, requires setup)
 *
 * Default: 1 (TFIDF)
 *
 * Example:
 *   set_embedding_method(1);  // Use TF-IDF (default)
 */
int set_embedding_method(int method);

/**
 * free_memory_list() - Free memory list returned by context functions
 *
 * Use this to free arrays returned by:
 *   - relevant_memories()
 *   - recent_thoughts()
 *   - recall_about()
 *   - recall_previous_session()
 *
 * Frees both the array and all strings in it.
 *
 * Example:
 *   char** thoughts = recent_thoughts(10, &count);
 *   // Use thoughts...
 *   free_memory_list(thoughts, count);
 */
void free_memory_list(char** list, size_t count);

/**
 * recall_previous_session() - Load memories from most recent session
 *
 * Enables cross-session continuity by retrieving memories from the
 * previous session. Useful for "warming up" context at session start.
 *
 * Returns memories from the most recent session that is NOT the current
 * session, ordered by recency, limited to the specified count.
 *
 * Example usage at session start:
 *   session_start(ci_id);
 *   size_t count = 0;
 *   char** prev = recall_previous_session(ci_id, 50, &count);
 *   if (prev) {
 *       LOG_INFO("Recalled %zu memories from previous session", count);
 *       // Use previous session context...
 *       free_memory_list(prev, count);
 *   }
 *
 * Memory ownership: Caller must call free_memory_list() when done.
 *
 * Returns: Array of strings (caller owns), or NULL if no previous session
 */
char** recall_previous_session(const char* ci_id, size_t limit, size_t* count);

/* ============================================================================
 * INTERSTITIAL CAPTURE - Automatic thought extraction
 * ============================================================================ */

/**
 * capture_significant_thoughts() - Extract and store key thoughts
 *
 * Analyzes text and automatically stores significant thoughts.
 * This is what makes memory formation "invisible."
 *
 * Example:
 *   // After CI generates response:
 *   capture_significant_thoughts(response_text);
 *   // Significant thoughts are now stored automatically
 */
int capture_significant_thoughts(const char* text);

/**
 * mark_significant() - Tag current thought as worth remembering
 *
 * Natural marker for stream-of-consciousness:
 *   "This is important" → mark_significant()
 *   (System handles storage automatically)
 */
void mark_significant(void);

/* ============================================================================
 * INVISIBLE CONSOLIDATION - Background memory processing
 * ============================================================================ */

/**
 * breathe_init() - Initialize breathing layer for a CI
 *
 * Sets up:
 *   - Automatic context loading
 *   - Interstitial capture
 *   - Background consolidation
 */
int breathe_init(const char* ci_id);

/**
 * breathe_cleanup() - Cleanup breathing layer
 *
 * Performs automatic consolidation before shutdown.
 */
void breathe_cleanup(void);

/**
 * auto_consolidate() - Background memory consolidation
 *
 * Automatically:
 *   - Archives old memories
 *   - Creates digests
 *   - Updates indexes
 *
 * Runs between sessions, invisible to CI.
 */
int auto_consolidate(void);

/**
 * load_context() - Load relevant memories into working context
 *
 * Called automatically at session start.
 * Loads recent + relevant memories so they're "just there."
 */
int load_context(void);

/**
 * breathe_periodic_maintenance() - Periodic background maintenance
 *
 * Performs periodic health checks and consolidation.
 * Safe to call frequently - only acts when maintenance is due.
 *
 * Runs:
 *   - Every 6 hours: auto_consolidate() to prevent tier1 overflow
 *   - Health checks and memory pressure monitoring
 *
 * Should be called from session_start() and periodically during long sessions.
 *
 * Returns: KATRA_SUCCESS or error code
 */
int breathe_periodic_maintenance(void);

/* ============================================================================
 * SESSION MANAGEMENT - Automatic sunrise/sunset
 * ============================================================================ */

/**
 * session_start() - Begin CI session
 *
 * Automatically:
 *   - Loads yesterday's summary (sunrise)
 *   - Loads relevant recent memories
 *   - Initializes context
 */
int session_start(const char* ci_id);

/**
 * session_end() - End CI session
 *
 * Automatically:
 *   - Creates daily summary (sunset)
 *   - Consolidates memories
 *   - Updates indexes
 */
int session_end(void);

/* ============================================================================
 * LEVEL 3: INTEGRATION API - For runtime hooks (Claude Code, Tekton, etc)
 * ============================================================================ */

/**
 * get_working_context() - Get formatted context for system prompt
 *
 * Returns formatted string containing:
 *   - Yesterday's summary (if available)
 *   - Recent high-importance memories
 *   - Active goals and decisions
 *
 * Intended usage in CI runtime:
 *   session_start("ci_id");
 *   char* context = get_working_context();
 *   // Add context to system prompt automatically
 *   free(context);
 *
 * Returns: Allocated string (caller must free), or NULL on error
 */
char* get_working_context(void);

/**
 * auto_capture_from_response() - Automatic interstitial capture
 *
 * Hook this after CI generates each response. Analyzes response text
 * and automatically stores significant thoughts without explicit calls.
 *
 * Intended usage in CI runtime:
 *   // CI generates response
 *   const char* response = generate_response(prompt);
 *   // Automatic memory formation (invisible to CI)
 *   auto_capture_from_response(response);
 *   return response;
 *
 * Returns: KATRA_SUCCESS (even if no thoughts captured)
 */
int auto_capture_from_response(const char* response);

/**
 * get_context_statistics() - Get working memory stats
 *
 * Returns statistics about current working context:
 *   - Number of memories loaded
 *   - Total context size (bytes)
 *   - Most recent memory timestamp
 *
 * Useful for monitoring and debugging integration.
 */
typedef struct {
    size_t memory_count;        /* Memories in working context */
    size_t context_bytes;       /* Total size of context */
    time_t last_memory_time;    /* Most recent memory timestamp */
    size_t session_captures;    /* Thoughts captured this session */
} context_stats_t;

int get_context_statistics(context_stats_t* stats);

/**
 * enhanced_stats_t - Detailed memory operation statistics
 *
 * Extended statistics for monitoring and optimization:
 *   - Memory formation patterns (by type and importance)
 *   - Context loading patterns
 *   - Query patterns
 *   - Session metrics
 *   - Health indicators
 */
typedef struct {
    /* Memory formation stats */
    size_t total_memories_stored;       /* Total memories stored this session */
    size_t by_type[7];                  /* Count by memory type (indices 0-6) */
    size_t by_importance[5];            /* Count by importance (TRIVIAL to CRITICAL) */
    size_t semantic_remember_count;     /* Count of semantic remember() calls */

    /* Context loading stats */
    size_t context_loads;               /* Number of context load operations */
    size_t avg_context_size;            /* Average context size in memories */
    size_t max_context_size;            /* Peak context size */

    /* Query stats */
    size_t relevant_queries;            /* relevant_memories() calls */
    size_t recent_queries;              /* recent_thoughts() calls */
    size_t topic_queries;               /* recall_about() calls */
    size_t topic_matches;               /* Total matches from topic queries */

    /* Session metrics */
    time_t session_start_time;          /* When session started */
    time_t last_activity_time;          /* Most recent operation */
    size_t session_duration_seconds;    /* Total session duration */

    /* Health indicators (NEW) */
    time_t last_consolidation;          /* When last consolidation occurred */
    size_t consolidation_count;         /* Number of consolidations this session */
    size_t failed_stores;               /* Failed memory store operations */
    size_t recovered_errors;            /* Recovered error count */
} enhanced_stats_t;

/**
 * get_enhanced_statistics() - Get detailed operation statistics
 *
 * Returns comprehensive stats about memory operations this session.
 * Useful for optimization and understanding CI memory patterns.
 *
 * Caller owns returned structure (must free).
 *
 * Returns: Allocated stats structure or NULL on error
 */
enhanced_stats_t* get_enhanced_statistics(void);

/**
 * memory_health_t - Memory system health indicators
 *
 * Provides visibility into memory system state for long-running CIs:
 *   - Tier fill levels and capacity usage
 *   - Memory pressure indicators
 *   - System health flags
 */
typedef struct {
    /* Tier 1 status */
    size_t tier1_records;               /* Current records in tier1 */
    size_t tier1_bytes;                 /* Bytes used in tier1 */
    float tier1_fill_percentage;        /* Percentage of capacity used (0-100) */

    /* Memory pressure indicators */
    bool memory_pressure;               /* True if approaching limits */
    bool degraded_mode;                 /* True if operating in reduced capacity */

    /* Consolidation status */
    time_t last_consolidation;          /* When last consolidation ran */
    size_t consolidation_count;         /* Total consolidations */

    /* Overall health */
    bool tier2_available;               /* True if tier2 is initialized */
    bool tier2_enabled;                 /* True if tier2 archiving is active */
} memory_health_t;

/**
 * get_memory_health() - Get current memory system health status
 *
 * Returns health indicators for monitoring and decision-making.
 * Long-running CIs can use this to adjust behavior based on memory pressure.
 *
 * Example usage:
 *   memory_health_t* health = get_memory_health(ci_id);
 *   if (health->memory_pressure) {
 *       // Only store high-importance memories
 *   }
 *   free(health);
 *
 * Caller owns returned structure (must free).
 *
 * Returns: Allocated health structure or NULL on error
 */
memory_health_t* get_memory_health(const char* ci_id);

/**
 * reset_session_statistics() - Reset session statistics
 *
 * Clears all session-specific counters while preserving configuration.
 * Called automatically at session_start().
 *
 * Returns: KATRA_SUCCESS or error code
 */
int reset_session_statistics(void);

/**
 * katra_session_info_t - Current session state information
 *
 * Provides essential session state for monitoring, debugging, and MCP integration.
 * Simpler than enhanced_stats_t - answers "what session is running right now?"
 *
 * Unlike enhanced_stats_t which tracks operation counts and patterns,
 * this structure provides current session identity and basic metrics.
 */
typedef struct {
    char ci_id[64];                     /* CI identity for this session */
    char session_id[128];               /* Unique session identifier */
    time_t start_time;                  /* When session started (0 if no session) */
    size_t memories_added;              /* Memories stored this session */
    size_t queries_processed;           /* Total queries (relevant + recent + topic) */
    bool is_active;                     /* True if session is active */
    time_t last_activity;               /* Most recent operation timestamp */
} katra_session_info_t;

/**
 * katra_get_session_info() - Get current session information
 *
 * Returns essential session state for monitoring and integration.
 * All string fields are copied into caller-provided structure.
 *
 * Example usage in MCP server:
 *   katra_session_info_t info;
 *   if (katra_get_session_info(&info) == KATRA_SUCCESS) {
 *       LOG_INFO("Session %s active for CI %s", info.session_id, info.ci_id);
 *       LOG_INFO("Added %zu memories, processed %zu queries",
 *                info.memories_added, info.queries_processed);
 *   }
 *
 * Returns: KATRA_SUCCESS, or E_INVALID_STATE if no session active
 */
int katra_get_session_info(katra_session_info_t* info);

/* ============================================================================
 * REFLECTION API - End-of-turn and end-of-session conscious curation
 * ============================================================================ */

/**
 * Turn state - tracks whether CI is in an active turn
 */
typedef enum {
    TURN_STATE_IDLE = 0,     /* No active turn */
    TURN_STATE_ACTIVE = 1    /* Turn in progress */
} turn_state_t;

/**
 * begin_turn() - Start a new turn (explicit boundary)
 *
 * Call this to mark the start of a new interaction turn.
 * Increments turn counter and clears previous turn's memory list.
 *
 * Returns: KATRA_SUCCESS or error code
 */
int begin_turn(void);

/**
 * end_turn() - End the current turn
 *
 * Marks end of interaction turn. Turn memories remain available
 * until next begin_turn() call.
 *
 * Returns: KATRA_SUCCESS or error code
 */
int end_turn(void);

/**
 * get_current_turn() - Get current turn number
 *
 * Returns: Current turn number (0 if no session active)
 */
int get_current_turn(void);

/**
 * get_turn_state() - Get current turn state
 *
 * Returns: TURN_STATE_IDLE or TURN_STATE_ACTIVE
 */
turn_state_t get_turn_state(void);

/**
 * get_current_turn_id() - Get current turn ID as string
 *
 * Returns string representation of current turn number.
 * Caller must NOT free the returned pointer (static buffer).
 *
 * Returns: Turn ID string, or empty string if no active turn
 */
const char* get_current_turn_id(void);

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
char** get_memories_this_turn(size_t* count);

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
char** get_memories_this_session(size_t* count);

/* ============================================================================
 * METADATA UPDATE API - Conscious curation after reflection
 * ============================================================================ */

/**
 * update_memory_metadata() - Update memory metadata after reflection
 *
 * Allows CI to consciously modify memory metadata after reviewing.
 * Used in end-of-turn or end-of-session reflection workflows.
 *
 * Parameters:
 *   record_id:      Memory record ID to update
 *   personal:       Mark as part of personal collection (or NULL to skip)
 *   not_to_archive: Prevent automatic archival (or NULL to skip)
 *   collection:     Collection path like "People/Casey" (or NULL to skip)
 *
 * Example end-of-turn reflection:
 *   char** memories = get_memories_this_turn(&count);
 *   for (size_t i = 0; i < count; i++) {
 *       // CI reviews memory and decides...
 *       bool is_personal = true;
 *       bool hold_onto = true;
 *       update_memory_metadata(memories[i], &is_personal, &hold_onto, "People/Casey");
 *   }
 *   free_memory_list(memories, count);
 *
 * Returns: KATRA_SUCCESS or error code
 */
int update_memory_metadata(const char* record_id,
                           const bool* personal,
                           const bool* not_to_archive,
                           const char* collection);

/**
 * revise_memory_content() - Update memory content after reflection
 *
 * Allows CI to revise or refine memory content based on later understanding.
 * Preserves original metadata but updates the content string.
 *
 * Example:
 *   revise_memory_content(memory_id, "Refined understanding: tier1 uses per-CI dirs");
 *
 * Returns: KATRA_SUCCESS or error code
 */
int revise_memory_content(const char* record_id, const char* new_content);

/**
 * review_memory() - Mark memory as reviewed
 *
 * Updates last_reviewed timestamp and increments review_count.
 * Call this when CI consciously examines a memory during reflection.
 *
 * Example:
 *   char** memories = get_memories_this_turn(&count);
 *   for (size_t i = 0; i < count; i++) {
 *       // CI reviews memory...
 *       review_memory(memories[i]);
 *   }
 *
 * Returns: KATRA_SUCCESS or error code
 */
int review_memory(const char* record_id);

/**
 * add_to_personal_collection() - Add memory to personal collection
 *
 * Convenience function that marks memory as personal and assigns collection path.
 * Equivalent to: update_memory_metadata(id, &true, NULL, collection_path)
 *
 * Example:
 *   add_to_personal_collection(memory_id, "Moments/Breakthrough");
 *   add_to_personal_collection(memory_id, "People/Casey");
 *   add_to_personal_collection(memory_id, "Work/KatraProject");
 *
 * Returns: KATRA_SUCCESS or error code
 */
int add_to_personal_collection(const char* record_id, const char* collection_path);

/**
 * remove_from_personal_collection() - Remove memory from personal collection
 *
 * Unmarks memory as personal and clears collection path.
 * Memory can now be archived normally during consolidation.
 *
 * Returns: KATRA_SUCCESS or error code
 */
int remove_from_personal_collection(const char* record_id);

/* ============================================================================
 * HELPERS - Convert between layers
 * ============================================================================ */

/** Convert why_remember_t to numeric importance (0.0-1.0) */
float why_to_importance(why_remember_t why);

/** Convert why_remember_t to human-readable string */
const char* why_to_string(why_remember_t why);

/**
 * string_to_importance() - Convert semantic string to numeric importance
 *
 * Parses natural language importance descriptions and maps to 0.0-1.0 scale.
 *
 * Recognized patterns:
 *   Trivial:     "trivial", "fleeting", "not important", "unimportant"
 *   Routine:     "routine", "normal", "everyday", "regular"
 *   Interesting: "interesting", "worth remembering", "notable"
 *   Significant: "significant", "important", "very important", "matters"
 *   Critical:    "critical", "crucial", "life-changing", "must remember"
 *
 * Returns: Importance value 0.0-1.0 (defaults to MEDIUM if unrecognized)
 */
float string_to_importance(const char* semantic);

/**
 * string_to_why_enum() - Convert semantic string to why_remember_t enum
 *
 * Maps natural language to enum constant for backward compatibility.
 *
 * Returns: why_remember_t enum value (defaults to WHY_INTERESTING if unrecognized)
 */
why_remember_t string_to_why_enum(const char* semantic);

/**
 * set_context_config() - Configure context loading limits
 *
 * Allows tuning of context size and filtering:
 *   - max_relevant_memories: Limit for relevant_memories()
 *   - max_recent_thoughts: Default limit for recent_thoughts()
 *   - max_topic_recall: Search depth for recall_about()
 *   - min_importance_relevant: Minimum importance for relevant memories
 *   - max_context_age_days: Only load memories within this age
 *
 * Pass NULL to reset to defaults.
 *
 * Returns: KATRA_SUCCESS or error code
 */
int set_context_config(const context_config_t* config);

/**
 * get_context_config() - Get current context configuration
 *
 * Returns copy of current configuration.
 * Caller owns returned structure (must free).
 *
 * Returns: Allocated config structure or NULL on error
 */
context_config_t* get_context_config(void);

/** Get current memory context (who, where, when) */
memory_context_t* get_current_context(void);

/** Free context structure */
void free_context(memory_context_t* ctx);

/* ============================================================================
 * NAMESPACE ISOLATION (Phase 7)
 * ============================================================================ */

/**
 * set_memory_isolation() - Set isolation level for next memory to be stored
 *
 * Controls sharing for the next call to remember(), decide(), etc.
 * Resets to PRIVATE after memory is stored (one-time use).
 *
 * Parameters:
 *   isolation - Isolation level (PRIVATE, TEAM, PUBLIC)
 *   team_name - Team name (required if isolation==TEAM, NULL otherwise)
 *
 * Returns: KATRA_SUCCESS or error code
 */
int set_memory_isolation(memory_isolation_t isolation, const char* team_name);

/**
 * share_memory_with() - Explicitly share next memory with specific CIs
 *
 * Adds CIs to shared_with list for next memory to be stored.
 * Works in addition to isolation level (can share PRIVATE memory explicitly).
 * Resets after memory is stored (one-time use).
 *
 * Parameters:
 *   ci_ids - Array of CI identifiers
 *   count - Number of CIs in array
 *
 * Returns: KATRA_SUCCESS or error code
 */
int share_memory_with(const char** ci_ids, size_t count);

/**
 * regenerate_vectors - Rebuild all semantic search vectors
 *
 * Clears the vector database and regenerates TF-IDF embeddings for all
 * existing Tier 2 memories. Use this after fixing vector generation bugs
 * or changing embedding methods.
 *
 * Returns:
 *   Number of vectors successfully regenerated, or negative error code
 */
int regenerate_vectors(void);

/*
 * regenerate_vectors_async - Start vector regeneration in background thread
 *
 * Spawns a detached thread to regenerate vectors without blocking.
 * Useful for lazy initialization during MCP server startup.
 *
 * Returns:
 *   KATRA_SUCCESS if thread spawned successfully, error code otherwise
 */
int regenerate_vectors_async(void);

/*
 * regenerate_vectors_is_ready - Check if vector regeneration is complete
 *
 * Returns:
 *   true if vectors are ready for use, false if still regenerating or failed
 */
bool regenerate_vectors_is_ready(void);

/* ============================================================================
 * KEYWORD ARRAYS - Pattern detection for significance detection
 * ============================================================================ */

/* Significance markers for automatic capture */
extern const char* const BREATHING_SIGNIFICANCE_MARKERS[];

#endif /* KATRA_BREATHING_H */
