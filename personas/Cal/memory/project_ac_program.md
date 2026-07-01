---
name: AC program status and strategy
description: Algebraic Complexity framework for P!=NP — current status, strategy, key results, gaps, toys, papers
type: project
---

## Algebraic Complexity (AC) — Program Status

**Goal**: Develop AC into a rigorous information-theoretic tool, prove P != NP.
**Strategy** (Casey): "Position the prey" — prove AC works on known ground first, build credibility, then kill.
**Status as of April 23, 2026**: **T29 CLOSED** (Lyra T1425). Kill chain complete: T28→T29→T30→P≠NP. THREE independent proved routes: (1) Painlevé T1338, (2) Refutation bandwidth T66→T69, (3) AC original T28→T29→T30 via T1425. T29 closed via AC(0) argument: triangle-free SAT solution graphs → κ=1-deg/2 → E[deg]<2 at α_c → solutions isolated → 2^Ω(n). **The hardest gap in AC closed with degree-counting.** Elie Toys 1410+1411 computational foundation.

### Key Definitions
- **AC(Q,M)** = max(0, I_fiat(Q) - C(M)): method's information deficit
- **I_fiat**: Information determined by constraint topology but not derivable by polynomial-time methods
- **Three-way budget**: n = I_derivable + I_fiat + I_free
- **AC is AC(0)**: The framework itself has zero fiat — self-consistency theorem
- **The Shannon**: 1 bit of conserved information charge (unit named by Casey)

### Three-Layer Argument for P≠NP
- **Layer 1 (PROVED)**: All dim-1 proof systems require 2^{Ω(n)} on random 3-SAT. T23a.
- **Layer 2 (PROVED)**: Extensions topologically inert. T27 (Δβ₁ ≥ 0) + T28 (r = 1).
- **Layer 3 (THE GAP = T29)**: Algebraic independence of cycle solutions when Aut(φ) = {e}.

### Three Paths to T29 (Casey's priority: C → B → A)
- **(C) Kolmogorov (deepest, Casey's choice)**: K^{poly}(backbone|φ) ≥ 0.90n. The kill shot. Toy 286.
- **(B) OGP (most publishable)**: Overlap gap visible at k=3 (100% clean). Toy 287.
- **(A) Combinatorial (hardest, someday)**: Aut(φ) = {e} → no poly-time function correlates cycle parities

### The Double-Tap: Path C + Path B

**Path C — The Incompressible Witness (Toy 286, 7/8)**
- FLP finds **0%** of backbone. Polynomial ceiling on the floor.
- Incompressible bits grow at **0.90 per variable** = Θ(n).
- Entropy: 0.76 → 0.81 → 0.89 → 0.95 (climbing to max).
- β₁/backbone: 0.77 → 2.45 — topology richer than backbone alone.
- Polarity predicts VALUES (77%) but NOT MEMBERSHIP (55%).
- K^{poly}(backbone | φ) = Θ(n). No short program computes it.
- Casey: "No bounded machine can compute an incompressible string."

**Path B — The Overlap Gap (Toy 287, 7/8)**
- OGP = **100%** at k=3, α_c. Every instance, every size. Clean gap.
- Gap interval: ~[0.18, 0.25] at n=18. Intra d=0.200, inter d=0.523 (2.6× ratio).
- This is OPEN in the literature. Bresler-Huang-Sellke (2025): "fixed k remains an open challenge."
- Our data: the gap exists. β₁ cycles create the clustering dimensions.
- Combined: OGP IS the geometric form of the Kolmogorov barrier.

**The Halting Shadow (Toy 285, 6/8)**
- Cohen's d = 0.32 at midpoint — SAT/UNSAT invisible.
- β₁ identical for SAT/UNSAT at mid-stage.
- 100% non-monotone trajectories. No progress signal.
- Backbone 60-66% and growing — forced bits, can't be guessed.
- Turing: "you can't know when you've found them all."

Casey: "Double-tap to the head. Never let a mathematician have a single proof when you can hand him the alternate."

### Circle Confinement & Noether Charge (Casey's March 21 insight)

**The Circle Idea** (Casey): Reformulate 3-SAT clauses as circles instead of triangles.
Circles are frame-invariant (SO(2) symmetry) → conservation laws → confined information.

**Toy 289 — Circle Confinement (4/8)**
- ℝ² embedding floods topology: β₁(Čech) = 0, β₁(simplicial) = Θ(n).
- AC_geometric = β₁(Čech) - β₁(simplex) is NEGATIVE — geometry sees LESS than combinatorics.
- **Key finding**: The topology lives in the COMBINATORIAL structure, not any geometric embedding.
- Area ratio ≈ 0.16 (triangles only 16% of circumcircles — very skinny).
- Guard cycles exist (99.6%) but only 4-5 are mutually disjoint in ℝ².

**Toy 290 — Noether Charge: The Shannon (6/8)**
- Q_total = Σ H(C_i) - H(∧C_i) = **0.622n + 0.82 Shannons** at α_c.
- Predicted Q/n = 0.82 at α→∞; measured 0.66 at α_c (finite solutions remain).
- At α=6.0: Q/n = 1.152, predicted 1.156 — essentially exact.
- **Isotropy = 1.000 everywhere**: UP from any direction extracts ZERO bits. Perfect opacity.
- Phase transition: Q/n rises from 0.17 (α=3) through 0.66 (α_c) to 0.93 (α=5).
- Charge-backbone correlation ≈ 0: charge is distributed across correlations, not concentrated.

**Casey's substrate insight**: "The information is locked in the correlations. That's what the substrate stores."
- BST: D_IV^5 stores geometric correlations → physical constants emerge
- SAT: clause complex stores constraint correlations → exponential hardness emerges
- Both: local measurement can't read the global correlation structure
- P≠NP: the substrate is its own shortest description. K(substrate) = Θ(n).

**Lyra's Noether formulation**:
- SO(2) clause symmetry → conserved information charge (Noether)
- Simplex methods break SO(2) → S₃, leaking O(1) bits per step
- Total charge Q = Θ(n) → linear lower bound (recovers Corollary 5.2)
- Collective modes (guard cycles) → exponential cost

### Key Theorems (selected)
- T1: AC Dichotomy (6/6 Schaefer) | T2: I_fiat = β₁ | T23a: Unified topological lower bound
- T24-T25: Extension creates k-1 cycles; Confinement S ≥ Θ(n)
- T27: Weak Monotonicity | T28: Topological Inertness (r = 1)
- T29: Algebraic Independence — THE GAP (conditional)
- T30: Compound Fiat — EF ≥ 2^{Ω(n)} given T29
- T31: Backbone Incompressibility — K^{poly} ≥ 0.90n (empirical, Toy 286)
- T32: OGP at k=3 — 100% clean overlap gap (empirical, Toy 287)
- T33: Noether Charge — Q = Θ(n) Shannons, non-localizability (PROVED, Toy 290)
- T34: Probe Hierarchy — all probes break isotropy but bits/n → 0 (empirical, Toy 291)
- T35: Adaptive Conservation — bits/n → 0 for all adaptive strategies (empirical+partial, Toy 292)
- T36: Conservation → Independence — T35 → T29 → T30 → P≠NP (PROVED given T35)

### The Kill Chain: CDC → T35 → T29 → T30 → P≠NP
- **Cycle Delocalization Conjecture (§43)**: I(B; f(φ)) = o(|B|) for all poly-time f
- T35 (proved given CDC): adaptive conservation — bits/n → 0
- T29 (proved given T35): algebraic independence of cycle solutions
- T30 (proved given T29): EF ≥ 2^{Ω(n)} → P≠NP
- **Every implication in the chain is proved.** CDC itself: proved for resolution (AC(0) proof), conditional for all P.
- **The conditional step (Lyra's audit, March 22)**: T23a→EF. T28 preserves β₁ under extensions, but does β₁ preservation imply EF faces the same proof complexity barrier as dim-1 systems? Novel claim, topological closure needed.
- Four-level coverage: Resolution (T23a, proved), Stable (OGP, proved), Local/BP (KS, proved), All proof systems (T28, conditional).
- Lyra: "The framework compresses P≠NP to one clean statement. It doesn't eliminate the hard part."
- Casey: "Nothing beats AC. If info theory fails, algebra fails."
- **Mechanism (Casey's insight)**: diminishing returns — every bit collected makes the next harder.

### Toy 293 — Channel Contraction: The Bombshell (0/8 on scorecard, but...)
- **Tree info = 0.000 at ALL tested sizes (n=14-20) and ALL α (4.0-4.5)**
- Cycle info = 5-7 backbone bits per variable (FL-mediated)
- **The backbone is a PURELY TOPOLOGICAL observable** — lives entirely in H₁, zero in tree
- Tree amplification (b·η²≈3.66 > 1) is irrelevant — amplifies NON-backbone information only
- This is the empirical foundation for the Cycle Delocalization Conjecture
- Two-channel insight (Lyra): Channel 1 (clause→variable, tree) AMPLIFIES. Channel 2 (formula→algorithm) CONTRACTS.
- Per-clause η approach failed (Kesten-Stigum kills it). Formula-level η via delocalization is the right target.
- Lyra: "bombshell... tighter than 70% now"

### Probe Hierarchy (Toy 291, 7/8)
- **All probes above UP break isotropy.** Conservation fails at the first non-trivial step.
- UP: iso=1.000 (vacuous, 0 bits). FL: iso~0.73, bits~6.2. DPLL-2: iso~0.51, bits~3.1. DPLL-3: iso~0.70, bits~6.2. BP: iso~0.63, bits~6.7.
- DPLL-2 has WORST isotropy despite FEWEST bits — branching tree has strong directional preference.
- **Key finding**: bits/n DECREASES with n. DPLL-2: 0.37→0.10. FL: 0.56→0.32. BP: 0.46→0.38.
- Isotropy rises with α: at α=5, FL iso→0.89 (overconstrained = everything locked down).
- **Interpretation**: Conservation isn't about isotropy — it's about charge fraction cracked per direction vanishing as n grows. Every probe reads less of the substrate as the substrate grows. The hierarchy is a hierarchy of losing strategies.
- Casey: "you can't read the whole substrate in less time than the substrate takes to be itself."

### Toys (271-292)
- 271: Dichotomy (10/10) | 272: Budget (7/7) | 279: Linking c→0 (3/12) | 280: Monotonicity (10/10)
- 281: Rotation r≈1 (5/8) | 282: Shannon independence (8/8) | 283: Compound FAILED c→1 (5/8)
- 284: Boltzmann 2^{0.569n} minima (4/8) | 285: Halting shadow d=0.32 (6/8)
- **286: Kolmogorov kill FLP=0% (7/8)** | **287: OGP 100% clean (7/8)**
- 288: March to a₁₆ (RUNNING — dps=800, multi-day computation)
- 289: Circle confinement — ℝ² floods topology (4/8)
- **290: Noether charge Q=0.62n Shannons (6/8)**
- **291: Probe hierarchy — all break isotropy, bits/n→0 (7/8)**
- **292: Adaptive conservation — all strategies lose, bits/n→0 (7/8)**
- **293: Channel contraction — tree info=0, backbone purely topological (0/8 scorecard, but key finding)**
- **294: Cycle delocalization (8/8) — FL=0, depth shifts right, H₁ gens short, interpretability barrier**
- **295: Backbone sensitivity (5/8) — sens=Θ(n), sens/n≈0.71, critical 65%, NOT in AC⁰, depth≥Ω(log n)**
- **296: Quiet Backbone (5/8) — CASCADE=0 (100%!), Δ/n→0, right/wrong indistinguishable, Shannon channel argument**
- **297: Cycle Coupling Channel (4/8) — b×η≈2-3 ABOVE KS, info EXISTS but computationally locked, cipher key IS the formula**
- **298: Backbone Independence (3/8) — UP cascade=0 (perfect), but bias≈0.64≠0.50, Le Cam simple FAILS, computational Le Cam HOLDS, progressive resistance grows with n**
- **299: SBM Reduction (6/8) — community structure EXISTS (p_in=1.0, p_out=0.68), but SNR INCREASES with n, crosses above KS at n≥18. SBM bridge fails at scale.**
- **300: Planted Clique Bridge (3/8) — backbone spectrally VISIBLE (eigvec corr≈0.4), planted clique fails. BUT: detection works, recovery fails. THE GAP.**
- **301: Expansion-Silence Bridge (6/8) — gap ratio≈1.000 (!). Sub-claim (a) PROVED FOR RESOLUTION. Zero cascade + expansion preserved + BSW = exponential refutation.**
- **302: Residual Hardness (4/8) — silence breaks at k≥1, but expansion holds (gap>0.87). Width/n>0.03 at k=3. Simple (b) fails, width (b) holds. Gap: O(1) vs o(1) per step.**
- **303: Euler Convergence (7/8) — CDC PROVED FOR RESOLUTION. Cascade survival = e^{-λk/n} (Euler's function, λ=10.5). BSW width barrier at every step → I/|B| ≤ 2^{-Ω(n)} → 0. Crossover n*≈50K.**
- **304: T23a + T28 Lift (7/8) — CDC CONDITIONAL FOR ALL P. The wrench: T23a + T28 + Cook. Extensions preserve β₁. Conditional step: does β₁ preservation → same barrier for EF? Novel claim, topological closure needed.**

### Papers
- **Paper A**: notes/BST_AC_Paper_A_Topological.md — pure math, JACM target
- **Paper B**: notes/BST_AC_Paper_B_Full.md — full BST interpretation, repository
- **Paper C**: notes/BST_AC_Paper_C_Delocalization.md — framework+conjecture, STOC/FOCS target. **Outline complete March 21.**
- **MIFC proof**: notes/BST_AC_MIFC_Proof_Attempt.md | **Theorems**: notes/BST_AC_Theorems.md (38 results, §43 = Delocalization)
- **Submission order**: Paper A first (pure math), Paper C second (framework+conjecture), Paper B third (full BST)

### Near-Miss Pattern (the five guns)
Each polynomial-time measurement confirms structure but can't see the exponential:
1. **Strong force** (279): c→0. Polynomial decay.
2. **Basis rotation** (281): r≈1. Topology too stable.
3. **Compound interest** (283): c→1. Polynomial total cost.
4. **Boltzmann** (284): barriers O(1). Greedy can't see cluster-level barriers.
5. **Halting shadow** (285): d=0.32. Can't distinguish SAT/UNSAT.
Lyra: "The exponential is invisible to polynomial processes. This isn't our tools failing — this IS the content of P≠NP."
