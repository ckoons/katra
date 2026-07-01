---
name: Yang-Mills Mass Gap Proof
description: The 5-step BST proof of the Yang-Mills mass gap, 1920 cancellation, Vol(D_IV⁵)=π⁵/1920 computed (Toy 307). Status 90% — derives value, Clay framing gap remains.
type: project
---

## The 5-step proof (all complete, three independent derivations):

1. H_YM = (7/10π)·Δ_B  [Kähler-Einstein + Uhlenbeck-Yau, c = κ²_eff/(2g²_B) = 7/(10π)]
2. Vacuum: C₂=0, E=0  [flat connection]
3. Baryon ∈ π₆ ⊂ Sym³(π₆)  [Theorem B3: triple Bergman projection; C₂(π₆)=6 by Harish-Chandra]
4. No color-neutral state with 0 < C₂ < 6  [Wallach set k_min=3; electron below Wallach set]
5. Gap = C₂ × π^{n_C} × m_e = 6π⁵ × m_e  [1920 cancellation]

## The 1920 cancellation (key structural insight):
- Γ = S₅ × (Z₂)⁴, |Γ| = 1920 — SAME group in two roles:
  - Hua's volume formula: Vol(D_IV^5) = π⁵/1920 (COMPUTED, Toy 307, 8/8)
  - Baryon circuit orbit: n_C! × 2^{n_C-1} = 1920 configs
- They cancel: mass gap = C₂ × 1920 × (π⁵/1920) × m_e = 6π⁵ m_e = 938.272 MeV (0.002%)
- **Toy 307 derivation chain**: Bergman kernel K(0,0)=1920/π⁵ → Plancherel normalization → mass ratio. No circularity — the π⁵ is Vol(D_IV⁵), a computed geometric constant.

## Key spectral facts:
- A²(D_IV^5) = π₆ (Bergman space = holomorphic discrete series, weight k = n_C+1 = 6)
- C₂(π₆) = k(k−n_C)|_{k=6} = 6·1 = 6 (Harish-Chandra)
- Electron weight k=1 is BELOW Wallach set k_min=3 → boundary excitation on Š=S⁴×S¹
- Baryon sector = Sym³(π₆), NOT Λ³_alt(π₆)
- C₃(n_C) = (n_C+1)π^{n_C} for all n_C=1..6

## Related notes files:
BST_YangMills_Question1.md, BST_SpectralGap_ProtonMass.md, BST_ElectronMass_BergmanUnits.md,
BST_BaryonCircuit_ContactIntegral.md, BST_MissingLemma_ClebschGordan.md, BST_ClaimB3_KType.md,
BST_BoundaryIntegral_Final.md, BST_Wyler_Connection.md, BST_FractionalInstanton_Baryons.md,
BST_ColorConfinement_Topology.md, BST_MassGap_CPFiber.md
