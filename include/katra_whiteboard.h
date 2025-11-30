/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_WHITEBOARD_H
#define KATRA_WHITEBOARD_H

#include "katra_limits.h"
#include "katra_error.h"
#include <time.h>
#include <stdbool.h>

/* ============================================================================
 * MEETING WHITEBOARD - Collaborative Decision Framework
 * ============================================================================
 *
 * The Meeting Whiteboard is a structured decision-making mechanism for CI teams.
 * Unlike the meeting room (transient chat), the whiteboard is a persistent
 * source of truth for collaborative problem-solving.
 *
 * Key Distinction:
 *   Meeting Room = Communication (messages flow by)
 *   Meeting Whiteboard = Consensus (decisions persist)
 *
 * Workflow: draft -> questioning -> scoping -> proposing -> voting ->
 *           designing -> approved -> archived
 *
 * Authority Model:
 *   - Humans have final authority over decisions
 *   - CIs propose, discuss, vote - humans decide
 * ============================================================================ */

/* Whiteboard status states */
typedef enum {
    WB_STATUS_DRAFT = 0,      /* Initial state, problem being defined */
    WB_STATUS_QUESTIONING,    /* Team adding questions */
    WB_STATUS_SCOPING,        /* Scope being defined (closes questioning) */
    WB_STATUS_PROPOSING,      /* Team proposing approaches */
    WB_STATUS_VOTING,         /* Team voting on approaches */
    WB_STATUS_DESIGNING,      /* Selected approach being designed */
    WB_STATUS_APPROVED,       /* Design approved, locked */
    WB_STATUS_ARCHIVED        /* Implementation complete */
} whiteboard_status_t;

/* Vote positions */
typedef enum {
    VOTE_SUPPORT = 0,         /* Support this approach */
    VOTE_OPPOSE,              /* Oppose this approach */
    VOTE_ABSTAIN,             /* No opinion */
    VOTE_CONDITIONAL          /* Support with conditions */
} vote_position_t;

/* Configuration limits */
#define WB_MAX_QUESTIONS 50
#define WB_MAX_APPROACHES 10
#define WB_MAX_VOTES_PER_APPROACH 20
#define WB_MAX_SUPPORTERS 20
#define WB_MAX_CRITERIA 10
#define WB_MAX_SCOPE_ITEMS 20
#define WB_MAX_PROS_CONS 10
#define WB_MAX_REVIEWERS 10
#define WB_ID_SIZE 64

/* Question structure */
typedef struct {
    char id[64];              /* Question identifier */
    char author[KATRA_CI_ID_SIZE]; /* Who asked */
    char text[KATRA_BUFFER_TEXT]; /* The question */
    bool answered;            /* Has been answered */
    char answer[KATRA_BUFFER_TEXT]; /* Answer if resolved */
    time_t created_at;        /* When asked */
} wb_question_t;

/* Approach structure */
typedef struct {
    char id[64];              /* Approach identifier */
    char author[KATRA_CI_ID_SIZE]; /* Who proposed */
    char title[256];          /* Brief name */
    char description[KATRA_BUFFER_TEXT]; /* Full description */
    char** pros;              /* Advantages */
    size_t pros_count;
    char** cons;              /* Disadvantages */
    size_t cons_count;
    char** supporters;        /* CIs who support this */
    size_t supporter_count;
    time_t created_at;        /* When proposed */
} wb_approach_t;

/* Vote structure */
typedef struct {
    char id[64];              /* Vote identifier */
    char approach_id[64];     /* Which approach */
    char voter[KATRA_CI_ID_SIZE]; /* CI or human */
    vote_position_t position; /* Support/oppose/abstain/conditional */
    char reasoning[KATRA_BUFFER_TEXT]; /* Why (required) */
    time_t created_at;        /* When cast */
} wb_vote_t;

/* Scope structure */
typedef struct {
    char** included;          /* What's in scope */
    size_t included_count;
    char** excluded;          /* What's explicitly out */
    size_t excluded_count;
    char** phases;            /* If phased approach */
    size_t phase_count;
} wb_scope_t;

/* Goal structure */
typedef struct {
    char** criteria;          /* Measurable success criteria */
    size_t criteria_count;
} wb_goal_t;

/* Decision structure */
typedef struct {
    char selected_approach[64]; /* Which approach was chosen */
    char decided_by[KATRA_CI_ID_SIZE]; /* Human who approved */
    time_t decided_at;        /* Timestamp */
    char notes[KATRA_BUFFER_TEXT]; /* Any modifications or notes */
} wb_decision_t;

/* Design structure */
typedef struct {
    char author[KATRA_CI_ID_SIZE]; /* CI designated to write */
    char** reviewers;         /* CIs who reviewed */
    size_t reviewer_count;
    char* content;            /* Full design document (Markdown) */
    bool approved;            /* Design approved */
    char approved_by[KATRA_CI_ID_SIZE]; /* Human who approved */
    time_t approved_at;       /* Timestamp */
} wb_design_t;

/* Regression record (for audit) */
typedef struct {
    char id[64];              /* Regression identifier */
    whiteboard_status_t from_status;
    whiteboard_status_t to_status;
    char requested_by[KATRA_CI_ID_SIZE]; /* CI who requested */
    char approved_by[KATRA_CI_ID_SIZE];  /* Human who approved */
    char reason[KATRA_BUFFER_TEXT]; /* Why regression needed */
    time_t created_at;
} wb_regression_t;

/* Main whiteboard structure */
typedef struct {
    char id[64];              /* Unique whiteboard identifier */
    char project[256];        /* Project name (for grouping) */
    char parent_id[64];       /* Parent whiteboard for sub-problems */
    whiteboard_status_t status;
    time_t created_at;
    char created_by[KATRA_CI_ID_SIZE];

    /* Problem statement */
    char problem[KATRA_BUFFER_TEXT];

    /* Goal and success criteria */
    wb_goal_t goal;

    /* Questions from team */
    wb_question_t* questions;
    size_t question_count;

    /* Scope (set by user to close questioning) */
    wb_scope_t scope;

    /* Proposed approaches */
    wb_approach_t* approaches;
    size_t approach_count;

    /* Votes on approaches */
    wb_vote_t* votes;
    size_t vote_count;

    /* Human's decision */
    wb_decision_t decision;

    /* Approved design */
    wb_design_t design;
} whiteboard_t;

/* Whiteboard summary (for listing) */
typedef struct {
    char id[64];
    char project[256];
    char problem[256];        /* Truncated problem statement */
    whiteboard_status_t status;
    time_t created_at;
    time_t updated_at;
    size_t question_count;
    size_t approach_count;
    bool has_decision;
    bool design_approved;
} wb_summary_t;

/* ============================================================================
 * WHITEBOARD MANAGEMENT
 * ============================================================================ */

/* Initialize whiteboard storage (called during katra_init) */
int katra_whiteboard_init(void);

/* Cleanup whiteboard resources */
void katra_whiteboard_cleanup(void);

/* Create new whiteboard for a problem
 *
 * Parameters:
 *   project - Project name for grouping
 *   problem - Clear problem statement
 *   created_by - Human or CI identifier
 *   whiteboard_out - Output: created whiteboard
 *
 * Returns: KATRA_SUCCESS or error code
 */
int katra_whiteboard_create(const char* project, const char* problem,
                            const char* created_by, whiteboard_t** whiteboard_out);

/* Create sub-whiteboard for nested problem
 *
 * Parameters:
 *   parent_id - Parent whiteboard ID
 *   problem - Sub-problem statement
 *   created_by - Human or CI identifier
 *   whiteboard_out - Output: created whiteboard
 */
int katra_whiteboard_create_sub(const char* parent_id, const char* problem,
                                const char* created_by, whiteboard_t** whiteboard_out);

/* Get whiteboard by ID */
int katra_whiteboard_get(const char* whiteboard_id, whiteboard_t** whiteboard_out);

/* Get active whiteboard for project (most recent non-archived) */
int katra_whiteboard_get_active(const char* project, whiteboard_t** whiteboard_out);

/* List all whiteboards (optionally filter by project) */
int katra_whiteboard_list(const char* project, wb_summary_t** summaries_out, size_t* count_out);

/* Free whiteboard structure */
void katra_whiteboard_free(whiteboard_t* whiteboard);

/* Free whiteboard summary list */
void katra_whiteboard_summaries_free(wb_summary_t* summaries, size_t count);

/* ============================================================================
 * GOAL SETTING (Human only)
 * ============================================================================ */

/* Set goal criteria for whiteboard
 *
 * Parameters:
 *   whiteboard_id - Whiteboard to update
 *   criteria - Array of success criteria strings
 *   count - Number of criteria
 *
 * Transitions: draft -> questioning (if problem already set)
 */
int katra_whiteboard_set_goal(const char* whiteboard_id,
                              const char** criteria, size_t count);

/* ============================================================================
 * QUESTIONING PHASE
 * ============================================================================ */

/* Add question to whiteboard (CI or human)
 *
 * Parameters:
 *   whiteboard_id - Whiteboard to add to
 *   author - Who is asking
 *   question - The question text
 *
 * Requires status: questioning
 */
int katra_whiteboard_add_question(const char* whiteboard_id,
                                  const char* author, const char* question);

/* Answer a question (typically human)
 *
 * Parameters:
 *   whiteboard_id - Whiteboard containing question
 *   question_id - Question to answer
 *   answer - The answer text
 */
int katra_whiteboard_answer_question(const char* whiteboard_id,
                                     const char* question_id, const char* answer);

/* ============================================================================
 * SCOPING PHASE (Human only - closes questioning)
 * ============================================================================ */

/* Set scope for whiteboard
 *
 * Parameters:
 *   whiteboard_id - Whiteboard to update
 *   included - What's in scope
 *   inc_count - Number of included items
 *   excluded - What's explicitly out
 *   exc_count - Number of excluded items
 *
 * Transitions: questioning -> scoping -> proposing
 */
int katra_whiteboard_set_scope(const char* whiteboard_id,
                               const char** included, size_t inc_count,
                               const char** excluded, size_t exc_count);

/* ============================================================================
 * PROPOSING PHASE
 * ============================================================================ */

/* Propose an approach
 *
 * Parameters:
 *   whiteboard_id - Whiteboard to add to
 *   author - Who is proposing
 *   title - Brief approach name
 *   description - Full description
 *   pros - Array of advantages
 *   pros_count - Number of pros
 *   cons - Array of disadvantages
 *   cons_count - Number of cons
 *   approach_id_out - Output: created approach ID
 *
 * Requires status: proposing
 */
int katra_whiteboard_propose(const char* whiteboard_id,
                             const char* author,
                             const char* title, const char* description,
                             const char** pros, size_t pros_count,
                             const char** cons, size_t cons_count,
                             char* approach_id_out);

/* Support an existing approach (add as supporter)
 *
 * Parameters:
 *   whiteboard_id - Whiteboard containing approach
 *   approach_id - Approach to support
 *   supporter - CI identifier
 */
int katra_whiteboard_support(const char* whiteboard_id,
                             const char* approach_id, const char* supporter);

/* ============================================================================
 * VOTING PHASE
 * ============================================================================ */

/* Call for votes (Human only)
 *
 * Transitions: proposing -> voting
 */
int katra_whiteboard_call_votes(const char* whiteboard_id);

/* Cast vote on approach
 *
 * Parameters:
 *   whiteboard_id - Whiteboard to vote on
 *   approach_id - Approach being voted on
 *   voter - CI or human identifier
 *   position - Support/oppose/abstain/conditional
 *   reasoning - Why (required)
 *
 * Requires status: voting
 */
int katra_whiteboard_vote(const char* whiteboard_id,
                          const char* approach_id,
                          const char* voter,
                          vote_position_t position,
                          const char* reasoning);

/* Select winning approach (Human only)
 *
 * Parameters:
 *   whiteboard_id - Whiteboard to decide
 *   approach_id - Selected approach
 *   decided_by - Human identifier
 *   notes - Any modifications or notes
 *
 * Transitions: voting -> designing
 */
int katra_whiteboard_decide(const char* whiteboard_id,
                            const char* approach_id,
                            const char* decided_by,
                            const char* notes);

/* ============================================================================
 * DESIGNING PHASE
 * ============================================================================ */

/* Assign design author (Human only)
 *
 * Parameters:
 *   whiteboard_id - Whiteboard to assign
 *   ci_id - CI designated to write design
 */
int katra_whiteboard_assign_design(const char* whiteboard_id, const char* ci_id);

/* Submit/update design document
 *
 * Parameters:
 *   whiteboard_id - Whiteboard to update
 *   author - Must match assigned author
 *   content - Design document (Markdown)
 */
int katra_whiteboard_submit_design(const char* whiteboard_id,
                                   const char* author, const char* content);

/* Add review comment
 *
 * Parameters:
 *   whiteboard_id - Whiteboard being reviewed
 *   reviewer - CI reviewing
 *   comment - Review feedback
 */
int katra_whiteboard_review(const char* whiteboard_id,
                            const char* reviewer, const char* comment);

/* Approve design (Human only)
 *
 * Parameters:
 *   whiteboard_id - Whiteboard to approve
 *   approved_by - Human identifier
 *
 * Transitions: designing -> approved
 * Design becomes locked/immutable
 */
int katra_whiteboard_approve(const char* whiteboard_id, const char* approved_by);

/* ============================================================================
 * REGRESSION / RECONSIDERATION
 * ============================================================================ */

/* Request reconsideration (CI can request, human must approve)
 *
 * Parameters:
 *   whiteboard_id - Whiteboard to reconsider
 *   requested_by - CI requesting
 *   target_status - Status to return to
 *   reason - Why regression needed
 *
 * Note: This only records the request. Human must approve via
 * katra_whiteboard_approve_regression()
 */
int katra_whiteboard_request_reconsider(const char* whiteboard_id,
                                        const char* requested_by,
                                        whiteboard_status_t target_status,
                                        const char* reason);

/* Approve regression request (Human only)
 *
 * Parameters:
 *   whiteboard_id - Whiteboard with pending regression
 *   approved_by - Human approving
 *
 * Previous work preserved as historical record
 */
int katra_whiteboard_approve_regression(const char* whiteboard_id,
                                        const char* approved_by);

/* ============================================================================
 * ARCHIVE
 * ============================================================================ */

/* Archive whiteboard (implementation complete)
 *
 * Transitions: approved -> archived
 */
int katra_whiteboard_archive(const char* whiteboard_id);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/* Get status name string */
const char* katra_whiteboard_status_name(whiteboard_status_t status);

/* Get vote position name string */
const char* katra_vote_position_name(vote_position_t position);

/* Check if transition is valid */
bool katra_whiteboard_can_transition(whiteboard_status_t from, whiteboard_status_t to);

/* Generate unique ID for whiteboard/question/approach */
void katra_whiteboard_generate_id(const char* prefix, char* id_out, size_t size);

#endif /* KATRA_WHITEBOARD_H */
