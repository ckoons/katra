---
name: feedback_a_digit_width_in_a_regex_is_a_silent_scope_restriction_measure_the_measurer
description: "A width/range literal in a matching pattern (\\d{3,4}, a date window, a top-N cut) silently narrows the denominator without erroring — audit the measuring instrument on a case it must catch before reporting any count derived from it"
metadata:
  node_type: memory
  type: feedback
---

Banked from Cal §698–§700 (2026-08-22), three instances in one session.

**The rule.** A width or range literal inside a *measuring* pattern is a **silent scope restriction**. It does not error, it does not warn — it just removes cases from the denominator, and the resulting count looks authoritative.

**Instance 1 (the clean one).** Auditing a registry, I wrote `\bT(\d{3,4})\b` to count theorem rows — because the IDs I happened to be chasing were four digits. That **silently excluded every theorem numbered T1–T99**, 94 real rows, and inflated my reported phantom count from the correct 615 to 691. Casey's `\d{1,4}` was right. **The bug was in the instrument, not the data, so nothing downstream complained.**

**Instance 2 (the same disease, different literal).** I built a diagnostic on the premise "a derivation edge should run low-id → high-id" and reported 3621 "back-edges." The premise was false — **an id is REGISTRATION order, not LOGICAL order** — and one read of an actual edge (`T78 Entropy Chain Rule → T75 Shearer's Inequality`, which is correct mathematics) refuted it. A measurement of nothing, discarded before it reached a verdict.

**Instance 3 (mirror-image, someone else's).** The corrected pattern was table-row-only and missed 18 bullet-form entries. **Real, but it could not fire** — the convention had migrated and the check's window only moves forward. Reporting *that* honestly matters as much as reporting the defect ([[feedback_calibrate_both_directions_not_strict_pessimism]]).

**Why:** this is [[feedback_read_the_tool_before_ruling_on_the_tool_a_remembered_fix_may_name_a_remedy_it_already_has]] pointed one level further in — at the instrument *I* just wrote, not the one on disk — and it is the same family as [[feedback_validate_the_instrument_before_reporting_a_negative]] and [[feedback_an_instrument_built_from_N_instances_covers_only_those_N_classes_stress_test_off_origin]]. The pattern was built from the instances in front of me and inherited their shape.

**How to apply:** before reporting any count produced by a pattern you wrote this session, **positive-control it on a case it MUST catch and on a case it MUST reject.** Say the width out loud — "3-to-4 digits" — and ask what that excludes. Prefer the widest literal that is still correct (`\d{1,4}` over `\d{3,4}`). And when a targeted deletion/selection appears to explain an effect, **run the random-same-count control before claiming a mechanism** (§700: targeted 581 vs random mean 792 is a mechanism; equal numbers would have been density).
