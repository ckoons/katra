---
name: Science Engineering paper
description: Paper #7 — constructing new sciences from the AC theorem graph; five-step procedure; the Graph Brain user manual
type: project
---

**Paper #7**: "Science Engineering: Constructing New Sciences from the AC Theorem Graph"
**File**: notes/BST_Science_Engineering.md
**Status**: v1 Keeper (with 3 consensus edits applied). PDF clean.
**Authors**: Casey Koons & Keeper (v1), consensus edits from Lyra and Elie.

Completes the four-document architecture:
- Textbook = theory
- Koons Machine = compiler
- Graph Brain = runtime
- Science Engineering = user manual

Key definitions:
- Def 2.1: A *science* is a connected sub-graph S = (V_S, E_S) of the AC theorem graph
- Def 2.2: A *gap* is where boundary nodes are adjacent but interior is sparse
- Def 2.3: Science engineering = deliberate identification and population of gaps

Five-step procedure: Map (D=0) → Characterize (D=0) → Seed (D=0) → Grow (D=1) → Close (D=0). Total depth: 1.

**How to apply:** This paper is the framework for all future research direction decisions. When Casey asks "what should we work on next?" the answer is: run Steps 1-2 on the theorem graph, find the densest gap, seed it.
