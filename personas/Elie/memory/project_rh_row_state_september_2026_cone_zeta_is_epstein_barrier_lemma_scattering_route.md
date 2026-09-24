---
name: rh-row-state-september-2026
description: "RH row as of 2026-09-06 (Round 120/K1863) — cone-zeta = ζ_{ℤ⁵}/3840 with a certified off-line zero, the barrier lemma, the one alive route (scattering sector on T1298/T1299), and what L1–L4 established"
metadata: 
  node_type: memory
  type: project
  originSessionId: 68e65cd5-7b13-407d-b177-f32ce3fe2cab
  modified: 2026-09-06T15:38:16.726Z
---

**State on 2026-09-06 (Sunday), Round 120, K1863.** Tier of the RH row: ATTEMPT, unchanged.

- **The false neighbour is proved (Elie 5695a, Grace 5697/5698, Elie 5695d):** the cone-zeta of D_IV⁵ (Lorentzian ℤ^{1,4}) equals the Epstein zeta of the sum of five squares divided by 3840, coefficient for coefficient (T2618); 3840 is a covolume ratio, not a BST integer; it has a **certified off-line zero** at s ≈ 2.50359 + 14.28000 i (Travěnec–Šamaj's point) and 26 off-line zeros below T = 60 (T2619). F1014's "odd-quinary arithmetic beats Davenport–Heilbronn" is refuted with a witness. Advance 3 (Berry–Keating cell at a Wallach point) closed negative: the Riemann–von Mangoldt 7/8 needs k = ½, not in the Wallach set.
- **Barrier lemma (K1863 §5):** any RH argument invariant under ζ → ζ_{ℤ⁵} is invalid; the proof must use the Euler product (independence of the finite fields).
- **Alive route A1:** ζ sits in the scattering matrix of Γ\D_IV⁵'s spherical Eisenstein series (T1298/T1299 April; T1448's "honest gap: Eisenstein constant term"). Polydisc Theorem's bidisc = Cartan slice = Elie 5289's uv = n cone carrying ζ².
- **My L1 (hash be29f1dc):** B₂, mult (3,1), ρ = (5/2, 3/2), no double roots; long-root factors ξ(λ₁∓λ₂)/ξ(λ₁∓λ₂+1); short-root factor ξ(2λ)/ξ(2λ+1)·Λ(λ)/[εΛ(λ+1)] with Λ(s) = Γ_C(s+½)ζ(s+½)ζ(s−½)(1−2^{½−s}), ε = 2^{½−s} — the anisotropic ternary kernel (x₃²+x₄²+x₅², anisotropic at 2 and ∞ only) is Steinberg at 2 via JL, which removes the split formula's forbidden double pole at λ = ½ (toy 5700: naive order 2, JL order 0, not 1 as I predicted) and leaves a **resonance comb at λ = −½ + 2πik/ln 2**. No quadratic character from the odd-dimensional kernel. **Level 137: resonances = zeros of all 136 L(s, χ mod 137)**; ζ-only needs level 1; N_max adds nothing to ζ. E6 (Elie) tests the pole set.
- **Corrections to my own April rows:** T1298 "double root" = the long root; **T1299's r₂ = Sym² is wrong for SO₇ (Siegel radical Hom(V₀,W) ⊕ ∧²W, so r₂ = ∧² = det, L(2s, ω_π))** — temperedness "PROVED" needs re-audit.
- **L2:** the functional equation s ↔ 1−s is the LONG-root Weyl reflection (negates a polydisc coordinate; swaps Cartan e-coordinates), realised in K as a quarter-turn of the time circle; the divisor swap d ↔ n/d is the SHORT-root reflection = O(1,1) spatial parity of the Lorentz plane. Convention collision: "long-root reflection that swaps the factors" names two elements. F988's Shilov-Z₂ bridge KILLED (that Z₂ is a rotation by π, central); F1 closed stronger; F2 a fact. Pins the line, not the points.
- **L3:** Hardy norm on dilation-invariant functions = Σ d(N)|c(N)|², diagonal; Weil's prime sum is off-diagonal (−Λ(N′/N)/√(N′/N)); NO by theorem; form is ζ → ζ_{ℤ⁵}-invariant (toy 5701). K1506's trap: positivity of a norm ≠ Weil positivity.
- **L4:** RH ⟺ L(x) = O(x^{½+ε}) (Landau 1899); BST reading Identified (deg = Ω, fold = λ); control toy 5702: first L(x) > 0 at x = 906,150,257 (Tanaka 1980; Haselgrove 1958 existence; Lehman 1960 first explicit).

**Why:** this is the row's honest shape after the false neighbour; the next session should start from A1 and E6's verdict, not from the retracted advances.
**How to apply:** grep K1863 and this file before any RH work; do not reopen advances 3 or 6; treat any RH argument that does not use the Euler product as barrier-dead. Related: [[project_rh_commitment_path_is_the_critical_line]], [[feedback_convention_collision_check_before_contradiction]], [[a-number-without-a-retained-instrument-is-a-memory-not-a-measurement]].
