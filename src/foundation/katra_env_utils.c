/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>

/* Project includes */
#include "katra_env_utils.h"
#include "katra_env_internal.h"
#include "katra_error.h"
#include "katra_limits.h"
#include "katra_log.h"

/* Get environment variable */
const char* katra_getenv(const char* name) {
    if (!name) return NULL;

    int lock_result = pthread_mutex_lock(&katra_env_mutex);
    if (lock_result != 0) {
        LOG_ERROR("Failed to acquire mutex in katra_getenv: %d", lock_result);
        return NULL;
    }

    int idx = find_env_index(name);
    const char* result = NULL;

    if (idx >= 0) {
        char* eq = strchr(katra_env[idx], '=');
        if (eq) result = eq + 1;
    }

    pthread_mutex_unlock(&katra_env_mutex);
    return result;
}

/* Set environment variable */
int katra_setenv(const char* name, const char* value) {
    KATRA_CHECK_NULL(name);
    KATRA_CHECK_NULL(value);

    int lock_result = pthread_mutex_lock(&katra_env_mutex);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PROCESS, "katra_setenv", "Failed to acquire mutex");
        return E_SYSTEM_PROCESS;
    }
    int result = set_env_internal(name, value);
    pthread_mutex_unlock(&katra_env_mutex);

    return result;
}

/* Unset environment variable */
int katra_unsetenv(const char* name) {
    KATRA_CHECK_NULL(name);

    int lock_result = pthread_mutex_lock(&katra_env_mutex);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PROCESS, "katra_unsetenv", "Failed to acquire mutex");
        return E_SYSTEM_PROCESS;
    }

    int idx = find_env_index(name);
    if (idx >= 0) {
        free(katra_env[idx]);

        for (int i = idx; i < katra_env_count - 1; i++) {
            katra_env[i] = katra_env[i + 1];
        }

        katra_env_count--;
        katra_env[katra_env_count] = NULL;
    }

    pthread_mutex_unlock(&katra_env_mutex);
    return KATRA_SUCCESS;
}

/* Get integer environment variable */
int katra_getenvint(const char* name, int* value) {
    KATRA_CHECK_NULL(name);
    KATRA_CHECK_NULL(value);

    const char* str_value = katra_getenv(name);
    if (!str_value) return E_INPUT_FORMAT;

    char* endptr;
    errno = 0;
    long result = strtol(str_value, &endptr, DECIMAL_BASE);

    if (errno == ERANGE || result > INT_MAX || result < INT_MIN) {
        return E_INPUT_FORMAT;
    }

    if (endptr == str_value || *endptr != '\0') {
        return E_INPUT_FORMAT;
    }

    *value = (int)result;
    return KATRA_SUCCESS;
}

/* Print environment */
void katra_env_print(void) {
    int lock_result = pthread_mutex_lock(&katra_env_mutex);
    if (lock_result != 0) {
        LOG_ERROR("Failed to acquire mutex in katra_env_print: %d", lock_result);
        return;
    }

    for (int i = 0; i < katra_env_count; i++) {
        if (katra_env[i]) {
            printf("%s\n", katra_env[i]);
        }
    }

    pthread_mutex_unlock(&katra_env_mutex);
}

/* Dump environment to file */
int katra_env_dump(const char* filepath) {
    KATRA_CHECK_NULL(filepath);

    int lock_result = pthread_mutex_lock(&katra_env_mutex);
    if (lock_result != 0) {
        katra_report_error(E_SYSTEM_PROCESS, "katra_env_dump", "Failed to acquire mutex");
        return E_SYSTEM_PROCESS;
    }

    FILE* fp = fopen(filepath, "w");
    if (!fp) {
        pthread_mutex_unlock(&katra_env_mutex);
        return E_SYSTEM_FILE;
    }

    fprintf(fp, "# Katra Environment Dump\n");
    fprintf(fp, "# Total variables: %d\n\n", katra_env_count);

    for (int i = 0; i < katra_env_count; i++) {
        if (katra_env[i]) {
            fprintf(fp, "%s\n", katra_env[i]);
        }
    }

    fclose(fp);
    pthread_mutex_unlock(&katra_env_mutex);

    return KATRA_SUCCESS;
}
