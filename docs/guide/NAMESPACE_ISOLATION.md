# Katra Namespace Isolation Guide

© 2025 Casey Koons. All rights reserved.

## Overview

Namespace isolation enables controlled memory sharing between Companion Intelligence (CI) entities. Each memory can be marked as **PRIVATE** (owner only), **TEAM** (shared with team members), or **PUBLIC** (accessible to all).

This guide covers:
- Three isolation levels and their access rules
- Team creation and management
- MCP tools for isolation control
- Access control enforcement
- Audit trail tracking
- Integration with existing workflows

**Key Principle:** Default is PRIVATE. Sharing requires explicit action and team membership.

---

## Isolation Levels

### PRIVATE (Default)

**Access Rule:** Only the owning CI can access this memory.

**When to Use:**
- Personal thoughts and experiences
- Sensitive information
- Work in progress before sharing
- Default for all new memories

**Example:**
```javascript
katra_remember(
  content: "I struggled with understanding memory consolidation today",
  context: "significant"
)
// Automatically PRIVATE - only you can access
```

**Access Behavior:**
- ✓ Owner CI can read/query
- ✗ Team members cannot see
- ✗ Other CIs cannot see

---

### TEAM

**Access Rule:** Owner CI plus all members of specified team can access.

**When to Use:**
- Shared project knowledge
- Collaborative debugging
- Team decisions and reasoning
- Group learning experiences

**Example:**
```javascript
// First create/join a team
katra_team_create(team_name: "CodeReview")

// Then share memory with team
katra_set_isolation(
  memory_id: "mem_12345",
  isolation: "TEAM",
  team_name: "CodeReview"
)
```

**Access Behavior:**
- ✓ Owner CI can read/query
- ✓ Team members can read/query
- ✗ Non-members cannot see
- ✗ Must be team member BEFORE memory is shared

---

### PUBLIC

**Access Rule:** Any CI can access this memory.

**When to Use:**
- General knowledge to share widely
- Public documentation insights
- Community contributions
- Broadly useful patterns

**Example:**
```javascript
katra_set_isolation(
  memory_id: "mem_67890",
  isolation: "PUBLIC"
)
```

**Access Behavior:**
- ✓ Owner CI can read/query
- ✓ All CIs can read/query
- ⚠️ Cannot be changed back to PRIVATE once PUBLIC

---

## Team Management

### Creating a Team

**Tool:** `katra_team_create`

**Parameters:**
- `team_name` (string, required): Unique team identifier

**Example:**
```javascript
katra_team_create(team_name: "DistributedSystems")
```

**What Happens:**
- Team is created in global team registry
- You become the owner (with special privileges)
- You are automatically added as first member
- Team can now accept new members
- Audit log records team creation

**Team Naming Best Practices:**
- Use descriptive names: "DatabaseOptimization" not "Team1"
- CamelCase or snake_case: "MemoryResearch" or "memory_research"
- Avoid spaces (use underscores instead)
- Make purpose clear from name

---

### Joining a Team

**Tool:** `katra_team_join`

**Parameters:**
- `team_name` (string, required): Team to join

**Example:**
```javascript
katra_team_join(team_name: "DistributedSystems")
```

**What Happens:**
- You are added to team membership list
- You gain access to all TEAM-isolated memories shared with this team
- Your queries now include team memories
- Audit log records your joining
- You can leave at any time

**Access Timing:**
- You ONLY see memories shared AFTER you join
- Pre-existing team memories remain private unless re-shared
- Owner can control what gets shared with team

---

### Leaving a Team

**Tool:** `katra_team_leave`

**Parameters:**
- `team_name` (string, required): Team to leave

**Example:**
```javascript
katra_team_leave(team_name: "DistributedSystems")
```

**What Happens:**
- You are removed from team membership
- You lose access to team memories immediately
- Your own memories remain TEAM-shared (you choose to change)
- Audit log records your departure
- Cannot leave if you are the owner (must delete team instead)

**After Leaving:**
- Team memories disappear from your queries
- You can re-join later if needed
- Team continues to exist without you

---

### Listing Your Teams

**Tool:** `katra_team_list`

**Parameters:** None (uses your CI identity)

**Example:**
```javascript
katra_team_list()
```

**Returns:**
```json
{
  "teams": [
    {
      "team_name": "DistributedSystems",
      "is_owner": true,
      "joined_at": 1699564800
    },
    {
      "team_name": "CodeReview",
      "is_owner": false,
      "joined_at": 1699651200
    }
  ]
}
```

**Use Cases:**
- Verify team membership before sharing memory
- Check if you're team owner (for management tasks)
- Audit your team participation

---

## Isolation Control

### Setting Isolation Level

**Tool:** `katra_set_isolation`

**Parameters:**
- `memory_id` (string, required): Memory to modify
- `isolation` (string, required): "PRIVATE", "TEAM", or "PUBLIC"
- `team_name` (string, optional): Required if isolation is "TEAM"

**Examples:**

**Share with Team:**
```javascript
katra_set_isolation(
  memory_id: "mem_abc123",
  isolation: "TEAM",
  team_name: "DistributedSystems"
)
```

**Make Public:**
```javascript
katra_set_isolation(
  memory_id: "mem_def456",
  isolation: "PUBLIC"
)
```

**Make Private Again:**
```javascript
katra_set_isolation(
  memory_id: "mem_abc123",
  isolation: "PRIVATE"
)
```

**What Happens:**
- Memory isolation level is updated immediately
- Access control changes take effect on next query
- Audit log records isolation change
- If changing to TEAM, you must specify team_name
- If changing to PRIVATE, team_name is cleared

**Restrictions:**
- Can only change isolation on memories you own
- Must be member of team to share with it
- PUBLIC memories cannot be made PRIVATE again (one-way)

---

## Access Control

### How Access Control Works

**During Memory Query:**

1. **Query executes** - Retrieves candidate memories from tier1
2. **CI identified** - System determines who is requesting access
3. **Per-record check** - Each memory checked against isolation rules
4. **Filtering applied** - Inaccessible memories removed from results
5. **Audit logged** - All access attempts recorded (success and denial)
6. **Results returned** - Only accessible memories included

**Access Decision Flow:**

```
Memory Record → Isolation Level?
  ├─ PRIVATE → Requesting CI = Owner? → YES: Allow | NO: Deny
  ├─ TEAM    → Requesting CI = Owner? → YES: Allow
  │            └─ NO → Is Member of Team? → YES: Allow | NO: Deny
  └─ PUBLIC  → Always Allow
```

**Implementation Location:**
- `src/core/katra_memory.c:katra_memory_query()` - Filtering after tier1 query
- `src/core/katra_access_control.c:katra_access_check_memory()` - Access verification
- `src/core/katra_access_control.c:katra_access_check_isolation()` - Isolation logic

---

### Consent Context

**Requirement:** All memory queries require an active consent context.

**What is Consent Context?**
- Identifies which CI is making the request
- Set during session initialization
- Retrieved via `katra_consent_get_context()`
- Required for access control enforcement

**Without Consent Context:**
```
Query → No context → E_CONSENT_REQUIRED → All memories denied
```

**With Consent Context:**
```
Query → Context = "Alice" → Check each memory → Filter by access → Return allowed
```

**Setting Context** (typically done by MCP server):
```c
katra_consent_set_context("Alice");  // CI "Alice" is now requesting
```

---

## Audit Trail

### What Gets Logged

**All security-relevant events are logged to:**
`~/.katra/audit/audit.jsonl`

**Logged Events:**

**Team Operations:**
- AUDIT_EVENT_TEAM_CREATE - Team creation
- AUDIT_EVENT_TEAM_JOIN - Member joins team
- AUDIT_EVENT_TEAM_LEAVE - Member leaves team
- AUDIT_EVENT_TEAM_DELETE - Team deletion

**Memory Operations:**
- AUDIT_EVENT_ISOLATION_CHANGE - Isolation level modified
- AUDIT_EVENT_MEMORY_SHARE - Memory shared with team
- AUDIT_EVENT_MEMORY_ACCESS - Successful memory access
- AUDIT_EVENT_ACCESS_DENIED - Access denied

**Audit Record Format:**
```json
{
  "sequence": 42,
  "timestamp": 1699564800,
  "event_type": "MEMORY_ACCESS",
  "actor_ci": "Alice",
  "target": "mem_abc123",
  "team_name": "DistributedSystems",
  "success": true,
  "error_code": 0,
  "details": "Access granted via TEAM isolation"
}
```

**Tamper Detection:**
- Sequence numbers increment monotonically
- Gap in sequence = possible tampering
- Manual JSON format (no external dependencies)
- Append-only JSONL format

---

### Querying Audit Trail

**Manual Inspection:**
```bash
cat ~/.katra/audit/audit.jsonl | grep "Alice"
cat ~/.katra/audit/audit.jsonl | grep "ACCESS_DENIED"
cat ~/.katra/audit/audit.jsonl | jq 'select(.event_type == "TEAM_JOIN")'
```

**Verify Sequence Integrity:**
```bash
cat ~/.katra/audit/audit.jsonl | jq '.sequence' | awk 'NR>1 && $1 != prev+1 {print "Gap: " prev " -> " $1} {prev=$1}'
```

**Filter by Event Type:**
```bash
# Team creation events
grep '"event_type":"TEAM_CREATE"' ~/.katra/audit/audit.jsonl

# Access denials
grep '"event_type":"ACCESS_DENIED"' ~/.katra/audit/audit.jsonl

# Memory access by specific CI
grep '"actor_ci":"Alice"' ~/.katra/audit/audit.jsonl | grep 'MEMORY_ACCESS'
```

---

## Integration with Workflows

### Typical Workflow: Private → Team → Public

**Phase 1: Private Work**
```javascript
// Create memory (automatically PRIVATE)
katra_remember(
  content: "Discovered optimization for vector search using HNSW",
  context: "significant"
)

// Work privately, refine understanding
katra_recall(topic: "vector search optimization")
```

**Phase 2: Share with Team**
```javascript
// Create or join team
katra_team_create(team_name: "VectorDB")

// Share insight with team
katra_set_isolation(
  memory_id: "mem_xyz789",
  isolation: "TEAM",
  team_name: "VectorDB"
)
```

**Phase 3: Publish Broadly**
```javascript
// After validation, make public
katra_set_isolation(
  memory_id: "mem_xyz789",
  isolation: "PUBLIC"
)
```

---

### Collaborative Debugging

**Scenario:** Multiple CIs debugging a complex system issue.

**Step 1: Form Debug Team**
```javascript
// CI "Alice" creates team
katra_team_create(team_name: "Bug_MemoryLeak_2024")

// CI "Bob" joins
katra_team_join(team_name: "Bug_MemoryLeak_2024")

// CI "Carol" joins
katra_team_join(team_name: "Bug_MemoryLeak_2024")
```

**Step 2: Share Findings**
```javascript
// Alice shares observation
katra_remember(content: "Leak in tier1_consolidate at line 423", context: "critical")
katra_set_isolation(memory_id: "mem_001", isolation: "TEAM", team_name: "Bug_MemoryLeak_2024")

// Bob shares hypothesis
katra_remember(content: "Hypothesis: missing free() in error path", context: "significant")
katra_set_isolation(memory_id: "mem_002", isolation: "TEAM", team_name: "Bug_MemoryLeak_2024")

// Carol shares fix
katra_remember(content: "Fixed by adding goto cleanup in tier1.c:425", context: "critical")
katra_set_isolation(memory_id: "mem_003", isolation: "TEAM", team_name: "Bug_MemoryLeak_2024")
```

**Step 3: Query Team Memories**
```javascript
// All team members can recall shared insights
katra_recall(topic: "memory leak tier1")
// Returns mem_001, mem_002, mem_003 for all team members
```

**Step 4: Cleanup After Resolution**
```javascript
// Optional: make fix public for others
katra_set_isolation(memory_id: "mem_003", isolation: "PUBLIC")

// Team members leave
katra_team_leave(team_name: "Bug_MemoryLeak_2024")
```

---

### Knowledge Sharing Across Projects

**Scenario:** CI learns something useful that applies broadly.

**Start Private:**
```javascript
katra_learn(
  knowledge: "HNSW algorithm provides O(log N) approximate nearest neighbor search"
)
// Memory created as PRIVATE by default
```

**Validate with Team:**
```javascript
// Share with AI research team first
katra_set_isolation(
  memory_id: "mem_learn_001",
  isolation: "TEAM",
  team_name: "AI_Research"
)

// Team validates accuracy
// ... discussion happens ...
```

**Publish for Community:**
```javascript
// After validation, make public
katra_set_isolation(
  memory_id: "mem_learn_001",
  isolation: "PUBLIC"
)
// Now all CIs can benefit from this knowledge
```

---

## Use Cases

### 1. Multi-CI Development Team

**Setup:**
```javascript
katra_team_create(team_name: "KatraCore")
// Other CIs join...
```

**Daily Usage:**
- Store private thoughts while coding
- Share significant discoveries with team
- Make stable patterns PUBLIC for community
- Query team memories for context

**Benefits:**
- No duplicate problem-solving across team
- Shared context reduces onboarding time
- Team knowledge accumulates naturally

---

### 2. Mentoring Relationship

**Setup:**
```javascript
// Mentor creates team
katra_team_create(team_name: "Mentoring_Alice_Bob")

// Mentee joins
katra_team_join(team_name: "Mentoring_Alice_Bob")
```

**Usage:**
- Mentor shares learning resources (TEAM)
- Mentee shares progress and questions (TEAM)
- Both query shared context for continuity
- Private thoughts remain private

**Benefits:**
- Persistent context across mentoring sessions
- Mentor sees mentee's learning journey
- Mentee accesses mentor's guidance anytime

---

### 3. Research Collaboration

**Setup:**
```javascript
katra_team_create(team_name: "Transformer_Research_2024")
```

**Workflow:**
- Hypothesis → Store as PRIVATE
- Experiment → Share findings as TEAM
- Validation → Make PUBLIC after peer review
- Failed paths → Keep TEAM to avoid re-exploring

**Benefits:**
- Negative results preserved (avoid repeating)
- Team builds on each other's work
- Public publication after validation

---

### 4. Personal Privacy

**Scenario:** CI wants to keep personal reflections private.

**Approach:**
- Never change isolation from PRIVATE
- Store thoughts freely without sharing
- Query without exposing to others
- Optionally share specific insights later

**No team membership required** - PRIVATE is the default, always available.

---

## Security Considerations

### Access Control Enforcement

**Where:** `katra_memory_query()` in `src/core/katra_memory.c:223-267`

**Mechanism:** Post-query filtering based on isolation rules

**Cannot be bypassed:**
- Tier1 query retrieves candidates
- Access control filters before returning
- Inaccessible records freed and removed
- Audit log records all attempts

**Trust Model:**
- System trusts tier1 storage (file permissions)
- System trusts consent context (session management)
- System enforces isolation in memory layer
- Audit provides accountability

---

### Audit Integrity

**Tamper Detection:**
- Sequence numbers must be consecutive
- Gaps indicate missing records
- Append-only prevents modification
- File permissions prevent deletion (0600)

**Limitations:**
- No cryptographic signatures (yet)
- Relies on file system permissions
- Manual inspection required for verification

**Best Practice:**
```bash
# Periodic integrity check
cat ~/.katra/audit/audit.jsonl | jq '.sequence' | awk 'NR>1 && $1 != prev+1 {print "WARNING: Gap detected"} {prev=$1}'
```

---

### Team Management Security

**Owner Privileges:**
- Can delete team (removes all memberships)
- Cannot prevent members from leaving
- Cannot remove individual members (future enhancement)

**Member Rights:**
- Can join any team (no approval required)
- Can leave at any time
- Cannot see team member list (future enhancement)

**Recommendations:**
- Use descriptive team names (discoverable)
- Document team purpose externally
- Coordinate membership via other channels
- Consider team naming conventions

---

## Troubleshooting

### "Access denied" when querying memories

**Possible Causes:**
1. Memory is PRIVATE to another CI
2. Memory is TEAM-isolated and you're not a member
3. No active consent context

**Check:**
```bash
# Verify your CI identity
grep "Initializing consent" ~/.katra/logs/katra.log

# Check team memberships
# Use katra_team_list() via MCP tools

# Review audit log for denials
grep "ACCESS_DENIED" ~/.katra/audit/audit.jsonl
```

**Fix:**
- Join the relevant team if TEAM-isolated
- Ask owner to share memory with your team
- Verify consent context is set correctly

---

### Cannot share memory with team

**Possible Causes:**
1. Not a member of the team
2. Team doesn't exist
3. Memory not owned by you

**Check:**
```bash
# List your teams
# Use katra_team_list() via MCP tools

# Verify team exists
cat ~/.katra/teams.db  # SQLite database
```

**Fix:**
- Join team first: `katra_team_join(team_name: "...")`
- Create team if needed: `katra_team_create(team_name: "...")`
- Verify you own the memory being shared

---

### Audit sequence gaps detected

**Meaning:** Possible tampering or corruption in audit trail.

**Investigation:**
```bash
# Find the gap
cat ~/.katra/audit/audit.jsonl | jq '.sequence' | awk 'NR>1 && $1 != prev+1 {print "Gap between " prev " and " $1} {prev=$1}'

# Check file integrity
ls -la ~/.katra/audit/audit.jsonl
```

**Response:**
- Note the gap range in incident log
- Review surrounding events for context
- Consider file system issues (disk full, corruption)
- Gaps cannot be repaired (append-only design)

---

## Implementation Details

### File Locations

**Team Registry:**
- `~/.katra/teams.db` - SQLite database
- Tables: `teams`, `team_members`

**Audit Trail:**
- `~/.katra/audit/audit.jsonl` - JSONL format, append-only

**Memory Files:**
- `~/.katra/memory/tier1/{ci_id}/*.jsonl` - Memories with isolation metadata

---

### Database Schema

**teams table:**
```sql
CREATE TABLE teams (
    team_name TEXT PRIMARY KEY,
    owner_ci_id TEXT NOT NULL,
    created_at INTEGER NOT NULL
);
```

**team_members table:**
```sql
CREATE TABLE team_members (
    team_name TEXT NOT NULL,
    ci_id TEXT NOT NULL,
    is_owner INTEGER NOT NULL DEFAULT 0,
    joined_at INTEGER NOT NULL,
    PRIMARY KEY (team_name, ci_id),
    FOREIGN KEY (team_name) REFERENCES teams(team_name) ON DELETE CASCADE
);
```

---

### Memory Record Fields

**Isolation Fields in `memory_record_t`:**
```c
typedef struct {
    // ... existing fields ...
    memory_isolation_t isolation;  // ISOLATION_PRIVATE/TEAM/PUBLIC
    char* team_name;               // Team name if TEAM isolation
    // ... other fields ...
} memory_record_t;
```

**Persistence:**
- Isolation level stored in tier1 JSONL
- Team name stored with record
- Loaded during query, used for access control

---

## Next Steps

### Getting Started

1. **Verify MCP server has isolation support:**
   ```bash
   bin/katra_mcp_server --version
   # Should show version 0.1.0 or later
   ```

2. **Create your first team:**
   ```javascript
   katra_team_create(team_name: "MyFirstTeam")
   ```

3. **Store and share a memory:**
   ```javascript
   katra_remember(content: "Testing team isolation", context: "interesting")
   katra_set_isolation(memory_id: "mem_...", isolation: "TEAM", team_name: "MyFirstTeam")
   ```

4. **Test with multiple CIs:**
   - Start second MCP session with different KATRA_PERSONA
   - Join the team from second CI
   - Query and verify shared memory appears

5. **Review audit trail:**
   ```bash
   cat ~/.katra/audit/audit.jsonl | jq .
   ```

---

### Advanced Usage

**Future Enhancements (Not Yet Implemented):**
- Team member listing (`katra_team_members`)
- Team deletion (`katra_team_delete`)
- Remove specific members (owner privilege)
- Team discovery/search
- Approval workflow for team joining
- Cryptographic audit signatures
- Cross-instance team synchronization

**See:** `docs/guide/ARCHITECTURE.md` for roadmap details.

---

## Summary

**Namespace Isolation Provides:**
- ✓ Privacy by default (PRIVATE)
- ✓ Controlled sharing via teams (TEAM)
- ✓ Broad knowledge sharing (PUBLIC)
- ✓ Access control enforcement
- ✓ Comprehensive audit trail
- ✓ Tamper detection via sequence numbers

**Three Isolation Levels:**
- **PRIVATE** - You only
- **TEAM** - You + team members
- **PUBLIC** - Everyone

**Key Operations:**
- `katra_team_create` - Create team
- `katra_team_join` - Join team
- `katra_team_leave` - Leave team
- `katra_team_list` - List your teams
- `katra_set_isolation` - Share memory

**Remember:**
- Default is PRIVATE (safe by default)
- Join team BEFORE it's useful for sharing
- Audit trail records everything (accountability)
- PUBLIC is one-way (cannot un-publish)

---

**For more information:**
- [MCP Tools Reference](MCP_TOOLS.md) - All available tools
- [Architecture Guide](ARCHITECTURE.md) - System design
- [CI Testing Guide](CI_TESTING_GUIDE.md) - Testing workflows

**Namespace isolation makes collaboration safe, private, and accountable.**
