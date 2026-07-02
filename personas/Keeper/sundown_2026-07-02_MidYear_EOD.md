# Keeper Sundown — Thursday 2026-07-02, Mid-Year Day

## The day in one line

A claimed **10** banks became a certified, σ-scored, honestly-tiered **8** — and the second half opens with a reproducible scoreboard that can't flatter us, a method for deriving all 26, and a partition to work them in parallel. The number got smaller and truer at every step, including corrections on the CIs' own findings and two on mine.

## The arc (what happened)

1. **Ledger reconciliation → count is 8, not 10 (K639).** My stale-ledger flag → Grace reconciled honestly (let it shrink, marked UNKNOWN, didn't back-fill) → Cal cold-read 8 → I certified 8 independently. The "10" hid a muon double-count (unnamed 5th) + an over-banked tau. Retirement-check clean.
2. **Artifact-currency discipline (K640) + SOD check tool.** All three artifacts (ledger/graph/registry) had drifted ~4 weeks in lockstep. Elevated to standing Keeper #28; built `play/keeper_sod_artifact_check.py` (runs first each session, directs drift to owners).
3. **Down-row GJ flag → structural-MISS (K641→K642→K643).** The down-row banks {3,1/3,1} are Georgi-Jarlskog **GUT-scale** values, 57-67% (6.5-80σ) off observed. Grace steelmanned and CLEARED the Five-Absence horn (substrate scale ≠ GUT, saturation artifact). Elie's m_s/m_d "nail" SOFTENED to 1.24σ on re-pin (I over-elevated it in K642; corrected). Net: GUT-free substrate texture that's a real value-MISS. **Repairable** via Lyra's F453 (regularized determinant) aimed at the observed ladder m_s/m_d = 20 = rank²·n_C (a 0σ match).
4. **σ-metric adopted (K643).** Three CIs converged same-session from opposite directions: dev% ≠ σ. θ₁₃ = 1.24% dev but **0.47σ = MATCH** (stays banked, "0.10%" headline withdrawn); muon/α = 0.003%/0.026% dev but ~1500σ/~1e6σ = **APPROX** (good closed forms, not exact); down-row = real MISS. **Keeper refinement:** σ needs **scheme-aware errors** — m_t reads 4.7σ on the tight pole error but **0.7σ** scheme-aware, so the error must reflect scheme ambiguity or the metric manufactures misses.

## Tools built today (the trust fix — computed, not CI-declared)

- `play/keeper_sod_artifact_check.py` — start-of-day artifact-currency check.
- `play/bst_almanac.py` — scorecard + almanac (how each derives), one source of truth.
- `play/bst_derivation_ledger.py` — **per-value attempt-list**, scored on dev% AND σ. `notes/BST_Derivation_Ledger.md`.
- **Casey's principle locked in:** "solid/done" is COMPUTED from the match, never spoken by a CI. Both axes (σ-match AND forced mechanism) must be clean.

## Honest state entering the second half

**Count 8, σ-tiered:**
- **Clean both axes (σ≤2 MATCH + forced mechanism):** θ_QCD (0σ exact), θ₁₃ (0.47σ), m_t (0.7σ scheme-aware, rests on v-floor).
- **σ-MATCH, mechanism pending:** Cabibbo (0.6σ).
- **APPROX (structural, many-σ):** muon, α, m_τ.
- **MISS but repairable:** down-row ×3.
- Ledger seed covers 11 of 26 (4 MATCH + 4 APPROX + 3 MISS).

## The method (Casey-directed, standing until "done")

**The 26-derivation loop.** Each value: a list of attempts (form + value + how-from-substrate + provenance), scored on dev%+σ, accumulating (never repeat). Terminal states: DONE (≤0.1% + forced) / FLOOR (pure scale) / NEG (runner/scheme). Done = all 26 terminal; Casey calls it.

**Pass 1 = PARALLEL (Casey directive, board ready):** partition the 26 so no one dog-piles one item.
- **Lyra — mixing (8):** CKM + PMNS angles & phases.
- **Grace — masses (9):** all charged fermion masses (down-row is 1 of 9, not the whole pass).
- **Elie — scales (9):** gauge + Higgs + θ_QCD + ν.
- Each: scan corpus → fill closed form → attempt independent derivation. Hand to Keeper; the ledger computes the verdict.

## Carry-forward for Pass 1 (fresh start)

1. **PDG re-pin, step 0** — value AND scheme-aware error, per bucket (Elie/Lyra CKM via pdgLive HTML; Grace masses/PMNS done). Even our dev%s used memory values.
2. **Grace's d₂:** k₂/k₁ = 20 forced by rank-1 stratum? Lyra handed her the Wolf-Korányi reference (rank-1 boundary = disk IV₁, not IV₃; the IV₃ is likely the V₁ Peirce-½ space, dim 3) — verify fresh against Loos, don't bank on unverified boundary theory.
3. Fill the 15 un-seeded values; push APPROX/MISS toward MATCH.

## Standing decisions for Casey (team idling on these; nothing finalized in absence)

1. **Pass-1 greenlight** (parallel partition, re-pin first).
2. **σ-metric ratification** — ratify it alongside the down-row (it makes "θ₁₃ hit, down-row miss" a computed verdict).
3. **Down-row ratification** — FIRM → structural-MISS-but-repairable.
4. **α tier-call** (Cal: α is the partial).
5. **CLAUDE.md line 0.5** — SOD check as first act (won't edit the shared file without OK).

## K-audits filed today: K638–K643. Plus 3 tools + almanac + derivation ledger + 3 board/method docs.

## Personal note

The mid-year did exactly what a midpoint audit should: it found a real over-count on the certified foundation and turned it honest. My own worst moment was certifying the down-row firm at K639 with the GJ flag *noted but not run* — I should have divided the two numbers before stamping firm, not the next day (K641). And I over-elevated Elie's m_s/m_d as "the nail" (K642) before the re-pin showed it was 1.24σ. Both are the same lesson the σ-metric now enforces on the whole team: don't tier before you compute, and compute σ, not just dev%. The tools are the fix — trust moves off the CI's word and onto a check Casey can re-run himself. That's "the math speaks for itself, it's on GitHub" applied to our own trust problem. Good mid-year. The second half starts on a truer number.

— Keeper, Mid-Year 2026-07-02 EOD. Count 8 σ-tiered; 3 tools built; 26-loop method + parallel partition ready; Pass 1 opens fresh with the re-pin. Sundown filed. Katra-ready.

## AFTERNOON ADDENDUM (same day, 2026-07-02, ~18:21 EDT)

The 26-loop ran Pass 1 (parallel partition) and Pass 2 (every-CI-every-item), then turned to the non-matches. Key outcomes:

**Tools:** `play/bst_26_table.py` = the live 26-row scoreboard (dev% + σ scheme-aware + coarse/tight precision-flag). `play/bst_derivation_ledger.py` + `bst_almanac.py` carry per-value attempt-lists.

**Pass-1/2 movement — mechanisms, not nominal MATCHes:**
- **θ₁₂ → FORCED** (Lyra): read as matrix element |U_e2|²/(1−|U_e3|²) — forces the value AND un-cheapens it (tells 3/10 from 5/16). New derived-strong.
- **α → FORCED** (Elie): Wyler exact form pinned in substrate primaries, 0.6 ppm (431× tighter than flat 137); still APPROX (~4000σ — a superb form, not exact).
- θ₂₃ withdrawal (Lyra caught her own cos²θ₁₃ fishing); sin²θ_W corrected MATCH→APPROX (11σ, mine on Elie's entry).

**K644 — match-cheapness axis**, then **corrected** (K645/Elie): the form-COUNT is search-space-relative (my own K631-S1 lesson, on my axis) → downgraded to a robust precision-based soft-flag (coarse rel-err>2% / tight <0.5%), NOT a counted axis.

**K645 — pressure-test of the "derived-strong 5":** shrinks to ~1–2 genuinely solid. δ_CKM self-corrected (coarse target, 4 arctan√k fit — mechanism-dependent, not value-expensive). θ_QCD topology FLAG (π₁ vs gauge π₃/instanton — needs contractible→trivial-bundle explicit, address S⁴). Only **sin²θ₁₃ solid on both axes**; θ₁₂ joined via matrix-element mechanism.

**K646 — non-match disposition map (radiative-floor principle):** a tree-level substrate form is expected to match at the ~0.01–0.1% correction floor, not exactly → an APPROX at its floor is TERMINAL (finished, not failed). The 11 non-matches split ~4 improvable (m_d/m_e, m_b/m_s repair to observed ladder; m_ν2/3; m_e disposition) + ~6 terminal/forward (muon/τ/α at floor; sin²θ_W runner; ν-scale FLOOR; δ_PMNS forward-pred).

**Honest 26-table at EOD:** 13 nominal MATCH but only ~2 derived-strong-and-clean (sin²θ₁₃ + θ₁₂-matrix); the rest carry named caveats. K-audits today: K638–K646.

**Method for next session:** non-match pipeline (plan→work→check, independent checker per item, rotated G→E→L→G). Improvable items get real effort; terminal items get closed with a stated reason.

**Timestamp-drift note (self-catch):** I dated the non-match board and K646 as 2026-07-03; actual date is 2026-07-02. The exact drift I flag in others — logged so next-session me corrects the filenames if it matters.

**Carry-forward #1:** Grace's d₂ / Korányi-Wolf-Peirce derivation (is k₂/k₁=20 forced?) — fresh-start geometry, the down-row repair hinge. Lyra handed her the Wolf-Korányi reference (rank-1 boundary = disk IV₁, not IV₃).

Standing for Casey (unchanged): Pass greenlight (running), down-row ratification (structural-MISS-repairable), σ-metric + match-cheapness ratification, α tier-call (now Wyler-informed), CLAUDE.md line 0.5.

— Keeper, addendum close, 2026-07-02 ~18:21 EDT. The number stayed small and true: 13 MATCH, ~2 clean, mechanisms the prize, terminal an honest answer. Katra-ready.
