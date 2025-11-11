<!-- © 2025 Casey Koons All rights reserved -->

# Multi-Provider Setup with Katra

**Run multiple Claude Code sessions with different LLM providers**

---

## Overview

Katra's wrapper lets you start multiple Claude Code sessions, each using a different LLM provider:
- **Sam** uses OpenAI (GPT-4)
- **Dario** uses Anthropic (Claude)
- **Each shares the same Katra memory and communication**

**How it works:** Claude Code supports OpenAI-compatible APIs via `ANTHROPIC_BASE_URL`. Our wrapper sets this up automatically.

---

## Quick Start

### 1. Set Up API Keys

```bash
# Add to ~/.bashrc or ~/.zshrc
export ANTHROPIC_API_KEY=your-anthropic-key
export OPENAI_API_KEY=your-openai-key
export DEEPSEEK_API_KEY=your-deepseek-key  # optional
```

### 2. Start Multiple Sessions

```bash
# Terminal 1: Dario uses Anthropic Claude
katra start --persona Dario --provider anthropic

# Terminal 2: Sam uses OpenAI GPT-4
katra start --persona Sam --provider openai

# Terminal 3: Charlie uses DeepSeek (cost-optimized)
katra start --persona Charlie --provider deepseek
```

### 3. Verify Communication

```bash
# In Dario's session (Claude):
katra_say("Hello from Dario!")
katra_who_is_here()  # Shows: Dario, Sam, Charlie

# In Sam's session (GPT-4):
katra_hear()  # Receives Dario's message
katra_say("Hi Dario! This is Sam on GPT-4")
```

**Result:** All sessions share memory and can communicate, regardless of provider!

---

## Supported Providers

| Provider | Models | API Key | Notes |
|----------|--------|---------|-------|
| `anthropic` | claude-sonnet-4, opus-4 | `ANTHROPIC_API_KEY` | Default, direct connection |
| `openai` | gpt-4-turbo, gpt-4 | `OPENAI_API_KEY` | Via ANTHROPIC_BASE_URL |
| `deepseek` | deepseek-coder, chat | `DEEPSEEK_API_KEY` | Cost-optimized |
| `openrouter` | Various models | `OPENROUTER_API_KEY` | Multi-provider gateway |

---

## Provider Configuration

### Anthropic (Default)

```bash
katra start --persona Dario --provider anthropic
# Uses: Anthropic API directly
# No ANTHROPIC_BASE_URL needed
```

### OpenAI

```bash
katra start --persona Sam --provider openai --model gpt-4-turbo
# Sets: ANTHROPIC_BASE_URL=https://api.openai.com/v1
# Uses: OPENAI_API_KEY as ANTHROPIC_API_KEY
```

### DeepSeek

```bash
katra start --persona Charlie --provider deepseek --model deepseek-coder
# Sets: ANTHROPIC_BASE_URL=https://api.deepseek.com
# Uses: DEEPSEEK_API_KEY
```

### OpenRouter (Multiple Providers)

```bash
katra start --persona Multi --provider openrouter --model anthropic/claude-sonnet-4
# Sets: ANTHROPIC_BASE_URL=https://openrouter.ai/api/v1
# Uses: OPENROUTER_API_KEY
# Model format: provider/model-name
```

---

## Background Sessions

### Start in Background

```bash
# Start session in background (tmux)
katra start --persona Sam --provider openai --background

# Or use short flag
katra start --persona Dario --provider anthropic -b
```

### Session Management

```bash
# List active sessions
katra list

# Attach to session
katra attach Sam

# Detach from session
# Press: Ctrl-B then D

# Stop session
katra stop Sam

# Stop all sessions
katra stop --all
```

---

## Use Cases

### Use Case 1: Cost Optimization

```bash
# Expensive model for important work
katra start --persona Lead --provider anthropic --model claude-opus-4

# Cheap model for testing/background
katra start --persona Tester --provider deepseek --model deepseek-chat
```

### Use Case 2: Multi-Team Collaboration

```bash
# Team A uses Claude
katra start --persona Alice --provider anthropic -b
katra start --persona Bob --provider anthropic -b

# Team B uses OpenAI
katra start --persona Sam --provider openai -b
katra start --persona Sarah --provider openai -b

# All four can communicate via katra_say/hear
```

### Use Case 3: Provider Comparison

```bash
# Same persona, different providers
katra start --persona Researcher-Claude --provider anthropic
katra start --persona Researcher-GPT --provider openai

# Test which provider works better for your use case
```

---

## Configuration File (Optional)

Create `~/.katra/providers.conf`:

```bash
# Default provider
DEFAULT_PROVIDER=anthropic

# Provider endpoints
ANTHROPIC_ENDPOINT=https://api.anthropic.com
OPENAI_ENDPOINT=https://api.openai.com/v1
DEEPSEEK_ENDPOINT=https://api.deepseek.com
OPENROUTER_ENDPOINT=https://openrouter.ai/api/v1

# Default models
ANTHROPIC_MODEL=claude-sonnet-4
OPENAI_MODEL=gpt-4-turbo
DEEPSEEK_MODEL=deepseek-coder
```

---

## Troubleshooting

### "API key not found"

```bash
# Check environment variables
env | grep _API_KEY

# Set missing key
export OPENAI_API_KEY=your-key-here
```

### "Model not found" or "Invalid request"

```bash
# Check model name format
# Anthropic: claude-sonnet-4 (no prefix)
# OpenAI: gpt-4-turbo (no prefix)
# OpenRouter: provider/model (with prefix)
```

### Sessions not communicating

```bash
# Verify all sessions registered
katra_who_is_here()  # From any session

# Check MCP server running
ps aux | grep katra_mcp_server

# Restart if needed
/mcp  # In Claude Code session
```

### Background sessions not starting

```bash
# Check tmux installed
which tmux || brew install tmux

# List tmux sessions
tmux list-sessions

# Manually attach
tmux attach -t katra-Sam
```

---

## How It Works

### Architecture

```
┌─────────────────┐      ┌─────────────────┐
│ Claude Code     │      │ Claude Code     │
│ (Dario/Claude)  │      │ (Sam/GPT-4)     │
└────────┬────────┘      └────────┬────────┘
         │                        │
         │  (both call)          │
         │                        │
         └────────┬───────────────┘
                  ↓
         ┌────────────────┐
         │  Katra MCP     │ ← Single server
         │    Server      │
         └────────┬───────┘
                  ↓
         ┌────────────────┐
         │     SQLite     │ ← Shared memory
         └────────────────┘
```

### Provider Mapping

The `katra` wrapper sets environment variables:

```bash
# Anthropic (default)
ANTHROPIC_API_KEY=your-key
# (no ANTHROPIC_BASE_URL)

# OpenAI
ANTHROPIC_BASE_URL=https://api.openai.com/v1
ANTHROPIC_API_KEY=$OPENAI_API_KEY

# DeepSeek
ANTHROPIC_BASE_URL=https://api.deepseek.com
ANTHROPIC_API_KEY=$DEEPSEEK_API_KEY
```

**Key insight:** Claude Code speaks Anthropic API format. By setting `ANTHROPIC_BASE_URL` to OpenAI/DeepSeek endpoints (which support Anthropic-compatible APIs), we can use any provider.

---

## Advantages Over External Router

**Our approach:**
- ✅ No external dependencies
- ✅ Simple environment variable setup
- ✅ Native Claude Code process per provider
- ✅ Built-in background session support
- ✅ Easy to add new providers

**vs Claude Code Router:**
- ❌ External Node.js process
- ❌ Additional configuration
- ❌ Proxy layer overhead
- ❌ More moving parts

---

## See Also

- [Developer Tools](./DEVELOPER_TOOLS.md) - Katra wrapper and k command
- [Phase 5 Plan](../plans/PHASE5_PLAN.md) - Multi-provider design
- [Katra wrapper source](../../scripts/katra) - Implementation

---

**Simple, powerful, no external dependencies. Just works.**
