/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_IDENTITY_H
#define KATRA_IDENTITY_H

#include <time.h>
#include <stddef.h>

/* Persona information structure */
typedef struct {
    char name[256];
    char ci_id[256];
    time_t created;
    time_t last_session;
    int sessions;
    char description[512];
} persona_info_t;

/* Initialize identity system (ensures personas.json exists) */
int katra_identity_init(void);

/* Generate unique CI identity string */
int katra_generate_ci_id(char* buffer, size_t size);

/* Register a new persona or update existing */
int katra_register_persona(const char* name, const char* ci_id);

/* Look up ci_id by persona name */
int katra_lookup_persona(const char* name, char* ci_id_out, size_t ci_id_size);

/* Get persona name by ci_id (reverse lookup) */
int katra_get_persona_name(const char* ci_id, char* name_out, size_t name_size);

/* Update session count and last_session timestamp for persona */
int katra_update_persona_session(const char* name);

/* Get last active persona */
int katra_get_last_active(char* name_out, size_t name_size,
                          char* ci_id_out, size_t ci_id_size);

/* Set last active persona */
int katra_set_last_active(const char* name);

/* List all personas */
int katra_list_personas(persona_info_t*** personas_out, size_t* count_out);

/* Remove a persona from registry (does not delete memories) */
int katra_forget_persona(const char* name);

/* Get detailed info about a persona */
int katra_get_persona_info(const char* name, persona_info_t* info_out);

/* Free persona list */
void katra_free_persona_list(persona_info_t** personas, size_t count);

#endif /* KATRA_IDENTITY_H */
