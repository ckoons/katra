/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_DB_H
#define KATRA_DB_H

#include "katra_memory.h"
#include <stdbool.h>

/* Database Backend Abstraction Layer
 *
 * "Store everywhere, synthesize on recall" - Every memory is stored
 * simultaneously across multiple database backends, each optimized for
 * different access patterns.
 *
 * Supported Backends:
 * - JSONL:  Source of truth, append-only, full fidelity
 * - SQLite: Fast structured queries, metadata indexing
 * - Vector: Semantic similarity search (future)
 * - Graph:  Relationship networks, association traversal (future)
 * - Cache:  Hot data, working memory (future)
 */

/* Backend types */
typedef enum {
    DB_BACKEND_JSONL = 0,
    DB_BACKEND_SQLITE = 1,
    DB_BACKEND_VECTOR = 2,
    DB_BACKEND_GRAPH = 3,
    DB_BACKEND_CACHE = 4
} db_backend_type_t;

/* Forward declarations */
typedef struct db_backend db_backend_t;
typedef struct db_query db_query_t;

/* Query structure (generic across backends) */
struct db_query {
    const char* ci_id;          /* Required: CI identifier */
    time_t start_time;          /* Time range start (0 = no limit) */
    time_t end_time;            /* Time range end (0 = no limit) */
    memory_type_t type;         /* Memory type filter (0 = all) */
    float min_importance;       /* Minimum importance (0.0 = all) */
    const char* content_match;  /* Content substring match (NULL = no filter) */
    size_t limit;               /* Max results (0 = no limit) */

    /* Backend-specific extensions */
    void* backend_params;       /* Opaque backend-specific params */
};

/* Database backend interface */
struct db_backend {
    char name[64];              /* Backend name (e.g. "jsonl", "sqlite") */
    db_backend_type_t type;     /* Backend type enum */
    void* context;              /* Backend-specific context */
    bool initialized;           /* Initialization status */

    /* Lifecycle operations */
    int (*init)(void* ctx, const char* ci_id);
    void (*cleanup)(void* ctx);

    /* Storage operations */
    int (*store)(void* ctx, const memory_record_t* record);
    int (*retrieve)(void* ctx, const char* record_id, memory_record_t** record);
    int (*query)(void* ctx, const db_query_t* query,
                 memory_record_t*** results, size_t* count);

    /* Metadata operations */
    int (*get_stats)(void* ctx, size_t* record_count, size_t* bytes_used);

    /* Optional operations (may be NULL) */
    int (*delete_record)(void* ctx, const char* record_id);
    int (*update)(void* ctx, const memory_record_t* record);
};

/* Create backend instances */
db_backend_t* katra_db_create_jsonl_backend(const char* ci_id);
db_backend_t* katra_db_create_sqlite_backend(const char* ci_id);

/* Generic backend operations */
int katra_db_backend_init(db_backend_t* backend, const char* ci_id);
void katra_db_backend_cleanup(db_backend_t* backend);
int katra_db_backend_store(db_backend_t* backend, const memory_record_t* record);
int katra_db_backend_retrieve(db_backend_t* backend, const char* record_id,
                               memory_record_t** record);
int katra_db_backend_query(db_backend_t* backend, const db_query_t* query,
                            memory_record_t*** results, size_t* count);

/* Free backend instance */
void katra_db_backend_free(db_backend_t* backend);

#endif /* KATRA_DB_H */
