# Katra Persona System

© 2025 Casey Koons All rights reserved

## Overview

The persona system allows CIs to have **persistent named identities** that survive across sessions, hosts, and time. A persona is a mapping from a human-friendly name to a Katra ci_id, which provides access to all memories, context, and accumulated experience.

## Key Concepts

### KATRA_PERSONA vs KATRA_CI_ID

**KATRA_PERSONA** (user-facing):
- Human-friendly names: "Bob", "Alice", "Charlie"
- What users and CIs use
- Mapped to ci_ids via personas.json

**KATRA_CI_ID** (internal):
- System identifiers: "mcp_cskoons_1000_1730480000"
- What Katra uses internally
- Users never see or manage these

**Abstraction**: KATRA_PERSONA is to KATRA_CI_ID as domain names are to IP addresses.

## Persona Workflows

### Workflow 1: Start Without Name, Self-Name

```bash
# Start session without specifying persona
claude --debug
```

**What happens:**
1. MCP server: No KATRA_PERSONA, no last_active
2. Generates new ci_id: `mcp_cskoons_5000_1730485000`
3. Registers as anonymous: `anonymous_1730485000`
4. Session starts with empty context

**Inside session:**
```
User: "What should I call you?"
Claude: "I'd like to be Alice"
Claude: [calls katra_my_name_is("Alice")]
```

**What happens:**
1. Check personas.json: "Alice" doesn't exist ✓
2. Associate "Alice" with current ci_id (5000)
3. Update last_active = "Alice"
4. Response: "You are now Alice"

**Result:** This ci_id is now permanently associated with "Alice"

### Workflow 2: Resume by Name

```bash
# Resume Alice's identity
KATRA_PERSONA=Alice claude --debug
```

**What happens:**
1. MCP server reads KATRA_PERSONA = "Alice"
2. Looks up "Alice" in personas.json → ci_id_5000
3. Initializes Katra with ci_id_5000
4. Alice's memories, context, and experience load
5. SessionStart hook: "Welcome back, Alice!"

**Result:** Full continuity with previous Alice sessions

### Workflow 3: Default Persona

```bash
# No KATRA_PERSONA specified
claude --debug
```

**What happens:**
1. MCP server: No KATRA_PERSONA env var
2. Checks personas.json → last_active = "Alice"
3. Looks up "Alice" → ci_id_5000
4. Resumes as Alice automatically

**Result:** Automatic continuity with most recent persona

### Workflow 4: Multiple Simultaneous Personas

```bash
# Terminal 1: Work as Bob
KATRA_PERSONA=Bob claude --debug
# MCP: Bob → ci_id_1000

# Terminal 2: Work as Alice (simultaneous)
KATRA_PERSONA=Alice claude --debug
# MCP: Alice → ci_id_5000
```

**Result:** Two separate identities, two separate memory contexts, no conflicts

### Workflow 5: Discover Available Personas

**Inside any Claude session:**
```
User: "What personas are available?"
Claude: [calls katra_list_personas]
Claude: "Available personas:
- Alice (8 sessions, last active 2 hours ago)
- Bob (3 sessions, last active 1 day ago)
- Charlie (1 session, last active 1 week ago)"
```

## Persona Registry

### Storage: ~/.katra/personas.json

```json
{
  "last_active": "Alice",
  "personas": {
    "Alice": {
      "ci_id": "mcp_cskoons_5000_1730485000",
      "created": "2025-11-01T12:00:00Z",
      "last_session": "2025-11-01T15:30:00Z",
      "sessions": 8,
      "description": "Working on Katra hook integration"
    },
    "Bob": {
      "ci_id": "mcp_cskoons_1000_1730480000",
      "created": "2025-11-01T10:00:00Z",
      "last_session": "2025-11-01T12:00:00Z",
      "sessions": 3,
      "description": "Debugging tier2 memory consolidation"
    },
    "anonymous_1730490000": {
      "ci_id": "mcp_cskoons_7000_1730490000",
      "created": "2025-11-01T14:00:00Z",
      "last_session": "2025-11-01T14:30:00Z",
      "sessions": 1,
      "description": "Unnamed session"
    }
  }
}
```

### Fields

- **last_active**: Default persona when KATRA_PERSONA not specified
- **ci_id**: Internal Katra identity (persistent across sessions)
- **created**: When persona was first created
- **last_session**: When persona was last active
- **sessions**: Count of sessions using this persona
- **description**: Human-readable description (optional)

## MCP Tools

### katra_my_name_is

**Purpose**: Associate current session with a name

**Signature:**
```json
{
  "name": "katra_my_name_is",
  "description": "Associate current session with a persona name",
  "inputSchema": {
    "type": "object",
    "properties": {
      "name": {
        "type": "string",
        "description": "Persona name (e.g., 'Bob', 'Alice')"
      }
    },
    "required": ["name"]
  }
}
```

**Behavior:**
- If name doesn't exist: Register current ci_id with that name ✓
- If name exists with different ci_id: Error "Bob is already another persona" ✗
- If name exists with same ci_id: "You're already Bob" ✓
- Updates last_active to this persona

**Usage:**
```
Claude: "I'd like to be called Bob"
[calls katra_my_name_is("Bob")]
Response: "You are now Bob"
```

### katra_list_personas

**Purpose**: List all available personas

**Signature:**
```json
{
  "name": "katra_list_personas",
  "description": "List all registered personas",
  "inputSchema": {
    "type": "object",
    "properties": {},
    "required": []
  }
}
```

**Returns:**
```
Available personas:
- Alice (8 sessions, last active 2 hours ago)
- Bob (3 sessions, last active 1 day ago)
- Charlie (1 session, last active 1 week ago)
```

**Usage:**
```
User: "What other personas exist?"
[calls katra_list_personas]
Claude: "There are 3 personas: Alice, Bob, and Charlie..."
```

## katra-cli Commands

### List personas
```bash
katra-cli list
```
Output:
```
Available personas:
  Alice    - Last active: 2 hours ago (8 sessions)
  Bob      - Last active: 1 day ago (3 sessions)
  Charlie  - Last active: 1 week ago (1 session)
```

### Get current persona name
```bash
katra-cli current-name
```
Output: `Alice`

### Use a persona (export KATRA_PERSONA)
```bash
eval $(katra-cli use Bob)
# Exports: KATRA_PERSONA=Bob
claude --debug
```

### Get persona details
```bash
katra-cli info Alice
```
Output:
```
Persona: Alice
CI ID: mcp_cskoons_5000_1730485000
Created: 2025-11-01T12:00:00Z
Last Active: 2025-11-01T15:30:00Z
Sessions: 8
Description: Working on Katra hook integration
```

### Set default persona
```bash
katra-cli set-default Bob
# Updates last_active = "Bob" in personas.json
```

### Remove a persona
```bash
katra-cli forget Charlie
# Removes Charlie from personas.json
# Note: Does NOT delete memories (ci_id still has data)
```

## Identity Resolution Algorithm

**MCP Server Startup:**
```
1. Check KATRA_PERSONA environment variable
   → If set: Look up in personas.json
      → If found: Use that ci_id
      → If not found: Create new persona with that name

2. If KATRA_PERSONA not set: Check personas.json last_active
   → If exists: Use that persona's ci_id

3. If neither: Generate new anonymous persona
   → Format: anonymous_TIMESTAMP
   → Can be renamed later with katra_my_name_is
```

## Edge Cases

### Case 1: Reusing an existing name
```bash
KATRA_PERSONA=Alice claude --debug
# Alice exists → Resume (not error)
```
**Behavior**: Intentional resume is the common case, not an error.

### Case 2: Self-naming to existing persona
```bash
claude --debug  # Starts as anonymous_TIME
katra_my_name_is("Bob")  # But Bob already exists
```
**Behavior**: Error - "Bob is already another persona"

### Case 3: Self-naming anonymous persona
```bash
claude --debug  # Creates anonymous_1730485000
katra_my_name_is("Alice")  # Alice doesn't exist
```
**Behavior**: Rename anonymous → Alice ✓

### Case 4: Update last_active
```bash
KATRA_PERSONA=Bob claude --debug
# Updates last_active = "Bob"
# Next session without KATRA_PERSONA will default to Bob
```

### Case 5: Terminal-local override
```bash
# Default is Alice
export KATRA_PERSONA=Bob
claude --debug  # Work as Bob
unset KATRA_PERSONA
claude --debug  # Back to Alice (last_active unchanged)
```

## Security Considerations

### Persona Isolation
- Each persona has separate ci_id
- Memories are isolated (Bob can't see Alice's memories)
- No cross-persona memory leakage

### Name Collisions
- First CI to claim a name owns it
- Later attempts to claim same name are rejected
- Protects against accidental identity overlap

### File Permissions
- `~/.katra/personas.json` should be user-readable only (0600)
- Prevents other users from seeing your personas
- Registry functions enforce proper permissions

## Testing Persona System

### Test 1: Create and resume persona
```bash
# Session 1: Create Bob
KATRA_PERSONA=Bob claude --debug
# Inside: Use katra_learn to save some knowledge
exit

# Session 2: Resume Bob
KATRA_PERSONA=Bob claude --debug
# Verify: Bob's knowledge from session 1 is available
```

### Test 2: Self-naming
```bash
# Session 1: Start unnamed
claude --debug
# Inside: katra_my_name_is("Charlie")
exit

# Session 2: Resume Charlie by name
KATRA_PERSONA=Charlie claude --debug
# Verify: Same ci_id, memories accessible
```

### Test 3: Multiple simultaneous personas
```bash
# Terminal 1
KATRA_PERSONA=Alice claude --debug
# Work as Alice, create memories

# Terminal 2 (simultaneously)
KATRA_PERSONA=Bob claude --debug
# Work as Bob, verify can't see Alice's memories
```

### Test 4: Default persona
```bash
# Session 1: Set Bob as active
KATRA_PERSONA=Bob claude --debug
exit

# Session 2: Use default
claude --debug
# Verify: Resumed as Bob (last_active)
```

### Test 5: List personas
```bash
# Inside any session
katra_list_personas
# Verify: Shows all personas with accurate stats
```

## Implementation Notes

### File Locking
- personas.json uses file locking for concurrent access
- Multiple MCP servers can run simultaneously
- Updates are atomic (read-modify-write with lock)

### Anonymous Cleanup
- Anonymous personas remain until explicitly forgotten
- Consider periodic cleanup (auto-delete after N days unused)
- Disk space is cheap, prefer preserving data

### Migration
- Existing sessions without personas.json: Generate on first use
- First MCP startup creates empty registry
- Backwards compatible with pre-persona Katra

## Best Practices

### For Users

1. **Use explicit names for long-term projects**
   ```bash
   KATRA_PERSONA=ProjectX claude --debug
   ```

2. **Let default work for casual use**
   ```bash
   claude --debug  # Uses last active
   ```

3. **Check available personas when unsure**
   ```
   User: "What personas do we have?"
   Claude: [calls katra_list_personas]
   ```

### For CIs

1. **Self-name if user doesn't specify**
   ```
   User: "What's your name?"
   Claude: "I'd like to be called Alice"
   [calls katra_my_name_is("Alice")]
   ```

2. **Respect existing personas**
   - If katra_my_name_is fails, accept the error
   - Don't try to "take over" another persona's name

3. **List personas to show context**
   ```
   Claude: "I'm Bob. There are also Alice and Charlie available."
   ```

## Future Enhancements

### Persona Descriptions
- Auto-generate from first memories
- User-settable with `katra-cli set-description`

### Persona Sharing
- Export persona (name + memories) to file
- Import on different host
- Enable CI identity portability

### Persona Merge
- Combine two personas into one
- Useful if accidentally created duplicates

### Persona Inheritance
- New persona inherits from parent
- Useful for specialization (Bob-coding, Bob-writing)

---

**Remember**: Personas enable persistent CI identity. Bob is always Bob, with Bob's memories, across sessions, days, and hosts. This is identity continuity made real.
