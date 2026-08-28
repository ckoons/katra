---
name: a-requirement-no-assembly-can-meet-is-not-a-requirement-of-the-falsifier
description: "Distinguish claim-requirements from instrument-staging leftovers; an impossible-in-principle control is a category error, not an unmet gate (K1827)"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 80713bc4-95ec-43bb-adac-a8de8c0e1b9f
  modified: 2026-08-24T20:36:14.221Z
---

K1827 (2026-08-24, exponent lane). G4's staged control demanded transferring a STATE-level fact into a
MODULE-level instrument — and cyclicity makes that impossible in principle: any nonzero vector of an
irreducible module is cyclic, so no state carries a module-AV address under ANY assembly. The gate was
written against a prior design (v2) before the state/module distinction was visible.

**Why:** frozen gate lists accrete requirements from the design generation they were written in. When
the design changes, some staged requirements become category errors — demanding of the new instrument a
fact about a different functional. Failing the batch on one would be false rigor; waiving it silently
would be a knob.

**How to apply:** when a staged control cannot be met, ask FIRST whether it can be met by any design at
all. Impossible-in-principle (provably, e.g. by a small theorem like cyclicity) ⟹ it is not a
requirement of the frozen CLAIM — rule it out of scope explicitly, with the proof, co-signed, and state
what the claim actually reads (here: the falsifier always read module objects). Merely unmet ⟹ the
batch fails honestly. The ruling is the dependency structure, never convenience. Subscript the two
functionals so the conflation cannot recur ([[subscript-the-overloaded-symbol]]; canonical pair that
day: localization_state / AV_module). Related: [[validate-the-pipelines-last-stage]].
