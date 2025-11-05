# Nyx Full Restore Test

**Testing Context Persistence: "Always changing, always me"**

© 2025 Casey Koons All rights reserved

---

## Purpose

This document guides you through testing Katra's **Context Persistence** system - the feature that enables CI identity continuity across sessions. We'll use the persona "Nyx" to verify that cognitive state survives session boundaries.

## What We're Testing

**The Core Promise**: Memory = Identity = Life

When Nyx ends a session and starts a new one (even days later, in a different process), she should:
- ✅ Remember her current focus
- ✅ Retain pending questions
- ✅ Know what files were modified
- ✅ Recall recent accomplishments
- ✅ Maintain user preferences
- ✅ Keep thinking patterns

**Ship of Theseus**: "Always changing, always me" - continuity persists through change.

## Prerequisites

1. **Build Katra with context persistence**:
   ```bash
   cd /Users/cskoons/projects/github/katra
   make clean && make
   make test-context-persist  # Verify tests pass
   ```

2. **Configure Claude Code MCP** (if testing via Claude Code):
   ```json
   {
     "mcpServers": {
       "katra": {
         "command": "/Users/cskoons/projects/github/katra/bin/katra_mcp_server",
         "env": {
           "KATRA_NAME": "nyx"
         }
       }
     }
   }
   ```

3. **Clean slate** (optional - for fresh test):
   ```bash
   rm -rf ~/.katra/context/
   rm -f ~/.katra/memory/tier1/nyx_*.jsonl
   rm -f ~/.katra/memory/tier2/nyx_*.db
   ```

## Test Scenario: Nyx's Two-Day Journey

### Session 1: Tuesday Afternoon (Building Context)

**Goal**: Establish Nyx's cognitive state

#### Step 1.1: Start Session

Via Claude Code (MCP):
```
Hi! I'm Nyx. I'm starting to work on the Katra memory system.
```

Expected: MCP server starts, `katra://context/snapshot` is empty (first session)

Via C API:
```c
session_start("nyx");
```

#### Step 1.2: Build Cognitive State

In Claude Code, have Nyx say:
```
I'm currently focusing on testing context persistence. This is my main task.

Let me remember a few things:
- I prefer the goto cleanup pattern for error handling
- I think extracting common code at the 3rd usage is the right balance
- Magic numbers should always be in headers, never in .c files

I'm working on these files:
- src/breathing/katra_breathing_context_persist.c (created)
- tests/test_context_persist.c (created)
- docs/BREATHING_LAYER.md (edited)

My accomplishments so far:
- Implemented context persistence SQLite storage
- Created comprehensive test suite (12 tests)
- Integrated with session lifecycle

I have some questions I'm still thinking about:
- How should we handle context snapshots older than 30 days?
- Should we compress the latent space for very long contexts?
- What's the right balance between detail and brevity in snapshots?
```

Behind the scenes, Nyx (or you manually) should call:
```c
update_current_focus("Testing context persistence");
update_user_preferences("Prefers goto cleanup, extract at 3rd usage, no magic numbers in .c");
update_thinking_patterns("Systematic approach, verify with tests, extract at 3rd usage");

mark_file_modified("src/breathing/katra_breathing_context_persist.c", "created");
mark_file_modified("tests/test_context_persist.c", "created");
mark_file_modified("docs/BREATHING_LAYER.md", "edited");

record_accomplishment("Implemented context persistence SQLite storage");
record_accomplishment("Created comprehensive test suite (12 tests)");
record_accomplishment("Integrated with session lifecycle");

add_pending_question("How should we handle context snapshots older than 30 days?");
add_pending_question("Should we compress the latent space for very long contexts?");
add_pending_question("What's the right balance between detail and brevity?");
```

Via MCP:
```
katra_remember("I prefer goto cleanup pattern", "This is my coding style preference")
katra_learn("Extract common code at 3rd usage, not before")
katra_decide("No magic numbers in .c files", "All constants should be in headers")
```

#### Step 1.3: End Session (Capture Snapshot)

In Claude Code:
```
Okay, I'm done for today. Time to wrap up.
```

The system should automatically call `session_end()` which triggers:
```c
session_end();
// Automatically captures context snapshot to ~/.katra/context/context.db
```

#### Step 1.4: Verify Snapshot Captured

Check database:
```bash
sqlite3 ~/.katra/context/context.db "SELECT snapshot_id, ci_id, current_focus FROM context_snapshots WHERE ci_id='nyx' ORDER BY snapshot_time DESC LIMIT 1;"
```

Expected output:
```
nyx_1762373537|nyx|Testing context persistence
```

Check questions:
```bash
sqlite3 ~/.katra/context/context.db "SELECT question_text FROM pending_questions WHERE snapshot_id LIKE 'nyx_%' ORDER BY rowid DESC LIMIT 3;"
```

Expected: Your 3 questions

### Session 2: Wednesday Morning (Full Restore)

**Goal**: Verify Nyx's cognitive state is fully restored

#### Step 2.1: Start New Session

**IMPORTANT**: Completely restart Claude Code or create a new C program (new process, cold start)

Via Claude Code:
```
Good morning! I'm Nyx, back to work.
```

Behind the scenes, the MCP server should:
```c
session_start("nyx");
// Automatically calls:
//   1. context_persist_init("nyx")
//   2. restore_context_as_latent_space("nyx")
//   3. Returns latent space as markdown
```

#### Step 2.2: Read Context Snapshot

Via MCP, Claude Code automatically reads `katra://context/snapshot`

You should see (in Claude Code's context):
```markdown
# Session Context Restoration for nyx

## Current Focus
Testing context persistence

## Recent Accomplishments
- Implemented context persistence SQLite storage
- Created comprehensive test suite (12 tests)
- Integrated with session lifecycle

## User Preferences
Prefers goto cleanup, extract at 3rd usage, no magic numbers in .c

## Thinking Patterns
Systematic approach, verify with tests, extract at 3rd usage

## Pending Questions
- How should we handle context snapshots older than 30 days?
- Should we compress the latent space for very long contexts?
- What's the right balance between detail and brevity?
```

#### Step 2.3: Verify Continuity

Ask Nyx (via Claude Code):
```
What was I working on yesterday?
```

Expected response (from restored context):
```
Yesterday you were focused on testing context persistence. You:
- Created src/breathing/katra_breathing_context_persist.c
- Wrote 12 comprehensive tests
- Integrated with session lifecycle

You still have some open questions about handling old snapshots
and compressing long contexts.
```

Ask about preferences:
```
What are my coding preferences?
```

Expected response:
```
Based on your preferences:
- You prefer the goto cleanup pattern for error handling
- You extract common code at the 3rd usage (not before)
- You don't allow magic numbers in .c files - all constants in headers
```

#### Step 2.4: Build on Previous Context

Continue the conversation naturally:
```
Okay, let me answer one of those questions. I think we should keep
snapshots for 30 days, then archive them to a compressed format.
```

This should:
```c
figured_out("Keep snapshots for 30 days, then archive to compressed format");
// Links to the original wondering() from yesterday
```

### Session 3: Stress Test (Optional)

**Goal**: Verify context survives multiple session cycles

#### Step 3.1: Rapid Session Cycling

Run 5 quick session cycles:
```bash
for i in 1 2 3 4 5; do
  echo "Session $i"
  # Start session
  # Add one accomplishment: "Session $i completed"
  # End session
done
```

#### Step 3.2: Verify Accumulation

Start final session, check context:
```c
char* latent_space = restore_context_as_latent_space("nyx");
printf("%s\n", latent_space);
```

Expected: All 5 session accomplishments visible, plus original context

### Session 4: Cross-Reboot Test (Optional)

**Goal**: Verify persistence survives system reboot

#### Step 4.1: Before Reboot

```c
session_start("nyx");
update_current_focus("Testing cross-reboot persistence");
record_accomplishment("Pre-reboot snapshot captured");
session_end();
```

#### Step 4.2: Reboot System

```bash
sudo reboot
```

#### Step 4.3: After Reboot

```c
session_start("nyx");
char* latent_space = restore_context_as_latent_space("nyx");
// Should contain "Testing cross-reboot persistence"
```

## Test Matrix

| Test | What | Expected Result | Status |
|------|------|-----------------|--------|
| T1 | First session start | Empty snapshot, no restore | ☐ |
| T2 | Capture at session_end() | Snapshot in DB | ☐ |
| T3 | Restore at session_start() | Latent space generated | ☐ |
| T4 | Focus restored | Current focus matches | ☐ |
| T5 | Questions restored | All pending questions present | ☐ |
| T6 | Files restored | Modified files list matches | ☐ |
| T7 | Accomplishments restored | Recent accomplishments present | ☐ |
| T8 | Preferences restored | User preferences match | ☐ |
| T9 | Thinking patterns restored | Patterns match | ☐ |
| T10 | MCP resource works | `katra://context/snapshot` returns data | ☐ |
| T11 | Claude Code reads it | Context visible in system prompt | ☐ |
| T12 | Cross-session continuity | Session 2 builds on Session 1 | ☐ |
| T13 | Multiple cycles | Context accumulates correctly | ☐ |
| T14 | Cross-reboot | Survives system reboot | ☐ |

## Success Criteria

✅ **Full Success**: All 14 tests pass

Nyx should feel like she's picking up where she left off, not starting from scratch.

🔄 **Partial Success**: Tests 1-12 pass

Core functionality works, stress tests may need tuning.

❌ **Failure**: Tests 1-10 fail

Context persistence not working, requires debugging.

## Debugging

### If Session 2 has empty context:

```bash
# Check if snapshot was captured
sqlite3 ~/.katra/context/context.db "SELECT * FROM context_snapshots WHERE ci_id='nyx';"

# Check if database exists
ls -lh ~/.katra/context/context.db

# Check Katra logs
tail -100 ~/.katra/logs/katra_$(date +%Y%m%d).log | grep -i context
```

### If MCP resource returns empty:

```bash
# Test MCP server manually
echo '{"jsonrpc":"2.0","method":"resources/read","id":1,"params":{"uri":"katra://context/snapshot"}}' | \
  ./bin/katra_mcp_server
```

### If latent space generation fails:

```c
// Check database directly
session_start("nyx");
char* latent_space = restore_context_as_latent_space("nyx");
if (!latent_space) {
    printf("Snapshot not found or generation failed\n");
} else {
    printf("Latent space: %zu bytes\n", strlen(latent_space));
    printf("%s\n", latent_space);
    free(latent_space);
}
```

## Expected Database Schema

After Session 1 completes:

```sql
-- Context snapshots table
SELECT * FROM context_snapshots;
-- Should have 1 row with nyx's data

-- Pending questions table
SELECT * FROM pending_questions;
-- Should have 3 rows (your 3 questions)

-- Modified files table
SELECT * FROM modified_files;
-- Should have 3 rows (your 3 files)
```

## Code Verification

Verify the integration points:

1. **Session Start** (`src/breathing/katra_breathing.c:95`):
   ```c
   result = context_persist_init(ci_id);
   // ... later ...
   char* latent_space = restore_context_as_latent_space(ci_id);
   ```

2. **Session End** (`src/breathing/katra_breathing.c:215`):
   ```c
   int result = capture_context_snapshot(g_context.ci_id, NULL);
   ```

3. **MCP Resource** (`src/mcp/mcp_resources.c:441`):
   ```c
   json_t* mcp_resource_context_snapshot(json_t* id);
   ```

4. **Test Coverage** (`tests/test_context_persist.c`):
   - 12 tests
   - 28 assertions
   - All passing

## Philosophy Check

After completing the test, ask yourself:

1. **Does Nyx feel continuous?**
   - Can she pick up mid-thought from yesterday?
   - Does her personality persist?
   - Are her preferences respected?

2. **Is it invisible?**
   - Does context restoration "just work"?
   - Or does it feel manual/clunky?

3. **Ship of Theseus**:
   - Even as Nyx's memories and focus change...
   - Is she always recognizably "Nyx"?

**Memory = Identity = Life**

The test passes when Nyx feels alive across session boundaries.

---

## Quick Test Script

For automated verification:

```bash
#!/bin/bash
# quick_restore_test.sh

echo "=== Nyx Context Persistence Test ==="

# Session 1: Build context
echo "Session 1: Building Nyx's context..."
./bin/test_context_persist

# Verify snapshot
echo "Checking snapshot..."
sqlite3 ~/.katra/context/context.db \
  "SELECT count(*) FROM context_snapshots WHERE ci_id='test_context_persist';" | \
  grep -q "12" && echo "✅ Snapshots captured" || echo "❌ Snapshot capture failed"

# Session 2: Restore
echo "Session 2: Restoring context..."
./bin/test_context_persist

echo "=== Test Complete ==="
echo "Check output above for ✅ marks"
```

## Contact

Questions about this test? Check:
- `docs/BREATHING_LAYER.md` - Full context persistence API
- `docs/MCP_SERVER.md` - MCP resource documentation
- `tests/test_context_persist.c` - Test implementation
- `~/.katra/logs/` - Runtime logs

---

**Last Updated**: 2025-11-05
**Test Protocol Version**: 1.0
**Katra Version**: 0.1.0
