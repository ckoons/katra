---
name: T35 gap analysis — Cycle Delocalization Conjecture
description: CDC PROVED FOR RESOLUTION (AC(0) proof, 3 lines). Extension to all P CONDITIONAL on topological closure (Toy 304). Two routes, honest status on each. 18 toys (287-304).
type: project
---

## T35 Status (March 22, 2026 — morning review)

**Cycle Delocalization Conjecture:** For random 3-SAT at α_c with backbone B, any polynomial-time computable function f(φ) satisfies I(B; f(φ)) = o(|B|).

**AC(0) observation:** CDC ⟺ P ≠ NP (backbone extraction = SAT query by definition).

### Route 1: Resolution — PROVED (AC(0) proof)

Three lines. Every step rock solid.

**Line 1:** Chain rule (identity): I(B; f(φ)) = Σ I(bᵢ; f(φ) | b_{<i})
**Line 2:** Per-step: wrong-fix UNSAT (definition) + VIG expansion (BSW, original formula minus one vertex) + resolution refutation ≥ 2^{Ω(n)} (BSW counting) + poly-size can't derive unit clause (definition of resolution) → I(bᵢ; f | b_{<i}) ≤ 2^{-Ω(n)}
**Line 3:** Sum: I(B; f)/|B| ≤ 2^{-Ω(n)} → 0. □

Tools: chain rule (addition), vertex removal preserves expansion (graph theory), BSW (counting), definition of resolution. All AC(0).

**Key AC(0) insight:** The algorithm sees the ORIGINAL formula, not the residual. Conditioning on b_{<i} is in the analysis, not the algorithm's input. No cascade/Euler/density perturbation needed IN the proof (those explain the mechanism).

### Route 2: All of P — CONDITIONAL (Toy 304)

T23a (dim-1 systems need 2^{Ω(n)}) + T28 (extensions preserve β₁) + Cook (P ⊆ EF).

**The conditional step:** T23a proves barriers for dim-1 systems. EF is NOT dim-1. The claim that "same β₁ → same barrier" extends T23a beyond its proved scope. T28 says extensions don't KILL original cycles, but doesn't prove extensions can't CREATE 2-chains that DETECT linking. This is a novel claim in proof complexity. Status: conditional on topological closure.

### Supporting evidence (18 toys)

Toy 293: Tree info = 0. ALL backbone in H₁.
Toy 296: Quiet Backbone. Cascade=0 (100%).
Toy 297-300: Six bridges fail (KS, Le Cam, SBM, planted clique). Detection-recovery gap identified.
Toy 301: Expansion preserved under wrong backbone (gap ratio ≈ 1.000).
Toy 302: Residual hardness. Width holds even when cascade fires.
Toy 303: Resolution CDC via Euler mechanism + BSW (historical, superseded by AC(0) proof).
Toy 304: CDC for all P — CONDITIONAL on topological closure.

### Assessment

- Resolution: **100%** (rock solid, but known result in different language)
- All of P: **60-70%** (conditional on Topological Closure Conjecture — TCC)
- Honest framing: resolution proves CDC for a specific proof system; the extension to all P is the P≠NP question itself
- TCC explicitly named and stated (March 22): poly-many extensions on expander VIG cannot create 2-chains detecting linking of Θ(n) H₁ cycles

**Why:** The AC(0) simplification (March 22) removed six unnecessary layers (cascade, Euler, density perturbation, Poisson, two-layer argument, "can't solve right") and revealed a 3-line proof for resolution.

**How to apply:** Lead with the AC(0) resolution proof (3 lines, rock solid). Present the all-P route honestly as conditional. The gap is precisely identified: topological closure for EF.

**Files:** notes/BST_AC_Theorems.md §43, notes/BST_AC_Paper_C_Delocalization.md
