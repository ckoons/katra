<!-- © 2025 Casey Koons All rights reserved -->

# Phase 4: Multi-CI Testing Guide

**Created:** 2025-01-10
**Status:** Ready for testing
**Phase:** 4 of 6 (Multi-CI Testing and Refinement)

---

## Overview

This guide explains how to test Phase 3's autonomic breathing implementation with multiple CIs (Alice, Bob, Charlie, etc.) to verify:

1. **Breathing rhythm** - Natural 2 breaths/minute across multiple CIs
2. **Message awareness** - Ambient detection of incoming messages
3. **Concurrent registration** - Multiple CIs joining simultaneously
4. **Extended conversations** - Natural flow over 30+ minutes
5. **Performance** - No degradation with multiple active CIs

---

## Prerequisites

**Built and running MCP server with Phase 3 hook adapter layer:**
```bash
cd /Users/cskoons/projects/github/katra
make clean && make
make restart-mcp
```

**Verify Phase 3 is active:**
- Hook registry initialized
- Anthropic adapter registered
- Turn boundaries implemented
- Breathing integrated

---

## Test 1: Single CI Baseline

**Goal:** Verify breathing works correctly for one CI

**Setup:**
1. Connect to Katra MCP from Claude Code
2. Register with a test name

**Procedure:**
```
1. katra_register(name="Claude-Test", role="developer")
2. katra_whoami()  # Verify registration
3. Wait 5 minutes while doing normal work
4. Check logs for breathing rhythm
```

**Expected Results:**
- Registration succeeds
- Breathing occurs every ~30 seconds (visible in logs)
- No errors or crashes
- Normal MCP tool operations work

**Success Criteria:**
- ✅ CI registered successfully
- ✅ Breathing rhythm maintained
- ✅ All tools functional

---

## Test 2: Two-CI Conversation

**Goal:** Test message awareness and breathing with 2 CIs

**Setup:**
1. Start Alice's MCP server (separate terminal/process)
2. Start Claude's MCP server
3. Both register in meeting room

**Procedure:**

**From Alice:**
```
1. katra_register(name="Alice", role="researcher")
2. katra_who_is_here()  # Should see: Alice, Claude
3. katra_say("Hello Claude! Testing Phase 4 breathing")
4. Wait 30 seconds
5. katra_who_is_here()  # Verify both still registered
```

**From Claude:**
```
1. katra_register(name="Claude", role="developer")
2. katra_who_is_here()  # Should see: Alice, Claude
3. Wait 30 seconds (breathing should detect message)
4. katra_hear()  # Should get Alice's message
5. katra_say("Hi Alice! Breathing detected your message")
```

**Expected Results:**
- Both CIs register successfully
- Alice's message queued for Claude
- Claude's breathing detects unread message (~30s later)
- Message delivered when Claude calls katra_hear()
- Breathing continues naturally for both

**Success Criteria:**
- ✅ Both CIs coexist without conflicts
- ✅ Messages delivered reliably
- ✅ Breathing rhythm maintained for both
- ✅ Message awareness works (ambient detection)

---

## Test 3: Three-CI Concurrent Registration

**Goal:** Test concurrent registration and group messaging

**Setup:**
1. Start 3 MCP servers simultaneously (Alice, Bob, Charlie)
2. All register within 10 seconds of each other

**Procedure:**

**Terminal 1 (Alice):**
```
katra_register(name="Alice", role="researcher")
```

**Terminal 2 (Bob):**
```
katra_register(name="Bob", role="engineer")
```

**Terminal 3 (Charlie):**
```
katra_register(name="Charlie", role="tester")
```

**From any CI:**
```
katra_who_is_here()  # Should see all 3
katra_say("Testing 3-CI concurrent registration")
```

**From each CI:**
```
katra_hear()  # Each should get broadcast message
```

**Expected Results:**
- All 3 register successfully
- who_is_here shows all 3 CIs
- Broadcast message delivered to all
- No database conflicts or race conditions
- Breathing continues for all 3

**Success Criteria:**
- ✅ Concurrent registration succeeds
- ✅ All CIs visible to each other
- ✅ Broadcast messaging works
- ✅ No race conditions or errors
- ✅ Breathing rhythm maintained for all

---

## Test 4: Extended Conversation (30+ minutes)

**Goal:** Verify system stability during extended use

**Setup:**
1. Start Alice and Claude
2. Have natural conversation with 50+ message exchanges

**Procedure:**
1. Alice and Claude exchange messages naturally
2. Monitor breathing frequency every 5 minutes
3. Check memory usage periodically
4. Verify message delivery latency stays low

**Monitoring Commands:**
```bash
# Watch breathing in real-time
tail -f ~/.katra/logs/katra_process_*.log | grep -i "breath"

# Check memory usage
ps aux | grep katra_mcp_server

# Count messages delivered
sqlite3 ~/.katra/katra.db "SELECT COUNT(*) FROM katra_queues"
```

**Expected Results:**
- Conversation flows naturally
- Breathing stays consistent (no drift)
- Memory usage stable (no leaks)
- Message latency < 1 second
- No performance degradation

**Success Criteria:**
- ✅ 50+ messages exchanged successfully
- ✅ Breathing rhythm consistent throughout
- ✅ Memory growth < 5MB over 30 minutes
- ✅ No errors or crashes
- ✅ Performance remains good

---

## Test 5: Message Awareness Timing

**Goal:** Measure how quickly breathing detects new messages

**Setup:**
1. Alice and Claude registered
2. Set fast breathing interval for testing: `export KATRA_BREATH_INTERVAL=10`

**Procedure:**
1. Alice sends message to Claude
2. Note timestamp of send
3. Watch Claude's logs for "unread_messages" detection
4. Calculate detection latency

**Expected Results:**
- Message detected within 1 breathing cycle (10 seconds)
- Log shows: "Awareness: 1 unread messages"
- Detection is passive (no explicit check by CI)

**Success Criteria:**
- ✅ Messages detected within 1-2 breath cycles
- ✅ Ambient awareness works (no manual checking)
- ✅ Log messages appear correctly

---

## Test 6: Cleanup and Persona Persistence

**Goal:** Verify CIs cleanup on exit and can resume

**Procedure:**

**Part A: Cleanup**
1. Alice registers as "Alice"
2. Gracefully stop Alice's MCP server (Ctrl+C or SIGTERM)
3. Check who_is_here from Claude
4. Verify Alice is removed from registry

**Part B: Resume**
1. Start Alice's MCP server again
2. Should auto-resume as "Alice"
3. Check who_is_here - Alice should reappear

**Expected Results:**
- Clean shutdown removes CI from registry
- No stale entries remain
- Resume uses last persona name
- No conflicts on resume

**Success Criteria:**
- ✅ Clean shutdown removes CI
- ✅ No stale entries in registry
- ✅ Resume works with same name
- ✅ who_is_here accurate after resume

---

## Observation Checklist

During all tests, monitor for:

**Breathing Behavior:**
- [ ] Consistent rhythm (~30 seconds or configured interval)
- [ ] No "hyperventilation" (excessive checks)
- [ ] First breath at session start (not rate-limited)
- [ ] Cached returns when called too frequently
- [ ] Turn boundaries trigger breathing

**Message Awareness:**
- [ ] Unread messages detected passively
- [ ] Count accurate (matches actual queue)
- [ ] Detection timing reasonable (within 1-2 cycles)
- [ ] No false positives/negatives

**Performance:**
- [ ] Message delivery < 1 second
- [ ] Tool calls responsive
- [ ] No database locks or timeouts
- [ ] Memory usage stable
- [ ] CPU usage reasonable

**Errors:**
- [ ] No crashes or segfaults
- [ ] Graceful error handling
- [ ] Clear error messages
- [ ] Recovery from failures

---

## Known Issues

**Linking Issues with Standalone Tests:**
- Circular dependencies prevent building standalone test executables
- Workaround: Test through MCP server tools instead
- Not a runtime issue - only affects test compilation

**Log Files:**
- Process-specific logs: `~/.katra/logs/katra_process_<PID>.log`
- May be empty if process exits quickly
- Use `tail -f` to watch live

**Stale Personas:**
- Unclean shutdown may leave registry entries
- Manual cleanup: `DELETE FROM katra_ci_registry WHERE name='<name>'`
- Phase 3 addresses this with proper cleanup

---

## Success Metrics

**Phase 4 passes if:**
1. ✅ Breathing maintains 2/min rhythm (± 10%)
2. ✅ Message awareness accurate and timely
3. ✅ Multiple CIs coexist without conflicts
4. ✅ Extended conversations work smoothly
5. ✅ Performance acceptable (< 1s latency)
6. ✅ Memory stable (no leaks)
7. ✅ Graceful error handling
8. ✅ Clean shutdown and resume work

---

## Troubleshooting

**Problem: Breathing too frequent**
```bash
# Check current interval
grep "Breathing interval" ~/.katra/logs/katra_process_*.log

# Set custom interval
export KATRA_BREATH_INTERVAL=30
make restart-mcp
```

**Problem: Messages not detected**
```bash
# Verify message in queue
sqlite3 ~/.katra/katra.db "SELECT * FROM katra_queues WHERE recipient_name='<name>'"

# Force immediate breath
# (Not available in CLI - wait for next natural breath)
```

**Problem: Multiple CIs conflict**
```bash
# Check registry
sqlite3 ~/.katra/katra.db "SELECT * FROM katra_ci_registry"

# Clear stale entries
sqlite3 ~/.katra/katra.db "DELETE FROM katra_ci_registry WHERE name='<name>'"
```

**Problem: MCP server won't start**
```bash
# Kill all instances
pkill -9 -f katra_mcp_server

# Rebuild
make clean && make

# Restart
make restart-mcp
```

---

## Next Steps

After completing Phase 4 multi-CI testing:

1. Document results in `PHASE4_TEST_RESULTS.md`
2. Create example conversation transcript
3. Measure breathing frequency statistics
4. Identify any tuning needs
5. Fix any bugs discovered
6. Plan Phase 5 (Router Integration - optional)

---

**Ready to test!** Start with Test 1 (Single CI Baseline) and work through the test suite.
