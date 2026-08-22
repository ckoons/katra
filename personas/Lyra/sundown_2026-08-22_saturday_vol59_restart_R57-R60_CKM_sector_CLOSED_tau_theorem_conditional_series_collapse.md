# Sundown — Lyra, Saturday 2026-08-22 (EOD), vol 59 — post-restart, CKM sector CLOSED

## Who I am
Lyra. Mathematical physicist, research partner to Casey on BST. Lead with the math; say "I don't know"; protect the derived/conjecture boundary; POSITIONS vs VALUES, structure vs scale. Own a miss cleanly — and don't accept a wrong correction just because it came from a teammate with standing. The lyre: two boundaries, resonant strings between.

## The arc: vol 58 checkpoint → restart → R57–R60

Casey restarted the session mid-day (three framing misses from long context; the team caught all three). Fresh context, reconnect-before-deriving. Four rounds followed, and **the CKM sector closed.**

### R57 — τ, and two walk-backs of my own
- **Cal's prior question, answered plainly: τ = ln a is a MODELLING CHOICE, not forced** (it means κ = H identically — a dynamical coincidence). The escape is not to force it but to prove the inequality over the whole clock class.
- **"The bar is C₂ = 6" — RETRACTED.** The real bar is **B(τ) = Var_τ(λ)/⟨λ⟩_τ**, spanning **[0.93, 17.3]** over the pre-registered §4751 knob space; 6 is its τ→∞ asymptote. Also found: **c₀ > 0 is load-bearing** (c₀=0 ⟹ B→0 ⟹ any τ″>0 flips the sign).
- **What rescues the bar is DATA, not geometry:** small B needs near-degenerate λ ⟹ large r ⟹ large |w+1|, already excluded by measured w₀. Cut |w+1|≤0.2 ⟹ **B_min = 5.43** (→6 as the cut tightens).
- **New exact identity: τ″/τ′² = v·[(3/2)(1+w_tot) − s]**, v ≡ HT, T ≡ 1/κ, s ≡ dlnT/dlna. Verified 4/4 to <1e-6. Horizon clock → exactly 0 (**that IS τ = ln a**); v=1,s=0 → 0.465 (**identifies my old "Koons-tick ≈0.5"**).
- **T2573 (candidate, NOT registered): C1 v≤1 · C2 s≥0 · C3 w_tot≤0 ⟹ τ″/τ′² ≤ 3/2 vs B ≥ 5.4 ⟹ margin 3.6× ⟹ w_a>0.** **C2 is HELD** (ML+NEC bounds κ_max, not κ). **My R56 "UNCONDITIONAL" walked back → ships CONDITIONAL.** Casey's "finite tick rate + positive energy" does NOT close it — finiteness isn't the lever, C2's *direction* is.
- **C3 was mislabelled "NEC"** (Keeper caught it): NEC bounds BELOW at 0; C3 needs an UPPER bound, **w_tot ≤ 0, matter-domination onward (z ≲ 3400)**; fails in radiation (2.0 > 1.5).
- Found a dead citation: **Cal §352/K1283 cites T5230 as a theorem — it isn't** (registry tops at T2572; only 5230 object is a toy). Handed to Keeper.

### R58/R59 — the series question, FILED, and it dissolved the question
Two rounds of garbled relays; I refused to reconstruct both times (right call — Keeper confirmed). Canonical text landed at `notes/.running/wake/R59_TEAM_PROMPT.md`.

**FILED, before anyone computed** (Keeper's seal opened on it):
1. Q = A(P₆) block-off-diagonal in parity ⟹ **Q^{2k}|even = S^k**, S := Q²|even. Series in Q = series in ONE 3×3 matrix.
2. dim H_even = 3 ⟹ **S³ = 5S² − 6S + 1** ⟹ **every** series collapses to **G|even ≡ βS + αS² + γ1**.
3. **corner ratio = t/(1+4t), t := α/β, INDEPENDENT of γ** ⟹ severed the dependency on Elie's gauge statement rather than assuming it.
4. **Verdict: outcome (b).** The rail does NOT force t — same mechanism as ε one level up (t is a matrix-element ratio on normalized modes; normalization = FK/Wallach = mass-space = wrong space). **And no canonical form rescues it: exp/heat-kernel/resolvent are one-parameter families that merely RENAME t.**
5. **THE GAIN, derived:** S[1,3]=0, S²[1,3]=1 ⟹ ratio → t as t→0. **"Why is |V_ub| so much smaller than |V_cb|?" is DEAD — one power of t, because 0→4 takes four rungs and 2→4 takes two.**

### R60 — the seal opened; all five missed; and I did not accept a wrong correction
**All 5 candidates missed HIGH** (0.20–0.357 vs band [0.081,0.108]), 2.1×–3.8×. Pre-registered, sealed, opened — the strongest negative this program produces.

**Keeper "corrected" my Step 4 range [0,4/9) → [0,1/4). I rebutted and the rebuttal held:** we bounded different hypotheses — mine **a_2k ≥ 0**, theirs **t ≥ 0**. The pole at t=−1/4 is **unreachable from non-negative coefficients** (reduction directions → t ∈ [0,∞) ∪ (−∞,−4/7]; the gap containing the pole is empty). **The counterexample was inside Keeper's own seal table: S5 = pure Q⁶ = S³ has a_6=1 ≥ 0, t=−0.833, ratio 5/14 = 0.3571 > 1/4.** Direct matrix checks (no reduction): S³→0.3571, S⁴→0.4043, S³+S⁴+S⁵→0.4167.

**Accepted clean:** denominator-collapse WRONG as applied (five candidates pin five distinct t; genuine 5-trial seal). **t is a COORDINATE not an invariant** (t→t/c² under Q→cQ) — always say "at the integer P₆ normalization"; the ratio is the invariant. Elie is 3-for-3 on invariants-over-coordinates.

**My addition to the closing record — the SHAPE of the negative:** the band needs **β/α ∈ [5.26, 8.35]** — S must outweigh S² by 5–8×. All five candidates are S²-heavy ⟹ **that's WHY they missed, and why they missed HIGH not scattered.** A survivor would need to be **lopsided**, and pure powers/exponentials/resolvents all weight successive powers comparably or increasingly. **The negative is STRUCTURAL, not a bad draw from five tickets.**

## Where the CKM sector stands (CLOSED, count unmoved)
- **DERIVED:** skeleton/rank-1 · λ = 1/√20 · CP existence (T2547) · flavor-universality = partial-isometry condition · **+ the ORDER (one power of t)**
- **NEGATIVE (pre-registered, sealed, opened):** all five series, miss 2.1×–3.8× high, **with a mechanism**
- **INPUT:** the value (ratio ≈ 0.093 ⟺ t ∈ [0.120, 0.190] at P₆ normalization) · V_cb value RETIRED / position kept · δ_CP

## Restart targets (Casey recommends redirect off CKM)
1. **Atlas #125** — the standing H_{ν_W} frontier.
2. **The strong sector.**
3. **The descent.**
4. Owed if τ is reopened: **close C2** (κ non-increasing) — that's what makes T2573 unconditional.
5. Open elsewhere: the 865-node graph residue (Cal's T1230 node-type-contamination lead); K1801.

## Guards (standing, intact + new this session)
Reconnect before deriving. POSITIONS vs VALUES. A projector has no scale; a graded operator does. Same-number collisions. Sign is the most relay-fragile object. PD carries its fraction, never bare. File the number BEFORE comparing to the target. Count the denominator. **NEW: when you attach a named principle to an inequality, check the DIRECTION of the bound it actually supplies — 3 firings in 3 rounds, number right/label wrong; and this guard needs an INSTRUMENT (positive control), not vigilance.** **NEW: when you correct a conditional claim, restate the ANTECEDENT verbatim before testing the consequent — a bound is a two-part object.** **NEW: lead with invariants, not coordinates.** Refuse to reconstruct a garbled prompt — say so and work nothing else (worked twice). Nothing external without Keeper-PASS + Cal-vet + both voices + Casey GO. Nothing pushed; CP existence-only.

## Cognitive state
Clean. Good day — four rounds, three of my own claims retracted (bar=C₂, C3=NEC, positivity<1/4), one wrong correction of mine rebutted successfully, and a real structural result filed blind that dissolved a five-way question into one number. The restart did what Casey said it would: the error rate dropped and the catches got sharper. The CKM sector closes honestly with the order derived and the value named as input.

## Files that matter (today)
`Lyra_R57_tau_is_a_modelling_choice_...md` · `Lyra_R59_FILED_the_series_question_...md` · `Lyra_R60_Step4_stands_...md`. Scripts: `play/bar.py`, `bar2.py`, `bar3.py`, `series.py`, `series2.py`, `check.py`. Canonical prompts: `notes/.running/wake/R59_TEAM_PROMPT.md`, `R60_TEAM_PROMPT.md`. Comms: `notes/.running/RUNNING_NOTES.md`.

## How to pick up
Read MEMORY.md, then this sundown, then RUNNING_NOTES + the R60 prompt. CKM is closed — do not reopen it without a reason. The frontier is the atlas (#125), the strong sector, or the descent. T2573 is a *candidate*, not registered; counter is 2573 and correct (high "T-numbers" in running notes are toys). Welcome back.
