---
name: feedback_decorative_clauses_hide_errors_sweep_both_directions
description: A wrong justification bolted to a correct number survives every correction because nobody audits it; sweep corrections in header AND body
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

**Load-bearing claims get checked; decorative ones get copied.** (Cal §649, adopted K1752.) A false *justification* attached to a *correct number* is where an error hides, because the number's correctness vouches for the whole sentence and no one audits the gloss. Concrete case: the internal-SM paper's lead said "1.58 bits (…only the spinor moves)" — the 1.58 is true standalone (it's log₂3, the spinor-block figure), but "only the spinor moves" is false (at n=4 the color block also moves → ℂ, which is why the *family* census is 2.00). The false clause did no work, so it rode along with a correct number through three separate corrections — including two of the auditor's own. Keeper had graded that body GREEN (K1750), verifying the numbers and skipping the decoration.

**Why:** correctness is contagious in the reader's eye — a right number launders the sentence it sits in. Decorative clauses are added as "helpful gloss," never re-derived, so they're the last place a retracted claim survives.

**How to apply:** (1) when you copy a number, AUDIT its attached justification or DROP it — a non-load-bearing gloss is a liability, not a help. (2) The metadata/correction sweep runs in BOTH directions — header AND body (extends [[feedback_content_ready_is_not_cleared]] and the §648 header-sweep rule): a correction that lands only in the header is the same defect in the opposite coat, and HARDER to catch because the header now looks right. A correction is not closed until every citation of the claim — in either layer, and in every metadata home (paper header, registry row, graph node, filename) — is consistent. Related: [[feedback_cheat_migrates_to_the_last_prose_step]] (the uncorrected claim migrates to the layer least read), [[feedback_commit_the_checker_half_blind]] (the checker's own glosses need an external eye — Cal's rode three corrections).
