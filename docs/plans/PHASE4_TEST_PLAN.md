<!-- © 2025 Casey Koons All rights reserved -->

# Phase 4: Multi-CI Testing and Refinement - Test Plan

**Created:** 2025-01-10
**Status:** In Progress
**Phase:** 4 of 6 (Multi-CI Testing and Refinement)

---

## Overview

Phase 4 focuses on verifying that the autonomic breathing system works correctly in real-world multi-CI scenarios. The goal is to ensure natural rhythm, ambient awareness, and graceful performance under extended use.

**Key Questions:**
1. Does breathing maintain natural 2-breaths-per-minute rhythm?
2. Does message awareness work without spam or delays?
3. Can multiple CIs register and communicate simultaneously?
4. Does the system perform well during extended conversations?
5. Are there any edge cases or race conditions?

---

## Test Categories

### 1. Breathing Rhythm Tests

**Purpose:** Verify rate-limiting works correctly and breathing feels natural

**Test 1.1: Default Breathing Interval**
- **Setup:** Start MCP server with default configuration
- **Action:** Monitor breathing frequency over 5 minutes
- **Expected:** ~2 breaths per minute (every 30 seconds)
- **Metrics:** Actual breath timestamps, intervals between breaths
- **Pass Criteria:** 95% of breaths within 28-32 second interval

**Test 1.2: Configured Breathing Interval**
- **Setup:** Set `KATRA_BREATH_INTERVAL=10`
- **Action:** Monitor breathing frequency over 2 minutes
- **Expected:** ~6 breaths per minute (every 10 seconds)
- **Metrics:** Actual breath timestamps, intervals between breaths
- **Pass Criteria:** 95% of breaths within 9-11 second interval

**Test 1.3: Cached Context Returns**
- **Setup:** Call katra_breath() multiple times rapidly
- **Action:** Call 5 times in quick succession (< 1 second apart)
- **Expected:** Only first call performs actual check, rest return cached
- **Metrics:** Log messages showing "cached" vs "actual check"
- **Pass Criteria:** 4 out of 5 calls use cached context

**Test 1.4: First Breath (Session Start)**
- **Setup:** Start new MCP server session
- **Action:** Monitor first breath at session start
- **Expected:** First breath performs actual check (not rate-limited)
- **Metrics:** Log message showing "actual check" on first breath
- **Pass Criteria:** First breath always performs actual check

---

### 2. Message Awareness Tests

**Purpose:** Verify ambient awareness of unread messages

**Test 2.1: Zero Messages**
- **Setup:** Start with empty message queue
- **Action:** Monitor breathing context
- **Expected:** unread_messages = 0
- **Metrics:** breath_context_t.unread_messages value
- **Pass Criteria:** Correctly reports 0 messages

**Test 2.2: Incoming Message Detection**
- **Setup:** Two CIs registered (Alice, Claude)
- **Action:**
  1. Alice sends message to Claude
  2. Wait for Claude's next breath
  3. Check breath_context_t
- **Expected:** unread_messages = 1
- **Metrics:** breath_context_t.unread_messages value
- **Pass Criteria:** Correctly reports 1 message

**Test 2.3: Message Consumption**
- **Setup:** Claude has 1 unread message
- **Action:**
  1. Call katra_hear() to consume message
  2. Wait for next breath
  3. Check breath_context_t
- **Expected:** unread_messages = 0 (after consumption)
- **Metrics:** breath_context_t.unread_messages value
- **Pass Criteria:** Correctly reports 0 after consumption

**Test 2.4: Multiple Messages**
- **Setup:** Three CIs registered (Alice, Bob, Charlie)
- **Action:**
  1. Alice sends message to Claude
  2. Bob sends message to Claude
  3. Charlie sends broadcast
  4. Wait for Claude's next breath
- **Expected:** unread_messages = 3
- **Metrics:** breath_context_t.unread_messages value
- **Pass Criteria:** Correctly reports all pending messages

---

### 3. Concurrent Registration Tests

**Purpose:** Verify multiple CIs can register and operate simultaneously

**Test 3.1: Sequential Registration**
- **Setup:** Start 3 MCP servers sequentially
- **Action:**
  1. Start Alice
  2. Start Bob (1 second later)
  3. Start Charlie (1 second later)
  4. Call who_is_here from each
- **Expected:** All 3 registered, all see each other
- **Metrics:** who_is_here results from each CI
- **Pass Criteria:** All 3 CIs appear in all who_is_here results

**Test 3.2: Simultaneous Registration**
- **Setup:** Start 3 MCP servers simultaneously (within 100ms)
- **Action:**
  1. Launch all 3 in parallel
  2. Wait 2 seconds
  3. Call who_is_here from each
- **Expected:** All 3 registered, no race conditions
- **Metrics:** who_is_here results, no database errors
- **Pass Criteria:** All 3 CIs registered successfully, no errors

**Test 3.3: Registration with Existing Names**
- **Setup:** Alice already registered
- **Action:** Start new MCP server with name "Alice"
- **Expected:** Old Alice replaced, new Alice active
- **Metrics:** who_is_here shows only 1 Alice
- **Pass Criteria:** Single Alice entry, new ci_id

**Test 3.4: High Concurrency (5+ CIs)**
- **Setup:** Start 5 MCP servers simultaneously
- **Action:** All register within 1 second
- **Expected:** All 5 registered successfully
- **Metrics:** Database integrity, all entries present
- **Pass Criteria:** All 5 CIs in registry, no corruption

---

### 4. Extended Conversation Tests

**Purpose:** Verify system performs well during long interactions

**Test 4.1: Long Conversation (30 minutes)**
- **Setup:** Two CIs (Alice, Claude) in conversation
- **Action:** Exchange 50+ messages over 30 minutes
- **Expected:**
  - Consistent breathing rhythm throughout
  - Message awareness stays accurate
  - No performance degradation
- **Metrics:**
  - Breathing intervals over time
  - Message delivery latency
  - Memory usage
- **Pass Criteria:**
  - Breathing stays within 28-32s
  - All messages delivered < 1s
  - Memory stable (no leaks)

**Test 4.2: Turn Boundary Breathing**
- **Setup:** Active CI interaction session
- **Action:** Perform 20 tool calls with turn start/end
- **Expected:**
  - katra_turn_start() called before each turn
  - katra_turn_end() called after each turn
  - Breathing rate-limited (not every turn)
- **Metrics:** Log messages showing turn boundaries
- **Pass Criteria:** Turn tracking works, breathing rate-limited

**Test 4.3: Memory Growth Over Time**
- **Setup:** CI interacting for 1 hour
- **Action:** Monitor memory usage every 5 minutes
- **Expected:** Stable memory (no unbounded growth)
- **Metrics:** RSS memory usage
- **Pass Criteria:** Memory growth < 5MB over 1 hour

---

### 5. Hook Adapter Tests

**Purpose:** Verify hook adapter layer works correctly

**Test 5.1: Anthropic Adapter Loading**
- **Setup:** Start MCP server
- **Action:** Check logs for hook registration
- **Expected:** "Hook adapter registered: anthropic" in logs
- **Metrics:** Log messages
- **Pass Criteria:** Adapter loads successfully

**Test 5.2: Hook Invocation**
- **Setup:** MCP server with hook adapter
- **Action:** Trigger lifecycle events (session start, turn start, etc.)
- **Expected:** Adapter hooks called, Katra lifecycle functions invoked
- **Metrics:** Log messages showing hook invocations
- **Pass Criteria:** All hooks route correctly to lifecycle functions

**Test 5.3: Adapter Fallback**
- **Setup:** No adapter registered
- **Action:** Call hook invocation functions
- **Expected:** Direct lifecycle function calls (fallback behavior)
- **Metrics:** Log messages showing fallback
- **Pass Criteria:** System works without adapter

---

### 6. Error Handling and Edge Cases

**Purpose:** Verify graceful handling of error conditions

**Test 6.1: Database Unavailable**
- **Setup:** Lock or remove database file
- **Action:** Try to perform breathing check
- **Expected:** Error logged, cached context returned
- **Metrics:** Error logs, no crash
- **Pass Criteria:** Graceful degradation, no crash

**Test 6.2: Breathing Disabled**
- **Setup:** Set breathing_enabled = false
- **Action:** Call katra_breath()
- **Expected:** Cached context returned, no actual check
- **Metrics:** Log messages
- **Pass Criteria:** Returns cached, no database access

**Test 6.3: Invalid Breath Interval**
- **Setup:** Set KATRA_BREATH_INTERVAL=0
- **Action:** Try to set breathing interval
- **Expected:** E_INVALID_PARAMS returned, default used
- **Metrics:** Error code returned
- **Pass Criteria:** Invalid value rejected

**Test 6.4: Session End Without Start**
- **Setup:** Call katra_session_end() without session_start
- **Action:** Monitor behavior
- **Expected:** E_INVALID_STATE returned
- **Metrics:** Error code returned
- **Pass Criteria:** Error returned gracefully

---

## Test Execution

### Manual Testing Procedures

**Setup Environment:**
```bash
# Build latest code
cd /Users/cskoons/projects/github/katra
make clean && make

# Set test breathing interval (optional)
export KATRA_BREATH_INTERVAL=10  # 10 seconds for faster testing

# Restart MCP server
make restart-mcp
```

**Run Single CI Tests:**
```bash
# Use Claude Code MCP tools
katra_register(name="TestCI", role="tester")
katra_who_is_here()  # Verify registration
katra_say("Test message")  # Send message
katra_hear()  # Check for messages
```

**Run Multi-CI Tests:**
```bash
# Terminal 1: Start Alice
KATRA_PERSONA=Alice bin/katra_mcp_server

# Terminal 2: Start Bob
KATRA_PERSONA=Bob bin/katra_mcp_server

# Terminal 3: Start Charlie
KATRA_PERSONA=Charlie bin/katra_mcp_server

# From Claude Code: Send messages between CIs
```

**Monitor Breathing:**
```bash
# Watch logs in real-time
tail -f ~/.katra/logs/katra_process_*.log | grep -i "breath"

# Extract breathing timestamps
grep "Breath (actual check)" ~/.katra/logs/katra_process_*.log | \
  awk '{print $2}' > breath_timestamps.txt

# Calculate intervals
python3 scripts/analyze_breath_intervals.py breath_timestamps.txt
```

---

## Success Criteria Summary

**Phase 4 passes if:**
1. ✅ Breathing maintains 2/min rhythm (± 10%)
2. ✅ Message awareness is accurate and timely
3. ✅ Multiple CIs can register without conflicts
4. ✅ Extended conversations work smoothly
5. ✅ Hook adapter layer functions correctly
6. ✅ Error handling is graceful (no crashes)
7. ✅ Performance is acceptable (< 100ms tool latency)
8. ✅ Memory usage is stable (no leaks)

---

## Deliverables

1. **Test Results Document** - `docs/plans/PHASE4_TEST_RESULTS.md`
   - All test cases executed
   - Pass/fail status for each test
   - Metrics collected
   - Issues discovered

2. **Breathing Frequency Analysis** - `docs/analysis/breathing_frequency.md`
   - Actual vs expected breathing intervals
   - Statistical analysis (mean, stddev, outliers)
   - Recommendations for tuning

3. **Multi-CI Conversation Transcript** - `docs/examples/multi_ci_transcript.md`
   - Example conversation between 3+ CIs
   - Demonstrates natural flow with breathing
   - Shows message awareness in action

4. **Bug Fixes** (if needed)
   - Any issues discovered during testing
   - Fixes implemented and verified

5. **Tuning Recommendations**
   - Optimal breath_interval for different scenarios
   - Configuration suggestions
   - Performance optimization opportunities

---

## Timeline

**Week 1:**
- Day 1-2: Breathing rhythm tests
- Day 3-4: Message awareness tests
- Day 5: Concurrent registration tests

**Week 2:**
- Day 1-3: Extended conversation tests
- Day 4: Hook adapter tests
- Day 5: Error handling and edge cases

**Week 3:**
- Day 1-2: Complete all tests
- Day 3-4: Document results
- Day 5: Bug fixes and tuning

---

## Notes

- Tests should be run with both default (30s) and fast (2s) breathing intervals
- Multi-CI tests require multiple terminal sessions or test infrastructure
- Performance tests should be run on representative hardware
- All test results should be reproducible

---

**Status:** Ready to begin testing
**Next Steps:** Execute Test Category 1 (Breathing Rhythm Tests)
