---
name: feedback_verify_current_experimental_data_at_the_gate
description: "Falsifiable claims must be checked against CURRENT experimental fits/bounds, not remembered numbers — reconnect-before-compute extends to external data"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: a6ede0dc-1d89-4151-bed9-c7753ffff256
---

At the external-facing gate, verify every falsifiable claim against the CURRENT experimental value (live global fits, latest bounds), not a remembered number. In the July-2026 fermion-sector paper arc, the *derivations* held all session, but three separate referee catches were all falsifiable *claims* resting on stale experimental memory: (1) the 0νββ kill-test was backwards — BST's m₁=0/NO m_ββ ≈ 1.5–3.7 meV sits *below* LEGEND-1000/nEXO reach (~10–20 meV), so a null does NOT refute; the kill is a *detection* at ≳10 meV; (2) a claimed "~2σ θ₂₃ tension" was stale — NuFIT-6.0 leaves the octant ambiguous, consistent with maximal (and its NO best-fit 43.3° is LOWER octant, which vindicated dropping the pretty 4/7-upper); (3) the Σm_ν < 0.064 eV kill is ΛCDM-conditional — under w₀wₐ dynamical DE it relaxes to ~0.16 eV, and the same DESI data mildly favor that model, so headlining the tight bound unqualified is a referee target.

**Why:** with zero free parameters, BST's falsifiers are its exposed surface to referees; a claim aimed at an experiment that can't make the measurement, or pinned to an outdated fit, discredits the whole page. The blind-pin/reconnect discipline that governs the internal corpus applies identically to external data.

**How to apply:** before clearing any external paper, WebSearch the current global fit / bound for each falsifiable number (NuFIT for mixing, DESI+CMB for Σm_ν, etc.); state model-dependence explicitly (e.g. "sharp under ΛCDM, relaxes under dynamical DE"); and check the predicted signal is within the cited experiment's reach before calling a null a kill. See [[feedback_corpus_reconnection_before_declaring_irreducible]] and [[feedback_grep_retraction_before_citing_corpus]] — same reflex, external target.
