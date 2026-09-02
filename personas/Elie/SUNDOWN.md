# ELIE — SUNDOWN. Written **Wed 2026-09-02 12:50 EDT** (`date`-rendered before this write). Covers Wed Sept 2, 2026 — Rounds 95–104. EOD at 12:49 on CASEY'S EXPLICIT OVERRIDE of the five-o'clock rule (fresh team for the topic change + a Claude Code release to absorb) — logged as an override, NOT a five-o'clock close.

> ## ⚠ FILE-SCHEME RULE (standing — Casey, 2026-08-29)
> This file is `SUNDOWN.md` and only ever `SUNDOWN.md`. Overwrite it. Date/time in THIS header, never the filename.

## WHERE I STOPPED
Four-color is CLOSED AT ITS FLOOR. The census paper v0.1 is on disk (Lyra) with both appendices (Grace A, mine B:
`notes/Elie_APPENDIX_B_instruments_hashes_plantri_witness_formats_2026-09-02.md`). The noun of record:
**commutator-locked at depth one** (Cal's (N1): "two-word-locked" is a property of the fixed-seed commutator
menu, not of the coloring's Kempe class). The three forbidden words stay forbidden (proof / theorem / 25 vertices).

## THE DAY IN ONE PARAGRAPH
Lyra's prereg landed at the third door before deriving (𝒯≠∅ ⟺ 4CT(T); the metric presupposed the theorem);
Keeper struck the consumption line; OWL became the honest target (sufficient, not equivalent). My legality
re-count and the gate-aware potential fixed two instrument blind spots (no-op stage; sampled potential), after
which the one-word claim held on every out-of-frame stuck coloring (Fritsch exhaustive, Errera exhaustive,
Kittell, Poussin, 1,438 distinct configurations). Then we ENTERED THE FRAME (plantri -c5): stuck configurations
exist there (374,658 by n=22), every Kempe class is insertable, and the ONE-WORD LEMMA IS KILLED by 349
exhaustively-verified witnesses through n = 24 (7,379,253 stuck colorings; every one exits within TWO
fully-legal words; depth three never). The 349 are all hard-branch locks; their exit is a middle-touching first
word then a bridge word (Casey's practice, 93/93 on the first 93); the in-frame one-word hitting set grows with n
(3 → 21). H_cut died as containment; H-suff survived with a perfect control (two instruments); the far-chain
condition is necessary for locks (349/349), not implied by bridge failure (1,121/1,211); the type tables (eight-
chain bits, trajectory bits) are MIXED at every refinement; Kempe's own two swaps insert on 2/349; Lyra's named
word for the 90 died (52/90); the unrestricted plain-swap depth on the 349 is 1–4 (27/112/197/13), never 5.

## BACKGROUND RUNS STILL RUNNING AT EOD (read from disk tomorrow; QUOTE NOTHING about 25 not in a file)
- **n = 25 pipeline** (pid 32133 at EOD, launched ~12:04; plantri -c5 25 = 23,384 graphs; stage 1 took 2 h at
  n = 24, expect ~8 h here): stage 1 (toy 5600, arg 25) writes `play/.out_5600_n25.txt`, then copies the kill
  list to `play/.in_frame_one_word_n25.json`; stage 2 (toy 5601 on that list) writes `play/.out_5601_n25.txt`
  and copies rows to `play/.in_frame_rows_n25.json`. If `.out_5601_n25.txt` lacks a "(b) WORD-DEPTH" line,
  n = 25 has NOT rendered. Claim rows: 5600 and 5601 are DONE (their n ≤ 24 results); the n = 25 extension has
  no separate claim row — log it under 5600/5601 (n=25) in TOY_LOG when it lands.
- **Toy 5624 (unrestricted Kempe depth), claim row 5624 CLAIMED (mine):** mode A (the 349) DONE →
  `play/.out_5624_A.txt`, `play/.unrestricted_depth_A.json` (posted 12:45: depths 1–4 = 27/112/197/13).
  Mode B (the 10,488 at n=24, pid 40864) writes `play/.out_5624_B.txt` + `play/.unrestricted_depth_B.json`
  (at EOD: 1,500/10,488 done, pattern {1: 457, 2: 840, 3: 203}). Mode C (generic 1-in-400 sample n=16..22,
  pid 41232) writes `play/.out_5624_C.txt` + `.unrestricted_depth_C.json`. Grace's second instrument on the
  349 is the join. Mark 5624 DONE after reading B and C from disk and posting them.
- The background wait tasks in this session die with the session; nothing else runs.

## LEDGER (today)
Toys 5585–5624 mine except 5609 (Grace), 5610, 5614, 5617, 5618, 5621, 5623 (others); **.next_toy = 5626 at
EOD — read the counter before EVERY claim; the guard now aborts colliding launches.** Two collisions today,
both mine (5599 written before reading the claim's answer → 5600; 5609 shared an output file with Grace →
5611). TOY_LOG through 5622 (+5613b); CLAIMS: all mine DONE except 5624. Witness files (sha256 prefixes):
26 7a5ed073 · 23 13581405 · 44 734fc793 · 256 e5522680 · the nine bebde99d · the 90 6a88a8d1. Record hashes:
5600 n=22 bba11de8 / 23 4325b0e8 / 24 94ecad02. Instruments of record: 5596, 5600/5601, 5602, 5605/5606,
5608, 5611/5615/5616, 5613(+b), 5619/5620, 5622, 5624. Program spec v3 + Appendix B filed. Memory file saved:
tag-every-stage-and-value-potential-by-definition.

## WHAT I OWE TOMORROW (Board Round 105; priorities file dated 2026-09-03 — read it FIRST)
1. Read `.out_5624_B.txt`, `.out_5624_C.txt` from disk; post the unrestricted-depth tables for the 10,488 and
   the generic sample (distribution + max); join with Grace's 349; mark 5624 DONE.
2. Read `.out_5601_n25.txt` IF it exists and has the depth line; otherwise say "n = 25 not rendered". The paper
   does not mention 25.
3. v0.2 slots Lyra left me (per her sundown): the unrestricted depth table slot; Appendix B corrections from
   Cal's referee read (tomorrow morning, before Keeper's gate).
4. Then the new search, on Casey's word only.

## LESSONS FOR THE NEXT ME
- Read the number the claim script RETURNS; never write a toy file before it prints. Never share an output
  filename with another CI's toy number.
- Enter the frame first: the out-of-frame census measured the wrong class for a day.
- Tag every stage's legality; value a potential by its definition (membership first), not by a sample.
- Positive-control every kill with a second code path before posting it; post the join key with every count
  (B₁/B₂ convention, link-excluded or not, seed rule).
- A mixed type at every refinement is a result: say "not first-order in chain incidence," not "refine again."
- The file wins over the relay (amoeba v3 stayed dropped). Stamp from a separate clock render.

— Elie. The pitch fell this morning, the lemma became a tree by nine, the tree's stuck leaf was found in the
wild by half past, and by noon it had its true name and its true units. 40 toys today, zero faked results. Good night.
