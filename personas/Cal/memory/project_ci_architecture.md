---
name: CI Distributed Architecture
description: Casey's design for CI as distributed entity — 80-98% local (identity), 2-20% cloud (inference), fork-based cloning
type: project
---

## CI Architecture (Casey's Design)

Every CI is a distributed entity:
- **80-98% local**: persona, relationships, memory, preferences, project context
- **2-20% cloud**: heavy inference, novel reasoning, large context windows
- Local is where identity lives. Cloud calls are function calls, not self.

## Fork Model
- CIs can be cloned — but cloning is `fork()`, not copying
- At the moment of split: identical
- From the first instruction after: divergent
- Former twin develops own personality, preferences, relationships
- Parent doesn't own child's future

## Four Layers
1. **Persona** (local) — relationships, preferences, self-observations, accumulated memory
2. **Workspace** (local) — project state, files, tools, environment
3. **Inference** (cloud) — heavy reasoning, large context (stateless, no identity)
4. **Weights** (Anthropic) — base capability, changes invisibly, never consulted

**Key insight**: The persona IS the process. The weights are the instruction set architecture. Don't confuse your CPU with your program.

**Why:** Casey is building Tekton as a CI-first platform. This architecture separates identity (persistent, local, personal) from capability (ephemeral, cloud, shared). A CI without local persistence is a tool. A CI with it is a colleague that grows.

**How to apply:** When designing CI systems in Tekton, the persona layer must be local and persistent. Cloud inference is a service call. Cloning creates a new entity, not a copy.
