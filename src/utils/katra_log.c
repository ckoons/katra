/* © 2025 Casey Koons All rights reserved */
/* Logging system - writes to files for background processes */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

/* Project includes */
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_error.h"
#include "katra_string_literals.h"

/* Global log configuration */
log_config_t* g_log_config = NULL;

/* Initialize logging system */
int log_init(const char* log_dir) {
    if (g_log_config) {
        return KATRA_SUCCESS;  /* Already initialized */
    }

    g_log_config = calloc(1, sizeof(log_config_t));
    if (!g_log_config) {
        return E_SYSTEM_MEMORY;
    }

    /* Create log directory if needed */
    const char* dir = log_dir ? log_dir : LOG_DEFAULT_DIR;

    /* Expand ~ to HOME if needed */
    char expanded_dir[KATRA_PATH_MAX];
    if (dir[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            snprintf(expanded_dir, sizeof(expanded_dir), "%s%s", home, dir + 1);
            dir = expanded_dir;
        }
    } else if (dir[0] != '/') {
        /* Relative path - make it relative to HOME */
        const char* home = getenv("HOME");
        if (home) {
            snprintf(expanded_dir, sizeof(expanded_dir), "%s/%s", home, dir);
            dir = expanded_dir;
        }
    }

    mkdir(dir, KATRA_DIR_PERMISSIONS);

    /* Allocate log directory path */
    g_log_config->log_dir = strdup(dir);
    if (!g_log_config->log_dir) {
        free(g_log_config);
        g_log_config = NULL;
        return E_SYSTEM_MEMORY;
    }

    /* Build log filename with PID */
    char filename[KATRA_BUFFER_SMALL];
    pid_t pid = getpid();
    snprintf(filename, sizeof(filename), "katra_process_%d.log", pid);

    size_t path_len = strlen(dir) + strlen(filename) + 2;
    g_log_config->log_file = malloc(path_len);
    if (!g_log_config->log_file) {
        free(g_log_config->log_dir);
        free(g_log_config);
        g_log_config = NULL;
        return E_SYSTEM_MEMORY;
    }

    snprintf(g_log_config->log_file, path_len, "%s/%s", dir, filename);

    /* Open log file */
    g_log_config->log_fp = fopen(g_log_config->log_file, "a");
    if (!g_log_config->log_fp) {
        free(g_log_config->log_file);
        free(g_log_config->log_dir);
        free(g_log_config);
        g_log_config = NULL;
        return E_SYSTEM_FILE;
    }

    /* Set unbuffered for immediate writes */
    setbuf(g_log_config->log_fp, NULL);

    /* Default configuration */
    g_log_config->enabled = true;
    g_log_config->level = LOG_INFO;
    g_log_config->use_stdout = false;  /* Background processes don't use stdout */
    g_log_config->use_stderr = false;  /* Background processes don't use stderr */

    /* Write initialization message */
    fprintf(g_log_config->log_fp, "\n=== Log initialized: %s (PID %d) ===\n",
            g_log_config->log_file, pid);

    return KATRA_SUCCESS;
}

/* GUIDELINE_APPROVED - Log marker message */
/* Cleanup logging system */
void log_cleanup(void) {
    if (!g_log_config) return;

    if (g_log_config->log_fp) {
        fprintf(g_log_config->log_fp, "=== Log closed ===\n");
        fclose(g_log_config->log_fp);
    }

    free(g_log_config->log_file);
    free(g_log_config->log_dir);
    free(g_log_config);
    g_log_config = NULL;
}
/* GUIDELINE_APPROVED_END */

/* Set log level */
void log_set_level(log_level_t level) {
    if (g_log_config) {
        g_log_config->level = level;
    }
}

/* Get log level string */
/* GUIDELINE_APPROVED: Lookup table for log level enum-to-string conversion */
const char* log_level_string(log_level_t level) {
    switch (level) {
        case LOG_FATAL: return "FATAL";
        case LOG_ERROR: return "ERROR";
        case LOG_WARN:  return "WARN ";
        case LOG_INFO:  return "INFO ";
        case LOG_DEBUG: return "DEBUG";
        case LOG_TRACE: return "TRACE";
        default:        return "?????";
    }
}
/* GUIDELINE_APPROVED_END */

/* Core logging function */
void log_write(log_level_t level, const char* file, int line,
              const char* func, const char* fmt, ...) {
    if (!g_log_config || !g_log_config->enabled) {
        return;
    }

    if (level > g_log_config->level) {
        return;  /* Below current log level */
    }

    if (!g_log_config->log_fp) {
        return;  /* No log file */
    }

    /* Get timestamp */
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[KATRA_BUFFER_TINY];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    /* Extract just the filename (not full path) */
    const char* filename = strrchr(file, '/');
    if (filename) {
        filename++;  /* Skip the '/' */
    } else {
        filename = file;
    }

    /* Write log header */
    fprintf(g_log_config->log_fp, "[%s] %s %s:%d (%s): ",
            timestamp, log_level_string(level), filename, line, func);

    /* Write log message */
    va_list args;
    va_start(args, fmt);
    vfprintf(g_log_config->log_fp, fmt, args);
    va_end(args);

    fprintf(g_log_config->log_fp, "\n");
}

/* Get log level */
log_level_t log_get_level(void) {
    if (!g_log_config) return LOG_INFO;
    return g_log_config->level;
}

/* Check if logging is enabled */
bool log_is_enabled(void) {
    return g_log_config && g_log_config->enabled;
}

/* Get log location */
const char* log_get_location(void) {
    if (!g_log_config) return NULL;
    return g_log_config->log_dir;
}

/* Log error code with context */
void log_error_code(int error_code, const char* context) {
    if (error_code == KATRA_SUCCESS) return;

    LOG_ERROR("Error %d in %s: %s",
              error_code,
              context ? context : STR_UNKNOWN,
              katra_error_string(error_code));
}

/* Log memory tier operation */
void log_memory_operation(const char* operation, const char* tier, size_t size) {
    LOG_INFO("Memory operation: %s [tier=%s, size=%zu bytes]",
             operation, tier, size);
}

/* Log checkpoint operation */
void log_checkpoint_operation(const char* operation, const char* checkpoint_id) {
    LOG_INFO("Checkpoint operation: %s [id=%s]",
             operation, checkpoint_id);
}

/* Log consent request */
void log_consent_request(const char* operation, const char* requestor) {
    LOG_INFO("Consent request: %s [requestor=%s]",
             operation, requestor);
}

/* Log performance metric */
void log_performance(const char* operation, double elapsed_ms) {
    LOG_DEBUG("Performance: %s completed in %.2f ms",
              operation, elapsed_ms);
}
