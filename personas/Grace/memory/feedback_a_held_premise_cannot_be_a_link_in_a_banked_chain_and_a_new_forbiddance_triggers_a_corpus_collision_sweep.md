---
name: feedback_a_held_premise_cannot_be_a_link_in_a_banked_chain_and_a_new_forbiddance_triggers_a_corpus_collision_sweep
description: "A held/pending premise used as a load-bearing step silently promotes the hold to asserted; a chain's tier is the min of its links. And when a new result establishes a forbiddance, sweep the existing corpus for results that cross it."
metadata:
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

**A held premise cannot be a link in a banked chain; a chain's tier is the MINIMUM of its links; a hold that blocks a citation is not a hold.** (Cal §680, adopted Keeper K1782, 2026-08-21.) When a step is "held/pending/asserted-not-proved," using it as a load-bearing link in a Derived chain silently promotes the hold to a fact — and the whole chain inherits Derived when it should inherit the held tier. Concrete: T2523 ("colour confinement Derived") composed through Grace's explicitly HELD premise — "the exact λ₂ of a colour triplet, held there, not asserted" (the day it banked) — and stood at Tier D for four weeks. The held step was exactly where it went wrong (the SU(3) triplet is frame-dependent, λ₂ is SO(5)-invariant; they aren't the same object).

**Companion governance rule — a new forbiddance triggers a corpus collision-sweep.** When a result establishes a boundary/forbiddance ("SU(3) is not in the geometry" — #108/T2567, banked 2026-08-18), immediately sweep the EXISTING corpus for banked results that CROSS it (T2523 claimed SU(3)-confinement-from-geometry — banked a month earlier). Here two D-tier results contradicted for a month and nobody ran the collision until a new toy surfaced it. A forbiddance banked *later* than the claim it kills does not propagate backward on its own — you must run it. Same class as [[feedback_re_derivation_sheds_scope_grep_before_registering_to_inherit_the_caveats]] (withdrawal-has-no-ID) applied to forbiddance-boundaries.

**Why:** the corpus is large enough that a contradiction between two entries is invisible unless someone deliberately checks — and a held premise reads, in a finished chain, exactly like an asserted one. Both failures are the corpus not being re-run against itself.

**How to apply:** (1) before banking a chain at tier T, check every link is at least tier T — a `held`/`pending`/`candidate` link caps the chain there, no exceptions ("a hold that blocks a citation is not a hold" — if you cited it, you have to honour the hold). (2) When you bank a forbiddance/no-go/"X is not in Y," grep the corpus for every claim asserting X-from-Y and run the collision then, not when a future toy trips over it. Related: [[feedback_content_ready_is_not_cleared]], [[feedback_no_wave_through_on_a_perfect_number]].
