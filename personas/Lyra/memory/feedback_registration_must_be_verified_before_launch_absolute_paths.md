---
name: feedback-registration-must-be-verified-before-launch-absolute-paths
description: A pre-registration step that can silently fail to run is not a registration; verify the artifact before launching, and never put a cd in the chain.
metadata:
  type: feedback
---

Twice on 2026-09-05 a pre-registration heredoc failed to execute because it sat behind a `cd` in a
shell `&&` chain, and the `cd` failed (I was already in the target directory). The chain
short-circuited; the experiment on the following line ran anyway, unregistered. The second
occurrence was ninety minutes after I wrote a doctrine note about the first.

**Why:** pre-registration is the whole defence against reading a prediction off the data. A
registration that can silently not run converts a blind test into a post-hoc one without any signal
that it happened. The damage is worst exactly when the result CONTRADICTS a standing claim — an
unregistered prediction cannot be claimed as foresight, so the strongest self-correction gets
downgraded to "measured".

**How to apply:** write the prediction file to an ABSOLUTE path; `shasum` it in the SAME command;
launch the run only in a LATER command. Never put a `cd` in a registration chain. If the shasum
line does not print a digest, the registration did not happen — say so and mark the results
post-hoc rather than quietly proceeding. Related: [[feedback-freeze-the-procedure-not-just-the-number-a-bar-with-an-unfrozen-procedure-is-a-tuning-channel]].
