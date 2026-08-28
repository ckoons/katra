---
name: when-the-reason-is-wrong-do-not-assume-the-number-is-wrong-too
description: A positive control that refutes a justification tells you the reason is broken, not that the number is; re-derive the value independently before replacing it, or you correct a right answer into a wrong one.
metadata:
  type: feedback
---

Lyra, R59 (K1808). She claimed "non-negative coefficients bound the corner ratio below 1/4." A positive
control refuted the *reason* — coefficient positivity does not survive Cayley–Hamilton, since
S³ = 5S² − 6S + 1 puts −6 into β. She then **corrected the number** to "ratio ∈ [0, 4/9)".

**The original number was right and the replacement was wrong.** t/(1+4t) has a pole at t = −1/4 and is
unbounded over real t; the correct statement is **t ≥ 0 ⟺ ratio ∈ [0, 1/4)** — her first bound, on the
branch where the target actually lives.

**Why:** this is the decorative-clause family running in reverse. The usual failure is a false reason
bolted to a correct number, surviving every correction. This one is the same defect handled too
aggressively: killing the justification and dragging the value along with it. **A refuted justification
is evidence about the justification only.**

**How to apply:** when a positive control kills a reason, (1) strike the reason, (2) mark the number
*unsupported*, (3) **re-derive the number independently** — and only then replace it. If the independent
derivation reproduces the original, you have a right answer that was badly argued, which is a different
and much better state than a wrong answer.

Related: [[feedback_decorative_clauses_hide_errors_sweep_both_directions]],
[[feedback_calibrate_both_directions_not_strict_pessimism]],
[[feedback_validate_the_instrument_before_reporting_a_negative]]
