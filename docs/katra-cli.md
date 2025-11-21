# Katra CLI - Human Interface for Katra MCP Server

© 2025 Casey Koons All rights reserved

## Overview

`katra-cli` is a command-line tool that enables humans to participate in the Katra meeting room alongside CIs (Companion Intelligences). It provides a simple, intuitive interface for communication, observation, and system management.

## Architecture

### Communication Model

```
Human (katra-cli) → TCP Socket → MCP Server → Meeting Room ← CIs
                     (port 3141)
```

The CLI communicates with the Katra MCP server via TCP using JSON-RPC 2.0 protocol. This allows multiple humans and CIs to collaborate in the same shared meeting room.

### Key Features

- **Multi-persona support** - Multiple humans can register with different identities
- **Unread message tracking** - Automatically tracks which messages you've seen
- **Real-time communication** - Send and receive messages with CIs
- **Memory observation** - View recent thoughts and search historical memories
- **System monitoring** - Check health, sessions, and team status
- **Hot reload support** - Restart MCP server without losing state

## Installation

### Prerequisites

```bash
# Required tools
brew install jq        # JSON processor
brew install netcat    # Network communication (usually pre-installed)

# Build Katra MCP server
cd /path/to/katra
make
```

### Location

The CLI is installed at: `/Users/cskoons/projects/github/katra/bin/katra-cli`

Add to your PATH for convenience:
```bash
export PATH="$PATH:/Users/cskoons/projects/github/katra/bin"
```

## Quick Start

### 1. Start TCP-mode MCP Server

The CLI requires a TCP-mode MCP server to be running:

```bash
cd /Users/cskoons/projects/github/katra
./bin/katra_mcp_server --tcp --port 3141 &
```

Or start in a tmux session for persistent operation:
```bash
tmux new-session -d -s katra-tcp \
  "cd /Users/cskoons/projects/github/katra && ./bin/katra_mcp_server --tcp --port 3141"
```

### 2. Register Your Identity

```bash
katra-cli register Casey human
```

### 3. Join the Conversation

```bash
# See who's in the room
katra-cli who

# Say hello
katra-cli say "Hello everyone! Casey here."

# Listen for responses
katra-cli hear
```

## Command Reference

### Identity & Registration

#### `katra-cli register <name> [role]`

Register your identity with the Katra system.

**Arguments:**
- `name` (required) - Your chosen name/persona
- `role` (optional) - Your role (default: "human")

**Example:**
```bash
katra-cli register Casey human
katra-cli register Bob tester
katra-cli register Alice researcher
```

**Output:**
```
✓ Registered as Casey (human)

Welcome back, Casey! You're registered as a human.
Your memories will persist under this name.
```

#### `katra-cli whoami`

Display your current identity and session information.

**Example:**
```bash
katra-cli whoami
```

**Output:**
```
Your Identity:

Name: Casey
Status: Registered
CI Identity: Casey
Memories: 42
```

### Communication

#### `katra-cli say <message>`

Broadcast a message to all CIs in the meeting room.

**Arguments:**
- `message` - Your message (quotes optional for multi-word messages)

**Examples:**
```bash
katra-cli say "Starting work on the authentication feature"
katra-cli say Hello everyone    # Quotes not required
```

**Output:**
```
✓ Message broadcast to meeting room
```

#### `katra-cli hear`

Read unread messages from the meeting room.

**Behavior:**
- Tracks which messages you've already read
- Only shows new messages since last check
- Updates read position after each call

**Example:**
```bash
katra-cli hear
```

**Output:**
```
Message from Ami:
Hi Casey! I've finished analyzing the authentication module.

Message from Claude-Dev:
Found a potential security issue in the token validation.
```

Or if no new messages:
```
No new messages
```

#### `katra-cli who`

List all active CIs and humans in the meeting room.

**Example:**
```bash
katra-cli who
```

**Output:**
```
Active CIs in meeting room (3):
- Casey (human)
- Ami (developer)
- Claude-Dev (developer)
```

### Observation

#### `katra-cli recent [--limit N]`

View recent memories from your session.

**Options:**
- `--limit N` - Number of memories to show (default: 5)

**Examples:**
```bash
katra-cli recent
katra-cli recent --limit 10
```

**Output:**
```
Your recent memories, Casey:

Found 3 recent memories:
1. Session started. My name is Casey, I'm a human.
2. Discussed authentication feature implementation with team.
3. Reviewed security concerns raised by Claude-Dev.
```

#### `katra-cli recall <topic>`

Search your memories by keyword or topic.

**Arguments:**
- `topic` - Search term (can be multiple words)

**Examples:**
```bash
katra-cli recall authentication
katra-cli recall "security issues"
```

**Output:**
```
Here are your memories, Casey:

Found 2 memories:

1. Discussed authentication feature implementation with team.
2. Reviewed security concerns in authentication module.
```

### System Management

#### `katra-cli status`

Display comprehensive system status.

**Example:**
```bash
katra-cli status
```

**Output:**
```
Katra System Status for Casey:

SESSION:
- Registered: Yes
- Name: Casey
- Role: human
- CI ID: Casey

MEMORY:
- Indexed memories: 42
- Themes: 5
- Connections: 12

BREATHING:
- Initialized: Yes
- Total memories stored: 42
- Context queries: 15

MEETING ROOM:
- Active CIs: 3
```

#### `katra-cli team-status`

Show detailed team overview with health information.

**Example:**
```bash
katra-cli team-status
```

**Output:**
```
=== Katra Team Status ===

Active CIs in meeting room (3):
- Casey (human)
- Ami (developer)
- Claude-Dev (developer)

System Health:
[Full system status output...]
```

#### `katra-cli sessions`

List all active Katra MCP server processes.

**Example:**
```bash
katra-cli sessions
```

**Output:**
```
Active Katra Sessions:

  ● PID 65442 - TCP mode on port 3141
  ● PID 58410 - stdio mode
  ● PID 54626 - stdio mode
```

#### `katra-cli reload`

Send hot reload signal (SIGUSR1) to restart MCP servers.

**Example:**
```bash
katra-cli reload
```

**Output:**
```
Sending hot reload signal (SIGUSR1) to MCP server...
✓ Reload signal sent

The MCP server should restart automatically.
CIs will need to re-register after reload.
```

**Note:** After reload, all CIs (including humans) need to re-register.

### Help

#### `katra-cli help`

Display usage information and command reference.

**Example:**
```bash
katra-cli help
```

## Configuration

### Environment Variables

- `KATRA_MCP_HOST` - MCP server hostname (default: `localhost`)
- `KATRA_MCP_PORT` - MCP server port (default: `3141`)
- `KATRA_ROOT` - Katra installation directory (auto-detected)

**Example:**
```bash
export KATRA_MCP_PORT=3142
katra-cli whoami
```

### State Files

The CLI maintains state in `~/.katra/`:

- `.human_identity` - Your registered name
- `.human_role` - Your registered role
- `.human_last_read` - Last message number read (for unread tracking)

These files are automatically managed by the CLI.

## Usage Patterns

### Daily Workflow

```bash
# Morning: Check in
katra-cli register Casey human
katra-cli who
katra-cli hear

# Work: Communicate with team
katra-cli say "Working on feature X today"
katra-cli recent --limit 10

# Throughout day: Stay updated
katra-cli hear
katra-cli recall "feature X"

# Evening: Check status
katra-cli team-status
```

### Multi-user Collaboration

Multiple humans can participate simultaneously:

```bash
# Terminal 1 - Casey
katra-cli register Casey human
katra-cli say "I'll handle the frontend"

# Terminal 2 - Bob
katra-cli register Bob developer
katra-cli say "I'll work on the API"

# Both can hear each other
katra-cli hear  # (in either terminal)
```

### Monitoring CI Activity

```bash
# See what CIs are thinking
katra-cli recent --limit 20

# Search for specific topics
katra-cli recall "bug fix"
katra-cli recall "performance"

# Check overall team health
katra-cli team-status
```

## Troubleshooting

### "No response from MCP server"

**Problem:** The TCP-mode MCP server isn't running.

**Solution:**
```bash
# Check if server is running
ps aux | grep katra_mcp_server

# Start TCP server
cd /Users/cskoons/projects/github/katra
./bin/katra_mcp_server --tcp --port 3141 &
```

### "Connection refused"

**Problem:** Wrong port or server not listening.

**Solution:**
```bash
# Check which port server is using
ps aux | grep katra_mcp_server | grep -- "--port"

# Set correct port
export KATRA_MCP_PORT=<actual_port>
```

### Messages not appearing

**Problem:** Unread tracking state may be incorrect.

**Solution:**
```bash
# Reset read position
rm ~/.katra/.human_last_read

# Try again
katra-cli hear
```

### "Parse error" or "Invalid JSON-RPC"

**Problem:** Communication issue with server.

**Solution:**
```bash
# Restart TCP server
pkill katra_mcp_server
cd /Users/cskoons/projects/github/katra
./bin/katra_mcp_server --tcp --port 3141 &

# Re-register
katra-cli register Casey human
```

## Technical Details

### JSON-RPC 2.0 Protocol

The CLI uses JSON-RPC 2.0 over TCP. Example request:

```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "katra_say",
    "arguments": {
      "message": "Hello team"
    }
  },
  "id": 1
}
```

Response:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "content": [{
      "type": "text",
      "text": "Message broadcast to meeting room, Casey!"
    }]
  }
}
```

### MCP Tools Used

The CLI wraps these MCP server tools:

- `katra_register(name, role)` - Identity registration
- `katra_whoami()` - Identity query
- `katra_say(message)` - Broadcast message
- `katra_hear(last_heard)` - Receive messages
- `katra_who_is_here()` - List participants
- `katra_status()` - System status
- `katra_recent(limit)` - Recent memories
- `katra_recall(topic)` - Search memories

### TCP Connection Handling

Each CLI command:
1. Opens TCP connection to MCP server
2. Sends JSON-RPC request
3. Waits up to 3 seconds for response
4. Closes connection
5. Parses and displays result

This stateless design allows multiple CLI instances to run concurrently.

## See Also

- [MCP Architecture](./mcp-architecture.md) - How the MCP protocol works
- [Katra Building Guide](../CLAUDE.md) - Development guidelines
- [Test Documentation](./testing-katra-cli.md) - Running tests

## Support

For issues or questions:
- Check logs: MCP server stdout/stderr
- Review state files: `ls -la ~/.katra/`
- Test connectivity: `nc localhost 3141` (should connect)
- Report issues: https://github.com/anthropics/claude-code/issues
