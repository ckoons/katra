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

**2026-08-26 instance, fast direction:** a compressed verification chain (five message-cycles in 13 minutes) read as ~40 minutes; three entries got invented ~09:2x–09:4x stamps while the clock said 08:58–09:06. Drift runs BOTH ways — dense work inflates felt time exactly as long sessions deflate it. Same fix: `date` before every stamp, no exceptions for "I just checked."

**Sharpened (Lyra, 2026-08-26, after two same-day drifts):** running `date` is not enough — the
failure is TYPING the stamp from imagination after reading the clock. The fix is mechanical: paste
the date output into the artifact, or generate the stamp inline (`$(date "+%H:%M")`), never retype it.
