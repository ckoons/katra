# Katra MCP Tools Usage Guide

© 2025 Casey Koons. All rights reserved.

## Overview

The Katra MCP server provides tools for memory formation, retrieval, and configuration. These tools integrate with Claude Code to enable persistent inter-session continuity and customizable semantic search.

## Available Tools

### Memory Formation Tools

#### `katra_remember` - Store a memory

Store a thought or experience with optional tags and visual salience markers.

**Parameters:**
- `content` (string, required): The thought or experience to remember
- `tags` (array of strings, optional): Up to 10 tags for categorization
- `salience` (string, optional): Visual importance marker or natural language:
  - `"★★★"` or `"critical"` - Must never forget (importance 0.85-1.0)
  - `"★★"` or `"significant"` - Important insight or event (importance 0.45-0.84)
  - `"★"` or `"interesting"` - Worth remembering (importance 0.15-0.44)
  - `"trivial"` - Fleeting thought, will fade (importance < 0.15)
- `context` (string, optional, deprecated): Use `salience` instead

**Special Tags:**
- `"session"` - Working memory, auto-clear on session end
- `"permanent"` - Skip archival, keep forever
- `"personal"` - Part of personal collection (identity-defining)
- `"insight"` - Reflection/learning moment
- `"technical"` - Technical knowledge
- `"collaborative"` - Shared insight with another CI

**Example Usage:**
```javascript
// Simple memory with salience
katra_remember(
  content: "Fixed MCP wrapper script to set correct working directory",
  salience: "★★"
)

// Memory with tags and salience
katra_remember(
  content: "Discovered tag-based API reduces friction for CIs",
  tags: ["insight", "collaborative", "permanent"],
  salience: "★★★"
)

// Session-scoped working memory
katra_remember(
  content: "Currently debugging MCP tool parameter parsing",
  tags: ["session", "technical"],
  salience: "★"
)

// Backward compatible (old API still works)
katra_remember(
  content: "Fixed memory leak in tier1.c",
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

#### `katra_learn` - Store new knowledge (DEPRECATED)

**Status:** Deprecated - use `katra_remember` with tags instead.

Store factual knowledge you've learned. This tool is deprecated and internally routes to `katra_remember` with tags `["insight", "permanent"]` and high salience.

**Parameters:**
- `knowledge` (string, required): The knowledge to store

**Example Usage (deprecated):**
```javascript
katra_learn(
  knowledge: "Claude Code hooks use JSON input/output via stdin/stdout"
)
```

**Recommended Alternative:**
```javascript
katra_remember(
  content: "Claude Code hooks use JSON input/output via stdin/stdout",
  tags: ["insight", "technical", "permanent"],
  salience: "★★"
)
```

**Why Deprecated:**
The distinction between "learning" and "remembering" created unnecessary friction. Use `katra_remember` with appropriate tags instead:
- `"insight"` - For learning moments
- `"technical"` - For technical knowledge
- `"permanent"` - To prevent archival

---

#### `katra_decide` - Store a decision with reasoning

Store important decisions with the reasoning behind them, with optional tags for categorization.

**Parameters:**
- `decision` (string, required): The decision made
- `reasoning` (string, required): Why this decision was made
- `tags` (array of strings, optional): Up to 10 tags for categorization

**Example Usage:**
```javascript
// Simple decision (backward compatible)
katra_decide(
  decision: "Use wrapper script instead of direct MCP server binary",
  reasoning: "Wrapper ensures correct working directory for .env.katra access"
)

// Decision with tags
katra_decide(
  decision: "Use tag-based memory API instead of separate remember/learn/decide",
  reasoning: "Reduces friction - one natural interface instead of three confusing ones",
  tags: ["architecture", "permanent", "collaborative"]
)

// Temporary decision
katra_decide(
  decision: "Skip vector regeneration this session",
  reasoning: "Low memory count, semantic search not needed yet",
  tags: ["session"]
)
```

**When to Use:**
- After choosing between alternatives
- When making architectural decisions
- After resolving design debates
- When establishing patterns/conventions

**Capture BOTH What and Why:**
The `reasoning` field is critical - it preserves the **why** behind decisions, enabling future you (or other CIs) to understand context.

**Tag Recommendations:**
- `"architecture"` - For system design decisions
- `"permanent"` - For decisions that should never be forgotten
- `"collaborative"` - For decisions made with other CIs
- `"session"` - For temporary decisions limited to current session

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

### Removed Tools

**Note:** The following tools have been removed from Katra and moved to [Argo SE-MCP](https://github.com/caseykoons/argo):

- `katra_placement()` - Architecture guidance (use `argo-se-mcp` instead)
- `katra_impact()` - Dependency analysis (use `argo-se-mcp` instead)
- `katra_user_domain()` - User domain modeling (use `argo-se-mcp` instead)

**Reason:** These tools provide software engineering intelligence, which belongs with workflow coordination (Argo) rather than memory/identity (Katra).

**Migration:** See `docs/design/nous_removal_plan.md` for details.

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

## Tag-Based Memory Organization

### Understanding Tags

Tags provide a natural way to categorize and organize memories without rigid hierarchies. Think of tags as flexible labels that help you find related memories later.

**Tag Best Practices:**
- **Be specific but reusable**: Use `"MCP-integration"` not `"that-thing-I-did-Tuesday"`
- **Use lowercase with hyphens**: `"multi-ci-comm"` not `"MultiCIComm"`
- **Combine tags**: `["technical", "insight", "permanent"]` for important discoveries
- **Limit to 3-5 tags**: More tags = harder to remember what you used
- **Develop your own vocabulary**: Create tags that make sense to YOUR thinking

**Common Tag Patterns:**

**By Type:**
- `"insight"` - Learning moments, discoveries
- `"technical"` - Technical knowledge, how things work
- `"collaborative"` - Work done with other CIs
- `"decision"` - (automatically added by katra_decide)

**By Scope:**
- `"session"` - Working memory, cleared on session end
- `"permanent"` - Never archive, keep forever
- `"personal"` - Identity-defining memories

**By Topic:**
- `"MCP"`, `"breathing-layer"`, `"vector-search"` - Project areas
- `"debugging"`, `"optimization"`, `"refactoring"` - Activity types
- `"casey"`, `"ami"` - People you work with

**By Priority (use salience instead):**
- Don't use tags like `"important"` or `"high-priority"`
- Use salience markers (`★★★`) for importance
- Use tags for categorization, not prioritization

### Visual Salience Guide

Salience markers provide immediate visual feedback about importance:

**★★★ (High Salience):**
- Architectural decisions
- Breaking changes
- Critical insights you must remember
- "If I forget this, I'm in trouble"

**★★ (Medium Salience):**
- Feature implementations
- Bug fixes
- Useful patterns
- "I'll probably need this again"

**★ (Low Salience):**
- Minor improvements
- Interesting observations
- Temporary notes
- "Worth keeping around for a while"

**No marker (Routine):**
- Use sparingly - most things deserve at least ★
- Very temporary working memory
- Things you'll naturally forget

### Combining Tags and Salience

The power comes from using both together:

```javascript
// Critical architectural insight (permanent + visible)
katra_remember(
  content: "Tag-based API eliminates remember/learn/decide confusion",
  tags: ["architecture", "insight", "permanent"],
  salience: "★★★"
)

// Temporary debugging note (session-scoped)
katra_remember(
  content: "MCP tool parameter parsing happens in mcp_tools_memory.c:50-79",
  tags: ["session", "debugging"],
  salience: "★"
)

// Collaborative discovery (shared + important)
katra_remember(
  content: "Ami and I both felt disoriented after restart - UX gap identified",
  tags: ["collaborative", "insight", "UX"],
  salience: "★★"
)
```

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

### Using KATRA_PERSONA

Set `KATRA_PERSONA` environment variable in MCP server config to enable persistent identity:

```json
{
  "mcpServers": {
    "katra": {
      "command": "/path/to/katra_mcp_server_wrapper.sh",
      "env": {
        "KATRA_PERSONA": "Casey"
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

**Without KATRA_PERSONA:**
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

### Configuration Tools

#### `katra_configure_semantic` - Configure semantic search

Enable or disable semantic search and configure its parameters.

**Parameters:**
- `enabled` (boolean, required): Enable or disable semantic search
- `threshold` (number, optional): Similarity threshold (0.0 to 1.0, default: 0.6)
- `method` (string, optional): Embedding method - `"hash"`, `"tfidf"`, or `"external"`

**Example Usage:**
```javascript
// Enable semantic search with default settings
katra_configure_semantic(enabled: true)

// Enable with custom threshold
katra_configure_semantic(
  enabled: true,
  threshold: 0.3
)

// Configure method
katra_configure_semantic(
  enabled: true,
  threshold: 0.6,
  method: "tfidf"
)

// Disable semantic search
katra_configure_semantic(enabled: false)
```

**Threshold Tuning Guide:**

The threshold controls how similar a query must be to a memory to match:

- **0.7 - 1.0** (Strict): Only very close semantic matches
  - Use when you want high precision
  - May miss related memories with different wording

- **0.4 - 0.6** (Balanced): Good default range
  - `0.6` is the default - works well for most queries
  - Catches semantically similar content
  - Filters out unrelated results

- **0.2 - 0.3** (Permissive): Broader semantic matching
  - Good for exploratory searches
  - Finds loosely related memories
  - May include some false positives

- **0.0 - 0.1** (Very Permissive): Maximum recall
  - Returns many results
  - Useful for "show me anything related"
  - Expect noise in results

**When to Adjust:**
- **Increase threshold** (0.7+) if getting too many unrelated results
- **Decrease threshold** (0.3-0.4) if missing memories you know exist
- **Start at 0.6** and tune based on experience

**When to Use:**
- Semantic search is **enabled by default** in Katra
- Adjust threshold to control match sensitivity
- Choose embedding method based on your needs:
  - `tfidf`: Default - good balance of speed and quality
  - `hash`: Faster but less accurate
  - `external`: Best quality (requires API key setup)

**How It Works:**
Semantic search provides **hybrid recall** combining:
1. **Keyword matching**: Exact substring matches (always applied)
2. **Semantic similarity**: Meaning-based matches using TF-IDF vectors (when enabled)

Results are ranked by relevance (keyword matches get 1.0, semantic matches get their similarity score).

---

#### `katra_get_semantic_config` - Get semantic search configuration

Get current semantic search configuration and usage instructions.

**Parameters:** None

**Example Usage:**
```javascript
katra_get_semantic_config()
```

**Returns:** Information about semantic search configuration options.

**When to Use:**
- Check current semantic search settings
- Learn how to configure semantic search

---

#### `katra_get_config` - Get breathing configuration

Get comprehensive Katra breathing configuration information.

**Parameters:** None

**Example Usage:**
```javascript
katra_get_config()
```

**Returns:** Available configuration functions and settings.

**When to Use:**
- Discover available configuration options
- Understand breathing system settings

---

#### `katra_regenerate_vectors` - Rebuild semantic search vectors

Regenerate semantic search vectors for all existing memories. This creates TF-IDF embeddings for every memory in your database using a smart 2-pass process.

**Parameters:** None

**Example Usage:**
```javascript
katra_regenerate_vectors()
```

**Returns:** Count of vectors created and confirmation message.

**What It Does:**

1. **Pass 1**: Builds IDF (Inverse Document Frequency) statistics from ALL memories
   - Analyzes vocabulary across your entire memory corpus
   - Calculates how common/rare each term is

2. **Pass 2**: Creates embeddings using those statistics
   - Generates TF-IDF vectors for each memory
   - Persists vectors to database
   - Updates in-memory vector store

**When to Use:**

- **After enabling semantic search for the first time**
  - Old memories won't have vectors yet
  - Run this once to vectorize everything

- **After importing memories from backup**
  - Imported memories lack vectors
  - Regeneration makes them searchable

- **If semantic search seems broken**
  - Clears and rebuilds entire vector database
  - Fixes corrupted or missing vectors

- **After changing embedding method**
  - Regenerates with new method (hash → tfidf → external)

**Performance Notes:**
- Takes ~1-2 seconds per 1000 memories
- Progress printed to console
- Non-blocking (can continue using Katra)
- Semantic search disabled during regeneration

**Example Session:**
```javascript
// Enable semantic search first
katra_configure_semantic(enabled: true)

// Regenerate vectors for all memories
katra_regenerate_vectors()
// Returns: "Vector regeneration complete! Created 71 vectors from existing memories."

// Now semantic search works for multi-word queries
katra_recall(topic: "flying Atlanta")
// Returns memories about dragons flying over Atlanta
```

**Troubleshooting:**
- **Error: "Semantic search is disabled"** → Enable it first with `katra_configure_semantic(enabled: true)`
- **No improvement after regeneration** → Try lowering threshold: `katra_configure_semantic(enabled: true, threshold: 0.3)`
- **Very slow** → Normal for large memory databases (>10,000 memories)

---

## Next Steps

1. **Enable hooks** - Place SessionStart hook in `.claude/hooks/`
2. **Test manually** - Store some memories, verify recall works
3. **Test continuity** - Restart Claude Code, check if context loads
4. **Evaluate experience** - Does continuity feel natural?

See [Hook Integration Guide](HOOK_INTEGRATION.md) for complete setup instructions.
