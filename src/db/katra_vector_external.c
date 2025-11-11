/* © 2025 Casey Koons All rights reserved */

/* External embeddings API integration (OpenAI, Anthropic) - Phase 6.1c */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <curl/curl.h>

/* Project includes */
#include "katra_vector.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"

/* OpenAI API configuration */
#define OPENAI_API_URL "https://api.openai.com/v1/embeddings"
#define OPENAI_MODEL "text-embedding-3-small"
#define MAX_API_RESPONSE_SIZE (1024 * 1024)  /* 1MB */

/* Response buffer for CURL */
typedef struct {
    char* data;
    size_t size;
} response_buffer_t;

/* CURL write callback */
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    response_buffer_t* mem = (response_buffer_t*)userp;

    if (mem->size + realsize > MAX_API_RESPONSE_SIZE) {
        LOG_ERROR("API response too large");
        return 0;
    }

    char* ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        LOG_ERROR("Failed to allocate memory for API response");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

/* Parse embedding vector from JSON response */
static int parse_embedding_from_json(const char* json_response,
                                     float* values, size_t dimensions) {
    if (!json_response || !values) {
        return E_INPUT_NULL;
    }

    /* Simple JSON parsing - look for "embedding": [ ... ] */
    const char* embedding_start = strstr(json_response, "\"embedding\"");
    if (!embedding_start) {
        LOG_ERROR("No embedding found in API response");
        return E_INPUT_INVALID;
    }

    /* Find the array start */
    const char* array_start = strchr(embedding_start, '[');
    if (!array_start) {
        LOG_ERROR("Invalid embedding format in API response");
        return E_INPUT_INVALID;
    }

    /* Parse floating point values */
    const char* ptr = array_start + 1;
    size_t count = 0;

    while (*ptr && count < dimensions) {
        /* Skip whitespace */
        while (*ptr && (*ptr == ' ' || *ptr == '\n' || *ptr == '\r' || *ptr == '\t')) {
            ptr++;
        }

        /* Check for array end */
        if (*ptr == ']') {
            break;
        }

        /* Parse number */
        char* endptr;
        float value = strtof(ptr, &endptr);
        if (ptr == endptr) {
            LOG_ERROR("Failed to parse embedding value at position %zu", count);
            return E_INPUT_INVALID;
        }

        values[count++] = value;
        ptr = endptr;

        /* Skip comma */
        while (*ptr && (*ptr == ',' || *ptr == ' ' || *ptr == '\n' || *ptr == '\r' || *ptr == '\t')) {
            ptr++;
        }
    }

    if (count != dimensions) {
        LOG_WARN("Expected %zu dimensions, got %zu", dimensions, count);
        /* Pad with zeros if needed */
        for (size_t i = count; i < dimensions; i++) {
            values[i] = 0.0f;
        }
    }

    return KATRA_SUCCESS;
}

/* Call OpenAI embeddings API */
static int call_openai_api(const char* text, const char* api_key,
                          float* values, size_t dimensions) {
    if (!text || !api_key || !values) {
        return E_INPUT_NULL;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        katra_report_error(E_SYSTEM_IO, __func__, "Failed to initialize CURL");
        return E_SYSTEM_IO;
    }

    int result = KATRA_SUCCESS;
    response_buffer_t response = {NULL, 0};
    struct curl_slist* headers = NULL;

    /* Build request JSON */
    char* request_json = NULL;
    size_t json_len = strlen(text) * 2 + 512;  /* Estimate with room for escaping */
    request_json = calloc(json_len, 1);
    if (!request_json) {
        curl_easy_cleanup(curl);
        return E_SYSTEM_MEMORY;
    }

    /* Escape quotes in text */
    char* escaped_text = calloc(strlen(text) * 2 + 1, 1);
    if (!escaped_text) {
        free(request_json);
        curl_easy_cleanup(curl);
        return E_SYSTEM_MEMORY;
    }

    const char* src = text;
    char* dst = escaped_text;
    while (*src) {
        if (*src == '"' || *src == '\\') {
            *dst++ = '\\';
        }
        *dst++ = *src++;
    }
    *dst = '\0';

    snprintf(request_json, json_len,
            "{\"input\":\"%s\",\"model\":\"%s\",\"dimensions\":%zu}",
            escaped_text, OPENAI_MODEL, dimensions);

    free(escaped_text);

    /* Set up headers */
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);

    /* Configure CURL */
    curl_easy_setopt(curl, CURLOPT_URL, OPENAI_API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_json);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);  /* 30 second timeout */

    /* Perform request */
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        katra_report_error(E_SYSTEM_IO, __func__, curl_easy_strerror(res));
        result = E_SYSTEM_IO;
        goto cleanup;
    }

    /* Check HTTP status */
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 200) {
        LOG_ERROR("OpenAI API returned HTTP %ld: %s", http_code,
                 response.data ? response.data : "no response");
        result = E_SYSTEM_IO;
        goto cleanup;
    }

    /* Parse embedding from response */
    if (!response.data) {
        LOG_ERROR("No response data from OpenAI API");
        result = E_SYSTEM_IO;
        goto cleanup;
    }

    result = parse_embedding_from_json(response.data, values, dimensions);

cleanup:
    free(request_json);
    free(response.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return result;
}

/* Create embedding using external API */
int katra_vector_external_create(const char* text, const char* api_key,
                                 const char* provider,
                                 vector_embedding_t** embedding_out) {
    if (!text || !embedding_out) {
        return E_INPUT_NULL;
    }

    if (!api_key) {
        LOG_WARN("No API key provided for external embeddings");
        return E_INPUT_NULL;
    }

    /* Create embedding structure */
    vector_embedding_t* embedding = calloc(1, sizeof(vector_embedding_t));
    if (!embedding) {
        return E_SYSTEM_MEMORY;
    }

    embedding->dimensions = VECTOR_DIMENSIONS;
    embedding->values = calloc(VECTOR_DIMENSIONS, sizeof(float));
    if (!embedding->values) {
        free(embedding);
        return E_SYSTEM_MEMORY;
    }

    /* Call appropriate API based on provider */
    int result;
    if (!provider || strcmp(provider, "openai") == 0) {
        result = call_openai_api(text, api_key, embedding->values, VECTOR_DIMENSIONS);
    } else {
        LOG_ERROR("Unsupported embedding provider: %s", provider);
        katra_vector_free_embedding(embedding);
        return E_INPUT_INVALID;
    }

    if (result != KATRA_SUCCESS) {
        katra_vector_free_embedding(embedding);
        return result;
    }

    /* Calculate magnitude */
    embedding->magnitude = 0.0f;
    for (size_t i = 0; i < VECTOR_DIMENSIONS; i++) {
        embedding->magnitude += embedding->values[i] * embedding->values[i];
    }
    embedding->magnitude = sqrtf(embedding->magnitude);

    /* Normalize if needed */
    if (embedding->magnitude > 0.0f) {
        for (size_t i = 0; i < VECTOR_DIMENSIONS; i++) {
            embedding->values[i] /= embedding->magnitude;
        }
        embedding->magnitude = 1.0f;
    }

    *embedding_out = embedding;

    LOG_DEBUG("Created external embedding via %s (magnitude: %.3f)",
             provider ? provider : "openai", embedding->magnitude);

    return KATRA_SUCCESS;
}

/* Check if external embeddings are available */
bool katra_vector_external_available(const char* api_key) {
    return (api_key != NULL && strlen(api_key) > 0);
}

/* Get API key from environment */
const char* katra_vector_external_get_api_key(void) {
    /* Try OpenAI key first */
    const char* key = getenv("OPENAI_API_KEY");
    if (key && strlen(key) > 0) {
        return key;
    }

    /* Try Anthropic key */
    key = getenv("ANTHROPIC_API_KEY");
    if (key && strlen(key) > 0) {
        return key;
    }

    return NULL;
}
