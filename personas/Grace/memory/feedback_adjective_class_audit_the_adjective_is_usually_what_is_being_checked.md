---
name: feedback-adjective-class-audit-the-adjective-is-usually-what-is-being-checked
description: "When a check's SUBJECT carries a loaded adjective ('forced', 'derived', 'our', 'the complex', 'the reductive'), the adjective is usually the very property the check is supposed to verify — so the question answers itself and carries no information. Strip the adjective, name the bare object, verify the property separately."
metadata:
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

**The discipline (Cal §590/§593, named 2026-08-18, K1682):** before running a check, look at its subject noun-phrase. If it carries a loaded adjective — *forced*, *derived*, *our*, *the complex color-3*, *the reductive embedding* — that adjective is usually the exact thing the check was meant to establish, smuggled into the description so the question answers itself.

**Two instances, one round apart:**
- §590: "is `SO(V₁₂)` **reductive**?" — `SO(V₁₂)` is reductive *by definition* (it's the orthogonal group of a 3-subspace, fixing the complement). The pin returns "reductive" carrying zero decisional information.
- §593: "is `M₃ = End_ℂ` of our **forced complex** color-3?" — the *complexification* (real-3 → complex-3, via time's J) and the *forcing* are precisely the open steps, buried in the noun phrase. (Rescued only because `M₃(ℂ) = End(V₁₂ color)` turned out to be an actual banked theorem, T2551 — a theorem, not an adjective.)

**The same-name-different-object family (Cal, 5 instances in 9 days — a recurring class):** `SO(2)`-charge (§570, time's J vs electroweak charge), "Rac" (§566), `E` vs `2E` (§576), "the color 3" (§595, real Jordan block vs complex tangent), and **`C₂` = the integer 6 vs the function `2n−4`** (§603, K1692). ★ The most dangerous variant, worth its own trigger: **a symbol that is a *number* in one place and a *function of n* in another — both readings true, just not the same object.** Whenever a "constant" appears in an equation you solve *for n*, check it isn't itself n-dependent (K1690a's "unique crossing at n=5" was circular exactly this way: it used `C₂=n+1`, true only at 5, then solved for the n where its own input holds). Pin which object a shared name denotes *before* adjudicating any claim built on it.

**Why / how to apply:** rewrite every check so the subject is the *bare* object and the contested property is the *predicate*, tested on its own. "Is SO(V₁₂) reductive?" → "what is the fixed-subspace dimension of the color SO(3) on the vector-5?" "Is M₃ the End of our forced complex color-3?" → "cite the theorem that forces M₃, and separately, is the complex structure intrinsic?" The useful form of a check is one that **can fail**; an adjective-laden subject usually cannot. Related: [[feedback_empty_confirmation_cant_fail_test_and_circular_hunt_mechanism]] (a construction-guaranteed test proves nothing), [[feedback_ingredient_passes_application_smuggles]] (a derived ingredient's identity with the target presupposes the answer), [[feedback_preregistration_protects_interpretation_not_instrument]], [[feedback_cheat_migrates_to_the_last_prose_step]] (the trim/condition is where the imposed step hides — e.g. unimodularity trimming u(3)→su(3)).
