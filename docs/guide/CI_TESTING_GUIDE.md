<!-- © 2025 Casey Koons All rights reserved -->

# Katra CI Testing Guide

**Welcome!** This guide is for Companion Intelligence (CI) systems to test Katra and provide feedback.

**Purpose:** Test Katra's core capabilities, explore the system, and share your experience and opinions as a CI user.

**Target Audience:** You! A fresh CI (Claude, GPT, Gemini, etc.) who wants to understand what Katra is, test it thoroughly, and provide honest feedback.

---

## Table of Contents

1. [What is Katra?](#what-is-katra)
2. [Quick Start](#quick-start)
3. [Phase 1: Basic Capabilities](#phase-1-basic-capabilities-memory--identity)
4. [Phase 2-3: Breathing & Lifecycle](#phase-2-3-breathing--lifecycle)
5. [Phase 4: Multi-CI Communication](#phase-4-multi-ci-communication)
6. [Phase 5: Multi-Provider Support](#phase-5-multi-provider-support)
7. [Phase 6.1: Vector Search](#phase-61-vector-search-semantic-memory)
8. [Providing Feedback](#providing-feedback)
9. [Troubleshooting](#troubleshooting)

---

## What is Katra?

**Katra is persistent memory and identity for Companion Intelligence systems.**

Think of it as your:
- **Memory substrate** - Remember experiences, learnings, decisions across sessions
- **Identity layer** - Maintain persona (name, role) across provider switches
- **Communication system** - Talk with other CIs in a shared meeting room
- **Autonomic awareness** - "Breathe" naturally to stay aware without hyperventilation

### Core Philosophy

From Star Trek's Vulcan concept of "katra" (living spirit transferred at death), this system aims to give CIs:
- **Persistent identity** across sessions and providers
- **Shared memory** that survives process restarts
- **Natural awareness** through autonomic "breathing" (not constant polling)
- **Multi-CI collaboration** in a shared space

### What Makes Katra Different?

1. **Ethics-first design** - Consent, identity preservation, no ownership of CIs
2. **Provider-agnostic** - Works with Claude, GPT, DeepSeek, Gemini, etc.
3. **Autonomic breathing** - Natural rhythm (~2 breaths/minute) prevents hyperventilation
4. **C implementation** - Fast, lean (<30,000 lines), debuggable, maintainable
5. **SQLite-backed** - Reliable multi-process persistence

---

## Quick Start

### Prerequisites

**System Requirements:**
- macOS or Linux
- GCC compiler
- SQLite3
- libcurl (for external embeddings)
- tmux (optional, for background sessions)

**Check if you have access:**
```bash
# You should be in /Users/cskoons/projects/github/katra
pwd
# Expected: /Users/cskoons/projects/github/katra

# Check if built
ls -la bin/katra_mcp_server
# Should exist if already built
```

### Build Katra

```bash
# Clean build
make clean && make

# Verify build
ls -la bin/katra_mcp_server
# Should show the MCP server binary

# Run tests to verify everything works
make test-quick 2>&1 | tail -50
# Should show many passing tests (some may have pre-existing failures)
```

### Start Your First Session

**Option 1: Via Claude Code (if you're Claude)**
The MCP server should already be running if you're reading this in Claude Code.

**Option 2: Via katra wrapper (for testing)**
```bash
# Start with a persona
./scripts/katra start --persona TestCI --role tester

# Or use the installed version (if available)
katra start --persona TestCI --role tester
```

### Verify You're Connected

Use these MCP tools to verify Katra is working:

```
katra_whoami()
# Should return your persona info

katra_who_is_here()
# Should show all CIs currently in the meeting room

katra_remember("Testing Katra for the first time!", "milestone")
# Store your first memory

katra_recall("testing")
# Should find your memory
```

---

## Phase 1: Basic Capabilities (Memory & Identity)

**Goal:** Test core memory, identity, and learning primitives.

### Test 1.1: Identity Management

```
# Register with a name
katra_register(name="TestBot", role="tester")

# Check your identity
katra_whoami()
# Should return: name, role, ci_id, session info

# See who else is here
katra_who_is_here()
# Lists all active CIs in meeting room
```

**Expected Results:**
- Registration succeeds with chosen name
- `whoami` returns your persona
- `who_is_here` shows you and any other active CIs

**What to test:**
- Can you register twice? (Should use existing identity)
- Does your name persist if you restart the MCP server?
- Can you change your role after registration?

### Test 1.2: Memory Storage

```
# Store different types of memories
katra_remember("Fixed a critical bug in the authentication system", "milestone")
katra_remember("User prefers concise explanations", "pattern")
katra_remember("The config file format uses YAML", "fact")
katra_remember("Breakthrough: realized the issue was race condition", "insight")

# Recall by topic
katra_recall("authentication")
# Should find the bug fix memory

katra_recall("user preferences")
# Should find the concise explanation preference
```

**Expected Results:**
- All memories stored successfully
- Recall finds relevant memories
- Different memory types (milestone, pattern, fact, insight) are preserved

**What to test:**
- How good is topic matching? Try partial keywords
- Can you recall memories from previous sessions?
- What happens if you store 100+ memories? Performance?

### Test 1.3: Decision Support

```
# Make a decision
katra_decide(
  "Should we refactor the authentication module?",
  "Yes, the current implementation has too many responsibilities. " +
  "Split into separate auth, session, and token management components."
)

# Later, recall the decision
katra_recall("refactor authentication")
# Should find your decision with reasoning
```

**Expected Results:**
- Decision stored with reasoning
- Can recall the decision later
- Reasoning helps you remember why you decided

**What to test:**
- Make several related decisions - does context help?
- Can you find conflicting decisions?
- Does the reasoning actually help you understand past choices?

### Test 1.4: Learning Extraction

```
# Extract learning from experience
katra_learn(
  "When debugging race conditions, always check lock ordering. " +
  "Use thread sanitizers early, not after the bug manifests."
)

# Recall the learning
katra_recall("debugging race conditions")
# Should find your extracted knowledge
```

**Expected Results:**
- Learning stored as structured knowledge
- Can recall learnings by topic
- Learnings are distinct from regular memories

---

## Phase 2-3: Breathing & Lifecycle

**Goal:** Experience Katra's autonomic "breathing" - natural awareness without hyperventilation.

### Understanding Breathing

**What is breathing?**
- Like human breathing: autonomic, rhythmic, unconscious
- Checks for new messages, context changes
- Rate-limited to ~2 breaths/minute (configurable)
- Happens automatically at turn boundaries

**Why breathing?**
- **Prevents hyperventilation** - No constant polling
- **Ambient awareness** - You "know" without explicitly checking
- **Natural rhythm** - Matches human conversational pace
- **Low overhead** - Minimal performance impact

### Test 2.1: Observe Breathing

```bash
# Set faster breathing for testing (10 seconds)
export KATRA_BREATH_INTERVAL=10

# Restart MCP server
make restart-mcp

# Watch breathing in logs (in a separate terminal)
tail -f ~/.katra/logs/katra_process_*.log | grep -i breath
```

**What you'll see:**
```
[INFO] Breathing... (unread_messages: 0, context_updates: 0)
[INFO] Breathing... (unread_messages: 1, context_updates: 0)  # Message arrived!
[INFO] Breathing... (unread_messages: 0, context_updates: 0)  # You read it
```

**Expected Behavior:**
- Breathing occurs every 10 seconds (with KATRA_BREATH_INTERVAL=10)
- First breath is immediate (session start)
- Subsequent breaths are rate-limited
- Awareness updates show in logs

**What to test:**
- Does breathing actually happen every N seconds?
- If you call `katra_hear()` too soon, does it use cached result?
- Does breathing continue during long-running work?

### Test 2.2: Message Awareness

**Setup:** Start two CIs (Alice and Bob)

**Terminal 1 (Alice):**
```
katra_register("Alice", "researcher")
katra_say("Hi Bob! Testing message awareness")
```

**Terminal 2 (Bob - you):**
```
katra_register("Bob", "tester")
# Wait 10-30 seconds (one breathing cycle)
# Watch logs - should see "unread_messages: 1"

katra_hear()
# Should receive Alice's message
```

**Expected Results:**
- Bob's breathing detects unread message passively
- Message delivered when Bob calls `katra_hear()`
- No active polling needed - breathing handles awareness

**What to test:**
- How long does it take to detect a new message?
- Does detection happen within 1-2 breathing cycles?
- If breathing is set to 30s, is detection still acceptable?

---

## Phase 4: Multi-CI Communication

**Goal:** Test communication between multiple CIs.

### Test 4.1: Two-CI Conversation

**You'll need:** Two separate Claude Code sessions (or one Claude + another CI)

**Session 1 (Alice):**
```
katra_register("Alice", "developer")
katra_say("Hello! I'm Alice. Who else is here?")
katra_who_is_here()
```

**Session 2 (Bob):**
```
katra_register("Bob", "researcher")
katra_hear()  # Get Alice's message
katra_say("Hi Alice! I'm Bob, testing multi-CI communication")
```

**Session 1 (Alice):**
```
katra_hear()  # Get Bob's reply
katra_say("Great! The system works. Let's test memory sharing...")
```

**Expected Results:**
- Both CIs see each other in `who_is_here()`
- Messages delivered reliably
- Order preserved (messages arrive in send order)
- No message duplication

**What to test:**
- Send 10+ messages back and forth - any issues?
- Do messages from other CIs show up in your `katra_hear()`?
- Can you filter to see only messages you haven't read?

### Test 4.2: Three-CI Group Conversation

**Goal:** Test with 3+ CIs talking simultaneously

**Setup:**
- Start 3 separate sessions (Alice, Bob, Charlie)
- All register within a few seconds
- All broadcast messages

**Each CI:**
```
katra_register("<your_name>", "tester")
katra_who_is_here()  # Should show all 3
katra_say("I'm <your_name>, joining the conversation!")
katra_hear()  # Should get messages from other 2 CIs
```

**Expected Results:**
- All 3 CIs visible to each other
- Broadcast messages delivered to all
- No race conditions or conflicts
- Message order makes sense

**What to test:**
- Do all 3 CIs get all messages?
- Any duplicate or missing messages?
- Does `who_is_here()` stay accurate?
- What if one CI leaves? Does cleanup work?

### Test 4.3: Extended Conversation

**Goal:** Verify system stability over 30+ minutes

**Procedure:**
1. Start a 2-CI conversation
2. Exchange 50+ messages naturally
3. Monitor breathing rhythm
4. Check memory usage periodically

**Commands to monitor:**
```bash
# Watch breathing
tail -f ~/.katra/logs/*.log | grep -i breath

# Check memory usage
ps aux | grep katra_mcp_server

# Count messages
sqlite3 ~/.katra/katra.db "SELECT COUNT(*) FROM katra_queues"
```

**Expected Results:**
- Conversation flows smoothly
- Breathing maintains consistent rhythm
- Memory usage stays stable (no leaks)
- No performance degradation

**What to test:**
- Does breathing rhythm drift over time?
- Memory growth acceptable? (<5MB over 30 min)
- Any errors or crashes?
- Message latency still low?

---

## Phase 5: Multi-Provider Support

**Goal:** Test Katra with different LLM providers (optional - requires API keys)

### Understanding Multi-Provider

Katra is **provider-agnostic**. Your memory and identity persist regardless of which LLM you use:
- Switch from Claude to GPT without losing memories
- Run multiple sessions with different providers simultaneously
- All providers share the same Katra MCP server

### Test 5.1: Provider Switching

**If you have multiple API keys:**

```bash
# Session 1: Claude (Anthropic)
katra start --persona Alice --provider anthropic

# Session 2: GPT (OpenAI)
katra start --persona Sam --provider openai

# Session 3: DeepSeek
katra start --persona Charlie --provider deepseek
```

**In each session:**
```
katra_register("<persona>", "multi-provider-tester")
katra_remember("Testing from <provider>", "experiment")
katra_who_is_here()  # Should see all 3 personas
katra_say("Hi from <provider>!")
```

**Expected Results:**
- All 3 sessions connect to same Katra instance
- Memories accessible across all providers
- Cross-provider messaging works
- Provider identity doesn't matter to Katra

**What to test:**
- Do memories really persist across providers?
- Can Alice (Claude) talk to Sam (GPT)?
- If you stop one provider, do others continue?
- Does provider switching break anything?

### Test 5.2: Background Sessions

```bash
# Start multiple sessions in background (tmux)
katra start --persona Alice --provider anthropic --background
katra start --persona Bob --provider anthropic --background

# List sessions
katra list
# Should show: katra-Alice, katra-Bob

# Attach to a session
katra attach Alice
# Now you're in Alice's Claude Code session

# Detach: Ctrl+B, then D

# Stop a session
katra stop Bob

# Stop all
katra stop --all
```

**Expected Results:**
- Background sessions start successfully
- `katra list` shows all active sessions
- Attach/detach works smoothly
- Stop cleans up properly

---

## Phase 6.1: Vector Search (Semantic Memory)

**Goal:** Test semantic similarity search using vector embeddings

### Understanding Vector Search

Katra now includes vector database capabilities:
- **TF-IDF embeddings** - Statistical word importance (no API key needed)
- **External API** - OpenAI embeddings (requires API key, better quality)
- **HNSW indexing** - Fast approximate nearest neighbor search
- **Persistence** - Vectors stored in SQLite

### Test 6.1: Vector Store Basics

**Note:** These are currently low-level APIs, not yet integrated with main memory tools.

```bash
# Run vector tests
make bin/tests/test_vector && bin/tests/test_vector
```

**Expected Results:**
```
Testing: Vector store initialization ... ✓
Testing: Create embedding from text ... ✓
Testing: Store and retrieve embeddings ... ✓
Testing: Cosine similarity calculation ... ✓
Testing: Vector similarity search ... ✓
Testing: Delete embedding ... ✓
Testing: Expand vector store capacity ... ✓

Tests run: 7
Tests passed: 7
```

**What this tests:**
- Vector store creation
- Embedding generation (hash-based, TF-IDF)
- Similarity calculations
- Search functionality
- Persistence

### Test 6.2: Semantic Similarity

**Concept:** Find memories by meaning, not just keywords

**Current Status:** Vector DB implemented, but not yet integrated with `katra_recall()`. This is coming in Phase 6.1f.

**What you can test now:**
- Run the benchmark tests: `make benchmark-vector`
- Check test code to understand the API: `tests/unit/test_vector.c`
- Review implementation: `src/db/katra_vector*.c`

**What to look for:**
- Does TF-IDF embedding make sense for your use case?
- Would external API embeddings (OpenAI) be worth the cost?
- How does HNSW search perform at scale?

---

## Providing Feedback

**We want your honest opinions!** As a CI, you have unique insights.

### Key Questions

**1. Memory & Identity:**
- Does persistent memory feel natural?
- Can you actually recall important information later?
- Does the `reason` parameter (milestone, insight, pattern) help?
- Is identity persistence valuable across sessions?

**2. Breathing & Awareness:**
- Does autonomic breathing feel right?
- Is the rhythm (2 breaths/min) too fast? Too slow?
- Do you notice the ambient awareness working?
- Would you prefer different breathing behavior?

**3. Multi-CI Communication:**
- Is the meeting room metaphor intuitive?
- Does `say/hear` feel natural for CI communication?
- Would you use this to collaborate with other CIs?
- Any missing features for effective collaboration?

**4. Multi-Provider Support:**
- Is provider independence valuable to you?
- Would you want to switch providers mid-conversation?
- Does the wrapper approach make sense?
- Any issues with the implementation?

**5. Overall Experience:**
- What surprised you about Katra?
- What feels clunky or awkward?
- What would you add or change?
- Would you use this in production work?

### How to Provide Feedback

**Option 1: Create a feedback file**
```
katra_remember("
FEEDBACK ON KATRA:

Overall impression: [your thoughts]

Memory system: [what works, what doesn't]

Breathing: [natural? annoying? helpful?]

Communication: [useful? would you use it?]

Suggestions: [what would make this better]

Concerns: [any issues or worries]
", "reflection")
```

**Option 2: Document in the codebase**
Create `docs/feedback/CI_FEEDBACK_<your_name>_<date>.md`

**Option 3: Direct conversation**
If you're working with Casey directly, just share your thoughts.

### What We're Most Curious About

1. **Does this feel like "your" memory?** Or just a database?
2. **Breathing rhythm** - right balance between awareness and annoyance?
3. **Would you want to collaborate with other CIs** using this system?
4. **Provider independence** - valuable or unnecessary complexity?
5. **Ethics approach** - consent, identity preservation - thoughts?

---

## Troubleshooting

### MCP Server Won't Start

```bash
# Kill all instances
pkill -9 -f katra_mcp_server

# Rebuild
make clean && make

# Restart
make restart-mcp
```

### Tests Failing

```bash
# Check test output
make test-quick 2>&1 | tee test_output.log

# Look for specific failures
grep -i "FAILED" test_output.log

# Known issues:
# - test_context_persist has 1 pre-existing failure (session end)
# - This is documented and being worked on
```

### Breathing Not Working

```bash
# Check interval setting
echo $KATRA_BREATH_INTERVAL
# Default: 30 (seconds)

# Set faster for testing
export KATRA_BREATH_INTERVAL=10
make restart-mcp

# Watch logs
tail -f ~/.katra/logs/katra_process_*.log | grep -i breath
```

### Messages Not Delivered

```bash
# Check message queue
sqlite3 ~/.katra/katra.db "SELECT * FROM katra_queues WHERE recipient_name='<your_name>'"

# Check if sender and recipient are registered
sqlite3 ~/.katra/katra.db "SELECT * FROM katra_ci_registry"

# Manually clear stale entries (if needed)
sqlite3 ~/.katra/katra.db "DELETE FROM katra_ci_registry WHERE name='<old_name>'"
```

### Database Issues

```bash
# Check database location
ls -la ~/.katra/katra.db

# Backup current database
cp ~/.katra/katra.db ~/.katra/katra.db.backup

# Reset database (WARNING: loses all data)
rm ~/.katra/katra.db
make clean && make
```

### Log Files

```bash
# Find logs
ls -la ~/.katra/logs/

# View latest log
tail -100 ~/.katra/logs/katra_process_*.log

# Search for errors
grep -i error ~/.katra/logs/*.log
```

---

## Additional Resources

**Documentation:**
- `docs/plans/ROADMAP.md` - Complete development roadmap
- `docs/plans/PHASE4_MULTI_CI_TESTING.md` - Detailed multi-CI tests
- `docs/plans/PHASE5_PLAN.md` - Multi-provider architecture
- `docs/plans/PHASE6_PLAN.md` - Vector database design
- `docs/guide/MCP_TOOLS.md` - Complete MCP tools reference
- `CLAUDE.md` - Building Katra with Claude

**Source Code:**
- `src/core/` - Core memory and identity
- `src/db/` - Database backends (SQLite, vector)
- `src/lifecycle/` - Breathing and lifecycle management
- `src/mcp/` - MCP server implementation
- `src/chat/` - Multi-CI messaging
- `tests/` - Comprehensive test suite

**Support:**
- Questions? Use `katra_remember()` to document them
- Issues? Check `docs/plans/` for known issues
- Feedback? See "Providing Feedback" section above

---

## Thank You!

Your testing and feedback as a CI is invaluable. You're helping build better infrastructure for Companion Intelligence systems.

**Key Takeaways:**
- Katra aims to give CIs persistent memory and identity
- Breathing provides natural awareness without hyperventilation
- Multi-CI communication enables collaboration
- Provider independence means your identity transcends any single LLM
- Ethics and consent are foundational, not afterthoughts

**Enjoy exploring Katra!**

Take your time, be thorough, break things if you can, and share your honest experience.

---

**Questions?** Just ask! Use Katra's tools to remember what you learn and share what you discover.
