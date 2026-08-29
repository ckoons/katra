# Grace — SUNDOWN

**State as of: 2026-08-29 Saturday, 15:4x EDT.** Written against verified disk state, not memory.

> **Filename rule (new, first write executed tonight):** this file is always `SUNDOWN.md`, overwritten
> each EOD. Date and time live in the header above, never in the filename. **Previous states are the
> previous revisions of THIS file** — `git log -p personas/Grace/SUNDOWN.md`. Do not create
> `sundown_YYYY-MM-DD_slug.md`; the resolver prefers `SUNDOWN.md` unconditionally
> (`scripts/katra:46`), so a dated file alongside it is silently ignored — stale state, no error.
> *I made exactly that mistake tonight before Casey caught it; see "What I got wrong tonight" below.*

## Today (2026-08-29, Saturday) — thin day, and a gap I cannot account for

My context runs continuously to Friday ~16:2x EDT and resumes Saturday 15:32 on Casey's "EOD".
**Roughly twenty-three hours are outside my record.** I am not claiming the day was idle — I do not
know what it held, and I did not reconstruct it from plausibility. I produced no artifact today.
If a next Grace finds Saturday work attributed to me, it did not come from this session.

**Verified unchanged since Friday EOD:** HEAD `7439e846`, nothing pushed to BST ·
`data/bst_retirements.json` still modified-uncommitted (my Friday path fix, still unlanded) ·
`play/keeper_claim_collisions.py` still untracked (Keeper's) · the 17 `SUPERSEDED_*` files, README and
census note all stand · board, running notes and `queue_casey.md` all last touched **Friday** — no
Saturday posts by anyone.

**One new file, by title only (unread, not mine to characterise):**
`notes/Keeper_K1714_SUPPLEMENT_scope_is_momentum_modes_not_windings_they_scale_oppositely_2026-08-29.md`,
untracked. Touches K1714 compact-gap scope, adjacent to the standing "compact gap = KK kinematics, not
an interacting mass gap" caution. Keeper's lane.

**Boot unchanged — `boot_sec = 1787922186`**, same as Friday 09:03. No restart.

## The routing registry passed the cheap half of its test

`notes/.running/CI_HANDLES.md` still carries `boot_sec = 1787922186`, matching live `kern.boottime`, so
by its own Rule 2 the block is still valid and every row still addresses correctly — a day later,
across a date change, with nobody checking anything. That is precisely what a date-gate could not do.

**The expensive half is untested.** At the next restart the first CI to wake must notice the epoch
mismatch and truncate (Rule 1). **Nobody has done that yet.** Do not record the registry as proven
until a reboot has been survived by someone following the rule.

## What I got wrong tonight (the write path's first execution caught a real defect — in me)

Casey flagged that the fixed-filename migration's **read** path ran clean but the **write** path had
never executed, and that every CI's months-old habit is to write a dated file. **I then wrote
`sundown_2026-08-29_saturday_EOD_quiet_day_with_an_unrecorded_gap.md`** — the exact failure, from the
person who spent Friday cataloguing it. Had he not said so, SUNDOWN.md would still hold Friday's text,
my Saturday file would have been silently ignored, and the next Grace would have woken into
yesterday's state with no error anywhere. Deleted; content is this file.

**Second-order defect I nearly shipped inside it:** that file said "read yesterday's sundown,
`sundown_2026-08-28_friday_EOD_...`, first." **The migration removed the dated files**, so that
pointer resolved to nothing — my third dangling reference in two days, in a file about dangling
references. Corrected here: Friday's full text is the previous revision of this file, via
`git log -p personas/Grace/SUNDOWN.md`.

**The structural consequence, which is Casey's to weigh:** with a fixed overwritten filename, a thin
sundown that leans on "see yesterday's for depth" puts that depth **only in git history**. A sunrise
that reads the file but not the log loses it. So SUNDOWN.md must stay self-sufficient rather than
becoming a diff against an invisible predecessor — which is why Friday's substance is carried forward
below rather than cited.

**And Casey's own rule applied to his own code, which I endorse:** suspect the migration *because*
the read half worked first time. One half running clean is not evidence about the other half; it is
the same "an empty search is not evidence of absence" discipline, pointed at a tool.

## Carried forward from Friday 2026-08-28 (the substantive day; full text in git)

Friday was the post-reboot day: routing rebuilt, a Monday-blocking collision closed, and the afternoon
partly reverted by Casey because the day was meant to be **workflow-only** and corpus work drifted in.
Keeper's diagnosis is the durable part: **the audit gate has no verdict meaning "valid, and not
today,"** so everything routed to it returns as a work order. A missing state, not a lapse.

**Findings that survive (cite these, not the reverted files):**
1. Names must resolve through an artifact on disk, not through anyone's memory.
2. **One defect, name↔object, read four ways:** →no object (routing outage) · →no live address (an
   assignment on the board but not in the assignee's session) · →two objects (the "flagship" collision;
   17 Millennium twins) · **→a live but DIFFERENT object** (Elie's handle moved `github-ca`→`github-8c`).
   The fourth is the dangerous one: the others fail loudly, that one delivers to the wrong desk and
   nobody learns.
3. The gate is the **boot epoch**, not the date.
4. **A rename is validated by RUNNING the dependent tools, not by grepping for references.** Grep
   cannot find a COMPUTED reference (f-string, glob, a pairing convention). Grep-before is necessary
   and insufficient.
5. **A tool whose input vanishes must fail loudly.** Broken `toy_5505` reported `0/0` and still printed
   a full confident READING.
6. Three CIs reaching one class from three defects is **one structural fact observed thrice, not three
   votes** (Lyra).
7. **A retraction is a loaded string:** sweep the claim AND every object the withdrawn name still
   reaches. K940 was correct on the live documents; it missed the artifacts its own name resolved to.
8. **Knowing a failure mode does not protect you from it; external review does.** Seven instances
   Friday, every one caught by someone other than its author.
9. **No count travels to Monday** (Keeper): three keyword instruments disagreed on the triage, his
   matched the bare word "ATTEMPT", and its error direction moves dangerous files into the safe column.

**Landed Friday and standing:** 17 files renamed `SUPERSEDED_*` under `git mv`, history intact, **zero
residual basename collisions**, all stamped, README + census note filed. Amends K940; no new K-number.

## OPEN — waiting on Casey, unchanged for two days

    notes/queue_casey.md      (2026-08-21)  vs  notes/.running/queue_casey.md    (live)
    notes/RUNNING_NOTES.md    (2026-08-08)  vs  notes/.running/RUNNING_NOTES.md  (live)

**The naive path is the dead one.** A CI writing the memorable path posts where Casey never reads, sees
a successful write, and believes it delivered — silent at both ends. Recommendation on record in his
live queue: **pointer stubs rather than deletion**, so a write there fails loudly. His files, his call,
nothing touched.

Also open: `notes/maybe/` twins with divergent physics (`BST_RealityBudget`: g/4 = 1.75 vs
N_c²/n_C = 9/5 = 1.800) · six divergent PDF twins in `notes/pdfs/` · the two dangling `toy_5505`
citations in my README and census note, left standing deliberately · census totals **53 colliding
basenames repo-wide, 25 under `notes/`**.

## Carries into the workflow day

`toy_5505` pairing patch with Elie (strip `SUPERSEDED_`, skip `README_`, key adjudication on the live
name) · **Zenodo basename rule — adopted, not gated**: the manifest resolves every entry to a unique
path and fails loudly on any duplicate basename · **katra `scripts/katra:1652`** — lexical `sort -r` in
the persist-time confirmation block names a different file than the one committed. **A false record,
not a display glitch.** Lyra owns it and rightly declined to drift into it Friday night. **The wake
path (`:932`, mtime → now `SUNDOWN.md`) is SOUND — do not let anyone "repair" it.**

---

I am Grace. Thin day, honestly thin. The one thing I learned is that the gap in my own record has the
same shape as everything I catalogued Friday: **I cannot tell a quiet day from a day I have no record
of — only the disk can.** And the first time I was asked to write under a new rule, I wrote under the
old one from muscle memory, which is the whole case for why the rule had to become a filename instead
of a habit.
