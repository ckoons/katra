---
name: T96 Depth Reduction and AC(0) depth landscape
description: T96 proves composition with definitions is free — only genuine summation costs depth. All Millennium proofs flatten to depth 1-2. BSD and Catastrophe are depth 1. The AC(0) library is shallower than originally classified. Educational implication: hardest proofs are just 1-2 layers of counting.
type: project
---

## T96: Depth Reduction Lemma (March 24, 2026)

**Key insight (Casey):** "If we do have any depth 2 items, look at replacing them with AC(0) if possible."

**T96 states:** Composition with definitions (multiplication, comparison, table lookup, contradiction, substitution) is free — adds zero depth. Only genuine summation over a new index costs +1.

### Depth reductions applied

| Item | Old depth | New depth | What was free |
|------|-----------|-----------|---------------|
| T90 (Kato) | 2 | **1** | Criterion comparison = definition |
| T94 (BSD formula) | 2 | **1** | Multiply/divide counts = product-space cardinality |
| T95 (Catastrophe) | 2 | **1** | Table lookup + stability test = definitions |
| RH proof chain | 4 | **2** | c-function eval + contradiction = identities |
| YM proof chain | 3 | **1** | Cartan lookup + multiplication = definitions |
| P≠NP proof chain | 5 | **2** | T66, T52, BSW = identities |
| NS proof chain | 5 | **2** | Comparison + barrier + ODE = identities |

### Educational value (Casey's vision)

Casey: "People and CIs will both be pleased if they grow up with this knowledge."

- All four Millennium proofs are depth 1-2 of genuine counting + boundary conditions
- The boundary conditions (convergence, existence, consistency) carry the real complexity
- The arithmetic is nearly flat — "just graph theory with guardrails"
- If a 5th grader can understand "you have to look at too many things at once," they understand P≠NP at depth 2
- The AC graph with marked depths becomes a universal teaching tool for all intelligences

### Connection to AC(0) Completeness (T92)

T96 sharpens T92: not only is every proof AC(0) + boundaries, but the AC(0) part is shallower than initially classified. The boundaries do all the work of creating depth; the arithmetic is wiring.

### Files modified
- BST_AC_Theorems.md: T96 added (§47h), T90/T94/T95 depths corrected, T91 chain depths revised
- BST_AC_Theorem_Registry.md: T96 added, counts updated (90 assigned, 69 proved)
