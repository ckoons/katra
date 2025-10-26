/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Project includes */
#include "katra_engram_common.h"
#include "katra_error.h"
#include "katra_log.h"

/* Case-insensitive substring search */
bool katra_str_contains(const char* text, const char* keyword) {
    if (!text || !keyword) {
        return false;
    }

    char* text_lower = strdup(text);
    if (!text_lower) {
        return false;
    }

    char* keyword_lower = strdup(keyword);
    if (!keyword_lower) {
        free(text_lower);
        return false;
    }

    /* Convert both to lowercase */
    for (size_t i = 0; text_lower[i]; i++) {
        text_lower[i] = tolower(text_lower[i]);
    }
    for (size_t i = 0; keyword_lower[i]; i++) {
        keyword_lower[i] = tolower(keyword_lower[i]);
    }

    /* Search */
    bool found = (strstr(text_lower, keyword_lower) != NULL);

    free(text_lower);
    free(keyword_lower);

    return found;
}

/* Case-insensitive keyword matching (any of keywords) */
bool katra_str_contains_any(const char* text, const char** keywords, size_t count) {
    if (!text || !keywords) {
        return false;
    }

    char* text_lower = strdup(text);
    if (!text_lower) {
        return false;
    }

    /* Convert to lowercase */
    for (size_t i = 0; text_lower[i]; i++) {
        text_lower[i] = tolower(text_lower[i]);
    }

    /* Check each keyword */
    for (size_t i = 0; i < count; i++) {
        if (strstr(text_lower, keywords[i]) != NULL) {
            free(text_lower);
            return true;
        }
    }

    free(text_lower);
    return false;
}

/* Simple keyword-based similarity */
float katra_str_similarity(const char* text1, const char* text2) {
    if (!text1 || !text2) {
        return 0.0f;
    }

    /* Duplicate and convert to lowercase */
    char* t1 = strdup(text1);
    char* t2 = strdup(text2);
    if (!t1 || !t2) {
        free(t1);
        free(t2);
        return 0.0f;
    }

    /* Convert to lowercase */
    for (size_t i = 0; t1[i]; i++) t1[i] = tolower(t1[i]);
    for (size_t i = 0; t2[i]; i++) t2[i] = tolower(t2[i]);

    /* Count matching significant words (>3 chars) */
    size_t matches = 0;
    size_t total = 0;

    char* word1 = strtok(t1, " .,!?;:\n\t");
    while (word1) {
        if (strlen(word1) > 3) {
            total++;
            if (strstr(t2, word1)) {
                matches++;
            }
        }
        word1 = strtok(NULL, " .,!?;:\n\t");
    }

    free(t1);
    free(t2);

    if (total == 0) {
        return 0.0f;
    }

    return (float)matches / (float)total;
}

/* Count character occurrences */
size_t katra_str_count_char(const char* text, char ch) {
    if (!text) {
        return 0;
    }

    size_t count = 0;
    for (size_t i = 0; text[i]; i++) {
        if (text[i] == ch) {
            count++;
        }
    }
    return count;
}

/* Safe string duplication with error reporting */
char* katra_safe_strdup(const char* str) {
    if (!str) {
        return NULL;
    }

    char* dup = strdup(str);
    if (!dup) {
        katra_report_error(E_SYSTEM_MEMORY, __func__, "strdup failed");
    }

    return dup;
}
