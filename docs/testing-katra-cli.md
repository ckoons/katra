# Testing Katra CLI

© 2025 Casey Koons All rights reserved

## Overview

This document describes how to test the `katra-cli` human interface tool. Tests verify all commands work correctly with the Katra MCP server.

## Test Strategy

### Test Levels

1. **Unit Tests** - Individual command validation
2. **Integration Tests** - CLI ↔ MCP server communication
3. **Functional Tests** - End-to-end user workflows
4. **System Tests** - Multi-user scenarios

### Test Environment

**Prerequisites:**
- Katra MCP server compiled (`make`)
- TCP-mode server running on port 3141
- `jq` and `nc` installed
- Clean state directory (`~/.katra/`)

## Running Tests

### Quick Test (Make Targets)

```bash
cd /Users/cskoons/projects/github/katra

# Run CLI tests (uses existing server if available)
make test-cli

# Run CLI tests with clean server restart
make test-cli-clean

# Run all tests (core + CLI)
make test
```

The test suite will:
- Use existing TCP MCP server on port 3141 if running
- Start a new server if none exists
- Only clean up servers it started
- Leave existing servers running

### Manual Test Suite

#### Setup

```bash
# 1. Start TCP MCP server
cd /Users/cskoons/projects/github/katra
./bin/katra_mcp_server --tcp --port 3141 > /tmp/katra_tcp_test.log 2>&1 &
export KATRA_TCP_PID=$!

# 2. Clean state
rm -f ~/.katra/.human_*

# 3. Set test environment
export KATRA_MCP_PORT=3141
export KATRA_MCP_HOST=localhost
```

#### Test Cases

##### Test 1: Help Command

```bash
# Expected: Display help text
./bin/katra-cli help | grep "katra-cli - Human CLI"
echo "Test 1: $?"  # Should be 0
```

##### Test 2: Register Identity

```bash
# Expected: Register successfully
./bin/katra-cli register TestUser human | grep "Registered as TestUser"
echo "Test 2: $?"  # Should be 0

# Verify state file created
test -f ~/.katra/.human_identity && echo "Test 2a: PASS" || echo "Test 2a: FAIL"
```

##### Test 3: Whoami Query

```bash
# Expected: Show TestUser identity
./bin/katra-cli whoami | grep "Name: TestUser"
echo "Test 3: $?"  # Should be 0
```

##### Test 4: Who Command

```bash
# Expected: List TestUser in meeting room
./bin/katra-cli who | grep "TestUser"
echo "Test 4: $?"  # Should be 0
```

##### Test 5: Say Command

```bash
# Expected: Broadcast message successfully
./bin/katra-cli say "Test message from automated suite" | grep "Message broadcast"
echo "Test 5: $?"  # Should be 0
```

##### Test 6: Hear Command (No Messages)

```bash
# Expected: No new messages (only TestUser in room)
./bin/katra-cli hear | grep "No new messages"
echo "Test 6: $?"  # Should be 0
```

##### Test 7: Status Command

```bash
# Expected: Show system status
./bin/katra-cli status | grep "SESSION:"
echo "Test 7: $?"  # Should be 0
```

##### Test 8: Recent Memories

```bash
# Expected: Show recent memories
./bin/katra-cli recent | grep "recent memories"
echo "Test 8: $?"  # Should be 0
```

##### Test 9: Recent with Limit

```bash
# Expected: Respect limit parameter
./bin/katra-cli recent --limit 10 | grep "recent memories"
echo "Test 9: $?"  # Should be 0
```

##### Test 10: Recall Search

```bash
# Expected: Search memories by topic
./bin/katra-cli recall "session" | grep "memories"
echo "Test 10: $?"  # Should be 0
```

##### Test 11: Team Status

```bash
# Expected: Show team overview
./bin/katra-cli team-status | grep "Katra Team Status"
echo "Test 11: $?"  # Should be 0
```

##### Test 12: Sessions List

```bash
# Expected: Show active sessions including our TCP server
./bin/katra-cli sessions | grep "TCP mode"
echo "Test 12: $?"  # Should be 0
```

##### Test 13: Multi-word Message (No Quotes)

```bash
# Expected: Handle multi-word messages without quotes
./bin/katra-cli say Testing multi word message | grep "Message broadcast"
echo "Test 13: $?"  # Should be 0
```

#### Cleanup

```bash
# Stop test server
kill $KATRA_TCP_PID 2>/dev/null || pkill -f "katra_mcp_server --tcp --port 3141"

# Clean state
rm -f ~/.katra/.human_*
```

## Automated Test Script

Location: `/Users/cskoons/projects/github/katra/tests/test_katra_cli.sh`

### Running Automated Tests

```bash
# From katra root
./tests/test_katra_cli.sh

# Or via make
make test-cli
```

### Test Output

```
========================================
Katra CLI Test Suite
========================================

Setup...
  ✓ Starting TCP MCP server (PID: 12345)
  ✓ Cleaning state directory

Running tests...
  ✓ Test 1: Help command
  ✓ Test 2: Register identity
  ✓ Test 3: Whoami query
  ✓ Test 4: Who command
  ✓ Test 5: Say command
  ✓ Test 6: Hear command
  ✓ Test 7: Status command
  ✓ Test 8: Recent memories
  ✓ Test 9: Recent with limit
  ✓ Test 10: Recall search
  ✓ Test 11: Team status
  ✓ Test 12: Sessions list
  ✓ Test 13: Multi-word message

Cleanup...
  ✓ Stopped MCP server
  ✓ Cleaned state

========================================
Results: 13/13 tests passed (100%)
========================================
```

## Integration with Make

Add to `/Users/cskoons/projects/github/katra/Makefile`:

```makefile
# Test targets
test: test-core test-cli

test-cli:
	@echo "Running Katra CLI tests..."
	@./tests/test_katra_cli.sh

.PHONY: test test-cli
```

## Advanced Testing

### Multi-User Testing

Test concurrent users:

```bash
# Terminal 1
./bin/katra-cli register Alice human
./bin/katra-cli say "Hello from Alice"

# Terminal 2
./bin/katra-cli register Bob developer
./bin/katra-cli say "Hello from Bob"

# Terminal 1
./bin/katra-cli hear  # Should see Bob's message
```

### Unread Tracking Test

Verify message tracking works:

```bash
# Setup: Register two users
./bin/katra-cli register User1 human &
./bin/katra-cli register User2 human &

# User2 sends message
echo '{"jsonrpc":"2.0","method":"tools/call","params":{"name":"katra_say","arguments":{"message":"Test"}},"id":1}' | nc localhost 3141

# User1 hears it
./bin/katra-cli hear  # Should show message

# User1 hears again
./bin/katra-cli hear  # Should show "No new messages"
```

### Performance Testing

Test response times:

```bash
# Measure command latency
time ./bin/katra-cli whoami
time ./bin/katra-cli status
time ./bin/katra-cli recent --limit 100
```

Expected:
- Simple commands: < 0.5s
- Complex queries: < 2s

### Error Handling Tests

#### No Server Running

```bash
# Stop server
pkill katra_mcp_server

# Expected: Clear error message
./bin/katra-cli whoami
# Output: "Error: No response from MCP server"
```

#### Invalid Arguments

```bash
# Expected: Usage error
./bin/katra-cli register  # Missing name
./bin/katra-cli say       # Missing message
./bin/katra-cli recall    # Missing topic
```

#### Network Issues

```bash
# Wrong port
export KATRA_MCP_PORT=9999
./bin/katra-cli whoami
# Expected: Connection error

# Invalid host
export KATRA_MCP_HOST=nonexistent.local
./bin/katra-cli whoami
# Expected: Connection error
```

## Continuous Integration

### GitHub Actions Workflow

```yaml
name: Test Katra CLI

on: [push, pull_request]

jobs:
  test-cli:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y jq netcat

      - name: Build Katra
        run: make

      - name: Run CLI tests
        run: make test-cli
```

## Test Coverage

### Current Coverage

- ✅ All 13 core commands
- ✅ Argument parsing
- ✅ Error handling
- ✅ State management
- ✅ Multi-user scenarios
- ✅ TCP communication
- ✅ JSON-RPC protocol

### Not Covered

- ⚠️  Network failures mid-request
- ⚠️  Malformed JSON responses
- ⚠️  Very large message payloads
- ⚠️  Concurrent access to state files

## Debugging Failed Tests

### Enable Verbose Output

```bash
# Show all communication
export KATRA_CLI_DEBUG=1
./bin/katra-cli whoami
```

### Check Server Logs

```bash
# View MCP server output
tail -f /tmp/katra_tcp_test.log
```

### Inspect State Files

```bash
# View state
ls -la ~/.katra/
cat ~/.katra/.human_identity
cat ~/.katra/.human_last_read
```

### Manual Protocol Test

```bash
# Send raw JSON-RPC request
echo '{"jsonrpc":"2.0","method":"tools/call","params":{"name":"katra_whoami","arguments":{}},"id":1}' | nc localhost 3141
```

## Best Practices

1. **Always clean state** before test runs
2. **Start fresh server** for each test suite
3. **Check exit codes** for all commands
4. **Verify output format** not just success
5. **Test error paths** as well as happy paths
6. **Use timeouts** to prevent hanging tests
7. **Clean up processes** in trap handlers

## See Also

- [Katra CLI Documentation](./katra-cli.md)
- [MCP Architecture](./mcp-architecture.md)
- [Katra Testing Guide](./TESTING.md)
