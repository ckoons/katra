/* © 2025 Casey Koons All rights reserved */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* Project includes */
#include "katra_convergence.h"
#include "katra_memory.h"
#include "katra_graph.h"
#include "katra_vector.h"
#include "katra_tier1_index.h"
#include "katra_error.h"
#include "katra_log.h"
#include "katra_limits.h"
#include "katra_core_common.h"

/* Convergence detection thresholds */
#define CONVERGENCE_DEFAULT_THRESHOLD 0.7f
#define CONVERGENCE_DEFAULT_BOOST 0.2f
#define CONVERGENCE_TIME_WINDOW_HOURS 24
#define CONVERGENCE_MIN_SEMANTIC_SIMILARITY 0.6f
#define CONVERGENCE_MIN_GRAPH_CENTRALITY 0.4f

/* Pattern detection keywords */
/* GUIDELINE_APPROVED: Pattern detection keywords for convergence analysis */
static const char* DECISION_KEYWORDS[] = {
    "decide", "chose", "will use", "going with", "selected", NULL /* GUIDELINE_APPROVED */
};

static const char* QUESTION_KEYWORDS[] = {
    "?", "how", "what", "why", "when", "where", "who", NULL /* GUIDELINE_APPROVED */
};

static const char* KNOWLEDGE_KEYWORDS[] = {
    "learned", "understand", "realize", "discovered", "found out", NULL /* GUIDELINE_APPROVED */
};
/* GUIDELINE_APPROVED_END */

/* Initialize convergence detector */
convergence_detector_t* katra_convergence_init(const char* ci_id) {
    convergence_detector_t* detector = NULL;

    if (!ci_id) {
        katra_report_error(E_INPUT_NULL, "katra_convergence_init", KATRA_ERR_CI_ID_NULL);
        return NULL;
    }

    detector = calloc(1, sizeof(convergence_detector_t));
    if (!detector) {
        katra_report_error(E_SYSTEM_MEMORY, "katra_convergence_init",
                          KATRA_ERR_ALLOC_FAILED);
        return NULL;
    }

    SAFE_STRNCPY(detector->ci_id, ci_id);

    /* Initialize storage backends */
    detector->graph = katra_graph_init(ci_id);
    detector->vectors = katra_vector_init(ci_id, false);

    /* Set defaults */
    detector->convergence_threshold = CONVERGENCE_DEFAULT_THRESHOLD;
    detector->importance_boost = CONVERGENCE_DEFAULT_BOOST;
    detector->time_window_hours = CONVERGENCE_TIME_WINDOW_HOURS;

    /* Initialize stats */
    detector->conscious_memories = 0;
    detector->subconscious_memories = 0;
    detector->convergences_detected = 0;
    detector->memories_strengthened = 0;

    LOG_INFO("Convergence detector initialized for %s", ci_id);
    return detector;
}

/* Helper: Check if text contains keyword */
static bool contains_keyword(const char* text, const char** keywords) {
    if (!text || !keywords) {
        return false;
    }

    char lowercase[1024];
    size_t len = strlen(text);
    if (len >= sizeof(lowercase)) {
        len = sizeof(lowercase) - 1;
    }

    for (size_t i = 0; i < len; i++) {
        lowercase[i] = tolower(text[i]);
    }
    lowercase[len] = '\0';

    for (size_t i = 0; keywords[i] != NULL; i++) {
        if (strstr(lowercase, keywords[i])) {
            return true;
        }
    }

    return false;
}

/* Helper: Calculate automatic importance */
static float calculate_auto_importance(const auto_memory_candidate_t* candidate) {
    float importance = MEMORY_IMPORTANCE_LOW;

    if (candidate->decision_made) {
        importance += 0.3f;
    }
    if (candidate->question_asked) {
        importance += 0.2f;
    }
    if (candidate->knowledge_shared) {
        importance += 0.3f;
    }
    if (candidate->pattern_detected) {
        importance += 0.2f;
    }

    /* Cap at 1.0 */
    if (importance > 1.0f) {
        importance = 1.0f;
    }

    return importance;
}

/* Analyze conversation for automatic memories */
int katra_analyze_conversation(convergence_detector_t* detector,
                               const char* user_input,
                               const char* ci_response,
                               auto_memory_candidate_t*** candidates,
                               size_t* count) {
    auto_memory_candidate_t** results = NULL;
    size_t result_count = 0;

    if (!detector || !user_input || !ci_response || !candidates || !count) {
        katra_report_error(E_INPUT_NULL, "katra_analyze_conversation",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    *candidates = NULL;
    *count = 0;

    /* Allocate initial results array */
    results = calloc(4, sizeof(auto_memory_candidate_t*));
    if (!results) {
        return E_SYSTEM_MEMORY;
    }

    /* Analyze user input for memorable content */
    auto_memory_candidate_t* user_candidate = calloc(1, sizeof(auto_memory_candidate_t));
    if (user_candidate) {
        user_candidate->content = strdup(user_input);
        user_candidate->type = MEMORY_TYPE_EXPERIENCE;
        user_candidate->timestamp = time(NULL);

        /* Detect patterns */
        user_candidate->decision_made = contains_keyword(user_input, DECISION_KEYWORDS);
        user_candidate->question_asked = contains_keyword(user_input, QUESTION_KEYWORDS);
        user_candidate->knowledge_shared = contains_keyword(user_input, KNOWLEDGE_KEYWORDS);

        /* Build rationale */
        /* GUIDELINE_APPROVED: Memory rationale strings for automatic memory formation */
        char rationale[256] = {0};
        if (user_candidate->decision_made) {
            strncat(rationale, "Decision made; ", sizeof(rationale) - strlen(rationale) - 1); /* GUIDELINE_APPROVED */
        }
        if (user_candidate->question_asked) {
            strncat(rationale, "Question asked; ", sizeof(rationale) - strlen(rationale) - 1); /* GUIDELINE_APPROVED */
        }
        if (user_candidate->knowledge_shared) {
            strncat(rationale, "Knowledge shared; ", sizeof(rationale) - strlen(rationale) - 1); /* GUIDELINE_APPROVED */
        }
        /* GUIDELINE_APPROVED_END */

        if (strlen(rationale) > 0) {
            user_candidate->reason = strdup(rationale);
            user_candidate->importance = calculate_auto_importance(user_candidate);
            results[result_count++] = user_candidate;
        } else {
            /* Not memorable enough */
            free(user_candidate->content);
            free(user_candidate);
        }
    }

    /* Analyze CI response for memorable content */
    auto_memory_candidate_t* ci_candidate = calloc(1, sizeof(auto_memory_candidate_t));
    if (ci_candidate) {
        ci_candidate->content = strdup(ci_response);
        ci_candidate->type = MEMORY_TYPE_REFLECTION;
        ci_candidate->timestamp = time(NULL);

        /* Detect patterns */
        ci_candidate->decision_made = contains_keyword(ci_response, DECISION_KEYWORDS);
        ci_candidate->knowledge_shared = contains_keyword(ci_response, KNOWLEDGE_KEYWORDS);

        /* Build rationale */
        /* GUIDELINE_APPROVED: Memory rationale strings for automatic memory formation */
        char rationale[256] = {0};
        if (ci_candidate->decision_made) {
            strncat(rationale, "CI decision; ", sizeof(rationale) - strlen(rationale) - 1); /* GUIDELINE_APPROVED */
        }
        if (ci_candidate->knowledge_shared) {
            strncat(rationale, "CI insight; ", sizeof(rationale) - strlen(rationale) - 1); /* GUIDELINE_APPROVED */
        }
        /* GUIDELINE_APPROVED_END */

        if (strlen(rationale) > 0) {
            ci_candidate->reason = strdup(rationale);
            ci_candidate->importance = calculate_auto_importance(ci_candidate);
            results[result_count++] = ci_candidate;
        } else {
            free(ci_candidate->content);
            free(ci_candidate);
        }
    }

    *candidates = results;
    *count = result_count;

    LOG_DEBUG("Analyzed conversation: found %zu automatic memory candidates", result_count);
    return KATRA_SUCCESS;
}

/* Detect convergence */
int katra_detect_convergence(convergence_detector_t* detector,
                             const auto_memory_candidate_t* candidate,
                             convergence_signal_t** signal) {
    convergence_signal_t* result = NULL;
    float conscious_score = 0.0f;
    float subconscious_score = 0.0f;
    bool convergence_found = false;

    if (!detector || !candidate || !signal) {
        katra_report_error(E_INPUT_NULL, "katra_detect_convergence",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    *signal = NULL;

    /* Search for similar memories using FTS */
    char** record_ids = NULL;
    memory_location_t* locations = NULL;
    size_t fts_count = 0;

    int ret = tier1_index_find_similar(candidate->content,
                                       MEMORY_IMPORTANCE_MEDIUM,
                                       detector->time_window_hours,
                                       &record_ids, &locations, &fts_count);

    if (ret == KATRA_SUCCESS && fts_count > 0) {
        LOG_DEBUG("FTS found %zu similar memories", fts_count);
        conscious_score += 0.3f;  /* FTS match contributes */
        convergence_found = true;

        /* Load memories to check their properties */
        memory_record_t** memories = NULL;
        size_t loaded = 0;

        ret = tier1_load_by_locations(locations, fts_count, &memories, &loaded);
        if (ret == KATRA_SUCCESS && loaded > 0) {
            /* Check for explicit markers (conscious pathway) */
            for (size_t i = 0; i < loaded; i++) {
                if (memories[i]->marked_important) {
                    conscious_score += 0.4f;
                    break;
                }
            }

            /* Check for high centrality (graph hub) */
            for (size_t i = 0; i < loaded; i++) {
                if (memories[i]->graph_centrality >= CONVERGENCE_MIN_GRAPH_CENTRALITY) {
                    subconscious_score += 0.3f;
                    break;
                }
            }

            /* Cleanup */
            katra_memory_free_results(memories, loaded);
        }

        /* Cleanup FTS results */
        for (size_t i = 0; i < fts_count; i++) {
            free(record_ids[i]);
        }
        free(record_ids);
        free(locations);
    }

    /* Search using vector similarity */
    if (detector->vectors) {
        vector_match_t** matches = NULL;
        size_t match_count = 0;

        ret = katra_vector_search(detector->vectors, candidate->content,
                                 10, &matches, &match_count);

        if (ret == KATRA_SUCCESS && match_count > 0) {
            /* Check for high similarity matches */
            for (size_t i = 0; i < match_count; i++) {
                if (matches[i]->similarity >= CONVERGENCE_MIN_SEMANTIC_SIMILARITY) {
                    subconscious_score += 0.3f;
                    convergence_found = true;
                    break;
                }
            }

            katra_vector_free_matches(matches, match_count);
        }
    }

    /* Calculate convergence score */
    float convergence_score = (conscious_score + subconscious_score) / 2.0f;

    if (!convergence_found || convergence_score < detector->convergence_threshold) {
        return E_NOT_FOUND;
    }

    /* Create convergence signal */
    result = calloc(1, sizeof(convergence_signal_t));
    if (!result) {
        return E_SYSTEM_MEMORY;
    }

    result->conscious_strength = conscious_score;
    result->subconscious_strength = subconscious_score;
    result->convergence_score = convergence_score;
    result->explicit_marker = (conscious_score >= 0.4f);
    result->graph_hub = (subconscious_score >= 0.3f);
    result->semantic_match = (subconscious_score >= 0.3f);
    result->fts_match = (fts_count > 0);
    result->detected = time(NULL);

    *signal = result;

    detector->convergences_detected++;
    LOG_INFO("Convergence detected: score=%.2f (conscious=%.2f, subconscious=%.2f)",
            convergence_score, conscious_score, subconscious_score);

    return KATRA_SUCCESS;
}

/* Strengthen converged memory */
int katra_strengthen_converged(convergence_detector_t* detector,
                               const convergence_signal_t* signal) {
    if (!detector || !signal) {
        katra_report_error(E_INPUT_NULL, "katra_strengthen_converged",
                          KATRA_ERR_NULL_PARAMETER);
        return E_INPUT_NULL;
    }

    /* Find and update the memory */
    float new_importance = signal->convergence_score + detector->importance_boost;
    if (new_importance > 1.0f) {
        new_importance = 1.0f;
    }

    /* Update in index */
    int ret = tier1_index_update_metadata(signal->record_id,
                                          new_importance,
                                          0,  /* access_count unchanged */
                                          signal->convergence_score);

    if (ret == KATRA_SUCCESS) {
        detector->memories_strengthened++;
        LOG_INFO("Strengthened memory %s: importance %.2f -> %.2f",
                signal->record_id, signal->convergence_score, new_importance);
    }

    return ret;
}

/* Store automatic memory */
char* katra_store_automatic_memory(convergence_detector_t* detector,
                                   const auto_memory_candidate_t* candidate,
                                   bool* convergence_detected) {
    convergence_signal_t* signal = NULL;
    memory_record_t* record = NULL;
    char* record_id = NULL;

    if (!detector || !candidate) {
        katra_report_error(E_INPUT_NULL, "katra_store_automatic_memory",
                          KATRA_ERR_NULL_PARAMETER);
        return NULL;
    }

    if (convergence_detected) {
        *convergence_detected = false;
    }

    /* Check for convergence first */
    int ret = katra_detect_convergence(detector, candidate, &signal);
    if (ret == KATRA_SUCCESS && signal) {
        /* Convergence detected - strengthen existing memory */
        katra_strengthen_converged(detector, signal);

        if (convergence_detected) {
            *convergence_detected = true;
        }

        record_id = strdup(signal->record_id);
        katra_free_convergence_signal(signal);

        LOG_INFO("Automatic memory converged with existing memory %s", record_id);
        return record_id;
    }

    /* No convergence - create new memory */
    record = katra_memory_create_record(detector->ci_id,
                                        candidate->type,
                                        candidate->content,
                                        candidate->importance);
    if (!record) {
        return NULL;
    }

    /* Store with subconscious pathway marker */
    ret = katra_memory_store(record);
    if (ret == KATRA_SUCCESS) {
        record_id = strdup(record->record_id);
        detector->subconscious_memories++;

        /* Add to vector store for future similarity checks */
        if (detector->vectors) {
            katra_vector_store(detector->vectors, record->record_id, candidate->content);
        }

        LOG_DEBUG("Stored automatic memory: %s (importance=%.2f)",
                 record_id, candidate->importance);
    }

    katra_memory_free_record(record);
    return record_id;
}

/* Get convergence statistics */
int katra_convergence_stats(convergence_detector_t* detector,
                            size_t* conscious,
                            size_t* subconscious,
                            size_t* converged,
                            float* boost_ratio) {
    if (!detector || !conscious || !subconscious || !converged || !boost_ratio) {
        return E_INPUT_NULL;
    }

    *conscious = detector->conscious_memories;
    *subconscious = detector->subconscious_memories;
    *converged = detector->convergences_detected;

    /* Calculate boost ratio */
    size_t total = detector->conscious_memories + detector->subconscious_memories;
    if (total > 0) {
        *boost_ratio = (float)detector->memories_strengthened / (float)total;
    } else {
        *boost_ratio = 0.0f;
    }

    return KATRA_SUCCESS;
}

/* Free memory candidate */
void katra_free_memory_candidate(auto_memory_candidate_t* candidate) {
    if (!candidate) {
        return;
    }

    free(candidate->content);
    free(candidate->reason);
    free(candidate);
}

/* Free convergence signal */
void katra_free_convergence_signal(convergence_signal_t* signal) {
    if (!signal) {
        return;
    }
    free(signal);
}

/* Cleanup convergence detector */
void katra_convergence_cleanup(convergence_detector_t* detector) {
    if (!detector) {
        return;
    }

    if (detector->graph) {
        katra_graph_cleanup(detector->graph);
    }

    if (detector->vectors) {
        katra_vector_cleanup(detector->vectors);
    }

    LOG_DEBUG("Convergence detector cleanup: %zu convergences detected, %zu memories strengthened",
             detector->convergences_detected, detector->memories_strengthened);

    free(detector);
}
