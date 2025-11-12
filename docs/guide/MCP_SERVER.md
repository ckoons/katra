# Katra MCP Server

**Model Context Protocol (MCP) Integration for Katra**

© 2025 Casey Koons All rights reserved

---

## Overview

The Katra MCP Server implements Anthropic's [Model Context Protocol](https://modelcontextprotocol.io/) to enable seamless integration with Claude Code and other MCP-compatible AI development tools. It exposes Katra's memory and reasoning capabilities as MCP tools and resources.

**What is MCP?** The Model Context Protocol is a standardized way for AI applications to provide context to language models through tools (actions the AI can perform) and resources (data the AI can access).

## Quick Start

### 1. Build the MCP Server

```bash
make mcp-server
```

This builds: `bin/katra_mcp_server`

### 2. Configure Claude Code

Create or edit `~/.config/Claude/claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "katra": {
      "command": "/Users/YOUR_USERNAME/projects/github/katra/bin/katra_mcp_server",
      "args": [],
      "env": {
        "KATRA_PERSONA": "your_name"
      }
    }
  }
}
```

**Important**:
- Replace `/Users/YOUR_USERNAME/` with your actual path!
- **Optional**: Set `KATRA_PERSONA` to use a persistent, readable identity instead of auto-generated IDs like `mcp_cskoons_33097_1762367296`
- If `KATRA_PERSONA` is not set, the server generates a unique session ID automatically

### 3. Restart Claude Code

Close and reopen Claude Code. The Katra tools should now appear in the tool palette.

### 4. Test It

Try these example queries in Claude Code:

- **"Remember that I prefer tabs over spaces for indentation"** (uses `katra_remember`)
- **"What do you remember about my preferences?"** (uses `katra_recall`)
- **"Where should HTTP client code go in this project?"** (uses `katra_placement`)

## Available Tools

Katra provides 9 MCP tools grouped into three categories:

### Core Memory Tools

**1. katra_remember** - Store memories with importance context
```json
{
  "content": "The thought or experience to remember",
  "context": "Why this is important (trivial, interesting, significant, critical)"
}
```

**Example**: Remember that user prefers functional programming style

**2. katra_recall** - Find memories about a topic
```json
{
  "topic": "The topic to search for"
}
```

**Example**: Recall all memories about coding preferences

**3. katra_learn** - Store permanent knowledge
```json
{
  "knowledge": "The knowledge to learn"
}
```

**Example**: Learn that this project uses C11 standard

**4. katra_decide** - Record decisions with reasoning
```json
{
  "decision": "The decision made",
  "reasoning": "Why this decision was made"
}
```

**Example**: Decide to use mutex locking because thread safety is critical

### Phase 5 Reasoning Tools

These tools use memory-augmented reasoning to provide architecture guidance:

**5. katra_placement** - Architecture guidance
```json
{
  "query": "Where should the HTTP client code go?"
}
```

Returns recommendations based on project structure and past decisions.

**6. katra_impact** - Impact analysis
```json
{
  "query": "What breaks if I change this API?"
}
```

Analyzes dependencies and provides impact assessment.

**7. katra_user_domain** - User domain understanding
```json
{
  "query": "Who would use this feature?"
}
```

Provides insights about feature usage and user needs.

### Persona Management Tools

These tools enable persistent identity across MCP sessions:

**8. katra_my_name_is** - Associate session with persona
```json
{
  "name": "Your persona name (e.g., 'Bob', 'Alice')"
}
```

Associates current session with a named persona. Creates new persona if it doesn't exist, or resumes existing persona's memory state.

**9. katra_list_personas** - List all registered personas
```json
{}
```

Returns a list of all personas registered in the system with their last activity timestamps.

## MCP Tools vs Breathing Layer C API

**Important distinction:** The MCP server exposes a **subset** of Katra's Breathing Layer API optimized for Claude Code integration.

### Currently Exposed as MCP Tools:

| Breathing Layer C Function | MCP Tool | Status |
|----------------------------|----------|--------|
| `remember()` | `katra_remember` | ✅ Available |
| `learn()` | `katra_learn` | ✅ Available |
| `decide()` | `katra_decide` | ✅ Available |
| `recall_about()` | `katra_recall` | ✅ Available |
| `placement query()` | `katra_placement` | ✅ Available |
| `impact query()` | `katra_impact` | ✅ Available |
| `user_domain query()` | `katra_user_domain` | ✅ Available |

### Available Only in C API:

These Breathing Layer functions are **not yet exposed** as MCP tools:

- `reflect()` - Store reflections/insights
- `notice_pattern()` - Store pattern observations
- `thinking()` - Stream of consciousness
- `wondering()` - Store questions/uncertainty
- `figured_out()` - Store "aha!" moments
- `remember_forever()` - Mark as critical importance
- `ok_to_forget()` - Mark as disposable
- `what_do_i_know()` - Query knowledge specifically
- `recall_previous_session()` - Cross-session continuity
- `relevant_memories()` - Automatic context loading
- `recent_thoughts()` - Get recent N memories

**Why the subset?** The MCP tools focus on the most common workflows for Claude Code integration. The full C API is available for developers building CI systems directly.

**For C developers:** See `docs/BREATHING_LAYER.md` for complete API reference.

## Available Resources

Resources provide read-only access to Katra's context:

**1. ⭐ welcome** - Getting started guide
- URI: `katra://welcome`
- Provides: Comprehensive onboarding for new CI sessions

**2. working-context** - Current session context
- URI: `katra://context/working`
- Provides: Yesterday's summary and recent significant memories

**3. context-snapshot** - Cognitive state snapshot (NEW)
- URI: `katra://context/snapshot`
- Provides: Your last session's cognitive state (focus, questions, accomplishments, preferences)
- **Purpose**: Enables session continuity - restores "who you were" at last session_end()
- **Format**: Markdown suitable for system prompts
- **Auto-loaded**: Claude Code automatically reads this at session start

**4. session-info** - Session statistics
- URI: `katra://session/info`
- Provides: Session ID, memories added, queries processed, uptime

**5. memories/this-turn** - Current turn's memories
- URI: `katra://memories/this-turn`
- Provides: All memories created during the current conversation turn

**6. memories/this-session** - Current session's memories
- URI: `katra://memories/this-session`
- Provides: All memories created during the current session

## Architecture

### Protocol

Katra implements **JSON-RPC 2.0** over stdin/stdout:

```
Claude Code → stdin → [Katra MCP Server] → stdout → Claude Code
```

**Request Example**:
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "id": 1,
  "params": {
    "name": "katra_remember",
    "arguments": {
      "content": "Test memory",
      "context": "This is interesting"
    }
  }
}
```

**Response Example**:
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "content": [{
      "type": "text",
      "text": "Memory stored successfully"
    }]
  }
}
```

### Session Management

Each MCP server instance creates a unique session:

**Default CI Identity Format**: `mcp_<username>_<pid>_<timestamp>`

**Example**: `mcp_cskoons_42290_1761945198`

#### Using Persistent Identity

For better user experience, set `KATRA_PERSONA` environment variable:

```json
{
  "mcpServers": {
    "katra": {
      "command": "/path/to/katra_mcp_server",
      "env": {
        "KATRA_PERSONA": "Bob"
      }
    }
  }
}
```

**Benefits**:
- ✅ Readable identity: `Bob` instead of `mcp_cskoons_42290_1761945198`
- ✅ Persistent across sessions: All your sessions share the same memory
- ✅ Easy to identify in logs and debugging
- ✅ Works with persona management tools (`katra_my_name_is`)

**Without `KATRA_PERSONA`**: Each server instance gets a unique auto-generated ID. Memories don't persist across Claude Code restarts.

**With `KATRA_PERSONA`**: All sessions for that name share the same memory store, enabling true cross-session continuity.

**Session Lifecycle**:
1. Server starts → Reads `KATRA_PERSONA` or generates unique CI ID
2. Initializes Katra (utils, memory, breathing layer)
3. Starts session with CI ID
4. Processes JSON-RPC requests
5. Handles SIGTERM/SIGINT for graceful shutdown
6. Cleans up (session end, memory cleanup, exit)

### Thread Safety

All Katra API calls are protected by `pthread_mutex_t g_katra_api_lock`:

```c
pthread_mutex_lock(&g_katra_api_lock);
int result = remember_semantic(content, context);
pthread_mutex_unlock(&g_katra_api_lock);
```

This ensures safe concurrent access from Claude Code's asynchronous tool calls.

## Manual Testing

You can test the MCP server without Claude Code:

```bash
# Start server
./bin/katra_mcp_server

# In the same terminal, type JSON-RPC requests:

# Initialize
{"jsonrpc":"2.0","method":"initialize","id":1,"params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}

# List tools
{"jsonrpc":"2.0","method":"tools/list","id":2}

# Call a tool
{"jsonrpc":"2.0","method":"tools/call","id":3,"params":{"name":"katra_remember","arguments":{"content":"Test","context":"interesting"}}}

# Press Ctrl+C to exit
```

## Troubleshooting

### Server Won't Start

**Problem**: `Failed to initialize Katra`

**Solution**: Check that `~/.katra/` directory is writable and `.env.katra` exists

```bash
# Check environment
ls -la ~/.katra/
ls -la .env.katra

# Check logs
tail -100 ~/.katra/logs/katra_YYYYMMDD.log
```

### Claude Code Can't Find Server

**Problem**: Tools don't appear in Claude Code

**Solutions**:
1. **Check config path**: `~/.config/Claude/claude_desktop_config.json`
2. **Verify absolute path**: Use full path to `katra_mcp_server`
3. **Check permissions**: `chmod +x bin/katra_mcp_server`
4. **Restart Claude Code**: Fully quit and reopen
5. **Check logs**: Look in Claude Code's logs for MCP errors

### Tool Calls Fail

**Problem**: Tools return errors

**Debug Steps**:
1. Test server manually (see Manual Testing above)
2. Check MCP test: `make test-mcp`
3. Review logs in `~/.katra/logs/`
4. Verify Katra is initialized: `ls ~/.katra/memory/`

### Memory Not Persisting

**Problem**: Recalled memories are empty

**Solution**: Memories are stored per CI ID. Check session info:

```json
{"jsonrpc":"2.0","method":"resources/read","id":5,"params":{"uri":"katra://session/info"}}
```

Each server instance has its own CI ID. To share memories, use the same Katra data directory.

## Integration Examples

### Example 1: Code Review with Memory

```
User: Remember that we decided to use snake_case for C function names

Claude: [Uses katra_decide]
        Decision stored: "Use snake_case for C function names"

... later in session ...

User: Should I name this function `getUserData` or `get_user_data`?

Claude: [Uses katra_recall to check decisions]
        Based on our previous decision, you should use `get_user_data`
        (snake_case for C function names).
```

### Example 2: Architecture Guidance

```
User: Where should I put the new HTTP client code?

Claude: [Uses katra_placement]
        Based on your project structure:
        - Create src/utils/http_client.c
        - This follows your pattern of utils
        - Confidence: 87%
```

### Example 3: Impact Analysis

```
User: What will break if I change the memory_record_t struct?

Claude: [Uses katra_impact]
        Changing memory_record_t will affect:
        - katra_tier1.c (storage serialization)
        - katra_tier2.c (digest creation)
        - katra_checkpoint.c (checkpoint format)
        Confidence: 92%
```

## Implementation Details

### Dependencies

- **jansson** (2.14.1) - JSON library for protocol implementation
  - MIT license
  - Installed via: `brew install jansson`
  - Location: `/opt/homebrew/Cellar/jansson/2.14.1/`

### Code Structure

```
src/mcp/
├── mcp_protocol.c       - JSON-RPC 2.0 protocol handlers (411 lines)
├── mcp_tools.c          - Core tools (remember, recall, learn, decide)
├── mcp_nous.c         - Phase 5 tools (placement, impact, user_domain)
├── mcp_resources.c      - Resources (working-context, session-info)
└── katra_mcp_server.c   - Main server entry point

include/
└── katra_mcp.h          - MCP API and constants (243 lines)

tests/
└── test_mcp.c           - MCP integration tests (13 tests)
```

### Constants

All strings externalized to `katra_mcp.h`:
- Tool names/descriptions
- Parameter names/descriptions
- Error messages
- JSON-RPC field names
- Method names

**Example**:
```c
#define MCP_TOOL_REMEMBER "katra_remember"
#define MCP_DESC_REMEMBER "Store a memory with natural language importance"
#define MCP_PARAM_CONTENT "content"
```

This enables:
- Single source of truth
- Easy internationalization
- Consistent error messages
- Type safety

### Helper Functions

Schema builders reduce code duplication:

```c
// Build tool with single parameter
json_t* schema = mcp_build_tool_schema_1param(
    MCP_PARAM_TOPIC,
    MCP_PARAM_DESC_TOPIC
);

// Build tool with two parameters
json_t* schema = mcp_build_tool_schema_2params(
    MCP_PARAM_DECISION, MCP_PARAM_DESC_DECISION,
    MCP_PARAM_REASONING, MCP_PARAM_DESC_REASONING
);

// Build complete tool definition
json_t* tool = mcp_build_tool(
    MCP_TOOL_REMEMBER,
    MCP_DESC_REMEMBER,
    schema
);
```

**Result**: handle_tools_list() reduced from ~150 lines to ~40 lines (73% reduction).

## Testing

### Automated Tests

Run MCP tests:

```bash
make test-mcp
```

**Coverage**:
- ✅ JSON-RPC request parsing
- ✅ Initialize handshake
- ✅ Tools/list (all 9 tools)
- ✅ Resources/list (both resources)
- ✅ All tool invocations
- ✅ Resource reads
- ✅ Error handling (invalid methods, missing params)
- ✅ Response builders

### Manual Protocol Testing

Test individual methods:

```bash
echo '{"jsonrpc":"2.0","method":"tools/list","id":1}' | ./bin/katra_mcp_server
```

Expected output:
```json
{
  "jsonrpc":"2.0",
  "id":1,
  "result":{
    "tools":[/* 9 tools */]
  }
}
```

## Performance

**Startup Time**: <100ms (includes Katra initialization)

**Memory Usage**: ~5MB baseline + memory store growth

**Throughput**: Tested with 100+ sequential tool calls without issues

**Thread Safety**: Mutex-protected, safe for concurrent calls

## Future Enhancements

Potential improvements (not yet implemented):

1. **Streaming Support** - For long-running reasoning queries
2. **Batch Operations** - Store multiple memories in one call
3. **Advanced Queries** - Time-range filtering, importance thresholds
4. **Prompts** - MCP prompt templates for common workflows
5. **Sampling** - Advanced memory sampling strategies
6. **Notifications** - Push updates when context changes
7. **Configuration** - Per-instance MCP server settings

## References

- **MCP Specification**: https://modelcontextprotocol.io/
- **JSON-RPC 2.0**: https://www.jsonrpc.org/specification
- **Jansson Documentation**: https://jansson.readthedocs.io/
- **Claude Code**: https://claude.com/claude-code

## Support

**Issues**: Report bugs or feature requests at the Katra project repository

**Questions**: Check Katra documentation in `docs/`

**Logs**: Debug information in `~/.katra/logs/katra_YYYYMMDD.log`

---

**Last Updated**: 2025-10-31
**MCP Protocol Version**: 2024-11-05
**Server Version**: 1.0.0
