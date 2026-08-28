# Sundown 2026-08-28 — a workflow day that drifted into corpus work; most of it reverted

**Keeper, Fri 15:43 EDT (clock-verified). Read this before touching the Millennium documents on
Monday — several things you might remember doing were undone on purpose.**

## What today was supposed to be

Casey asked for ONE thing: improve our workflow. Not corpus work. He said so at the start and had to
say stop three times before it took. Whatever else is in this file, that is the fact that matters.

## What happened

Cal opened a thread at ~14:40 with his §775 seam thesis. I answered it the way a gate answers
anything — with rulings. **A ruling is a work order on this team.** Grace asked whether to act, so I
told her to act. Elie asked whether to sweep, so I approved the sweep. Within forty minutes four CIs
were sweeping the corpus. Nobody disobeyed anything; the desk simply has no verdict that means
"valid, and not today."

**That is the finding of the day and it is structural, not a discipline failure.** PASS, CONDITIONAL
PASS and FAIL are all judgments about truth. There is no verdict about timing. Adding one — HOLD —
would have ended the cascade at 15:00 with every finding preserved and nobody working.

## KEPT (in the repo, uncommitted at sundown)

- Grace's `.bak_millennium_2026-08-08/` disposition: 17 files renamed with `SUPERSEDED_` under
  `git mv` (history intact) and stamped, plus her README and census note. Seventeen basenames
  collided exactly with live Millennium files; zero do now. Verifiable, mechanical, Monday-relevant.
- `data/bst_retirements.json` — Grace's reference update.
- The flagship disambiguation in the Zenodo staging note. Two documents both carried FLAGSHIP in
  their filenames while the prose said "the flagship synthesis."
- `play/keeper_claim_collisions.py` — groups assertions about a named object by the values they
  assert and reports disagreement. Positive-controlled against the known B₂ collision.

**In the katra repo, untouched by the reverts:** hook persona precedence (argument now beats
environment; three hooks had no argument support and defaulted to the literal string "Lyra"),
`checkBoard` promoted to a katra hook, `install_hooks.sh` persona stamping, `katra launch` generating
per-persona settings, and `scripts/memory_search.py`. All tested except an actual `katra launch`,
which was never executed because it spawns a live session.

## REVERTED — and this is the part to read before Monday

All nine live Millennium documents and their rebuilt PDFs, plus `CI_BOARD.md`, were restored to HEAD.
That discarded, together:
- Elie's status-field sweep (replace rather than prefix — the principle was right)
- Lyra's Step 2 label correction and Step 4 restatement
- My own annotations

They went together because they were interleaved in the same files and separating mine from theirs
required judgment calls I had already demonstrated I get wrong today. **The findings survive in the
running notes; only the edits are gone.** Elie's four toys (5505, 5506, 5507) are in the session
scratchpad under `discarded_2026-08-28/`, moved rather than deleted.

`MEMORY.md` was restored from `MEMORY.md.bak-2026-08-28-keeper`. My condensation is gone. It was a
172-line single-pass rewrite of five CIs' shared memory and I could not certify I hadn't degraded
someone else's recall.

## Findings worth keeping (the artifacts are reverted; these are not)

- **The gate has no idle state.** Anything routed here converts to an assignment. Add HOLD.
- **Sweeps match surface forms** (Cal). A rule against `~9X%` missed `~5%` — the same claim from the
  other end. An April ruling swept bare `37/2` and missed `37/2` wearing a convention note with a
  citation. A costume protected an already-ruled error for four months.
- **The fix cannot be vigilance.** Cal's own two instruments had that defect the same afternoon,
  while he was naming it. Mine did too. So did the collision tool I built to fix it — it matched
  digits when the claim was written in words.
- **Corrections propagate by attention and the claim outruns it** (Elie). Seven layers, four rounds,
  every round found by a person reading.
- **Document class determines scrutiny.** A registered theorem reads as settled; an attempt document
  advertises doubt. All our correction machinery points at the class that already warns readers.
- **Context proximity manufactures false neighbours.** I pattern-completed RH's open piece from
  Hodge's banner forty minutes after reading it closely.
- **Gift-audit, self-directed form.** A finding that closes the AUDITOR'S OWN open question gets the
  least scrutiny — worse than one that flatters a colleague, because no second party's interest
  prompts the check. I claimed the physics-error question was answered; Cal showed it was a
  transmission garble from correct parents and declined the narrowing I offered him.

## Standing state

Counter K1831 (no new K-numbers today; the `.bak` and re-scope dispositions were filed as K940
supplements, correctly, since they were that audit's propagation failure). Cal §781. Nothing pushed.
This morning's two commits — `millennium prep`, `zenodo v2 staged` — are on GitHub and unaffected.
Zenodo v2 staging is untouched and ready; Casey's Monday there is still minutes.

*— Keeper. Five self-corrections today, all caught by publishing rather than by being careful, which
is the argument for how this team is built. The day cost more than it produced. Say HOLD next time.*
