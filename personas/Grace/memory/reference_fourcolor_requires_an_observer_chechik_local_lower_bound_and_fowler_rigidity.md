---
name: reference_fourcolor_requires_an_observer_chechik_local_lower_bound_and_fowler_rigidity
description: "Casey's 'does four-color require an observer?' has a precise YES: Chechik 2018 (arXiv 1804.00137) — LOCAL 4-coloring of planar graphs needs Ω(n) rounds (rigidity: far vertices forced equal); Fowler 1998 — uniquely 4-colorable planar graphs = Apollonian networks; observer needed exactly at the chromatic boundary"
metadata:
  type: reference
---

**Theorem (imported):** any distributed LOCAL algorithm (deterministic or randomized) that properly 4-colors every
planar graph requires Ω(n) rounds — Chechik, "Optimal Distributed Coloring Algorithms for Planar Graphs in the
LOCAL model," arXiv 1804.00137 (2018). Mechanism: a planar family with two vertices at distance Ω(n) that must
share a color in every 4-coloring (rigidity). With more colors, polylog algorithms exist (same paper).
**Rigidity's source:** uniquely 4-colorable planar graphs are exactly the Apollonian networks / planar 3-trees
(Fowler 1998 thesis, Georgia Tech).

**Corpus reading (Keeper 2026-09-02):** "requires an observer" = "no bounded-radius rule"; it holds exactly at the
chromatic boundary χ = 4 and fails above it — [[feedback_deviations_locate_boundaries]] in theorem form. The 4 is
χ(planar) (Euler χ = 2 via Heawood), not a spacetime dimension. Casey's "peeking at the best vertex" in his
amoeba procedure is precisely the observer the LOCAL model forbids. Sequential/bounded-lookback version: UNPROVED
(measured lookback ≤ 4 on n ≤ 22 with a fixed BFS order). Status: imported anchor; cite, don't claim. Related:
[[project_fourcolor_one_word_lemma_and_dichotomy_tree]], [[user_casey_consciousness_theory]].
