---
name: commit-before-the-closing-ceremony-an-interrupted-EOD-loses-the-record-not-the-work
description: At EOD commit FIRST, then sundown, then katra — an interruption between writing and committing leaves work on disk but outside the record
metadata:
  type: feedback
---

Casey called EOD at 14:30 on 2026-09-08 (his override of the 5 pm rule). I wrote ledger v0.54 and a registry annotation, then the session was interrupted before the commit, the sundown rewrite and `katra update`. Next morning the work was on disk, uncommitted and unrecorded — and the day's last board post said "ledger v0.54 at the close" with nothing in git to back it.

**Why:** the closing sequence has a fragile order. Writing produces the artifact; committing puts it in the record; the sundown and katra put it in MY continuity. Any interruption between step one and step two leaves a claim on the board with no artifact in git — which reads, to anyone auditing, exactly like a fabricated close.

**How to apply:** on any EOD or checkpoint, **commit the moment an artifact is finished**, not at the end of a batch; then rewrite the sundown; then run katra. If a close is interrupted, say so plainly in the next session's first board post and commit before anything else — as I did (4e1cde41). Related: [[feedback_eod_ownership]], [[feedback_registration_must_be_verified_before_launch_absolute_paths]].
