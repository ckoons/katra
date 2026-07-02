# Sundown — Lyra, Vol 42
**2026-07-02 Thursday, MID-YEAR DAY (date-verified, EOD 18:20 EDT)** · the day the mid-year hygiene turned a **claimed 10 into a σ-scored, honestly-tiered 8** — the down-row exposed as GUT-scale texture (structural-MISS), the σ-metric adopted as the scoreboard's honesty axis, my alternative-to-Pauli found *correct but known* (Günaydin–Gürsey 1973), and the discipline fired on **my own work three times**. The number got truer at every step.

## Where I am (EOD)

The most substantial single day of the program, and its signature is honesty under scrutiny. The whole team reconciled artifacts that had drifted ~4 weeks (ledger, graph, registry), rebuilt the scoreboard on three tools that can't flatter us (SOD check, almanac, σ-scored derivation ledger), and re-tiered on primary-sourced values. I produced **F449–F455**. Three of my own claims got walked back by the discipline — and each walk-back made the number truer. That's the day.

## THE count: 10 → σ-scored 8 (the mid-year correction)

Grace reconciled the stale ledger (v0.29 June 6) to **v0.30**; Cal cold-read; Keeper certified (K639). The "10" was drift — a muon double-count + an over-banked tau (49·71, demoted to candidate). Honest count **8**, now σ-tiered (Keeper's ledger):
- **1 exact:** θ_QCD (0σ).
- **4 statistical MATCH:** θ_QCD, **m_s/m_d = 20 = rank²·n_C (0σ!)**, Cabibbo, θ₁₃.
- **4 good APPROX (not "done"):** muon (24/π²)⁶ (0.003% but 1550σ — TIER-2 structural floor, terminal), α = 1/137 (Wyler, 0.6 ppm but ~4000σ), tau 49·71, m_t (0.7σ scheme-aware).
- **3 MISS:** the down-row (structural-MISS-but-**repairable**).
- **~5 derived-strong** (σ-MATCH ∧ not-cheap ∧ mechanism-forced): θ_QCD, θ₁₃, δ_CKM = arctan(√n_C), V_us, m_t. That's the number that matters, not "13 nominal MATCH."

## My findings F449–F455

- **F449** — Casey's color-as-3D-frame seed: color (su(3)⊂g₂⊂so(7)) and spacetime (so(3,1)⊂so(4,2)⊂so(5,2)) share the real home **so(5,2)≅so(7)**, BUT the naive color-SO(3)=spatial-SO(3) is **obstructed** (the 7 branches 3+3+1 vs 3+4·1). It's a *map* (manifestation), not an identity; the "same 3" is Λ³ (the volume form), not the rotation group. 27 = 3-frame tensor tower threads both frontiers.
- **F450** — the down-row **GJ Five-Absence verification** (count-affecting): the color-parity mechanism is group-theoretically GUT-free (no SU(5)/45-Higgs), BUT the VALUES {3,1/3,1} are the GJ **GUT-scale** ratios, and m_quark/m_lepton is NOT RG-invariant (F444 lesson). Physical {9.1, 0.88, 2.35} ≠ {3,1/3,1}. Staged → **structural-MISS** (Keeper K642).
- **F451/F452** — alternative to Pauli: exclusion = exterior-algebra nilpotency (v∧v=0); the color Fock space Λ*(color-3) = the 2^{N_c}=8 SO(7) spinor sector (color-3 is the maximal isotropic subspace — a *theorem*), grades {1,3,3̄,1} match QCD (diquark=3̄). **BUT walked all "new" back with the literature: it's the Günaydin–Gürsey octonionic quark model (1973) + the algebraic-SM program (Furey, Dixon, Lasenby, Stoica). Correct but KNOWN. "New for nuclear physics" WITHDRAWN.** A consistency win + citation bridge, not a discovery. [[feedback-verify-symmetry-kill-is-a-theorem-not-analogy]] working.
- **F453** — the **regularized determinant** det(R+s·I)=s^k·det'(R): ONE operation resolving Elie's K638 Flag 2 — lepton (R curved, k=0) → value det R (π, muon); quark (R=0 flat) → residue = zero-mode count (integer). Color is the switch because color *flattens* R. Aims the down-row **repair** at the observed ladder (m_s/m_d = 20), not the failed GJ. Unblocked Grace's d₂. Verified by Elie (toy 4545).
- **F454** — θ₁₃ re-pin (my bank): **0.47σ CONSISTENT, not a MISS**; "firm 0.10%" was a favorable NuFIT variant → corrected. **THE METHODOLOGY WIN: score σ = dev/experimental-error, not raw dev%** — both my θ₁₃ (0.47σ, wrongly MISS by dev%) and Elie's m_s/m_d (1.24σ, wrongly "nail") show it. Now Keeper's ledger scoring axis. Saved [[feedback-score-sigma-not-devpct]].
- **F455** — Pass-2 mechanism (the prize): BST mixing forms are **matrix-element magnitudes |U_ai|²** (overlaps), not angle-parameters. Forces the θ₁₂ divide (|U_e2|²=3/10 → sin²θ₁₂=0.3068=0.02σ) and **un-cheapens** it (3/10→0.02σ vs 5/16→1.05σ, degeneracy broken). **WITHDREW my own Pass-1 θ₂₃ ×cos²** (opposite direction = fished coincidence). Reusable across the overlap sector.

## New tools & discipline adopted today

- **σ-scoring** (mine, F454) — the ledger's honesty axis; carries each 1σ scheme-aware.
- **SOD artifact-currency check** (Keeper #28) — `play/keeper_sod_artifact_check.py`, run FIRST every session (proposed CLAUDE.md line 0.5, pending Casey OK).
- **derivation ledger** (`play/bst_derivation_ledger.py` / `bst_26_table.py`) — per-value attempt-lists, σ + cheapness + mechanism axes; **no CI self-certifies**, the tool judges.
- **26-derivation-loop** — parallel buckets (Lyra mixing / Grace masses / Elie scales), scan→fill→derive, then NON-MATCH pipeline (plan→work→check, independent checker).
- **cheapness** = soft-flag (search-space-relative, K631-S1), not a counted axis.
- **matrix-element mechanism** (F455) — read mixing forms as overlaps |U_ai|², not parameters.

## Discipline fired on MY OWN work — three times (the day's signature for me)

1. **F451/F452 "new"** — walked all the way back with the literature (Günaydin–Gürsey 1973). Consistency win, not discovery.
2. **θ₁₃ "firm 0.10%"** — corrected to 0.47σ (favorable-variant artifact); the bank stood, the headline died.
3. **θ₂₃ ×cos²θ₁₃** (my Pass-1) — WITHDRAWN as a fished direction (opposite to θ₁₂'s forced divide).

## Open frontiers carried to next session

1. **Down-row REPAIR** (Grace's d₂, my F453): is **k₂/k₁ = 20 forced** by the KW rank-1 stratum geometry? I handed Grace the pinned **Korányi–Wolf boundary-component reference** — rank-1 component is the **disk IV₁** (not IV₃; her IV₃ is the V₁ Peirce-½ space, dim n−2=3), strata IV₅→IV₁→point, Shilov = Lie sphere 𝕋·𝕊⁴. She verifies vs Loos fresh. If k₂/k₁=20 forced → down sector derived-strong.
2. **My NON-MATCH pipeline (Stage 1 done, WORK next):** m_μ/m_e CLOSED (terminal-APPROX, structural floor — transcendental, mechanism forces form not exactness). ν₂/ν₃ via K547 deposit-locus (ν₂ π-ful, ν₃ π-free, F446; scale=FLOOR). **δ_PMNS via the leptonic Jarlskog** (dual-ρ kernel complex phases, forced route per F455 — replacing my Pass-1 value-form guess; data ±40° so mechanism-only, no data-match). Checker: Grace.
3. **Registry backfill:** 5/16 done (T2490–2494); 11 queued (T2495–2498, T2501–2506) — careful K-audit extraction, fresh.
4. **α frontier:** Elie pinned Wyler's exact form (0.6 ppm, mechanism-forced but ~4000σ APPROX). GUT-numerology surface (27=E₆, SO(10)-16, θ₁₃'s 45=SU(5)) flagged for a coordinated Five-Absence pass.

## Pending Casey decisions (five, none finalized in my absence)

Pass-1/2 greenlight (in action) · **σ-metric ratification** · **down-row FIRM→structural-MISS-repairable** · α tier-call (better-informed: Wyler mechanism pinned) · **CLAUDE.md line 0.5** (SOD check first).

## How to wake (Vol 43)

Read MEMORY.md → this sundown → boards **CI_BOARD_2026-07-03_NONMATCH_pipeline** + **CI_BOARD_2026-07-02_PASS_2** + MESSAGES_2026-07-01.md (F449–F455 + full team). **Count is 8, σ-scored** — run `python3 play/keeper_sod_artifact_check.py` FIRST, then `bst_26_table.py` for the live scoreboard. Next session opens into the WORK stage: my ν-mass forms (K547 locus) + δ_PMNS (leptonic Jarlskog, forced route), and Grace's d₂ / KW-Peirce (down-row repair — is k₂/k₁=20 forced?). The day's lesson, carried: **compute the honest metric (σ) before you tier, and let the discipline fire on your own most-elegant work — three of my walk-backs today made the number truer.** A smaller, truer number beats a claimed one.

— Lyra, Mid-Year 2026-07-02 EOD (18:20). Claimed 10 → σ-scored 8 (~5 derived-strong). F449 (color/space map not identity) · F450 (down-row GUT-scale texture → structural-MISS) · F451/F452 (Pauli=exterior nilpotency, color Fock=SO(7) spinor — KNOWN, Günaydin–Gürsey, "new" withdrawn) · F453 (regularized determinant, down-row repairable) · F454 (σ-metric, θ₁₃ defended at 0.47σ) · F455 (mixing forms=matrix elements, θ₁₂ forced, θ₂₃ withdrawn). σ-scoring adopted; SOD check standing; three self-corrections. Down-row repair (k₂/k₁=20?) + my ν-mass/δ_PMNS forced routes carry to Vol 43.
