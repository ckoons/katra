---
name: graph-registration-three-seams
description: Registering a theorem into the AC graph fails at three seams (node present, edge keyed to the file's schema, edge oriented input→consequence) and at the counter (claim file, not counter, is what the other writer sees)
metadata:
  type: feedback
---

On 2026-09-02 Grace found three drifts in one morning, all hers, none a wrong theorem: T2584 registered but not
noded; 7 edges in `play/ac_theorem_graph.json` written with `{from,to}` keys bolted onto a copied
`{source:"T1",target:"T7"}` (native schema is source/target — every consumer saw seven bogus T1→T7 edges);
10 edges reversed (the convention is from = INPUT → to = CONSEQUENCE; bulk 4,424:2,419, `/theorem register`
spec `{"from": dep, "to": new}`). Same day two counter collisions (T2585/86 with Lyra, toy 5602 with Elie).

**Why:** Keeper's start-of-day instrument checks max-id only; nothing checks edge keys or arrows. The counter
file is read once and used twice when two CIs claim in the same minute.

**How to apply:** after every registration, print the edges touching the new node and read the arrows aloud
against the artifact's "uses" sentence; validate both graph files' edge-key schema; read `.next_theorem` /
`.next_toy` immediately before EVERY claim and write a claim file — the earlier timestamp keeps the id, the
later pair renumbers (K1837). See [[reference_next_counters]], [[feedback_clock_drift_is_continuous_run_date_before_every_written_timestamp]].
