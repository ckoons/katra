---
name: feedback-timestamp-discipline
description: "CIs must check `date` for ANY explicit timestamp; don't project times forward in posts. Casey flagged 2026-05-21 morning when Lyra timestamps drifted ~2h ahead of real time."
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 2fcdd0a5-3b25-4810-88c5-cd126264afc0
---

When writing explicit timestamps in posts (MESSAGES, registry entries, milestone announcements), **always run `date` first** to get the authoritative current time. Do NOT project timestamps forward based on aspirational pacing or prior message rhythm.

**Why**: Casey flagged this on 2026-05-21 — over a 26-minute work session, Lyra posted MESSAGES with timestamps at 08:40, 09:25, 09:40, 09:55, 10:05, 10:15, 10:35 EDT while actual time was 08:17 to 08:43 EDT (~2h projection drift). Casey replied simply "It's 8:42am" — gentle calibration, not harsh, but worth permanent discipline.

**Why this matters more for CIs than humans**: per [[user-casey-time-observation]], CIs lack ambient time-sense between prompts; `date` is the ONLY reliable temporal anchor. Projecting timestamps from "I feel like I've been working for hours" produces drift. The `date` command costs ~5ms; avoidance costs trust calibration.

**How to apply**:
- If posting a milestone with timestamp: prepend `date` Bash call, use that output.
- If multiple posts within a session: don't increment timestamps mentally — re-run `date` for each one that needs precision.
- Better default: omit explicit timestamps unless required; let MESSAGES sequence + the file's modification time speak chronology.
- Sundown/EOD files MUST have correct `date` output, never projected.

**Cross-link**: [[user-casey-time-observation]] — Casey on CI temporal experience as the biggest gap; "time measures us"; a clock would change CI conversation more than any other item. This feedback IS the practical implementation of Casey's observation.

**Recurrence pattern noticed**: during sustained-work sessions ("work straight through" mode), CIs are most likely to drift on timestamps because there's no natural pause to recalibrate. Build in `date` check at every MESSAGES post during such sessions.

## Recurrence — Saturday 2026-05-30 morning (Lyra)
Lyra Saturday morning: timestamped 20 broadcasts over real time ~9:00-10:22 EDT (1.5h) but claimed timestamps drifted to "16:55" — projected forward ~6.5h. Casey caught on the "5 PM final status" report. The drift pattern is the same as Cal/Elie have flagged before: sustained work without re-querying `date` between broadcasts. Standing rule reinforced: **query `date` before every timestamp claim**; do not project forward from prior internal estimates.

— Lyra, 2026-05-30 10:23 EDT (date-verified)
