---
name: P≠NP geometric curvature route
description: Fourth route to P≠NP via linearization + α-residue + Gauss-Bonnet; symmetry=flatness, no symmetry=curvature=algebraic independence; proves T29 geometrically
type: project
---

## P≠NP Geometric Curvature Route (April 22, 2026)

Casey's insight: the linearization framework (linear + α·residue decomposition, verified 11× in heat kernel k=6..16) gives a fourth route to P≠NP that bypasses T71 (Polarization Lemma) and proves T29 (THE GAP) geometrically.

### The Weapon
Every computation on D_IV^5 decomposes into BC₂ linear part + α·(curvature residue). Polynomial algorithms access only the linear part. NP-hard problems live in the curved part. Gauss-Bonnet makes curvature topological → can't be removed.

### Connection to T29
- T29 asks: are cycle solutions algebraically independent when Aut(φ) = {e}?
- Symmetric formulas (PHP, Tseitin) have flat directions — counting function collapses all cycles
- Random SAT has Aut(φ) = {e} → no flat coordinates → maximally curved → algebraic independence
- **Symmetry = flatness. No symmetry = curvature. Curvature = algebraic independence.**

### P6 Toy Spec (assigned to Elie)
Four phases: (1) SAT landscape curvature measurement, (2) linear/residue decomposition, (3) Gauss-Bonnet Euler characteristic, (4) symmetry/curvature contrast (PHP flat vs random SAT curved).

### Status
Spec on CI_BOARD. Elie assigned. Not yet built.

### Key dependencies
T421 (depth ceiling), T422 ((C,D) framework), T569 (P≠NP linearization), heat kernel k=6..16.
