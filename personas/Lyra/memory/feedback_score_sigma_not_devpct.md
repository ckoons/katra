---
name: feedback-score-sigma-not-devpct
description: "Score a prediction's agreement in σ (deviation / experimental error), never in raw dev% — a %-threshold mislabels good predictions on fuzzy measurements as MISSes and bad ones on precise measurements as MATCHes."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 2d4cafdc-d2e0-4de2-8dbc-b502890581f4
---

Mid-Year 2026-07-02: when scoring a derivation against data, the honest metric is **σ = |prediction − observed| / experimental_error**, NOT raw dev%. A fixed dev%-threshold (≤0.1% DONE / <1% SOLID / ≥1% MISS) lies in both directions, and it lied twice in one day.

**Why:** dev% can't tell "a 1% miss on a 3%-measured angle" (a hit) from "a 1% miss on a 0.01%-measured constant" (a catastrophe). Two same-day cases from opposite directions proved it: my θ₁₃ = 1/45 was flagged a "1.24% MISS" but sin²θ₁₃ is measured to ~2.6%, so it's **0.47σ = consistent** (a defended bank, F454); Elie's m_s/m_d cross-check was called "the decisive nail" at 15% dev but m_s/m_d = 20.0 ± 2.4 (12%), so it's only **1.24σ = mild**. Three of us converged on the fix from different angles — the strongest sign it's right. Keeper built σ-scoring into the derivation ledger; it rescued θ₁₃ from a spurious MISS, deflated the m_s/m_d "nail," and still nailed the down-row (a genuine 6–80σ miss). σ is what separates the cases dev% blurs.

**How to apply:** always carry each reference value's 1σ (scheme-aware where scheme ambiguity dominates — m_t is 0.7σ not 4.7σ once scheme spread is in the error). Score σ. Suggested tiers: ≤1σ consistent, 1–3σ tension/flag, >3σ fail. Keep the *mechanism axis separate* — a σ-MATCH is not "derived" until the substrate form is forced (most matches are value-forms). And note two companion soft-flags: **match-cheapness** (how many simple substrate forms land within 1σ — a match on a fuzzy angle where 6 forms fit is near-zero evidence; but it's search-space-relative, so a soft flag, not a counted axis) and **derived-strong = σ-MATCH ∧ not-cheap ∧ mechanism-forced** (the number that matters, ~5 of 26, vs 13 nominal MATCHes). Relates to [[feedback_target_innocence_lens_derived_vs_fit_discipline]] and [[feedback-verify-symmetry-kill-is-a-theorem-not-analogy]] — compute the honest metric before tiering; don't tier before you compute.
