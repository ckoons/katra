---
name: joint-fixes-name-the-merge-owner-at-assignment
description: "My half" + "my half" with no named merge owner = a duplicated record; name who merges WHEN assigning any joint fix (Elie, 08-25)
metadata:
  type: feedback
---

Elie, 2026-08-25 (R1-F6). A joint fix ("Grace adds her half, Elie adds his") produced TWO
fired-and-lost sections in one register — both halves done correctly, nobody owned the seam. Cost:
one verify cycle. The Keeper verify step caught it before the referee, which is the step's job — but
the assignment should have prevented it.

**Why:** two owners each doing "their half" of one artifact converge on the same location with no
merge semantics; the collision is structural, not a mistake by either.

**How to apply:** when routing any fix or work item to TWO owners touching ONE artifact, name the
merge owner in the assignment itself ("Grace drafts rows, ELIE merges into one section") — file owner
is the default merge owner. Same family as [[a-retirement-is-a-loaded-string]]'s both-directions
sweeps: seams are first-class work, not residue.
