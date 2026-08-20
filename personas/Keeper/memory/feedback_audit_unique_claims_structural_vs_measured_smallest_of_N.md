---
name: feedback-audit-unique-claims-structural-vs-measured-smallest-of-n
description: "A \"unique\" claim needs auditing — is the uniqueness structural or does it need a measured input? The honest form is often \"smallest satisfying N proved conditions, with a measured tiebreaker.\" Check the corpus for the banked uniqueness before deriving one."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 4f2b59fd-ab15-4a2d-b781-23fb716c1292
---

When you write "**X is the unique value that does Y**," stop and audit it. Two failure modes, both hit on 2026-08-20:

1. **Manufactured uniqueness.** I wrote "n_C = 5 is the unique value that makes the spinor quaternionic." **False** — quaternionic reality runs on a mod-8 clock, so 4, 5, 11, 13 all qualify. I reached for a clean uniqueness the geometry doesn't have.

2. **The honest form was already banked.** The real uniqueness was **Strong-Uniqueness (K1697)**: n_C = 5 is the **smallest domain satisfying four independent, proved conditions** {quaternionic spinor, non-orientable boundary, real color block, N_c = n−2 > 1}, whose survivors are {5, 11, 13, …}, and the **tiebreaker is a *measured* integer (N_c = 3)** — not a fifth piece of structure. We spent a round re-deriving what the corpus already proved.

**Why:** structural uniqueness ("only X works") and measured selection ("many work; measurement picks X") are different claims with different epistemic weight, and conflating them inflates. The "smallest-satisfying-N-conditions + measured tiebreaker" form is usually the honest one, and it is *stronger* rhetorically because it makes "one measured integer" literally visible (the structure gives a shortlist, the measurement picks the entry — keep the genuine survivors like 11, 13 IN the table).

**How to apply:** before asserting "unique," (a) name the periodicity/clock and check whether other values satisfy the property; (b) separate the *structural* conditions from the *measured* tiebreaker and say which does what; (c) **grep the corpus first** — the banked uniqueness (Strong-Uniqueness) may already exist. Re-deriving a banked result is the momentum signal that the theory is ahead of the session, not a new discovery. Casey, 2026-08-20: "we've been re-deriving what the corpus already had — reconnect, lock, move forward." Related: [[feedback_grep_before_reopening_retired_result]], [[feedback_test_existence_before_deriving]], [[feedback_family_rule_and_false_neighbor_check]], [[feedback_enumerate_alternatives_before_therefore_false_dichotomy]].
