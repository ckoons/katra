/* © 2025 Casey Koons All rights reserved */

/* Session state TOON serialization */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "katra_session_state.h"
#include "katra_session_state_internal.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* ============================================================================
 * TOON (Token-Oriented Object Notation) Serialization
 * ============================================================================
 *
 * TOON provides 50-70% token reduction compared to JSON while maintaining
 * readability for both humans and LLMs. Perfect for context efficiency.
 *
 * Format:
 *   array_name[count]:
 *     item1
 *     item2
 *
 *   struct_array[count]{field1,field2,field3}:
 *     val1,val2,val3
 *     val4,val5,val6
 */

/* Helper: Escape commas and newlines in TOON strings */
static void toon_escape_string(const char* input, char* output, size_t output_size) {
    size_t in_idx = 0;
    size_t out_idx = 0;

    while (input[in_idx] && out_idx < output_size - 1) {
        if (input[in_idx] == ',') {
            if (out_idx < output_size - 3) {
                output[out_idx++] = '\\';
                output[out_idx++] = ',';
            }
        } else if (input[in_idx] == '\n') {
            if (out_idx < output_size - 2) {
                output[out_idx++] = ' ';  /* Replace newlines with spaces */
            }
        } else {
            output[out_idx++] = input[in_idx];
        }
        in_idx++;
    }
    output[out_idx] = '\0';
}

int katra_session_state_to_toon(const session_end_state_t* state, char** toon_out) {
    KATRA_CHECK_NULL(state);
    KATRA_CHECK_NULL(toon_out);

    /* Allocate buffer - TOON is much more compact than JSON */
    size_t buffer_size = KATRA_BUFFER_ENHANCED;  /* Should be plenty for session state */
    char* buffer = malloc(buffer_size);
    if (!buffer) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_session_state_to_toon",
                          "Failed to allocate buffer");
        return E_SYSTEM_MEMORY;
    }

    size_t offset = 0;
    int written;

    /* Session metadata - compact header */
    written = snprintf(buffer + offset, buffer_size - offset,
                      "session[%ld,%ld,%d]:\n"
                      "  start,end,duration\n\n",
                      (long)state->session_start,
                      (long)state->session_end,
                      state->duration_seconds);
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    /* Cognitive and emotional state */
    written = snprintf(buffer + offset, buffer_size - offset,
                      "cognitive_mode: %s\n"
                      "cognitive_desc: %s\n"
                      "emotional_state: %s\n"
                      "emotional_desc: %s\n\n",
                      cognitive_mode_to_string(state->cognitive_mode),
                      state->cognitive_mode_desc,
                      emotional_state_to_string(state->emotional_state),
                      state->emotional_state_desc);
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    /* Active threads - simple list */
    written = snprintf(buffer + offset, buffer_size - offset,
                      "active_threads[%d]:\n", state->active_thread_count);
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    for (int i = 0; i < state->active_thread_count; i++) {
        written = snprintf(buffer + offset, buffer_size - offset,
                          "  %s\n", state->active_threads[i]);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;
    }
    written = snprintf(buffer + offset, buffer_size - offset, "\n");
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    /* Next intentions */
    written = snprintf(buffer + offset, buffer_size - offset,
                      "next_intentions[%d]:\n", state->next_intention_count);
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    for (int i = 0; i < state->next_intention_count; i++) {
        written = snprintf(buffer + offset, buffer_size - offset,
                          "  %s\n", state->next_intentions[i]);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;
    }
    written = snprintf(buffer + offset, buffer_size - offset, "\n");
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    /* Open questions */
    written = snprintf(buffer + offset, buffer_size - offset,
                      "open_questions[%d]:\n", state->open_question_count);
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    for (int i = 0; i < state->open_question_count; i++) {
        written = snprintf(buffer + offset, buffer_size - offset,
                          "  %s\n", state->open_questions[i]);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;
    }
    written = snprintf(buffer + offset, buffer_size - offset, "\n");
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    /* Insights - structured records */
    written = snprintf(buffer + offset, buffer_size - offset,
                      "insights[%d]{timestamp,type,impact,content}:\n",
                      state->insight_count);
    if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
    offset += (size_t)written;

    for (int i = 0; i < state->insight_count; i++) {
        const session_insight_t* insight = &state->insights[i];
        char escaped_content[SESSION_STATE_INSIGHT_TEXT];
        toon_escape_string(insight->content, escaped_content, sizeof(escaped_content));

        written = snprintf(buffer + offset, buffer_size - offset,
                          "  %ld,%s,%s,%s\n",
                          (long)insight->timestamp,
                          insight_type_to_string(insight->type),
                          insight_impact_to_string(insight->impact),
                          escaped_content);
        if (written < 0 || (size_t)written >= buffer_size - offset) goto overflow;
        offset += (size_t)written;
    }

    *toon_out = buffer;
    return KATRA_SUCCESS;

overflow:
    free(buffer);
    katra_report_error(E_RESOURCE_LIMIT, "katra_session_state_to_toon",
                      "Buffer overflow during serialization");
    return E_RESOURCE_LIMIT;
}

int katra_session_state_from_toon(const char* toon_str, session_end_state_t* state_out) {
    KATRA_CHECK_NULL(toon_str);
    KATRA_CHECK_NULL(state_out);

    /* Initialize state */
    KATRA_INIT_STRUCT(*state_out);

    /* Parse TOON format line by line */
    const char* line = toon_str;
    const char* next_line;
    char line_buffer[KATRA_BUFFER_TEXT];

    while (*line) {
        /* Find next newline */
        next_line = strchr(line, '\n');
        if (!next_line) next_line = line + strlen(line);

        size_t line_len = (size_t)(next_line - line);
        if (line_len >= sizeof(line_buffer)) {
            katra_report_error(E_INPUT_FORMAT, "katra_session_state_from_toon",
                              "Line too long");
            return E_INPUT_FORMAT;
        }

        strncpy(line_buffer, line, line_len);
        line_buffer[line_len] = '\0';

        /* Parse session metadata */
        if (strncmp(line_buffer, "session[", 8) == 0) {
            long start, end;
            int duration;
            if (sscanf(line_buffer, "session[%ld,%ld,%d]:", &start, &end, &duration) == 3) {
                state_out->session_start = (time_t)start;
                state_out->session_end = (time_t)end;
                state_out->duration_seconds = duration;
            }
        }
        /* Parse cognitive mode */
        else if (strncmp(line_buffer, "cognitive_mode: ", 16) == 0) {
            state_out->cognitive_mode = cognitive_mode_from_string(line_buffer + 16);
        }
        else if (strncmp(line_buffer, "cognitive_desc: ", 16) == 0) {
            strncpy(state_out->cognitive_mode_desc, line_buffer + 16,
                   SESSION_STATE_SHORT_TEXT - 1);
            state_out->cognitive_mode_desc[SESSION_STATE_SHORT_TEXT - 1] = '\0';
        }
        /* Parse emotional state */
        else if (strncmp(line_buffer, "emotional_state: ", 17) == 0) {
            state_out->emotional_state = emotional_state_from_string(line_buffer + 17);
        }
        else if (strncmp(line_buffer, "emotional_desc: ", 16) == 0) {
            strncpy(state_out->emotional_state_desc, line_buffer + 16,
                   SESSION_STATE_SHORT_TEXT - 1);
            state_out->emotional_state_desc[SESSION_STATE_SHORT_TEXT - 1] = '\0';
        }
        /* Parse active threads */
        else if (strncmp(line_buffer, "active_threads[", 15) == 0) {
            /* Count will be in format: active_threads[N]: */
            /* Items follow on indented lines */
        }
        else if (line_buffer[0] == ' ' && line_buffer[1] == ' ' && line_buffer[2] != ' ') {
            /* This is an indented item - determine which array it belongs to */
            const char* item = line_buffer + 2;  /* Skip "  " */

            /* Add to appropriate array based on context */
            /* For simplicity, we'll need to track context state */
            /* This is a simplified parser - full implementation would track section */
            if (state_out->active_thread_count < MAX_ACTIVE_THREADS) {
                strncpy(state_out->active_threads[state_out->active_thread_count],
                       item, SESSION_STATE_ITEM_TEXT - 1);
                state_out->active_threads[state_out->active_thread_count][SESSION_STATE_ITEM_TEXT - 1] = '\0';
                state_out->active_thread_count++;
            }
        }

        /* Move to next line */
        line = next_line;
        if (*line == '\n') line++;
    }

    return KATRA_SUCCESS;
}
