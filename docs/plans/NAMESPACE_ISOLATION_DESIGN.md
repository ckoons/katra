# Namespace Isolation Design

© 2025 Casey Koons All rights reserved

## Executive Summary

**Problem**: Katra currently has strict per-CI memory isolation. CIs can communicate via ephemeral messages (Meeting Room) but cannot share persistent memories for collaboration.

**Solution**: Add opt-in memory sharing with three isolation levels (private, team, public) while maintaining backward compatibility and strong privacy defaults.

**Feedback Context**: "Shared memory is powerful for collaboration but needs opt-in control" (Alice/Kari testing feedback)

---

## Current Architecture

### Memory Isolation (Status Quo)

Every `memory_record_t` has a `ci_id` field:
```c
typedef struct {
    char* ci_id;               /* Which CI this memory belongs to */
    // ...
    bool personal;             /* Part of personal collection */
    char* collection;          /* Collection path: "People/Casey" */
    // ...
} memory_record_t;
```

**Current Behavior**:
- All queries require `ci_id` in `memory_query_t`
- Memories are strictly scoped to owning CI
- No cross-CI memory access exists

### Meeting Room (Ephemeral Communication)

Provides temporary message passing between CIs:
- 2-hour message TTL
- Broadcast and direct messaging
- Self-filtering (CIs don't see own messages)
- **Not persistent memories** - just chat messages

### The Gap

CIs collaborating on a project cannot:
1. Share persistent memories about shared work
2. Build collective knowledge bases
3. Create team-visible decision records
4. Grant selective memory access to specific CIs

---

## Design Principles

1. **Privacy by Default**: All memories private unless explicitly shared
2. **Explicit Opt-in**: Sharing requires conscious choice
3. **Granular Control**: Per-memory, per-CI, and per-query scoping
4. **Backward Compatible**: Existing code works unchanged
5. **Ethics First**: Clear ownership, audit trails, consent

---

## Proposed Solution: Three-Level Isolation

### Isolation Levels

```c
typedef enum {
    MEMORY_ISOLATION_PRIVATE = 0,   /* Only owning CI (default) */
    MEMORY_ISOLATION_TEAM = 1,      /* Named team/project members */
    MEMORY_ISOLATION_PUBLIC = 2     /* All CIs (global knowledge) */
} memory_isolation_t;
```

**Level 0 - PRIVATE** (default):
- Only owning CI can query
- Existing behavior (backward compatible)
- Personal thoughts, private notes

**Level 1 - TEAM**:
- Shared with specific named team
- Project-scoped collaboration
- Example: "tekton-dev-team", "research-group-alpha"

**Level 2 - PUBLIC**:
- Visible to all CIs
- Shared knowledge base
- Example: API documentation, shared learnings

### Memory Record Extensions

Add to `memory_record_t` structure:

```c
typedef struct {
    // ... existing fields ...

    /* Namespace isolation (Phase 6.2) */
    memory_isolation_t isolation;   /* Isolation level (default: PRIVATE) */
    char* team_name;                /* Team name if isolation==TEAM (NULL otherwise) */
    char** shared_with_ci_ids;      /* Explicit CI whitelist (NULL if unused) */
    size_t shared_with_count;       /* Number of whitelisted CIs */

    /* Audit trail for shared memories */
    time_t shared_at;               /* When memory was shared (0 if never shared) */
    char* shared_by_ci_id;          /* CI that shared it (NULL if owner is sharer) */
} memory_record_t;
```

### Query Parameter Extensions

Add to `memory_query_t` structure:

```c
typedef struct {
    // ... existing fields ...

    /* Namespace isolation query options */
    bool include_team_memories;     /* Include TEAM memories caller has access to */
    const char* team_name;          /* Specific team to query (NULL = all teams) */
    bool include_public_memories;   /* Include PUBLIC memories */

    /* Explicit cross-CI query (requires consent) */
    const char* query_ci_id;        /* Query another CI's memories (NULL = own) */
    bool consent_granted;           /* True if cross-CI query authorized */
} memory_query_t;
```

---

## Implementation Strategy

### Phase 1: Foundation (Week 1)

**Add isolation fields to memory_record_t**:
- Add `isolation`, `team_name`, `shared_with_ci_ids` fields
- Default `isolation = MEMORY_ISOLATION_PRIVATE`
- Update JSON serialization/deserialization
- Update memory creation to set defaults

**Files to modify**:
- `include/katra_memory.h` - Add fields to struct
- `src/core/katra_memory.c` - Update create/free functions
- `src/core/katra_tier1_json.c` - Update JSON parsing
- `src/core/katra_tier2_json.c` - Update tier2 JSON

**Test coverage**:
- Unit test: Create memory with different isolation levels
- Unit test: JSON round-trip for new fields
- Integration test: Existing code works unchanged

### Phase 2: Team Management (Week 1-2)

**Create team registry**:
- SQLite table: `teams(team_name, ci_id, role, joined_at)`
- API: `katra_team_create()`, `katra_team_join()`, `katra_team_leave()`
- API: `katra_team_list_members()`, `katra_team_check_membership()`

**Files to create**:
- `include/katra_team.h` - Team management API
- `src/core/katra_team.c` - Team implementation
- `tests/unit/test_team.c` - Team tests

**Schema**:
```sql
CREATE TABLE teams (
    team_name TEXT NOT NULL,
    ci_id TEXT NOT NULL,
    role TEXT DEFAULT 'member',  /* 'owner', 'member' */
    joined_at INTEGER NOT NULL,
    PRIMARY KEY (team_name, ci_id)
);

CREATE INDEX idx_teams_ci ON teams(ci_id);
CREATE INDEX idx_teams_name ON teams(team_name);
```

### Phase 3: Query Logic (Week 2)

**Update memory query to respect isolation**:

```c
int katra_memory_query_with_isolation(const memory_query_t* query,
                                     memory_record_t*** results,
                                     size_t* count) {
    const char* caller_ci_id = query->ci_id;

    /* Build query with isolation rules */
    /* 1. Always include caller's PRIVATE memories */
    /* 2. If include_team_memories: include TEAM memories for caller's teams */
    /* 3. If include_public_memories: include PUBLIC memories */
    /* 4. If query_ci_id != caller_ci_id: require consent_granted */

    /* Execute query with proper filtering */
    /* ... */
}
```

**Files to modify**:
- `src/core/katra_memory.c` - Add isolation filtering logic
- `src/core/katra_tier1.c` - Update tier1 query filtering
- `src/core/katra_tier2.c` - Update tier2 query filtering

**Test coverage**:
- Private memories: Only owner can query
- Team memories: Only team members can query
- Public memories: Everyone can query
- Cross-CI queries: Require consent

### Phase 4: Breathing Layer Integration (Week 2-3)

**Update semantic memory operations**:

```c
/* Add parameter to katra_remember() */
int katra_remember(const char* content,
                  const char* context,
                  memory_isolation_t isolation,
                  const char* team_name);

/* Add parameter to katra_recall() */
int katra_recall(const char* topic,
                memory_scope_t scope);  /* PRIVATE_ONLY, INCLUDE_TEAM, INCLUDE_PUBLIC */
```

**Files to modify**:
- `include/katra_breathing.h` - Update API signatures
- `src/breathing/katra_breathing_primitives.c` - Add isolation support
- `src/breathing/katra_breathing_semantic.c` - Update recall logic

**MCP tool updates**:
- `katra_remember` - Add optional `isolation` and `team` parameters
- `katra_recall` - Add optional `scope` parameter
- New tool: `katra_team_join(team_name)` - Join a team
- New tool: `katra_team_create(team_name)` - Create a team

### Phase 5: Safety & Ethics (Week 3)

**Audit logging**:
- Log when memories are shared (level change)
- Log cross-CI queries (even with consent)
- Track team membership changes

**Consent verification**:
- Cross-CI queries must be explicit (no auto-consent)
- Warning messages when querying other CI's memories
- Audit trail for all cross-CI access

**Files to create/modify**:
- `src/core/katra_audit.c` - Audit logging
- `tests/ethical/test_isolation_consent.c` - Consent tests

---

## API Examples

### Example 1: Private Memory (Default)

```c
/* CI creates private memory (existing behavior) */
katra_remember("My personal insight about the project", "significant");
/* Stored with isolation=PRIVATE, only creator can recall */
```

### Example 2: Team Memory

```c
/* CI joins a team */
katra_team_join("tekton-dev-team");

/* CI creates team-shared memory */
katra_remember("API endpoint design decision for /users",
              "significant",
              MEMORY_ISOLATION_TEAM,
              "tekton-dev-team");

/* Another CI on the team can recall it */
katra_recall("API endpoint design", MEMORY_SCOPE_INCLUDE_TEAM);
/* Returns the team memory */
```

### Example 3: Public Knowledge

```c
/* CI creates public knowledge memory */
katra_remember("C11 standard: malloc() can return NULL on failure",
              "knowledge",
              MEMORY_ISOLATION_PUBLIC,
              NULL);  /* No team needed for public */

/* Any CI can recall public knowledge */
katra_recall("malloc NULL", MEMORY_SCOPE_INCLUDE_PUBLIC);
/* Returns the public knowledge */
```

### Example 4: Explicit Cross-CI Query (with Consent)

```c
/* CI A wants to understand CI B's perspective */
memory_query_t query = {
    .ci_id = "current_ci_id",           /* Caller's ID */
    .query_ci_id = "other_ci_id",       /* Target CI */
    .consent_granted = true,            /* Explicit consent */
    .include_team_memories = false,     /* Only their private memories */
    .include_public_memories = false
};

/* Requires explicit consent flag - forces conscious choice */
katra_memory_query_with_isolation(&query, &results, &count);
```

---

## Migration Strategy

### Backward Compatibility

**Existing code works unchanged**:
1. All new fields default to PRIVATE isolation
2. Existing queries work without changes (PRIVATE-only by default)
3. No breaking API changes to core functions

**Gradual adoption**:
1. Phase 1-2: Foundation + Teams (no behavior change)
2. Phase 3: Query logic (opt-in via new parameters)
3. Phase 4: Breathing layer (opt-in via new tools)
4. Phase 5: Full rollout with ethics review

### Data Migration

**Existing memories**:
- All existing memories implicitly `isolation=PRIVATE`
- No database migration needed (use defaults for missing fields)
- JSON fields optional (NULL = PRIVATE)

**Schema evolution**:
```sql
/* Add team table (new) */
CREATE TABLE IF NOT EXISTS teams (...);

/* Existing memory tables unchanged */
/* Use JSON for new fields (optional) */
```

---

## Open Questions

1. **Team ownership model**: Who can add/remove members?
   - **Proposal**: Team creator is owner, can add/remove members
   - Owner can transfer ownership or delete team

2. **Memory re-sharing**: Can team members re-share to other teams?
   - **Proposal**: No - only original creator can change isolation level
   - Audit trail tracks all isolation level changes

3. **Team memory deletion**: What happens when team is dissolved?
   - **Proposal**: Memories revert to PRIVATE for original creators
   - Clear warning before team deletion

4. **Public memory limits**: Should there be rate limits?
   - **Proposal**: Yes - max N public memories per CI per day
   - Prevent public namespace pollution

5. **Cross-CI query consent**: How is consent verified?
   - **Proposal**: Explicit boolean flag in query (no implicit consent)
   - Could add persistent consent tokens in future phase

---

## Success Metrics

**Functionality**:
- [ ] CI can create team-shared memories
- [ ] CI can query team memories they have access to
- [ ] CI cannot query private memories of other CIs
- [ ] Public memories visible to all CIs
- [ ] Audit trail for all cross-CI operations

**Performance**:
- Query latency: <10ms additional overhead for isolation filtering
- Storage overhead: <5% increase for isolation metadata
- Team membership check: <1ms per query

**Ethics**:
- Zero unauthorized cross-CI memory access
- 100% of sharing operations logged
- Clear user understanding of isolation levels

---

## Future Enhancements (Post-Phase 6.2)

1. **Fine-grained sharing**: Individual CI whitelist/blacklist
2. **Time-bounded sharing**: Memories shared for limited duration
3. **Role-based access**: Team roles with different permissions
4. **Federated teams**: Teams spanning multiple Katra instances
5. **Memory delegation**: CI A grants CI B ability to manage their memories

---

## References

- **Kari/Alice Feedback**: "Shared memory is powerful for collaboration but needs opt-in control"
- **Current Architecture**: `include/katra_memory.h`, `include/katra_meeting.h`
- **Ethical Framework**: `CLAUDE.md` - "No one owns an animate object"
- **Phase 6.1 Status**: Vector search partially complete, namespace isolation next priority
