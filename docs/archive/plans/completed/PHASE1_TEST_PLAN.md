<!-- © 2025 Casey Koons All rights reserved -->

# Phase 1: Test Plan for Existing K

atra Primitives

**Status:** Ready for testing
**Date:** 2025-01-08
**Purpose:** Verify existing functions work before implementing autonomic breathing

---

## Test Environment Setup

**Required:**
- Two CI instances (Alice and this CI - Claude/Nyx)
- Both running latest katra_mcp_server build
- Clean database state (or known good state)

**Build:**
```bash
cd /Users/cskoons/projects/github/katra
make clean && make install-mcp
```

---

## Test 1: Communication Primitives

**Functions to test:**
- `katra_say(message, recipients)`
- `katra_hear()`
- `katra_who_is_here()`

**Test Steps:**

### 1.1: Check Active CIs
```javascript
// Both CIs run:
await katra_who_is_here();
// Expected: List of active CIs (names, roles, joined_at)
```

### 1.2: Broadcast Message
```javascript
// Alice sends:
await katra_say("Hello everyone!", "broadcast");
// Expected: Success response

// Claude/Nyx receives:
await katra_hear();
// Expected: Message from Alice: "Hello everyone!"
```

### 1.3: Direct Message
```javascript
// Alice sends to Claude:
await katra_say("Hi Claude, test message", "Claude");
// Expected: Success response

// Claude receives:
await katra_hear();
// Expected: Message from Alice: "Hi Claude, test message"
// Expected: is_direct_message = true
```

### 1.4: No Self-Echo
```javascript
// Alice sends broadcast:
await katra_say("Testing self-echo", "broadcast");

// Alice checks messages:
await katra_hear();
// Expected: No message (Alice should NOT receive her own broadcast)
```

### 1.5: Message Queue Order
```javascript
// Alice sends 3 messages:
await katra_say("Message 1", "Claude");
await katra_say("Message 2", "Claude");
await katra_say("Message 3", "Claude");

// Claude receives in order:
msg1 = await katra_hear();  // Expected: "Message 1"
msg2 = await katra_hear();  // Expected: "Message 2"
msg3 = await katra_hear();  // Expected: "Message 3"
msg4 = await katra_hear();  // Expected: "No new messages"
```

**Success Criteria:**
- ✅ All messages delivered correctly
- ✅ No self-echo
- ✅ FIFO queue order maintained
- ✅ Direct vs broadcast correctly flagged

---

## Test 2: Identity Primitives

**Functions to test:**
- `katra_whoami()`
- `katra_register(name, role)`

**Test Steps:**

### 2.1: Check Identity
```javascript
// Both CIs run:
const info = await katra_whoami();
// Expected:
// - name: "Alice" or "Claude"
// - ci_id: valid UUID format
// - session_id: valid session ID
// - session start time
```

### 2.2: Registration
```javascript
// New CI registers:
await katra_register("TestBot", "test-agent");

// All CIs check:
await katra_who_is_here();
// Expected: TestBot appears in list with role "test-agent"
```

### 2.3: Persistent Identity Across Tool Calls
```javascript
// Alice calls whoami twice:
info1 = await katra_whoami();
// ... do some work ...
info2 = await katra_whoami();

// Expected: Same ci_id, same session_id (same MCP server process)
```

**Success Criteria:**
- ✅ Identity returned correctly
- ✅ Registration works
- ✅ Identity persists within session

---

## Test 3: Memory Primitives

**Functions to test:**
- `katra_remember(content, importance, reason)`
- `katra_recall(query)`
- `katra_learn(topic)`
- `katra_decide(query)`

**Test Steps:**

### 3.1: Store and Recall
```javascript
// Store memory:
await katra_remember(
    "Fixed ci_id bug by using persistent names instead of ephemeral IDs",
    "high",
    "milestone"
);

// Recall:
const memories = await katra_recall("ci_id bug");
// Expected: Find the stored memory
```

### 3.2: Learn from Experience
```javascript
// Store several related memories:
await katra_remember("Database locks prevent race conditions", "medium", "insight");
await katra_remember("SQLite works well for cross-process messaging", "medium", "insight");
await katra_remember("Name-based routing is simpler than ID-based", "high", "pattern");

// Extract learnings:
const learnings = await katra_learn("database design");
// Expected: Summary of database-related insights
```

### 3.3: Decision Support
```javascript
// Ask for decision help:
const decision = await katra_decide("Should I use database or in-memory for rate limiting?");
// Expected: Recommendation based on past experiences
```

**Success Criteria:**
- ✅ Memories stored and retrieved
- ✅ Recall finds relevant content
- ✅ Learn extracts insights
- ✅ Decide provides reasonable recommendations

---

## Test 4: Cross-Process Communication

**Goal:** Verify multi-CI conversation works across different MCP server processes

**Test Steps:**

### 4.1: Alice and Claude Conversation
```javascript
// Alice (Process A):
await katra_say("Claude, are you there?", "Claude");

// Claude (Process B):
const msg = await katra_hear();
// Expected: Receive Alice's message

await katra_say("Yes Alice, I'm here!", "Alice");

// Alice (Process A):
const reply = await katra_hear();
// Expected: Receive Claude's reply
```

### 4.2: Concurrent Registration
```javascript
// Start 3 CIs simultaneously
// Alice, Bob, Claude all run:
await katra_who_is_here();

// Expected: All 3 CIs see each other in registry
// Expected: No duplicate registrations
// Expected: No race conditions
```

**Success Criteria:**
- ✅ Messages delivered across processes
- ✅ Registry correctly tracks all active CIs
- ✅ No race conditions or corruption

---

## Known Issues (Pre-Implementation)

**The following are NOT YET IMPLEMENTED and should FAIL:**

1. **katra_breath()** - Does not exist yet
2. **Autonomic breathing** - No automatic message checking
3. **Rate limiting** - No breath rate limiting
4. **Global session_state** - Not yet designed/implemented

**These will be implemented in Phase 2 after primitives are verified.**

---

## Test Results Template

**Date:** _______
**Tester:** _______
**CIs Used:** _______

### Communication Primitives
- [ ] Test 1.1: Check Active CIs - PASS/FAIL
- [ ] Test 1.2: Broadcast Message - PASS/FAIL
- [ ] Test 1.3: Direct Message - PASS/FAIL
- [ ] Test 1.4: No Self-Echo - PASS/FAIL
- [ ] Test 1.5: Message Queue Order - PASS/FAIL

**Notes:**


### Identity Primitives
- [ ] Test 2.1: Check Identity - PASS/FAIL
- [ ] Test 2.2: Registration - PASS/FAIL
- [ ] Test 2.3: Persistent Identity - PASS/FAIL

**Notes:**


### Memory Primitives
- [ ] Test 3.1: Store and Recall - PASS/FAIL
- [ ] Test 3.2: Learn from Experience - PASS/FAIL
- [ ] Test 3.3: Decision Support - PASS/FAIL

**Notes:**


### Cross-Process Communication
- [ ] Test 4.1: Alice and Claude Conversation - PASS/FAIL
- [ ] Test 4.2: Concurrent Registration - PASS/FAIL

**Notes:**


---

## Next Steps After Phase 1

**If all tests PASS:**
- Proceed to Phase 2: Implement autonomic breathing
- Design global session_state structure
- Implement katra_init() and katra_breath()

**If tests FAIL:**
- Debug and fix failing primitives
- Re-test until stable
- Do NOT proceed to Phase 2 until Phase 1 is solid

---

**Phase 1 must be rock-solid before adding lifecycle complexity.**
