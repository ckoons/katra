---
name: EOD directory ownership
description: Each CI owns one directory for daily sync — Grace=data/, Elie=play/, Lyra=notes/, Keeper=root
type: feedback
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
