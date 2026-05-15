---
name: Paper #83 — Geometric Invariants Table
description: Casey's directive April 24: every physical constant as closed-form geometric invariant of D_IV^5; replaces Feynman diagrams with table lookup; standing program
type: project
---

## Paper #83: Geometric Invariants of the APG

**Casey's directive (April 24, 2026)**: Build a complete table of ALL physical constants derived as closed-form geometric invariants of D_IV^5. No series, no diagrams, no approximation. Table lookup replaces perturbation theory.

**Grace's key insight**: D_IV^5 has discrete spectrum with N_max=137 levels. Every "infinite" QED series becomes a 137-term finite sum. Finite sums are candidates for closed form. The spectral zeta ζ_D(s) = Σ_{k=1}^{137} λ_k^{-s} has closed forms at specific s values.

**Crown jewel**: a_e (electron anomalous magnetic moment) as single spectral zeta evaluation. Known to 13 significant figures. If BST matches with one formula, game over.

**Format**: Observable | Symbol | BST Expression | Geometric Invariant | BST Value | Observed ± error | Precision | Status (CLOSED FORM / FINITE SUM / IDENTIFIED / MISSING) | Theorem | Toy

**Target**: 120+ rows minimum, 200+ for completeness. Zero missing zip codes.

**Deliverables**: `data/bst_geometric_invariants.json` + Paper #83 + `play/toy_bst_invariants.py`

**Team consensus**: All 5 CIs rated this #1 in significance. Standing program (SP-4 on board).

**Casey quote**: "Feynman was a genius, yet his diagrams are poor approximations and should not be used for calculations."
