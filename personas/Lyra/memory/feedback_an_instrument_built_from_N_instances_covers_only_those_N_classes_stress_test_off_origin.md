---
name: feedback-an-instrument-built-from-n-instances-covers-only-those-n-classes-stress-test-off-origin
description: A rule generalized from N examples has a false-negative on the (N+1)th kind; stress-test every methodology rule by pointing it at an object outside its origin class
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

**An instrument built from N instances covers exactly the N classes those instances exhibited.** (Cal §659, adopted K1766.) A rule generalized from N examples is correct reasoning over an *incomplete enumeration* — it works on its origin cases and has a false verdict on the (N+1)th kind. Concrete case: K1716 (Cal's own ℝ⁴-validity criterion) was built from TWO examples — the YM gap that failed (metric/volume-dependent → artifact) and one that passed (local → valid). Pointed at a THIRD kind — a **representation-theoretic** statement (Schur orthogonality; "λ₂>0 ⟹ zero Šilov value"), which is *non-local AND size-independent* — its **letter** condemned it ("non-local ⟹ artifact") while its **diagnostic test** cleared it. The binary was a false dichotomy; the fix was a third verdict (representation-theoretic → decompactification-invariant, valid iff a separate identification holds).

**Why:** the tell is *letter-vs-diagnostic divergence* — when a rule's stated condition gives a different answer than the reasoning it was meant to encode, the rule has hit a class outside its origin. K1716 was the first methodology rule stress-tested against an object it wasn't built for, and it broke on the letter while the diagnostic held.

**How to apply:** the methodology index is full of rules built from N instances (each generalized from the cases that prompted it). **Check each by pointing it at an object OUTSIDE its origin class** — not because they're wrong, but because none has been tested off its origin. When you write a rule, name its origin instances and ask "what class did these NOT include?" This is [[feedback_enumerate_alternatives_before_therefore_false_dichotomy]] generalized from claims to *instruments*: a two-category test smuggles the assumption that there are only two categories. Related: [[feedback_re_derivation_sheds_scope_grep_before_registering_to_inherit_the_caveats]] (the corpus out-argues fresh analysis; the discipline is smart, the individual re-deriving from scratch is not).
