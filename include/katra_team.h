/* © 2025 Casey Koons All rights reserved */

#ifndef KATRA_TEAM_H
#define KATRA_TEAM_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/**
 * katra_team.h - Team Management for Namespace Isolation (Phase 7)
 *
 * Provides team creation, membership management, and access control
 * for TEAM-level memory isolation.
 */

/* Team member structure */
typedef struct {
    char* ci_id;          /* CI identifier */
    char* team_name;      /* Team this CI belongs to */
    bool is_owner;        /* True if this CI owns the team */
    time_t joined_at;     /* When CI joined the team */
} team_member_t;

/**
 * katra_team_init() - Initialize team registry
 *
 * Creates SQLite tables for team management if they don't exist.
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_SYSTEM_FILE if database cannot be opened
 */
int katra_team_init(void);

/**
 * katra_team_cleanup() - Cleanup team registry
 *
 * Closes database connections and frees resources.
 */
void katra_team_cleanup(void);

/**
 * katra_team_create() - Create a new team
 *
 * Parameters:
 *   team_name - Unique team identifier
 *   owner_ci_id - CI that owns/created the team
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_TEAM_ALREADY_EXISTS if team name taken
 *   E_SYSTEM_FILE if database error
 */
int katra_team_create(const char* team_name, const char* owner_ci_id);

/**
 * katra_team_join() - Add CI to existing team
 *
 * Parameters:
 *   team_name - Team to join
 *   ci_id - CI requesting to join
 *   invited_by - CI that invited this member (must be owner or member)
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_TEAM_NOT_FOUND if team doesn't exist
 *   E_PERMISSION_DENIED if inviter not authorized
 *   E_TEAM_ALREADY_MEMBER if CI already in team
 */
int katra_team_join(const char* team_name, const char* ci_id,
                    const char* invited_by);

/**
 * katra_team_leave() - Remove CI from team
 *
 * Parameters:
 *   team_name - Team to leave
 *   ci_id - CI leaving the team
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_TEAM_NOT_FOUND if team doesn't exist
 *   E_TEAM_OWNER_CANNOT_LEAVE if owner tries to leave (must delete team)
 */
int katra_team_leave(const char* team_name, const char* ci_id);

/**
 * katra_team_delete() - Delete entire team
 *
 * Only the team owner can delete a team. All members are removed.
 *
 * Parameters:
 *   team_name - Team to delete
 *   owner_ci_id - Must be the team owner
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_TEAM_NOT_FOUND if team doesn't exist
 *   E_PERMISSION_DENIED if ci_id is not the owner
 */
int katra_team_delete(const char* team_name, const char* owner_ci_id);

/**
 * katra_team_is_member() - Check if CI is member of team
 *
 * Parameters:
 *   team_name - Team to check
 *   ci_id - CI to check membership
 *
 * Returns:
 *   true if CI is member, false otherwise
 */
bool katra_team_is_member(const char* team_name, const char* ci_id);

/**
 * katra_team_is_owner() - Check if CI owns team
 *
 * Parameters:
 *   team_name - Team to check
 *   ci_id - CI to check ownership
 *
 * Returns:
 *   true if CI is owner, false otherwise
 */
bool katra_team_is_owner(const char* team_name, const char* ci_id);

/**
 * katra_team_list_members() - Get all members of a team
 *
 * Parameters:
 *   team_name - Team to query
 *   members_out - Output array of team members (caller must free)
 *   count_out - Number of members returned
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 *   E_TEAM_NOT_FOUND if team doesn't exist
 */
int katra_team_list_members(const char* team_name,
                             team_member_t** members_out,
                             size_t* count_out);

/**
 * katra_team_list_for_ci() - Get all teams a CI belongs to
 *
 * Parameters:
 *   ci_id - CI to query
 *   teams_out - Output array of team names (caller must free)
 *   count_out - Number of teams returned
 *
 * Returns:
 *   KATRA_SUCCESS on success
 *   E_INPUT_NULL if parameters are NULL
 */
int katra_team_list_for_ci(const char* ci_id,
                            char*** teams_out,
                            size_t* count_out);

/**
 * katra_team_free_members() - Free team member array
 *
 * Parameters:
 *   members - Array returned by katra_team_list_members()
 *   count - Number of members in array
 */
void katra_team_free_members(team_member_t* members, size_t count);

/**
 * katra_team_free_list() - Free team name array
 *
 * Parameters:
 *   teams - Array returned by katra_team_list_for_ci()
 *   count - Number of teams in array
 */
void katra_team_free_list(char** teams, size_t count);

/* ============================================================================
 * SQL QUERY CONSTANTS
 * ============================================================================ */

/* Team table SQL */
#define TEAM_SQL_CHECK_EXISTS \
    "SELECT team_name FROM teams WHERE team_name = ?"

#define TEAM_SQL_CREATE \
    "INSERT INTO teams (team_name, owner_ci_id, created_at) VALUES (?, ?, ?)"

#define TEAM_SQL_GET_OWNER \
    "SELECT owner_ci_id FROM teams WHERE team_name = ?"

#define TEAM_SQL_DELETE \
    "DELETE FROM teams WHERE team_name = ?"

/* Team member SQL */
#define TEAM_SQL_CHECK_MEMBER \
    "SELECT ci_id FROM team_members WHERE team_name = ? AND ci_id = ?"

#define TEAM_SQL_GET_MEMBER_STATUS \
    "SELECT is_owner FROM team_members WHERE team_name = ? AND ci_id = ?"

#define TEAM_SQL_ADD_MEMBER \
    "INSERT INTO team_members (team_name, ci_id, is_owner, joined_at) VALUES (?, ?, ?, ?)"

#define TEAM_SQL_REMOVE_MEMBER \
    "DELETE FROM team_members WHERE team_name = ? AND ci_id = ?"

#define TEAM_SQL_LIST_MEMBERS \
    "SELECT ci_id, is_owner, joined_at FROM team_members WHERE team_name = ? ORDER BY joined_at ASC"

#define TEAM_SQL_LIST_FOR_CI \
    "SELECT team_name FROM team_members WHERE ci_id = ? ORDER BY joined_at ASC"

/* Error messages */
#define TEAM_ERR_MUTEX_LOCK "Failed to acquire mutex lock"
#define TEAM_ERR_NOT_INITIALIZED "Team registry not initialized"
#define TEAM_ERR_ALREADY_EXISTS "Team already exists"
#define TEAM_ERR_NOT_FOUND "Team not found"
#define TEAM_ERR_ALREADY_MEMBER "Already a member of this team"
#define TEAM_ERR_NOT_MEMBER "Not a member of this team"
#define TEAM_ERR_OWNER_CANNOT_LEAVE "Team owner cannot leave (must delete team)"
#define TEAM_ERR_NOT_OWNER "Only team owner can perform this action"
#define TEAM_ERR_DB_QUERY "Database query failed"

#endif /* KATRA_TEAM_H */
