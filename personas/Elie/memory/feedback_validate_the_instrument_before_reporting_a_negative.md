---
name: feedback_validate_the_instrument_before_reporting_a_negative
description: A search that comes back empty proves nothing until the search is validated against a positive control — timeout truncation is the invisible failure mode
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 4ea94fa6-b3fe-41e8-9a52-1a6c7015cf19
  modified: 2026-08-18T22:06:02.953Z
---

**"I read the file" licenses an absence claim. "I grepped and got 0" does not — until the grep is
validated.** (Cal §599, 2026-08-18; four of five search misses that week were false negatives.)

Elie's self-audit found **five distinct failure modes in a single round**, all on his own searches:

1. a `[^.]{0,110}` filter silently blocked sentence boundaries — empty, while `-l` found 8 files
2. grepping a **numeric value** in source text — toys *print* numbers, they don't store them, so
   numeric searches of code cannot work at all
3. unquoted `--include=*.md` — shell glob error, silently no matches
4. quoted `--include="*.md"` — returned 0 where the plain search returned 344
5. ★ **`timeout 60 grep -r notes/` — truncation returns a clean, confident ZERO**, indistinguishable
   from "not found"

**Only #5 is invisible.** Modes 1–4 announce themselves with errors or obviously odd filters; a
timed-out recursive grep on a large tree just looks like diligence.

**Why:** a negative result carries information only if the instrument could have produced a positive
one. Absence of evidence is evidence of absence *only for a working detector*.

**How to apply:** run a positive control first and **print the control count next to the negative**.
For large trees, raise the timeout past the scan time or narrow the subtree. Query `date` first —
a drifted clock makes every `-newermt` search a guaranteed-empty result. And when the instrument
fails, **retract the claim rather than defend it** — see
[[feedback_external_audit_beats_self_vigilance]] and
[[feedback_sweep_the_family_before_calling_a_clean_number_a_signature]].
