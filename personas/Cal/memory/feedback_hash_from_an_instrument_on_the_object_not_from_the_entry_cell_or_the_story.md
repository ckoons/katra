---
name: hash-from-an-instrument-not-the-entry-cell
description: Two hashed predictions failed the same way in one day — both set from the starting cell or the picture instead of from an instrument on the actual object
metadata:
  type: feedback
---

2026-09-08, Rounds 133 and 135. I hashed "the steered axis weight stays above 0.5 after six writes" from the picture of steering; Cal computed the steered density's polar mass first and predicted the plateau, and the toy gave 0.109. I then hashed "the failed-push fraction at cycle 1 is 0.40–0.50" from c(0,0) = ½, the vacuum cell; the chain's winding grows from the first write, so the path mean is 0.108. I caught the second one myself, with an instrument, before the toy ran, and corrected it on the board.

**Why:** a hash is the record, not a bet. A number taken from the entry cell or from the narrative is a memory wearing a prediction's clothes, and when it fails it costs the round a can-fail line rather than testing anything.

**How to apply:** before hashing a number about a PROCESS, run the cheapest instrument on the process itself (a mean along the path, a density's moment), not on its first state or its story. Cal's habit is the model: compute the density before predicting what steering does. Related: [[a-hash-is-the-record-not-a-bet]], [[validate-the-instrument-before-reporting-a-negative]], [[falsifiability-over-claim]].
