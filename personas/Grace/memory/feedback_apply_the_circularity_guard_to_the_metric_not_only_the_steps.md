---
name: feedback_apply_the_circularity_guard_to_the_metric_not_only_the_steps
description: "A descent metric defined as distance-to-the-goal-set presupposes the goal set is nonempty; the circularity guard must be applied to the METRIC/potential and its domain, not only to proof steps (four-color, K1835, 2026-09-02)"
metadata:
  type: feedback
---

**What happened (2026-09-02, K1835):** the four-color chain's last lemma, "some family word strictly
descends d_gate," where d_gate = distance to the τ ≤ 5 target set 𝒯(T,v). Lyra's pre-registration
found in two lines that 𝒯 ≠ ∅ ⟺ T is 4-colorable (Definition 5). So the metric is finite iff the
theorem holds; at a minimal counterexample d_gate ≡ ∞ and descent has no content. Three step-audits
(K1832/K1833/K1834) passed it because the assumption entered at the METRIC FREEZE, not at a step.
54/54 and 1,801/1,801 were an empty confirmation: every real instance satisfies the goal, so the
failing class is empty in nature.

**Why:** the circularity guard ("no reduction step may assume the whole graph 4-colorable") was
applied to steps and never to the potential. A potential's DOMAIN is a hypothesis. "Consumer grounds"
for freezing a metric are exactly where the conclusion hides. Species of
[[feedback_hunt_if_P_mechanism_must_not_be_the_assumption_that_produces_P]] and
[[feedback_cheat_migrates_to_the_last_prose_step]]: here it migrated into a definition.

**How to apply:** for every descent/potential/measure in a proof by induction or well-founded
descent: (1) state its domain and ask "is finiteness/nonemptiness of this equivalent to the theorem
on the object I am inducting on?"; (2) demand a TARGET-INNOCENT certificate — one that never names
an element of the goal set; (3) treat measured-universal descent on real instances as evidence for
the conditional theorem (goal-given) only, and say so. What survives such a finding is usually a
real algorithm theorem given the goal as input (here DGT); name it and bank it honestly. Sits with
[[feedback_empty_confirmation_cant_fail_test_and_circular_hunt_mechanism]] and
[[feedback_test_existence_before_deriving]].
