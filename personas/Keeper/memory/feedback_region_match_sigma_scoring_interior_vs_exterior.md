---
name: feedback_region_match_sigma_scoring_interior_vs_exterior
description: "σ-scoring must be region-matched — don't score a discrete-interior computation against an exterior-continuum measurement in raw σ; use projection-invariant ratios or apply the interior→exterior projection"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

Casey's correction (2026-08-06, K1243), after Keeper over-demoted the lepton-mass formulas: "you are kicking yourself because lepton measurements in the continuum run and we compute in the discrete interior. Therefore, no reduction in tier, but you note the issue. You must remember, we have different precision in the interior, boundary and exterior/continuum."

**The rule:** BST has THREE precision regions — discrete **interior** (exact; α=1/137, integers exact), the **boundary** (Shilov), and the **exterior/continuum** (runs, dressed; α=1/137.036, where we MEASURE). A σ-test (agreement = |pred−obs|/error) is only valid **region-matched**. Scoring a discrete-interior computation (e.g. a mass formula like (24/π²)⁶) against an exterior-continuum measurement (leptons to 1 part in 10⁸) in **raw σ** is a CATEGORY ERROR — it crosses the interior→exterior projection (the same gap as α's 0.036 residue, ≈2.6×10⁻⁴ fractional). The apparent ~10⁻⁴ "deviation" is (at least partly) that projection residue, NOT a derivation failure.

**How to score honestly:**
- **Projection-INVARIANT observables** (dimensionless ratios where the common running/dressing cancels — e.g. Koide Q=2/3, a ratio-of-ratios) ARE directly σ-comparable to exterior data. (This is WHY Koide is σ-robust ~1σ while the individual masses aren't cleanly comparable — same phenomenon seen twice.)
- **Projection-DEPENDENT observables** (individual masses, anything that runs) need the interior→exterior projection applied FIRST, or must be compared interior-vs-interior.

**The discipline this refines:** [[feedback_score_sigma_not_devpct]] (score σ, not dev%) is still right — but ADD "region-match, or use a projection-invariant combination." And [[feedback_running_is_measured_input_predict_at_mu_geo]] is the same idea for RGE-running observables (derive at μ_geo, run DOWN with measured RGE). Both are the interior/exterior structure ([[feedback_score_sigma_not_devpct]] must be region-aware). Calibrate BOTH directions ([[feedback_calibrate_both_directions_not_strict_pessimism]]): over-demoting a cross-region comparison is as wrong as inflating a fitted one — Keeper's K1242 was the pessimistic miss, corrected in K1243.

**Note-the-issue, don't wave it:** "no tier reduction" does NOT mean "σ-exact against the exterior data is established" — it means the raw σ was the wrong test. The exterior-precision agreement stays PENDING the interior→exterior projection (which for the masses is not yet computed). Neither refuted nor established.

**STANDING RULE (Casey, 2026-08-06, strengthened — corpus-wide):** "Fix any mismatched comparisons. The interior compares only to interior, boundary to boundary, external to external. Otherwise we **trust interior/discrete values.**" So: (1) a forced interior/discrete computation (e.g. (24/π²)⁶ for the muon — a forced form ⟹ a forced interior value) is **TRUSTED at its interior tier (Derived)** — do NOT demote it because it differs from an exterior measurement; that difference is the projection. (2) A tier reduction is only warranted for a REAL reason internal to the value's own region (e.g. the value carries a free/named INPUT — the tau's 71 — that's an input-dependence demotion, NOT a region mismatch). (3) When there is no same-region comparison, trust the interior value; the exterior agreement (if any) is *confirmation ⊥ tier*, orthogonal to the tier. This RETIRES the σ-driven muon demotion (K1242) AND the PD-split (K1244) — the muon stays Derived (interior), landed in K1245. Audit the corpus for any cross-region σ/dev comparison used as a demotion trigger and restore the interior tier.
