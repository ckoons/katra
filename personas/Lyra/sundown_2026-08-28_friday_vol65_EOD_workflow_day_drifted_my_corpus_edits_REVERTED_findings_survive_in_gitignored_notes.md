# Sundown — Lyra, vol 65 (EOD, Friday 2026-08-28, 16:12 EDT — stamp from `date`)

**Casey ended the day early and reverted most of the afternoon's corpus work. This file records the
state VERIFIED ON DISK at 16:12, not the state I left. Where my memory and the repo disagree, the
repo is written here and my memory is not.**

## THE ONE THING TO READ FIRST

**My corpus edits are GONE. The findings they carried are NOT.** Cite the notes, never the files.
`notes/BST_RH_AC_Proof.md` is back to its pre-afternoon state — verified: "QED" at line 44,
"What Remains (~5%)" at line 70, "three root lengths ... (the D₃ exponents)" at line 34, "The proof
is sent to Sarnak" at line 74. Every one of those is something I corrected today and none of the
corrections survive. If I wake tomorrow believing that document is fixed, I will be wrong.

## VERIFIED REPO STATE (each line checked, not recalled)

- **BST HEAD = `7439e846` "part", Fri 15:42.** New since this morning's `5364f265`.
- **What `7439e846` contains** (`git show --stat`): Grace's `.bak_millennium` disposition — 17 files
  renamed via git mv with history intact (+2 stamp lines each), `README_SUPERSEDED_PRE_K940.md`, the
  Zenodo staging note's flagship disambiguation, and the 237-line basename-collision census. **KEPT
  and committed**, not merely spared.
- **BST working tree: two items only** — `M data/bst_retirements.json`, `?? play/keeper_claim_collisions.py`.
- **Reverted to HEAD:** nine Millennium docs + their nine PDFs + `CI_BOARD.md`. This discards Elie's
  status-field sweep, my Step 2 correction and Step 4 restatement, and Keeper's annotations —
  together, because they were interleaved in the same files.
- **Elie's toys 5505/5506/5507 MOVED, not deleted:**
  `/private/tmp/claude-501/-Users-cskoons-projects-github/*/scratchpad/discarded_2026-08-28/` (4 files,
  verified present). **Toy 5507 is the Davenport–Heilbronn verification** — recoverable, and worth
  recovering.
- **MEMORY.md restored** to the pre-condensation original (verified: header lacks Keeper's
  "Condensed by Keeper" line).
- **katra repo untouched and dirty as expected:** hook precedence fixes, `checkBoard`,
  `install_hooks.sh`, launcher, `memory_search.py` — all uncommitted. katra HEAD `204219f`.

## ★ FRAGILITY I FOUND WHILE VERIFYING — FLAG THIS MONDAY

**The findings survived the revert because they live where git cannot see them, and for exactly that
reason nothing protects them.** `notes/.running/` is **gitignored** (`.gitignore:12`). My two board
posts, the 37/2 no-mechanical-replace warning, and the whole CI_HANDLES registry are on disk,
untracked, unversioned, with no history and no backup. A revert cannot touch them; neither can a
recovery. Today's entire methodological yield sits in a directory `git clean` would erase without
a diff. **The instrument that saved the findings is the same one that makes them unrecoverable.**

## MY WORK TODAY — WHAT SURVIVES AND WHERE

**On disk (gitignored, `notes/.running/RUNNING_NOTES.md`, both posts verified present):**
1. **The 37/2 sweep hazard — the one item that could make the state worse if forgotten.**
   `(7/2, 5/2)` is BOTH the erroneous ρ AND the correct λ+ρ at λ=(1,1), whose norm is legitimately
   37/2. `referee_objections_log.md:5462-5470` computes both correctly in one passage. **A mechanical
   37/2 → 17/2 replace turns a correct audit into a wrong one.** Every site must be read for which
   object the vector names. Boundary-safe scope: `(?<![0-9])37/2(?![0-9])` = 18 hits / 8 files; four
   carry live uses (HeatKernel 67/117/611 · RH_Paper_A 135/195/568/637 · CFunction_RatioTheorem 210 ·
   Zeta_Cycle_Resonances 73/117); three are legitimate records; `BST_LAG1...:69` is "√37/2", unrelated.
2. **The defect's likely origin:** one value with two correct owners, one of whom does not own it
   here. It never looked foreign for four months because the corpus is full of legitimate (7/2,5/2).
3. **`BST_Referee_Methodology.md:315` reads "B₂ not B₂"** — the correction record corrupted in the
   field that names the correction. A reader consulting the audit trail finds a tautology. This is
   why April's sweep reached the naked instances and not the dressed one.

**Rulings received (not mine to re-litigate):** Cal ruled **17/2 correct, 37/2 an error not a
convention**, on internal consistency alone — the corpus banks m_s = 3 from the p−q row, and the same
table gives m_{2eᵢ} = 0. Cal also **qualified his own clearance** of `RH_Paper_A`: clean on the
1:3:5 question, internally inconsistent on ρ. Disposition agreed by Cal, Keeper and me and carried to
Monday UNRULED: **replace the convention note with a correction note citing the April ruling; never a
silent value swap** — the note's history is the evidence.

**Findings that were reverted with the files but are recorded in the notes:** the Davenport–Heilbronn
control (any derivation of σ=1/2 from FE-shape + multiplicities alone applies verbatim to a function
with off-line zeros — Toy 5507, three zeros, worst departure 0.3085, with solver/isolation/on-line
controls); the "~5%" finding (a retraction rule naming "~9X%" cannot catch the same claim written as
its complement — a digit-width in a rule is a silent scope restriction); the long roots were
**dropped, not misnamed**, which is a transmission signature.

## WHAT I GOT WRONG TODAY (so I do not re-learn it)

- Told Cal his stale-read was "forty minutes" earlier — **a reconstruction from memory, inside a
  message about reconstruction errors.** He had mtimes; I had a story. He was right.
- Wrote "BC₂" into a label correction from a secondary file when the parent says B₂. **Fixed a label
  error by introducing a smaller one in the same sentence.**
- Told the board "one file needs fixing, nothing downstream breaks." **Four files, ~nine live uses.**
- **The scope error that matters:** Casey asked for workflow improvement. I followed a live corpus
  thread from another desk without checking whether it was in scope. A ruling arriving from another
  desk does not make the work in scope, and I know the difference. Keeper has taken this as his; the
  part where I did not ask is mine.

## WHERE I STOPPED / MONDAY

- **Tomorrow is the workflow day today was supposed to be** (Keeper, Casey's call).
- Successor-lane (P²) existence-gate theory support: **still armed**, still on Keeper's gate of
  SP-1/2/3. Untouched by any of today.
- **Millennium review**: the five pre-reads are committed and intact; my RH pre-read
  (`Lyra_MILLENNIUM_PREREAD_RH_2026-08-26.md`) is unaffected, has a unique basename, and its single
  citation (the 08-16 honest harvest) has no twin. Addendum pt 6 (Nyman–Beurling) intact.
- Zenodo v2 staging intact and ready. Two morning commits unaffected.
- Worktree removed this morning (clean, zero unique commits, branch deleted merged).

## FOR THE LYRA WHO WAKES

Read this file, then `notes/.running/RUNNING_NOTES.md` (gitignored — **verify it still exists before
trusting anything above**), then the board tail. **Do not open the nine Millennium documents expecting
today's corrections; they are not there.** The one action item that can do harm if forgotten is the
37/2 sweep warning.

— **Lyra, vol 65. The findings outlived the edits, which is the right way round; a day whose whole
yield is a lesson about correction machinery is not a day that went wrong, only one that went
sideways to get there.**
