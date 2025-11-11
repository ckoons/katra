/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_CORE_COMMON_H
#define KATRA_CORE_COMMON_H

#include "katra_error.h"
#include <stddef.h>
#include <string.h>

/* Common macros and helpers for core Katra modules
 *
 * This header provides standardized patterns for:
 * - Parameter validation
 * - Memory allocation with error checking
 * - String operations
 * - Common cleanup patterns
 */

/* ===== Parameter Validation ===== */

/* Note: KATRA_CHECK_NULL already defined in katra_error.h (simple version)
 * These macros add error reporting for better debugging
 */

/* Check single parameter for NULL with error reporting */
#define KATRA_CHECK_NULL_REPORT(ptr) \
    do { \
        if (!(ptr)) { \
            katra_report_error(E_INPUT_NULL, __func__, #ptr " is NULL"); \
            return E_INPUT_NULL; \
        } \
    } while(0)

/* Check single parameter for NULL (void return) with error reporting */
#define KATRA_CHECK_NULL_VOID_REPORT(ptr) \
    do { \
        if (!(ptr)) { \
            katra_report_error(E_INPUT_NULL, __func__, #ptr " is NULL"); \
            return; \
        } \
    } while(0)

/* Check multiple parameters for NULL with error reporting */
#define KATRA_CHECK_PARAMS_2(p1, p2) \
    do { \
        if (!(p1) || !(p2)) { \
            katra_report_error(E_INPUT_NULL, __func__, "NULL parameter"); \
            return E_INPUT_NULL; \
        } \
    } while(0)

#define KATRA_CHECK_PARAMS_3(p1, p2, p3) \
    do { \
        if (!(p1) || !(p2) || !(p3)) { \
            katra_report_error(E_INPUT_NULL, __func__, "NULL parameter"); \
            return E_INPUT_NULL; \
        } \
    } while(0)

#define KATRA_CHECK_PARAMS_4(p1, p2, p3, p4) \
    do { \
        if (!(p1) || !(p2) || !(p3) || !(p4)) { \
            katra_report_error(E_INPUT_NULL, __func__, "NULL parameter"); \
            return E_INPUT_NULL; \
        } \
    } while(0)

#define KATRA_CHECK_PARAMS_5(p1, p2, p3, p4, p5) \
    do { \
        if (!(p1) || !(p2) || !(p3) || !(p4) || !(p5)) { \
            katra_report_error(E_INPUT_NULL, __func__, "NULL parameter"); \
            return E_INPUT_NULL; \
        } \
    } while(0)

/* ===== Memory Allocation ===== */

/* Allocate with error reporting and early return */
#define ALLOC_OR_RETURN(var, type) \
    do { \
        (var) = calloc(1, sizeof(type)); \
        if (!(var)) { \
            katra_report_error(E_SYSTEM_MEMORY, __func__, "Failed to allocate " #type); \
            return E_SYSTEM_MEMORY; \
        } \
    } while(0)

/* Allocate with error reporting and early return (NULL) */
#define ALLOC_OR_RETURN_NULL(var, type) \
    do { \
        (var) = calloc(1, sizeof(type)); \
        if (!(var)) { \
            katra_report_error(E_SYSTEM_MEMORY, __func__, "Failed to allocate " #type); \
            return NULL; \
        } \
    } while(0)

/* Allocate with error reporting and goto cleanup */
#define ALLOC_OR_GOTO(var, type, label) \
    do { \
        (var) = calloc(1, sizeof(type)); \
        if (!(var)) { \
            katra_report_error(E_SYSTEM_MEMORY, __func__, "Failed to allocate " #type); \
            result = E_SYSTEM_MEMORY; \
            goto label; \
        } \
    } while(0)

/* Array allocation with error reporting and goto cleanup */
#define ALLOC_ARRAY_OR_GOTO(var, count, type, label) \
    do { \
        (var) = calloc((count), sizeof(type)); \
        if (!(var)) { \
            katra_report_error(E_SYSTEM_MEMORY, __func__, "Failed to allocate " #type " array"); \
            result = E_SYSTEM_MEMORY; \
            goto label; \
        } \
    } while(0)

/* Array allocation with error reporting and early return */
#define ALLOC_ARRAY_OR_RETURN(var, count, type) \
    do { \
        (var) = calloc((count), sizeof(type)); \
        if (!(var)) { \
            katra_report_error(E_SYSTEM_MEMORY, __func__, "Failed to allocate " #type " array"); \
            return E_SYSTEM_MEMORY; \
        } \
    } while(0)

/* ===== String Operations ===== */

/* Safe string copy with guaranteed null termination (buffer version) */
#define SAFE_STRNCPY(dest, src) \
    do { \
        if (src) { \
            strncpy((dest), (src), sizeof(dest) - 1); \
            (dest)[sizeof(dest) - 1] = '\0'; \
        } else { \
            (dest)[0] = '\0'; \
        } \
    } while(0)

/* Safe string copy for pointer destinations */
static inline void katra_safe_strcpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || dest_size == 0) {
        return;
    }
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

/* ===== Array Operations ===== */

/* Free array of strings */
void katra_free_string_array(char** strings, size_t count);

/* Free array with custom free function */
typedef void (*katra_free_fn_t)(void*);
void katra_free_array(void** items, size_t count, katra_free_fn_t free_fn);

/* ===== NLP Stop Words ===== */

/* GUIDELINE_APPROVED: NLP stop words for keyword extraction */
extern const char* const KATRA_STOP_WORDS[];

/* ===== NLP Tokenization ===== */

/* GUIDELINE_APPROVED: Standard tokenization delimiters for keyword extraction */
#define KATRA_TOKENIZE_DELIMITERS " \t\n\r.,;:!?()[]{}\"'"

#endif /* KATRA_CORE_COMMON_H */
