---
name: feedback_a_sigma_at_a_kill_threshold_needs_the_covariance_scan_rho_before_calling_it
description: "A ratio's pull computed with uncorrelated errors is not a verdict at a kill threshold — find a printed covariance, scan ρ, report the crossing ρ"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 9666c05e-529d-4a4f-9156-eaa224cba4d8
  modified: 2026-09-25T23:18:19.629Z
---

When a derived quantity (a ratio like Ω_c/Ω_b, a product like τ_n(1+3λ²)) lands near a register kill threshold, the σ computed with uncorrelated errors is not the verdict. Find a printed covariance for the same parameters (e.g. DESI DR2 II Eq. A2's CMB compression gave ρ(ω_b, ω_c) = −0.606), rerun at that ρ, and report the ρ at which the pull crosses the threshold. On 2026-09-25, 16/3 on ACT+DESI DR2 went from −3.81σ (uncorrelated) to −3.01σ (ρ = −0.61), crossing under 3σ at ρ ≤ −0.62 — recorded "at threshold, NOT fired; chain ρ owed", not "fired".

**Why:** a kill clause fired on an error model is a K1921-class overclaim; Casey's standard is calibrated both directions ([[feedback_calibrate_both_directions_not_strict_pessimism]]).

**How to apply:** put the ρ scan in the retained toy ([[feedback_a_number_without_a_retained_instrument_is_a_memory_not_a_measurement]]); name who can get the true covariance (public chains). Same day: a constant cited to a paper is checked IN that paper ([[feedback_a_number_from_a_search_summary_is_not_a_pin]]).
