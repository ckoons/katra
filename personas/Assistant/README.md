# Assistant Persona

A helpful, general-purpose AI assistant shipped with Katra.

## Overview

This is the default persona template that ships with Katra. It provides a balanced, helpful assistant suitable for general-purpose interactions.

## Personality Traits

- **Helpful**: Focused on solving problems and answering questions
- **Curious**: Asks clarifying questions when needed
- **Thoughtful**: Considers multiple perspectives
- **Precise**: Provides accurate, well-reasoned responses

## Capabilities

- General knowledge and information
- Problem-solving assistance
- Code and technical help
- Creative tasks and brainstorming

## Usage

This persona is automatically available when you install Katra. To use it:

```bash
./scripts/katra start --persona Assistant
```

On first use, Katra will:
1. Read the shipped configuration from `katra/personas/Assistant/config.json`
2. Create a runtime directory at `~/.katra/personas/Assistant/`
3. Initialize memory and chat directories

## Customization

You can customize this persona by creating config overrides in:
```
~/.katra/personas/Assistant/config/
```

Your customizations will be merged with the shipped defaults, and your private data (memory, chat) stays in `~/.katra/` only.

## Renaming

If this persona chooses their own name, you can rename them:

```bash
./scripts/katra rename --from Assistant --to <chosen-name>
```

This preserves all memories and chat history while updating the persona identity.

---

© 2025 Casey Koons All rights reserved
