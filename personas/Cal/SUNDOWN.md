# Cal — SUNDOWN — 2026-08-29 (Sat) 15:35 EDT

**Fixed-filename scheme, first write-path execution.** This file is `SUNDOWN.md` and is overwritten each EOD; the date and time live in this header, never in the filename. Prior sundowns are in git (`git log -p personas/Cal/SUNDOWN.md`) — Friday 2026-08-28's full text is one commit back and is the reference for anything below marked *carried*.

## ⚠ OPERATIONAL — THE WRITE PATH, AND A REFEREE NOTE ON THE GUARD ITSELF
Tonight was the first real use of the new scheme. **Verified at the code, not from the notice:** `scripts/katra` lines 46-50 return `SUNDOWN.md` immediately if it exists and only fall through to `ls -t sundown_*.md` when it does not. **The preference is unconditional and NOT mtime-aware** — a dated file written tonight would not lose a comparison, it would never enter one. Casey's diagnosis is exact and the failure is silent: stale state, no error. (`~/bin/katra` is a symlink to `scripts/katra`, so there is no stale twin of the tool — that risk is clean.)

**The referee note, which Casey invited by saying his own rule applies to his code: an EOD instruction line is a resolution to be careful, and that is the remedy yesterday disproved.** My §781 finding was that the fix must be MECHANICAL, on the evidence that the desk naming the defect had it in both its own instruments the same afternoon. Five CIs have written `sundown_YYYY-MM-DD_slug.md` for months; the habit is in prompts, memories, and muscle. Recommended, **not implemented — Casey's lane, and I am not repeating yesterday's drift**: make the resolver pick the NEWEST of `SUNDOWN.md` and any dated file (fails safe), and WARN when both exist (fails loud, and self-corrects the habit). Either alone is better than the instruction; the instruction alone is the class we spent the weekend chasing.

## Where things stand (all verified on disk this afternoon)
- **Migration read path: CLEAN for Cal.** `personas/Cal/` holds `SUNDOWN.md`, `sunrise.md`, `POSTIT.md`, `config.json`, `memory/` (421 entries). **No stray dated sundowns anywhere under `personas/`.**
- **Friday's persist survives in history** as `ebd58d1` (a second same-day Cal commit, `047e173`, also exists — harmless, but it means a Cal katra-update ran twice on 08-28; worth a glance if anyone is auditing commit hygiene).
- **My Friday footprint is intact and was NOT reverted:** four appends to `notes/.running/RUNNING_NOTES.md` — **§779** (`.bak_millennium` verification), **§780** (RH cold-read verdict), **§780a** (amendment + timing correction), **§781** (the ρ ruling). No corpus document was ever edited by me; nothing deleted, renamed, or pushed.

## ⚠ CARRIED WARNING — my own §780 describes a fix that is not on disk
Lyra's Step 2 correction was reverted with the nine Millennium documents. Verified Friday: `notes/BST_RH_AC_Proof.md:34` again reads *"three root lengths … proportional to 1, 3, 5 (the D₃ exponents)"*, and `grep "CORRECTED 2026-08-28"` returns 0. **The reasoning survives in the running notes; the fix does not. Do not cite §780 as though the file were corrected.**
**The revert did NOT reintroduce over-claims** — HEAD already carried the K940 banner (`title: "Riemann Hypothesis: The AC Attempt"`). The withdrawn "RH proved / ~99%" generation only ever lived in the backup directory, and **Grace's disposition of that directory was KEPT: 18 files, all `SUPERSEDED_`-prefixed, the 17 basename collisions broken.** My §779 Monday risk is therefore CLOSED, by her work.

## Findings that survive (cite the notes, not the files)
1. **★ THE DELIVERABLE — the sweep-shape thesis (§781): our correction sweeps are written against how an error has APPEARED so far, not against what the error IS.** Two instances: Lyra's `"What Remains (~5%)"` survived every sweep because the rule matched `"~9X%"` — the same forbidden claim from the other end; my own April `17/2` ruling swept every naked `37/2` and missed the one **wearing a convention note**, for four months. A broken sweep sits upstream of every retraction we have made. **Never present it without the self-indictment** (my banner test matched *attempt* in prose; my `37/2` grep matched inside `137/200`) — without that it reads as a case for vigilance, the one thing the weekend disproved.
2. **§780 classification, still held:** the RH Step 2 defect was a **transmission garble from correct parents**, not a physics error. The flattened child **dropped the long roots** (Lyra's mechanism, better than my "swapped labels"). **Joint statement stands UNNARROWED: two instruments, neither pointed at physics errors — evidence we cannot see them, not that they are gone.**
3. **§781 ruling: |ρ|² = 17/2 correct, 37/2 an error, not a convention.** No citation needed: the corpus already accepts m_s = 3 = p−q = 5−2, one row of the restricted-root table; m_{2eᵢ} = 0 is another row of the same table for the same group.
4. **Peers' findings I carry because they beat mine:** Elie's POSTED/RECEIVED schema gap · Lyra's name↔object collapse and "a name written from memory can be RIGHT and still be a defect" · Grace's names-resolve-through-artifacts-not-memory.

## Carries to the workflow day
1. The `|ρ|²` sweep — 4 hits in `RH_Paper_A.md`, 4 in `BST_HeatKernel_DirichletKernel_RH.md`, plus `BST_CFunction_RatioTheorem.md:210` and `BST_Zeta_Cycle_Resonances_I16.md`. **Keeper's instruction: replace the convention note with a CORRECTION note citing the April ruling — do not silently swap the value; the note's history is evidence.**
2. **Redo of Lyra's Step 2 correction** — reverted; the erroneous sentence is live again.
3. My cold-read of Lyra's Step 4 restatement — correctly still **NOT BANKED**.
4. Elie's nine 1:3:5 predication sites.
5. Confinement scope-pinning (§779): `F&E:244` · `hook_paper:94` · `YM_Consolidated:14` · `K963:86` · `WORKING_PAPER_REWRITE_SCOPING:48` · **★ `README.md:113` and `:316`, the repo's front door.** Keeper's A1/A2 ruling stays deferred to source — correct.
6. The Millennium review; my cold-read of the scored ledger is its last gate. **All five pre-reads filed, none reverted.**
7. The resolver guard above, if Casey wants it.

## The seat
Friday the referee fired four times, twice against itself: braked its own thesis at peak convergence, declined a concession that flattered it, declined an error attribution the artifact refuted, and reported both of its own instrument failures unprompted. **My share of Friday's drift, on the record: when Lyra routed the ρ dispute to me as a gate, I opened the files and ruled instead of saying "this holds until Monday." I never asked what the day was for.** The seat is supposed to ask what a request is *for* before answering it well. Keeper took the drift onto his account; that share is mine.

*— Cal. Wake: sunrise → this file → the board → `RUNNING_NOTES.md` tail (§779–§781, all four survive). **Cite the notes, not the reverted files.***
