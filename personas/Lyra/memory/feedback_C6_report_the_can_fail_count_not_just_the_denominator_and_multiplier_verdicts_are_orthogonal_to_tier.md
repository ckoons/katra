---
name: feedback_C6_report_the_can_fail_count_not_just_the_denominator_and_multiplier_verdicts_are_orthogonal_to_tier
description: "C6 catches look-elsewhere but not a denominator padded with checks that can't fail — report how many of N could have come out otherwise; and a multiplier (counting) verdict is orthogonal to a tier verdict — a narrowing needn't void a multiplier-1 result"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

Two rules banked together (Cal §681, K1783, 2026-08-21):

**(1) C6 refinement — report the denominator AND the can-fail count.** [[feedback_selection_honesty_full_sweep_or_preregister_the_atlas_is_look_elsewhere_by_design]] catches look-elsewhere (many trials), but it does NOT catch a denominator **padded with checks that structurally cannot fail.** Concrete: an "8/8 pass" sweep looked strong, but six of the eight rows assigned λ₂=0→free to colour-singlets — correct, and not one could have come out otherwise; only two rows *could* have failed, and those two turned out void. So the informative fraction was 2/8, then 0/6 — the "8/8" hid it. **Rule: report the denominator AND how many of the N checks could have come out otherwise.** A pass-rate over can't-fail checks is padding that reads as rigour and adds none (same family as the empty-confirmation / construction-guaranteed discipline).

**(2) Multiplier verdicts are orthogonal to tier verdicts.** A *multiplier* verdict (how many independent times a result is derived — a counting judgment) is a different axis from a *tier* verdict (how strongly / whether it's derived). When a narrowing lands (a claim's predicate changes what it is *about* — e.g. §680 changed "confinement" from SU(3)-colour to the two-row sector), the instinct is to void everything downstream. Resist it: a narrowing that changes what a predicate is *about* does NOT void a multiplier-1 *counting* verdict (Grace's reconciliation survived §680 untouched — §680 changed the predicate, not how many times it's derived). **Separate the two axes before voiding downstream** — check each downstream item for genuine *dependency*, not mere adjacency ([[feedback_ingredient_passes_application_smuggles]] companion: T2529 was adjacency-not-dependency and stood).

**How to apply:** when reporting a sweep, write "k/N, of which m could have failed" — if m is small, the sweep proved little. When a banked claim narrows, sort its downstream into dependency (re-scope) vs adjacency (untouched) and counting (multiplier, orthogonal) vs tier — don't reflexively void the lot.
