/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_ENCODER_H
#define KATRA_ENCODER_H

#include "katra_db.h"

/* Universal Encoder
 *
 * "Store everywhere, synthesize on recall"
 *
 * The universal encoder stores each memory simultaneously across multiple
 * database backends, each optimized for different access patterns:
 * - JSONL: Source of truth, append-only, full fidelity
 * - SQLite: Fast structured queries, metadata indexing
 * - Vector: Semantic similarity search (future)
 * - Graph: Relationship networks (future)
 * - Cache: Hot data, working memory (future)
 */

/* Maximum number of backends */
#define MAX_BACKENDS 5

/* Universal encoder structure */
typedef struct {
    char ci_id[256];
    db_backend_t* backends[MAX_BACKENDS];
    size_t backend_count;
    bool initialized;
} universal_encoder_t;

/* Create universal encoder */
universal_encoder_t* katra_encoder_create(const char* ci_id);

/* Add backend to encoder */
int katra_encoder_add_backend(universal_encoder_t* encoder, db_backend_t* backend);

/* Initialize encoder (initializes all backends) */
int katra_encoder_init(universal_encoder_t* encoder);

/* Store to all backends simultaneously */
int katra_encoder_store(universal_encoder_t* encoder,
                        const memory_record_t* record);

/* Query from best backend (with fallback) */
int katra_encoder_query(universal_encoder_t* encoder,
                        const db_query_t* query,
                        memory_record_t*** results,
                        size_t* count);

/* Cleanup encoder (cleans up all backends) */
void katra_encoder_cleanup(universal_encoder_t* encoder);

/* Free encoder instance */
void katra_encoder_free(universal_encoder_t* encoder);

#endif /* KATRA_ENCODER_H */
