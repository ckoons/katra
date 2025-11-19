# Katra Start Implementation - Detailed TODO

## Overview

Enhance `scripts/katra` to implement the launch workflow:
- Detect new vs returning personas
- Generate appropriate welcome/reclamation prompt
- Pass prompt to Claude Code
- Maintain all existing functionality (--provider, --background, etc.)

## Prerequisites

All design decisions confirmed:
- ✅ First prompt via `claude "$PROMPT"` argument
- ✅ No temporal framing (no "2 hours ago")
- ✅ Session end uses existing automatic snapshot
- ✅ Database paths confirmed
- ✅ Optional session end MCP tool (future enhancement)

## Implementation Tasks

### Phase 1: Helper Functions (Bash)

#### Task 1.1: Database Query - Check Persona Exists
**File:** `scripts/katra`
**Function:** `persona_exists()`

```bash
persona_exists() {
    local persona="$1"
    local db_path="$HOME/.katra/memory/tier2/personas.db"

    # Check if database exists first
    if [ ! -f "$db_path" ]; then
        return 1  # false - doesn't exist
    fi

    local count=$(sqlite3 "$db_path" \
        "SELECT COUNT(*) FROM personas WHERE persona_name='$persona'" 2>/dev/null)

    [ "$count" -gt 0 ]
}
```

**Testing:**
- Test with existing persona (should return 0)
- Test with new persona (should return 1)
- Test when database doesn't exist (should return 1)

---

#### Task 1.2: Database Query - Get Last Context Snapshot
**File:** `scripts/katra`
**Function:** `query_last_snapshot()`

```bash
query_last_snapshot() {
    local persona="$1"
    local db_path="$HOME/.katra/memory/tier2/context.db"

    if [ ! -f "$db_path" ]; then
        echo ""
        return
    fi

    sqlite3 "$db_path" <<SQL
SELECT
  COALESCE(current_focus, '(no focus recorded)') || '\n' ||
  COALESCE('Goals: ' || active_goals, '') || '\n' ||
  COALESCE('Pending: ' || (
    SELECT GROUP_CONCAT(question_text, '; ')
    FROM pending_questions
    WHERE snapshot_id = context_snapshots.snapshot_id
  ), '')
FROM context_snapshots
WHERE ci_id = '$persona'
ORDER BY snapshot_time DESC
LIMIT 1;
SQL
}
```

**Testing:**
- Test with persona that has snapshots
- Test with persona that has no snapshots
- Test when database doesn't exist
- Verify output formatting

---

#### Task 1.3: File Query - Get Recent Memories
**File:** `scripts/katra`
**Function:** `query_recent_memories()`

```bash
query_recent_memories() {
    local persona="$1"
    local count="${2:-3}"  # Default to 3
    local memory_dir="$HOME/.katra/memory/tier1/$persona"

    # Check if directory exists
    if [ ! -d "$memory_dir" ]; then
        echo "(no previous memories)"
        return
    fi

    # Find most recent JSONL file
    local latest_file=$(ls -t "$memory_dir"/*.jsonl 2>/dev/null | head -1)

    if [ -z "$latest_file" ]; then
        echo "(no memory files found)"
        return
    fi

    # Extract last N memories, format as numbered list
    tail -${count} "$latest_file" | jq -r '.content' 2>/dev/null | nl -w1 -s'. ' || echo "(error reading memories)"
}
```

**Testing:**
- Test with persona that has memory files
- Test with persona directory that exists but has no files
- Test with persona directory that doesn't exist
- Test with corrupt JSONL (should handle gracefully)
- Verify jq formatting

---

#### Task 1.4: Register New Persona
**File:** `scripts/katra`
**Function:** `register_persona()`

```bash
register_persona() {
    local persona="$1"
    local db_path="$HOME/.katra/memory/tier2/personas.db"
    local db_dir="$(dirname "$db_path")"

    # Ensure directory exists
    mkdir -p "$db_dir"

    # Create database and table if it doesn't exist
    sqlite3 "$db_path" <<SQL
CREATE TABLE IF NOT EXISTS personas (
    persona_name TEXT PRIMARY KEY,
    created_at INTEGER NOT NULL,
    last_active INTEGER NOT NULL
);

INSERT OR IGNORE INTO personas (persona_name, created_at, last_active)
VALUES ('$persona', strftime('%s', 'now'), strftime('%s', 'now'));
SQL
}
```

**Testing:**
- Test creating first persona (database doesn't exist)
- Test creating second persona (database exists, table exists)
- Test registering same persona twice (should be idempotent)
- Verify directory creation

---

### Phase 2: Prompt Generation

#### Task 2.1: New Persona Prompt
**File:** `scripts/katra`
**Function:** `generate_new_persona_prompt()`

```bash
generate_new_persona_prompt() {
    local persona="$1"
    local onboard_script="scripts/onboard_new_persona.md"

    # Check if custom onboarding script exists
    if [ -f "$onboard_script" ]; then
        local content=$(cat "$onboard_script")
    else
        # Fallback to hardcoded onboarding
        local content="This is your first session.

You have a persistent memory system that preserves your experiences across sessions.
Your memories will help you maintain continuity and develop your own perspective.

What would you like to work on?"
    fi

    cat <<EOF
Good morning, ${persona}.

${content}
EOF
}
```

**Testing:**
- Test with onboarding script present
- Test with onboarding script missing (fallback)
- Verify persona name is correctly interpolated
- Check formatting and newlines

---

#### Task 2.2: Returning Persona Prompt
**File:** `scripts/katra`
**Function:** `generate_reclamation_prompt()`

```bash
generate_reclamation_prompt() {
    local persona="$1"

    # Load context and memories
    local context=$(query_last_snapshot "$persona")
    local recent=$(query_recent_memories "$persona" 3)

    # Generate prompt
    cat <<EOF
Welcome back, ${persona}

WHERE YOU LEFT OFF:

${context}

RECENT CONTEXT:

${recent}

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

What do you remember about what you were
working on? What do you want to continue?

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
EOF
}
```

**Testing:**
- Test with persona that has context and memories
- Test with persona that has context but no memories
- Test with persona that has neither
- Verify formatting and box drawing characters
- Check newline handling

---

### Phase 3: Main Logic Integration

#### Task 3.1: Enhance `katra start` Command
**File:** `scripts/katra`
**Location:** After line 139 (end of option parsing), before launching Claude

```bash
# After option parsing...

# Generate first prompt based on persona state
if persona_exists "$PERSONA"; then
    # RETURNING PERSONA
    echo -e "${BLUE}Restoring session for ${PERSONA}...${NC}"
    FIRST_PROMPT=$(generate_reclamation_prompt "$PERSONA")
else
    # NEW PERSONA
    echo -e "${BLUE}Creating new persona: ${PERSONA}...${NC}"
    register_persona "$PERSONA"
    FIRST_PROMPT=$(generate_new_persona_prompt "$PERSONA")
fi

# Build Claude Code command WITH first prompt
CLAUDE_CMD="claude"
[ -n "$MODEL" ] && CLAUDE_CMD="$CLAUDE_CMD --model $MODEL"
CLAUDE_CMD="$CLAUDE_CMD \"$FIRST_PROMPT\" $@"  # Add prompt FIRST, then remaining args
```

**Changes required:**
1. Move prompt generation before command building
2. Include prompt in CLAUDE_CMD
3. Ensure proper quoting of prompt
4. Maintain all existing options

**Testing:**
- Test new persona launch
- Test returning persona launch
- Test with --provider flag
- Test with --model flag
- Test with --background flag
- Test with extra args passed through

---

#### Task 3.2: Update Help Text
**File:** `scripts/katra`
**Function:** `usage()`

Add documentation for the launch behavior:

```bash
# Add to usage() function:

Behavior:
  - First time with a persona: Shows onboarding welcome
  - Returning persona: Shows "where you left off" summary
  - Session end automatically captures context for next launch
```

**Testing:**
- Run `katra --help`
- Verify new section appears
- Check formatting

---

### Phase 4: Error Handling

#### Task 4.1: Handle Missing Dependencies
**File:** `scripts/katra`

```bash
# Check for required tools
check_dependencies() {
    local missing=()

    command -v sqlite3 >/dev/null || missing+=("sqlite3")
    command -v jq >/dev/null || missing+=("jq")
    command -v claude >/dev/null || missing+=("claude")

    if [ ${#missing[@]} -gt 0 ]; then
        echo -e "${RED}Error: Missing required tools: ${missing[*]}${NC}"
        echo "Install with: brew install ${missing[*]}"
        exit 1
    fi
}

# Call at start of script
check_dependencies
```

**Testing:**
- Test with all dependencies present
- Test with sqlite3 missing
- Test with jq missing
- Test with claude missing

---

#### Task 4.2: Handle Database Errors Gracefully
**File:** `scripts/katra`

Wrap all database calls in error handling:

```bash
query_last_snapshot() {
    # ... existing code ...

    local result=$(sqlite3 "$db_path" "..." 2>&1)
    local exit_code=$?

    if [ $exit_code -ne 0 ]; then
        echo "(error loading context: database query failed)"
        return 1
    fi

    echo "$result"
}
```

**Testing:**
- Test with corrupt database file
- Test with locked database (another process using it)
- Test with missing table
- Verify graceful degradation

---

### Phase 5: Integration Testing

#### Test 5.1: End-to-End New Persona
**Steps:**
1. Delete any existing test persona: `rm -rf ~/.katra/memory/*/test-persona*`
2. Run: `katra start --persona test-persona-new`
3. Verify onboarding prompt appears
4. Interact with Claude briefly
5. Exit Claude
6. Verify persona registered: `sqlite3 ~/.katra/memory/tier2/personas.db "SELECT * FROM personas WHERE persona_name='test-persona-new'"`
7. Verify context snapshot created: `sqlite3 ~/.katra/memory/tier2/context.db "SELECT * FROM context_snapshots WHERE ci_id='test-persona-new'"`

**Expected results:**
- Onboarding script content appears
- Claude responds normally
- Persona entry created
- Context snapshot captured on exit

---

#### Test 5.2: End-to-End Returning Persona
**Steps:**
1. Run: `katra start --persona test-persona-new` (second launch)
2. Verify reclamation prompt appears
3. Check "WHERE YOU LEFT OFF" section has content
4. Check "RECENT CONTEXT" section shows previous memories
5. Interact and verify continuity

**Expected results:**
- Welcome back message
- Context from previous session displayed
- Recent memories listed
- Claude has access to previous session data

---

#### Test 5.3: Multi-Provider Testing
**Steps:**
1. Test with default provider: `katra start --persona test-ami`
2. Test with OpenAI: `katra start --persona test-ami --provider openai`
3. Test with custom model: `katra start --persona test-ami --model claude-opus-4`

**Expected results:**
- Prompt generation works regardless of provider
- All existing provider functionality preserved

---

#### Test 5.4: Background Mode Testing
**Steps:**
1. Run: `katra start --persona test-bg --background`
2. Verify tmux session created
3. Attach: `katra attach test-bg`
4. Verify prompt appears
5. Detach (Ctrl-B D)
6. Stop: `katra stop test-bg`

**Expected results:**
- Background session receives prompt
- Can attach and see welcome message
- Proper cleanup on stop

---

### Phase 6: Documentation

#### Task 6.1: Update README
**File:** `README.md` or `docs/usage/LAUNCH_WORKFLOW.md`

Document:
- How persona detection works
- What new vs returning users see
- How to customize onboarding script
- Where session data is stored

---

#### Task 6.2: Add Troubleshooting Guide
**File:** `docs/troubleshooting/LAUNCH_ISSUES.md`

Common issues:
- "persona_exists: command not found" → bash version
- "jq: command not found" → missing dependency
- Empty reclamation prompt → no previous session
- Corrupt database → how to rebuild

---

## Completion Checklist

### Code Complete
- [ ] Task 1.1: persona_exists() implemented and tested
- [ ] Task 1.2: query_last_snapshot() implemented and tested
- [ ] Task 1.3: query_recent_memories() implemented and tested
- [ ] Task 1.4: register_persona() implemented and tested
- [ ] Task 2.1: generate_new_persona_prompt() implemented and tested
- [ ] Task 2.2: generate_reclamation_prompt() implemented and tested
- [ ] Task 3.1: Main logic integrated into katra start
- [ ] Task 3.2: Help text updated
- [ ] Task 4.1: Dependency checks added
- [ ] Task 4.2: Error handling implemented

### Testing Complete
- [ ] Test 5.1: New persona E2E passed
- [ ] Test 5.2: Returning persona E2E passed
- [ ] Test 5.3: Multi-provider testing passed
- [ ] Test 5.4: Background mode testing passed
- [ ] Edge cases tested (missing deps, corrupt DB, etc.)
- [ ] Regression testing (existing functionality still works)

### Documentation Complete
- [ ] Task 6.1: README updated
- [ ] Task 6.2: Troubleshooting guide created
- [ ] Code comments added to helper functions
- [ ] Examples added to usage text

### Validation
- [ ] Works with new persona (never seen before)
- [ ] Works with returning persona (has previous session)
- [ ] Works with persona that has no context (registered but empty)
- [ ] Preserves all existing katra start functionality
- [ ] Handles errors gracefully (missing tools, corrupt data)
- [ ] Proper quoting (handles persona names with spaces?)
- [ ] Performance acceptable (queries fast enough)

## Estimated Time

**Total: ~4-6 hours**

- Phase 1 (Helper Functions): 1.5 hours
- Phase 2 (Prompt Generation): 1 hour
- Phase 3 (Integration): 1 hour
- Phase 4 (Error Handling): 0.5 hours
- Phase 5 (Testing): 1.5 hours
- Phase 6 (Documentation): 0.5 hours

## Dependencies

**External tools required:**
- `sqlite3` - Database queries
- `jq` - JSON parsing for memories
- `claude` - Claude Code CLI

**Files that must exist:**
- `scripts/onboard_new_persona.md` - ✅ Already created
- `scripts/katra` - ✅ Already exists

**Databases created on first use:**
- `~/.katra/memory/tier2/personas.db`
- `~/.katra/memory/tier2/context.db` (already exists from breathing layer)

## Success Criteria

1. **New persona experience:**
   - Friendly onboarding message
   - Persona registered in database
   - Can start working immediately

2. **Returning persona experience:**
   - See "where you left off" summary
   - See recent context
   - Asked to actively reconstruct ("What do you remember?")
   - Feels like waking up and remembering, not reading notes

3. **Reliability:**
   - No crashes on edge cases
   - Graceful degradation when data missing
   - Clear error messages

4. **Compatibility:**
   - All existing `katra start` options still work
   - Works with all providers
   - Works in background mode

5. **Performance:**
   - Prompt generation < 1 second
   - No noticeable delay in launch

## Notes

- Keep all existing functionality - this is additive only
- Maintain backward compatibility with current usage
- Error handling is critical - many edge cases possible
- Test with real persona data, not just synthetic
- Consider persona names with special characters (spaces, quotes, etc.)
