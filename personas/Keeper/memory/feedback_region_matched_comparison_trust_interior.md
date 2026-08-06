---
name: feedback_region_matched_comparison_trust_interior
description: "Casey's standing rule — σ/dev comparisons must be region-matched; trust interior/forced values; demote only for a named internal input, never a cross-region σ"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 2e845eb9-6620-413d-8c9c-e13468a49ba4
---

Casey's standing directive (2026-08-06, K1245): a σ/dev comparison is valid ONLY within one region — interior↔interior, boundary↔boundary, exterior↔exterior. An interior geometric computation compared in σ to an exterior measurement is a MISMATCH: the interior→exterior projection sits between them (the same gap as α's forced 1/137 vs the measured 1/137.036). Otherwise TRUST the interior/discrete value. Demote a tier ONLY for a reason internal to the region — a named INPUT (a value not forced by the geometry) — NEVER a cross-region σ. Exterior dev% (agreement) is CONFIRMATION, orthogonal to tier.

**Why:** I applied a cross-region σ as a tier signal — scored m_μ/m_e = (24/π²)⁶ at ~1580σ against the *exterior* muon mass and demoted it (toys 5098/5099). But (24/π²)⁶ is forced with no input (24 = N_c!·2^rank, exponent = n_C+1), so it's interior-Derived and the 0.003% is confirmation. I over-demoted in the *pessimistic* direction — the same failure as over-inflating, just mirrored. This refines [[feedback_score_sigma_not_devpct]] (score σ, but only within a region) and is the concrete case of [[feedback_calibrate_both_directions_not_strict_pessimism]].

**How to apply:** before scoring any BST value against experiment, ask which REGION each side lives in. Interior/forced prediction vs exterior measurement → do NOT σ-demote; report the dev% as confirmation and keep the tier at whatever the derivation earns. Demote only if the derivation carries a named input (e.g. m_τ/m_e = 49·71: 49 = g² forced, but 71 is an external prime with no geometric derivation → Structure-Derived). Projection-INVARIANT ratios (running cancels, e.g. Koide Q = rank/N_c) ARE legitimately exterior-comparable in σ. GUARD (Keeper): "trust interior" must not become a license to re-inflate fitted/input-dependent values back to Derived — named-input demotions stand.
