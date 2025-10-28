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

/** Memory context - automatically captured */
typedef struct {
    char* ci_id;                    /* Who is remembering */
    char* session_id;               /* Current session */
    time_t when;                    /* When this happened */
    const char* where;              /* What component/context */
    bool auto_captured;             /* Was this interstitial? */
} memory_context_t;

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

/**
 * notice_pattern() - Store an observed pattern
 *
 * Example:
 *   notice_pattern("CIs find numeric importance scores unnatural");
 */
int notice_pattern(const char* pattern);

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
 * free_memory_list() - Free memory list returned by context functions
 *
 * Use this to free arrays returned by:
 *   - relevant_memories()
 *   - recent_thoughts()
 *   - recall_about()
 *
 * Frees both the array and all strings in it.
 *
 * Example:
 *   char** thoughts = recent_thoughts(10, &count);
 *   // Use thoughts...
 *   free_memory_list(thoughts, count);
 */
void free_memory_list(char** list, size_t count);

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

/* ============================================================================
 * HELPERS - Convert between layers
 * ============================================================================ */

/** Convert why_remember_t to numeric importance (0.0-1.0) */
float why_to_importance(why_remember_t why);

/** Convert why_remember_t to human-readable string */
const char* why_to_string(why_remember_t why);

/** Get current memory context (who, where, when) */
memory_context_t* get_current_context(void);

/** Free context structure */
void free_context(memory_context_t* ctx);

#endif /* KATRA_BREATHING_H */
