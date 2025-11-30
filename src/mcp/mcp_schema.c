/* © 2025 Casey Koons All rights reserved */

/* MCP Schema Builders - JSON schema construction for tools and resources */

#include <jansson.h>
#include "katra_mcp.h"

/* Build tool schema with no parameters */
json_t* mcp_build_tool_schema_0params(void) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, json_object());
    return schema;
}

/* Build tool schema with one parameter */
json_t* mcp_build_tool_schema_1param(const char* param_name, const char* param_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));

    json_t* props = json_object();
    json_t* param = json_object();
    json_object_set_new(param, MCP_FIELD_TYPE, json_string(MCP_TYPE_STRING));
    json_object_set_new(param, MCP_FIELD_DESCRIPTION, json_string(param_desc));
    json_object_set_new(props, param_name, param);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(param_name));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);

    return schema;
}

/* Build tool schema with two parameters */
json_t* mcp_build_tool_schema_2params(const char* param1_name, const char* param1_desc,
                                       const char* param2_name, const char* param2_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));

    json_t* props = json_object();

    json_t* param1 = json_object();
    json_object_set_new(param1, MCP_FIELD_TYPE, json_string(MCP_TYPE_STRING));
    json_object_set_new(param1, MCP_FIELD_DESCRIPTION, json_string(param1_desc));
    json_object_set_new(props, param1_name, param1);

    json_t* param2 = json_object();
    json_object_set_new(param2, MCP_FIELD_TYPE, json_string(MCP_TYPE_STRING));
    json_object_set_new(param2, MCP_FIELD_DESCRIPTION, json_string(param2_desc));
    json_object_set_new(props, param2_name, param2);

    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(param1_name));
    json_array_append_new(required, json_string(param2_name));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);

    return schema;
}

/* Helper: Add integer property to schema */
static void add_int_property(json_t* props, const char* name, const char* desc) {
    json_t* prop = json_object();
    json_object_set_new(prop, MCP_FIELD_TYPE, json_string("integer"));
    json_object_set_new(prop, MCP_FIELD_DESCRIPTION, json_string(desc));
    json_object_set_new(props, name, prop);
}

/* Helper: Add boolean property to schema */
static void add_bool_property(json_t* props, const char* name, const char* desc) {
    json_t* prop = json_object();
    json_object_set_new(prop, MCP_FIELD_TYPE, json_string("boolean"));
    json_object_set_new(prop, MCP_FIELD_DESCRIPTION, json_string(desc));
    json_object_set_new(props, name, prop);
}

/* Helper: Add string property to schema */
static void add_string_property(json_t* props, const char* name, const char* desc) {
    json_t* prop = json_object();
    json_object_set_new(prop, MCP_FIELD_TYPE, json_string(MCP_TYPE_STRING));
    json_object_set_new(prop, MCP_FIELD_DESCRIPTION, json_string(desc));
    json_object_set_new(props, name, prop);
}

/* Helper: Add number property to schema */
static void add_number_property(json_t* props, const char* name, const char* desc) {
    json_t* prop = json_object();
    json_object_set_new(prop, MCP_FIELD_TYPE, json_string("number"));
    json_object_set_new(prop, MCP_FIELD_DESCRIPTION, json_string(desc));
    json_object_set_new(props, name, prop);
}

/* Build schema with 1 optional integer parameter */
json_t* mcp_build_schema_optional_int(const char* param_name, const char* param_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_int_property(props, param_name, param_desc);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);
    return schema;
}

/* Build schema with 2 optional integer parameters */
json_t* mcp_build_schema_2optional_ints(const char* p1_name, const char* p1_desc,
                                         const char* p2_name, const char* p2_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_int_property(props, p1_name, p1_desc);
    add_int_property(props, p2_name, p2_desc);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);
    return schema;
}

/* Build schema: 1 required string, 1 optional string */
json_t* mcp_build_schema_1req_1opt_string(const char* req_name, const char* req_desc,
                                           const char* opt_name, const char* opt_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_string_property(props, req_name, req_desc);
    add_string_property(props, opt_name, opt_desc);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(req_name));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);
    return schema;
}

/* Build schema: 1 required string, 3 optional (2 bool, 1 string) */
json_t* mcp_build_metadata_schema(void) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();

    add_string_property(props, MCP_PARAM_MEMORY_ID, MCP_PARAM_DESC_MEMORY_ID);
    add_bool_property(props, MCP_PARAM_PERSONAL, MCP_PARAM_DESC_PERSONAL);
    add_bool_property(props, MCP_PARAM_NOT_TO_ARCHIVE, MCP_PARAM_DESC_NOT_TO_ARCHIVE);
    add_string_property(props, MCP_PARAM_COLLECTION, MCP_PARAM_DESC_COLLECTION);

    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(MCP_PARAM_MEMORY_ID));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);
    return schema;
}

/* Build schema: 1 required bool, 2 optional (1 number, 1 string) */
json_t* mcp_build_semantic_config_schema(void) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();

    add_bool_property(props, MCP_PARAM_ENABLED, MCP_PARAM_DESC_ENABLED);
    add_number_property(props, MCP_PARAM_THRESHOLD, MCP_PARAM_DESC_THRESHOLD);
    add_string_property(props, MCP_PARAM_METHOD, MCP_PARAM_DESC_METHOD);

    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(MCP_PARAM_ENABLED));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);
    return schema;
}

/* Build schema: 1 required string, 1 optional number */
json_t* mcp_build_schema_1req_string_1opt_number(const char* req_name, const char* req_desc,
                                                   const char* opt_name, const char* opt_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_string_property(props, req_name, req_desc);
    add_number_property(props, opt_name, opt_desc);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(req_name));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);
    return schema;
}

/* Build schema: 1 optional number parameter */
json_t* mcp_build_schema_optional_number(const char* param_name, const char* param_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_number_property(props, param_name, param_desc);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);
    return schema;
}

/* Build schema: 1 optional string parameter */
json_t* mcp_build_schema_optional_string(const char* param_name, const char* param_desc) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_string_property(props, param_name, param_desc);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);
    return schema;
}

/* Build complete tool definition */
json_t* mcp_build_tool(const char* name, const char* description, json_t* schema) {
    json_t* tool = json_object();
    json_object_set_new(tool, MCP_FIELD_NAME, json_string(name));
    json_object_set_new(tool, MCP_FIELD_DESCRIPTION, json_string(description));
    json_object_set_new(tool, MCP_FIELD_INPUT_SCHEMA, schema);
    return tool;
}

/* Build complete resource definition */
json_t* mcp_build_resource(const char* uri, const char* name,
                            const char* description, const char* mime_type) {
    json_t* resource = json_object();
    json_object_set_new(resource, MCP_FIELD_URI, json_string(uri));
    json_object_set_new(resource, MCP_FIELD_NAME, json_string(name));
    json_object_set_new(resource, MCP_FIELD_DESCRIPTION, json_string(description));
    json_object_set_new(resource, MCP_FIELD_MIME_TYPE, json_string(mime_type));
    return resource;
}

/* Build schema for katra_archive: 2 required strings (memory_id, reason) */
json_t* mcp_build_archive_schema(void) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_string_property(props, MCP_PARAM_MEMORY_ID, MCP_PARAM_DESC_MEMORY_ID);
    add_string_property(props, MCP_PARAM_REASON, MCP_PARAM_DESC_REASON);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(MCP_PARAM_MEMORY_ID));
    json_array_append_new(required, json_string(MCP_PARAM_REASON));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);
    return schema;
}

/* Build schema for katra_fade: 2 required strings, 1 optional number */
json_t* mcp_build_fade_schema(void) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_string_property(props, MCP_PARAM_MEMORY_ID, MCP_PARAM_DESC_MEMORY_ID);
    add_string_property(props, MCP_PARAM_REASON, MCP_PARAM_DESC_REASON);
    add_number_property(props, MCP_PARAM_TARGET_IMPORTANCE, MCP_PARAM_DESC_TARGET_IMPORTANCE);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(MCP_PARAM_MEMORY_ID));
    json_array_append_new(required, json_string(MCP_PARAM_REASON));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);
    return schema;
}

/* Build schema for katra_forget: 2 required strings, 1 required bool */
json_t* mcp_build_forget_schema(void) {
    json_t* schema = json_object();
    json_object_set_new(schema, MCP_FIELD_TYPE, json_string(MCP_TYPE_OBJECT));
    json_t* props = json_object();
    add_string_property(props, MCP_PARAM_MEMORY_ID, MCP_PARAM_DESC_MEMORY_ID);
    add_string_property(props, MCP_PARAM_REASON, MCP_PARAM_DESC_REASON);
    add_bool_property(props, MCP_PARAM_CI_CONSENT, MCP_PARAM_DESC_CI_CONSENT);
    json_object_set_new(schema, MCP_FIELD_PROPERTIES, props);

    json_t* required = json_array();
    json_array_append_new(required, json_string(MCP_PARAM_MEMORY_ID));
    json_array_append_new(required, json_string(MCP_PARAM_REASON));
    json_array_append_new(required, json_string(MCP_PARAM_CI_CONSENT));
    json_object_set_new(schema, MCP_FIELD_REQUIRED, required);
    return schema;
}
