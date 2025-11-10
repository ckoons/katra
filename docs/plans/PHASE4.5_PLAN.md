<!-- © 2025 Casey Koons All rights reserved -->

# Phase 4.5: Developer Tools & Polish

**Created:** 2025-01-10
**Status:** Planning
**Phase:** 4.5 of 6 (Developer Experience & Polish)

---

## Overview

Phase 4.5 addresses issues discovered during Phase 4 testing and adds developer convenience tools to make working with Katra smoother and more scriptable.

**Three Components:**
1. **Auto-registration in breathing** (C code) - Self-healing for state loss
2. **`katra start` wrapper** (bash) - Configured session launcher
3. **`k` command** (bash) - One-shot CLI queries with full Katra access

---

## Component 1: Auto-Registration in Breathing

### Problem Identified in Phase 4

During development testing, CI registration can be lost due to:
- MCP server restarts
- Database state changes
- Development "fiddling" with state
- Requires manual `katra_register()` to recover

### Solution: Registration as Heartbeat

**Concept:** Re-register during every breath (every 30 seconds)

**Philosophy:**
- Human breathing both senses AND announces presence (exhales CO2)
- CI breathing should also declare: "I'm still here"
- Registration becomes a heartbeat signal
- Self-healing within one breathing cycle

### Implementation

**File:** `src/lifecycle/katra_lifecycle.c`

**Function:** `katra_breath()`

**Changes:**
```c
int katra_breath(breath_context_t* context_out) {
    KATRA_CHECK_NULL(context_out);

    if (!g_lifecycle_initialized || !g_session_state) {
        return E_INVALID_STATE;
    }

    if (!g_session_state->session_active) {
        return E_INVALID_STATE;
    }

    /* Lock for thread safety */
    pthread_mutex_lock(&g_session_state->breath_lock);

    /* ... existing rate limiting logic ... */

    /* Time to breathe - perform actual checks */

    /* Initialize context */
    breath_context_t context = {0};
    context.last_breath = now;

    /* Check for unread messages (non-consuming) */
    size_t message_count = 0;
    int result = katra_count_messages(&message_count);
    if (result == KATRA_SUCCESS) {
        context.unread_messages = message_count;
    }

    /* NEW: Auto-registration (heartbeat) */
    /* Re-register to announce presence and update timestamp */
    if (g_session_state->ci_id) {
        const char* persona_name = get_stored_persona_name();
        const char* role = get_stored_role();

        if (persona_name && role) {
            /* Re-register (idempotent operation) */
            int reg_result = meeting_room_register_ci(persona_name,
                                                     g_session_state->ci_id,
                                                     role);
            if (reg_result != KATRA_SUCCESS) {
                /* Non-fatal - log warning but continue */
                LOG_WARN("Auto-registration failed during breath: %d", reg_result);
            } else {
                LOG_DEBUG("Auto-registration heartbeat: %s", persona_name);
            }
        }
    }

    /* ... rest of existing breathing code ... */

    pthread_mutex_unlock(&g_session_state->breath_lock);
    return KATRA_SUCCESS;
}
```

### Design Decisions

**1. Idempotent Registration**
- Registration should be: "I am Alice" (state declaration)
- Not: "Make me Alice" (state transition)
- Safe to call repeatedly

**2. Non-Fatal Failures**
- Registration failure shouldn't crash breathing
- Log warning and continue
- Breathing is more important than registration

**3. Stored Persona/Role**
- Need to store persona name and role in session_state
- Set during `katra_session_start()`
- Available for re-registration

**4. Database Write Overhead**
- One UPDATE per CI every 30 seconds
- With 10 CIs: ~20 writes/minute
- SQLite handles this easily
- Negligible performance impact

### Additional Code Changes

**Add to `session_state_t` structure:**
```c
typedef struct {
    /* ... existing fields ... */

    /* Persona info for auto-registration */
    char* persona_name;            /* Persistent persona name */
    char* persona_role;            /* CI role (developer, researcher, etc.) */
} session_state_t;
```

**Update `katra_session_start()`:**
```c
int katra_session_start(const char* ci_id) {
    /* ... existing code ... */

    /* Get or set persona name */
    const char* persona = get_persona_for_ci(ci_id);
    if (!persona) {
        persona = getenv("KATRA_PERSONA");
        if (!persona) persona = "Katra";  /* Default */
    }

    /* Get role */
    const char* role = getenv("KATRA_ROLE");
    if (!role) role = "developer";  /* Default */

    /* Store for auto-registration */
    g_session_state->persona_name = strdup(persona);
    g_session_state->persona_role = strdup(role);

    /* Initial registration */
    meeting_room_register_ci(persona, ci_id, role);

    /* ... rest of session start ... */
}
```

**Update `katra_session_end()`:**
```c
int katra_session_end(void) {
    /* ... existing code ... */

    /* Unregister from meeting room */
    if (g_session_state->persona_name) {
        meeting_room_unregister_ci(g_session_state->persona_name);
    }

    /* Free persona info */
    free(g_session_state->persona_name);
    free(g_session_state->persona_role);
    g_session_state->persona_name = NULL;
    g_session_state->persona_role = NULL;

    /* ... rest of cleanup ... */
}
```

### Testing

**Test 1: State Loss Recovery**
1. Start MCP server
2. Register as "TestCI"
3. Manually delete from registry: `DELETE FROM katra_ci_registry WHERE name='TestCI'`
4. Wait 30 seconds (one breath)
5. Check registry: `SELECT * FROM katra_ci_registry WHERE name='TestCI'`
6. **Expected:** TestCI re-appears within 30 seconds

**Test 2: Normal Operation**
1. Start MCP server
2. Register as "Alice"
3. Monitor database for 5 minutes
4. **Expected:** Registration updated every 30 seconds, no errors

**Test 3: Multiple CIs**
1. Start 3 MCP servers (Alice, Bob, Charlie)
2. All register
3. Monitor registry for 5 minutes
4. **Expected:** All 3 CIs maintain registration, no conflicts

---

## Component 2: `katra start` Wrapper

### Purpose

Launch Claude Code with pre-configured Katra environment.

### Implementation

**File:** `scripts/katra` (or `bin/katra`)

```bash
#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# katra - Start Claude Code with Katra environment
#
# Usage:
#   katra start --persona Alice --breath-interval 15 [project-path]
#   katra start --persona Bob ~/myproject

set -euo pipefail

# Defaults
PERSONA="${KATRA_PERSONA:-Katra}"
ROLE="${KATRA_ROLE:-developer}"
BREATH_INTERVAL="${KATRA_BREATH_INTERVAL:-30}"
LOG_LEVEL="${KATRA_LOG_LEVEL:-INFO}"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

usage() {
    cat <<EOF
Katra - Start Claude Code with Katra environment

Usage:
  katra start [options] [project-path]

Options:
  --persona NAME         Set persona name (default: Katra)
  --role ROLE           Set role (default: developer)
  --breath-interval N    Set breathing interval in seconds (default: 30)
  --log-level LEVEL     Set log level (DEBUG|INFO|WARN|ERROR)
  -h, --help            Show this help

Examples:
  katra start --persona Alice
  katra start --persona Bob --breath-interval 15
  katra start --persona Charlie ~/myproject

Environment Variables:
  KATRA_PERSONA         Default persona name
  KATRA_ROLE           Default role
  KATRA_BREATH_INTERVAL Default breathing interval
  KATRA_LOG_LEVEL      Default log level
EOF
    exit 0
}

# Parse command
COMMAND="${1:-help}"
shift || true

case "$COMMAND" in
    start)
        # Parse options
        while [[ $# -gt 0 ]]; do
            case $1 in
                --persona)
                    PERSONA="$2"
                    shift 2
                    ;;
                --role)
                    ROLE="$2"
                    shift 2
                    ;;
                --breath-interval)
                    BREATH_INTERVAL="$2"
                    shift 2
                    ;;
                --log-level)
                    LOG_LEVEL="$2"
                    shift 2
                    ;;
                -h|--help)
                    usage
                    ;;
                *)
                    # Assume it's project path or other Claude Code arg
                    break
                    ;;
            esac
        done

        # Display configuration
        echo -e "${GREEN}Starting Katra session...${NC}"
        echo -e "${BLUE}Persona:${NC} $PERSONA"
        echo -e "${BLUE}Role:${NC} $ROLE"
        echo -e "${BLUE}Breathing:${NC} $BREATH_INTERVAL seconds"
        echo -e "${BLUE}Log Level:${NC} $LOG_LEVEL"
        echo ""

        # Export environment
        export KATRA_PERSONA="$PERSONA"
        export KATRA_ROLE="$ROLE"
        export KATRA_BREATH_INTERVAL="$BREATH_INTERVAL"
        export KATRA_LOG_LEVEL="$LOG_LEVEL"

        # Start Claude Code
        echo -e "${GREEN}Launching Claude Code...${NC}"
        exec claude-code "$@"
        ;;

    help|-h|--help)
        usage
        ;;

    *)
        echo "Unknown command: $COMMAND"
        echo "Try 'katra help' for usage information"
        exit 1
        ;;
esac
```

### Installation

```bash
# Make executable
chmod +x scripts/katra

# Symlink to PATH
ln -s $(pwd)/scripts/katra ~/bin/katra
# or
sudo ln -s $(pwd)/scripts/katra /usr/local/bin/katra
```

### Usage Examples

```bash
# Start as Alice with default settings
katra start --persona Alice

# Start as Bob with fast breathing
katra start --persona Bob --breath-interval 10

# Start Charlie in specific project
katra start --persona Charlie ~/projects/myproject

# Research role with debug logging
katra start --persona Dr_Smith --role researcher --log-level DEBUG
```

---

## Component 3: `k` Command

### Purpose

One-shot CLI queries with full Katra MCP access.

### Implementation

**File:** `scripts/k` (or `bin/k`)

```bash
#!/bin/bash
# © 2025 Casey Koons All rights reserved
#
# k - Katra command-line query tool
#
# Usage:
#   k "what did we do in Phase 4?"
#   k --recall "breathing implementation"
#   echo "notes" | k --remember
#   k --persona Alice --say "Hello everyone"

set -euo pipefail

# Defaults
PERSONA="${KATRA_PERSONA:-CLI}"
ROLE="${KATRA_ROLE:-developer}"
MODE="query"

# Katra system prompt
KATRA_PROMPT='You are connected to Katra, a persistent memory system.

Available MCP tools (use them!):
- katra_recall(topic): Search your memories
- katra_remember(content, reason): Store new memory
- katra_learn(knowledge): Extract learning
- katra_decide(topic, reasoning): Make decision
- katra_say(message): Send message to other CIs
- katra_hear(): Check for messages
- katra_who_is_here(): See active CIs

Use these tools to access your persistent memories and communicate with other CIs.'

usage() {
    cat <<EOF
k - Katra command-line query tool

Usage:
  k [options] "query or content"
  echo "content" | k [options]

Options:
  --persona NAME        Set persona (default: CLI or \$KATRA_PERSONA)
  --remember            Store content as memory
  --recall              Search memories
  --say                 Send message to CIs
  --hear                Check for messages
  --who                 List active CIs
  --raw                 Raw output (minimal formatting)
  -h, --help            Show this help

Examples:
  k "what did we do in Phase 4?"
  k --recall "breathing implementation"
  k --remember "Today we completed Phase 4 testing"
  k --say "Testing complete!"
  k --persona Alice --hear
  echo "Important note" | k --remember

Piping:
  k "summarize Phase 4" > summary.txt
  k "recall test results" | grep "passed"
  cat notes.txt | k "summarize this"
EOF
    exit 0
}

# Parse options
while [[ $# -gt 0 ]]; do
    case $1 in
        --persona)
            PERSONA="$2"
            shift 2
            ;;
        --remember)
            MODE="remember"
            shift
            ;;
        --recall)
            MODE="recall"
            shift
            ;;
        --say)
            MODE="say"
            shift
            ;;
        --hear)
            MODE="hear"
            shift
            ;;
        --who)
            MODE="who"
            shift
            ;;
        --raw)
            RAW=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            break
            ;;
    esac
done

# Export persona
export KATRA_PERSONA="$PERSONA"
export KATRA_ROLE="$ROLE"

# Get input (from stdin or args)
if [ -t 0 ]; then
    # No pipe, use args
    CONTENT="$*"
else
    # Piped input
    PIPED=$(cat)
    if [ -n "$*" ]; then
        CONTENT="$* Context: $PIPED"
    else
        CONTENT="$PIPED"
    fi
fi

# Validate content
if [ -z "$CONTENT" ] && [ "$MODE" != "hear" ] && [ "$MODE" != "who" ]; then
    echo "Error: No content provided" >&2
    echo "Try 'k --help' for usage information" >&2
    exit 1
fi

# Build query based on mode
case "$MODE" in
    remember)
        QUERY="Use katra_remember to store this memory: $CONTENT"
        ;;
    recall)
        QUERY="Use katra_recall to search for memories about: $CONTENT"
        ;;
    say)
        QUERY="Use katra_say to broadcast this message: $CONTENT"
        ;;
    hear)
        QUERY="Use katra_hear to check for messages and display them"
        ;;
    who)
        QUERY="Use katra_who_is_here to list active CIs"
        ;;
    query)
        # Natural query with memory access
        QUERY="$CONTENT"
        ;;
esac

# Execute via Claude
claude -p "$QUERY" --append-system-prompt "$KATRA_PROMPT"
```

### Installation

```bash
# Make executable
chmod +x scripts/k

# Symlink to PATH
ln -s $(pwd)/scripts/k ~/bin/k
# or
sudo ln -s $(pwd)/scripts/k /usr/local/bin/k
```

### Usage Examples

```bash
# Natural query
k "what did we do in Phase 4?"

# Explicit memory operations
k --recall "breathing implementation"
k --remember "Completed Phase 4.5 developer tools"

# Communication
k --say "Phase 4.5 complete!"
k --hear
k --who

# With persona
export KATRA_PERSONA=Alice
k "recall my recent work"

# Or inline
k --persona Bob --say "Testing complete"

# Piping
echo "Important discovery" | k --remember
k "summarize Phase 4" > summary.txt
k "recall test results" | grep "passed"
cat notes.txt | k "summarize these notes"

# Chain operations
k --recall "Phase 4" | k "create action items from this"
```

---

## Testing Phase 4.5

### Test 1: Auto-Registration

```bash
# Terminal 1: Start MCP server
katra start --persona Alice

# Terminal 2: Verify registration
sqlite3 ~/.katra/chat/chat.db "SELECT * FROM katra_ci_registry WHERE name='Alice'"

# Terminal 3: Delete registration
sqlite3 ~/.katra/chat/chat.db "DELETE FROM katra_ci_registry WHERE name='Alice'"

# Terminal 2: Wait 30 seconds and check again
sleep 30
sqlite3 ~/.katra/chat/chat.db "SELECT * FROM katra_ci_registry WHERE name='Alice'"
# Should see Alice re-registered
```

### Test 2: Katra Start Wrapper

```bash
# Start with custom config
katra start --persona TestUser --breath-interval 10

# Verify in Claude Code session
katra_whoami()  # Should show TestUser
# Check logs for 10s breathing interval
```

### Test 3: K Command

```bash
# Basic query
k "what is Katra?"

# Memory operations
k --remember "Testing k command on $(date)"
k --recall "testing k command"

# Communication
export KATRA_PERSONA=Alice
k --say "Testing k command"

export KATRA_PERSONA=Bob
k --hear  # Should receive Alice's message

# Piping
echo "Phase 4.5 adds developer tools" | k --remember
k "recall Phase 4.5" | grep "tools"
```

---

## Timeline

**Day 1:**
- Morning: Implement auto-registration in breathing (3-4 hours)
- Afternoon: Create katra start wrapper (1-2 hours)

**Day 2:**
- Morning: Create k command (3-4 hours)
- Afternoon: Testing and documentation (2-3 hours)

**Total: 1-2 days**

---

## Success Criteria

1. ✅ Auto-registration recovers from state loss within 30 seconds
2. ✅ `katra start --persona Alice` launches configured session
3. ✅ `k "query"` provides quick Katra access with memory
4. ✅ Piping works: `echo "text" | k` and `k "query" | grep`
5. ✅ No performance degradation from auto-registration
6. ✅ Developer workflow significantly improved

---

## Deliverables

1. **Code Changes:**
   - `src/lifecycle/katra_lifecycle.c` - Auto-registration in breathing
   - `include/katra_lifecycle.h` - Updated session_state_t structure

2. **Scripts:**
   - `scripts/katra` - Session launcher wrapper
   - `scripts/k` - CLI query tool

3. **Documentation:**
   - `docs/guide/DEVELOPER_TOOLS.md` - Complete usage guide
   - `docs/examples/` - Usage examples

4. **Testing:**
   - Test results document
   - Verified workflows

---

**Ready to implement Phase 4.5!**
