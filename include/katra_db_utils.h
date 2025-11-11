/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_DB_UTILS_H
#define KATRA_DB_UTILS_H

#include <sqlite3.h>

/* SQLite binding helper functions - provide common defaults */

/* Bind text with SQLITE_STATIC (most common case) */
static inline int katra_bind_text(sqlite3_stmt* stmt, int index, const char* value) {
    return sqlite3_bind_text(stmt, index, value, -1, SQLITE_STATIC);
}

/* Bind text with SQLITE_TRANSIENT (for temporary strings) */
static inline int katra_bind_text_transient(sqlite3_stmt* stmt, int index, const char* value) {
    return sqlite3_bind_text(stmt, index, value, -1, SQLITE_TRANSIENT);
}

/* Bind integer */
static inline int katra_bind_int(sqlite3_stmt* stmt, int index, int value) {
    return sqlite3_bind_int(stmt, index, value);
}

/* Bind 64-bit integer */
static inline int katra_bind_int64(sqlite3_stmt* stmt, int index, sqlite3_int64 value) {
    return sqlite3_bind_int64(stmt, index, value);
}

/* Bind double */
static inline int katra_bind_double(sqlite3_stmt* stmt, int index, double value) {
    return sqlite3_bind_double(stmt, index, value);
}

/* Bind blob with SQLITE_STATIC */
static inline int katra_bind_blob(sqlite3_stmt* stmt, int index, const void* data, int size) {
    return sqlite3_bind_blob(stmt, index, data, size, SQLITE_STATIC);
}

/* Bind blob with SQLITE_TRANSIENT */
static inline int katra_bind_blob_transient(sqlite3_stmt* stmt, int index, const void* data, int size) {
    return sqlite3_bind_blob(stmt, index, data, size, SQLITE_TRANSIENT);
}

#endif /* KATRA_DB_UTILS_H */
