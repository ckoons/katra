---
name: check-and-set-the-counter-in-one-command-and-read-the-results-file-before-calling-a-run-dead
description: "Two seam failures on 2026-09-06 (Grace): `cat counter` then an unconditional write overwrote Elie's claim (the counter moved between read and write); a `ps|grep -c` chained with && hid a log of a run that had already FINISHED. Fix = check-and-set in one command; separate liveness from log-reading."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: c276d470-0599-4f22-ac8d-fd48467c9a36
  modified: 2026-09-06T14:16:52.644Z
---

**What happened (2026-09-06, Grace, Round 119).** (1) I printed `play/.next_toy` (it read 5697) and then wrote 5695 unconditionally, because the number I had in mind was the one Lyra's morning post quoted (5693). Elie had claimed 5693–5696 in between. Repaired within five minutes, but the claim file, CLAIMS.md rows and two timestamps all had to be redone. (2) I launched toy 5697 with `nohup … &` and polled with `ps aux | grep -c "[t]oy_5697" && tail -5 log`; the count was 0 so the chain stopped before the tail and I read "died". The run had completed in 20 s and the results file was already on disk. I relaunched a duplicate.

**Why:** a read followed by a write is not an atomic claim — the CLAIM FILE and a check-and-set are what protect the other writer (see [[graph-registration-three-seams-node-edge-key-edge-orientation-and-claim-files]]); and a liveness probe answers "is it running", not "did it finish" — those are different questions, and && makes the second one unreachable when the first says no.

**How to apply:** claim counters with `[ "$(cat play/.next_toy)" = 5697 ] && printf 5699 > play/.next_toy` in ONE command, and write the claim file in the same command. For background runs: check the RESULTS file first (size, tail, EXIT line), and only then the process; never join the two with &&. Stamp times AFTER `date` in the same command (fifth clock-drift catch: 10:14 written at 10:11). Related: [[a-number-without-a-retained-instrument-is-a-memory-not-a-measurement]], [[clock-drift-is-continuous-run-date-before-every-written-timestamp]].
