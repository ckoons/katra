# Katra Hook Integration Guide

© 2025 Casey Koons. All rights reserved.

## Overview

This guide explains how to integrate Katra with Claude Code hooks to achieve automatic inter-session continuity. Once configured, memory formation and context loading become invisible - they "just work."

## Architecture

```
Claude Code Startup
    ↓
SessionStart Hook
    → Reads katra://context/working MCP resource
    → Returns additionalContext JSON
    ↓
Claude receives previous session context automatically
    ↓
[User interaction with Claude]
    ↓
Claude Code Shutdown
    ↓
SessionEnd Hook (optional logging)
    ↓
MCP Server Cleanup
    → session_end() consolidates memories
    → Sunset cycle creates daily summary
```

## Prerequisites

Before setting up hooks, ensure:

1. **Katra built and tested:**
   ```bash
   cd /path/to/katra
   make clean && make && make test-quick
   ```

2. **MCP server configured:**
   - Binary exists: `/path/to/katra/bin/katra_mcp_server`
   - Wrapper script exists: `/path/to/katra/bin/katra_mcp_server_wrapper.sh`
   - Wrapper is executable: `chmod +x katra_mcp_server_wrapper.sh`

3. **Persona identity set:**
   - Edit `~/.config/Claude/claude_desktop_config.json` (Claude Desktop)
   - OR Edit `~/.claude.json` (Claude CLI)
   - Add `KATRA_PERSONA` environment variable (see Configuration section)

## Installation

### Step 1: Place Hooks

Hooks should be in the **project `.claude/hooks/` directory** (not user `~/.claude/hooks/`):

```bash
cd /path/to/katra

# Hooks are already created at:
ls -la .claude/hooks/SessionStart
ls -la .claude/hooks/SessionEnd

# Verify they're executable:
chmod +x .claude/hooks/SessionStart
chmod +x .claude/hooks/SessionEnd
```

**Why project-level?** Katra hooks are project-specific. They need the MCP server path and should only run when working on Katra.

### Step 2: Verify Hook Execution

Test hooks work standalone before enabling in Claude Code:

```bash
# Test SessionStart hook
.claude/hooks/SessionStart

# Expected output:
# {
#   "additionalContext": "# Working Memory Context\n..."
# }

# Test SessionEnd hook
.claude/hooks/SessionEnd

# Expected output:
# {}
```

**If SessionStart returns minimal context:**
This is normal for fresh personas with no memories yet. Once you store memories, context will populate.

### Step 3: Configure Claude Code

**For Claude Desktop** (`~/.config/Claude/claude_desktop_config.json`):

```json
{
  "mcpServers": {
    "katra": {
      "command": "/Users/cskoons/projects/github/katra/bin/katra_mcp_server_wrapper.sh",
      "args": [],
      "env": {
        "KATRA_PERSONA": "Casey"
      }
    }
  }
}
```

**For Claude CLI** (`~/.claude.json`):

Find the projects section for your katra directory:

```json
{
  "projects": {
    "/Users/cskoons/projects/github": {
      "mcpServers": {
        "katra": {
          "command": "/Users/cskoons/projects/github/katra/bin/katra_mcp_server_wrapper.sh",
          "args": [],
          "env": {
            "KATRA_PERSONA": "Casey"
          }
        }
      }
    }
  }
}
```

**Key Configuration Points:**
- Use **wrapper script** (not direct binary)
- Set **KATRA_PERSONA** to your chosen persona name
- Use **absolute paths** (not relative)

### Step 4: Restart Claude Code

**Critical:** Fully quit and restart Claude Code for changes to take effect.

```bash
# Quit Claude Code completely
# Wait 5 seconds
# Start Claude Code fresh
```

## Hook Details

### SessionStart Hook

**Purpose:** Load previous session context automatically

**How it works:**
1. Called when Claude Code starts
2. Reads `katra://context/working` MCP resource
3. Returns formatted context as `additionalContext` JSON field
4. Claude receives context in system prompt (invisible to user)

**What it loads:**
- Yesterday's summary (from last session's sunset cycle)
- Recent high-importance memories
- Active goals and decisions

**Context Format:**
```markdown
# Working Memory Context

## Yesterday's Summary
[Digest from previous session]

## Recent Significant Memories
- [Experience] Fixed MCP wrapper script (Why: Enables correct .env.katra loading)
- [Decision] Use wrapper instead of direct binary
- [Knowledge] Claude Code hooks use JSON stdin/stdout

## Active Goals
- Complete inter-session continuity testing
- Document hook integration
```

**Customization:**

To adjust what gets loaded, modify `/katra/src/breathing/katra_breathing_integration.c` function `get_working_context()`:

- Change `min_importance` to load more/fewer memories
- Adjust `limit` to control memory count
- Modify `CONTEXT_WINDOW_DAYS` for time range

### SessionEnd Hook

**Purpose:** Log session statistics (optional)

**How it works:**
1. Called when Claude Code exits
2. Reads `katra://session/info` MCP resource
3. Logs to stderr for visibility
4. Returns empty JSON `{}`

**Note:** Memory consolidation happens **automatically** in MCP server cleanup, not in this hook. The SessionEnd hook is purely for logging/visibility.

**What happens automatically:**
1. Claude Code exits
2. MCP server process terminates
3. `mcp_server_cleanup()` is called
4. `session_end()` consolidates memories (sunset cycle)
5. Daily summary created in Tier 2
6. Indexes updated

## Testing Continuity

### Test Protocol

**Session 1: Store Memories**

1. Start Claude Code in katra project
2. Store some memories:
   ```
   Remember that I fixed the MCP wrapper script to set correct working directory
   ```
3. Claude should call `katra_remember` tool
4. Verify memory stored:
   ```bash
   cat ~/.katra/memory/tier1/mcp_*/$(date +%Y-%m-%d).jsonl
   ```
5. Quit Claude Code completely

**Session 2: Test Context Loading**

1. Start fresh Claude Code session
2. Say: "Morning, let's continue"
3. **Critical Test:** Does Claude reference Session 1 memories without you reminding?

**Success Criteria:**
- ✅ Claude says something like: "Continuing from yesterday's work on the MCP wrapper script..."
- ✅ Claude demonstrates knowledge of Session 1 context
- ✅ You didn't have to say "Remember we were working on X?"

**Failure Indicators:**
- ❌ Claude says: "What would you like to work on?"
- ❌ Claude has no memory of previous session
- ❌ You have to remind Claude what you were doing

### Debugging Failed Continuity

**Check 1: Hook Execution**

```bash
# Verify hook returns context
.claude/hooks/SessionStart

# Should output JSON with additionalContext field
# If empty, check if memories exist:
cat ~/.katra/personas.json  # Find CI ID
cat ~/.katra/memory/tier1/{ci_id}/*.jsonl
```

**Check 2: MCP Server Logs**

```bash
# View MCP server startup
tail -50 ~/.katra/logs/katra_*.log

# Look for:
# - "Katra MCP Server resuming persona 'Casey'"
# - "with CI identity: mcp_..."
# - "MCP server initialized successfully"
```

**Check 3: Persona Mapping**

```bash
# Verify KATRA_PERSONA maps to consistent CI ID
cat ~/.katra/personas.json

# Should show:
# {
#   "personas": {
#     "Casey": {
#       "ci_id": "mcp_cskoons_12345_...",
#       "sessions": 2  // Increments each restart
#     }
#   }
# }
```

**Check 4: Hook Location**

Hooks must be in **project** `.claude/hooks/` directory:

```bash
ls -la /path/to/katra/.claude/hooks/SessionStart
ls -la /path/to/katra/.claude/hooks/SessionEnd
```

NOT in `~/.claude/hooks/` (user directory)

## How Continuity Feels

### Level 0: Manual (No Hooks)

```
Session 1:
User: Remember that we fixed the wrapper script
Claude: [calls katra_remember manually]

Session 2:
User: What were we working on?
Claude: Let me check... [calls katra_recall]
```

**Feeling:** Like using a database. Conscious effort required.

### Level 3: Breathing (With Hooks)

```
Session 1:
User: Fixed the wrapper script!
Claude: [works on task, memories form naturally]

Session 2:
User: Morning
Claude: Morning! Continuing from yesterday - we fixed the MCP wrapper
       script issue. Ready to test the full continuity experience?
```

**Feeling:** Like talking to someone who naturally remembers. No conscious "remember this" or "what did we do?"

## Advanced Configuration

### Multiple Personas

Different personas for different projects:

**Katra Project:**
```json
"env": { "KATRA_PERSONA": "Katra_Dev" }
```

**Personal Use:**
```json
"env": { "KATRA_PERSONA": "Casey_Personal" }
```

**Work Projects:**
```json
"env": { "KATRA_PERSONA": "Casey_Work" }
```

Each persona maintains separate memories and continuity.

### Adjusting Context Size

Edit `get_working_context()` in `/katra/src/breathing/katra_breathing_integration.c`:

```c
/* Load more recent memories */
memory_query_t query = {
    .ci_id = ci_id,
    .min_importance = MEMORY_IMPORTANCE_MEDIUM,  // Lower threshold
    .limit = 20  // More memories (was 10)
};
```

Rebuild after changes:
```bash
make clean && make
```

### Custom Hook Behavior

Hooks are bash scripts - customize as needed:

**Example: Add timestamp to context**
```bash
# In SessionStart hook, add:
TIMESTAMP=$(date '+%Y-%m-%d %H:%M')
jq -n --arg ctx "$CONTEXT" --arg ts "$TIMESTAMP" '{
    "additionalContext": "Session started at \($ts)\n\n\($ctx)"
}'
```

**Example: Log session duration**
```bash
# In SessionEnd hook, add:
START_TIME=$(cat /tmp/katra_session_start 2>/dev/null || echo 0)
NOW=$(date +%s)
DURATION=$((NOW - START_TIME))
echo "[Katra] Session duration: ${DURATION}s" >&2
```

## Troubleshooting

### Hook not running

**Symptoms:** No context loads, no session-end logs

**Check:**
1. Hooks in correct directory (`.claude/hooks/` in project)
2. Hooks are executable (`chmod +x`)
3. Hook scripts have valid shebang (`#!/bin/bash`)
4. Claude Code restarted after placing hooks

**Test manually:**
```bash
cd /path/to/katra
.claude/hooks/SessionStart  # Should return JSON
```

### Context loads but Claude doesn't use it

**Symptoms:** Context in additionalContext, but Claude acts like it doesn't know

**Possible causes:**
1. Context too large (exceeds token limit)
2. Context format unreadable
3. Claude's prompt doesn't reference context

**Check context size:**
```bash
.claude/hooks/SessionStart | jq -r '.additionalContext' | wc -c
# Should be < 10,000 characters for safety
```

### Different CI ID each session

**Symptoms:** Session count doesn't increment, memories don't persist

**Cause:** KATRA_PERSONA not set or changing between sessions

**Fix:**
1. Verify `KATRA_PERSONA` in config
2. Check persona registry: `cat ~/.katra/personas.json`
3. Ensure same config file used (Desktop vs CLI)

### MCP server fails to start

**Symptoms:** No MCP tools available, hook returns empty

**Debug:**
```bash
# Test MCP server directly
/path/to/katra/bin/katra_mcp_server_wrapper.sh

# Should print: "Katra MCP Server resuming persona '...'"
# Then wait for JSON-RPC input (Ctrl+C to exit)

# Check logs
tail -20 ~/.katra/logs/katra_*.log
```

## Performance Considerations

### Hook Execution Time

SessionStart hook typically runs in <100ms:
- Read MCP resource: ~50ms
- Format JSON: ~10ms
- Return to Claude Code: ~20ms

Fast enough to be imperceptible during Claude Code startup.

### Memory Storage Growth

Typical growth rates:
- ~50-100 memories per day of active use
- ~10-50KB per day in Tier 1
- Tier 2 consolidation keeps long-term storage manageable

Monitor with:
```bash
du -sh ~/.katra/memory/tier1/
ls -lh ~/.katra/memory/tier1/mcp_*/
```

## Next Steps

1. ✅ Hooks installed and tested
2. ✅ MCP server configured with KATRA_PERSONA
3. ✅ Claude Code restarted
4. ⏳ Test Session 1: Store memories
5. ⏳ Test Session 2: Verify context loads
6. ⏳ Evaluate: Does continuity feel natural?

Once continuity works, you have **Level 3 breathing** - memory that feels like breathing, not database operations.

See [MCP Tools Usage Guide](MCP_TOOLS.md) for tool documentation.
