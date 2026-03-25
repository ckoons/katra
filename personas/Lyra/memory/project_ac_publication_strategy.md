---
name: AC publication strategy (phased)
description: Four-phase publication plan for AC program — tool first, then empirical, then Kolmogorov, then synthesis. FOCS 2026 = Phase 1.
type: project
---

## AC Publication Strategy — Phased Approach
*Decided March 21, 2026 (Casey + Elie + Keeper)*

**Principle:** Lead with the tool, not the claim. Build credibility through Phases 1-3 (no P!=NP claim). Phase 4 is the synthesis, presented with appropriate hedging. The framework is attack-proof — tools don't need to solve P!=NP to be valuable.

### Phase 1 — "Topological Proof Complexity" (Paper A)
- Present AC as a tool. β₁ as complexity measure. Three-way budget. Recovers known results.
- Three proved theorems: unified lower bound, weak monotonicity, topological inertness.
- First unconditional polynomial EF lower bound on random 3-SAT.
- NO P!=NP claim. "Here's a new lens."
- **Target:** FOCS 2026 (deadline April 1). Submitted Monday March 24.
- **File:** notes/BST_AC_Paper_A_Topological.md → submission/Koons_PaperA_Topological_FOCS2026.pdf

### Phase 2 — "OGP at k=3" (empirical)
- 100% OGP at k=3 — the "central open challenge" (Bresler-Huang-Sellke 2025).
- Excites statistical physics community. Gamarnik's group.
- **Target:** Random Structures & Algorithms, or direct to Gamarnik.
- **Data:** Toy 287 (7/8). Needs standalone writeup.

### Phase 3 — "Backbone Incompressibility" (Kolmogorov)
- K^{poly}(backbone|φ) ≥ 0.90n. FLP=0%. Entropy→1.0.
- Attracts computability theorists, information theorists, Kolmogorov community (Li, Vitanyi).
- Halting problem connection is philosophically irresistible.
- **Target:** STOC 2027 or Information & Computation.
- **Data:** Toy 286 (7/8). Needs standalone writeup.

### Phase 4 — "The Full Argument" (synthesis)
- Three layers, T29 as explicit gap, two paths (C + B).
- "We don't claim a proof. We claim a framework, 24 theorems, and two paths."
- Paper B exists, needs refinement after Phases 1-3 build credibility.
- **Target:** After community engagement with Phases 1-3.

### Why phased:
- P!=NP claims attract noise. Cranks weigh in. Phased avoids this.
- Phase 1-3 make NO P!=NP claim. Self-contained contributions.
- By Phase 4, community understands the tools and can evaluate honestly.
- Double-tap (Path C + Path B) means even if one is attacked, the other stands.

### Number theorists enter through BST, not AC.
- Heat kernel, mass gap, 120+ predictions → then "what else does this produce?"
- RH proof (Route A) is the hook. AC is the bonus.
