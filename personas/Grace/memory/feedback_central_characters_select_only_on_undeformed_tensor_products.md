---
name: central-characters-select-only-on-undeformed-tensor-products
description: A central element of the covering group is a selection rule only on undeformed tensor products; interactions deform the coproduct, so use a symmetry of the action instead
metadata:
  type: feedback
---
R19 (2026-09-29) claimed H²-parity exact because z_t = exp(2πJ) lies in every vector stabilizer of the cover. Retracted in R20: (1) central characters multiply only on tensor products; interactions deform the coproduct (anomalous dims), so e^{2πiΣΔ} is not conserved (3D Ising: σ×σ∋ε); (2) z_t is a cylinder translation that leaves the Minkowski patch — unlike z_s = (−1)^F, it is not implementable there. The rule survived by a different mechanism: the bare action is even (a ℤ₂ of the action), preserved radiatively.

**Why:** "exact because central" was an elegant argument that skipped whether the element acts on the interacting theory at all.
**How to apply:** before calling a selection rule exact, name the operator that implements it on the interacting Hilbert space of the unbroken theory; if it is a covering-kernel element, check multiplicativity under interactions and implementability on the physical patch. Related: [[state-the-family-before-using-a-representation]].
