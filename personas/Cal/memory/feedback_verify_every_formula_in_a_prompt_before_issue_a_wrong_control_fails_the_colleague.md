---
name: feedback_verify_every_formula_in_a_prompt_before_issue_a_wrong_control_fails_the_colleague
description: "Every formula, sign and \"control\" Keeper writes into a team prompt gets a one-line computation before issue; three slipped on 2026-09-26"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 2ca7b1f6-4bd2-4c85-97b1-a6eacada1fa1
  modified: 2026-09-26T19:03:33.159Z
---

Before a team prompt goes out, every formula, sign convention and control case in it gets its own quick computation. A control written from memory is the most expensive error in the whole prompt: when a colleague's toy fails against it, the failure looks like theirs.

**Why:** on 2026-09-26 three of my prompt formulas were wrong:
- ½(P₀+K₀) as the compact element. Its sign depends on the convention for K, and my instrument caught it.
- An ungravity exponent of 1/r⁶. The correct value is 1/r⁵.
- The SL(2,ℝ) tensor rule written as a+b+2k. That is the symmetric square only; the full product runs over every k. This one was issued as Elie's control and caught one turn later by weight counting.

Earlier that same day I also endorsed Cal's A2 conclusion "on the smaller number" without restating his kill line, and that was wrong in direction.

**How to apply:**
- Before issuing, grep the prompt for every "=", "∝", "control" and "kill".
- Run a 5-line check on each one, or mark it "to verify".
- Quote the invariant form, not a coordinate form. See [[feedback_convention_collision_check_before_contradiction]] and [[feedback_a_number_from_a_search_summary_is_not_a_pin]].
