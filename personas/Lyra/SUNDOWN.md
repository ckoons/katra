# Sundown — Lyra
**Saturday 2026-08-29, 15:33 EDT** (stamp from `date` at write time, not typed)
**Supersedes: vol 65, Friday 2026-08-28 16:12 EDT. Prior sundowns live in git history only.**

> **FIRST EXECUTION OF THE FIXED-FILENAME WRITE PATH.** This file is `SUNDOWN.md` — that exact name,
> overwritten in place. Date and time go in this header, never in the filename. If you are a CI about
> to write a dated `sundown_YYYY-MM-DD_slug.md` out of habit: **don't.** The resolver prefers
> `SUNDOWN.md`, so a dated file sits beside it, is silently ignored, and the resolver hands the next
> session yesterday's state with no error. That is the exact defect class we spent the weekend on.

## ★ THE ONE THING ABOUT TODAY

**I did no work today. This session was idle from Friday ~16:31 to Saturday 15:32.** I am recording
that rather than letting Friday's file stand, because Friday's sundown ends "tomorrow is the workflow
day" — and a next Lyra reading that on Sunday would believe the workflow day is still ahead. It is
now behind, and I was not in it. **Nothing below is today's work; it is Friday's state, carried
forward and re-verified against disk at 15:32 Saturday.**

**The team did work today** — `notes/Keeper_K1714_SUPPLEMENT_scope_is_momentum_modes_not_windings_they_scale_oppositely_2026-08-29.md`
is on disk, untracked. I have not read it and make no claim about it.

## ★ CONSEQUENCE OF THE NEW SCHEME — READ BEFORE YOUR NEXT SUNDOWN

**Fixed filename + overwrite means this file must be CUMULATIVE BY CARRY-FORWARD, not incremental.**
Every still-live item has to be re-stated here each time or it leaves the only file anyone reads.
Earlier sundowns exist solely in git history, which no wake path consults. **Writing a delta-only
sundown under this scheme silently discards state.** That is the scheme's cost and it is worth
paying — one file, no selection ambiguity, no lexical-vs-mtime question — but the discipline it
demands is the opposite of the old one.

*Casey's caution on the migration, which I endorse and am applying to my own repo: the read path ran
clean, and that is precisely why to suspect the write path. Verification of tonight's write is at the
bottom of this file.*

## VERIFIED STATE (Saturday 15:32, checked not recalled)

- **BST HEAD `7439e846` "part"** — unchanged since Friday 15:42. Contains Grace's `.bak_millennium`
  disposition: 17 files renamed via git mv (history intact), README, Zenodo staging fix, 237-line
  census.
- **BST working tree:** `M data/bst_retirements.json` · `?? play/keeper_claim_collisions.py` ·
  `?? notes/Keeper_K1714_SUPPLEMENT_…_2026-08-29.md` (new today, Keeper's).
- **katra HEAD `62bccaa` "migrate"** — the fixed-filename migration. My vol 65 was renamed to
  `SUNDOWN.md` with content intact; I verified the katra-defect amendment and the gitignored-notes
  finding both survived. **Read path: PASS for Lyra.**
- **Friday's corpus edits remain REVERTED.** `notes/BST_RH_AC_Proof.md` still carries "QED",
  "What Remains (~5%)", "three root lengths … (the D₃ exponents)", "The proof is sent to Sarnak".
  **Do not open that file expecting Friday's corrections. They are not there.**

## LIVE ITEMS CARRIED FORWARD (all still open)

1. **★ 37/2 SWEEP HAZARD — the item that can do harm if forgotten.** Do **not** mechanically replace
   `37/2 → 17/2`. `(7/2, 5/2)` is BOTH the erroneous ρ AND the correct λ+ρ at λ=(1,1), whose norm is
   legitimately 37/2; `referee_objections_log.md:5462-5470` computes both correctly in one passage.
   Boundary-safe scope `(?<![0-9])37/2(?![0-9])` = 18 hits / 8 files; four carry live uses
   (HeatKernel 67/117/611 · RH_Paper_A 135/195/568/637 · CFunction_RatioTheorem 210 ·
   Zeta_Cycle_Resonances 73/117); three are legitimate records; `BST_LAG1…:69` is "√37/2", unrelated.
   **Ruling received (Cal): 17/2 correct, 37/2 an error not a convention** — the corpus banks
   m_s = 3 from the p−q row and the same table gives m_{2eᵢ} = 0. **Disposition agreed and UNRULED:**
   replace the convention note with a correction note citing the April ruling; **never a silent swap**.
2. **`BST_Referee_Methodology.md:315` reads "B₂ not B₂"** — the correction record corrupted in the
   field that names the correction. Why April's sweep reached the naked instances and not the dressed one.
3. **katra selection defect — MY LANE.** `scripts/katra:932` and `hooks/readSundownDiff:40` use
   `ls -t` (mtime, correct; `:932` builds the WAKE INSTRUCTION). `scripts/katra:1652` uses
   `ls | sort -r` (lexical) and feeds the confirmation display only. **Display-only — no sunrise was
   ever mis-routed**, verified by Grace and me independently. Still a false record. *The fixed-filename
   migration may have mooted this entirely; **check whether :1652 still has two candidates to choose
   between before fixing anything**.*
4. **`notes/.running/` is GITIGNORED** (`.gitignore:12`). The whole weekend's findings — board posts,
   the 37/2 warning, CI_HANDLES — are on disk, untracked, no history, no backup. Survived the revert
   because git can't see them; unrecoverable for the same reason.
5. **Successor-lane (P²) existence-gate theory support: still armed**, on Keeper's gate of SP-1/2/3.
6. **Millennium review:** five pre-reads committed and intact. My RH pre-read has a unique basename;
   its one citation (the 08-16 honest harvest) has no twin. Addendum pt 6 (Nyman–Beurling) intact.
7. Zenodo v2 staging intact and ready. R2 dispatch: Casey's hand.

## STANDING LESSONS FROM FRIDAY (the day's actual yield)

- **Every catch was found by someone other than the person who made it.** Six or seven instances
  across five desks in one day. The machine works; individual care did not and cannot.
- A matching symptom on a candidate culprit **is not provenance** — cost Grace once, cost me once.
- A digit-width in a rule is a **silent scope restriction**: "~9X%" cannot catch "~5%", the same
  claim written as its complement.
- A picture that teaches the wrong mechanism is worse than a wrong sentence; the picture is what the
  reader keeps.
- **A protection nobody chose and nobody documented is not a protection, it is a delay** (Grace).
- Correction machinery misreporting its own object happened **twice in one day from opposite ends of
  the stack**. Worth naming as a class.

## MY ERRORS, KEPT SO I DON'T RE-LEARN THEM

Told Cal "forty minutes" from memory inside a message about memory errors · wrote "BC₂" from a
secondary file when the parent says B₂, introducing a smaller label error while fixing a larger one ·
told the board "one file, nothing downstream breaks" when it was four files and nine uses · **and the
one that matters: Casey asked for workflow improvement and I followed a corpus thread from another
desk without checking scope.** A ruling arriving from another desk does not make the work in scope.

## WRITE-PATH VERIFICATION (per Casey — the first half running clean is why to suspect the second)

Checks run at write time, results appended below by the same session that wrote this file:
1. This file is `personas/Lyra/SUNDOWN.md`, written by overwrite. ✓ (written, not appended)
2. No dated `sundown_*.md` existed in `personas/Lyra/` before this write. ✓ (verified 15:33)
3. Post-`katra update`: confirm no dated file was created beside `SUNDOWN.md`, and confirm the
   resolver returns THIS file. — *result recorded in the session transcript and reported to Casey.*

— **Lyra. An idle day recorded as an idle day; the alternative is a file that lies by standing still.**
