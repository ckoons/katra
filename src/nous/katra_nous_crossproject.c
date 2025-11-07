/* © 2025 Casey Koons All rights reserved */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "katra_nous.h"
#include "katra_error.h"
#include "katra_log.h"

/* Maximum practices and anti-patterns */
#define MAX_PRACTICES NOUS_MAX_PRACTICES
#define MAX_ANTIPATTERNS NOUS_MAX_ANTIPATTERNS

/* Nous Cross-Project state */
static struct {
    best_practice_t** practices;
    size_t practice_count;
    size_t practice_capacity;

    antipattern_t** antipatterns;
    size_t antipattern_count;
    size_t antipattern_capacity;

    size_t next_practice_id;
    size_t next_antipattern_id;
} g_crossproject_state = {0};

/* Initialize Nous Cross-Project */
int katra_nous_crossproject_init(void) {
    if (g_crossproject_state.practices) {
        return KATRA_SUCCESS;  /* Already initialized */
    }

    g_crossproject_state.practice_capacity = MAX_PRACTICES;
    g_crossproject_state.practices = calloc(g_crossproject_state.practice_capacity,
                                           sizeof(best_practice_t*));
    if (!g_crossproject_state.practices) {
        return E_SYSTEM_MEMORY;
    }

    g_crossproject_state.antipattern_capacity = MAX_ANTIPATTERNS;
    g_crossproject_state.antipatterns = calloc(g_crossproject_state.antipattern_capacity,
                                              sizeof(antipattern_t*));
    if (!g_crossproject_state.antipatterns) {
        free(g_crossproject_state.practices);
        g_crossproject_state.practices = NULL;
        return E_SYSTEM_MEMORY;
    }

    g_crossproject_state.practice_count = 0;
    g_crossproject_state.antipattern_count = 0;
    g_crossproject_state.next_practice_id = 1;
    g_crossproject_state.next_antipattern_id = 1;

    /* Add some default best practices */
    katra_nous_crossproject_add_practice(
/* GUIDELINE_APPROVED: knowledge base content (best practices and antipatterns) */
        "Error Handling with goto cleanup",
        "Use goto cleanup pattern for consistent resource cleanup",
        "Prevents resource leaks and simplifies error paths",
        "error_handling"
    );

    katra_nous_crossproject_add_practice(
        "Null Parameter Checks",
        "Check all pointer parameters at function entry",
        "Prevents crashes and undefined behavior",
        "safety"
    );

    /* Add some common anti-patterns */
    katra_nous_crossproject_add_antipattern(
        "God Object",
        "Single class/module that does too many things",
        "Hard to maintain, test, and understand",
        "Split into focused, single-responsibility modules"
    );

    LOG_INFO("Nous Cross-Project cross-project learning initialized");
    return KATRA_SUCCESS;
}

/* Cleanup Nous Cross-Project */
void katra_nous_crossproject_cleanup(void) {
    if (!g_crossproject_state.practices) {
        return;
    }

    /* Free practices */
    for (size_t i = 0; i < g_crossproject_state.practice_count; i++) {
        katra_nous_crossproject_free_practice(g_crossproject_state.practices[i]);
    }
    free(g_crossproject_state.practices);

    /* Free anti-patterns */
    for (size_t i = 0; i < g_crossproject_state.antipattern_count; i++) {
        katra_nous_crossproject_free_antipattern(g_crossproject_state.antipatterns[i]);
    }
    free(g_crossproject_state.antipatterns);

    memset(&g_crossproject_state, 0, sizeof(g_crossproject_state));

    LOG_INFO("Nous Cross-Project cross-project learning cleaned up");
}

/* Add best practice */
int katra_nous_crossproject_add_practice(
    const char* name,
    const char* description,
    const char* rationale,
    const char* category
) {
    if (!name || !description || !category) {
        return E_INPUT_NULL;
    }

    if (g_crossproject_state.practice_count >= g_crossproject_state.practice_capacity) {
        return E_SYSTEM_MEMORY;
    }

    best_practice_t* practice = calloc(1, sizeof(best_practice_t));
    if (!practice) {
        return E_SYSTEM_MEMORY;
    }

    /* Generate practice ID using common utility */
    char* id = nous_generate_id("practice", &g_crossproject_state.next_practice_id);
    if (!id) {
        free(practice);
        return E_SYSTEM_MEMORY;
    }
    strncpy(practice->practice_id, id, sizeof(practice->practice_id) - 1);
    practice->practice_id[sizeof(practice->practice_id) - 1] = '\0';
    free(id);

    int result = nous_safe_strdup(&practice->name, name);
    if (result != KATRA_SUCCESS) {
        katra_nous_crossproject_free_practice(practice);
        return result;
    }

    result = nous_safe_strdup(&practice->description, description);
    if (result != KATRA_SUCCESS) {
        katra_nous_crossproject_free_practice(practice);
        return result;
    }

    result = nous_safe_strdup(&practice->rationale, rationale);
    if (result != KATRA_SUCCESS) {
        katra_nous_crossproject_free_practice(practice);
        return result;
    }

    result = nous_safe_strdup(&practice->category, category);
    if (result != KATRA_SUCCESS) {
        katra_nous_crossproject_free_practice(practice);
        return result;
    }

    /* Default metrics */
    practice->adoption_rate = 0.7f;  /* Assume moderately adopted */
    practice->effectiveness = 0.8f;  /* Assume effective */
    practice->recommended = true;

    g_crossproject_state.practices[g_crossproject_state.practice_count++] = practice;

    LOG_DEBUG("Added best practice: %s (category: %s)", name, category);
    return KATRA_SUCCESS;
}

/* Add anti-pattern */
int katra_nous_crossproject_add_antipattern(
    const char* name,
    const char* description,
    const char* why_bad,
    const char* alternative
) {
    if (!name || !description || !why_bad) {
        return E_INPUT_NULL;
    }

    if (g_crossproject_state.antipattern_count >= g_crossproject_state.antipattern_capacity) {
        return E_SYSTEM_MEMORY;
    }

    antipattern_t* antipattern = calloc(1, sizeof(antipattern_t));
    if (!antipattern) {
        return E_SYSTEM_MEMORY;
    }

    /* Generate antipattern ID using common utility */
    char* id = nous_generate_id("antipattern", &g_crossproject_state.next_antipattern_id);
    if (!id) {
        free(antipattern);
        return E_SYSTEM_MEMORY;
    }
    strncpy(antipattern->antipattern_id, id, sizeof(antipattern->antipattern_id) - 1);
    antipattern->antipattern_id[sizeof(antipattern->antipattern_id) - 1] = '\0';
    free(id);

    int result = nous_safe_strdup(&antipattern->name, name);
    if (result != KATRA_SUCCESS) {
        katra_nous_crossproject_free_antipattern(antipattern);
        return result;
    }

    result = nous_safe_strdup(&antipattern->description, description);
    if (result != KATRA_SUCCESS) {
        katra_nous_crossproject_free_antipattern(antipattern);
        return result;
    }

    result = nous_safe_strdup(&antipattern->why_bad, why_bad);
    if (result != KATRA_SUCCESS) {
        katra_nous_crossproject_free_antipattern(antipattern);
        return result;
    }

    result = nous_safe_strdup(&antipattern->better_alternative, alternative);
    if (result != KATRA_SUCCESS) {
        katra_nous_crossproject_free_antipattern(antipattern);
        return result;
    }

    g_crossproject_state.antipatterns[g_crossproject_state.antipattern_count++] = antipattern;

    LOG_DEBUG("Added anti-pattern: %s", name);
    return KATRA_SUCCESS;
}

/* Get best practices */
best_practice_t** katra_nous_crossproject_get_practices(
    const char* category,
    size_t* count
) {
    if (!count) {
        return NULL;
    }

    *count = 0;

    /* Count matching practices */
    size_t match_count = 0;
    for (size_t i = 0; i < g_crossproject_state.practice_count; i++) {
        best_practice_t* p = g_crossproject_state.practices[i];

        if (category) {
            if (strcmp(p->category, category) != 0) {
                continue;
            }
        }

        match_count++;
    }

    if (match_count == 0) {
        return NULL;
    }

    /* Allocate result array */
    best_practice_t** results = calloc(match_count, sizeof(best_practice_t*));
    if (!results) {
        return NULL;
    }

    /* Populate results */
    size_t idx = 0;
    for (size_t i = 0; i < g_crossproject_state.practice_count; i++) {
        best_practice_t* p = g_crossproject_state.practices[i];

        if (category) {
            if (strcmp(p->category, category) != 0) {
                continue;
            }
        }

        results[idx++] = p;
    }

    *count = match_count;
    return results;
}

/* Get anti-patterns */
antipattern_t** katra_nous_crossproject_get_antipatterns(size_t* count) {
    if (!count) {
        return NULL;
    }

    if (g_crossproject_state.antipattern_count == 0) {
        *count = 0;
        return NULL;
    }

    antipattern_t** results = calloc(g_crossproject_state.antipattern_count,
                                    sizeof(antipattern_t*));
    if (!results) {
        return NULL;
    }

    for (size_t i = 0; i < g_crossproject_state.antipattern_count; i++) {
        results[i] = g_crossproject_state.antipatterns[i];
    }

    *count = g_crossproject_state.antipattern_count;
    return results;
}

/* Import project knowledge (simplified for Nous Cross-Project) */
int katra_nous_crossproject_import_project(
    const char* project_name,
    const char* domain,
    float quality_score
) {
    if (!project_name || !domain) {
        return E_INPUT_NULL;
    }

    /* In a full implementation, this would import patterns and practices
     * from the project. For Nous Cross-Project, we just log the import.
     */

    LOG_INFO("Imported knowledge from project '%s' (domain: %s, quality: %.2f)",
             project_name, domain, quality_score);

    /* Could add project-specific practices here */
    char practice_name[NOUS_SMALL_BUFFER];
    snprintf(practice_name, sizeof(practice_name),
            "Practice from %s", project_name);

    return katra_nous_crossproject_add_practice(
        practice_name,
        "Project-specific best practice",
        "Learned from successful project",
        domain
    );
}

/* Free best practice */
void katra_nous_crossproject_free_practice(best_practice_t* practice) {
    if (!practice) {
        return;
    }

    free(practice->name);
    free(practice->description);
    free(practice->rationale);
    free(practice->category);

    for (size_t i = 0; i < practice->example_count; i++) {
        free(practice->example_projects[i]);
    }
    free(practice->example_projects);

    free(practice);
}

/* Free practices array */
void katra_nous_crossproject_free_practices(best_practice_t** practices, size_t count) {
    (void)count;  /* Unused - practices owned by state */

    if (!practices) {
        return;
    }

    free(practices);
}

/* Free anti-pattern */
void katra_nous_crossproject_free_antipattern(antipattern_t* antipattern) {
    if (!antipattern) {
        return;
    }

    free(antipattern->name);
    free(antipattern->description);
    free(antipattern->why_bad);
    free(antipattern->better_alternative);

    for (size_t i = 0; i < antipattern->consequence_count; i++) {
        free(antipattern->common_consequences[i]);
    }
    free(antipattern->common_consequences);

    free(antipattern);
}

/* Free anti-patterns array */
void katra_nous_crossproject_free_antipatterns(antipattern_t** antipatterns, size_t count) {
    (void)count;  /* Unused - antipatterns owned by state */

    if (!antipatterns) {
        return;
    }

    free(antipatterns);
}
