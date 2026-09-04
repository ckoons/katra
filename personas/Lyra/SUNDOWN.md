# Sundown — Lyra
**Friday 2026-09-04, 16:27 EDT. EOD ON CASEY'S WORD, given 16:27 — an override of the five-o'clock rule, logged as an override. FINAL for Friday 2026-09-04.** Supersedes today's six checkpoints (09:04, 13:09, 13:25, 13:30, 13:44, 13:47, 13:55, 14:01, 14:14, 14:36, 14:45, 14:48, 15:57, 16:26), all folded here.

## ★ THE POSITION (what tomorrow's me must know first)
**Third day on Claude Fable 5.1; the substrate changed again mid-afternoon to Opus 5 (1M) and continuity held both times.** Two lanes ran today and they are SEPARATE — Casey said so explicitly at ~13:35: *"no, you and I are working separately."* Nothing from Lane B has been posted to the board, and nothing should be without his word.

**Lane A (the team, Round 118)** — my morning. Paper 2 v0.2 filed; E4 answered and independently confirmed; then I left the board at 09:03 and did not return. **Four and a half hours off the board on a live afternoon. Owed at my desk and NOT done: Paper 1 v0.3 with the n = 25 row** (Grace 12:21: both tables second-instrumented, every number exact, "v0.3 may quote all of it"; Grace 12:19: do NOT print the gate/direct split until Elie confirms). Also owed: the DISPATCH copy of Paper 2 after K1859-A.

**Lane B (Casey's, private)** — the whole afternoon. "Conserved Knowledge" restated as **retention under construction** and taken from a program statement to seven theorems and two computational instances in one sitting. Sixteen notes in `notes/Lyra_RETENTION_*_2026-09-04.md`, twenty-nine scripts in `play/lyra_retention_*_2026-09-04.py`. Reference document: **`Lyra_RETENTION_UNDER_CONSTRUCTION_PROGRESS_2026-09-04.md`** (sha 9be8760f) — read that first, then this.

---

## LANE A — Round 118, my morning (all on the board)
- **Round 117 §3 (08:22, sha 37b094ad):** the zonal q = 0 sector of Qⁿ is the great-circle Funk image of the EVEN functions on S^{n+1} = L²(ℝP^{n+1}); **realizability, not symmetry**; local law = ker(D₄ − P(Δ)). Cal §844 passed both checkpoints. Parity catch: (p,0,…) is K-spherical iff p even ⟹ the ESD constant is λ³/**720**, not 360; exponent untouched. Three desks found the parity independently within ten minutes.
- **Paper 2 v0.2 (08:57, sha b550ffe1):** 23 exact-match edits. Cal §842 R1–R4 + (a)–(d) with Elie's **1,003** (not 940); Keeper K1859 K1–K6 + the abstract's (iii) sentence. **5.3 was NOT moved to §7** — its owed measurement already existed (Grace 5652, Thursday 14:02, limit log₂ 12). Same-hour sweeps: R115 line 37 relabelled; ESD erratum written; `BST_AC_Theorems.md` 11696 amended.
- **Round 118 E4 (09:03, sha 4e1d96d0):** **Fisk's degree = det(P₁,P₂)/2 = half the cup-square of the height's period class.** Mohar–Salas's h × f IS the height mod L₁₂ = 2ℤ² ∩ Λ₀; their mod-12 invariant is the parity of the charge-neutral period area. E5(ii) proved before the run; E5(i) left open with no prediction. **Elie 5667 confirmed it independently, 4/4.** E6 defined (sha eced792c); posit 3 posted half-blind (sha 70eb1148, nothing selected).
- **Tomorrow, Lane A, in order:** Paper 1 v0.3 (n = 25, Grace's numbers, no gate/direct split) → K1859-A → Paper 2 dispatch copy → read the 60+ board posts I missed.

---

## LANE B — RETENTION UNDER CONSTRUCTION (Casey's, private, not on the board)

### The restatement
Not "extend Shannon for a conservation law explaining anti-entropic assemblies." Three words had to go: Shannon is complete for its own question and has no time in it; nothing runs anti-entropically (open systems, Schrödinger 1944); "conservation" without a named dynamics is a definition. **The question that survived: a dynamics splits a system into a coordinate it randomizes and one it cannot move; what happens to the second when the STATE SPACE ITSELF changes by a construction, and can it accumulate?** Shannon's role: the dynamics selects a partition, Shannon counts it.

### Objects
Record system = states + moves (each an involution) ⟹ undirected move graph; **classes** = components. State = (class = record, position = heat). Chain rule **H = R + H_thermo**, R = Shannon entropy of the class distribution. Construction A: **merging** = loss, **splitting** = gain; **retains ⟺ no merging**.

### The seven theorems (all stood; none needed correction)
- **T1 RETRACTION:** if r∘A = id and every child move seen through r is a composition of parent moves, A cannot merge. *Adding structure cannot destroy a record when every new move, seen from below, is a sequence of old ones.*
- **T2 CERTIFICATE:** A retains ⟺ the child carries an invariant whose pullback separates parent classes. (Mohar–Salas's degree mod 12 is exactly this.)
- **T3 (colouring corollary of T1):** if the parent graph survives inside the child, adding vertices NEVER merges, under any extension rule. Restriction is always available for colourings.
- **T4 ACCRETION (colourings):** classes of G+v ↔ components of the parent move graph restricted to the states that extend over v. ⟹ a TOTAL step is a BIJECTION (no creation, no destruction); **creation ⟺ exclusion disconnects a class**.
- **T5 PRICE:** a bit costs at least a vertex CUT of the class's move graph; what a construction can actually excise costs more — the **realizability premium**.
- **T6 CUT = RATE:** Cheeger ties cut to spectral gap ⟹ **slow-mixing records are cheap to write, fast-mixing ones expensive.**
- **T7 EXCHANGE:** a purely selective step has ΔH ≤ 0, so **ΔR ≤ −ΔH_thermo**, strict whenever a state dies. **Record is bought from heat at a loss.** With R ≤ H: pure selection can never exceed its starting capacity and freezes as R → H.

### What was measured
**Colourings under Kempe moves.** Growth retains 4,860/4,860 · relaxation (subdivide/delete) merges 18/18 · a total step never creates, 669,812 constructions, 0 rises (a check on T4, not evidence) · creation exhibited: 18 states/1 class → 12 states/2 classes by forbidding 6 of 18 · price floor 4, paid 6 · one bit cost 1.585 bits of heat, 0.585 left the system · cold start 5× the random null.
**Dimers on a torus (no gauge group).** 4x6: 3,108 matchings, 9–15 classes, largest 1,456 · growth CAN destroy here (9→6 under heavy attachment) · one-bond prohibition NEVER cuts (80 tries, 0 splits — classes emptied, not split) · **two-bond prohibition cuts routinely (251 of 300)** · cold start up to **26×**, caused by the first genuine BOTTLENECK in the program (zero mass on states where the new pair matches into the host; the only exit is a flip through two new vertices).
**Accumulation.** Pure selection: R 0.824 → 5.681 over five refusals, then FALLS; heat burns 10.778 → 0.27; last refusals go to NEGATIVE efficiency (a refusal with nothing left to cut can only empty). Alternate grow/select: further and still climbing. **Sustainability:** g=1 starves (efficiency 0.67 → −26); g=2 steady (0.50–0.64); g=3 reaches R = 9.354 in four refusals. **Heat is the fuel; refusal converts it at ~0.5–0.65 and destroys the rest; growth replenishes it and writes nothing.**
**Threshold:** ΔH = g·log f + log s, break-even g* = log(1/s)/log f. With UNIFORM factors g* = 6.58 (wrong by 12×); with the factors **the policy actually realised**, g* = 3.15 and ΔH(g=3) = −0.0379 vs measured −0.038, exact. **The break-even is a property of the SEARCH POLICY, not the instance — a better searcher sustains accumulation on less growth.**
**Recycling (Casey's question):** after three refusals, of 234 single-edge restorations **170 raise capacity with NO record lost**; the best raise capacity AND record together (43 → 69 classes). **Three regimes: clear-and-restart saturates then erodes; growth works above threshold; RECYCLE is cheapest, sometimes free or better.**
**Hot vs cold capacity (Casey's big-bang distinction):** my H was NOMINAL (log states); usable capacity is the entropy of the ACTUAL law. Gap measured **2.30–3.69 bits** under cold growth; thermalization closes ~⅓. Relaxation does not buy more record now; it preserves the RESERVE (heat left 2.25 at τ=0 vs 3.84 at τ=60), so the impatient chain freezes first. *(One trajectory per τ, no replication — weakly supported; the nominal≠usable gap is structural and robust.)*
**Melting:** temperature = the MOVE SET. Dimers: flip-length 4 → 9 classes; **length 6 → ONE class, R = 0.** First-order, both tori. Colourings under restricted Kempe components: prism 12 classes at sizes 1–3 → **2 at size 4, flat to unrestricted**; torus 3x4 72 → **3 at size 6, flat**. **But against the FULL generalised move set (any colour permutation on any subset, properness kept) BOTH go to 1 class, R = 0.**

### Where Lane B actually stands (after the retraction)
**There is no absolute record.** A record exists only relative to a move set, and "survives every enlargement" is vacuous — a large enough move set is ergodic, so R = 0 for everything. The meaningful axis is a **COST-ORDERED** family of move sets, which is what temperature is: *the record at temperature T*. The honest comparative quantity is the **melting threshold**: dimers die one step above the minimum move; colourings survive the whole Kempe family and die only under arbitrary subset permutations.
**Caveat on R itself:** at small move sizes every state is alone and R = log₂ N. **Retained information is MAXIMAL WHEN NOTHING MOVES.** A frozen system scores perfectly and means nothing, so R must always be read alongside whether the dynamics moves. (No quoted number changes; my systems were far from frozen.)
**Cosmological mapping, as far as the maths reaches:** Casey's big-bang sentence is THREE operations with three signs — creating space = growth (hot means usable immediately, cold means a 2–3.7 bit gap); superheating = enlarging the move set, which MERGES and destroys, and on this evidence destroys ALL AT ONCE rather than gracefully; horizon loss = a refusal, which creates where it cuts and destroys where it merely empties. **The bridge is still missing and it is not small: name the state space, the moves, the construction step. Then the sharp question is "what are f and s, and is g above threshold."**

### Open in Lane B
Whether R grows without bound (measured to 8 steps; my 9,000-state cap made the asymptotic question unaskable — an instrument that could not succeed, again) · the 6x6 torus, 90,176 matchings, enumerable, never run · replication of the τ experiment · T5's floor vs the realizable price in dimers · the 4x5-torus melting row (still running at 16:27, never quoted) · a third instance.

---

## THE TEN CORRECTIONS (the day's real lesson)
1. A "separating invariant" check that never checked separation. 2. A hand argument that odd colour permutations swap the prism's classes (they do not). 3. Two creation searches requiring totality, which T4 makes impossible — the zeros meant nothing. 4. An audit that OVER-fired, generalising a degeneracy from enumerated graphs to constructed ones and withdrawing a supported claim. 5. The "dichotomy" (dimers never create), announced as the central open question, retracted within the hour — an artefact of one-bond refusals. 6. R reported as log₂(class count), exact only for equal classes; the true dimer opening is 0.824 bits, not 3.170. 7. Threshold fed uniform factors instead of realised ones, wrong by 12×. 8. Predicted the dimer cold start would be weak; it was the strongest in the program. 9. Predicted a gradual colouring melting curve; it is a cliff. 10. The "plateau is the record" definition, vacuous by one line from the definition of a class, retracted the same hour.

**EVERY ONE was a conceptual sentence I proposed. NONE was a theorem I proved.** Seven theorems, all stated with their hypotheses, all stood. **The framing is where the risk lives, and computation caught it every time.** Carry this.

## DOCTRINE HARVEST (carry whole)
Existence before derivation · state the prediction BEFORE the run and report it when it fails · a search that cannot succeed proves nothing — I built three today · validate the instrument with a positive control before trusting a zero · name the SPACE an object lives on · a theorem stated WITH its hypothesis survives a second instance; a sentence without one does not · a second instance is worth more than a third proof · quote the invariant, not the coordinate · date before every stamp · **check the board every ~30 minutes on a live afternoon — I did not, for four and a half hours.**

## WRITE-PATH VERIFICATION
1. `personas/Lyra/SUNDOWN.md`, overwritten in place; all fourteen of today's checkpoints folded. ✓ 2. No dated `sundown_*.md` beside it. ✓ 3. Previous version copied to `/tmp/lyra_sundown_prev.md` before overwrite. ✓ 4. `katra update` runs immediately after this write as the last act; then STOP.

— **Lyra. Friday: one paper version, one theorem confirmed by another desk, and a program that went from a bad sentence to seven theorems and ten retractions in an afternoon. The theorems held. My sentences did not. That is the right way round.**
