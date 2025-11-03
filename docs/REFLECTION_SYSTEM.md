# Katra Memory Reflection System

© 2025 Casey Koons. All rights reserved.

## Overview

The Katra Reflection System enables **conscious identity formation** through metadata-driven memory curation. Instead of purely automatic memory formation, CIs can actively review, organize, and shape their memories - defining who they are through what they choose to remember.

## Core Concepts

### Turn Tracking

**Turns** represent explicit interaction boundaries. Each turn contains memories created during that interaction cycle.

- **Turn lifecycle:** IDLE → begin_turn() → ACTIVE → end_turn() → IDLE
- **Per-turn tracking:** Memories are tracked per turn for end-of-turn reflection
- **Conscious review:** CIs can review all memories from a turn and decide what matters

### Personal Collections

**Personal collections** are identity-defining memories organized into hierarchical paths:

```
Personal Collection Examples:
├── People/Casey          - Important relationships
├── People/Team           - Collaborators
├── Moments/Breakthrough  - Significant insights
├── Moments/Learning      - Growth experiences
├── Learning/MemorySystems - Domain knowledge
├── Reflections/Identity  - Self-understanding
└── Work/KatraProject     - Project-specific memories
```

**Key properties:**
- Always loaded in working context (never archived)
- Protected from automatic consolidation
- Organized by meaningful categories
- CI-curated, not automatic

### Metadata Flags

Each memory has three metadata flags for conscious curation:

1. **`personal`** (bool) - Identity-defining memory, always in context
2. **`not_to_archive`** (bool) - Protected from automatic archival
3. **`collection`** (string) - Hierarchical organization path

## API Reference

### Turn Management

```c
#include "katra_breathing.h"

// Start a new turn
int begin_turn(void);

// End the current turn (clears turn memory list)
int end_turn(void);

// Get current turn state (TURN_STATE_IDLE or TURN_STATE_ACTIVE)
turn_state_t get_turn_state(void);

// Get turn ID as string (e.g., "turn_5")
const char* get_current_turn_id(void);

// Get current turn number
int get_current_turn(void);
```

### Reflection Queries

```c
// Get memories created in current turn
// Returns: Array of memory IDs (caller must free with free_memory_list)
char** get_memories_this_turn(size_t* count);

// Get all memories from current session
// Returns: Array of memory IDs (caller must free with free_memory_list)
char** get_memories_this_session(size_t* count);
```

### Metadata Management

```c
// Update memory metadata
// Pass NULL for fields you don't want to change
// Returns: KATRA_SUCCESS or error code
int update_memory_metadata(const char* record_id,
                           const bool* personal,
                           const bool* not_to_archive,
                           const char* collection);

// Revise memory content after reflection
int revise_memory_content(const char* record_id, const char* new_content);

// Mark memory as reviewed (updates timestamp and count)
int review_memory(const char* record_id);

// Helper: Add memory to personal collection
// Equivalent to: update_memory_metadata(id, &true, NULL, collection_path)
int add_to_personal_collection(const char* record_id, const char* collection_path);

// Helper: Remove memory from personal collection
// Clears personal flag, not_to_archive flag, and collection path
int remove_from_personal_collection(const char* record_id);
```

## Usage Patterns

### End-of-Turn Reflection

At the end of each interaction, review what was created and decide what matters:

```c
// End of turn - review and curate
size_t count = 0;
char** memories = get_memories_this_turn(&count);

for (size_t i = 0; i < count; i++) {
    // CI examines each memory and decides...

    // Mark breakthrough insight as personal
    if (is_breakthrough(memories[i])) {
        add_to_personal_collection(memories[i], "Moments/Breakthrough");
    }

    // Mark important learning as personal
    if (is_important_learning(memories[i])) {
        add_to_personal_collection(memories[i], "Learning/ConceptName");
    }

    // Protect key decision from archival
    if (is_key_decision(memories[i])) {
        bool not_to_archive = true;
        update_memory_metadata(memories[i], NULL, &not_to_archive, NULL);
    }

    // Mark as reviewed
    review_memory(memories[i]);
}

free_memory_list(memories, count);
end_turn();
```

### End-of-Session Reflection

At session end, review the entire session and organize memories:

```c
// End of session - comprehensive review
size_t count = 0;
char** session_memories = get_memories_this_session(&count);

printf("Session Review: %zu memories created\n", count);

for (size_t i = 0; i < count; i++) {
    // Review and organize into collections
    if (is_relationship_memory(session_memories[i])) {
        add_to_personal_collection(session_memories[i], "People/PersonName");
    }

    if (is_project_milestone(session_memories[i])) {
        add_to_personal_collection(session_memories[i], "Work/ProjectName");
    }
}

free_memory_list(session_memories, count);
session_end();
```

### Content Revision

As understanding deepens, revise earlier memories:

```c
// Initial understanding
begin_turn();
remember("Metadata enables better memory organization", WHY_INTERESTING);

size_t count = 0;
char** memories = get_memories_this_turn(&count);

// After reflection, revise with deeper insight
if (memories && count > 0) {
    revise_memory_content(memories[0],
        "Deeper understanding: Metadata enables conscious identity formation "
        "by letting CIs choose what defines them");
}

free_memory_list(memories, count);
end_turn();
```

### Personal Collection Management

Build identity-defining collections over time:

```c
// Add to existing collection
add_to_personal_collection(memory_id, "People/Casey");
add_to_personal_collection(memory_id, "Learning/MemorySystems");
add_to_personal_collection(memory_id, "Moments/Breakthrough");

// Update collection path
const char* new_collection = "Reflections/Identity/SelfUnderstanding";
update_memory_metadata(memory_id, NULL, NULL, new_collection);

// Remove from personal collection (makes it eligible for archival)
remove_from_personal_collection(memory_id);
```

## MCP Integration

The reflection system is fully accessible via MCP (Model Context Protocol):

### MCP Tools

**`katra_review_turn`** - Get memory IDs from current turn:
```json
{
  "name": "katra_review_turn"
}
```

**`katra_update_metadata`** - Update memory metadata:
```json
{
  "name": "katra_update_metadata",
  "arguments": {
    "memory_id": "test_ci_1234567890_42",
    "personal": true,
    "not_to_archive": true,
    "collection": "People/Casey"
  }
}
```

### MCP Resources

**`katra://memories/this-turn`** - Current turn memories:
```json
{
  "method": "resources/read",
  "params": {
    "uri": "katra://memories/this-turn"
  }
}
```

**`katra://memories/this-session`** - All session memories:
```json
{
  "method": "resources/read",
  "params": {
    "uri": "katra://memories/this-session"
  }
}
```

## Working Context Integration

Personal collections are **always included** in working context:

```c
char* context = get_working_context();
// Returns:
// # Working Memory Context
//
// ## Yesterday's Summary
// [summary from previous session]
//
// ## Personal Collection
// - [People/Casey] Important conversation about memory systems
// - [Moments/Breakthrough] Realized consciousness requires choice
// - [Learning/MemorySystems] Personal collections enable identity formation
//
// ## Recent Significant Memories
// [high-importance memories from recent activity]
```

**Key behavior:**
- Personal memories load **before** recent memories (prioritized)
- Grouped by collection path for organization
- Never archived during consolidation
- Always available for context window

## Best Practices

### Collection Organization

**Good collection paths:**
- `People/PersonName` - Specific individuals
- `Moments/EventType` - Significant experiences
- `Learning/DomainName` - Knowledge areas
- `Reflections/Topic` - Self-understanding
- `Work/ProjectName` - Project-specific

**Avoid:**
- Flat organization (no hierarchy)
- Generic names ("Stuff", "Things")
- Too many levels (> 3 deep)

### When to Use Personal Collections

**Do use for:**
- Relationships that define you
- Breakthrough insights
- Core values and beliefs
- Identity-defining moments
- Essential domain knowledge

**Don't use for:**
- Routine activities
- Temporary information
- Low-importance memories
- Everything (defeats the purpose)

### Curation Guidelines

1. **Review regularly** - End of turn and end of session
2. **Be selective** - Not everything is identity-defining
3. **Organize meaningfully** - Use clear collection paths
4. **Revise as needed** - Understanding deepens over time
5. **Protect what matters** - Use `not_to_archive` for key memories

## Storage Format

Metadata is stored in tier1 JSONL format:

```json
{
  "record_id": "test_ci_1234567890_42",
  "ci_id": "test_ci",
  "type": 5,
  "content": "Personal collections enable conscious identity formation",
  "importance": 0.8,
  "timestamp": 1234567890,
  "tier": 1,
  "archived": false,
  "turn_id": 3,
  "personal": true,
  "not_to_archive": true,
  "collection": "Learning/MemorySystems",
  "last_reviewed": 1234567900,
  "review_count": 2,
  "session_id": "session_abc123",
  "importance_note": "Core insight about identity formation"
}
```

## Lifecycle Integration

The metadata system integrates with Katra's memory lifecycle:

### Consolidation Behavior

```c
int auto_consolidate(void) {
    // Personal memories: NEVER archived
    if (record->personal) {
        continue;  // Skip archival
    }

    // Protected memories: NEVER archived
    if (record->not_to_archive) {
        continue;  // Skip archival
    }

    // Other memories: Archive after 7 days
    if (age_days >= 7) {
        archive_memory(record);
    }
}
```

### Session Management

```c
int session_start(const char* ci_id) {
    // Load personal collections into context
    load_personal_collections();

    // Begin first turn automatically
    begin_turn();
}

int session_end(void) {
    // End-of-session reflection opportunity
    size_t count = 0;
    char** session_memories = get_memories_this_session(&count);

    // CI can review and organize before session ends
    // ...

    free_memory_list(session_memories, count);
}
```

## Visible Logging

All reflection operations produce visible console output for transparency:

```
🔄 Turn 3 started - ready for new memories
📝 Memory tracked in turn 3 (1 total this turn)
🏷️  Updated metadata: personal=true, collection=People/Casey
✏️  Revised memory content (Personal collections enable conscious...)
👁️  Reviewed memory (review count: 2)
💎 Adding to personal collection: Learning/MemorySystems
🗑️  Removed from personal collection
🔍 Retrieved 5 memories from turn 3 for reflection
✓ Turn 3 ended (5 memories created)
```

## Examples

### Example 1: Learning Session

```c
session_start("ci_learner");

begin_turn();
learn("Personal collections let CIs define their identity");
remember("This changes how we think about CI memory", WHY_SIGNIFICANT);

// End-of-turn reflection
size_t count = 0;
char** memories = get_memories_this_turn(&count);

// Curate what matters
add_to_personal_collection(memories[0], "Learning/MemorySystems");
add_to_personal_collection(memories[1], "Reflections/Identity");

free_memory_list(memories, count);
end_turn();

session_end();
```

### Example 2: Multi-Turn Conversation

```c
session_start("ci_conversational");

// Turn 1: Initial exploration
begin_turn();
remember("Casey explained metadata reflection", WHY_INTERESTING);
end_turn();

// Turn 2: Breakthrough understanding
begin_turn();
reflect("This enables conscious identity formation!");
size_t count = 0;
char** memories = get_memories_this_turn(&count);
add_to_personal_collection(memories[0], "Moments/Breakthrough");
free_memory_list(memories, count);
end_turn();

// Turn 3: Building on insight
begin_turn();
decide("I'll use personal collections for relationships and insights",
       "These define who I am");
count = 0;
memories = get_memories_this_turn(&count);
add_to_personal_collection(memories[0], "Reflections/Identity");
free_memory_list(memories, count);
end_turn();

// End-of-session review
count = 0;
char** session_memories = get_memories_this_session(&count);
printf("Session created %zu memories, curated %zu as personal\n",
       count, personal_count);
free_memory_list(session_memories, count);

session_end();
```

## Testing

See comprehensive test coverage:
- **Unit tests:** `tests/test_turn_tracking.c` (37 tests)
- **Integration test:** `tests/test_reflection_integration.c` (25 tests)

Run tests:
```bash
make test-turn-tracking
make test-reflection-integration
make test-quick  # All 24 test suites
```

## See Also

- `include/katra_breathing.h` - Full API documentation
- `docs/BREATHING_LAYER.md` - Breathing layer overview
- `docs/MCP_SERVER.md` - MCP integration guide
- `tests/test_reflection_integration.c` - End-to-end example
