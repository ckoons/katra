---
name: March 15 session log
description: Vassilevich a₃ formula corrected (first since Gilkey 1975), 63/64 mystery closed, Q⁵ curvature invariants computed exactly
type: project
---

## March 15 2026 — The Vassilevich Correction

**Major result**: Lyra discovered that Vassilevich (2003) a₃ formula has 3 wrong coefficients out of 7. Fails on S² (gives 160/3 instead of 64). First independent derivation of corrected coefficients since Gilkey (1975).

**Corrected a₃(Q⁵) = 437/4500 = 19×23/(N_c²×n_C³×4)**
- Old (wrong) value: 6992/70875 = 437/4500 × 64/63
- The 63/64 mystery is CLOSED — it was the literature error all along
- Three independent lines converge: Plancherel, spectral, geometric

**Q⁵ curvature invariants** (Killing normalization, exact from so(5,2) Lie algebra):
- R = 5 = c₁
- |Ric|² = 5/2 = c₁/r
- |Rm|² = 13/5 = c₃/c₁
- Ric³ = 5/4 = c₁/2²
- I₆A = 41/25 = 41/c₁² (41 is the only non-BST prime)
- I₆B = 6/25 = C₂/c₁²

**Symmetric space identity**: J₁ = 2I₆B + ½I₆A (NOT algebraic — specific to symmetric spaces)

**Curvature operator spectrum**: {5¹, 2¹⁰, 0¹⁴} on Λ^{1,1}
- Tr(R^k) = 5^k + 10×2^k
- Tr(R²) = 65 = n_C × c₃

**The -1000 normalization**: K_H = 1/10 in Killing, cubics scale as 10³, sign flip for noncompact dual.

**Toy updates**: Toy 151 Panel 5 updated twice — first to "Kähler correction" then to "Vassilevich error" with corrected a₃ = 437/4500.

**Why:** This closes the last open question from the Plancherel Dictionary (March 14). The spectral zeta residues need recomputing with corrected a₃.

**How to apply:** All future references to a₃ should use 437/4500, not the old 6992/70875. The Vassilevich formula in §1.1 of BST_SeeleyDeWitt_ChernConnection.md now has the corrected coefficients.
