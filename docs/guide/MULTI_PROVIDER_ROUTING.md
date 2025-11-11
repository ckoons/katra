<!-- © 2025 Casey Koons All rights reserved -->

# Multi-Provider Routing with Katra

**Quick setup guide for using Katra with multiple LLM providers**

---

## Overview

Katra supports **optional** multi-provider routing through Claude Code Router integration. This enables:
- Different personas using different LLM providers
- Cost optimization (route background tasks to cheaper models)
- Provider-specific features (web search, reasoning, etc.)
- Cross-provider communication (Alice on Claude, Bob on GPT-4)

**Default:** Katra works perfectly with Anthropic alone (no router needed).

---

## Quick Start

### 1. Install Claude Code Router

```bash
npm install -g @musistudio/claude-code-router
```

### 2. Configure Katra Personas

```bash
# Copy example config
cp ~/.katra/router/personas.json.example ~/.katra/router/personas.json

# Edit to add your personas and providers
$EDITOR ~/.katra/router/personas.json
```

**Minimal Example:**
```json
{
  "personas": {
    "Alice": {
      "provider": "anthropic",
      "model": "claude-sonnet-4"
    },
    "Bob": {
      "provider": "openai",
      "model": "gpt-4-turbo"
    }
  }
}
```

### 3. Generate Router Config

```bash
katra-router-config
```

This reads `~/.katra/router/personas.json` and generates `~/.claude-code-router/config.json`.

### 4. Set API Keys

```bash
export ANTHROPIC_API_KEY=your-anthropic-key
export OPENAI_API_KEY=your-openai-key
# Add other providers as needed
```

### 5. Start Router (in separate terminal)

```bash
ccr code --port 5173
```

### 6. Start Katra with Routing

```bash
# Terminal 1: Alice uses Claude
katra start --persona Alice --use-router

# Terminal 2: Bob uses GPT-4
katra start --persona Bob --use-router
```

---

## Configuration Reference

### Persona Config Structure

```json
{
  "version": "1.0",
  "default_provider": "anthropic",

  "personas": {
    "PersonaName": {
      "provider": "provider-name",
      "model": "model-name",
      "description": "Optional description",
      "api_key_env": "API_KEY_ENV_VAR"
    }
  },

  "providers": {
    "provider-name": {
      "api_base_url": "https://api.provider.com",
      "api_key_env": "PROVIDER_API_KEY",
      "models": ["model1", "model2"]
    }
  }
}
```

### Supported Providers

- **Anthropic**: claude-sonnet-4, claude-opus-4
- **OpenAI**: gpt-4-turbo, gpt-4, gpt-3.5-turbo
- **DeepSeek**: deepseek-coder, deepseek-chat
- **Gemini**: gemini-pro, gemini-ultra
- **Others**: Via OpenRouter, Volcengine, SiliconFlow

See `~/.katra/router/personas.json.example` for full config.

---

## Usage Patterns

### Pattern 1: Single Provider (Default)

```bash
# No router needed - direct connection
katra start --persona Alice
```

### Pattern 2: Multi-Provider Team

```bash
# Alice (Claude) - Developer
katra start --persona Alice --use-router

# Bob (GPT-4) - Reviewer
katra start --persona Bob --use-router

# Charlie (DeepSeek) - Tester (cost-optimized)
katra start --persona Charlie --use-router
```

### Pattern 3: Auto-Detection

```bash
# Uses router if installed and configured
katra start --persona Alice

# Explicitly disable router
katra start --persona Alice --no-router
```

---

## Cross-Provider Communication

Memory and communication work seamlessly across providers:

```bash
# Terminal 1: Alice (Claude)
katra_say("Testing cross-provider messaging")

# Terminal 2: Bob (GPT-4)
katra_hear()  # Receives Alice's message
```

**How it works:**
- Katra memory stored in provider-agnostic SQLite
- Meeting room communication database-backed
- Provider routing transparent to Katra core

---

## Troubleshooting

### Router not found

```bash
# Install router
npm install -g @musistudio/claude-code-router

# Verify installation
which ccr
```

### Configuration errors

```bash
# Validate config
katra-router-config --validate-only

# Regenerate router config
katra-router-config
```

### API key issues

```bash
# Check environment variables
env | grep _API_KEY

# Set missing keys
export ANTHROPIC_API_KEY=your-key-here
```

### Router not routing correctly

```bash
# Check router logs
ccr code --port 5173 --verbose

# Verify ANTHROPIC_BASE_URL is set
echo $ANTHROPIC_BASE_URL  # Should be http://localhost:5173/v1
```

---

## Advanced Topics

### Custom Routing Rules

Add routing rules for specific scenarios:

```json
{
  "routing_rules": {
    "background": {
      "provider": "deepseek",
      "model": "deepseek-chat"
    },
    "think": {
      "provider": "anthropic",
      "model": "claude-opus-4"
    }
  }
}
```

### Dynamic Provider Switching

```bash
# Start with one provider
katra start --persona Alice --use-router

# Switch provider mid-session
/model openai,gpt-4

# Katra memory persists across switch
```

---

## See Also

- [Phase 5 Plan](../plans/PHASE5_PLAN.md) - Complete Phase 5 specification
- [Developer Tools](./DEVELOPER_TOOLS.md) - Katra wrapper and k command
- [Claude Code Router Docs](https://github.com/musistudio/claude-code-router)

---

**Router integration is optional. Katra works perfectly with Anthropic alone.**
