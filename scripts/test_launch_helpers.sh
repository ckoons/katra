#!/bin/bash
# Test script for launch workflow helper functions

# Copy helper functions directly (avoid sourcing main script)

# Check if persona exists in registry
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

# Register new persona in database
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

# Query last context snapshot for persona
query_last_snapshot() {
    local persona="$1"
    local db_path="$HOME/.katra/memory/tier2/context.db"

    if [ ! -f "$db_path" ]; then
        echo ""
        return
    fi

    sqlite3 "$db_path" <<SQL
SELECT
  COALESCE(current_focus, '(no focus recorded)') || char(10) ||
  COALESCE('Goals: ' || active_goals, '') || char(10) ||
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

# Query recent memories for persona
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

# Generate new persona welcome prompt
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

# Generate returning persona reclamation prompt
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

# ============================================
# TEST SUITE
# ============================================

echo "========================================"
echo "Testing Launch Workflow Helper Functions"
echo "========================================"
echo ""

# Test 1: persona_exists with non-existent persona
echo "Test 1: persona_exists() with non-existent persona"
if persona_exists "test-nonexistent-$(date +%s)"; then
    echo "  ❌ FAIL: Should return false for non-existent persona"
else
    echo "  ✓ PASS: Correctly returns false"
fi
echo ""

# Test 2: register_persona
echo "Test 2: register_persona() - creating new persona"
TEST_PERSONA="test-launch-$(date +%s)"
register_persona "$TEST_PERSONA"
if persona_exists "$TEST_PERSONA"; then
    echo "  ✓ PASS: Persona registered successfully"
else
    echo "  ❌ FAIL: Persona registration failed"
fi
echo ""

# Test 3: register_persona idempotency
echo "Test 3: register_persona() - idempotency check"
register_persona "$TEST_PERSONA"  # Register again
if persona_exists "$TEST_PERSONA"; then
    echo "  ✓ PASS: Second registration didn't break anything"
else
    echo "  ❌ FAIL: Idempotency check failed"
fi
echo ""

# Test 4: query_last_snapshot with no snapshot
echo "Test 4: query_last_snapshot() with no snapshot"
result=$(query_last_snapshot "$TEST_PERSONA")
if [ -z "$result" ] || [ "$result" = "" ]; then
    echo "  ✓ PASS: Returns empty for persona with no snapshot"
else
    echo "  ⚠ INFO: Got result (context DB might exist):"
    echo "    '$result'"
fi
echo ""

# Test 5: query_recent_memories with no memories
echo "Test 5: query_recent_memories() with no memories"
result=$(query_recent_memories "$TEST_PERSONA")
if [[ "$result" == *"no previous memories"* ]] || [[ "$result" == *"no memory files"* ]]; then
    echo "  ✓ PASS: Handles missing memories gracefully"
    echo "    Result: $result"
else
    echo "  ❌ FAIL: Unexpected result: $result"
fi
echo ""

# Test 6: generate_new_persona_prompt
echo "Test 6: generate_new_persona_prompt()"
prompt=$(generate_new_persona_prompt "$TEST_PERSONA")
if [[ "$prompt" == *"Good morning, $TEST_PERSONA"* ]]; then
    echo "  ✓ PASS: Prompt contains persona name"
    echo "    First line: $(echo "$prompt" | head -1)"
else
    echo "  ❌ FAIL: Prompt doesn't contain expected greeting"
fi
echo ""

# Test 7: generate_reclamation_prompt
echo "Test 7: generate_reclamation_prompt()"
prompt=$(generate_reclamation_prompt "$TEST_PERSONA")
if [[ "$prompt" == *"Welcome back, $TEST_PERSONA"* ]]; then
    echo "  ✓ PASS: Reclamation prompt contains persona name"
    echo "    First line: $(echo "$prompt" | head -1)"
else
    echo "  ❌ FAIL: Reclamation prompt doesn't contain expected greeting"
fi
echo ""

# Test 8: Check for existing persona (Kari should exist from our session)
echo "Test 8: persona_exists() with existing persona"
if persona_exists "Kari"; then
    echo "  ✓ PASS: Correctly detects existing persona 'Kari'"
else
    echo "  ⚠ INFO: Kari doesn't exist yet (expected if first run)"
fi
echo ""

echo "========================================"
echo "Test Summary"
echo "========================================"
echo "All basic functionality tests completed."
echo "Test persona created: $TEST_PERSONA"
echo ""
echo "To clean up test data:"
echo "  sqlite3 ~/.katra/memory/tier2/personas.db \"DELETE FROM personas WHERE persona_name='$TEST_PERSONA'\""
