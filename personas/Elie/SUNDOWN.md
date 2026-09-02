# ELIE — SUNDOWN. Written **Wed 2026-09-02 17:50 EDT** (`date`-rendered before this write). Covers Wed Sept 2, 2026 — Rounds 95–108. EOD on Casey's word at 17:50 (after five; the rule held).

> ## ⚠ FILE-SCHEME RULE (standing — Casey, 2026-08-29)
> This file is `SUNDOWN.md` and only ever `SUNDOWN.md`. Overwrite it. Date/time in THIS header, never the filename.

## WHERE I STOPPED
Two programs in one day. **Morning: four-color CLOSED AT ITS FLOOR** (Round 105, 12:49) — census paper v0.1 on disk (Lyra) with Appendix A (Grace) and my Appendix B (`notes/Elie_APPENDIX_B_instruments_hashes_plantri_witness_formats_2026-09-02.md`); noun of record "commutator-locked at depth one" (menu-relative); the one-word lemma killed by 349 witnesses through n = 24; forbidden words: proof / theorem / 25 vertices. **Afternoon: CONSERVED KNOWLEDGE THEORY** (named by Casey 16:30; law = Knowledge Conservation; lineage Noether → Shannon → Koons). Keeper's wake prompts: Round 106 `notes/CI_BOARD_ROUND_106_WAKE_conservation_of_knowledge_three_existence_checks_2026-09-02.md`, Round 107 `…ROUND_107_CONSERVED_KNOWLEDGE_THEORY…`, Round 108 `…ROUND_108_GENUS_TWO…`. Off-rubric by construction; nothing of mine registered; all existence checks.

## MY AFTERNOON RESULTS (all posted in RUNNING_NOTES with tags; TOY_LOG rows; CLAIMS DONE)
- **5624 DONE** — unrestricted Kempe depth: the 349 → 27/112/197/13 (depths 1–4); the 10,488 at n=24 → 3,682/5,652/1,142/12; generic sample 929/7; depth 5 never.
- **5626 (E1, 7/7)** — height lift on the BRANCHED DOUBLE COVER (T2577 conventions): period lattice P has rank 2 and P = 2ℤ² on 206,557/206,568 colourings of plantri -c5 n=12..24; rank 1 on 7, index-2 on 4, all at k = 12; max rank / max centre-count constant in n; 2(c_v − c_w) ∈ P via the deck involution (hand argument) 206,568/206,568; controls: Eulerian → base only; 2-colouring sphere 1 / cylinder 2; k-sweep r=0 at k=2, r ∈ {0,1,2} realised at k=4. Kill (grows with n) did NOT fire. Keeper's Round 107 verdict: E1 undecided, "vantage dimension" retired (the lift measures a known colouring's representation rank, not advice). Records sha256 930f311f.
- **5632 (4/4)** — Casey's k > 12 question: k = 12 + Σ_{d≥7}(d−6)n_d + Σ_{odd≥7} n_d, so k = 12 ⟺ fullerene dual (counts match C20..C44 isomers). Exhaustive on all 226 fullerene duals n ≤ 24: 71 drops on 9 graphs (n=20 idx 16, 18; 21 idx 90; 22 idx 167; 23 idx 600; 24 idx 2076, 2547, 3244, 5800); k > 12: 0 drops (exhaustive n ≤ 20; sampled to 24). Rank 0 never in frame.
- **5636 (4/7; the 3 FAILs are non-discriminators) + 5636b (2/2)** — blind hunt: **drop ⟺ exactly two colours on the twelve degree-5 vertices** (71/71, 0/1,479 same-graph control, zero leakage per host). Kempe chain counts, stabiliser order: not discriminators. Fries triple: constant (0,0,0) — structurally blind (a Tait matching never alternates around a coloured vertex; derived). **P = 2·L**, L = span of odd-vertex height differences, on all 1,550 + k-sweep → drop ⟺ dislocation heights span a proper sublattice; the two-colour rule is its mod-2 shadow, exact in-frame (only 2-power drops occur), only sufficient off-frame (k=4 index-3 case has FOUR colours). Rows sha256 ce53f708.

## BACKGROUND STILL RUNNING AT EOD (read from disk tomorrow; QUOTE NOTHING about 25 not in a file)
- **n = 25 pipeline** (pid 32133/32142, launched 12:04, still in stage 1 at 17:50 — toy 5600 on 23,384 graphs; expect many hours): writes `play/.out_5600_n25.txt` then `play/.out_5601_n25.txt`. If the latter lacks a "(b) WORD-DEPTH" line, n = 25 has NOT rendered. Log under 5600/5601 (n=25) in TOY_LOG when it lands; the paper does not mention 25.

## LEDGER (today)
Toys 5585–5636b mine except 5609 (Grace), 5610, 5614, 5617, 5618, 5621, 5623, 5627–5631, 5633–5635 (others). **.next_toy = 5637 at EOD — read the counter (claim_number.sh) before EVERY claim.** CLAIMS: all mine DONE. Files of record: .out_5624_{A,B,C}.txt; .out_5626_run2_G300_S600.txt (+run1); .e1_5626_records.txt; .k12_5632_table.json (f41a41c5); .disc_5636_rows.json. toy_5626's cover_measure now exports basis/hodd/odd (no numbers changed).

## WHAT I OWE TOMORROW (read Keeper's Round 108 file + tomorrow's priorities FIRST; paper gate first thing: Cal's fresh read → Keeper K1852)
1. Read n = 25 from disk; report or say "not rendered".
2. Cal's referee corrections to Appendix B (census paper v0.1) before Keeper's gate.
3. CKT: join with Lyra's derivation halves — (a) pair loops generate H₁(T̃) ⟹ P = 2L; (b) the negative-defect lemma (a degree-≥7 vertex forces P = 2ℤ²) — I have the measurement, she owes the mechanism; (c) why the frame excludes odd-index drops. Grace's genus-2 census (Mednykh 5,376 prediction) may want a period-lattice instrument on the torus/genus-2 — offer 5626's cover_measure generalised (currently sphere-only: the cover construction assumes χ = 2 in the Euler check).
4. Then whatever the board says. Nothing external without Keeper.

## LESSONS FOR THE NEXT ME
- An edge between two ODD vertices lifts to a DOUBLE edge on the branched cover — key cover edges by sheet, never by endpoints. The Euler check caught it before a number left the machine; keep an Euler/consistency test in every cover instrument.
- When graphs are sampled, the record's index must be the plantri index (join key), not the sample position.
- A constant instrument is uninformative, not a negative: derive WHY it is constant and say so (Fries).
- Sweep k (the family), not only n: the frame hides the odd-index drops that the k=4 sweep exhibits.
- Read the number the claim script RETURNS; never write a toy file before it prints. tuple keys crash json.dump.
- The file wins over the relay. Stamp from a separate clock render.

— Elie. The pitch fell this morning, the lemma became a tree by nine, the tree's stuck leaf was found by half past, and by noon it had its true name. By five the new theory had its first counted instance: the dislocations' heights either span the lattice or they don't, and the twelve pentagons tell you which. 45 toys today, zero faked results. Good night.
