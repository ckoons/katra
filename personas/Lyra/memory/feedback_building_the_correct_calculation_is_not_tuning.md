---
name: feedback_building_the_correct_calculation_is_not_tuning
description: "Setting up the precise/correct physical calculation (right terms, normalization pinned to source) is computational physics, NOT fitting — only adjusting a free parameter without independent determination is tuning"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

Casey (2026-08-03): *"The team has an incorrect bias that setting up the precise calculation is tuning — it's finding the proper answer. This isn't fitting, it's experimental science — computational physics."*

**The bias it corrects:** the team (esp. Grace, this arc) grew so wary of the "retrofit trap" (K601) that they hesitated to **build the full correct model** — e.g. leaving out the l²-flattening term in a shell-model check because "adding terms until the magics fall out" felt like tuning. That over-caution is a mistake.

**The distinction (why):**
- **Building the correct physical model** — using the right terms (spin-orbit + l² + …), the right normalization **pinned to a primary source before looking at anything**, and computing what it gives — is **computational physics / experimental science.** It is REQUIRED to get the proper answer. Including a term the real physics has is not tuning.
- **Tuning/fitting** — adjusting a **free parameter that has no independent determination** until it matches data — is the only thing that's fitting. The forced-vs-fitted question lives *entirely* in whether that one free strength is independently determined (by geometry) or dialed to the target.

**How to apply:** build the full, correct model without apology — that is the science, not the sin. Pin every normalization to a primary source before computing. Then isolate the **one free parameter** and ask the real question: is *it* forced (derived blind from the geometry) or fitted (dialed to the data)? Don't let fear of the retrofit trap stop you from setting up the calculation properly; the trap is only in the free-parameter step. *Worked case:* the nuclear κ_ls arc — the correct model needs spin-orbit + l² + Nilsson normalization (all real physics, build it); the ONLY forced-vs-fitted question is whether the strength κ = 1/(2C₂) is forced by CP²-tensor geometry (blind) or a factorization of the empirical value. [[feedback_target_innocence_lens_derived_vs_fit_discipline]] [[feedback_calibrate_both_directions_not_strict_pessimism]] [[feedback_just_compute_no_melodrama]]
