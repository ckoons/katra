/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_LIMITS_H
#define KATRA_LIMITS_H

/*
 * Katra System Limits and Buffer Sizes
 *
 * This header defines all numeric constants used throughout the Katra system
 * to ensure consistency and maintainability.
 */

/* ===== Version Information ===== */
#define KATRA_VERSION_MAJOR 0
#define KATRA_VERSION_MINOR 1
#define KATRA_VERSION_PATCH 0
#define KATRA_VERSION "0.1.0"

/* Stringify macro for embedding constants in strings */
#define KATRA_STRINGIFY_HELPER(x) #x
#define KATRA_STRINGIFY(x) KATRA_STRINGIFY_HELPER(x)

/* ===== Buffer Sizes ===== */

/* Small buffers for names, identifiers, and short strings */
#define KATRA_BUFFER_TINY 32
#define KATRA_BUFFER_SMALL 64
#define KATRA_BUFFER_NAME 128
#define KATRA_BUFFER_MEDIUM 256
#define KATRA_BUFFER_MESSAGE 512    /* Message and error detail buffers */
#define KATRA_BUFFER_TEXT 1024      /* Text processing and conversion buffers */

/* Identity-specific buffer sizes */
#define KATRA_PERSONA_SIZE KATRA_BUFFER_NAME   /* Persona name buffer size */
#define KATRA_CI_ID_SIZE KATRA_BUFFER_MEDIUM   /* CI identity string size */
#define KATRA_ROLE_SIZE KATRA_BUFFER_SMALL     /* CI role buffer size */

/* Standard I/O and content buffers */
#define KATRA_BUFFER_STANDARD 4096
#define KATRA_BUFFER_ENHANCED 8192  /* Enhanced context and protocol buffers */
#define KATRA_BUFFER_LARGE 16384

/* Growth buffer for streaming operations */
#define KATRA_BUFFER_GROWTH_THRESHOLD 1024  /* Reserve 1KB margin when growing buffers */

/* Extra large buffers for JSON serialization */
#define KATRA_BUFFER_XLARGE 32768           /* 32KB for large JSON structures */

/* HTTP/Network constants */
#define HTTP_STATUS_LINE_OK "HTTP/1.1 200 OK\r\n"
#define MAX_TCP_PORT 65535                  /* Maximum valid TCP port number */

/* Semantic search defaults (breathing layer) */
#define SEMANTIC_DEFAULT_MAX_RESULTS 20     /* Default max semantic search results */
#define SEMANTIC_DEFAULT_THRESHOLD 0.3f     /* 30% similarity threshold */

/* Graph edge detection defaults */
#define GRAPH_DEFAULT_SIMILARITY_THRESHOLD 0.5f   /* 50% for SIMILAR edges */
#define GRAPH_DEFAULT_TEMPORAL_WINDOW_SEC 300     /* 5 minutes for SEQUENTIAL edges */

/* Time unit constants */
#define SECONDS_PER_HOUR 3600               /* For time calculations */

/* Search and recall multipliers */
#define EMOTION_SEARCH_MULTIPLIER 10        /* Multiplier for emotion-based search */
#define INITIAL_CAPACITY_FALLBACK 10        /* Default initial capacity for arrays */

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

/* ===== Dynamic Array Initial Capacities ===== */

/* Initial capacities for dynamically-growing arrays */
#define KATRA_INITIAL_CAPACITY_SMALL 10   /* Small collections (< 100 items expected) */
#define KATRA_INITIAL_CAPACITY_MEDIUM 16  /* Medium collections (100-1000 items) */
#define KATRA_INITIAL_CAPACITY_STANDARD 32  /* Standard collections (1000+ items) */
#define KATRA_INITIAL_CAPACITY_LARGE 64   /* Large collections (10000+ items) */
#define KATRA_INITIAL_CAPACITY_GRAPH 100  /* Graph structures */

/* Maximum pattern members in tier1 pattern detection */
#define TIER1_MAX_PATTERN_MEMBERS 256     /* Maximum members per pattern */

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

/* ===== Memory Deduplication (Phase 4.5.1) ===== */

/* Time window for duplicate detection (5 minutes) */
#define KATRA_DEDUP_TIME_WINDOW_SEC 300

/* Enable deduplication by default */
#define KATRA_DEDUP_ENABLED_DEFAULT true

/* Maximum CIs to explicitly share memory with */
#define KATRA_MAX_SHARE_COUNT 10

/* ===== Filesystem Paths and Permissions ===== */

/* Maximum path length for Katra filesystem operations */
#define KATRA_PATH_MAX 512

/* Directory permissions for Katra-created directories */
#define KATRA_DIR_PERMISSIONS 0755

/* File permissions for Katra-created files */
#define KATRA_FILE_PERMISSIONS 0644

/* File permissions for private files (e.g., persona registry) */
#define KATRA_FILE_PERMISSIONS_PRIVATE 0600

/* Unix socket file permissions (group/world writable for IPC) */
#define KATRA_SOCKET_PERMISSIONS 0666

/* Directory names for Katra standard directories */
#define KATRA_DIR_CONFIG "config"
#define KATRA_DIR_LOGS "logs"
#define KATRA_DIR_MEMORY "memory"
#define KATRA_DIR_CHECKPOINTS "checkpoints"
#define KATRA_DIR_AUDIT "audit"

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
#define NANOSECONDS_PER_SECOND 1000000000L   /* For timespec calculations */
#define MICROSECONDS_PER_MILLISECOND 1000
#define MILLISECONDS_PER_SECOND 1000
#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24
#define SECONDS_PER_DAY (HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE)
#define DAYS_PER_WEEK 7
#define WEEKS_PER_MONTH 4
#define WEEKS_PER_YEAR 52

/* Time period thresholds */
#define RECENT_ACCESS_THRESHOLD_DAYS 21   /* Consider access "recent" if < 21 days */
#define CONTEXT_WINDOW_DAYS 7             /* Default context window for queries */

/* Time component values */
#define END_OF_DAY_HOUR 23
#define END_OF_DAY_MINUTE 59
#define END_OF_DAY_SECOND 59
#define TM_YEAR_OFFSET 1900  /* struct tm year offset from 1900 */

/* Data size conversions */
#define BYTES_PER_KILOBYTE 1024
#define KILOBYTES_PER_MEGABYTE 1024
#define BYTES_PER_MEGABYTE (BYTES_PER_KILOBYTE * KILOBYTES_PER_MEGABYTE)

/* ===== Memory Preservation and Archival ===== */

/* Voluntary marking scores (absolute preservation/archival) */
#define PRESERVATION_SCORE_ABSOLUTE 100.0f   /* Voluntary preserve: always keep */
#define ARCHIVAL_SCORE_ABSOLUTE -100.0f      /* Voluntary archive: always remove */

/* Percentage multiplier for displaying ratios */
#define PERCENTAGE_MULTIPLIER 100.0f

/* ===== Tag-Based Memory (Phase 1) ===== */

/* Maximum tags per memory record */
#define KATRA_MAX_TAGS_PER_MEMORY 10

/* ===== Working Memory Budget (Phase 2) ===== */

/* Working memory budget limits */
#define WORKING_MEMORY_SOFT_LIMIT 35    /* Archive oldest session memories at this count */
#define WORKING_MEMORY_HARD_LIMIT 50    /* Delete oldest session memories at this count */
#define WORKING_MEMORY_BATCH_SIZE 10    /* Archive/delete this many at once when over limit */
#define WORKING_MEMORY_DEFAULT_ENABLED 1 /* Enable by default (0=disabled, 1=enabled) */

/* Visual salience markers (UTF-8 stars) */
#define SALIENCE_HIGH    "\u2605\u2605\u2605"   /* ★★★ High importance (0.85-1.0) */
#define SALIENCE_MEDIUM  "\u2605\u2605"         /* ★★   Medium importance (0.45-0.84) */
#define SALIENCE_LOW     "\u2605"               /* ★    Low importance (0.15-0.44) */

/* Importance thresholds for salience mapping */
#define IMPORTANCE_THRESHOLD_HIGH    0.85f
#define IMPORTANCE_THRESHOLD_MEDIUM  0.45f
#define IMPORTANCE_THRESHOLD_LOW     0.15f

/* Special tag names */
#define TAG_SESSION      "session"      /* Working memory, auto-clear on session end */
#define TAG_PERMANENT    "permanent"    /* Skip archival, keep forever */
#define TAG_PERSONAL     "personal"     /* Part of personal collection */
#define TAG_INSIGHT      "insight"      /* Reflection/learning moment */
#define TAG_TECHNICAL    "technical"    /* Technical knowledge */
#define TAG_COLLABORATIVE "collaborative" /* Shared insight with another CI */

/* ===== Algorithm Parameters ===== */

/* Cognitive workflow detection */
#define HEDGE_KEYWORD_COUNT 7               /* Number of hedge keywords to check */
#define MIN_HEDGE_DETECTION_LENGTH 10       /* Minimum content length for hedge detection */

/* Graph algorithms */
#define PAGERANK_ITERATION_COUNT 10         /* Number of PageRank iterations */

/* JSON string parsing */
#define JSON_ARCHIVED_PREFIX_LENGTH 11      /* Length of "\"archived\":" prefix */

/* ===== String Parsing ===== */

/* strtol base for decimal number parsing */
#define DECIMAL_BASE 10

/* sscanf buffer size limits (buffer_size - 1 for null terminator) */
#define SSCANF_FMT_BUFFER_NAME_SIZE 127   /* KATRA_BUFFER_NAME - 1 */
#define SSCANF_FMT_BUFFER_MEDIUM_SIZE 255 /* KATRA_BUFFER_MEDIUM - 1 */
#define SSCANF_FMT_BUFFER_NOTES_SIZE 511  /* For 512-byte notes buffer */
#define SSCANF_FMT_BUFFER_SMALL_SIZE 63   /* KATRA_BUFFER_SMALL - 1 */
#define SSCANF_FMT_BUFFER_TINY_SIZE 31    /* KATRA_BUFFER_TINY - 1 */

/* sscanf format string helpers */
#define SSCANF_FMT_NAME "%127[^\"]"
#define SSCANF_FMT_MEDIUM "%255[^\"]"
#define SSCANF_FMT_NOTES "%511[^\"]"
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

/* UUID and identifier buffer sizes */
#define UUID_BUFFER_SIZE 40                   /* Standard UUID (36 chars + padding) */
#define NAMESPACE_BUFFER_SIZE 64              /* Namespace identifier buffer */

/* Content preview limits */
#define CONTENT_PREVIEW_MAX_LENGTH 200        /* Maximum content preview length */
#define CONTENT_HINT_MAX_LENGTH 255           /* Maximum hint/snippet length */

/* Default query limits */
#define DEFAULT_MEMORY_QUERY_LIMIT 20         /* Default memory query limit */
#define DEFAULT_RECENT_MEMORIES_LIMIT 10      /* Default recent memories limit */
#define DEFAULT_MAX_TAGS 10                   /* Maximum tags per memory */
#define DEFAULT_MAX_CLIENTS 32                /* Maximum HTTP daemon clients */
#define DEFAULT_HTTP_PORT 9742                /* Default HTTP daemon port */

/* Error message buffer size */
#define ERROR_MSG_BUFFER_SIZE 256

/* Context token budget */
#define CONTEXT_TOKEN_BUDGET 100000      /* Total token budget for context (100k) */

/* Hint buffer sizes */
#define HINT_BUFFER_SIZE 256             /* Buffer for content hints */
#define HINT_ELLIPSIS_MARGIN 4           /* Room for "..." + null */

/* Initial collection capacity */
#define INITIAL_COLLECTION_CAPACITY 16   /* Initial array capacity for collections */

/* Date/time format widths */
#define YEAR_WIDTH 4                     /* Width for year in date formats */
#define MONTH_WIDTH 2                    /* Width for month in date formats */
#define DAY_WIDTH 2                      /* Width for day in date formats */
#define WEEK_WIDTH 2                     /* Width for week number in week IDs */

/* Checksum formatting */
#define CHECKSUM_WIDTH 16                /* Width for checksum hex output */

/* Exit codes */
#define EXIT_CODE_SUCCESS 0
#define EXIT_CODE_FAILURE 1

/* ===== Working Memory MCP Constants ===== */
#define WM_DISPLAY_CONTENT_MAX_LEN 60       /* Max content length in status display */
#define WM_DISPLAY_BUFFER_RESERVE 200       /* Buffer reserve for status display */
#define WM_RECORD_ID_RANDOM_MAX 10000       /* Random component for working memory record IDs */
#define WM_PERCENT_MULTIPLIER 100.0f        /* Multiplier for percentage calculation */

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

/* Periodic maintenance intervals */
#define BREATHING_MAINTENANCE_INTERVAL_SECONDS (6 * 3600)  /* 6 hours */

/* Memory pressure thresholds */
#define BREATHING_MEMORY_PRESSURE_WARN_PERCENT 75    /* Warn at 75% full */
#define BREATHING_MEMORY_PRESSURE_CRITICAL_PERCENT 90 /* Critical at 90% full */
#define BREATHING_TIER1_SOFT_LIMIT 7500               /* Soft limit for tier1 entries */
#define BREATHING_TIER1_HARD_LIMIT 9000               /* Hard limit before forcing consolidation */

/* Turn tracking for end-of-turn reflection */
#define BREATHING_DEFAULT_TURN_CAPACITY 16            /* Initial turn memory tracking capacity */
#define BREATHING_GROWTH_FACTOR 2                     /* Array growth factor */

/* ===== Archive Scoring Constants ===== */

/* Archive scoring for memory preservation/archival decisions (tier1_archive.c) */
#define ARCHIVE_SCORE_RECENT_ACCESS_MAX 30     /* Maximum score for recent access */
#define ARCHIVE_SCORE_EMOTIONAL_MAX 25         /* Maximum score for emotional salience */
#define ARCHIVE_SCORE_CENTRALITY_MAX 20        /* Maximum score for graph centrality */
#define ARCHIVE_SCORE_PATTERN_OUTLIER 15       /* Score bonus for pattern outliers */
#define ARCHIVE_SCORE_BASE_IMPORTANCE_MAX 10   /* Maximum base importance score */
#define ARCHIVE_AGE_THRESHOLD_DAYS 14          /* Age threshold before applying penalty */
#define ARCHIVE_AGE_PENALTY_PER_DAY 1          /* Score penalty per day after threshold */

/* ===== Date Formatting Constants ===== */

/* Date format field widths for snprintf/strftime */
#define DATE_FORMAT_YEAR_WIDTH 4       /* YYYY width */
#define DATE_FORMAT_MONTH_WIDTH 2      /* MM width */
#define DATE_FORMAT_DAY_WIDTH 2        /* DD width */
#define DATE_FORMAT_WEEK_WIDTH 2       /* WW width for week numbers */

/* ===== Buffer Safety Margins ===== */

/* Safety margins for response buffer overflow prevention */
#define JSON_ARRAY_CLOSE_RESERVE 10               /* Reserve for closing JSON array brackets */
#define JSON_ARRAY_ELEMENT_RESERVE 50             /* Reserve for JSON array element completion */
#define RESPONSE_BUFFER_SAFETY_MARGIN_SMALL 100   /* Reserve 100 bytes for completion */
#define RESPONSE_BUFFER_SAFETY_MARGIN_LARGE 200   /* Reserve 200 bytes for longer completions */
#define RESPONSE_BUFFER_RESERVE 500               /* Reserve 500 bytes for MCP tool responses */

/* ===== Vector Store Configuration ===== */

/* Vector store query and processing limits */
#define VECTOR_REGENERATION_QUERY_LIMIT 50000     /* Maximum memories to query for vector regeneration */
#define PROGRESS_LOG_INTERVAL 1000                /* Log progress every N operations */
#define MIN_VECTOR_COUNT_THRESHOLD 10             /* Minimum vector count before regeneration offered */
#define VECTOR_DEBUG_INDEX_SMALL 16               /* Small debug array index for vector logging */
#define VECTOR_DEBUG_INDEX_LARGE 100              /* Large debug array index for vector logging */

/* ===== MCP Tool Configuration ===== */

/* Topic and memory display limits */
#define MAX_TOPICS_TO_DISPLAY 10                  /* Maximum number of topics to show in responses */

/* Identity summary truncation */
#define IDENTITY_SUMMARY_TRUNCATE_LENGTH 80       /* Maximum characters before truncation marker */
#define IDENTITY_SUMMARY_BUFFER_SIZE 84           /* Buffer size for truncated summary (80 + "..." + null) */

/* ===== Session State Capture ===== */

/* Session state capture limits */
#define MAX_ACTIVE_THREADS 5                      /* Maximum active thoughts/threads to track */
#define MAX_NEXT_INTENTIONS 5                     /* Maximum next intentions to track */
#define MAX_OPEN_QUESTIONS 5                      /* Maximum open questions to track */
#define MAX_SESSION_INSIGHTS 10                   /* Maximum insights to track per session */

/* Session state text buffer sizes */
#define SESSION_STATE_SHORT_TEXT 128              /* Short descriptive text (cognitive mode, emotional state) */
#define SESSION_STATE_ITEM_TEXT 256               /* Individual item text (thread, intention, question) */
#define SESSION_STATE_INSIGHT_TEXT 512            /* Insight content text */

/* ===== HTTP Status Codes ===== */

/* Standard HTTP status codes for external API calls */
#define HTTP_STATUS_OK 200                        /* Successful HTTP request */

/* ===== Utility Macros - Week 2/3 Improvements ===== */

/*
 * Note: KATRA_CHECK_NULL, KATRA_CHECK_RANGE, and other error checking macros
 * are defined in katra_error.h
 */

/*
 * KATRA_INIT_STRUCT - Zero-initialize a structure
 *
 * Usage:
 *   memory_record_t record;
 *   KATRA_INIT_STRUCT(record);
 *
 * Expands to:
 *   memset(&record, 0, sizeof(record));
 */
#define KATRA_INIT_STRUCT(var) memset(&(var), 0, sizeof(var))

/*
 * CLEANUP_GOTO - Jump to cleanup label with result code
 *
 * Usage:
 *   if (error) CLEANUP_GOTO(cleanup, E_SYSTEM_FILE);
 *
 * Expands to:
 *   do { result = E_SYSTEM_FILE; goto cleanup; } while (0)
 */
#define CLEANUP_GOTO(label, code) \
    do { result = (code); goto label; } while (0)

/*
 * CLEANUP_RETURN - Return from cleanup block with result
 *
 * Usage:
 *   cleanup:
 *       free(buffer);
 *       CLEANUP_RETURN(result);
 *
 * Expands to:
 *   return result;
 */
#define CLEANUP_RETURN(code) return (code)

#endif /* KATRA_LIMITS_H */
