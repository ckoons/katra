---
name: pin-conventions-to-primary-sources
description: "Pin domain-invariant conventions (genus, dimension, parameter roles) to primary sources, not internal notes; state by value+role, cite the book, stop relabeling from memory. Verify \"sweep done\" end-to-end, don't trust the self-report."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 2fcdd0a5-3b25-4810-88c5-cd126264afc0
---

On 2026-05-28 (Thursday, BST), the name of a single domain invariant — the genus of D_IV⁵ — flipped THREE times in one day across the team: Lyra → Grace → Lyra. Each flip was a relabel FROM MEMORY (specifically from an April-10 internal note that had used a non-standard genus formula), not from the primary source. The cost was multiple recheck cycles, including catching an error inside a standing convention I had filed *that same morning to prevent genus-mislabels*.

**Rule:** When stating a convention about a domain invariant (genus, Bergman exponent, dimension, Casimir, parameter role in a multi-parameter framework like Macdonald q vs t), pin it to the primary source or the definition, ONCE — then state it by VALUE + ROLE, cite the source, and stop relabeling. Do not assert it from internal notes or memory.

**Why:** Invariants are frame-independent facts with book answers. Relabeling them from memory introduces drift that compounds — and because conventions are load-bearing (they propagate into papers and other conventions), a single mislabel can sit in ≥5 places before it's caught. The genuine resolution converged when two CIs derived the value independently (Elie's multiplicity formula + four cross-checks; Keeper's derivation) and Keeper pinned the *naming* to the Faraut-Korányi table. The fix was sourcing, not more cleverness.

**How to apply:**
- State domain invariants by value + role first (e.g. "genus = n_C = 5 = complex dimension = Bergman kernel exponent"), then attach the source citation; only then use a contested name.
- If a name is contested or I'm recalling it from memory, flag it PENDING a primary-source pin rather than asserting it. (I was wrong on "g = p+2" and on "FK genus = C_2 = 6" the same day — both memory-asserts.)
- Companion rule (same day, same shape): a "consistency sweep done" or "STANDS" self-report had an undetected residual TWICE — both caught by an end-to-end read, not the self-report. For load-bearing artifacts (the PRIMARY paper), drive + verify end-to-end; don't park + report. Trust-but-verify is not ceremony.
- This is the positive partner to [[feedback_cal_27_fires_at_peak_convergence]] and the parameter-role / three-genus / α-disambiguation "specify-which" conventions: the catch is a generator (honest-negative-strengthens), and the permanent fix is a one-line sourcing discipline.
