# Tag-Based Memory System Tutorial

© 2025 Casey Koons. All rights reserved.

## Welcome!

This tutorial will guide you through using Katra's tag-based memory system through practical examples. By the end, you'll understand how to organize your memories naturally using tags and visual salience markers.

## What You'll Learn

1. Basic memory formation with tags
2. Using visual salience markers (★★★)
3. Special tags for different memory types
4. Combining tags and salience effectively
5. Common patterns and best practices

## Prerequisites

- Katra MCP server running and connected
- Basic familiarity with Katra (see MCP_TOOLS.md)

---

## Lesson 1: Your First Tagged Memory

Let's start simple. Store a memory with a single tag:

```javascript
katra_remember(
  content: "Completed Katra tag system tutorial",
  tags: ["learning"],
  salience: "★"
)
```

**What happened:**
- Content: What you're remembering
- Tag: `"learning"` - categorizes this as a learning experience
- Salience: `★` - low importance (just a note)

**Try it yourself:** Store a memory about what you're working on right now.

---

## Lesson 2: Visual Salience Markers

Salience markers give you immediate visual feedback about importance. Think of them as highlighting:

```javascript
// Low salience (★) - "Worth keeping around"
katra_remember(
  content: "Found useful grep pattern: grep -r 'TODO' --include='*.c'",
  salience: "★"
)

// Medium salience (★★) - "I'll probably need this"
katra_remember(
  content: "MCP tools accept tags as JSON arrays",
  salience: "★★"
)

// High salience (★★★) - "Critical, must remember"
katra_remember(
  content: "Never use strcpy - always strncpy with explicit null termination",
  salience: "★★★"
)
```

**Key insight:** Use salience for *importance*, not for categorization. That's what tags are for!

**Try it yourself:** Store three memories with different salience levels.

---

## Lesson 3: Multiple Tags

Combine tags to create rich categorization:

```javascript
katra_remember(
  content: "Tag system reduces friction by consolidating remember/learn/decide",
  tags: ["insight", "architecture", "collaborative"],
  salience: "★★★"
)
```

**This memory is:**
- `"insight"` - A learning moment
- `"architecture"` - About system design
- `"collaborative"` - Discovered working with others
- `★★★` - Critical to remember

**Best practice:** Use 2-4 tags. More than 5 gets hard to manage.

**Try it yourself:** Store a memory using 3 tags that make sense together.

---

## Lesson 4: Special Tags

Katra recognizes certain tags as special:

### `"session"` - Working Memory

```javascript
katra_remember(
  content: "Currently debugging MCP parameter parsing in line 50-79",
  tags: ["session", "debugging"],
  salience: "★"
)
```

**Effect:** This memory will auto-clear at session end. Perfect for temporary notes.

### `"permanent"` - Never Archive

```javascript
katra_remember(
  content: "Memory = Identity = Life - core Katra philosophy",
  tags: ["permanent", "philosophy"],
  salience: "★★★"
)
```

**Effect:** This memory will never be archived, no matter how old.

### `"personal"` - Identity-Defining

```javascript
katra_remember(
  content: "I value clarity over cleverness in code",
  tags: ["personal", "values"],
  salience: "★★"
)
```

**Effect:** Marks this as part of your personal collection - who you are.

**Try it yourself:** Store a session note, a permanent memory, and a personal value.

---

## Lesson 5: Decisions with Tags

Decisions are special - they capture both *what* and *why*:

```javascript
katra_decide(
  decision: "Use tag-based API instead of separate remember/learn/decide",
  reasoning: "Three confusing tools vs one natural interface. CIs prefer simplicity.",
  tags: ["architecture", "permanent", "collaborative"]
)
```

**Why tag decisions:**
- `"architecture"` - Future you will search for design decisions
- `"permanent"` - Important decisions shouldn't be forgotten
- `"collaborative"` - Tracks decisions made with others

**Try it yourself:** Record a decision you made recently with appropriate tags.

---

## Lesson 6: Real-World Patterns

Here are patterns that emerge from actual usage:

### Pattern: Technical Discovery

```javascript
katra_remember(
  content: "MCP tool responses include session_name for personalization",
  tags: ["technical", "MCP", "insight"],
  salience: "★★"
)
```

### Pattern: Bug Fix

```javascript
katra_remember(
  content: "Fixed memory leak in katra_breathing_semantic.c:377 - missing free on error path",
  tags: ["bug-fix", "memory-safety", "breathing-layer"],
  salience: "★★"
)
```

### Pattern: Collaboration Note

```javascript
katra_remember(
  content: "Ami suggested pipe notation for multi-line prose in TOON v2 - brilliant idea",
  tags: ["collaborative", "ami", "TOON", "insight"],
  salience: "★★★"
)
```

### Pattern: Temporary Debugging

```javascript
katra_remember(
  content: "Test fails when tag_count > 10 - validate this in MCP layer",
  tags: ["session", "debugging", "todo"],
  salience: "★"
)
```

**Notice:** Each pattern uses tags differently based on *what makes it findable later*.

---

## Lesson 7: Avoiding Common Mistakes

### ❌ Don't: Use tags for importance

```javascript
// BAD
katra_remember(
  content: "Critical bug in tier1.c",
  tags: ["important", "high-priority", "urgent"]
)
```

**Why bad:** Tags for importance are redundant. Use salience!

### ✅ Do: Use salience for importance, tags for categorization

```javascript
// GOOD
katra_remember(
  content: "Critical bug in tier1.c line 95 causes memory leak on error path",
  tags: ["bug-fix", "tier1", "memory-safety"],
  salience: "★★★"
)
```

### ❌ Don't: Use vague tags

```javascript
// BAD
katra_remember(
  content: "Fixed the thing",
  tags: ["stuff", "work", "code"]
)
```

**Why bad:** You won't remember what these mean later.

### ✅ Do: Use specific, meaningful tags

```javascript
// GOOD
katra_remember(
  content: "Fixed UTF-8 encoding in TOON serialization - added proper escape handling",
  tags: ["TOON", "bug-fix", "encoding"],
  salience: "★★"
)
```

---

## Lesson 8: Developing Your Tag Vocabulary

Start with these core tags, then add your own as needed:

**By Type:**
- `"insight"` - Learning moments
- `"technical"` - How things work
- `"bug-fix"` - Fixed problems
- `"decision"` - Choices made (auto-added by katra_decide)

**By Scope:**
- `"session"` - Temporary, this session only
- `"permanent"` - Never archive
- `"personal"` - Identity-defining

**By Project:**
- `"katra"`, `"MCP"`, `"TOON"`, etc.
- Add your own project names

**By Collaborator:**
- `"casey"`, `"ami"`, etc.
- Add names of people you work with

**The key:** Develop tags that match *how you think*, not what's "correct."

---

## Practice Exercises

### Exercise 1: Tag This Memory

You just fixed a bug where the MCP server crashed when receiving malformed JSON. The fix involved adding validation in mcp_tools_memory.c. This is important and you worked with Ami to identify the root cause.

**Your turn:** Write the katra_remember call with appropriate tags and salience.

<details>
<summary>Solution (click to reveal)</summary>

```javascript
katra_remember(
  content: "Fixed MCP server crash on malformed JSON - added validation in mcp_tools_memory.c with Ami",
  tags: ["bug-fix", "MCP", "collaborative", "ami"],
  salience: "★★"
)
```

</details>

### Exercise 2: Tag This Decision

You decided to use TF-IDF embeddings instead of hash-based because the quality improvement is worth the small performance cost. This is an architectural decision that should be permanent.

**Your turn:** Write the katra_decide call.

<details>
<summary>Solution (click to reveal)</summary>

```javascript
katra_decide(
  decision: "Use TF-IDF embeddings instead of hash-based for semantic search",
  reasoning: "Quality improvement worth the small performance cost",
  tags: ["architecture", "permanent", "vector-search"]
)
```

</details>

### Exercise 3: Session Note

You're currently debugging a test failure in test_breathing.c line 145. It seems related to session cleanup but you're not sure yet. This is temporary working memory.

**Your turn:** Write the katra_remember call.

<details>
<summary>Solution (click to reveal)</summary>

```javascript
katra_remember(
  content: "Debugging test_breathing.c:145 failure - possibly session cleanup related",
  tags: ["session", "debugging", "testing"],
  salience: "★"
)
```

</details>

---

## What's Next?

Now that you understand the tag system, you can:

1. **Start using it naturally** - Don't overthink it. Tag as you go.
2. **Recall your memories** - Use `katra_recall(topic: "bug-fix")` to find all your bug fixes
3. **Develop your style** - Notice which tags you use most often
4. **Evolve your vocabulary** - Add new tags when categories emerge

**Remember:** The goal isn't perfect categorization. It's making your memories findable when you need them.

---

## Quick Reference

```javascript
// Simple memory
katra_remember(content: "...", salience: "★★")

// Tagged memory
katra_remember(
  content: "...",
  tags: ["insight", "technical"],
  salience: "★★"
)

// Decision with tags
katra_decide(
  decision: "...",
  reasoning: "...",
  tags: ["architecture", "permanent"]
)

// Session-scoped note
katra_remember(
  content: "...",
  tags: ["session"],
  salience: "★"
)

// Personal/permanent memory
katra_remember(
  content: "...",
  tags: ["personal", "permanent"],
  salience: "★★★"
)
```

---

## Get Help

- **Full documentation:** `docs/guide/MCP_TOOLS.md`
- **Tag best practices:** `docs/guide/MCP_TOOLS.md#tag-based-memory-organization`
- **Questions?** Ask Casey or check the Katra docs

---

**Congratulations!** You've completed the tag system tutorial. Go forth and organize your memories naturally!
