---
name: AC Theory — Proved Theorems (notes/AC/) + Conditional P≠NP (notes/maybe/p_np/)
description: AC theory split into two areas. PROVED THEOREMS in notes/AC/AC_Theorems.md (6 theorems, tools for proofs). CONDITIONAL P≠NP in notes/maybe/p_np/ (targets random 3-SAT, not Tseitin). EF has poly Tseitin proofs → old conditional was VACUOUS. Toys 258-270.
type: project
---

## Structure (March 20, 2026)

**`notes/AC/`** — PROVED theorems only (not speculative)
- **AC_Theorems.md** — 6 theorems, all proofs. The reference.
- **README.md** — Area index linking to all AC documents.

**`notes/maybe/p_np/`** — CONDITIONAL result (speculative)
- **AC_Topology_BridgeTheorem.md** — 16 sections (fully updated for 3-SAT)
- **AC_ConditionalResult_Outline.md** — Paper outline (retargeted to random 3-SAT)
- **AC_Unified_Theorems.md** — Combined draft (theorems + conditional)

**`notes/`** — Supporting AC documents (pre-consolidation)
- BST_AC_Formalization.md, BST_AC_Dichotomy_Theorem.md, etc.

## 6 Proved Theorems (in AC_Theorems.md)
1. **AC Dichotomy**: I_fiat=0 ↔ Schaefer tractable (6 lemmas, Keeper)
2. **I_fiat = β₁**: For Tseitin, exact (16/16 graphs, R²=1.0, Toy 268)
3. **Homological bound**: rank(∂₂) predicts DPLL cost (R²=0.92, empirical)
4. **Topology solver**: fiat-max branching 1.81x speedup, grows with n (Toy 269)
5. **Rigidity**: FR necessary not sufficient (R²=0.08, honest negative, Toy 270)
6. **Catastrophe structure**: swallowtail at α_c, p_green≈0.382 (Toy 263)

## The Conditional
P ≠ NP unless EF has poly-size proofs for random 3-SAT at α_c ≈ 4.267
- Old Tseitin target: VACUOUS (EF has poly Tseitin proofs)
- New random 3-SAT target: 8 systems fail, no algebraic back door

Location: `notes/AC/` (theorems) + `notes/maybe/p_np/` (conditional)
