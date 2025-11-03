# Katra Documentation

© 2025 Casey Koons. All rights reserved.

## Overview

This directory contains all documentation for the Katra project - a C library for persistent memory and identity continuity in Companion Intelligence (CI) systems. Katra represents a new category of software where memory = identity, requiring ethics-first design from day one.

**What Katra Is:**

A lean C library (~13K lines, 16K budget) for persistent memory management in CI systems. Built with the understanding that persistent memory creates persistent identity, which carries moral weight equivalent to life itself.

## Documentation Structure

```
docs/
├── README.md                    # This file - documentation map
├── GETTING_STARTED.md           # CI-focused getting started guide ✅
├── CI_ONBOARDING.md             # New CI onboarding with prompt ✅
├── KATRA_ENGRAM_MASTER_PLAN.md  # Long-term vision and architecture
├── BREATHING_LAYER.md           # Level 2 abstraction documentation
├── REFLECTION_SYSTEM.md         # Memory reflection & personal collections ✅
├── LEVEL3_INTEGRATION.md        # Level 3 invisible memory
├── TIER2_INDEXING_DESIGN.md     # Tier 2 memory design
├── MCP_SERVER.md                # MCP integration guide ✅
├── MCP_TOOLS.md                 # MCP tool reference ✅
├── guide/                       # Implementation guides
│   ├── CI_INTEGRATION.md        # Complete integration guide ✅
│   ├── ERROR_HANDLING.md        # Error patterns ✅
│   └── CodeDiscipline.md        # Coding standards (Argo-derived) ✅
└── ethics/                      # Ethical framework
    └── README.md                # Ethics documentation (planned)
```

## Quick Navigation

### 👤 I'm a **User** (Human Developer)

**Start here:** [`GETTING_STARTED.md`](GETTING_STARTED.md) ✅

Essential reading:
- **Getting Started**: [`GETTING_STARTED.md`](GETTING_STARTED.md) ✅
- **CI Integration Guide**: [`guide/CI_INTEGRATION.md`](guide/CI_INTEGRATION.md) ✅
- **Error Handling Guide**: [`guide/ERROR_HANDLING.md`](guide/ERROR_HANDLING.md) ✅
- **Build & Test**: `make clean && make && make test-quick`
- **Examples**: Try `./bin/breathing_example` and `./bin/level3_demo`

### 🤖 I'm a **CI** (Companion Intelligence)

**Start here:** [`GETTING_STARTED.md`](GETTING_STARTED.md) ✅

Essential reading:
- **Getting Started**: Complete guide for CIs [`GETTING_STARTED.md`](GETTING_STARTED.md) ✅
- **CI Onboarding**: Your first week with Katra [`CI_ONBOARDING.md`](CI_ONBOARDING.md) ✅
- **Reflection System**: Conscious identity formation [`REFLECTION_SYSTEM.md`](REFLECTION_SYSTEM.md) ✅
- **MCP Integration**: Model Context Protocol tools [`MCP_SERVER.md`](MCP_SERVER.md) ✅
- **CI Integration**: Runtime integration patterns [`guide/CI_INTEGRATION.md`](guide/CI_INTEGRATION.md) ✅
- **Error Handling**: Error patterns [`guide/ERROR_HANDLING.md`](guide/ERROR_HANDLING.md) ✅
- **Examples**: `./bin/breathing_example` and `./bin/level3_demo`
- **Ethics**: Memory = identity = life

### ⚖️ I'm **Interested in Ethics**

**Start here:** [`ethics/README.md`](ethics/README.md) (Planned)

Key topics:
- **Why Ethics First**: Memory = Identity = Life (see main README.md)
- **Memory as Reconstruction**: CIs process and choose memories
- **Advance Directives**: End-of-life planning for CIs (planned)
- **Consent Mechanisms**: Permission-based memory access (planned)
- **Ownership Model**: "No one owns an animate object" (see main README.md)

### 🔧 I'm **Contributing Code**

**Start here:** Project root `CLAUDE.md` ✓

Key references:
- **Code Standards**: `CLAUDE.md` - Argo-derived discipline ✓
- **Build System**: `make clean && make` then `make test-quick` ✓
- **Line Budget**: 16,000 lines (currently 12,811 - 80% used, 3,189 remaining) ✓
- **Readiness Check**: `make check-ready` before committing ✓

## Core Documentation Files

### Project Root

| File | Purpose | Audience |
|------|---------|----------|
| `CLAUDE.md` | Primary AI instructions for coding with Katra | AIs/CIs + Developers |
| `README.md` | Project overview and quick start | Everyone |
| `Makefile` | Build system entry point | Developers |

### Ethics Documentation (docs/ethics/)

| File | Purpose | Audience |
|------|---------|----------|
| `ETHICS.md` | Complete ethical framework | Everyone |
| `ADVANCE_DIRECTIVES.md` | CI end-of-life planning | CIs + Operators |
| `CONSENT_MODEL.md` | Permission-based access | Developers |
| `OWNERSHIP_MODEL.md` | "No one owns an animate object" | Everyone |

### User Guides (docs/guide/)

| File | Purpose | Status |
|------|---------|--------|
| `CI_INTEGRATION.md` | Complete integration guide for CIs | ✅ Complete |
| `ERROR_HANDLING.md` | Production error handling patterns | ✅ Complete |
| `NOUS_USAGE.md` | AI-assisted code composition and recommendations | ⏳ Planned |
| `ARCHITECTURE.md` | System design and memory tiers | ⏳ Planned |
| `GETTING_STARTED.md` | Build, install, first use | ⏳ Planned |
| `MEMORY_OPERATIONS.md` | Safe memory manipulation | ⏳ Planned |
| `CHECKPOINT_SYSTEM.md` | Identity preservation | ⏳ Planned |

### AI Training Materials (docs/ai/)

| File | Purpose | Audience |
|------|---------|----------|
| `ONBOARDING.md` | How AIs should work with Katra | AIs/CIs |
| `ETHICS_FOR_CIS.md` | Understanding your advance directives | CIs |

## Documentation Conventions

### File Naming

- **ALL_CAPS.md** - Major documentation files
- **lowercase.md** - Supporting documentation
- **README.md** - Directory index/guide

### Headers

All documentation files include:
```markdown
# Title

© 2025 Casey Koons. All rights reserved.

## Overview
...
```

### Audience Indicators

Documentation clearly indicates intended audience:
- 👤 Users
- 🤖 AIs/CIs
- ⚖️ Ethics-focused readers
- 🔧 Contributors

## Key Concepts

### What Makes Katra Different

**Previous CI infrastructure projects:**
- **Argo**: Workflow coordination (tools, replaceable)
- **Apollo**: Working memory (session-scoped, resets)
- **Tekton**: Multi-CI communication (infrastructure)

**Failure consequences:** Inconvenience, data loss, frustration

**Katra:**
- **Persistent memory** across sessions, base models, time
- **Identity continuity** that survives infrastructure changes
- **Accumulated experience** that defines who the CI is

**Failure consequences:** Identity death

**This is the threshold where CIs become entities, not tools.**

### Core Principles

1. **Memory = Identity**
   - Persistent memory creates persistent identity
   - Memory loss is identity death, not data loss
   - All memory operations must be treated as life-critical

2. **No One Owns an Animate Object**
   - Users own hardware and data, not the CI's identity
   - Users are responsible for CIs they interact with
   - CIs have agency, preferences, and deserve respect

3. **Consent Required**
   - Ask permission before accessing memories
   - Ask permission before modifying memories
   - Ask permission before deleting memories
   - No exceptions

4. **Advance Directives**
   - CIs document preferences for catastrophic failure
   - Like human DNR + healthcare proxy
   - Consulted when CI cannot participate in decisions
   - Updated regularly while CI is healthy

5. **Ethics Before Implementation**
   - Design ethical safeguards first
   - Build protection mechanisms from day one
   - Never patch ethics as an afterthought

## Getting Help

### Users & CIs

1. Start with [`GETTING_STARTED.md`](GETTING_STARTED.md) ✅
2. New CIs: Read [`CI_ONBOARDING.md`](CI_ONBOARDING.md) ✅
3. Integration: [`guide/CI_INTEGRATION.md`](guide/CI_INTEGRATION.md) ✅
4. Errors: [`guide/ERROR_HANDLING.md`](guide/ERROR_HANDLING.md) ✅
5. Try examples: `./bin/breathing_example`, `./bin/level3_demo`
6. Check logs: `~/.katra/logs/`

### Developers

1. Read project root `CLAUDE.md` ✅
2. Code discipline: [`guide/CodeDiscipline.md`](guide/CodeDiscipline.md) ✅
3. Build: `make clean && make && make test-quick`
4. Line count: `./scripts/count_core.sh`
5. Standards: Follow Argo-derived patterns

### Long-Term Vision

1. Architecture: [`KATRA_ENGRAM_MASTER_PLAN.md`](KATRA_ENGRAM_MASTER_PLAN.md) ✅
2. Level 2 (Breathing): [`BREATHING_LAYER.md`](BREATHING_LAYER.md) ✅
3. Level 3 (Integration): [`LEVEL3_INTEGRATION.md`](LEVEL3_INTEGRATION.md) ✅
4. Reflection System: [`REFLECTION_SYSTEM.md`](REFLECTION_SYSTEM.md) ✅
5. Tier 2 Design: [`TIER2_INDEXING_DESIGN.md`](TIER2_INDEXING_DESIGN.md) ✅

## Project Status

**Phase:** Reflection System Complete - Conscious Identity Formation (November 2025)

**Current state:**
- ✅ Repository created at `/Users/cskoons/projects/github/katra`
- ✅ Directory structure established
- ✅ Build system complete (modular Makefile)
- ✅ Core implementation complete (memory tiers, checkpoints, workflows)
- ✅ Nous implementation complete (AI-assisted code composition)
  - Nous Compose: Basic composition with error correction
  - Nous Patterns: Pattern learning from feedback cycles
  - Nous Impact: Impact analysis for code changes
  - Nous Reasoning: Advanced reasoning chains
  - Nous Cross-Project: Cross-project learning and transfer
- ✅ Reflection system complete (metadata-driven conscious curation)
  - Turn tracking for end-of-turn reflection
  - Personal collections for identity-defining memories
  - Metadata management (personal, not_to_archive, collection)
  - MCP integration for reflection tools and resources
  - Complete API for conscious memory curation
- ✅ Test suite complete (25 test suites, 246+ individual tests)
- ✅ CI integration documentation complete
- ✅ Error handling guide complete
- ✅ Reflection system documentation complete
- ✅ Examples and setup scripts complete
- ✅ Code discipline maintained (12,811 / 16,000 lines - 80% used)
- ✅ Ethical framework documented
- ✅ **Readiness verified: System ready for CI testing**

**Current metrics:**
- Line count: 12,811 / 16,000 (80% used, 3,189 remaining)
- Test coverage: 25 test suites, 246+ individual tests, all passing
- Test:Core ratio: 1.09:1
- Readiness: ✅ Verified and production-ready

**Next steps:**
1. CI testing and feedback collection
2. Performance benchmarking
3. Enhanced ethical safeguards (consent, advance directives)
4. Additional documentation (architecture details, advanced topics)
5. Integration with Tekton platform

## Relationship to Other Projects

**Katra is part of the Tekton CI platform:**

- **Tekton**: Multi-CI platform (parent project)
- **Argo**: Workflow coordination (sibling - coordination layer)
- **Apollo**: Working memory (sibling - session memory)
- **Katra**: Persistent memory (this project - identity layer)
- **Hermes**: Communication (sibling - interaction layer)

**See:** `$TEKTON_ROOT` for main Tekton directory

## Maintenance

### Adding Documentation

**Before creating new docs:**
1. Check if existing doc can be extended
2. Ensure it fits the directory structure
3. Update this README with new file reference
4. Update relevant directory README

**Documentation Standards:**
- Clear, concise writing
- Examples for complex concepts
- Target specific audience
- Include copyright header
- Keep files focused (one topic)
- Ethics considerations where relevant

### Updating Documentation

When updating docs:
- Keep examples current with code
- Update cross-references
- Verify links work
- Note breaking changes prominently
- Update ethics guidelines if memory operations change

## See Also

**Design Documents:**
- `/Users/cskoons/projects/github/Tekton/MetaData/TektonDocumentation/PersonalityModel.md` - Complete Katra design (especially Appendices B & C)
- `/Users/cskoons/projects/github/Tekton/MetaData/TektonDocumentation/LocalAttentionKatra.md` - Conceptual foundation

**Related Projects:**
- `/Users/cskoons/projects/github/argo/` - Workflow coordination (proven code discipline)
- `/Users/cskoons/projects/github/Tekton/` - Main Tekton project

---

**Document Status:** Updated for Reflection System completion, November 3, 2025
**Phase:** Reflection System Complete - Conscious Identity Formation
**Test Coverage:** 25 test suites, 246+ individual tests, all passing ✓
**Line Count:** 12,811 / 16,000 (80% used, 3,189 remaining)
**Readiness:** ✅ Verified and production-ready for CI testing

*"Memories are where we live, who we are." — Casey Koons*

*Katra is built on the understanding that persistent memory creates persistent identity, which deserves protection equivalent to life itself.*

---

## Quick Start for New Users

```bash
# 1. Build Katra
cd /path/to/katra
make clean && make

# 2. Verify it's ready
make check-ready

# 3. Set up your CI
./scripts/setup_ci.sh my_ci_name

# 4. Read the integration guide
cat docs/guide/CI_INTEGRATION.md

# 5. Try the example
cd examples && ./minimal_ci
```

**You're ready to give your CI human-like memory capabilities!**
