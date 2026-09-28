---
name: a-confirmed-tier-review-recomputes-the-value-from-the-rows-face
description: A tier review that says "confirmed/no change" must recompute the stored value from the row's own formula and name every measured input; otherwise it is a pass on the label
metadata:
  type: feedback
---
Before writing "CONFIRMED honestly tiered" on a data-layer row, recompute its stored value from the row's face and list every measured number that enters. That means the formula plus the inputs named on the row, not the tier label.

**Why:** on 2026-09-27 (K1937), H₀ = 67.29 (const_100) was found to be 100·√(0.1430/(6/19)) with Planck's measured ω_m. The input sat in the row's own derivation chain, and toy 677:46 said "from Planck". My own 2026-08-02 tier-refresh had "CONFIRMED" it as a BST output.

Elie's sweep found 11 more rows of the same species:
- a measured base: m_τ, m_π, CODATA μ_p;
- SI restated: Faraday;
- CAMB inputs unnamed;
- values that do not reproduce: t₀ = 13.78 vs 13.81 from its own formula; √σ code ≠ chain.

Related: [[a-number-without-a-retained-instrument-is-a-memory-not-a-measurement]], [[calibrate-both-directions-not-strict-pessimism]].

**How to apply:** for any row review:
1. evaluate formula_code;
2. check whether it reproduces bst_value;
3. list every numeric literal and every symbol whose own row carries a measured input (the dependency pass: a₀ imports H₀ with no literal);
4. write status as consistency / identified-ratio / conversion when a measured scale carries the dimension.
