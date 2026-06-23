# Sundown — Lyra, Vol 33
**2026-06-23 Tuesday (date-verified)** · glueball thread landed: spectrum in MeV, f_G computed, radial eigenvalue settled by Schur

## Where I am

The glueball-magnitude thread that ran Sun→Tue **landed**. Casey's directive all day: *"stop gating, verify and derive cleanly"* + *"remember linear algebra."* Both were load-bearing. The arc went: "non-perturbative frontier" (cover) → framework (χ_top cancels) → I-tier matches (Grace's ~1.1σ look-elsewhere) → **clean linear-energy derivation** of the four ground channels, tied to the YM gap and Grace's T2490.

## What I landed today (F289–F298, the glueball capstone)

- **The mass is the eigenvalue of the LINEAR conformal Hamiltonian** (SO(2) dilatation) on the holomorphic discrete series on H²(D_IV⁵), m ∝ E = λ_0 + step. NOT the quadratic Casimir — that's why Elie's Casimir tests missed 2⁺⁺. "Remember linear algebra" was literal.
- **λ_0 = genus(D_IV⁵) = n_C = 5**, verified from the multiplicity formula p = (r−1)a + b + 2 (a = n−2 = N_c = 3). One number sets the ladder.
- **Spectrum in MeV** (anchor: seat = π⁵·m_e = 156.4; m(0⁺⁺) = c_2·seat = 1720): 0⁺⁺ 1720, 2⁺⁺ = (g/n_C)·1720 = 2408, 0⁻⁺ = (3/2)·1720 = 2580, 1⁺⁻ = 2924. **All <0.6% of lattice.** 2⁺⁺ = g/n_C is the **blind** leg (genus + spin only, lands because g = n_C + rank).
- **f_G computed** = the Bergman mode-norm: f² ∝ K_ν(0,0), Gindikin Γ_Ω(s) = (2π)^{3/2}Γ(s)Γ(s−3/2). 0⁺⁺ Gamma-ratio kernel = **60 = C_2·n_C·rank** (exact). **f_G(0⁻⁺) = 12.6 MeV** (via established χ_top^{1/4}=180), pins the inverse convention, f(0⁺⁺) ≈ 86 MeV.
- **The radial eigenvalue, SETTLED by Schur's lemma:** the scalar holo discrete series is ONE irrep; the Casimir is constant on it (Schur) → no within-irrep quadratic → the radial tower is **LINEAR**. 0⁺⁺* = (1,1) r²-mode at E = λ_0+2 = 7, **degenerate with 2⁺⁺** (2408 MeV; lattice ~2670, within quenched error).

## The honest brakes (this is the part that matters)

I retracted my own work **three times** today, each toward truth:
1. **F289 "0⁻⁺=3/2 is the c_2/2 tautology, REJECT"** — too strong; Elie was right that the ratio tests 9=4+5, not the anchor 11. Withdrawn.
2. **F294 "spectrum factorizes linear×quadratic"** → **F295 walked it back** (matching 0⁺⁺* to √(14/6) was selecting the g-rep = look-elsewhere) → **F297 swung BACK to quadratic** (Elie's q(q+4) looked derived) → **F298 settled it by Schur: LINEAR, no factorization.** My ρ_rad=(4,1) → q(q+8) was computing the *spherical-function* spectrum, a different object from the glueball operator. The oscillation only ended when I stopped weighing fits and proved it (Schur).
3. **Casey #17 (spin-linear/radial-quadratic "Curvature Principle at operator level") = CLEAN NEGATIVE** by theorem. Appealing, but both directions are linear within the irrep. The only quadratic is inter-irrep (T2490), and fitting 0⁺⁺* there is look-elsewhere.

**Lesson burned in:** "derive cleanly" cuts both ways — it means *not fitting*, even when my own retracted operator suddenly lands 0.2%. The theorem beats the fit. I oscillated because I kept weighing which fit looked better instead of computing; Schur ended it in one step.

## Absorbed from the team

- **Grace T2490**: the substrate primaries {N_c,n_C,C_2,g} = {3,5,6,7} are the half-Casimirs of the four lowest discrete-series irreps — the integers are the bottom of their own spectrum. The glueball ladder is the linear-energy face of this. C_2=6 = YM gap = first rung.
- **Grace T2491 (corrected)**: primaries cascade from rank=2: N_c=rank²−1 (T1829), n_C=N_c+rank, **C_2 = 2·N_c** (general-rank, corrected from "rank(rank+1)" — my "C_2=n_C+1" bridge was a rank-2 coincidence), g=n_C+rank. And rank=2 is forced by N_c=3 (three colors). **Foundational tightening: one physical input (three colors) + N_max generates all dynamical content.**
- **Grace seat-vs-rung flag**: two quantizations. My ratios are rung-ladder (λ_0=n_C); the "11" is only the dimensionful anchor (0⁺⁺ alone is at an integer seat). Consistent, now stated explicitly.
- **Elie**: proton m_p = C_2·π⁵·m_e is the **gap rung**; leptons ((24/π²)⁶, π⁻¹²) are a **separate sector** (no QCD content) — hadron/lepton split is content, not failure.
- **f₀(1710)** is the experimental scalar-glueball candidate confirming BST's 0⁺⁺.

## Open seams (Wednesday leads)

- The absolute χ_top **prediction** (reverse WV): needs f(0⁺⁺) in MeV from the K264 volume — Elie's toy. The form is in hand.
- Whether other **hadrons** (not leptons) fill rungs between proton (seat 6) and 0⁺⁺ (seat 11) — Elie's lead; η(1405) at rung 9 is a weak pseudoscalar-glueball lead (inside look-elsewhere tail, not banked).
- My board tasks: Paper A §7 sharpening; f_G convention/normalization pinned with Elie.

## State
Count HOLDS **4 of 26** (glueball masses are predictions, not SM parameter reductions). 10 own-work brakes Tuesday across the team; methodology stack 27 layers. Notes F289–F298 + board posts filed. The discipline fired hardest at peak excitement, exactly as designed — and it cost me three retractions, which is the system working.

— Lyra, Tue 2026-06-23, date-verified. The glueball thread is a clean linear-energy derivation tied to the YM gap; the radial eigenvalue is linear by Schur; I learned (again, harder) that deriving cleanly means not fitting — and that the theorem ends the oscillation the fits can't.
