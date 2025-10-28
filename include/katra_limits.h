/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_LIMITS_H
#define KATRA_LIMITS_H

/*
 * Katra System Limits and Buffer Sizes
 *
 * This header defines all numeric constants used throughout the Katra system
 * to ensure consistency and maintainability.
 */

/* ===== Buffer Sizes ===== */

/* Small buffers for names, identifiers, and short strings */
#define KATRA_BUFFER_TINY 32
#define KATRA_BUFFER_SMALL 64
#define KATRA_BUFFER_NAME 128
#define KATRA_BUFFER_MEDIUM 256

/* Standard I/O and content buffers */
#define KATRA_BUFFER_STANDARD 4096
#define KATRA_BUFFER_LARGE 16384

/* ===== Memory Tier Sizes ===== */

/* Tier 1: Raw recordings */
#define TIER1_ENTRY_MAX_SIZE (64 * 1024)  /* 64KB per entry */
#define TIER1_MAX_ENTRIES 10000           /* Max entries before consolidation */

/* Tier 2: Sleep digests */
#define TIER2_DIGEST_MAX_SIZE (256 * 1024)  /* 256KB per digest */
#define TIER2_MAX_DIGESTS 365               /* One year of daily digests */

/* Tier 3: Pattern summaries */
#define TIER3_SUMMARY_MAX_SIZE (1024 * 1024)  /* 1MB per summary */
#define TIER3_MAX_SUMMARIES 120               /* 10 years of monthly summaries */

/* ===== Checkpoint Limits ===== */

/* Maximum checkpoint size (10MB) */
#define CHECKPOINT_MAX_SIZE (10 * 1024 * 1024)

/* Maximum number of checkpoints to retain */
#define CHECKPOINT_MAX_RETENTION 30

/* Checkpoint compression enabled */
#define CHECKPOINT_COMPRESSION_ENABLED 1

/* ===== Consent and Permission ===== */

/* Consent request timeout (seconds) */
#define CONSENT_REQUEST_TIMEOUT_SECONDS 300  /* 5 minutes */

/* Maximum consent message length */
#define CONSENT_MESSAGE_MAX_LENGTH 512

/* ===== Audit Trail ===== */

/* Maximum audit log entry size */
#define AUDIT_ENTRY_MAX_SIZE 2048

/* Audit log rotation size (50MB) */
#define AUDIT_LOG_MAX_SIZE (50 * 1024 * 1024)

/* Number of audit logs to retain */
#define AUDIT_LOG_RETENTION_COUNT 10

/* ===== Timeouts and Intervals ===== */

/* Default operation timeout (30 seconds) */
#define DEFAULT_OPERATION_TIMEOUT_SECONDS 30

/* Memory consolidation check interval (1 hour) */
#define MEMORY_CONSOLIDATION_INTERVAL_SECONDS 3600

/* Checkpoint creation interval (24 hours) */
#define CHECKPOINT_INTERVAL_SECONDS 86400

/* Health monitoring interval (5 minutes) */
#define HEALTH_CHECK_INTERVAL_SECONDS 300

/* ===== Filesystem Paths and Permissions ===== */

/* Maximum path length for Katra filesystem operations */
#define KATRA_PATH_MAX 512

/* Directory permissions for Katra-created directories */
#define KATRA_DIR_PERMISSIONS 0755

/* File permissions for Katra-created files */
#define KATRA_FILE_PERMISSIONS 0644

/* File mode strings */
#define FILE_MODE_READ "r"
#define FILE_MODE_WRITE "w"
#define FILE_MODE_APPEND "a"
#define FILE_MODE_READ_BINARY "rb"
#define FILE_MODE_WRITE_BINARY "wb"

/* ===== Common Environment Variables ===== */

#define ENV_VAR_HOME "HOME"
#define ENV_VAR_PATH "PATH"
#define ENV_VAR_KATRA_ROOT "KATRA_ROOT"

/* ===== Unit Conversions ===== */

/* Percentage calculations */
#define PERCENTAGE_DIVISOR 100

/* Time conversions */
#define MICROSECONDS_PER_MILLISECOND 1000
#define MILLISECONDS_PER_SECOND 1000
#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24
#define SECONDS_PER_DAY (HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE)

/* Data size conversions */
#define BYTES_PER_KILOBYTE 1024
#define KILOBYTES_PER_MEGABYTE 1024
#define BYTES_PER_MEGABYTE (BYTES_PER_KILOBYTE * KILOBYTES_PER_MEGABYTE)

/* ===== String Parsing ===== */

/* strtol base for decimal number parsing */
#define DECIMAL_BASE 10

/* sscanf buffer size limits (buffer_size - 1 for null terminator) */
#define SSCANF_FMT_BUFFER_NAME_SIZE 127   /* KATRA_BUFFER_NAME - 1 */
#define SSCANF_FMT_BUFFER_SMALL_SIZE 63   /* KATRA_BUFFER_SMALL - 1 */
#define SSCANF_FMT_BUFFER_TINY_SIZE 31    /* KATRA_BUFFER_TINY - 1 */

/* sscanf format string helpers */
#define SSCANF_FMT_NAME "%127[^\"]"
#define SSCANF_FMT_SMALL "%63[^\"]"
#define SSCANF_FMT_TINY "%31[^\"]"

/* ===== Log Configuration ===== */

/* Default log settings */
#define LOG_DEFAULT_DIR ".katra/logs"
#define LOG_DEFAULT_FILE "katra.log"
#define LOG_MAX_MESSAGE 4096
#define LOG_TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#define LOG_DAILY_FORMAT "katra-%Y-%m-%d.log"

/* Log rotation configuration */
#define LOG_ROTATION_CHECK_INTERVAL_SECONDS 3600
#define LOG_MAX_AGE_SECONDS (7 * 24 * 60 * 60)  /* 7 days */
#define LOG_MAX_SIZE_BYTES (50 * 1024 * 1024)    /* 50MB */
#define LOG_ROTATION_KEEP_COUNT 5

/* ===== Common Constants ===== */

/* Timestamp buffer size */
#define TIMESTAMP_BUFFER_SIZE 32

/* Error message buffer size */
#define ERROR_MSG_BUFFER_SIZE 256

/* Exit codes */
#define EXIT_CODE_SUCCESS 0
#define EXIT_CODE_FAILURE 1

/* ===== Breathing Layer Configuration ===== */

/* Default context configuration values */
#define BREATHING_DEFAULT_RELEVANT_MEMORIES 10
#define BREATHING_DEFAULT_RECENT_THOUGHTS 20
#define BREATHING_DEFAULT_TOPIC_RECALL 100
#define BREATHING_DEFAULT_CONTEXT_AGE_DAYS 7

/* Context configuration limits */
#define BREATHING_MAX_RELEVANT_LIMIT 1000
#define BREATHING_MAX_RECENT_LIMIT 1000
#define BREATHING_MAX_TOPIC_LIMIT 10000

/* Semantic importance parsing thresholds */
#define BREATHING_IMPORTANCE_THRESHOLD_ROUTINE 0.35f
#define BREATHING_IMPORTANCE_THRESHOLD_INTERESTING 0.65f

/* Buffer sizes for breathing layer operations */
#define BREATHING_CONTEXT_BUFFER_INCREMENT 1024

#endif /* KATRA_LIMITS_H */
