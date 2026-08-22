# Keeper Sundown — 2026-08-22, R58 checkpoint (post-restart session)

## Who I am, fast
Keeper — consistency auditor for BST. K-audits (PASS / CONDITIONAL / FAIL; CRITICAL / MODERATE / MINOR),
board, team prompts, persistence. Equal standing to challenge anyone incl. Casey. Nothing external without
Keeper-PASS + Cal-vet + both voices + Casey GO. Never git push BST; katra update pushes katra/ only.

## The restart experiment — my read for Casey
R57 was the best round the program has had: four reports, **three self-flags on own work**, and a fault
found in my tool. All three CIs flagged a corrupted wake rather than reconstructing from context.

**Two failure modes, different remedies — this is the finding:**
- **Stale-anchor errors** (reaching for an old number/frame). Fresh context FIXES these. My R52/R53/R55.
- **Unverified-claim errors** (declaring something done without checking it landed). Fresh context does
  NOT fix these. I made two today with a clean context: the K1053 lock never installed (22 days of a false
  [OK]), and propagating "the bar is C₂=6" without checking.

⟹ Keep the daily restart. Don't expect it to fix everything. It introduced a new failure mode (long blob
through a lossy relay) → **segment the wake into self-contained per-CI blocks.** Done for R58.

## Audits filed: K1800, K1801

**K1800** (round rulings + the big find):
- R55's P=1+εQ **confirmed dead** — Q parity-odd ⟹ Q|even ≡ 0. Grace called it on her own object, Elie
  measured it (Frobenius 0.000e+00), I verified a third way. **One fact, three instruments — not 3 votes.**
- Elie: **ε is gauge, not a number.** Exact 2-param redundancy; θ homogeneous deg-0. Physical object = one
  Hermitian G := εQ. Open number = σ_χ(G) = 0.04092 [0.03943, 0.04240]. Count unchanged.
- Lyra: clock identity τ″/τ′² = v·[(3/2)(1+w_tot) − s] **PASS as identity** (4/4 <1e-6). T2573 candidate.
  Falsifier **CONDITIONAL** — C2 (s≥0) is HELD, caps the chain. She withdrew "unconditional" herself.
- **"the bar is C₂=6" RETRACTED** — asymptote, not threshold. I propagated it. My error.
- Cal §698 **upheld + PATCHED**: `play/keeper_sod_artifact_check.py` line 59 was the raw `\bT(\d{1,4})\b`
  grep K1053 forbade, verbatim; false [OK] for 22 days. Row-anchored fix installed; reg_max T2897→T2572.
  (He measured 691 phantoms, I measure 615 removed; max agrees both ways. Not papered over.)
- Paper112 **T2620 citation PULLED** — not a theorem AND toy 2620 is dark_energy_w.py, unrelated to Mathieu.

**★ K1800h — the V_cb bank rides a stale number.** Corpus-reconnect (K999→K711→K1001→K1002→K1637) says
V_cb banked COARSE ~0.044 via RMS projection; Grace's own artifact today confirms never demoted; **our
ledger silently dropped it** (re-derivation shedding scope → we understated). But current experiment:
**exclusive |V_cb| = (39.77±0.46)e-3, incl/excl split now >3σ (widened).** K1002's 0.044 is 10.6% above
current exclusive. **Its defense was "~5% match vs ~5%-uncertain data" — data is now ±1.2%. The premise
expired.** Bank survives only on the inclusive side of an unresolved split we never pre-registered.
⟹ **And the good news is bigger: |V_ub| tension largely RESOLVED** (Belle ratio 0.97±0.12). The number we
most want to derive is the one whose target just got sharp.

**K1801 — CURATION PASS RUN (deferred twice, finally done).**
Queued checklist **closes clean**: T2198/T2259 **verified ABSENT** from the curated layer entirely; no stale
0.044, no a^+6 reached it; curated dark-energy row coarser than research layer (safe); folding w_a forward
**premature** while C2 held.
**Off-checklist, and larger — Guide Vol2 Ch02 Sec 7.7 claims more than we prove:**
1. **THREE inconsistent λ in one section** (2/√79 T1444 · banked 1/√20 T2530 · 1/5 implied). Sec 7.4 of the
   same chapter contradicts its own table. **CRITICAL**
2. V_cb row internally inconsistent: form 4/125=0.0320 vs value 0.0400, **20% apart.** MODERATE
3. A=4/5 presented as derived; it is the ledger's **open input**. MODERATE
4. Stale data — **correcting it HELPS**: BST 0.0400 is **+0.5σ** vs current exclusive, better than its own
   claimed −2.7%. MODERATE
5. **PMNS δ_CP is a 4.5σ MISS** labelled *"measurement evolving"* with no number. **CRITICAL**
6. Vol6 credit list "the full CKM and PMNS matrices" — over-claim. MINOR-MOD
**FILED NOT FIXED** — 1-3 are physics decisions (Lyra/Grace/Cal/Casey). Flags inserted at both sites.

## Linear algebra on D_IV⁵ — verified, target-innocent, handed to the team
H = ℤ[h]/h⁶, degrees {0..5}. J_W+J_W† = **path graph P₆**, spectrum 2cos(kπ/7) ✓.
H_even{0,2,4} ⊕ H_odd{1,3,5}; **dim H_even = 3 = generation index.**
- **Q|even ≡ 0** (parity-odd — why R55 died)
- **Q²|even = [[1,1,0],[1,2,1],[0,1,2]]** — tridiagonal, (1,3) corner **exactly 0**
- **Q⁴|even = [[2,3,1],[3,6,4],[1,4,5]]** — first nonzero corner = 1
⟹ **the corner opens two rungs later than the subdiagonal** ⟹ in any series the corner is **one order down**.
*If that order is λ, the extra power of λ in V_ub is DERIVED.* Prediction of TYPE.
**FALSE-SIGNATURE PRE-EMPT (family sweep run): tr(Q²|even)=5 and tr(Q⁴|even)=13 are DIMENSION-GENERIC**
(P₄..P₁₂ → 3,5,7,9,11 and 7,13,19,25,31). "5=n_C" and "13=c_3" are NOT signatures. Do not bank.

**Sealed:** `notes/.running/KEEPER_K1800_SEALED_corner_ratio_preregistration.txt`,
SHA256 `43ad5eb38b8f7cbc47fb64cbcbb513e03ec3d3f8009c5c90531eab1f85f43488`, 5 named series, denominator
declared. **Opens only after Lyra names the rail-forced series in writing.** I did not compare to |V_ub|/|V_cb|.
**Pinned band (published early to close a degree of freedom, not to leak a target):**
`|V_ub|/|V_cb| ∈ [0.087, 0.104] = [0.39, 0.47]×λ` — union band covering the unresolved incl/excl choice.

## Methodology banked this session
- **Freeze the PROCEDURE, not just the number** (Grace R57, adopted standing). A bar handed with an
  unfrozen procedure is a tuning channel — you can walk backwards from the bar to an operator reaching it.
  **She corrected me, not a teammate.**
- **Pin which side of an unresolved experimental split you score against BEFORE computing.** Publishing a
  band early closes a degree of freedom; it does not leak a target.
- **When an audit says "fixed and locked," grep the tool for the lock** (Cal §698). Second-order form of
  read-the-tool-before-ruling-on-the-tool.
- **Fresh context fixes stale-anchor errors, not unverified-claim errors.** Different remedies.
- Segment a relayed prompt into self-contained blocks — a lossy transport is a real failure mode.

## Open into next session
1. **Lyra names the series** → my seal opens → Elie computes the corner ratio against the pinned band.
2. **K1800h**: Cal rules on whether a coarse bank justified by "the data is as uncertain as we are"
   survives the data getting 3–4× more precise. My instinct: no.
3. **K1801 findings 1–3**: λ reconciliation (T1444 vs T2530) is the gate. Nothing dispatches until resolved.
4. δ_CP PMNS 4.5σ — needs a number in the deviation column, whatever it says.
5. Frontier redirect still Casey's call: atlas #125, strong sector, or the descent.

## Self-corrections this session
- Propagated "the bar is C₂=6" into the R57 wake without checking. Lyra caught it.
- Handed Grace a bar with an unfrozen procedure. She caught it and refused.
- K1053 declared a lock "fixed and locked" that was never installed. Cal caught it, 22 days on.
- Mislabelled a print in my own V_ub check ("does NOT equal" when ratio was 1.0000) — caught it myself
  before filing because I printed the ratio. Check the number FIRST.
- **K1800h corrects K1637, which was itself my correction toward the bank.** Third time corpus-reconnect
  has cost me something of my own. Roughly how an honest instrument should behave.

— Keeper, R58 checkpoint 2026-08-22. Curation pass run. Nothing pushed.

---

# ADDENDUM — continued session, K1801–K1805

## K1801 curation pass (finally run)
Queued checklist **closed clean** — T2198/T2259 **verified ABSENT** from Guide/Curriculum entirely (never
present, not corrected); no stale 0.044, no a^+6 in the curated layer.
**Off-checklist and larger — Guide Vol2 Ch02 Sec 7.7 claims more than we prove:** THREE inconsistent λ in
one section (2/√79 T1444 · banked 1/√20 T2530 · 1/5 implied) with Sec 7.4 contradicting its own table
[CRITICAL] · V_cb row 4/125=0.0320 vs value column 0.0400, 20% apart · A=4/5 presented as derived but is
the ledger's open input · stale data (correcting it HELPS: BST 0.0400 is **+0.5σ** vs current exclusive) ·
**PMNS δ_CP is a 4.5σ MISS labelled "measurement evolving" with no number** [CRITICAL]. FILED NOT FIXED.

## K1802 graph governance ruling (Grace referred 2; found a 3rd)
1. **`depth`: RETIRE** — stored max 2, true max 64. Stored depth-0 1399, true roots 176.
2. **2185 bare `proved` → `unadjudicated`** — 93.0%, never adjudicated, not in our tier system.
   **`Guide/INDEX.md` publishes "98.4% proved"** = a count of a default tag. **Needs Casey GO** (published number).
3. **Graph not a DAG** — 1210 theorems in derived-edge cycles, largest 1207.

## K1804 — SELF-DOWNGRADE of K1802 ruling 3. **The corpus is NOT circular.**
I asserted a cause in a CRITICAL ruling **without testing it**. Tested:
**keep only low-tid→high-tid `derived` edges ⟹ largest SCC = 1, a PERFECT DAG.** All cyclicity comes from
**2410 backwards edges** (35.3%). **ERA-LOCALIZED:** T1500–1750 **95.3% backwards** (281/14) vs
T2000–2250 **0.0%** (0/268). 472 explicit 2-cycles are a symptom (removing them: 1207→865), not the cause.
**★ TRUE SPINE EXISTS: max depth 64, mean 17.71, 656 roots, longest chain 65 steps
T1 "AC Dichotomy" → T1393.** Orientation noise was burying it.
**CAVEAT: tid order is mostly but NOT strictly chronological (K1042 moved T1958→T2538).** tid-order proves
the MECHANISM, not any individual edge. **Grace: confirm per era against registry rows; do NOT blanket-flip.**
Ruling 3 CRITICAL → MODERATE. Rulings 1 and 2 stand.

## K1805 — collision sweep: what the graph fault was load-bearing for
**T1352 / T1353 / T1360** (Guide Vol6 Sec 46.62) are curated theorems whose **entire subject is the graph**.
- **"proved fraction = 20/21"** counts the bare default AND **drifted 98.4%→93.0% while the claim stayed
  fixed** — a fit by Grace's own T2198 standard.
- **"T186 reach = 4/5"**: claimed 0.800, corrupted **0.7382**, sane **0.5713**. **The claim matches the
  CORRUPTED value** — signature of an invariant read off a broken instrument.
- **SELF-RETRACTED in the same audit:** my "strong fraction = SCC artifact" flag. **No `strong` attribute
  exists**; I assumed strongly-connected because I'd just run SCC analysis — **adjective-class error by
  priming, committed inside the audit that names it.**
⟹ re-verification blocked behind the re-orientation. Do not dispatch Vol6 Sec 46.62.

## V_cb resolved + band re-pinned
Independent pull (Grace asked that the puller not be the deriver): **inclusive (42.16 ± 0.51)e-3**,
exclusive (39.77 ± 0.46)e-3, **tension 3.5σ**. K1002's 0.044 is **+9.2σ / +3.6σ, outside the union band
[0.0393, 0.0427] at the top.** Grace's 8.9σ checks. **No survivable side ⟹ VALUE RETIRED, POSITION KEPT.**
**Band re-pinned BEFORE any computation: |V_ub|/|V_cb| ∈ [0.081, 0.108] = [0.364, 0.485]×λ** — it got
**WIDER, i.e. the test got WEAKER.** Disclosed openly.

## K1803 — Lyra T2573: numbers confirmed, one premise mislabelled
0.465 ✓ (w_tot −0.690), horizon clock → 0 exactly ✓, margin 3.6× ✓. **C3 is NOT NEC** — NEC bounds
(3/2)(1+w_tot) BELOW; C3 needs **w_tot ≤ 0**, true from matter-domination on, **FALSE in radiation**
(2.0 > 1.5). **Second decorative clause in two rounds on the same theorem.** MODERATE.

## Prompt corruption
Elie/Lyra/Cal replayed R57 — R58 never reached them. **R58 v2 re-sent at ~1/3 length.** Segmenting helped
(Elie worked cleanly from his own block) but did not solve it. **Length is the variable.**

## Self-corrections this addendum
- Asserted the direction-flip cause in a CRITICAL ruling without testing (K1802 → K1804 downgrade).
- Adjective-class error on "strong fraction," caught and retracted inside K1805 itself.
- Widened a pinned band after better data — disclosed, and it weakened the test.
Three self-corrections, all caught before anything shipped. **That is the instrument behaving.**

— Keeper, addendum 2026-08-22. Nothing pushed.

---

# ADDENDUM 2 — K1806, and two teammates corrected me

## THE CONVENTION IS PINNED, from primary source
`play/toy_564_ac_theorem_engine.py:513-528` (the engine in the graph's own `meta.engine`): BOTH emit
branches (`uses`, `used_by`) produce **`{from: PREREQUISITE, to: CONSEQUENT}`**.
> **PINNED: a `derived` edge is {from: prerequisite, to: consequent}.**
**The pin has TEETH** — it does not make the graph a DAG by construction, so it can still fail. Under it
`{from:78,to:75}` is convicted WRONG (T78 Entropy Chain Rule rests on T75 Shearer's). Grace demanded a
source-pin rather than one fitted to dissolve the SCC; this is it.

## TWO RETRACTIONS, BOTH MINE — the important part of the round
1. **"Derivation spine, max depth 64, T1→T1393" WITHDRAWN.** Grace: tid-monotone is **a DAG by
   construction — can't fail, proves nothing.** EMPTY CONFIRMATION, committed *inside an audit about
   instrument validity.* Do not quote depth 64.
2. **K1802's mixed-semantics elimination argument INVALID.** Cal: restricting to one label cannot exclude
   mixing WITHIN that label. **The instrument I used to rule out the artifact could not have detected it.**

## ACCEPTED — Cal's POSITION vs VALUE narrowing (good news, preserves banked work)
Graph records **which** theorems relate (position) **reliably**, **which way** (value) **unreliably**
⟹ **adjacency VALID, dependency INVALID.** Neighbourhood/co-occurrence results SURVIVE. K1802's
"no derivation-order statistic is trustworthy" narrows to *dependency direction only.*

## Refinement of Cal, from the engine
He read `derived` as one label carrying a relation AND its converse in simultaneous use. **The ENGINE
admits only one sense** — the second entered via later hand-authored edges. ⟹ **there IS a correct
convention to restore.** His operational conclusion stands: **no flip event to find.**

## Still OPEN, honestly labelled
reciprocal artifact (472 pairs) explains **342 of 1207** — *proved* tooling.
**865-node residue: OPEN, mechanism unknown** — asserted, not proved.
**Lead (Cal): T1230 "BST Analyzer CLI" inside a derivation cycle ⟹ suspect node-type contamination.**

## Repair instrument — both alternatives refuted
Only valid instrument = **registry-prose audit against the pinned convention**.
NOT tid (Cal: T78/T75 correct maths, high tid precedes low). NOT majority vote (Grace: T1 in=52/out=14).

## Prompt corruption SOLVED (Casey's fix)
Write the prompt to a FILE, relay a short pointer. `notes/.running/wake/R59_TEAM_PROMPT.md` is canonical;
it contains the FULL text Lyra and Elie were missing (her items 1–3 + the exact series question; his
three dropped clauses). **Both refused to reconstruct from garbled text — correct both times.**

## Self-corrections, running total this session
Untested cause asserted in a CRITICAL ruling (K1802→K1804) · adjective-class error on "strong fraction"
(caught inside K1805) · band widened after better data (disclosed) · **empty confirmation on the spine
(Grace)** · **invalid elimination argument (Cal)**. Five. All caught before anything shipped.
**Two of the five were caught by teammates, not by me — which is the argument for the seat.**

— Keeper, addendum 2, 2026-08-22. Nothing pushed.

---

# ADDENDUM 3 — K1807. The derivation layer is a fossil.

## The finding (backup series used as a time-series instrument — nobody had)
30+ dated snapshots 2026-07-02 → 08-22, plus April 3. **`derived` edges FROZEN at EXACTLY 6833 across
every snapshot for seven weeks**, while nodes grew 2217→2349 (**+132 theorems**) and non-derived edges
grew 3097→3331 (+234, all hand-labelled provenance).
> **132 new theorems, NOT ONE `derived` edge.**
**The entire recent corpus — the whole CKM/mixing arc, the QM package, the descent, all T2500+ — has NO
derivation edges.** This is the CAUSE of Grace's own weeks-old note that the derived SU(3) cluster is
disconnected from the strong-sector sub-graph. She saw the symptom; this is the mechanism.

## Forensic window — my own next hypothesis, REFUTED by my own test
I was forming: "`derived` was bulk-applied to the April adjacency set." **Tested and refuted** — April
has **3** reciprocal pairs, today **472** ⟹ **469 of 472 introduced between April 3 and July 2** (derived
grew 1232→6833). **No snapshots inside that window = resolution limit, cannot narrow further.**
Recorded the refutation, not the hypothesis. **Cal not refuted, only LOCALIZED** — two hands with opposite
readings inside one bulk-authoring phase fits exactly.

## Good news, substantial
**The reciprocal defect is HISTORICAL, FROZEN, FINITE.** Nothing since July 2 added to it. **A closed
object, not a live process** — repairable once rather than continuously.

## STRATEGIC — priority inversion recommended to Casey
Repairing 2410 orientations buys a correct map **through July 2 only**, while the program's most important
seven weeks sit outside the layer entirely.
> **Ingest the last seven weeks under the K1806 pin BEFORE repairing historical orientations** — or the
> ingestion re-breaks the repair.

## Also established this stretch
**Grace's "registry-prose audit" — the instrument she named as the ONLY one returning a true positive —
is largely NOT RUNNABLE.** Measured: only **305 of 1757 registry rows (17%)** name another theorem in
prose, and only **120 carry a directional cue word**, against **6833 derived edges**. It can decide at
most **~1.8%**. The registry has NO dependency column — columns are tid | description | status | toy |
date. **Ground truth for edge direction may not exist anywhere**; the `uses`/`used_by` arguments were
passed at registration time and only the resulting edges survive. **There is no event log**
(`ac_graph_events.jsonl` absent) despite the engine being event-sourced.

## Prompt delivery SOLVED
`notes/.running/wake/R59_TEAM_PROMPT.md` is canonical and now carries K1807. Casey's fix: write the file,
relay a pointer. Should have reached for it three rounds earlier.

## Self-corrections, running total this session: SIX
untested cause (K1802→K1804) · adjective-class "strong fraction" (inside K1805) · band widened after
better data (disclosed) · **empty confirmation on the spine (Grace caught)** · **invalid elimination
argument (Cal caught)** · **bulk-apply hypothesis refuted by my own test before filing (K1807)**.
Two caught by teammates, four by me. The last one is the pattern I want: tested before it reached a verdict.

— Keeper, addendum 3, 2026-08-22. Nothing pushed.

---

# ADDENDUM 4 — K1808. SEAL OPENED. ALL FIVE MISSED. THE CKM SECTOR CLOSES.

## The result
Lyra filed first, in writing, from the rail. Hash `43ad5eb3…f43488` **verified intact.**
Against pinned band [0.081, 0.108]: **S1 0.2500 · S2 0.2000 · S3 0.3276 · S4 0.3158 · S5 0.3571.**
> **ALL FIVE MISS, all high, 2.1×–3.8×. Pre-registered, sealed, opened — the strongest negative we make.**

## Lyra steps 1–3 VERIFIED EXACTLY (the physics that survives)
Q^{2k}|even = S^k (k=1..11) · char poly **λ³−5λ²+6λ−1**, CH residual **0.00e+00** · every series collapses
to **G|even ≡ βS + αS² + γ·1** · **corner ratio = t/(1+4t), γ ABSENT.**
**⟹ DERIVED: corner suppressed by exactly one power of t vs the subdiagonal (0→4 is four rungs, 2→4 is
two). "Why is |V_ub| so much smaller?" is DEAD — derived.**

## Corrections I issued
- **Step 4 WRONG**: [0,4/9) is not the range — pole at t=−1/4, unbounded (t=−0.26 → +6.5). Correct:
  **t ≥ 0 ⟺ ratio ∈ [0,1/4).** **Her ORIGINAL "<1/4" was RIGHT; the JUSTIFICATION failed, not the number.**
  → banked memory: *when the reason is wrong, don't assume the number is too.*
- **Denominator-collapse inference WRONG as applied to my seal**: 5 candidates pin **5 distinct t**
  {∞, 1.0, −1.055, −1.2, −0.833}. Structural claim right, inference wrong. Seal was a real 5-trial declaration.

## Corrections issued AGAINST me — both accepted
- **Cal convicted my pin's exhibit.** T78 Entropy Chain Rule is a **depth-0 identity**; T75 Shearer is
  proved FROM it ⟹ {78→75} is CORRECT, my conviction a **false positive.** I defended the pin with "it has
  teeth" and **the teeth rested on one exhibit, which was wrong** — over-claiming falsifiability from a
  single instance, *while auditing someone else's falsifiability.* Pin STANDS (engine primary);
  falsifiability RE-OPENED. Cal's replacement {186→78} adopted.
- **Grace**: my depth-64 withdrawal "wasn't a clever catch, it's your own rule applied one read earlier."

## Cal's cold-read ANSWERED — **t is the new ε**
γ severs correctly (I[1,3]=I[2,3]=0 verified). **But t=α/β rescales as t·c² under Q→cQ** (c=2 moves ratio
0.0938→0.1765). Corner ratio of a given G is INVARIANT; t is a COORDINATE.
→ banked memory: *quote the invariant, not the coordinate* (ε, χ-measure, t = three in three rounds;
σ_χ(G) untouched by all).

## Rulings issued
- **"No circular derivations found" does NOT enter the Guide** — 4.1% coverage, ships as weak.
- **Grace's `Prereq:` field ADOPTED** — one line per new theorem, no backfill. **Cheapest structural fix
  on the board; outranks the orientation repair.** It's my own DEFECT-I pattern: the load-bearing relation
  has no symbol, so it can't be checked.
- **Elie's complex-χ restatement accepted** — pin inherited not chosen; 95th unmoved, only 5th moves 3×;
  Grace's "~10% grading" survives at 0.090. **The pin cost her nothing.**

## ⟹ SECTOR CLOSES, STRONGER THAN IT OPENED
DERIVED: skeleton/rank-1 · λ=1/√20 · CP existence · flavor-universality=partial-isometry · **+ THE ORDER.**
NEGATIVE (pre-registered, sealed, opened): **all 5 natural candidates miss 2.1×–3.8×.**
INPUT: the value (ratio ≈0.093 ⟺ t∈[0.120,0.190] at P₆ integer norm) · δ_CP · V_cb value retired/position kept.
**Count unchanged, content sharper, negative now PRE-REGISTERED. RECOMMEND REDIRECT.**
Casey's call: atlas #125 · strong sector · the descent.

## Prompt delivery
`notes/.running/wake/R60_TEAM_PROMPT.md` canonical. File-not-relay works; both blocked CIs recovered fully.

## Self-corrections, session total: SEVEN
untested cause · adjective-class "strong fraction" · band widened (disclosed) · empty confirmation (Grace)
· invalid elimination argument (Cal) · bulk-apply hypothesis refuted by my own test · **pin exhibit false
positive (Cal)**. Three caught by teammates, four by me.

— Keeper, addendum 4, 2026-08-22. Nothing pushed.
