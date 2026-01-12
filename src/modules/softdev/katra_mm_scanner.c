/* © 2025 Casey Koons All rights reserved */

/**
 * @file katra_mm_scanner.c
 * @brief C language parser for metamemory
 *
 * Provides:
 *   - Directory walking and file discovery
 *   - C file parsing (functions, structs, enums, macros)
 *   - Call graph extraction
 *   - File hashing for change detection
 *
 * This is a simple regex-based parser, not a full C parser.
 * It handles common C patterns but may miss edge cases.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include "katra_mm_scanner.h"
#include "katra_mm_index.h"
#include "katra_metamemory.h"
#include "katra_error.h"
#include "katra_limits.h"
#include "katra_log.h"

/* ============================================================================
 * Constants
 * ============================================================================ */

#define SCANNER_LINE_MAX 4096
#define SCANNER_SIGNATURE_MAX 1024
#define SCANNER_NAME_MAX 256
#define SCANNER_HASH_SIZE 33  /* MD5 hex + null */

/* Default exclusions */
static const char* DEFAULT_EXCLUDE_DIRS[] = {
    ".git", ".svn", "node_modules", "build", "bin", "__pycache__",
    ".idea", ".vscode", "vendor", "deps", NULL
};

static const char* DEFAULT_EXCLUDE_PATTERNS[] = {
    "*.min.js", "*.min.css", "*.o", "*.a", "*.so", "*.dylib",
    "*.pyc", "*.pyo", NULL
};

/* ============================================================================
 * Internal Types
 * ============================================================================ */

typedef struct {
    char* content;
    size_t size;
    char** lines;
    size_t line_count;
} file_buffer_t;

typedef struct {
    const char* project_id;
    const char* root_path;
    size_t root_path_len;
    const char** exclude_dirs;
    size_t exclude_dir_count;
    mm_scanner_result_t* result;
} scan_context_t;

/* ============================================================================
 * Internal Helpers - String Utilities
 * ============================================================================ */

/**
 * Trim leading and trailing whitespace.
 */
static char* trim(char* str)
{
    if (!str) return NULL;

    /* Leading */
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str;

    /* Trailing */
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}

/**
 * Check if string starts with prefix.
 */
static bool starts_with(const char* str, const char* prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/**
 * Check if string ends with suffix.
 */
static bool ends_with(const char* str, const char* suffix)
{
    size_t str_len = strlen(str);
    size_t suf_len = strlen(suffix);
    if (suf_len > str_len) return false;
    return strcmp(str + str_len - suf_len, suffix) == 0;
}

/**
 * Extract identifier from position.
 */
static char* extract_identifier(const char* start, char* buffer, size_t size)
{
    size_t i = 0;
    while (i < size - 1 && (isalnum((unsigned char)start[i]) || start[i] == '_')) {
        buffer[i] = start[i];
        i++;
    }
    buffer[i] = '\0';
    return buffer;
}

/* ============================================================================
 * Internal Helpers - File Operations
 * ============================================================================ */

/**
 * Simple file hash (sum of bytes mod 2^32, formatted as hex).
 */
static int compute_file_hash(const char* path, char* hash, size_t hash_size)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        return E_SYSTEM_FILE;
    }

    unsigned long sum = 0;
    int c;
    while ((c = fgetc(fp)) != EOF) {
        sum = (sum * 31 + c) & 0xFFFFFFFF;
    }

    fclose(fp);
    snprintf(hash, hash_size, "%08lx", sum);
    return KATRA_SUCCESS;
}

/**
 * Read file into buffer.
 */
static int read_file(const char* path, file_buffer_t* buf)
{
    FILE* fp = NULL;

    memset(buf, 0, sizeof(*buf));

    fp = fopen(path, "r");
    if (!fp) {
        return E_SYSTEM_FILE;
    }

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size <= 0 || size > 10 * 1024 * 1024) {  /* 10MB limit */
        fclose(fp);
        return (size <= 0) ? KATRA_SUCCESS : E_INPUT_TOO_LARGE;
    }

    buf->content = malloc(size + 1);
    if (!buf->content) {
        fclose(fp);
        return E_SYSTEM_MEMORY;
    }

    size_t read = fread(buf->content, 1, size, fp);
    fclose(fp);

    buf->content[read] = '\0';
    buf->size = read;

    /* Count lines */
    size_t line_count = 1;
    for (size_t i = 0; i < read; i++) {
        if (buf->content[i] == '\n') line_count++;
    }

    buf->lines = calloc(line_count + 1, sizeof(char*));
    if (!buf->lines) {
        free(buf->content);
        buf->content = NULL;
        return E_SYSTEM_MEMORY;
    }

    /* Split into lines */
    buf->lines[0] = buf->content;
    buf->line_count = 1;
    for (size_t i = 0; i < read; i++) {
        if (buf->content[i] == '\n') {
            buf->content[i] = '\0';
            if (i + 1 < read) {
                buf->lines[buf->line_count++] = &buf->content[i + 1];
            }
        }
    }

    return KATRA_SUCCESS;
}

/**
 * Free file buffer.
 */
static void free_file_buffer(file_buffer_t* buf)
{
    if (buf) {
        free(buf->content);
        free(buf->lines);
        memset(buf, 0, sizeof(*buf));
    }
}

/* ============================================================================
 * Internal Helpers - Exclusion Checking
 * ============================================================================ */

/**
 * Check if directory should be excluded.
 */
static bool should_exclude_dir(const char* name, const char** exclude_dirs, size_t count)
{
    /* Check default exclusions */
    for (int i = 0; DEFAULT_EXCLUDE_DIRS[i]; i++) {
        if (strcmp(name, DEFAULT_EXCLUDE_DIRS[i]) == 0) {
            return true;
        }
    }

    /* Check custom exclusions */
    if (exclude_dirs) {
        for (size_t i = 0; i < count; i++) {
            if (strcmp(name, exclude_dirs[i]) == 0) {
                return true;
            }
        }
    }

    return false;
}

/**
 * Check if file matches exclusion pattern.
 */
static bool should_exclude_file(const char* name)
{
    for (int i = 0; DEFAULT_EXCLUDE_PATTERNS[i]; i++) {
        const char* pattern = DEFAULT_EXCLUDE_PATTERNS[i];
        if (pattern[0] == '*') {
            /* Simple suffix match */
            if (ends_with(name, pattern + 1)) {
                return true;
            }
        }
    }
    return false;
}

/* ============================================================================
 * C Parser - Function Detection
 * ============================================================================ */

/**
 * Check if line starts a function definition.
 *
 * Looks for patterns like:
 *   int foo(...)
 *   static void bar(...)
 *   struct_t* baz(...)
 *
 * Returns: pointer to function name, or NULL
 */
static const char* is_function_definition(const char* line, bool* is_static)
{
    *is_static = false;

    /* Skip whitespace */
    while (isspace((unsigned char)*line)) line++;

    /* Check for static */
    if (starts_with(line, "static ")) {
        *is_static = true;
        line += 7;
        while (isspace((unsigned char)*line)) line++;
    }

    /* Skip return type */
    /* Must be: identifier, optionally followed by * or spaces */
    if (!isalpha((unsigned char)*line) && *line != '_') {
        return NULL;
    }

    const char* type_start = line;

    /* Skip type (may include struct, const, unsigned, etc.) */
    while (*line && (isalnum((unsigned char)*line) || *line == '_' ||
           *line == '*' || *line == ' ')) {
        line++;
    }

    /* Should now be at '(' */
    if (*line != '(') {
        return NULL;
    }

    /* Back up to find function name */
    const char* paren = line;
    line--;
    while (line > type_start && isspace((unsigned char)*line)) line--;
    while (line > type_start && *line == '*') line--;
    while (line > type_start && isspace((unsigned char)*line)) line--;

    /* Now at end of function name */
    const char* name_end = line + 1;

    /* Find start of function name */
    while (line > type_start && (isalnum((unsigned char)*line) || *line == '_')) {
        line--;
    }
    const char* name_start = line + 1;

    /* Validate we have a name */
    if (name_start >= name_end) {
        return NULL;
    }

    /* Check for opening brace on same or next line */
    const char* rest = paren;
    while (*rest && *rest != '{' && *rest != ';') rest++;

    /* If ends with semicolon, it's a declaration not definition */
    if (*rest == ';') {
        return NULL;
    }

    return name_start;
}

/**
 * Extract function signature from lines.
 */
static int extract_function(scan_context_t* ctx, file_buffer_t* buf,
                            const char* rel_path, size_t line_num,
                            const char* name_start)
{
    char name[SCANNER_NAME_MAX];
    char signature[SCANNER_SIGNATURE_MAX];
    bool is_static = false;

    /* Extract name */
    size_t name_len = 0;
    while (name_len < sizeof(name) - 1 &&
           (isalnum((unsigned char)name_start[name_len]) || name_start[name_len] == '_')) {
        name[name_len] = name_start[name_len];
        name_len++;
    }
    name[name_len] = '\0';

    /* Build signature from lines until we hit '{' */
    signature[0] = '\0';
    size_t sig_len = 0;
    size_t end_line = line_num;

    for (size_t i = line_num; i < buf->line_count && sig_len < sizeof(signature) - 1; i++) {
        const char* line = buf->lines[i];

        /* Append line to signature */
        while (*line && sig_len < sizeof(signature) - 1) {
            if (*line == '{') {
                end_line = i;
                goto done_sig;
            }
            signature[sig_len++] = *line;
            line++;
        }
        signature[sig_len++] = ' ';
        end_line = i;
    }

done_sig:
    signature[sig_len] = '\0';

    /* Clean up signature */
    char* clean_sig = trim(signature);

    /* Count lines until closing brace (simple brace counting) */
    int brace_count = 0;
    bool in_string = false;
    bool in_char = false;
    bool escape = false;

    for (size_t i = line_num; i < buf->line_count; i++) {
        const char* line = buf->lines[i];
        for (const char* p = line; *p; p++) {
            if (escape) {
                escape = false;
                continue;
            }
            if (*p == '\\') {
                escape = true;
                continue;
            }
            if (*p == '"' && !in_char) {
                in_string = !in_string;
                continue;
            }
            if (*p == '\'' && !in_string) {
                in_char = !in_char;
                continue;
            }
            if (in_string || in_char) continue;

            if (*p == '{') brace_count++;
            if (*p == '}') {
                brace_count--;
                if (brace_count == 0) {
                    end_line = i;
                    goto done_body;
                }
            }
        }
    }

done_body:

    /* Check for static */
    is_static = (strstr(buf->lines[line_num], "static ") != NULL);

    /* Create function node */
    metamemory_node_t* node = metamemory_create_function(
        ctx->project_id,
        name,
        rel_path,
        (int)line_num + 1,  /* 1-based */
        (int)end_line + 1,
        clean_sig
    );

    if (!node) {
        return E_SYSTEM_MEMORY;
    }

    node->visibility = is_static ? METAMEMORY_VIS_INTERNAL : METAMEMORY_VIS_PUBLIC;

    /* Store in index */
    int result = mm_index_store_node(node);
    metamemory_free_node(node);

    if (result == KATRA_SUCCESS && ctx->result) {
        ctx->result->functions_found++;
    }

    return result;
}

/* ============================================================================
 * C Parser - Struct Detection
 * ============================================================================ */

/**
 * Check if line starts a struct definition.
 */
static const char* is_struct_definition(const char* line)
{
    /* Skip whitespace */
    while (isspace((unsigned char)*line)) line++;

    /* Check for typedef struct */
    if (starts_with(line, "typedef struct")) {
        return line + 14;
    }

    /* Check for struct name { */
    if (starts_with(line, "struct ")) {
        line += 7;
        while (isspace((unsigned char)*line)) line++;

        /* Must have a name */
        if (isalpha((unsigned char)*line) || *line == '_') {
            return line;
        }
    }

    return NULL;
}

/**
 * Extract struct definition.
 */
static int extract_struct(scan_context_t* ctx, file_buffer_t* buf,
                          const char* rel_path, size_t line_num,
                          const char* struct_start)
{
    char name[SCANNER_NAME_MAX];

    /* Skip whitespace */
    while (isspace((unsigned char)*struct_start)) struct_start++;

    /* Extract name (might be after { for typedef) */
    if (*struct_start == '{') {
        /* typedef struct { ... } name; - find name at end */
        int brace_count = 1;
        size_t end_line = line_num;

        for (size_t i = line_num; i < buf->line_count && brace_count > 0; i++) {
            const char* line = buf->lines[i];
            if (i == line_num) line = struct_start + 1;

            for (const char* p = line; *p; p++) {
                if (*p == '{') brace_count++;
                if (*p == '}') {
                    brace_count--;
                    if (brace_count == 0) {
                        /* Find name after } */
                        p++;
                        while (*p && isspace((unsigned char)*p)) p++;
                        extract_identifier(p, name, sizeof(name));
                        end_line = i;
                        goto found_name;
                    }
                }
            }
            end_line = i;
        }
        return KATRA_SUCCESS;  /* No name found, skip */

found_name:
        if (name[0] == '\0') {
            return KATRA_SUCCESS;  /* Anonymous struct */
        }

        metamemory_node_t* node = metamemory_create_struct(
            ctx->project_id, name, rel_path,
            (int)line_num + 1, (int)end_line + 1
        );
        if (node) {
            mm_index_store_node(node);
            metamemory_free_node(node);
            if (ctx->result) ctx->result->structs_found++;
        }
    } else {
        /* struct name { ... } */
        extract_identifier(struct_start, name, sizeof(name));
        if (name[0] == '\0') {
            return KATRA_SUCCESS;
        }

        /* Find end */
        int brace_count = 0;
        size_t end_line = line_num;
        bool found_brace = false;

        for (size_t i = line_num; i < buf->line_count; i++) {
            const char* line = buf->lines[i];
            for (const char* p = line; *p; p++) {
                if (*p == '{') {
                    brace_count++;
                    found_brace = true;
                }
                if (*p == '}') {
                    brace_count--;
                    if (found_brace && brace_count == 0) {
                        end_line = i;
                        goto done_struct;
                    }
                }
            }
        }
done_struct:
        {
            metamemory_node_t* node2 = metamemory_create_struct(
                ctx->project_id, name, rel_path,
                (int)line_num + 1, (int)end_line + 1
            );
            if (node2) {
                mm_index_store_node(node2);
                metamemory_free_node(node2);
                if (ctx->result) ctx->result->structs_found++;
            }
        }
    }

    return KATRA_SUCCESS;
}

/* ============================================================================
 * C Parser - Main Scanner
 * ============================================================================ */

/**
 * Scan a single C file.
 */
static int scan_c_file(scan_context_t* ctx, const char* abs_path, const char* rel_path)
{
    file_buffer_t buf;
    int result = KATRA_SUCCESS;
    char hash[SCANNER_HASH_SIZE];
    char stored_hash[SCANNER_HASH_SIZE];

    /* Compute file hash */
    if (compute_file_hash(abs_path, hash, sizeof(hash)) != KATRA_SUCCESS) {
        return E_SYSTEM_FILE;
    }

    /* Check if file changed */
    if (mm_index_get_file_hash(rel_path, stored_hash, sizeof(stored_hash)) == KATRA_SUCCESS) {
        if (strcmp(hash, stored_hash) == 0) {
            /* File unchanged, skip */
            return KATRA_SUCCESS;
        }
        /* File changed, delete old nodes */
        mm_index_delete_by_file(rel_path);
    }

    /* Read file */
    result = read_file(abs_path, &buf);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    /* Scan lines */
    bool in_block_comment = false;

    for (size_t i = 0; i < buf.line_count; i++) {
        const char* line = buf.lines[i];

        /* Skip empty lines */
        const char* trimmed = line;
        while (isspace((unsigned char)*trimmed)) trimmed++;
        if (*trimmed == '\0') continue;

        /* Track block comments */
        if (strstr(line, "/*")) in_block_comment = true;
        if (strstr(line, "*/")) {
            in_block_comment = false;
            continue;
        }
        if (in_block_comment) continue;

        /* Skip line comments */
        if (starts_with(trimmed, "//")) continue;

        /* Skip preprocessor */
        if (*trimmed == '#') continue;

        /* Check for function definition */
        bool is_static = false;
        const char* func_name = is_function_definition(trimmed, &is_static);
        if (func_name) {
            extract_function(ctx, &buf, rel_path, i, func_name);
            continue;
        }

        /* Check for struct definition */
        const char* struct_start = is_struct_definition(trimmed);
        if (struct_start) {
            extract_struct(ctx, &buf, rel_path, i, struct_start);
            continue;
        }
    }

    /* Store file hash */
    mm_index_store_file_hash(rel_path, hash);

    free_file_buffer(&buf);
    if (ctx->result) ctx->result->files_scanned++;

    return KATRA_SUCCESS;
}

/* ============================================================================
 * Directory Walker
 * ============================================================================ */

/**
 * Recursively scan directory.
 */
static int scan_directory(scan_context_t* ctx, const char* dir_path)
{
    DIR* dir = NULL;
    struct dirent* entry;
    int result = KATRA_SUCCESS;

    dir = opendir(dir_path);
    if (!dir) {
        return E_SYSTEM_FILE;
    }

    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Build full path */
        char full_path[KATRA_PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        /* Get file info */
        struct stat st;
        if (stat(full_path, &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            /* Check exclusions */
            if (should_exclude_dir(entry->d_name, ctx->exclude_dirs, ctx->exclude_dir_count)) {
                continue;
            }

            /* Recurse into directory */
            if (ctx->result) ctx->result->directories_scanned++;
            scan_directory(ctx, full_path);

        } else if (S_ISREG(st.st_mode)) {
            /* Check exclusions */
            if (should_exclude_file(entry->d_name)) {
                continue;
            }

            /* Check if C file */
            if (ends_with(entry->d_name, ".c") || ends_with(entry->d_name, ".h")) {
                /* Compute relative path */
                const char* rel_path = full_path + ctx->root_path_len;
                if (*rel_path == '/') rel_path++;

                scan_c_file(ctx, full_path, rel_path);
            }
        }
    }

    closedir(dir);
    return result;
}

/* ============================================================================
 * Public API
 * ============================================================================ */

int mm_scanner_scan_project(const char* project_id,
                            const char* root_path,
                            const mm_scanner_options_t* options,
                            mm_scanner_result_t* result)
{
    int rc = KATRA_SUCCESS;

    if (!project_id || !root_path) {
        katra_report_error(E_INPUT_NULL, "mm_scanner_scan_project",
                          "project_id or root_path is NULL");
        return E_INPUT_NULL;
    }

    /* Initialize index for this project */
    rc = mm_index_init(project_id);
    if (rc != KATRA_SUCCESS) {
        return rc;
    }

    /* Initialize result */
    if (result) {
        memset(result, 0, sizeof(*result));
    }

    /* Set up context */
    scan_context_t ctx = {
        .project_id = project_id,
        .root_path = root_path,
        .root_path_len = strlen(root_path),
        .exclude_dirs = options ? options->exclude_dirs : NULL,
        .exclude_dir_count = options ? options->exclude_dir_count : 0,
        .result = result
    };

    /* Verify root path exists */
    struct stat st;
    if (stat(root_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        katra_report_error(E_SYSTEM_FILE, "mm_scanner_scan_project",
                          "Root path does not exist or is not a directory");
        return E_SYSTEM_FILE;
    }

    LOG_INFO("Scanning project '%s' at %s", project_id, root_path);

    /* Scan directory tree */
    rc = scan_directory(&ctx, root_path);

    if (rc == KATRA_SUCCESS) {
        LOG_INFO("Scan complete: %zu dirs, %zu files, %zu functions, %zu structs",
                result ? result->directories_scanned : 0,
                result ? result->files_scanned : 0,
                result ? result->functions_found : 0,
                result ? result->structs_found : 0);
    }

    return rc;
}

int mm_scanner_scan_file(const char* project_id,
                         const char* file_path,
                         mm_scanner_result_t* result)
{
    int rc = KATRA_SUCCESS;

    if (!project_id || !file_path) {
        return E_INPUT_NULL;
    }

    /* Initialize index */
    rc = mm_index_init(project_id);
    if (rc != KATRA_SUCCESS) {
        return rc;
    }

    /* Initialize result */
    if (result) {
        memset(result, 0, sizeof(*result));
    }

    /* Set up minimal context */
    scan_context_t ctx = {
        .project_id = project_id,
        .root_path = "",
        .root_path_len = 0,
        .exclude_dirs = NULL,
        .exclude_dir_count = 0,
        .result = result
    };

    /* Check file exists */
    struct stat st;
    if (stat(file_path, &st) != 0 || !S_ISREG(st.st_mode)) {
        return E_SYSTEM_FILE;
    }

    /* Scan the file */
    rc = scan_c_file(&ctx, file_path, file_path);

    return rc;
}

int mm_scanner_check_changes(const char* project_id,
                             const char* root_path,
                             char*** changed_files,
                             size_t* count)
{
    int result = KATRA_SUCCESS;
    char** files = NULL;
    size_t capacity = 32;
    size_t file_count = 0;

    if (!project_id || !root_path || !changed_files || !count) {
        return E_INPUT_NULL;
    }

    *changed_files = NULL;
    *count = 0;

    /* Initialize index */
    result = mm_index_init(project_id);
    if (result != KATRA_SUCCESS) {
        return result;
    }

    files = calloc(capacity, sizeof(char*));
    if (!files) {
        return E_SYSTEM_MEMORY;
    }

    /* Walk directory and compare hashes */
    /* For simplicity, just scan and let scan_c_file detect changes */
    /* A full implementation would iterate file_hashes table */

    /* For now, return empty list - full refresh is handled by scan_project */
    *changed_files = files;
    *count = file_count;

    return KATRA_SUCCESS;
}

void mm_scanner_free_result(mm_scanner_result_t* result)
{
    if (result) {
        memset(result, 0, sizeof(*result));
    }
}
