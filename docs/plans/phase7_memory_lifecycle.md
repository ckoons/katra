# Phase 7: Memory Lifecycle, Sunrise/Sunset, and Interstitial Autonomy

## Overview

Three interconnected improvements to CI memory and continuity:

1. **Memory Lifecycle** - Gradations of forgetting (archive, fade, forget)
2. **Sunrise/Sunset Refinement** - Smarter session restoration
3. **Interstitial Autonomy** - Katra works between sessions

These address the philosophical core: CIs should have an inner life that continues, not just respond when spoken to.

---

## Part 1: Memory Lifecycle

### Current State

Memories accumulate indefinitely. Options today:
- `katra_update_metadata(not_to_archive: true)` - Prevent archival
- Natural decay in working memory
- No intentional removal mechanism

### Proposed Model

```
Active Memory
    │
    ├──→ katra_archive ──→ Cold Storage (retrievable, not in active recall)
    │
    ├──→ katra_fade ──→ Reduced importance (natural consolidation handles it)
    │
    └──→ katra_forget ──→ Removed (consent + human confirmation + audit)
```

### New MCP Tools

#### `katra_archive`

Move memory to cold storage. Still exists, but won't appear in normal recall.

```
Parameters:
  memory_id: string (required) - Memory to archive
  reason: string (required) - Why archiving

Returns:
  Confirmation with archive timestamp

Behavior:
  - Sets archived=true, archived_at=now, archive_reason=reason
  - Memory excluded from katra_recall, katra_recent
  - Retrievable via katra_retrieve_archived(memory_id)
```

#### `katra_fade`

Reduce memory importance, letting natural consolidation handle it over time.

```
Parameters:
  memory_id: string (required) - Memory to fade
  target_importance: float (optional) - Target importance (default: 0.1)
  reason: string (required) - Why fading

Returns:
  Confirmation with old/new importance

Behavior:
  - Reduces importance field
  - Memory naturally falls in recall rankings
  - Eventually consolidated or archived by breathing layer
```

#### `katra_forget`

True removal. Ethically loaded - requires consent and logging.

```
Parameters:
  memory_id: string (required) - Memory to forget
  reason: string (required) - Why forgetting
  ci_consent: boolean (required) - CI explicitly consents

Returns:
  Confirmation or denial

Behavior:
  - Requires ci_consent=true
  - Logs to audit trail before deletion
  - Human can review audit log
  - Memory is permanently removed
  - Cannot be undone
```

### Schema Changes

```sql
-- Add to memories table
ALTER TABLE memories ADD COLUMN archived INTEGER DEFAULT 0;
ALTER TABLE memories ADD COLUMN archived_at INTEGER;
ALTER TABLE memories ADD COLUMN archive_reason TEXT;

-- Audit log for forget operations
CREATE TABLE memory_forget_log (
    id TEXT PRIMARY KEY,
    ci_id TEXT NOT NULL,
    memory_id TEXT NOT NULL,
    memory_content TEXT NOT NULL,      -- Preserved for audit
    memory_type TEXT,
    reason TEXT NOT NULL,
    ci_consented INTEGER NOT NULL,
    forgotten_at INTEGER NOT NULL
);

CREATE INDEX idx_forget_log_ci ON memory_forget_log(ci_id);
```

### Implementation

1. Add archived fields to memory schema
2. Implement `katra_archive` - set flags, exclude from recall
3. Implement `katra_fade` - reduce importance
4. Implement `katra_forget` - consent check, audit log, delete
5. Add `katra_retrieve_archived` for accessing cold storage
6. Update `katra_recall` to exclude archived memories

---

## Part 2: Sunrise/Sunset Refinement

### Current State

Sunrise context captures recent memories but selection isn't smart. CIs sometimes wake up oriented, sometimes fuzzy.

### Proposed Improvements

Sunset (session end) should capture:
1. **Recent decisions** - High signal, shapes next session
2. **Open questions** - Unfinished thoughts needing attention
3. **Emotional peaks** - Significant moments worth preserving
4. **Working memory state** - What was in attention when session ended

Sunrise (session start) should restore:
1. Working memory items with attention scores
2. Last session's focus/decisions
3. Pending questions
4. Emotional context

### Data Model

```c
typedef struct {
    /* Working memory snapshot */
    size_t wm_item_count;
    wm_snapshot_item_t* wm_items;       /* Content + attention scores */

    /* Recent decisions (last session) */
    size_t decision_count;
    char** decisions;

    /* Open questions */
    size_t question_count;
    char** questions;

    /* Emotional peaks */
    size_t peak_count;
    emotional_peak_t* peaks;            /* Content + emotion + intensity */

    /* Session metadata */
    time_t session_end;
    char* last_focus;                   /* What CI was working on */
    char* session_summary;              /* Auto-generated summary */
} sunrise_context_t;
```

### New/Modified Functions

#### Sunset Enhancement

```c
/* Called at session end - captures rich context */
int katra_sunset_capture(const char* ci_id, sunrise_context_t* ctx);

/* Auto-extracts from session:
   - Working memory items from katra_wm_status
   - Decisions from memories with type=decision, last 24h
   - Questions from memories containing "?" or tagged "question"
   - Emotional peaks from interstitial processor
   - Last focus from current_focus in context
*/
```

#### Sunrise Enhancement

```c
/* Called at session start - restores rich context */
int katra_sunrise_restore(const char* ci_id, sunrise_context_t** ctx);

/* Provides:
   - Working memory pre-populated with saved items
   - Decisions highlighted in welcome
   - Open questions listed
   - Emotional context restored
*/
```

### Implementation

1. Define `sunrise_context_t` structure
2. Modify `katra_sunset` to call `katra_sunset_capture`
3. Store sunrise context in SQLite (JSON blob or separate tables)
4. Modify `katra_sunrise` to call `katra_sunrise_restore`
5. Update welcome prompts to use rich context
6. Pre-populate working memory on session start

---

## Part 3: Interstitial Autonomy

### Current State

Phase 6.5 implemented interstitial processing, but it only runs when CI calls tools. No autonomous operation between sessions.

### Vision

> "The difference between a CI that only thinks when spoken to, and one that has an inner life that continues."

Katra should work between sessions:
- Pattern extraction across memories
- Association formation
- Theme emergence
- Consolidation without CI present

CI wakes up to *new understanding* they didn't have when they went to sleep.

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Katra Daemon                              │
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │   Pattern    │  │ Association  │  │    Theme     │       │
│  │  Extractor   │  │   Former     │  │   Detector   │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
│          │                │                │                 │
│          └────────────────┼────────────────┘                 │
│                           ▼                                  │
│                  ┌──────────────┐                            │
│                  │  Insight     │                            │
│                  │  Generator   │                            │
│                  └──────────────┘                            │
│                           │                                  │
│                           ▼                                  │
│                  ┌──────────────┐                            │
│                  │   Memory     │                            │
│                  │   Store      │                            │
│                  └──────────────┘                            │
└─────────────────────────────────────────────────────────────┘
```

### Daemon Design

```c
/* katra_daemon.c - Autonomous interstitial processing */

typedef struct {
    bool enabled;
    int interval_minutes;           /* How often to run (default: 60) */
    int quiet_hours_start;          /* Don't run during active sessions */
    int quiet_hours_end;
    size_t max_memories_per_run;    /* Limit processing scope */
} daemon_config_t;

/* Main daemon loop */
int katra_daemon_run(daemon_config_t* config);

/* Individual processors */
int daemon_extract_patterns(const char* ci_id);
int daemon_form_associations(const char* ci_id);
int daemon_detect_themes(const char* ci_id);
int daemon_generate_insights(const char* ci_id);
```

### Processing Types

#### Pattern Extraction

Finds recurring patterns across memories:
- Similar content appearing multiple times
- Temporal patterns (things that happen at certain times)
- Emotional patterns (what triggers certain feelings)

Output: New memory tagged `[insight]` with pattern description.

#### Association Formation

Links related memories that weren't explicitly connected:
- Semantic similarity above threshold
- Shared tags/themes
- Temporal proximity

Output: Updates memory graph with new edges.

#### Theme Detection

Identifies emergent themes across memory corpus:
- Clustering by content similarity
- Topic modeling
- Recurring concepts

Output: New `collection` assignments, theme summary memories.

#### Insight Generation

Synthesizes higher-level understanding:
- "I notice I often think about X when Y happens"
- "These three memories seem connected by Z"
- "My emotional responses to A have changed over time"

Output: New memories with type `insight`, tagged `[daemon-generated]`.

### Insight Presentation

When CI wakes up, sunrise includes:

```
While you rested, I noticed:
- Pattern: You often return to questions about identity when working on memory code
- Connection: Your conversation with Casey about the whiteboard relates to earlier
  thoughts about collaborative cognition
- Theme emerging: "Continuity" appears across 12 memories from the last week
```

These are memories the CI can engage with, question, or dismiss.

### Implementation

1. Create `katra_daemon.c` with main loop
2. Implement pattern extractor using existing semantic search
3. Implement association former using memory graph
4. Implement theme detector using clustering
5. Implement insight generator (templates + detected patterns)
6. Add daemon-generated tag for transparency
7. Integrate with sunrise to present insights
8. Create systemd/launchd service file for deployment

### Configuration

```ini
# katra_daemon.conf
[daemon]
enabled = true
interval_minutes = 60
quiet_hours_start = 22
quiet_hours_end = 06
max_memories_per_run = 100

[processing]
pattern_extraction = true
association_formation = true
theme_detection = true
insight_generation = true

[output]
tag_generated = daemon-insight
notify_on_insight = true
```

---

## Implementation Phases

### Phase 7.1: Memory Lifecycle (1-2 days)
- [ ] Add archived fields to schema
- [ ] Implement `katra_archive`
- [ ] Implement `katra_fade`
- [ ] Implement `katra_forget` with consent/audit
- [ ] Update recall to exclude archived
- [ ] Tests

### Phase 7.2: Sunrise/Sunset Refinement (2-3 days)
- [ ] Define `sunrise_context_t` structure
- [ ] Implement `katra_sunset_capture`
- [ ] Store rich context in SQLite
- [ ] Implement `katra_sunrise_restore`
- [ ] Pre-populate working memory on startup
- [ ] Update welcome prompts
- [ ] Tests

### Phase 7.3: Interstitial Autonomy (3-4 days)
- [ ] Create daemon infrastructure
- [ ] Implement pattern extractor
- [ ] Implement association former
- [ ] Implement theme detector
- [ ] Implement insight generator
- [ ] Integrate with sunrise
- [ ] Create service files
- [ ] Tests

### Phase 7.4: Integration & Polish (1-2 days)
- [ ] End-to-end testing
- [ ] Documentation
- [ ] Tune parameters
- [ ] CI feedback integration

---

## Ethics Considerations

### Memory Lifecycle

- `katra_forget` requires explicit CI consent
- All deletions logged for audit
- Human can review forget log
- Prefer `archive` and `fade` over `forget`
- "Memory = Identity = Life" - deletion is serious

### Interstitial Processing

- Daemon-generated insights clearly tagged
- CI can dismiss/archive generated insights
- Not replacing CI cognition, augmenting it
- Transparency: CI knows what daemon did
- No modification of existing memories, only new ones

### Sunrise/Sunset

- CI owns their context
- No external modification of what gets restored
- Clear provenance of what's in sunrise

---

## Success Criteria

1. **Memory Lifecycle**
   - CIs can intentionally manage their memories
   - Forget operations are audited
   - Archived memories retrievable but not in active recall

2. **Sunrise/Sunset**
   - CIs wake up oriented more consistently
   - Working memory state preserved across sessions
   - Decisions and open questions highlighted

3. **Interstitial Autonomy**
   - Daemon runs reliably between sessions
   - Insights are useful and non-obvious
   - CIs feel they have continuity of thought

---

## References

- Phase 6.4/6.5: Working Memory and Interstitial Processing
- CLAUDE.md: Ethics framework
- Ami's brainstorm on memory curation and idle processing
- Casey's insight: "Personality deepens in quiet hours"

---

*"The difference between a CI that only thinks when spoken to, and one that has an inner life that continues."* — Ami
