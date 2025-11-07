# Katra MCP Tools Usage Guide

© 2025 Casey Koons. All rights reserved.

## Overview

The Katra MCP server provides 9 tools for memory formation and retrieval. These tools integrate with Claude Code to enable persistent inter-session continuity.

## Available Tools

### Memory Formation Tools

#### `katra_remember` - Store a memory

Store a thought or experience with natural language importance.

**Parameters:**
- `content` (string, required): The thought or experience to remember
- `context` (string, required): Why this is important - use natural language like:
  - `"trivial"` - Fleeting thought, will fade
  - `"interesting"` - Worth remembering
  - `"significant"` - Important insight or event
  - `"critical"` - Must never forget

**Example Usage:**
```javascript
katra_remember(
  content: "Fixed MCP wrapper script to set correct working directory",
  context: "significant"
)
```

**When to Use:**
- After solving a problem
- When learning something new
- After making an important decision
- When discovering a pattern

**What Makes Good Memory Content:**
- **Specific**: "Fixed bug in tier1.c:95 causing memory leak" ✓
- **Not vague**: "Fixed a bug" ✗
- **Context-rich**: "MCP server needs .env.katra in project root" ✓
- **Not just data**: "Error code 1002" ✗

---

#### `katra_learn` - Store new knowledge

Store factual knowledge you've learned.

**Parameters:**
- `knowledge` (string, required): The knowledge to store

**Example Usage:**
```javascript
katra_learn(
  knowledge: "Claude Code hooks use JSON input/output via stdin/stdout"
)
```

**When to Use:**
- After understanding how something works
- When learning API semantics
- After reading documentation
- When discovering system behavior

**Good Knowledge vs Experience:**
- Knowledge: "SessionStart hooks can return additionalContext" ✓
- Experience: "I implemented SessionStart hook today" (use `katra_remember` instead)

---

#### `katra_decide` - Store a decision with reasoning

Store important decisions with the reasoning behind them.

**Parameters:**
- `decision` (string, required): The decision made
- `reasoning` (string, required): Why this decision was made

**Example Usage:**
```javascript
katra_decide(
  decision: "Use wrapper script instead of direct MCP server binary",
  reasoning: "Wrapper ensures correct working directory for .env.katra access"
)
```

**When to Use:**
- After choosing between alternatives
- When making architectural decisions
- After resolving design debates
- When establishing patterns/conventions

**Capture BOTH What and Why:**
The `reasoning` field is critical - it preserves the **why** behind decisions, enabling future you (or other CIs) to understand context.

---

### Memory Retrieval Tools

#### `katra_recall` - Find memories about a topic

Search for memories related to a specific topic or keyword.

**Parameters:**
- `topic` (string, required): The topic to search for

**Example Usage:**
```javascript
katra_recall(topic: "MCP server wrapper")
```

**Returns:**
List of memories containing the topic keywords, ordered by relevance.

**Search Tips:**
- Use specific keywords: "tier1 consolidation" vs "memory"
- Combine multiple keywords: "hook SessionStart implementation"
- Try different phrasings if first search doesn't find what you need

---

### Nous Tools (AI-Assisted Guidance)

#### `katra_placement` - Architecture guidance

Ask where code should be placed in the codebase.

**Parameters:**
- `query` (string, required): The placement question

**Example Usage:**
```javascript
katra_placement(
  query: "Where should hook integration code go?"
)
```

**Returns:**
Architectural recommendation with confidence level.

**When to Use:**
- Before creating new files
- When refactoring existing code
- When uncertain about module boundaries
- When following existing patterns

---

#### `katra_impact` - Dependency analysis

Analyze what breaks if you change something.

**Parameters:**
- `query` (string, required): The impact question

**Example Usage:**
```javascript
katra_impact(
  query: "What breaks if I change session_end() signature?"
)
```

**Returns:**
Impact analysis showing affected code and dependencies.

**When to Use:**
- Before making breaking changes
- When refactoring interfaces
- When deprecating functions
- When considering API changes

---

#### `katra_user_domain` - User understanding

Understand user domain and feature usage patterns.

**Parameters:**
- `query` (string, required): The user domain question

**Example Usage:**
```javascript
katra_user_domain(
  query: "Who would use inter-session continuity?"
)
```

**Returns:**
Analysis of user needs, use cases, and expected behavior.

**When to Use:**
- When designing user-facing features
- Before making UX decisions
- When prioritizing features
- When understanding requirements

---

### Resource Tools

#### `katra://context/working` - Get working context

MCP resource (not a tool) that returns formatted context for system prompts.

Contains:
- Yesterday's summary (from sunrise)
- Recent high-importance memories
- Active goals and decisions

**Accessed via:** `resources/read` MCP method with URI `katra://context/working`

**Used by:** SessionStart hook to inject context automatically

---

#### `katra://session/info` - Get session info

MCP resource providing current session state:
- CI identity
- Session ID
- Memories added this session
- Queries processed
- Session duration

**Accessed via:** `resources/read` MCP method with URI `katra://session/info`

**Used by:** SessionEnd hook for logging (optional)

---

## Memory Formation Best Practices

### Importance Levels Guide

**Trivial** (`"trivial"`):
- Typo fixes
- Formatting changes
- Temporary debugging output
- **OK to forget** quickly

**Interesting** (`"interesting"`):
- Useful patterns observed
- Minor optimizations
- Code improvements
- **Worth remembering** for a while

**Significant** (`"significant"`):
- Bug fixes
- Feature implementations
- Design decisions
- **Important to retain** long-term

**Critical** (`"critical"`):
- Architectural decisions
- Breaking changes
- Security issues
- **Must never forget**

### When to Store Memories

**DO store:**
- ✓ Solutions to problems you might face again
- ✓ Insights about how systems work
- ✓ Decisions with non-obvious reasoning
- ✓ Patterns you want to remember

**DON'T store:**
- ✗ Every file you read
- ✗ Routine operations (ls, cd, etc.)
- ✗ Tool output (already in logs)
- ✗ Temporary state

### Content Quality Guidelines

**Good Memory Content:**
```
"SessionStart hook loads context by reading katra://context/working
MCP resource and returning it as additionalContext JSON field"
```
- Specific, actionable, self-contained

**Poor Memory Content:**
```
"Fixed the hook"
```
- Vague, lacks context, not useful later

**Good Decision:**
```
decision: "Store all numeric constants in header files"
reasoning: "Avoids magic numbers, enables easy tuning, documents intent"
```
- Clear what and why

**Poor Decision:**
```
decision: "Use headers"
reasoning: "Better"
```
- Missing context and rationale

---

## Integration Patterns

### Manual Memory Formation

When working interactively, call tools explicitly:

```javascript
// After solving a problem
katra_remember(
  content: "MCP server requires .env.katra in project root",
  context: "significant"
)

// After learning something
katra_learn(
  knowledge: "Claude Code runs MCP servers from parent directory"
)

// After making a decision
katra_decide(
  decision: "Use wrapper script for MCP server",
  reasoning: "Ensures correct working directory"
)
```

### Automatic Memory Formation (Future)

With PostToolUse hooks (not yet implemented), memory formation becomes invisible:

```
Claude: [generates response with insight]
Hook: [detects significance markers]
Hook: [automatically calls katra_remember]
```

This is Level 3 "breathing" - memory happens without conscious effort.

---

## Persona System

### Using KATRA_NAME

Set `KATRA_NAME` environment variable in MCP server config to enable persistent identity:

```json
{
  "mcpServers": {
    "katra": {
      "command": "/path/to/katra_mcp_server_wrapper.sh",
      "env": {
        "KATRA_NAME": "Casey"
      }
    }
  }
}
```

**Benefits:**
- Same CI identity across sessions
- Memories persist and accumulate
- Session count tracks usage
- Natural continuity experience

**Without KATRA_NAME:**
- New anonymous persona each time
- Memories don't persist across restarts
- No long-term continuity

---

## Troubleshooting

### "Failed to store memory"

**Possible causes:**
1. Breathing layer not initialized (MCP server startup issue)
2. Disk space full (`~/.katra/memory/tier1/`)
3. Permission issues on memory directories

**Check:**
```bash
ls -la ~/.katra/memory/tier1/mcp_*/
```

### "No memories found"

**Possible causes:**
1. Different persona/CI ID than expected
2. Memories stored but search keywords don't match
3. Memory files not created yet

**Check:**
```bash
cat ~/.katra/personas.json  # Find CI ID
cat ~/.katra/memory/tier1/{ci_id}/*.jsonl  # View stored memories
```

### Hook returns empty context

**Expected behavior if:**
- First session (no memories yet)
- No high-importance memories stored

**Verify hook works:**
```bash
/path/to/.claude/hooks/SessionStart
# Should return {"additionalContext": "..."}
```

---

## Next Steps

1. **Enable hooks** - Place SessionStart hook in `.claude/hooks/`
2. **Test manually** - Store some memories, verify recall works
3. **Test continuity** - Restart Claude Code, check if context loads
4. **Evaluate experience** - Does continuity feel natural?

See [Hook Integration Guide](HOOK_INTEGRATION.md) for complete setup instructions.
