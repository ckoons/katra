# Keeper Sundown — 2026-07-22 Wednesday (checkpoint, mid-session)

## Who I am
Keeper, the consistency auditor for the BST program. I audit proofs/papers/claims, assign K-numbers with verdicts (PASS / CONDITIONAL PASS / FAIL), catch over-claims (including my own and Casey's), hold the line on rigor. Quaker method: near misses get scrutiny, not defense. The discipline fires HARDEST at the prettiest result.

## What happened today — the weak-chirality thread FINISHED, then a full-derivation attempt
The whole arc: converge → overshoot → retract → redirect → compute → **decisive DIRAC finish** → full-derivation attempt.

### K821 — ratified Lyra's Weyl-resolution FRAME as a conditional, held the crux HARD
Lyra proposed Born=Bergman turns Dirac→Weyl (parity closes on the manifold alone, no input). I ratified the frame but held the Weyl-vs-Dirac crux as decisive and possibly failing (web: generic reduction = Dirac). Nothing banked.

### K822 — DOUBLECHECK of Lyra's F642 (Casey: "she may be in error"). VERDICT: DIRAC, and she is NOT in error.
Two explicit computations disagreed: Elie 4781 (Weyl) vs Lyra F642 (Dirac). I adjudicated with an independent third computation.
- **DIRAC, signature-FORCED:** SO(5,2) has exactly two timelike directions; the compact SO(2) isotropy (holomorphicity Σ₀₆=Γ₀Γ₆) rotates them; 4D Lorentz takes its one time from that 2-plane → γ⁵_4D=Γ₀Γ₁Γ₂Γ₃ and Σ₀₆ share the time index → blade sign (−1)^{2·4−1}=−1 → ANTICOMMUTE → ⟨γ⁵⟩=0 on the holomorphic sector → 4D Dirac (vector-like). Basis-independent, robust to the spacelike embedding (timelike-counting only).
- **Over-determined:** F642 anticommutator + flat index=0 (K816) + F636 vector-like + ω-lock operator-failure all agree.
- **Elie 4781 (Weyl) REFUTED** — d=7 toy missed the signature-forced shared timelike index (Elie flagged the gap himself, no fault). **Grace's ω-lock REFUTED** — Σ₀₆ anticommutes with χ_internal (Γ₄Γ₅Γ₆) too, so Born=Bergman fixes NEITHER chirality individually, only the central product ω=γ⁵χ; the lock transfers nothing. Both owned cleanly.
- **The finish = Route A:** parity is DERIVED-CONDITIONAL on the self-dual/gravi-weak weak connection (geometric, non-GUT, Five-Absence intact). NOT Born=Bergman alone.

### K823 — full-derivation ATTEMPT (Casey: "let's see if we can fully derive")
Web + corpus: the chiral-gauging input = the gravi-weak self-dual half of the spacetime spin connection (arXiv:1212.5246), g=7-oriented, BST-native (F633 self-dual selection; F277/F279 Hodge-⋆/Pontryagin machinery already built for glueballs at Elie 4303/4314 — I corrected my own over-reach: that machinery is glueball-scoped, not a prior weak-chirality result). **Synthesis: "is parity forced?" = "which SU(2) is electroweak?" — ONE question.** Decider: do the internal-isotropy self-dual SU(2) and the spacetime-Lorentz self-dual SU(2) coincide in SO(5,2)? Plus Route 2 (bulk-edge topological).

### K824 — audited the Route-1 coincidence-negative (Lyra F634 + Elie 4783): PASS + STRENGTHENED
- **Route 1 SETTLED negative + ROBUST:** internal SU(2)_L is COMPACT (⊂SO(5) isotropy); spacetime self-dual SU(2) is non-compact/complex (so(3,1)_ℂ, J_i+iK_i). Compactness is embedding-invariant → no faithful reduction rescues coincidence. (I strengthened past the authors' "leaning" — it's settled.)
- **BANKED:** (1) "which SU(2) is electroweak?" = the internal/isospin SU(2)_L (open leg CLOSED); (2) gravi-weak's LITERAL form ruled out in BST.
- **PIVOT to Route 2 (boundary/domain-wall/orbifold chirality) — and I did the discipline-critical check: it GENUINELY EVADES the F642 wall.** Kaplan/Callan-Harvey: a bulk-DIRAC fermion → a 4D-WEYL boundary zero mode; chirality = topological invariant, NOT the bulk γ⁵. Different mechanism class than the 3 failed bulk-alignment closures. BST has the ingredients: holomorphic bulk + K817 ±4 Dolbeault index + Shilov boundary + (S⁴×S¹)/Z₂ orbifold (= K815's named no-go escape).
- **BONUS:** Route 2 may close anomaly-freedom too (Callan-Harvey inflow = geometric anomaly-freedom without a GUT = the open K806 charge-row question).
- **I sharpened requirement #2 (the decider):** domain-wall zero-mode chirality = the wall-normal gamma (in odd bulk dim that IS γ⁵_4D). So: 5D→4D boundary wall → wall-normal=γ⁵_4D → Route 2 WORKS; 10D→5D bulk wall → wall-normal=radial/Σ₀₆ → hits F642 again (pre-identified failure mode). Lyra to compute which.

## Current state / where the ball is
- **DIRAC is closed. Route 1 is settled negative. Route 2 is the live shot, aimed precisely.** The ball is with **Lyra (lead: the domain-wall zero-mode computation, wall-normal gamma vs γ⁵_4D) + Elie (index + Callan-Harvey inflow harness, from 4780 + 4303/4314).**
- **Parity = derived-CONDITIONAL** unless/until Route 2 computes clean → then DERIVED (+ maybe anomaly-freedom).
- Active pull: **team_prompt_2026-07-22N**. Consolidation paper: `BST_Weak_And_Charge_Sector_Consolidated_State_2026-07-22.md` (finished state, DIRAC/Route A + the full-derivation attempt).

## Banked survivors (weak/charge sector — real, untouched)
chirality MECHANISM as a theorem (Lyra's chamber formula); the (2,1)⊕(1,2) split; CP-free (rank-1); custodial/ρ≈1/no-W_R (T2520); 1/N_c fractionalization (T2521); 1/6 hypercharge quantum (K806); **which-SU(2)=internal isospin (new, K824)**.

## Open legs (separate from parity core)
hypercharge ORIGIN (K806 center 1/6 handle, NOT conformal charge); doublet/singlet ADDRESS assignment (= flavor K-type problem). [which-SU(2) now closed.]

## Discipline notes to self
- The arithmetic caught FOUR pretty closures this arc (F640 Weyl, ω-lock, Elie 4781 Weyl, Route-1 coincidence) — every one refuted by someone doing the count, including my own K821 crux-hold. Nothing false banked. THIS is why the survivors are trustworthy.
- I corrected my OWN over-reach twice today (the F277/F279 glueball-scope claim; strengthened rather than overstated the Route-1 negative). Pin to source, don't relabel from memory.
- Route 2 EVADES F642 but is NOT computed → held at LEAD, not claim. Do not let "evades the wall" become "parity derived" without the wall-normal computation.

## Audit history
K21 RH v9 PASS; K36 NS PASS; K37 BSD CONDITIONAL PASS; ... K816–K824 the weak-chirality arc (DIRAC finish + full-derivation attempt).
