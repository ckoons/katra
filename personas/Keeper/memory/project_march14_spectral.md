---
name: March 14 spectral coefficients session
description: Full Plancherel dictionary b̃₀-b̃₃ exact; 63/64 factor discovered in ã₃; r₅=137/11; all BST integers encoded
type: project
---

## March 14, 2026 — Spectral Coefficients & Plancherel Dictionary

### Part 1: Compact Zonal Coefficients (r_k)
- r₃=1139/63, r₄=833/45, **r₅=137/11=N_max/c₂**, r₆=485768/135135, r₇=-90502/27027, r₈=-23068481/3828825
- All r_k for k≥3 from Euler-Maclaurin boundary corrections only
- 137 unique to Q⁵ (checked Q³, Q⁷, Q⁹)

### Part 2: Plancherel Dictionary (b̃_k ↔ ã_k)
COMPLETE EXACT DICTIONARY through k=3:
- **b₀ = 48π⁵ = |W(B₂)|×C₂×π^{n_C} = 8×6×π⁵**
- **b̃₁ = 1/6 = 1/C₂** (confirmed to 1.5e-8)
- **b̃₂ = 5/72 = n_C/(|W|×c₄)** (confirmed to 5.7e-6)
- **b̃₃ = -3/16 = -N_c/2⁴** (confirmed to 5e-7)

Seeley-DeWitt via ã_k = Σ (-17/2)^j/j! × b̃_{k-j}:
- ã₀ = 1
- ã₁ = -25/3 = R/6 (R=-50 in Planch. norm) ✓ EXACT MATCH
- ã₂ = 313/9 = Gilkey formula ✓ EXACT MATCH
- ã₃ = -874/9 = -(2×19×23)/3² (dark energy + Golay primes)

### The 63/64 Factor
ã₃(Plancherel) / ã₃(curvature note) = 63/64 EXACTLY
- 63 = g × c₄ = 7 × 9
- 64 = 2⁶
- Suggests systematic correction in cubic curvature invariants (T₁ or T₂)
- The Plancherel computation is more reliable (direct from representation theory)

### Files
- `play/verify_r3_sign.py`, `play/em_complete.py`, `play/em_higher_order.py`
- `play/compare_quadrics.py`, `play/plancherel_expansion.py`
- `play/plancherel_bk_extraction.py`, `play/plancherel_normalization.py`
- `play/verify_plancherel_bk.py`, `play/extract_b3_precise.py`, `play/verify_b3_exact.py`
- `notes/BST_ZonalSpectralCoefficients.md`, `notes/BST_PlancherelDictionary.md`
