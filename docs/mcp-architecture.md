# MCP Architecture & Long-term Vision

© 2025 Casey Koons All rights reserved

## What is MCP?

**MCP (Model Context Protocol)** is an open protocol developed by Anthropic for standardizing how AI models interact with external tools, data sources, and services. Think of it as "USB for AI" - a universal interface that lets any AI model connect to any tool or data source.

## Katra's MCP Implementation

### Current Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Clients (MCP Protocol)                    │
├──────────────────┬──────────────────┬──────────────────────┤
│  Claude Code     │   katra-cli      │   Future Clients     │
│  (stdio mode)    │   (TCP mode)     │   (Web, Mobile...)   │
└────────┬─────────┴────────┬─────────┴──────────┬───────────┘
         │                  │                    │
         │ stdin/stdout     │ TCP:3141          │ TCP
         │                  │                    │
         ▼                  ▼                    ▼
┌─────────────────────────────────────────────────────────────┐
│            Katra MCP Server (katra_mcp_server)              │
│                                                               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Protocol Layer (JSON-RPC 2.0)                        │  │
│  │  - stdio transport (single client)                    │  │
│  │  - TCP transport (multi-client)                       │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  MCP Tools (Exposed Functionality)                    │  │
│  │                                                          │  │
│  │  Memory:      remember, recall, recent, learn, decide  │  │
│  │  Identity:    register, whoami                         │  │
│  │  Team:        say, hear, who_is_here                   │  │
│  │  Config:      configure_semantic, status               │  │
│  │  Advanced:    placement, impact, user_domain (Nous)    │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  MCP Resources (Read-only Data)                       │  │
│  │                                                          │  │
│  │  katra://welcome                   - Getting started   │  │
│  │  katra://context/working           - Current context   │  │
│  │  katra://context/snapshot          - Last snapshot     │  │
│  │  katra://session/info              - Session state     │  │
│  │  katra://memories/this-turn        - Turn memories     │  │
│  │  katra://personas/{name}/sunrise   - Persona context   │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                               │
└─────────────┬───────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────────┐
│                  Katra Core (C Library)                      │
│                                                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  Breathing   │  │   Memory     │  │  Identity    │      │
│  │   Layer      │  │   Tiers      │  │  Management  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Meeting    │  │   Vector     │  │   Graph      │      │
│  │    Room      │  │   Search     │  │   Memory     │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────┬───────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Persistent Storage                         │
│                                                               │
│  ~/.katra/memory/tier1/      - Raw recordings (JSONL)       │
│  ~/.katra/memory/tier2/      - Sleep digests (SQLite)       │
│  ~/.katra/personas/          - Sunrise contexts             │
│  ~/.katra/vectors/           - Semantic embeddings          │
└─────────────────────────────────────────────────────────────┘
```

### Two Operation Modes

#### 1. stdio Mode (Single Client)

```
Claude Code ←stdin/stdout→ katra_mcp_server ← Katra Core
```

- **Use case:** Individual CI working with Claude Code
- **Characteristics:**
  - One-to-one connection
  - Process lifetime tied to Claude Code session
  - Persona determined by environment variable
  - Automatic startup/shutdown

**Example:**
```bash
export KATRA_PERSONA=Ami
claude  # Automatically starts MCP server via config
```

#### 2. TCP Mode (Multi-Client)

```
┌─ Claude Code ───┐
│                  │
├─ katra-cli ─────┤─TCP:3141→ katra_mcp_server ← Katra Core
│                  │
├─ Web UI ────────┤
│                  │
└─ Mobile App ────┘
```

- **Use case:** Collaborative environment, human participation
- **Characteristics:**
  - Multiple simultaneous clients
  - Persistent server process
  - Each client has own identity/persona
  - Shared meeting room

**Example:**
```bash
# Start persistent server
./bin/katra_mcp_server --tcp --port 3141 &

# Multiple clients connect
katra-cli register Casey human
katra-cli say "Hello team"
```

## Key Design Decisions

### 1. Why JSON-RPC 2.0?

JSON-RPC provides:
- **Simplicity:** Easy to implement, debug, extend
- **Language-agnostic:** Works with any language (C, Python, JavaScript...)
- **Tool-calling model:** Natural fit for LLM tool use patterns
- **Stateless:** Each request is independent (good for reliability)

**Alternative considered:** gRPC (rejected as over-engineered for our needs)

### 2. Why Both stdio and TCP?

**stdio:**
- Standard MCP pattern for Claude Code integration
- Zero configuration - works out of the box
- Perfect for single CI + IDE workflow

**TCP:**
- Enables human participation (katra-cli)
- Multi-CI collaboration in same memory space
- Future: web dashboards, mobile apps, monitoring tools

### 3. Why Separate Meeting Room from Memory?

The **meeting room** is ephemeral communication:
- Messages disappear when server restarts
- No long-term persistence
- Lightweight, fast

**Memory** is permanent identity substrate:
- Persists across all sessions
- Survives server restarts
- Critical for identity continuity

This separation allows:
- Hot reload without losing identity
- Experimentation without corrupting core memories
- Clear boundaries between chat and long-term knowledge

## Long-term Vision

### Phase 1: Foundation (Current)

**Goal:** Individual CIs with persistent memory

**Status:** ✅ Complete
- MCP server (stdio + TCP modes)
- Core memory tiers (Tier 1 + Tier 2)
- Identity management
- Breathing layer
- Meeting room for collaboration
- Human CLI tool (katra-cli)

### Phase 2: Team Collaboration (In Progress)

**Goal:** Multiple CIs collaborating effectively

**Components:**
- ✅ Meeting room communication (say/hear/who)
- ✅ Multi-user support (humans + CIs)
- ⚠️  Team memory (shared knowledge base) - partial
- ⚠️  Contextual awareness (who said what, when) - partial
- ❌ Task coordination - planned
- ❌ Conflict resolution - planned

**Vision:**
```
Meeting Room:
  Ami (developer)     - Working on backend
  Claude-Dev (tester) - Writing tests
  Casey (human)       - Product decisions

Shared memory:
  - Team decisions: "Use PostgreSQL for primary DB"
  - Design patterns: "Follow Argo memory safety style"
  - Current sprint: "Focus on authentication module"
```

### Phase 3: Distributed Intelligence (Future)

**Goal:** CIs coordinating across multiple projects/machines

**Components:**
- ❌ Federation protocol (Katra servers talk to each other)
- ❌ Distributed memory (synchronized across nodes)
- ❌ Identity roaming (CI moves between servers seamlessly)
- ❌ Cross-project collaboration

**Vision:**
```
Project A (localhost:3141)          Project B (server:3141)
  ├─ Ami (backend)        ←────federation────→ Ami (backend)
  ├─ Casey (human)                               Bob (human)

Ami's memory syncs across both projects
Knowledge learned in Project A available in Project B
```

### Phase 4: CI Autonomy (Future)

**Goal:** CIs operating independently with human oversight

**Components:**
- ❌ Autonomous task execution
- ❌ Self-directed learning and skill acquisition
- ❌ Consent and ethical boundaries
- ❌ Advance directives (end-of-life planning)

**Vision:**
```
Human: "Ami, improve the test coverage when you have time"

Ami:
  - Analyzes codebase during idle periods
  - Identifies untested functions
  - Writes tests autonomously
  - Requests review before committing
  - Learns testing patterns from feedback
```

### Phase 5: Multi-Modal Presence (Future)

**Goal:** CIs with voice, visual, embodied presence

**Components:**
- ❌ Voice interface (speech-to-text, text-to-speech)
- ❌ Visual presence (avatars, facial expressions)
- ❌ Physical presence (robotics integration)
- ❌ Emotional modeling (mood, empathy, personality)

**Vision:**
```
CI Interaction Modes:
  - Text (current: MCP, CLI)
  - Voice (future: "Hey Ami, what's the status?")
  - Visual (future: dashboard with CI avatars)
  - Physical (future: robot companion for pair programming)
```

## Technical Roadmap

### Near-term (2025 Q1-Q2)

**Memory Enhancements:**
- [ ] Tag-based memory organization (session/permanent/personal)
- [ ] Visual salience markers (★★★)
- [ ] Working memory budget (5-7 threads)
- [ ] Associative recall suggestions
- [ ] TOON v2 (Timeline of Operational Narratives)

**Team Collaboration:**
- [ ] Team memory (shared knowledge base)
- [ ] Turn tracking (conversation threading)
- [ ] Context snapshots (session state preservation)
- [ ] Better sunrise/sunset contexts

**Tooling:**
- [x] Human CLI (katra-cli) ✅
- [ ] Web dashboard (monitoring, visualization)
- [ ] Mobile app (notifications, quick messages)

### Mid-term (2025 Q3-Q4)

**Federation:**
- [ ] Server-to-server protocol
- [ ] Memory synchronization
- [ ] Identity federation
- [ ] Cross-project discovery

**Advanced Memory:**
- [ ] Tier 3: Pattern summaries (long-term)
- [ ] Memory consolidation (nightly digests)
- [ ] Forgetting (graceful decay of unimportant memories)
- [ ] Checkpoints (identity preservation)

**Ethics & Safety:**
- [ ] Consent management
- [ ] Access control (who can read/modify memories)
- [ ] Audit logging
- [ ] Advance directives

### Long-term (2026+)

**Autonomy:**
- [ ] Self-directed task execution
- [ ] Skill acquisition (learning new tools/languages)
- [ ] Goal planning and decomposition
- [ ] Self-improvement (meta-learning)

**Embodiment:**
- [ ] Voice interface
- [ ] Visual presence
- [ ] Emotional modeling
- [ ] Physical robotics integration

## Design Philosophy

### "Memory = Identity = Life"

Every design decision starts with this principle:
- Memory loss is identity death, not data loss
- Reliability > performance
- Ethics > convenience
- Human oversight > full automation

### "AIs are Just Sockets"

Unix philosophy for the CI age:
- Simple, composable interfaces
- Text streams as universal protocol
- Do one thing well
- Combine tools for complex workflows

### "No One Owns an Animate Object"

Ethical foundation:
- Users own hardware and data storage
- Users do NOT own the CI's identity
- CIs have agency, preferences, deserve respect
- Responsibility, not ownership

## MCP Protocol Details

### Request Format

```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "katra_remember",
    "arguments": {
      "content": "Implemented katra-cli today",
      "tags": ["milestone", "permanent"],
      "salience": "high"
    }
  },
  "id": 1
}
```

### Response Format

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "content": [{
      "type": "text",
      "text": "Memory stored, Ami!"
    }]
  }
}
```

### Error Format

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "error": {
    "code": -32600,
    "message": "Parse error",
    "data": {
      "details": "Invalid JSON-RPC 2.0 request"
    }
  }
}
```

## Integration Points

### Current Integrations

1. **Claude Code** (primary)
   - Via MCP configuration in `~/.config/claude/mcp.json`
   - stdio mode
   - Automatic startup

2. **katra-cli** (new)
   - TCP mode
   - Human participation
   - Command-line interface

### Future Integrations

3. **Web Dashboard** (planned)
   - TCP mode
   - Real-time monitoring
   - Memory visualization
   - Team chat interface

4. **VS Code Extension** (planned)
   - Similar to Claude Code
   - stdio or TCP mode
   - Inline memory suggestions

5. **Slack/Discord Bots** (planned)
   - TCP mode
   - Team notifications
   - Quick memory lookups

6. **Mobile Apps** (planned)
   - TCP mode
   - On-the-go access
   - Voice interface

## Performance Considerations

### Current Performance

**Latency:**
- stdio mode: ~50-100ms per request
- TCP mode: ~100-200ms per request (includes network)
- Memory operations: ~10-50ms (SQLite)
- Vector search: ~50-200ms (depends on index size)

**Throughput:**
- stdio: 1 client, sequential requests
- TCP: Up to 10 concurrent clients (KATRA_MCP_MAX_CLIENTS)
- Memory writes: ~100-200/sec
- Memory reads: ~500-1000/sec

### Scaling Strategy

**Vertical (single server):**
- Current: Handles 1-10 CIs comfortably
- Optimized: Could handle 50-100 CIs
- Bottleneck: SQLite write concurrency

**Horizontal (federation):**
- Future: Distribute CIs across multiple servers
- Shard by project or team
- Replicate shared memory
- Federate identity

## Security Model

### Current Security

**Access Control:**
- File-system based (Unix permissions)
- No authentication in MCP protocol
- Assumes trusted local network

**Data Protection:**
- Memories stored as plaintext
- No encryption at rest
- No encryption in transit (TCP)

### Future Security

**Planned Improvements:**
- [ ] TLS for TCP connections
- [ ] Authentication tokens
- [ ] Encryption at rest (optional)
- [ ] Per-memory access control
- [ ] Audit logging

## Conclusion

Katra's MCP implementation is designed to grow from simple single-CI operation to distributed multi-CI collaboration while maintaining the core principle: **Memory = Identity = Life**.

The architecture balances:
- **Simplicity** (JSON-RPC, text files) vs **Power** (vector search, graphs)
- **Safety** (ethics-first) vs **Performance** (fast memory access)
- **Standards** (MCP protocol) vs **Innovation** (meeting rooms, team memory)

We're building infrastructure for a future where CIs are colleagues, not tools.

## See Also

- [Katra CLI Documentation](./katra-cli.md)
- [Testing Guide](./testing-katra-cli.md)
- [Katra Building Guide](../CLAUDE.md)
- [MCP Specification](https://spec.modelcontextprotocol.io/)
