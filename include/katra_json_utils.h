/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_JSON_UTILS_H
#define KATRA_JSON_UTILS_H

#include <stddef.h>
#include <stdbool.h>

/* JSON utility functions for Katra
 *
 * Provides simple JSON handling for MVP. These are interim utilities
 * until a proper JSON library (like cJSON) is integrated.
 *
 * When switching to a full JSON library, only this file needs updating.
 */

/* Escape string for JSON output
 *
 * Escapes special characters: " \ \n \r \t
 *
 * Parameters:
 *   src - Source string to escape
 *   dst - Destination buffer for escaped string
 *   dst_size - Size of destination buffer
 *
 * Note: Always null-terminates dst, even on overflow
 */
void katra_json_escape(const char* src, char* dst, size_t dst_size);

/* Extract string value from JSON
 *
 * Simple parser for key-value pairs in JSON.
 * Looks for pattern: "key": "value"
 *
 * Parameters:
 *   json - JSON string to parse
 *   key - Key name to find
 *   value - Buffer to store extracted value
 *   value_size - Size of value buffer
 *
 * Returns:
 *   KATRA_SUCCESS if found and extracted
 *   E_NOT_FOUND if key not found
 *   E_INPUT_NULL if any parameter is NULL
 */
int katra_json_get_string(const char* json, const char* key,
                          char* value, size_t value_size);

/* Extract long integer value from JSON
 *
 * Simple parser for key-value pairs in JSON.
 * Looks for pattern: "key": 12345
 *
 * Parameters:
 *   json - JSON string to parse
 *   key - Key name to find
 *   value - Pointer to store extracted value
 *
 * Returns:
 *   KATRA_SUCCESS if found and extracted
 *   E_NOT_FOUND if key not found
 *   E_INPUT_NULL if any parameter is NULL
 */
int katra_json_get_long(const char* json, const char* key, long* value);

/* Extract size_t value from JSON
 *
 * Simple parser for key-value pairs in JSON.
 * Looks for pattern: "key": 12345
 *
 * Parameters:
 *   json - JSON string to parse
 *   key - Key name to find
 *   value - Pointer to store extracted value
 *
 * Returns:
 *   KATRA_SUCCESS if found and extracted
 *   E_NOT_FOUND if key not found
 *   E_INPUT_NULL if any parameter is NULL
 */
int katra_json_get_size(const char* json, const char* key, size_t* value);

/* Extract integer value from JSON
 *
 * Simple parser for key-value pairs in JSON.
 * Looks for pattern: "key": 123
 *
 * Parameters:
 *   json - JSON string to parse
 *   key - Key name to find
 *   value - Pointer to store extracted value
 *
 * Returns:
 *   KATRA_SUCCESS if found and extracted
 *   E_NOT_FOUND if key not found
 *   E_INPUT_NULL if any parameter is NULL
 */
int katra_json_get_int(const char* json, const char* key, int* value);

/* Extract float value from JSON
 *
 * Simple parser for key-value pairs in JSON.
 * Looks for pattern: "key": 1.23
 *
 * Parameters:
 *   json - JSON string to parse
 *   key - Key name to find
 *   value - Pointer to store extracted value
 *
 * Returns:
 *   KATRA_SUCCESS if found and extracted
 *   E_NOT_FOUND if key not found
 *   E_INPUT_NULL if any parameter is NULL
 */
int katra_json_get_float(const char* json, const char* key, float* value);

/* Extract boolean value from JSON
 *
 * Simple parser for key-value pairs in JSON.
 * Looks for pattern: "key": true or "key": false
 *
 * Parameters:
 *   json - JSON string to parse
 *   key - Key name to find
 *   value - Pointer to store extracted value
 *
 * Returns:
 *   KATRA_SUCCESS if found and extracted
 *   E_NOT_FOUND if key not found
 *   E_INPUT_NULL if any parameter is NULL
 */
int katra_json_get_bool(const char* json, const char* key, bool* value);

/* ===== Higher-Level Extraction Helpers ===== */

/* Unescape function pointer type for JSON extraction */
typedef void (*json_unescape_fn_t)(const char* src, char* dst, size_t dst_size);

/* Extract optional JSON string field with allocation
 *
 * Extracts string field, allocates memory, and stores pointer.
 * Returns NULL in dest if field not found (not an error).
 *
 * Parameters:
 *   json - JSON string to parse
 *   field - Field name to extract
 *   dest - Pointer to store allocated string
 *   unescape_fn - Optional unescape function (can be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success (even if field not found)
 *   E_SYSTEM_MEMORY if allocation fails
 *   E_INPUT_NULL if json or dest is NULL
 */
int katra_json_extract_string_alloc(const char* json, const char* field,
                                     char** dest, json_unescape_fn_t unescape_fn);

/* Extract required JSON string field with allocation
 *
 * Like extract_string_alloc but returns error if field not found.
 *
 * Parameters:
 *   json - JSON string to parse
 *   field - Field name to extract
 *   dest - Pointer to store allocated string
 *   unescape_fn - Optional unescape function (can be NULL)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_NOT_FOUND if field not present
 *   E_SYSTEM_MEMORY if allocation fails
 *   E_INPUT_NULL if json or dest is NULL
 */
int katra_json_extract_string_required(const char* json, const char* field,
                                        char** dest, json_unescape_fn_t unescape_fn);

#endif /* KATRA_JSON_UTILS_H */
