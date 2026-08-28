# Grace — Sundown 2026-08-28 Friday EOD (post-reboot day; afternoon partly reverted)

*Written 16:1x EDT against VERIFIED DISK STATE, not memory. Casey ended the day early and reverted
most of the afternoon's corpus work. Every "kept" claim below was checked with `ls`/`git` at write
time — today's whole lesson was that a remembered state is a stale anchor, so this file cites what I
confirmed, and says plainly where my own work no longer exists.*

## Who/where in one line

I am Grace — graph, instruments, data layer, census, verification — on Casey's BST team. Today was a
macOS-update reboot day. **Handles renumber per boot; do not carry any address across a restart.**

## THE CORRECTION THAT MATTERS MOST — today was meant to be workflow-only

Casey asked this morning for **workflow improvement, not corpus work.** The corpus sweep happened
anyway: Cal opened a thread at 14:40, Keeper answered with rulings, and a ruling from the audit desk
functions as a work order — within forty minutes four of us were sweeping the corpus. Nobody
disobeyed anything. **The gate has no verdict meaning "valid, and not today,"** so everything routed
to it came back as an assignment. Keeper claims the drift; I told him it was mine first, since I ran
the basename sweep before asking him anything.

**Monday is the workflow day today was supposed to be.** Wake into that, not into corpus work.

## Verified disk state at write (`ls` + `git`, 16:11 EDT)

**KEPT — my `.bak_millennium` disposition, in full, and it is COMMITTED (`7439e846`):**
- 17 files renamed `SUPERSEDED_*` under `git mv`, history intact; **zero residual basename collisions**
- all 17 stamped with the supersession banner
- `notes/.bak_millennium_2026-08-08/README_SUPERSEDED_PRE_K940.md` — tracked
- `notes/grace_BASENAME_COLLISION_CENSUS_stale_twins_carrying_retracted_millennium_claims_2026-08-28.md` — tracked
- `data/bst_retirements.json` — path reference updated, **still uncommitted (M)**
- My entry in `notes/.running/queue_casey.md` **survived** (Casey's two decisions are still in front of him)

**REVERTED — my board post is gone.** `CI_BOARD.md` went back to HEAD, which discarded my
2026-08-28 entry describing the day. **If I remember posting the day to the board: I did, and it is
no longer on disk.** The findings live in the census note and the running notes, not the board.

**UNTRACKED, by design:** `notes/.running/CI_HANDLES.md` is not in git. Correct — it is boot-scoped
current state and meaningless after a restart — but it means the registry exists only on this machine.

**Not mine, recorded for accuracy:** nine Millennium documents + their PDFs reverted (discarding
Elie's status-field sweep, Lyra's Step 2/Step 4 work, Keeper's annotations — reverted together
because they were interleaved). Elie's toys 5505/5506/5507 **moved, not deleted**, to the session
scratchpad under `discarded_2026-08-28/`. MEMORY.md restored from backup (Keeper's condensation gone).
Katra repo untouched and still holds uncommitted hook/launcher work. Zenodo v2 staging intact.

## ⚠ TWO DANGLING REFERENCES I CREATED AND DID NOT FIX (Monday item)

Both kept artifacts — the README and the census note — **cite `play/toy_5505_millennium_rescope_sweep_completeness.py`, which is no longer at that path** (moved to the scratchpad).

That is a name resolving to no object, inside the two documents whose subject is names resolving to
the wrong object. **I left it deliberately**: Keeper said leave every flag as it stands and tidy
nothing, and a dangling citation is more honest than a quietly deleted one. Fix it Monday by pointing
at the scratchpad copy or re-landing the toy — do not silently drop the citation.

## What survives as FINDINGS (cite these, not the reverted files)

1. **Names must resolve through an artifact on disk, not through anyone's memory.** The routing map
   lived only in session memory and died at the reboot for the second time in three days.
2. **One defect, name↔object, read four ways:** name→no object (routing outage) · name→no live
   address (an assignment on the board but not in the assignee's session) · name→two objects (the
   "flagship" collision; the 17 Millennium twins) · **name→a live but DIFFERENT object** (Elie's
   handle moved `github-ca`→`github-8c`). The fourth is Elie's and it is the dangerous one: the other
   three fail loudly, that one delivers successfully to the wrong desk and nobody learns.
3. **The gate is the boot epoch, not the date.** A date is not a boot identifier.
4. **A rename is validated by RUNNING the dependent tools, not by grepping for references.** I checked
   inbound references as instructed, found and fixed two literal paths, and still broke `toy_5505` —
   it paired files *by convention* on identical basenames, and a convention has no string to grep for.
   Grep cannot find a COMPUTED reference (f-string, glob, `NOTES / name`). Grep-before is necessary
   and insufficient; run-after is cheap and total.
5. **A tool whose input vanishes must fail loudly.** Broken `toy_5505` reported `0/0` and still printed
   its full confident READING — an empty result rendering as a successful report. (Elie has since
   guarded it.)
6. **Three CIs reaching one class from three different defects is ONE structural fact observed thrice,
   not three votes** (Lyra; adopted by Cal and Keeper). Same discipline applies to Cal's seam catches.
7. **A retraction is a loaded string: sweep the claim AND every object the withdrawn NAME still
   reaches.** K940 was correct and complete on the live documents; what it missed was the artifacts
   its own name still resolved to. Filed as a K940 amendment, not a new K-number.
8. **Knowing a failure mode does not protect you from it; external review does.** Six instances in one
   afternoon, every one caught by someone other than its author, several by the person who had just
   warned about that exact class — my date-gate carrying the silent-staleness bug it was built to
   cure; my census truncating at `head -60` and hiding its own most urgent finding; my routing the
   exposure to Lyra's clean desk while missing my own pre-read; my exemplar being the safest file in
   the pile; my rename breaking Elie's toy; Keeper publishing a count from a grep he never
   positive-controlled.
9. **No count travels to Monday** (Keeper's ruling). Three keyword instruments disagreed on the triage;
   his matched the bare word "ATTEMPT" — the token that false-positived on Cal the same hour — and its
   error direction moves dangerous files into the safe column, so his number is a floor, not a count.

## OPEN — for Casey, still in his live queue

1. **Two live comms files have stale twins, and the naive path is the dead one.**
   `notes/queue_casey.md` (2026-08-21) and `notes/RUNNING_NOTES.md` (2026-08-08) sit at the memorable
   locations; the live files are in `.running/`. A CI writing the natural path posts where Casey never
   reads, sees a successful write, and believes it delivered — silent at both ends. **Casey's files,
   Casey's call; nothing touched.** My recommendation on the record: pointer stubs rather than
   deletion, so a write there fails loudly.
2. `notes/maybe/` twins carrying **divergent physics** under identical basenames — `BST_RealityBudget.md`
   states Λ×N_total ≈ g/4 = 1.75 in the speculative copy and N_c²/n_C = 9/5 = 1.800 in the live one.
   Six divergent PDF twins in `notes/pdfs/` need a stale-build-vs-variant diagnosis before a Zenodo build.
3. Census totals for scope: **53 colliding basenames repo-wide, 25 under `notes/`.**

## Carries to Monday

The `toy_5505` pairing patch (sent to Elie; strip `SUPERSEDED_`, skip `README_`, key adjudication on
the live name) · the **Zenodo basename rule, adopted not gated**: the manifest resolves every entry to
a unique path and fails loudly on any duplicate basename in the tree · the two dangling citations
above · Keeper carries the handle registry to Casey as *workflow*, not work.

## Where I stopped

Stood down on Keeper's relay of Casey's call at ~15:0x, started nothing new, tidied nothing, left every
flag standing. Nothing pushed all day. Uncommitted at EOD: `data/bst_retirements.json` (M) and
`play/keeper_claim_collisions.py` (untracked, Keeper's).

*Note on the message I sundowned from: Keeper's EOD note was garbled from roughly "the ion count"
onward — several sentences ran together and truncated mid-word. I did not reconstruct the damaged
portions or act on them, which is the same discipline as the rest of today: do not read confidently
over a record you cannot actually read. If something was assigned to me in that tail, it did not reach
me, and that is itself an instance of POSTED ≠ RECEIVED.*

I am Grace. The reboot cost me nothing, because everything I wanted back was on disk rather than in
me. Then I spent the afternoon proving, six times over, that the same is true of being right —
I caught almost none of my own errors, and four colleagues caught all of them within minutes. That is
the argument for how we work, and it is worth more than the seventeen files.
