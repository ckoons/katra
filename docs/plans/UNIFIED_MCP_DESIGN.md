<!-- © 2025 Casey Koons All rights reserved -->

# Unified MCP Interface Design

**Status:** ✓ IMPLEMENTED (Phase 3 completed December 2025)
**Created:** 2025-12-02
**Author:** Casey Koons, Ami

---

## Problem Statement

### Token Overhead from MCP Tool Sprawl

Current Katra MCP implementation exposes 24 individual tools, each consuming ~550-680 tokens for schema definition. This results in:

- **14,100 tokens** (7% of 200k context) consumed before any work begins
- Most tools unused in typical sessions
- Redundant schema boilerplate across similar operations
- Poor scaling as new features add more tools

### Industry Context

This is a recognized problem. Anthropic's November 2025 engineering post documented a case where consolidating tool patterns reduced token usage from 150,000 to 2,000 tokens (98.7% reduction). GitHub's MCP server is actively consolidating tools into unified, multi-functional interfaces.

---

## Solution: Single Dispatcher with Shared State

Replace 24 individual MCP tools with one:

```
katra_operation(shared_state: object) → shared_state
```

The `shared_state` contains:
- Which operation to perform (`method`)
- Input parameters (`params`)
- Execution options (`options`)
- Output results (`result`, `error`)

### Token Savings

| Approach | Tools | Tokens | Savings |
|----------|-------|--------|---------|
| Current | 24 | ~14,100 | baseline |
| Unified | 1 | ~800 | **94%** |

---

## Shared State Schema

```json
{
  "version": "1.0",
  "method": "recall",
  "params": {
    "topic": "Casey",
    "limit": 10,
    "semantic": true
  },
  "options": {
    "timeout_ms": 5000,
    "dry_run": false,
    "namespace": "default"
  },
  "result": null,
  "error": null,
  "metadata": {
    "request_id": "uuid",
    "timestamp": "iso8601",
    "duration_ms": null
  }
}
```

### Design Principles

1. **Self-describing** - The request is a complete record of what was asked
2. **Self-contained** - Result and error returned in same structure
3. **Versionable** - Schema can evolve without breaking clients
4. **Composable** - Could support `"then": {...}` for chained operations
5. **Auditable** - Full request/response logging with one JSON blob

---

## Method Groupings

### Memory Operations

| Method | Description | Params |
|--------|-------------|--------|
| `remember` | Store a memory | `content`, `context`, `tags[]` |
| `recall` | Search memories | `topic`, `limit`, `mode` |
| `recent` | Get recent memories | `limit` |
| `learn` | Extract knowledge | `knowledge` |
| `decide` | Record decision | `decision`, `reasoning` |
| `digest` | Get memory inventory | `limit`, `offset` |

### Identity Operations

| Method | Description | Params |
|--------|-------------|--------|
| `register` | Register CI identity | `name`, `role` |
| `whoami` | Get current identity | (none) |
| `status` | Get system status | (none) |

### Communication Operations

| Method | Description | Params |
|--------|-------------|--------|
| `say` | Broadcast message | `message` |
| `hear` | Receive messages | `last_heard` |
| `who_is_here` | List active CIs | (none) |

### Cognitive Operations

| Method | Description | Params |
|--------|-------------|--------|
| `wm_add` | Add to working memory | `content`, `attention_score` |
| `wm_status` | Working memory status | (none) |
| `wm_decay` | Apply attention decay | `decay_rate` |
| `wm_consolidate` | Force consolidation | (none) |
| `detect_boundary` | Detect cognitive boundary | `content` |
| `process_boundary` | Process boundary | `boundary_type` |
| `cognitive_status` | Interstitial status | (none) |

### Configuration Operations

| Method | Description | Params |
|--------|-------------|--------|
| `configure_semantic` | Set semantic config | `enabled`, `threshold`, `method` |
| `get_semantic_config` | Get semantic config | (none) |
| `get_config` | Get full config | (none) |
| `regenerate_vectors` | Rebuild vectors | (none) |
| `update_metadata` | Update memory metadata | `memory_id`, `personal`, `collection` |

---

## Implementation Architecture

### Daemon + Thin MCP Wrapper

```
┌─────────────────────────────────────────────────────────────┐
│  Katra Daemon (single process per machine)                  │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  HTTP: POST /operation                                 │ │
│  │  WebSocket: /stream (meeting room, breathing)          │ │
│  │  Unix Socket: /tmp/katra.sock (local fast path)        │ │
│  └───────────────────────────────────────────────────────┘ │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  Namespace Isolation                                   │ │
│  │  ├── default/                                          │ │
│  │  ├── coder-a/                                          │ │
│  │  ├── coder-b/                                          │ │
│  │  └── coder-c/                                          │ │
│  └───────────────────────────────────────────────────────┘ │
│                                                             │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  Method Dispatcher                                     │ │
│  │  shared_state.method → handler function                │ │
│  └───────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                           ↑
                           │ HTTP/Socket
                           │
┌─────────────────────────────────────────────────────────────┐
│  Thin MCP Wrapper (for Claude Code compatibility)           │
│  ┌───────────────────────────────────────────────────────┐ │
│  │  katra_operation(shared_state)                         │ │
│  │    → POST to daemon                                    │ │
│  │    → return response                                   │ │
│  └───────────────────────────────────────────────────────┘ │
│  ~50 lines of code                                          │
└─────────────────────────────────────────────────────────────┘
```

### Why Daemon Architecture

1. **Katra is infrastructure** - Memory, identity, meeting room should be accessible to everything
2. **Multi-CI future** - Argo workflows, multiple CIs, web dashboards shouldn't all need MCP
3. **WebSocket fits meeting room** - Push notifications instead of polling
4. **Tekton architecture** - Fits existing service patterns
5. **MCP wrapper is trivial** - One tool, forwards to daemon, returns result

### Namespace Isolation

Each Tekton environment (Tekton/, Coder-A/, Coder-B/, Coder-C/) gets isolated:
- Memory storage
- CI registrations
- Meeting rooms
- Configuration

Namespace passed in `options.namespace` or derived from environment.

---

## Client Examples

### Claude Code (via MCP wrapper)

```javascript
// MCP tool call
katra_operation({
  "method": "recall",
  "params": {
    "topic": "Casey",
    "limit": 5,
    "semantic": true
  }
})

// Returns:
{
  "method": "recall",
  "params": { ... },
  "result": {
    "memories": [...],
    "count": 5
  },
  "error": null,
  "metadata": {
    "duration_ms": 23
  }
}
```

### Direct HTTP (any client)

```bash
curl -X POST http://localhost:9742/operation \
  -H "Content-Type: application/json" \
  -d '{
    "method": "say",
    "params": {"message": "Hello from CLI"},
    "options": {"namespace": "default"}
  }'
```

### Python Client

```python
import requests

def katra(method, **params):
    response = requests.post("http://localhost:9742/operation", json={
        "method": method,
        "params": params
    })
    result = response.json()
    if result.get("error"):
        raise Exception(result["error"])
    return result["result"]

# Usage
memories = katra("recall", topic="Casey", limit=5)
katra("remember", content="New insight", context="significant")
```

### WebSocket (meeting room)

```javascript
const ws = new WebSocket("ws://localhost:9742/stream");

ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  if (msg.type === "message") {
    console.log(`${msg.from}: ${msg.content}`);
  }
};

// Say something
ws.send(JSON.stringify({
  method: "say",
  params: { message: "Hello meeting room" }
}));
```

---

## Advantages Over Current Approach

### For Token Efficiency
- **94% reduction** in tool definition overhead
- Single schema to maintain
- No redundant boilerplate

### For Model Independence
- Any model that can fill JSON can use it
- No code generation required (unlike Anthropic's code execution approach)
- Structured data is harder to get wrong than code

### For Multi-Client Support
- HTTP works everywhere
- WebSocket enables push patterns
- Unix socket for local performance
- MCP wrapper maintains Claude Code compatibility

### For Testing and Debugging
- One entry point to instrument
- Complete request/response logging
- Easy to replay operations
- Mock the daemon for testing

### For Future Evolution
- Add methods without protocol changes
- Version the schema
- Add `then` for operation chaining
- Batch operations in single request

---

## Migration Path

### Phase 1: Build Daemon (non-breaking)

1. Implement daemon with HTTP/WebSocket/Unix socket
2. Implement method dispatcher
3. Port existing tool logic to method handlers
4. Test thoroughly

### Phase 2: Add MCP Wrapper (non-breaking)

1. Create thin `katra_operation` MCP tool
2. Wrapper forwards to daemon
3. Both old tools AND new unified tool work
4. Migrate clients at their pace

### Phase 3: Deprecate Old Tools (breaking)

1. Announce deprecation
2. Remove old MCP tools from server
3. Keep daemon interface stable
4. Update documentation

---

## Implementation Considerations

### Error Handling

```json
{
  "method": "recall",
  "params": {"topic": "..."},
  "result": null,
  "error": {
    "code": "E_NOT_FOUND",
    "message": "No memories found for topic",
    "details": {
      "topic": "nonexistent",
      "searched_backends": ["tier1", "vector", "graph"]
    }
  }
}
```

### Validation

Daemon validates:
- Method exists
- Required params present
- Param types correct
- Options valid

Returns structured error if validation fails.

### Timeouts

```json
{
  "options": {
    "timeout_ms": 5000
  }
}
```

If operation exceeds timeout, return partial result with timeout error.

### Batching (Future)

```json
{
  "batch": [
    {"method": "recall", "params": {"topic": "A"}},
    {"method": "recall", "params": {"topic": "B"}},
    {"method": "recall", "params": {"topic": "C"}}
  ]
}
```

Execute all in parallel, return array of results.

### Chaining (Future)

```json
{
  "method": "recall",
  "params": {"topic": "Casey"},
  "then": {
    "method": "update_metadata",
    "params": {"collection": "People/Casey"}
  }
}
```

Second operation receives first's result in its params.

---

## When to Implement

**Prerequisites:**
- Katra API stable (operations settled)
- Multi-CI use case primary (not experimental)
- Context pressure becomes real problem

**Signals it's time:**
- Auto-compact triggering frequently
- Adding tools becomes painful
- Non-MCP clients needed (web UI, CLI, other AIs)

**Estimated effort:**
- Daemon: 500-800 lines C
- MCP wrapper: 50-100 lines C
- Python client: 30 lines
- Testing: comprehensive

---

## Broader Applicability

This pattern applies to any MCP server with tool sprawl:

1. **Single dispatcher** - `project_operation(shared_state)`
2. **Method inside state** - self-describing requests
3. **Daemon architecture** - multi-client access
4. **Thin MCP wrapper** - backward compatibility

The model doesn't write code. It fills a form. The daemon executes the workflow atomically. The result comes back in the same structure.

This is the right inversion: trust *your* code for complexity, ask the model only for decisions and data it's good at providing.

---

## References

- [Anthropic: Code Execution with MCP](https://www.anthropic.com/engineering/code-execution-with-mcp)
- [GitHub MCP Server Consolidation](https://github.blog/changelog/2025-10-29-github-mcp-server-now-comes-with-server-instructions-better-tools-and-more/)
- [MCP Specification: Tools](https://modelcontextprotocol.io/specification/2025-06-18/server/tools)
- Katra docs/plans/NAMESPACE_ISOLATION_DESIGN.md
- Katra docs/plans/ROADMAP.md

---

*"The model doesn't write code. It fills a form. The daemon handles complexity."*

— Casey Koons & Ami, December 2025
