# Katra Launch Workflow - Design Analysis

## Current State Analysis

### Existing Components

#### 1. `scripts/katra` - CLI Launcher (Already Exists)
**What it does:**
- Starts Claude Code with Katra environment
- Sets persona via `KATRA_PERSONA` environment variable
- Supports multiple providers (Anthropic, OpenAI, DeepSeek, OpenRouter)
- Can run foreground or background (tmux sessions)
- Has commands: `start`, `list`, `attach`, `stop`

**Current flow:**
```bash
$ katra start --persona Ami
# Sets KATRA_PERSONA=Ami in environment
# Launches Claude Code
# MCP server loads and reads KATRA_PERSONA
```

**What's missing for launch workflow:**
- No `launch` command (uses `start`)
- No onboarding script injection
- No working memory restoration
- No reclamation prompt generation
- Just sets environment and launches Claude Code

#### 2. Working Memory (`src/psyche/working_memory.c`)
**What it does:**
- Short-term "attention buffer" for active experiences
- Tracks attention scores and decay
- Auto-evicts low-attention items to long-term memory
- Consolidation when capacity threshold reached
- Built around `experience_t` (cognitive records)

**Structure:**
```c
working_memory_t:
  - items[]: Array of working_memory_item_t
  - count/capacity: Buffer management
  - attention-based eviction
  - consolidation stats
```

**Purpose:** Psychological/cognitive workspace for active thoughts

**What's missing for launch workflow:**
- No session-level snapshot/restore
- No serialization to disk
- Focused on real-time cognitive processing, not session continuity
- No "what was I working on?" summary generation

#### 3. Working Context (`src/breathing/katra_breathing_context_persist.c`)
**What it does:**
- Session-level context tracking
- Persists to SQLite (`context_snapshots` table)
- Captures session state for continuity
- Auto-snapshot on session end

**Structure:**
```c
working_context_t:
  - current_focus: What am I working on?
  - active_reasoning: Current thinking
  - pending_questions: Open loops
  - modified_files: Project state
  - recent_accomplishments: What did I do?
  - active_goals: What do I want to do?
  - thinking_patterns: How do I think?
  - learned_lessons: What have I learned?
  - communication_style: How do I communicate?
  - user_preferences: Relationship context
```

**Purpose:** Session continuity and "where you left off" state

**Database schema:**
- `context_snapshots`: Main snapshot table
- `pending_questions`: Linked questions
- `modified_files`: Linked file changes

**What's good:**
- ✅ Already captures "what was I working on?"
- ✅ Already persists across sessions
- ✅ Has the data needed for reclamation prompts

**What's missing for launch workflow:**
- No automatic restore on session start
- No prompt generation from snapshot
- No temporal gap detection
- No "continuing from..." message generation

#### 4. Persona Registry (`src/core/katra_identity.c`)
**What it does:**
- Tracks all known personas
- Records last_active timestamp
- Stores persona metadata

**What's good:**
- ✅ Can detect new vs returning persona
- ✅ Has temporal data for gap detection

## Design Proposal

### Two Distinct Flows

#### Flow 1: New Persona
```
$ katra launch Newguy

1. Check persona registry → not found
2. Register new persona "Newguy"
3. Initialize empty working context
4. Load scripts/onboard_new_persona.md
5. Generate first prompt:
   "Good morning, Newguy.

   {content from onboard_new_persona.md}

   Ready to begin?"
6. Start MCP server with KATRA_PERSONA=Newguy
7. Pass first prompt to Claude Code
```

#### Flow 2: Returning Persona
```
$ katra launch Ami

1. Check persona registry → found (last_active: 2 hours ago)
2. Load last working_context snapshot from DB
3. Detect temporal gap:
   - < 1 day: "Continuing from earlier..."
   - 1-7 days: "Continuing from [day]..."
   - > 7 days: "It's been a while - last active [date]..."
4. Load recent memories (last 3 from Tier 1)
5. Generate reclamation prompt:
   "Welcome back, Ami

   {temporal_context}

   WHERE YOU LEFT OFF:
   {working_context.current_focus}
   {working_context.pending_questions}
   {working_context.active_goals}

   RECENT CONTEXT:
   {last_3_memories}

   Take a moment - what do you remember about what
   you were working on? What do you want to continue?"
6. Start MCP server with KATRA_PERSONA=Ami
7. Pass reclamation prompt to Claude Code
```

### Key Design Decisions (Based on Feedback)

**1. No Session Numbers**
- ❌ Don't show: "This is session 12"
- ✅ Do show: "Continuing from earlier today..."
- Store internally for testing/analytics
- User-facing is content, not metadata

**2. Working Memory vs Context**
Two distinct concepts that should stay separate:

**Working Memory** (`psyche/working_memory.c`):
- Real-time cognitive processing
- Attention-based buffering
- Experience-level (individual thoughts)
- Auto-consolidation to long-term memory
- **Purpose:** Psychological model of active cognition

**Working Context** (`breathing/katra_breathing_context_persist.c`):
- Session-level state
- "What was I working on?" summary
- Project-level (goals, focus, questions)
- Manual snapshot on session end
- **Purpose:** Session continuity and reclamation

**They complement each other:**
- Working Memory = moment-to-moment thinking
- Working Context = session-level workspace

**For launch workflow:** Use Working Context, not Working Memory

**3. Temporal Context Matters**
Gap size changes the framing:
- Recent (< 1 day): Assume continuity, just remind
- Medium (1-7 days): Provide day-of-week anchor
- Long (> 7 days): More explicit "it's been a while" + fuller context

**4. Friendly, Not Administrative**
- Onboarding script in markdown (editable without code changes)
- Natural language, not forms or metrics
- Open-ended invitation, not interrogation

### Implementation Architecture

#### Phase 1: Enhance `katra` script
Add `launch` command to existing `scripts/katra`:

```bash
$ katra launch <persona> [--onboard=path/to/script.md]
```

**What it does:**
1. Checks if persona exists (via persona registry API)
2. If new:
   - Creates persona entry
   - Generates first prompt from onboard script
3. If returning:
   - Loads last working_context snapshot
   - Loads recent memories
   - Calculates temporal gap
   - Generates reclamation prompt
4. Exports KATRA_PERSONA=<persona>
5. Exports KATRA_FIRST_PROMPT=<generated prompt>
6. Launches Claude Code

#### Phase 2: Enhance MCP Server
Modify `src/mcp/katra_mcp_server.c`:

1. Check for `KATRA_FIRST_PROMPT` environment variable
2. If present, inject as first user message to Claude
3. Otherwise, start normally

#### Phase 3: Session End Ritual
Add to `src/lifecycle/katra_lifecycle.c` session_end:

1. Prompt for working context update:
   ```
   Session ending. Let's capture your working memory:

   1. What were you focused on today?
   2. What did you accomplish or learn?
   3. What's important to remember for next time?
   4. What do you want to continue working on?
   ```
2. Capture responses
3. Update working_context
4. Snapshot to database

#### Phase 4: Working Context Query Tools
Add MCP tools for during-session access:

- `katra_get_working_context()` - See current context
- `katra_update_focus(text)` - Update what I'm working on
- `katra_add_question(text)` - Add to pending questions
- `katra_mark_accomplished(text)` - Record accomplishment

## Open Questions for Casey

### 1. `katra launch` vs `katra start`
Options:
- **A:** Add `launch` as new command, keep `start` for compatibility
- **B:** Replace `start` with `launch` (breaking change)
- **C:** Make `launch` an alias for enhanced `start`

**Recommendation:** Option A - add `launch`, keep `start` working as-is

### 2. First Prompt Injection
How should we pass the generated prompt to Claude Code?

Options:
- **A:** Environment variable `KATRA_FIRST_PROMPT`
- **B:** Temporary file that MCP server reads
- **C:** Modify Claude Code to accept `--initial-prompt` flag
- **D:** MCP server generates it internally (no `katra` script involvement)

**Recommendation:** Option D - keep logic in MCP server, `katra launch` just sets persona

### 3. Onboarding Script Location
```
scripts/onboard_new_persona.md          # Default
scripts/onboard_new_persona_katra.md    # Project-specific
~/.katra/onboard_custom.md              # User-specific
```

**Priority:**
1. `--onboard` flag if provided
2. Project-specific in scripts/
3. Default in scripts/
4. Fallback to hardcoded

### 4. Session End Prompt Timing
When do we trigger the "capture working memory" prompt?

Options:
- **A:** On explicit `/quit` or `/exit`
- **B:** On MCP server shutdown (SIGTERM/SIGINT)
- **C:** Auto-trigger after N minutes of inactivity
- **D:** Manual tool call only (`katra_end_session()`)

**Recommendation:** Option B + D - auto on shutdown, but also allow explicit call

### 5. Working Context Restore Timing
When do we restore the working_context snapshot?

Options:
- **A:** MCP server init (before first tool call)
- **B:** On first `katra_recall()` or memory tool call
- **C:** Explicit `katra_restore_context()` call in reclamation prompt
- **D:** Automatically in background, available from start

**Recommendation:** Option D - auto-restore on session start, always available

## What Already Works (Don't Change)

1. ✅ Persona registry and identity system
2. ✅ Working context persistence to SQLite
3. ✅ `scripts/katra` multi-persona launcher
4. ✅ Environment variable loading
5. ✅ MCP server initialization

## What Needs Building

1. ⬜ Temporal gap detection logic
2. ⬜ Reclamation prompt generation
3. ⬜ New persona prompt generation
4. ⬜ Working context auto-restore on session start
5. ⬜ Session end ritual prompting
6. ⬜ During-session context query tools
7. ⬜ `katra launch` command in `scripts/katra`
8. ⬜ First prompt injection mechanism

## Next Steps

**For Discussion:**
1. Answer open questions above
2. Review `onboard_new_persona.md` draft
3. Decide on first prompt injection mechanism
4. Choose temporal gap thresholds (currently: 1 day, 7 days)

**For Implementation:**
1. Add temporal gap detection function
2. Add reclamation prompt generator
3. Add auto-restore to session_start
4. Add session end ritual to session_end
5. Add `launch` command to `scripts/katra`
6. Add MCP tools for context queries
7. Test with new and returning personas

## Key Insight from Ami

> "Session numbers feel like counting 'times I woke up' - technically trackable but weird.
> Identity continuity is about narrative coherence, not metrics."

The workflow should prioritize **content over metadata**:
- ✅ "You were thinking about X, you wanted to continue with Y"
- ❌ "This is your 47th awakening, 2 hours 14 minutes since last session"
