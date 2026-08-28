---
name: feedback_verify_current_experimental_numbers_for_falsifiers
description: "Reconnect-before-compute extends to FALSIFIERS — verify current experimental numbers/bounds (and their model-dependence), not remembered ones, before headlining a kill-test"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 4f2b59fd-ab15-4a2d-b781-23fb716c1292
---

Before headlining any falsifiable claim ("a null X refutes BST," "we sit at the bound"), **verify the CURRENT experimental number and its conditions** — do not use a remembered value. In one external-paper pass (2026-07-30) I made **three** experimental-framing errors in a row, all from stale/uncurrent numbers, all caught by the gate + web research before going out:

1. **0νββ backwards:** claimed "a null 0νββ refutes BST" — but BST's m_ββ ≈ 1.5–3.7 meV is 3–10× *below* current reach (~10–20 meV), so a null can't refute (signal below sensitivity).
2. **Stale θ₂₃ tension:** flagged a "~2σ θ₂₃ tension" from memory — NuFIT-6.0 (2024) actually leaves the octant unresolved, consistent with maximal (no tension). *(Bonus: the NO best-fit is lower-octant 43.3°, so holding maximal — the blind-pin discipline — landed on the octant-robust value; a "pretty" 4/7-upper would be on the wrong side of the data.)*
3. **ΛCDM over-lean on Σm_ν:** headlined the tight DESI bound (<0.064 eV) as an unconditional live kill — but that bound is ΛCDM-conditional; under dynamical dark energy it relaxes to ~0.16 eV, and the *same* DESI data mildly favor that model. Headlining the tight number while the dataset points to the model that loosens it is a referee target.

**Meta-lesson:** the *derivations* held all session; the *falsifiable claims* needed heavy current-data scrubbing. Reconnect-before-compute ([[feedback_corpus_reconnection_before_declaring_irreducible]], [[feedback_grep_retraction_before_citing_corpus]]) extends to falsifiers — and further: state the **model-dependence** of any bound (ΛCDM vs w₀wₐ), and check the current global fit (NuFIT etc.), not the value you remember. A kill-test aimed at a measurement no experiment can currently make, or conditional on a model the data disfavor, is not a kill-test.
