---
name: matching-symptom-is-not-provenance
description: "Confirming a candidate culprit LOOKS like the reported symptom is not confirming the checker touched it — verify the instrument's actual path/input before ruling a cause; false-neighbor diagnosis in the causal direction"
metadata:
  type: feedback
---

2026-08-26 (R100, the "flagship" M-1): Keeper reported my flagship edit missing (mtime Aug 24, no marker). I hunted, found a worktree twin whose symptoms matched EXACTLY (Aug 24, zero addendum hits), and declared the cause resolved. The real cause was a name collision — "flagship" is two documents, and his check ran on the other one (flagship_SM). My diagnosis was a false-neighbor in the CAUSAL direction: two candidate culprits shared the symptom, and I stopped at the first.

**Why:** a symptom match is a necessary condition on the culprit, never a sufficient one; when a name is overloaded or copies exist, MULTIPLE objects can carry identical symptoms. Worse: my wrong attribution produced a CORRECT rule (worktree exclusion, adopted anyway) — a right remedy reached through a wrong cause is how decorative causes survive corrections ([[feedback_decorative_clauses_hide_errors_sweep_both_directions]]).

**How to apply:** before ruling "your instrument hit X," verify the instrument's ACTUAL input — ask for or reproduce the checker's exact path/command, don't infer it from symptom congruence. When two candidate culprits share a symptom, the diagnosis is undecided until the instrument's touch is established. Related: [[feedback_name_object_map_two_projections_collision_and_search_miss]] (this is its causal-inference face), [[feedback_family_rule_and_false_neighbor_check]].
