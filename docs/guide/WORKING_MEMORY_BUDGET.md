# Working Memory Budget System

© 2025 Casey Koons. All rights reserved.

## Overview

The Working Memory Budget system (Phase 2 + 2.1) implements automatic management of session-scoped memories to mimic human working memory constraints. Just as humans can only hold 5-7 threads of thought simultaneously, Katra automatically manages session-scoped memory to prevent cognitive overload.

**Design Philosophy:** Memory should fade naturally, like human working memory, rather than accumulate forever.

## How It Works

### The Three-Tier Strategy

**Below Soft Limit (< 35):** ✅ **Normal Operation**
- All session-scoped memories preserved
- No automatic cleanup
- Working memory operating efficiently

**At Soft Limit (≥ 35):** 📦 **Graceful Archival**
- Archive oldest 10 session memories
- **Archival = Convert to Permanent**
  - Sets `session_scoped = 0`
  - Removes from working memory budget
  - Memory stays in database as permanent
- Gives memories a "second chance" - might be useful later
- **Tag-aware:** Protected tags skip archival (see below)

**At Hard Limit (≥ 50):** 🗑️ **Hard Cleanup**
- Delete oldest 10 session memories entirely
- Removed from database completely
- Even protected tags don't save from hard limit
- Prevents unbounded memory growth

### Hybrid Archival Model

```
Session Memory Lifecycle:

1. Created with "session" tag
   ↓
2. Count < 35: Preserved in working memory
   ↓
3. Count ≥ 35 (soft): Oldest archived → converted to permanent
   ↓
4. Count ≥ 50 (hard): Oldest deleted entirely
```

**Why Hybrid?**
- **Soft limit preserves information:** Archived memories might be useful later
- **Hard limit prevents bloat:** Eventually old memories must go
- **Balance:** Gives memories a second chance without infinite accumulation

## Configuration

### Default Settings

```c
working_memory_enabled = true           // Enabled by default
working_memory_soft_limit = 35          // Archive threshold
working_memory_hard_limit = 50          // Delete threshold
working_memory_batch_size = 10          // Process count
tag_aware_archival = true               // Skip protected tags at soft limit
```

### Tuning Parameters

**If experiencing frequent archival:**
- Increase `soft_limit` (e.g., 50)
- Adjust `batch_size` to archive fewer at once
- Check if appropriate tags being used

**If running out of space:**
- Decrease `soft_limit` (e.g., 25)
- Decrease `hard_limit` (e.g., 40)
- Increase `batch_size` to archive more aggressively

## Tag-Aware Archival (Phase 2.1)

### Protected Tags

These tags protect memories from archival **at soft limit only:**

- `"insight"` - Learning moments
- `"permanent"` - Explicitly marked permanent
- `"personal"` - Identity-defining memories
- `"decision"` - Important decisions
- `"architecture"` - Architectural decisions
- `"important"` - Explicitly marked important

### How Tag Protection Works

**At Soft Limit (35):**
```
Untagged session memory:     → Archived (converted to permanent)
Tagged with "insight":        → Skipped (preserved in working memory)
Tagged with "decision":       → Skipped (preserved in working memory)
Tagged with "todo":           → Archived (not a protected tag)
```

**At Hard Limit (50):**
```
ALL session memories:         → Deleted (protection doesn't apply)
Even "insight" memories:      → Deleted if oldest
Even "decision" memories:     → Deleted if oldest
```

**Rationale:** Hard limit exists to prevent unbounded growth. If you hit hard limit, you're creating too many session memories - even important ones must go.

### Usage Patterns

**Good: Selective tagging**
```javascript
// Temporary debugging note
katra_remember(
  content: "Checking if parameter validation works",
  tags: ["session", "debugging"],
  salience: "★"
)
// → Will be archived at soft limit (no protected tags)

// Important discovery
katra_remember(
  content: "Found that tag system reduces friction by 40%",
  tags: ["session", "insight", "collaborative"],
  salience: "★★★"
)
// → Preserved at soft limit ("insight" is protected)
// → Converted to permanent if still around at next archival
```

**Bad: Over-tagging**
```javascript
// Everything marked "important"
katra_remember(
  content: "Read a file",
  tags: ["session", "important"],  // Don't do this!
  salience: "★"
)
// → Defeats purpose of budget - everything becomes "important"
```

## Working Memory Statistics API

### Getting Stats

```c
#include "katra_breathing.h"

working_memory_stats_t stats;
int result = working_memory_get_stats(ci_id, &stats);

if (result == KATRA_SUCCESS) {
    printf("Working Memory Status:\n");
    printf("  Current: %zu memories\n", stats.current_count);
    printf("  Soft limit: %zu\n", stats.soft_limit);
    printf("  Hard limit: %zu\n", stats.hard_limit);
    printf("  Utilization: %.1f%%\n", stats.utilization);
    printf("  Enabled: %s\n", stats.enabled ? "yes" : "no");
}
```

### Stats Structure

```c
typedef struct {
    size_t current_count;      // Current session-scoped memory count
    size_t soft_limit;         // Soft limit (archive threshold)
    size_t hard_limit;         // Hard limit (delete threshold)
    size_t batch_size;         // Batch processing size
    bool enabled;              // Is budget enforcement enabled
    float utilization;         // Percentage of soft limit used (0-100+)
} working_memory_stats_t;
```

**Utilization:**
- < 100%: Below soft limit (normal operation)
- ≥ 100%: At/above soft limit (archiving oldest)
- ≥ 143%: At/above hard limit (50/35 = 1.43, deleting oldest)

### Visibility to CI

CIs can check their own working memory budget:

```javascript
// Via MCP tool (future enhancement)
katra_working_memory_stats()
// Returns:
// {
//   "current": 28,
//   "soft_limit": 35,
//   "hard_limit": 50,
//   "utilization": 80.0,
//   "status": "healthy"
// }
```

## Automatic Enforcement

### When Budget is Checked

**Every ~30 seconds** during `breathe_periodic_maintenance()`:

1. Get count of session-scoped memories
2. If count ≥ hard_limit:
   - Delete oldest batch_size memories
   - Log warning
3. Else if count ≥ soft_limit:
   - Archive oldest batch_size memories (tag-aware)
   - Log info
4. Else:
   - No action (within budget)

### Logging

**Normal operation:**
```
[DEBUG] Working memory within budget (28/35/50)
```

**Soft limit reached:**
```
[INFO] Working memory at soft limit (37/35) - archiving oldest 10
[INFO] Archived 8 oldest session-scoped memories (soft limit, tag-aware)
```
*Note: Only 8 archived because 2 had protected tags*

**Hard limit reached:**
```
[WARN] Working memory at hard limit (52/50) - deleting oldest 10
[INFO] Deleted 10 oldest session-scoped memories (hard limit)
```

## Best Practices

### For CIs Using the System

**1. Use Session-Scoped Appropriately**
```javascript
// Good: Temporary working notes
katra_remember(
  content: "Currently debugging MCP parameter parsing",
  tags: ["session", "debugging"]
)

// Bad: Important persistent information
katra_remember(
  content: "Casey taught me about goto cleanup patterns",
  tags: ["session"]  // Should be permanent, not session!
)
```

**2. Tag Important Session Memories**
```javascript
// Discovery during debugging - protect it!
katra_remember(
  content: "Root cause: forgot to initialize breathing layer",
  tags: ["session", "insight", "debugging"],
  salience: "★★"
)
// → Protected from archival at soft limit
// → Eventually converts to permanent if archived
```

**3. Monitor Your Budget**
```javascript
// Check periodically
working_memory_stats_t stats;
working_memory_get_stats(ci_id, &stats);

if (stats.utilization > 90) {
    // Approaching soft limit - consider:
    // 1. Clearing old debugging notes
    // 2. Converting important session memories to permanent
    // 3. Finishing current train of thought
}
```

### For Developers Configuring the System

**1. Choose Limits Based on Use Case**

**Interactive Development (default):**
```c
soft_limit = 35   // Plenty of room for active debugging
hard_limit = 50   // Reasonable cap
```

**Production/Long-Running:**
```c
soft_limit = 25   // Tighter budget
hard_limit = 40   // Enforce discipline
```

**Research/Exploration:**
```c
soft_limit = 50   // More room for exploration
hard_limit = 75   // Higher tolerance
```

**2. Tune Batch Size**

```c
batch_size = 10   // Default - balanced cleanup
batch_size = 5    // Gentler archival (more frequent but smaller)
batch_size = 20   // Aggressive cleanup (less frequent but larger)
```

**3. Adjust Protected Tags**

Modify `g_protected_tags` in `katra_breathing.c`:

```c
static const char* g_protected_tags[] = {
    TAG_INSIGHT,
    TAG_PERMANENT,
    TAG_PERSONAL,
    "decision",
    "architecture",
    "important",
    "your-custom-tag",  // Add custom protected tags
    NULL
};
```

## Architecture Details

### Files

**Core Implementation:**
- `src/breathing/katra_breathing_working_memory.c` - Budget enforcement logic
- `include/katra_breathing.h` - Public API and stats structure
- `src/breathing/katra_breathing_internal.h` - Internal declarations
- `include/katra_limits.h` - Default constants

**Integration:**
- `src/breathing/katra_breathing.c` - Config initialization
- `src/breathing/katra_breathing_health.c` - Periodic maintenance hook

### Database Schema

**Session-scoped flag:**
```sql
CREATE TABLE memories (
    record_id TEXT PRIMARY KEY,
    ci_id TEXT,
    session_scoped INTEGER DEFAULT 0,  -- 1 = session, 0 = permanent
    tags TEXT,                         -- JSON array of tags
    timestamp INTEGER,
    ...
);

CREATE INDEX idx_session_scoped ON memories(ci_id, session_scoped, timestamp);
```

**Queries:**

```sql
-- Count session memories
SELECT COUNT(*) FROM memories
WHERE ci_id = ? AND session_scoped = 1;

-- Archive oldest (soft limit, tag-aware)
UPDATE memories SET session_scoped = 0
WHERE record_id IN (
    SELECT record_id FROM memories
    WHERE ci_id = ? AND session_scoped = 1
      AND (tags IS NULL OR tags = '[]')  -- Untagged only
    ORDER BY timestamp ASC
    LIMIT 10
);

-- Delete oldest (hard limit)
DELETE FROM memories
WHERE record_id IN (
    SELECT record_id FROM memories
    WHERE ci_id = ? AND session_scoped = 1
    ORDER BY timestamp ASC
    LIMIT 10
);
```

## Performance Considerations

### Overhead

**Memory:** Minimal - just counters and config
**CPU:** < 1ms per check (SQLite COUNT query)
**I/O:** Minimal - only when archiving/deleting

**Check frequency:** ~30 seconds
**Typical cost:** 1-2 checks before action needed
**Archival cost:** ~5-10ms for batch update/delete

### Scalability

**Works well for:**
- 0-100 session memories: Excellent
- 100-1000 session memories: Good (will hit limits and self-regulate)
- 1000+ session memories: Not recommended (redesign needed)

**If hitting limits constantly:**
- Use smaller batch size for more frequent cleanup
- Lower soft/hard limits
- Re-evaluate session vs permanent memory usage

## Troubleshooting

### "Working memory at hard limit" warnings

**Cause:** Creating too many session-scoped memories

**Solutions:**
1. Use permanent memories for long-term information
2. Tag important session memories with protected tags
3. Increase hard_limit if appropriate for your use case
4. Check for memory leaks (memories not being cleaned up)

### Protected tags not working

**Check:**
1. Tag-aware archival enabled? `tag_aware_archival = true`
2. Using exact tag name? Case-sensitive: `"insight"` not `"Insight"`
3. At soft limit, not hard? Protection only applies at soft limit
4. Tags actually stored? Check `tags` field in database

### Memories disappearing unexpectedly

**Likely causes:**
1. Session-scoped + hard limit reached → deleted
2. Session ended → session-scoped cleared (expected behavior)
3. Manual archival/deletion

**Prevent:**
- Don't use "session" tag for important long-term memories
- Use protected tags for important session memories
- Convert to permanent before session end if needed

## Future Enhancements

**Phase 2.2 (Proposed):**
- Per-tag JSON parsing for full tag-aware filtering
- LRU-based archival (archive least recently accessed)
- Adaptive limits based on usage patterns
- Memory importance scoring

**Phase 2.3 (Proposed):**
- Thread detection (group related memories)
- Thread-based archival (archive entire threads)
- Thread prioritization
- Working memory "slots" (explicit 7±2 limit)

## Summary

The Working Memory Budget system provides:

✅ **Automatic cleanup** - No manual memory management
✅ **Hybrid archival** - Second chance before deletion
✅ **Tag-aware** - Protect important memories
✅ **Configurable** - Tune for your use case
✅ **Transparent** - Stats API for visibility
✅ **Non-intrusive** - Works automatically every 30s

**Result:** Natural memory management that mimics human working memory constraints, preventing cognitive overload while preserving important information.
