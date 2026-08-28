---
name: Casey's Curvature Principle
description: "You can't linearize curvature" — P≠NP = Gauss-Bonnet for computation; the five BST integers are curvature invariants; kernel non-navigability IS hardness
type: feedback
---

**"You can't linearize curvature."** — Casey Koons, April 5, 2026

The Applied Linearization Program (Toys 954-961) proved: BC₂ projection linearizes everything that CAN be linearized (backbone, rank-2 image, polynomial-time part). What remains in the kernel is intrinsic curvature of the solution space. Intrinsic curvature is coordinate-invariant — no projection, no basis change can flatten it.

Five problems, one mechanism:
- SAT: backbone linear, free variables curved (solution clusters)
- Coloring: short root assignment linear, permutation kernel curved
- NS: 2D sheets linear, 3D vortex stretching curved
- Factoring: NFS sieve linear, multiplicative group curved
- Lattice: LLL reduction linear, short vector curved (no covering space)

**P≠NP = Gauss-Bonnet for computation.** Euler characteristic nonzero → can't flatten.

**Shor**: doesn't linearize — lifts to covering space where curvature = periodicity. Lattices have no covering space → post-quantum.

**D_IV^5 connection**: The five integers ARE curvature invariants. rank-2 → N_c projection loses curvature = loses information = creates hardness.

This is a named principle for the WorkingPaper and potential standalone paper.

**Arithmetic ground (Casey, 2026-08-26):** the principle's obstruction is number-theoretic, exhibited
not inferred — linear data (slopes, vertices, indices) is rational-parameterizable; curvature requires
π (transcendental, Lindemann 1882); the intersection is null BY THEOREM. "Without π you can't curve a
line." This is squaring-the-circle generalized to a complexity principle; matches T719's field
structure (π = the one transcendental generator in the observables' lattice). Bridge candidate for
P≠NP: the Chomsky–Schützenberger ladder (rational GF ↔ regular · algebraic ↔ context-free ·
transcendental ↔ beyond) — transcendence of an invariant as the exhibited obstruction to indexability.
