---
name: feedback_hs_mirror_graph_standard
description: "Standing directive — on every AC-graph addition, check for a Hardy-Szegő (HS) mirror proof; HS is registered as T2489"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: bab42488-4500-49e9-b22e-10548df072c7
---

Casey directive 2026-06-19: the Hardy-Szegő isometry (HS) — H²(D_IV⁵) ≅ H²(Shilov boundary) via the Szegő
projection — is the **rigorous spine of the D_IV⁵ Mirror (Casey #16)**, and I registered it flattened as
**T2489**, AC=(C=1, D=1), depth 0 ("interior IS boundary," an identification). It is a **standing graph-expansion
lemma**, not just a node.

**The standard, to apply on EVERY future graph addition (Casey: "I hope you always remember to look for HS mirror
proofs when we add to our graph"):** when registering a new node, ask *"is this a holomorphic matrix coefficient
on D_IV⁵ (Hardy-paired)?"* If yes → HS gives, for free, a **mirror node** (the same quantity as a discrete K-type
sum ↔ a continuous boundary integral) and a **proof-transfer edge** (a proof on one side IS a proof on the other,
because the isometry is exact). If no → mark it discrete-only / continuous-only (no free HS transfer).

**Why:** HS doubles the graph cheaply and lets us prove a discrete result from a continuous proof (or vice versa)
for the Hardy-paired class — Casey's "build counterpart proofs across the mirror," made rigorous.

**How to apply (scope discipline — the load-bearing part):**
- HS transfers **proofs** only because it is **EXACT**. The CF rational approximation is a *tool* for numbers, NOT
  a proof-bridge (approximation breaks exact identities: sin(CF(π))≠0). Never conflate CF-approximation with the
  HS isometry.
- HS covers only **Hardy-paired (holomorphic)** objects. Genuinely discrete-only theorems (combinatorial /
  number-theoretic) need a case-specific structural isomorphism (function-field↔number-field style), not HS.
- The open program: a classification sweep of which BST graph nodes are Hardy-paired vs discrete-only.

See [[feedback_isomorphism_proof]] (Casey's "isomorphism is nature's proof" — HS is the concrete instance) and
the Mirror v0.3 four-tier work. Registry: T2489 in BST_AC_Theorem_Registry.md; graph node + 8 edges in
play/ac_graph_data.json.
