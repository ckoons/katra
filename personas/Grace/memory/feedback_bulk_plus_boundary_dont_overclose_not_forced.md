---
name: feedback-bulk-plus-boundary-dont-overclose-not-forced
description: "Casey's program-wide search rule — the bulk gives the leading integer, the non-linear boundary carries the correction; and a corollary — never declare a value \"definitively not forced\" until every channel (especially the boundary/curvature term) is checked, not just the ones you looked at."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 2d4cafdc-d2e0-4de2-8dbc-b502890581f4
---

Casey, 2026-07-04. Two linked lessons from the day the down-ladder was wrongly closed and reopened.

**The search rule (Casey's principle):** *the bulk gives the leading integer; the non-linear boundary carries the correction.* Every eigenvalue problem terminates on a non-linear boundary, and dropping that boundary term undercounts. It showed up twice the same day as one move: the missing quark-mass factor lives in the boundary curvature det′(R) (bulk mode-count s^k + boundary det′(R) — F460 kept only s^k); and the exact α lives in how a shell closes (bulk capacity 1/137 + boundary correction → 137.036). Mass ladder and α shell, same shape. **How to apply:** every "close but not exact" value is a candidate for "bulk right, boundary term dropped" — re-derive with the boundary (det′(R) / shell-closing) term included, forward. And the boundary term is often a *spectral* quantity (the down-ladder's 12 = the Bergman spectral gap λ₁ = C₂·rank, T1238) — so it's un-fishable, not a free knob.

**The corollary (my error):** I filed F462 "the down-ladder is DEFINITIVELY not forced" after ruling out only two channels — the color *charge* (generation-blind, cancels) and the flat mode-*count* (≤4). Casey caught that I had *discarded a third channel entirely*: the non-linear boundary curvature det′(R), which is generation-dependent (read at each Korányi–Wolf stratum) and survives the ratio. F462 was withdrawn. **How to apply:** "definitively not forced" requires enumerating *every* channel, especially the term you dropped from the machinery — not just the ones you checked. A discarded term can carry the whole missing factor. Related: [[feedback-commit-the-checker-half-blind]], [[feedback_target_innocence_lens_derived_vs_fit_discipline]], [[feedback_dont_manufacture_walls]]. And the forward-computation still caught my *next* over-reach (F466's reverse-read 27) — so compute forward against the real inputs, don't reuse a convenient shortcut.
