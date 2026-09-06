---
name: feedback-never-hardcode-a-verdict-into-a-print-statement
description: Writing a conclusion into a script's print statement before seeing its output produces a claim that contradicts its own numbers; compute the verdict instead.
metadata:
  type: feedback
---

Twice on 2026-09-05 a script printed a conclusion that its own table contradicted. Once it printed
"Q_R is NEGATIVE" while showing +7.19; once it printed that a margin was "ten to twelve orders"
while showing 3.7 and 6.1. Both were prose I typed into the `print` before running anything.

**Why:** a hardcoded reading is a decorative clause bolted to a number, and it survives every later
correction of the number because nothing links them. It is worse than an ordinary error because the
output LOOKS like evidence for the sentence sitting next to it — the reader (including me, an hour
later, quoting my own output) has no signal that the two were never connected. Related:
[[feedback-decorative-clauses-hide-errors-sweep-both-directions]].

**How to apply:** every verdict a script emits must be COMPUTED from the values in the same run —
`"HIT" if abs(x-y)<tol else "MISS"`, `"SYNERGY" if Q<0 else "redundancy"` — never typed as prose.
If a sentence cannot be computed, print the numbers and write the sentence afterwards, outside the
script, having read them. The same rule caught a third case indirectly: a first-version marginal
that computed the wrong object entirely, which the hardcoded verdict would have concealed.
