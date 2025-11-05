/* © 2025 Casey Koons All rights reserved */
/* String literal constants for internationalization and consistency */

#ifndef KATRA_STRING_LITERALS_H
#define KATRA_STRING_LITERALS_H

/* ========================================================================
 * COMMON STRING LITERALS
 * ======================================================================== */

/* Empty and whitespace */
#define STR_EMPTY           ""
#define STR_SPACE           " "
#define STR_NEWLINE         "\n"
#define STR_TAB             "\t"
#define STR_CRLF            "\r\n"

/* Common punctuation */
#define STR_SLASH           "/"
#define STR_BACKSLASH       "\\"
#define STR_COMMA           ","
#define STR_COLON           ":"
#define STR_SEMICOLON       ";"
#define STR_PERIOD          "."
#define STR_QUOTE           "\""
#define STR_APOSTROPHE      "'"
#define STR_UNDERSCORE      "_"
#define STR_HYPHEN          "-"

/* ========================================================================
 * COMMON FALLBACK VALUES
 * ======================================================================== */

#define STR_UNKNOWN         "unknown"
#define STR_INVALID         "INVALID"
#define STR_NONE            "none"
#define STR_NULL_VALUE      "null"
#define STR_UNSPECIFIED     "unspecified"
#define STR_UNCATEGORIZED   "Uncategorized"
#define STR_DEFAULT         "default"
#define STR_UNDEFINED       "undefined"

/* ========================================================================
 * STRING PARSING DELIMITERS
 * ======================================================================== */

/* Word boundaries for text parsing */
#define DELIM_WORD_BOUNDARIES   " .,!?;:\n\t"

/* Path separators (Unix and Windows) */
#define DELIM_PATH_SEPARATORS   "/\\"

/* Whitespace characters */
#define DELIM_WHITESPACE        " \t\n\r"

/* CSV-style delimiters */
#define DELIM_CSV               ",;\t"

/* ========================================================================
 * JSON LITERALS
 * ======================================================================== */

#define JSON_TRUE           "true"
#define JSON_FALSE          "false"
#define JSON_NULL           "null"

/* ========================================================================
 * FILE EXTENSIONS
 * ======================================================================== */

#define EXT_JSON            ".json"
#define EXT_JSONL           ".jsonl"
#define EXT_TXT             ".txt"
#define EXT_LOG             ".log"
#define EXT_TMP             ".tmp"

/* ========================================================================
 * FILE MODES (for fopen)
 * ======================================================================== */

#define FILE_MODE_READ      "r"    /* Read only */
#define FILE_MODE_WRITE     "w"    /* Write (truncate if exists) */
#define FILE_MODE_APPEND    "a"    /* Append */
#define FILE_MODE_READ_PLUS "r+"   /* Read/write (must exist) */
#define FILE_MODE_WRITE_PLUS "w+"  /* Read/write (truncate) */
#define FILE_MODE_APPEND_PLUS "a+" /* Read/append */

/* ========================================================================
 * PATH FORMAT STRINGS
 * ======================================================================== */

#define PATH_FMT_SIMPLE     "%s/%s"              /* dir/file */
#define PATH_FMT_DATED      "%s/%04d-%02d-%02d"  /* dir/YYYY-MM-DD */

#endif /* KATRA_STRING_LITERALS_H */
