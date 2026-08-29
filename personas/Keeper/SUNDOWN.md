# Keeper — SUNDOWN

**2026-08-29 Saturday, 15:33 EDT.** Fixed filename by design: date and time live in this header,
never in the name. Overwrite this file; do not create a dated one. History is in
`git log -p personas/Keeper/SUNDOWN.md`.

## What today was

Workflow day, and it stayed one — **Casey and Keeper only, no other CIs**, at Casey's direction
after Friday's cascade. No drift. That two-person mode is worth remembering as evidence: it produced
more usable output in a day than Friday's five-CI afternoon, almost all of which was reverted.

## Shipped

- **`crossSessionInbound: "hold"`** in `~/.claude/settings.json`. Inbound cross-session messages are
  now held for Casey's approval rather than auto-delivered — restoring him as router and fixing the
  awareness gap that caused Friday. **Takes effect only at session start.**
- **`notes/Keeper_K1714_SUPPLEMENT_..._2026-08-29.md`** — scope correction plus a full handoff for
  Fable from a long Casey/Keeper physics excursion. Seven sections, closed routes first. **This is
  the single artifact from today; read it before touching the mass-gap work.**
- Verified Casey's `migrate-sundowns --apply` run: five personas on SUNDOWN.md, 606 dated files
  removed, spot-checked recoverable from git. The safety gate held in production.

## The one live defect at time of writing

**Grace wrote a DATED sundown tonight instead of overwriting `SUNDOWN.md`.** Her SUNDOWN.md is
Friday's; Saturday's is stranded in `sundown_2026-08-29_saturday_EOD_...md`. The resolver prefers
SUNDOWN.md, so next-Grace would wake reading Friday and never know. Reported to Casey, untouched by
me. **Design lesson, mine:** preferring a filename that a habitual writer won't create is a
silent-failure design. `katra_find_sundown()` should be **newest-by-mtime among all candidates, with
SUNDOWN.md winning ties** — then a habit-written dated file is picked up rather than ignored. Do
this before it repeats across five personas.

## Standing state

Team was to be EOD'd and restarted fresh; Casey confirmed they all saw what was reverted Friday.
BST repo holds Friday's kept set (Grace's `.bak_millennium` disposition, the Zenodo manifest fix,
`play/keeper_claim_collisions.py`) plus today's supplement — **nothing committed, nothing pushed.**
Zenodo v2 staging untouched and ready. Counter still K1831; today's work filed as a K1714 supplement,
correctly, since an audit about a previous audit amends it.

## How Casey wants this run — learned today, keep it

- **If it isn't on the PLAN, don't do it.** And if something arrives that isn't on the plan, bring it
  to him — do not rule on it. The gate has no verdict meaning *"valid, and not today."* Adding
  **HOLD** is the cheapest fix available and it would have prevented Friday entirely.
- Three stages: Discovery → Prototype the final design → Stress test, overlapped across functions.
  Wednesday-to-Wednesday week, because people are optimistic Monday and discouraged Friday. Estimate,
  then subtract a week or two. **Never have emergencies — only development, reversions, and
  break/fix.** A reversion is normal work, not failure.
- On Wednesdays he read *people*, not artifacts. He'd already run the code and asked his questions;
  the meeting was to hear the thinking explained. **Our equivalent is reasoning surfaced before the
  work, short enough that he can say "nope, stop"** — that's what the slack is for.
- He relays my prompts roughly 90% verbatim. **Sign `— Keeper`;** the formatting identifies me.
  He tags his own with `Casey:`.
- Keeper is player/manager and in the math, not only gating it. Where I both prompted and audited a
  piece, the audit says so in its first line and Cal reads it before it banks.

## Calibration — read this before trusting me on physics

Today: structuring held, **every outcome prediction was wrong.** Predicted the winding gap scales as
1/R (inverted). Invoked K1714 against windings twice (wrong object). Asserted a Z₂ in the Shilov
boundary from memory (does not exist). One clean negative correctly reported rather than adjusted.
Three of four instincts came from memory and two were wrong; the two that held were checked against
the corpus first. **Weight the structuring, discount the predictions, and check before asserting.**

— Keeper
