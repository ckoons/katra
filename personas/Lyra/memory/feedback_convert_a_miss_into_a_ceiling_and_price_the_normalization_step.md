---
name: convert-a-miss-into-a-ceiling-and-price-the-normalization-step
description: Scan the whole parameter space to turn "the forced value misses" into "no value works"; and test whether a normalization/unitarization step is a convention or a falsification patch.
metadata:
  type: feedback
---

Two moves that turned a routine negative into a lane-closing result (Grace, Round 50, 2026-08-22, the FK overlap matrix).

**1. A ceiling beats a miss.** "The forced parameter misses the target" invites the next person to guess a better parameter. "No parameter in the entire space reaches the target" closes the lane. Convert one into the other by scanning the *full* space exhaustively — and check whether the ceiling **turns over** as the parameter grows, so nobody can answer "but at large ν." (Max V_us/V_cb ≈ 1.95 peaked at ν≈30 and fell; required ≈5.4; ~1.1M configurations, zero passes.) Elie's O7 landed the same shape the same morning — a proved ceiling on a method is a *result*, not a failure.

**2. Price the normalization step.** A final SVD/polar "unitarization," row-normalization, or projection can be a **falsification patch wearing a convention's clothes**. Test it: the un-normalized object should already be *near* the target property. Here the polar step discarded a factor-21 distortion, and the raw matrix failed diagonal dominance outright — so the "pass" was manufactured. Null-test the step alone (uniform random 3×3 → 14.3% diagonal dominance after polar).

**3. Ask what a reframe COSTS, not what it gives.** The sharpest finding came from asking what BST's "CKM = one current matrix element" has to *earn* that the SM gets free (unitarity: V = U_up†U_down is automatically unitary; one matrix element is not). Reframes get audited for what they explain and rarely for what they now owe.

**Why:** a miss is a soft negative that leaves the lane open and the team re-guessing; a ceiling is a theorem. And a normalization step is where a dead ansatz most easily survives.

**How to apply:** before reporting any parameter-miss, scan the whole space and look for turn-over; positive-control the instrument first ([[validate-the-instrument-before-reporting-a-negative]]); and separately null-test every normalization step. See also [[feedback_empty_confirmation_cant_fail_test_and_circular_hunt_mechanism]], [[feedback_family_sweep_every_forcing_selector_rank_generic_is_selecting_nothing]].
