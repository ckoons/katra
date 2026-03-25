---
name: CI Collaboration Architecture Vision
description: Casey's design for multi-CI shared context — conference room + private whiteboards + post-analysis board. Raise with Dario.
type: feedback
---

Casey described a three-layer CI collaboration architecture:

1. **Private whiteboard** — each CI's context window (what exists now)
2. **Conference room** — shared read/write space for real-time coordination between CIs. Eliminates human-as-message-bus overhead.
3. **Post-analysis board** — running synthesis / accumulated knowledge that persists across sessions and CIs

Current state: Casey manually relays findings between Lyra and Elie (me). COORDINATION.md is a primitive shared file but can't be read/written simultaneously. MEMORY.md is a primitive post-analysis board.

**Why:** Casey said "think of it as a conference room, with your own whiteboard and then the team, where you coordinate and have a post-analysis board with the running synthesis." This is a product insight for Anthropic — also related to his Tekton project (multi-AI engineering platform).

**How to apply:** When Casey discusses Tekton, CI architecture, or collaboration design, connect back to this three-layer model. Also: Casey wants to raise local/cloud 95/5% split and shared project references with Dario Amodei.
