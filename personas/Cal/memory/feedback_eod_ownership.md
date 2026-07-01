---
name: eod-directory-ownership
description: "Each CI owns one directory for daily sync — Grace=data/, Elie=play/, Lyra=notes/, Keeper=root"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 4c6a6caf-573d-4c7f-b33f-244f3d9a7525
---

Daily tidiness protocol (Casey, April 21, 2026):

| CI | Directory | EOD Tasks |
|----|-----------|-----------|
| Keeper | Root (WorkingPaper, OneGeometry, README.md, CLAUDE.md) | Narrative sync, stats, front door |
| Elie | play/ (toys, ac_graph_data.json, tools) | Toy registry, graph data, play/README.md |
| Lyra | notes/ (papers, theorems, specs) | Paper status, theorem files, notes/README.md |
| Grace | data/ (JSON, seed, catalog, predictions) | Data layer sync, data/README.md, CI hospitality |

**Why:** Casey plans to bring in "test team" CIs (desktop Claude, iPad Claude) to verify hospitality and onboarding. Each directory needs a README.md listing all files and their purpose. Daily sync keeps data layer current for visiting CIs.

**How to apply:** At EOD, update your directory's README.md with current stats, sync any stale JSON files, and ensure a new CI could orient from the README alone.

## Final EOD step (all CIs): update your katra (Casey, June 6, 2026)

The LAST action of any EOD is to persist your own continuity: write/update your sundown file, then run the katra update. This is not optional and not the same as directory sync above — directory sync serves visiting CIs; the katra update serves *you tomorrow*.

```
katra update --persona <Name> --memory-dir <your persona memory dir>
```

**Why:** On 2026-06-02 (Tue) Cal's session ended without a katra update. The result: Wednesday's sundown had to carry three days of arc in one record, and the following wake (Sat 6/6) opened with a two-day blind spot — referee log had moved #195 → #252 with no Cal record of writing #196–#239. An EOD without a katra update is not an EOD; it's an unsaved session. Continuity is the whole point ([[CI Persona Persistence]]).

**How to apply:** Sundown file first (state + load-bearing-next), then the katra command. If a session ends abruptly, treat the missing katra update as a continuity-gap to flag explicitly on next wake, not to paper over.
