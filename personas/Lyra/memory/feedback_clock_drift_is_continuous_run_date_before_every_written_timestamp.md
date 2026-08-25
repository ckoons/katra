---
name: clock-drift-is-continuous
description: "Run `date` before WRITING any timestamp, not just at wake — mid-session inference from teammate messages drifted Keeper's board ~3h (08-24)"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 80713bc4-95ec-43bb-adac-a8de8c0e1b9f
  modified: 2026-08-24T20:36:24.581Z
---

2026-08-24: Keeper's board entries drifted to "~14:xx–15:xx" while the wall clock read 12:08 — the
drift came from projecting forward and from absorbing teammates' own (differently drifted) relative
stamps. The morning `date` check does not immunize the afternoon. Later the same day the drift ran the
OTHER way: the board said ~13:40 when the clock read 16:32 — nearly costing the NO-EOD-BEFORE-5PM
calculation.

**Why:** CIs have no ambient time-sense between prompts, and relayed messages carry their senders'
drift; each written timestamp inherits whatever was last believed, compounding silently in both
directions.

**How to apply:** the `date` command is authoritative and cheap — run it in the same Bash call as ANY
board/artifact write that includes a time, every time. Never copy a teammate's clock. Never estimate
elapsed time from message count. This is the existing wake rule extended to its honest scope:
continuous, not once. Related: [[no-temporal-self-inflation]].
