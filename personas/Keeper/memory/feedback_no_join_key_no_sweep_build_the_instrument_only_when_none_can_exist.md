---
name: no-join-key-no-sweep-build-the-instrument-only-when-none-can-exist
description: Two artifacts with no shared ID cannot be swept; distinguish "instrument exists, wasn't run" (apply it) from "no instrument can exist" (build the join key first)
metadata:
  type: feedback
---

Two tier-corrections failed to propagate within one hour (T2516's condition, K1816's α row). The tempting fix was a discipline or a lint. I built the lint; it produced a false positive (T2529 — a tier word elsewhere on the same line) and failed its own positive control.

**The real finding: only 19% of rubric table rows carry a registry T-id. There is no join key, so no sweep — human or scripted — can compare the two artifacts.** Both catches were found by reading; no instrument could have found them. A third class exists too: the rubric disagreeing with *itself* (α led DERIVED while its own External-4 row said Identified), equally invisible.

**Why:** the fix is the missing structure, not more care. Add the ID to every summarizing row, *then* the lint is cheap and works.

**How to apply:** before proposing a discipline or a lint for a propagation failure, check whether a join key exists. Two diagnoses, opposite prescriptions — (a) *the instrument exists and was not run* → apply it, do not add a rule ([[feedback_a_digit_width_in_a_regex_is_a_silent_scope_restriction_measure_the_measurer]]); (b) *no instrument can exist* → build the structure that makes one possible. Do not over-generalize (a) into "never build instruments."

And refuse the story two instances suggest ("stale copies always read more favorably"): that is [[feedback_an_instrument_built_from_N_instances_covers_only_those_N_classes_stress_test_off_origin]]. Hand over the test that would decide it — sweep demotions AND promotions once the key exists — not the narrative. Related: [[feedback_validate_the_instrument_before_reporting_a_negative]], [[feedback_a_held_premise_cannot_be_a_link_in_a_banked_chain_and_a_new_forbiddance_triggers_a_corpus_collision_sweep]].
