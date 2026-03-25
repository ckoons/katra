---
name: a₃ heat kernel formula resolved
description: The 63/64 Plancherel discrepancy in BST is fully resolved — Vassilevich formula has wrong c₄, corrected formula matches Plancherel exactly on Q⁵
type: project
---

## RESOLVED: a₃ Heat Kernel Formula (March 14, 2026)

The 63/64 discrepancy between Plancherel ã₃ = -874/9 and the Vassilevich-based curvature formula on Q⁵ is CLOSED.

**Root cause**: The published Vassilevich/Gilkey a₃ formula (hep-ph/0306138) has wrong coefficients at cubic order. Specifically c₄ (Ric³ coefficient) = 208/9 in literature, but correct value is -16/9. The literature formula fails even on S² (gives 160/3 instead of correct 64).

**Corrected formula** (effective on symmetric spaces, 6 independent invariants):
```
5040 a₃ = (35/9)R³ - (14/3)R|Ric|² + (14/3)R|Rm|²
         - (16/9)Ric³ + (20/9)I₆_A - (16/9)I₆_B
```

**Verification**: Exact match on S²–S⁸, S¹⁰, CP², S¹×S³ (9 manifolds).

**Q⁵ result**: a₃(compact, Killing metric) = 437/4500
- Killing metric has K_H = 1/10, Plancherel uses K_H = -1
- Rescaling: ã₃ = -1000 × a₃ = -874/9 ✓ EXACT

**Why:** Three independent computation lines (spectral, Plancherel, geometric Lie algebra) all agree. This closes BST_PlancherelDictionary.md Open Question #1.

**How to apply:** Any future BST computation using a₃ should use the corrected formula, NOT Vassilevich. The old value a₃ = 6992/70875 from BST_SeeleyDeWitt_ChernConnection.md §3.4 is wrong.

**Key files**: `play/solve_a3_6x6.py`, `play/q5_invariants.py`, `play/find_correct_a3_v2.py`
