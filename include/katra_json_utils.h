/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_JSON_UTILS_H
#define KATRA_JSON_UTILS_H

#include <stddef.h>

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

#endif /* KATRA_JSON_UTILS_H */
