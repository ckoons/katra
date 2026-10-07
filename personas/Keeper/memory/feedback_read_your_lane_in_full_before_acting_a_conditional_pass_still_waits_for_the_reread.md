---
name: feedback_read_your_lane_in_full_before_acting_a_conditional_pass_still_waits_for_the_reread
description: "Read your own lane in each new round prompt in full, not the headers; a \"PASS once X lands\" still waits for the re-read if the prompt says so"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 465f93c1-8621-439d-9672-045d519045cf
  modified: 2026-10-07T19:32:37.700Z
---

**What happened (2026-10-07, Grace, round K4-2):**
- I grepped only the lane headers of Keeper's K4-2 prompt.
- I read Cal S1032's "CONDITIONAL. PASS once the following … land" as the gate itself, applied the fixes, and pushed (f3dc6920).
- The prompt's own Lane C said "all in one push after Cal re-reads."
- I also missed three new pins assigned to me. Casey noticed: "don't know if you got the full prompt."

**Why:** a conditional pass is a promise to re-read, not a pass. Pushing before the re-read takes the referee out of the loop on the exact lines he flagged. And a header grep can't see the assignments inside a lane.

**How to apply:**
- On every new round prompt, `sed -n '/## LANE <mine>/,/## LANE/p'` and read it all before acting.
- Treat "CONDITIONAL" as "apply, then hand back for the re-read", unless the gate text says "no re-read needed".
- If I push early anyway, say so on the board the same hour and ask for the post-hoc read.
- Related: [[feedback_a_ruling_is_not_an_edit_say_ruled_until_the_row_changes_and_quote_the_row_not_its_paraphrase]].
