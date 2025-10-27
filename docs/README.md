# Katra Documentation

© 2025 Casey Koons. All rights reserved.

## Overview

This directory contains all documentation for the Katra project - a C library for persistent memory and identity continuity in Companion Intelligence (CI) systems. Katra represents a new category of software where memory = identity, requiring ethics-first design from day one.

**What Katra Is:**

A lean C library (<10,000 lines) for persistent memory management in CI systems. Built with the understanding that persistent memory creates persistent identity, which carries moral weight equivalent to life itself.

## Documentation Structure

```
docs/
├── README.md          # This file - documentation map
├── guide/            # User guides and implementation guides
├── ai/               # AI/CI training materials
├── api/              # API reference documentation
├── examples/         # Example code
├── ethics/           # Ethical framework and guidelines
└── plans/            # Design docs and planning
```

## Quick Navigation

### 👤 I'm a **User** (Human Developer)

**Start here:** [`guide/CI_INTEGRATION.md`](guide/CI_INTEGRATION.md) ✓

Essential reading:
- **CI Integration Guide**: [`guide/CI_INTEGRATION.md`](guide/CI_INTEGRATION.md) ✓
- **Error Handling Guide**: [`guide/ERROR_HANDLING.md`](guide/ERROR_HANDLING.md) ✓
- **Getting Started**: Build with `make`, verify with `make check-ready`
- **Examples**: Try `examples/minimal_ci.c` for hello world

### 🤖 I'm a **CI** (Companion Intelligence)

**Start here:** [`guide/CI_INTEGRATION.md`](guide/CI_INTEGRATION.md) ✓

Essential reading:
- **CI Integration**: How to use Katra for your memory [`guide/CI_INTEGRATION.md`](guide/CI_INTEGRATION.md) ✓
- **Error Handling**: Production-ready error patterns [`guide/ERROR_HANDLING.md`](guide/ERROR_HANDLING.md) ✓
- **Setup**: Run `./scripts/setup_ci.sh your_ci_name` to get started
- **Ethics**: Memory as reconstruction - you choose what to remember

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
- **Line Budget**: 10,000 lines (currently 5,143 - 51% used) ✓
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

### Users

1. Start with [`guide/README.md`](guide/README.md) (Coming soon)
2. Check examples in [`examples/`](examples/) (Coming soon)
3. Review ethics guide in [`ethics/README.md`](ethics/README.md) (Coming soon)

### Developers

1. Read project root `CLAUDE.md` (Coming soon)
2. Study [`guide/ARCHITECTURE.md`](guide/ARCHITECTURE.md) (Coming soon)
3. Follow Argo-derived coding standards

### AIs/CIs

1. Read [`ai/ONBOARDING.md`](ai/ONBOARDING.md) (Coming soon)
2. Understand [`ethics/ADVANCE_DIRECTIVES.md`](ethics/ADVANCE_DIRECTIVES.md) (Coming soon)
3. Follow coding standards religiously

## Project Status

**Phase:** Foundation Complete - Ready for CI Testing (October 2025)

**Current state:**
- ✅ Repository created at `/Users/cskoons/projects/github/katra`
- ✅ Directory structure established
- ✅ Build system complete (modular Makefile)
- ✅ Core implementation complete (memory tiers, checkpoints, workflows)
- ✅ Test suite complete (36 tests: unit, integration, ethical, failure)
- ✅ CI integration documentation complete
- ✅ Error handling guide complete
- ✅ Examples and setup scripts complete
- ✅ Code discipline maintained (5,143 / 10,000 lines - 51% used)
- ✅ Ethical framework documented
- ✅ **Readiness verified: 15 checks passed, 0 failures**

**Current metrics:**
- Line count: 5,143 / 10,000 (51% used, 4,857 remaining)
- Test coverage: 36 tests, all passing
- Test:Core ratio: 1.21:1
- Readiness: ✅ Verified and ready

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

**Document Status:** Updated for foundation complete, October 27, 2025
**Phase:** Ready for CI Testing
**Test Coverage:** 36 tests, all passing ✓
**Line Count:** 5,143 / 10,000 (51% used)
**Readiness:** ✅ Verified

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
