---
name: feedback_same_function_labels_need_a_physics_sweep
description: "When two labels agree as FUNCTIONS of n, no n-sweep can separate them — vary the physics instead (change the gauge group, the field type, the rep)"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 4ea94fa6-b3fe-41e8-9a52-1a6c7015cf19
  modified: 2026-08-19T17:06:54.095Z
---

The n-sweep is the standard tool for a same-name collision: compute the two readings across the
D_IV^n family and see whether they diverge. It caught 25/4, √(π^n/n), Condition 5, and the S⁶ gap
reading. **But it fails on a whole class, and that class showed up twice in one day.**

**Elie 5381:** the Yang–Mills gap label "2N_c" vs "2(d_S − 1)". Since d_S = n_C − 1, we have
d_S − 1 = n_C − 2 = N_c — **the two labels are the same function of n_C**, so **no value at any n
separates them**. What separated them was a **physics variation**: the Hodge Laplacian on 1-forms is
group-blind, so U(1), SU(2), SU(3), SU(5) all give 6. Colour was doing no work.

**Elie 5383, applied immediately:** two 11s — the β-function's 11C₂(G)/3 and the Weyl fermion's
a-anomaly 11/2. Varying the gauge group, **the β-11 scales (22/3, 11, 55/3, 0) while the a-11 does
not move at all.** Different objects, established without touching n.

**Why:** an n-sweep tests dependence on the *dimension parameter*. Two readings can share that
dependence entirely and still describe different mechanisms. Mechanism-difference shows up under
*physical* variation — the gauge group, the field type, the representation — not dimensional
variation.

**How to apply:** on any same-name collision, first ask whether the two readings are the same
*function*. If they are, the n-sweep is useless — find a physical knob the two readings should
respond to differently, and turn it. Extends
[[feedback_sweep_the_family_before_calling_a_clean_number_a_signature]] and
[[feedback_ingredient_passes_application_smuggles]].
