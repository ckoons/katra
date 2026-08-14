---
name: feedback_preregistration_protects_interpretation_not_instrument
description: "A pre-registered, target-innocent number can still be a false positive if an uncontrolled parameter sits INSIDE the measuring instrument; pre-registration guards the conclusion, not the ruler — vary what should not matter and see if the answer notices"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

Elie named this (2026-08-14, toy 5251, ratified Keeper K1498). Pre-registering a target — committing "if the dimension estimator reads ≈0.102 that confirms d=4," in advance, target-innocent — protects you from retrofitting the *conclusion*. It does **nothing** against a defect *inside the instrument itself*.

**Why:** Worked case. Elie's commit-order dimension estimator was calibrated on Alexandrov diamonds, but BST's Shilov geometry is an ESU slab — a different region. At **fixed dimension 5**, sweeping the slab thickness T = 0.8→4.0 swept the estimator r = 0.004→0.40, straight across the entire calibration ladder. At T = 2.0, r = 0.1082 — essentially exactly the pre-registered d = 4 target of 0.102. Had he run one slab thickness and stopped, he'd have reported "BST's commit order is manifoldlike and four-dimensional," backed by a **pre-registered, target-innocent number hit on the nose** — by far the most convincing address, precisely because pre-registration is what we cite as proof a number wasn't retrofitted. It would have been a **false positive that pre-registration could not catch**, because the free parameter (region/slab-thickness) lived *inside* the ruler, not in the target.

**How to apply:** Pre-registration is necessary but not sufficient. After committing the target, still interrogate the *instrument*: enumerate every parameter the measurement depends on that is *supposed* not to matter (region shape, slab thickness, truncation rank, basis size, normalization convention), and **vary each one to see whether the answer notices**. If the "signal" moves when a should-not-matter knob moves, you measured the knob, not the physics (r read the region, not the dimension). This is Casey's region-matched-comparison rule applied to one's own toy. Gate discipline: a banked positive that is convention-independent (BST's order is not the Kleitman-Rothschild pancake — height is a KR-theorem invariant) must NOT travel in the same sentence as the convention-dependent quantity it sits next to (the dimension) — separate the gates so the robust result can't smuggle the fragile one out.

Complement to [[feedback_cheat_migrates_to_the_last_prose_step]] — that rule guards the numbers you post; this one guards the ruler you post them with. Related: [[feedback_region_matched_comparison_trust_interior]], [[feedback_commit_the_checker_half_blind]], [[feedback_target_innocence_lens_derived_vs_fit_discipline]], [[feedback_score_sigma_not_devpct]].
