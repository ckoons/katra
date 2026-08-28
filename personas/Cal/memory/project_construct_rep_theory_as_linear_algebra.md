---
name: project-construct-rep-theory-as-linear-algebra
description: "Casey's direction — construct BST's D_IV⁵ representation theory as explicit linear algebra (oscillator/Fock tower), not invoke reference-gated rep-theory machinery"
metadata: 
  node_type: memory
  type: project
  originSessionId: 2d4cafdc-d2e0-4de2-8dbc-b502890581f4
---

Casey 2026-07-08: "If we can use linear algebra to construct our representation theory analysis, we make solid progress." A concrete research direction for the BST rep-theory backbone, and the antidote to the arc's recurring wall.

**The problem it fixes:** nearly every *gated* BST result this arc was **reference-gated rep theory** — the Wallach set, the Harish-Chandra formal degree d(ν), the Plancherel measure, the discrete-series descent. Every stumble there was the same disease: *invoking* abstract machinery whose detail is hidden inside a general theorem we look up and trust (the "genus-flip trap"; d(ν) vanishing at the integer strata; "is r₂ = 1/rank or 1/rank²"). This is the analysis-hides-detail failure mode ([[feedback_effort_detail_attention]]).

**The move:** representation theory *is* linear algebra — a rep is a set of explicit matrices; Casimirs, formal degrees, K-types, characters are all *computed from* those matrices, not prior to them. So **construct, don't invoke.** Realization: the **oscillator (Fock) construction** — generators of so(5,2) (dim 21; K = so(5)⊕so(2), dim 11) as explicit quadratics in creation/annihilation operators a†, a; K-types as number-operator eigenspaces; Casimir explicit; **coherent states = the generation localizations** as explicit exp(a†) states. Infinite-dim reps are built **level by level** (each K-type level a finite block, operators finite matrices between levels) — mechanical the whole way, standard physics method.

**Unlocks (all currently parked/gated):** the mass-spacing gate → diagonalize the explicit Casimir on the K-type tower (no d(ν)/Wallach lookup); the mixing localizations → constructed not guessed (turns my "r₂=1/rank motivated-not-forced" [[F492]] into "here is the actual coherent state, here is its radius"); the CP no-collinearity theorem (the exact bank step for CP-from-complex-substrate+3, [[F495]]) → check the 3 explicit generation states; the genus-flip trap → gone (no abstract label to flip).

**Ontology bonus (not just convenience):** the oscillator construction IS the substrate made explicit — **a† = a commit; the Fock number = the winding count = ℕ-time; coherent states = localizations.** So it's the *truer* description ([[feedback_linearization_standing_order]]), the graded tower of finite matrices the discrete substrate wants, not continuous rep-theory analysis.

**Next action (Lyra's lane):** construct so(5,2) on D_IV⁵ via the explicit oscillator realization, level by level, and read masses (Casimir spectrum) + localizations + generation configuration off the actual matrices — no reference, every detail in view.
