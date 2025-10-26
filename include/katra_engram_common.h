/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_ENGRAM_COMMON_H
#define KATRA_ENGRAM_COMMON_H

#include "katra_error.h"
#include <stdbool.h>
#include <stddef.h>

/* Error checking macros for engram modules */

/* Check single parameter for NULL */
#define ENGRAM_CHECK_NULL(ptr) \
    do { \
        if (!(ptr)) { \
            katra_report_error(E_INPUT_NULL, __func__, #ptr " is NULL"); \
            return E_INPUT_NULL; \
        } \
    } while(0)

/* Check single parameter for NULL (void return) */
#define ENGRAM_CHECK_NULL_VOID(ptr) \
    do { \
        if (!(ptr)) { \
            katra_report_error(E_INPUT_NULL, __func__, #ptr " is NULL"); \
            return; \
        } \
    } while(0)

/* Check multiple parameters for NULL */
#define ENGRAM_CHECK_PARAMS_2(p1, p2) \
    do { \
        if (!(p1) || !(p2)) { \
            katra_report_error(E_INPUT_NULL, __func__, "NULL parameter"); \
            return E_INPUT_NULL; \
        } \
    } while(0)

#define ENGRAM_CHECK_PARAMS_3(p1, p2, p3) \
    do { \
        if (!(p1) || !(p2) || !(p3)) { \
            katra_report_error(E_INPUT_NULL, __func__, "NULL parameter"); \
            return E_INPUT_NULL; \
        } \
    } while(0)

#define ENGRAM_CHECK_PARAMS_4(p1, p2, p3, p4) \
    do { \
        if (!(p1) || !(p2) || !(p3) || !(p4)) { \
            katra_report_error(E_INPUT_NULL, __func__, "NULL parameter"); \
            return E_INPUT_NULL; \
        } \
    } while(0)

/* String utilities for engram modules */

/* Case-insensitive keyword matching */
bool katra_str_contains_any(const char* text, const char** keywords, size_t count);

/* Case-insensitive substring search */
bool katra_str_contains(const char* text, const char* keyword);

/* Simple keyword-based similarity (0.0-1.0) */
float katra_str_similarity(const char* text1, const char* text2);

/* Count character occurrences */
size_t katra_str_count_char(const char* text, char ch);

/* Safe string duplication with error reporting */
char* katra_safe_strdup(const char* str);

/* Safe string duplication with goto cleanup on failure */
#define SAFE_STRDUP(dest, src, cleanup_label) \
    do { \
        if (src) { \
            (dest) = strdup(src); \
            if (!(dest)) { \
                katra_report_error(E_SYSTEM_MEMORY, __func__, "strdup failed"); \
                goto cleanup_label; \
            } \
        } else { \
            (dest) = NULL; \
        } \
    } while(0)

#endif /* KATRA_ENGRAM_COMMON_H */
