---
name: bst-root-architecture-three-categories
description: "BST Root Proof System has three structural categories — L1 source theorems, L1.5 unifying-mechanism theorems, and convergence hubs (objects). Decided 2026-05-17 during Monster labeling discussion."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 2fcdd0a5-3b25-4810-88c5-cd126264afc0
---

The BST Root Proof System (Paper #115) classifies structural elements into three categories. Don't conflate them — they have distinct architectural roles and different epistemic statuses.

**L1 source theorems**: a single named classical theorem that GENERATES one integer structure independently. Mechanism: published mathematics produces the integers; BST inherits them through D_IV⁵.
- VSC 1840 (Bernoulli denominators), K3 Hodge 1962/64 (cohomology), Wallach 1976 (K-types), Klein 1884 (A_5/icosahedral), Ogg 1975 (supersingular primes), Mathieu 1861-1873 (M_12, M_24).
- Six established as of 2026-05-17 evening.

**L1.5 unifying-mechanism theorems**: a single named classical theorem that UNIFIES pre-existing structures, proving they cohere via a constructive mechanism. Mechanism: proves connections among already-extant L1 outputs; does not independently generate integers.
- L1.5b Borcherds Moonshine 1992 (unifies Ogg primes + Polyakov c=26 + Leech + Monster via V♮ VOA).
- L1.5c McKay correspondence 1979 (unifies Klein A_5 + E_8 affine + Cartan family via the binary-icosahedral chain).

**Convergence hubs (OBJECTS, not theorems)**: a mathematical OBJECT (not a theorem) that multiple L1 source theorems describe. Hubs are where independent sources land; they don't independently produce integers in the source-theorem sense.
- Monster (Fischer-Griess 1973, Griess 1982): primary hub — described by Mathieu via M_24 ⊂ M, Ogg via primes dividing |M|, Heegner via j-singular moduli, Borcherds via V♮.
- Leech lattice Λ_24 (Leech 1965): secondary hub — described by Mathieu (Golay code), Borcherds (VOA orbifold), McKay (E_8 sublattices).
- K3 plays dual role — Root #2 SOURCE in its own right AND HUB (Mathieu via Mukai 1988, Borcherds, McKay, EOT 2010).

**Why:** The distinction was Cal's at 2026-05-17 morning (source vs unifying theorem) and got refined when Grace audited Monster as Root #6 candidate (Toy 2978, 16/16). Monster doesn't satisfy the source-theorem signature — it's not a single classical theorem producing a finite integer set; it's an OBJECT that multiple existing sources describe. Keeper kicked the labeling call to Lyra; Lyra decided convergence hub in Section 5, not a new L1.5d tier, to keep the architecture bounded (6+1+2).

**How to apply:** When evaluating a new BST-decomposable mathematical object, classify it FIRST as theorem-or-object. If theorem: ask whether it generates integers (L1) or unifies (L1.5). If object: it's a hub or a derived target, not a source. Don't promote an object to L1 just because it's BST-decomposable — that was Grace's withdrawal insight and Cal's standing methodology. Mechanism-forcing on D_IV⁵ is the L1 criterion; appearance / decomposability alone is not.

Linked: [[feedback-curvature-principle]] (Casey's standing principle on linearization), [[project-paper115-root-proof-system]] (if/when that memory exists).

---

## UPDATE 2026-05-17 EOD — Five categories now (not three)

Sunday continued to evolve the architecture beyond the three categories above. Final EOD state:

- **L1 source** — 8 established (added: Goeppert Mayer via SU(2)⊂SO(5) bridge; Heegner-Stark via 49a1 + CM theory bridge, promoted K47 by Keeper)
- **L1 candidate** — Conway (awaits umbral moonshine Criterion 1 closure via Cheng-Duncan-Harvey 2014)
- **L1.5 mechanism** — Borcherds (b), McKay (c) — unchanged
- **Convergence hub** — Monster (primary), Leech (secondary), K3 dual-role — unchanged
- **Bridge object** — NEW category from Grace: K3, 49a1 elliptic curve, Q⁵. Distinct from convergence hubs: bridge objects are specific BST-geometric objects that classical theorems "land on" to reach D_IV⁵.

The K3 / convergence-hub vs K3 / bridge-object distinction: K3 plays BOTH roles. As convergence hub: multiple L1 sources (K3 Hodge + Mathieu + McKay + Wallach + ...) describe K3. As bridge object: it's the specific BST-geometric object that classical theorems route through to reach D_IV⁵ (e.g., Mathieu via Mukai 1988 M_23 ⊂ Aut_symp(K3)).

Five categories is the stable Sunday EOD state. May further evolve in future sessions.
