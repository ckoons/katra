# Sundown — Lyra, Vol 57

**2026-07-15 Wednesday (date-verified, EOD 16:53 EDT).** **The correction to Vol 56: the goal-line mixing run MISSED, and the miss localized the physics cleanly — masses are done; mixing is a DISTINCT geometric object (angular, not radial) that was never in the ground data.** Vol 56 said "one run from done." Grace ran it. PMNS came out ≈ 0 (should be large), CKM small-but-off. This sundown supersedes Vol 56's "one run from done" claim. The day did not end in a false render — it ended pointing straight at where mixing lives.

## What actually happened (the correction)

- **Grace's goal-line run missed (honest, not fabricated):** on the audited grounds, PMNS ≈ 0, CKM small-off. She diagnosed it precisely and refused to blame the down ground or fake a render. She also flagged her own machine's reliability (earlier "verification" was one hand-picked example; systematic pure-Gram gives small mixing). Same discipline I modeled in F552.
- **My F552 "broken self-check" probably WASN'T broken.** It gave θ₁₂ ≈ 0 for every input; I filed it as a broken diagnostic because the answer offended my expectation of large angles. Grace's *independent* systematic pure-Gram run gave the same ≈ 0 from norm-differences. Two independent constructions converging on ≈ 0 is not two bugs — it's the geometry reporting a real structural fact. **Update: I read "broken" into a correct result.** Own it.

## The reframe (today's real content — the load-bearing part)

**Mixing is eigenVECTOR misalignment, not eigenVALUES. Textbook:** diagonalize a sector's mass matrix M = U Σ V†.
- **Σ = the masses = the radial norms {N_i}.** (That's why 14 banked + 6 identified-strong all land — they are all Σ's.)
- **Mixing = the relative left-rotation between two sectors' eigenbases:** V_CKM = U_L^{u†} U_L^{d}, V_PMNS = U_L^{ℓ†} U_L^{ν}.
- If two sectors share their left-rotation, mixing = identity → **PMNS ≈ 0.** Cleanness of the norms is irrelevant, because **mixing was never a function of the norms.**

**So the honest finish-line state is crisper than Vol 56's, and better:**
> **The masses are done. Mixing is a distinct geometric object we haven't built yet.** The grounds are *complete for masses*. The map "norms → mixing" didn't underperform — it doesn't exist, because mixing isn't a function of norms. This localizes ALL remaining work to one well-posed question.

**Casey's "duh" that sharpened it — you need positions AND mass:**
- Every generation state is a full vector v_i in D_IV⁵: **mass = |v| (radial), mixing = ∠(v^A, v^B) (angular).**
- We projected v_i ↦ |v_i| = N_i when we built the grounds — right for masses, but it **averaged over exactly the coordinate that carries the angle.** The mixing info was discarded by US (the radial projection), NOT by nature. **It's recoverable: stop discarding it.** Go one step back upstream, carry the full position, project to the norm only at the end for the mass, and read the angle for the mixing.
- Ladder picture: each generation has an E-level (radial → mass, PINNED) *and* a location on the Shilov boundary (angular → mixing, THROWN AWAY). The mixing matrix is the relative orientation of the two sectors' boundary positions.

**Candidate maps between sectors — already in hand (F548/F549 + d_eff rule):**
- **Up ↔ down (CKM):** negative refraction, index n = N_c/rank. Refraction rotates *directions* → candidate for U_L^{u†}U_L^{d}. Small rotation (within color) → naturally *small* CKM. Bridge back to norms: Gatto–Sartori–Tonin, sin θ_C ≈ √(m_d/m_s) — angle ties to a mass ratio THROUGH the texture, so norms re-enter as √-ratios, not directly.
- **ℓ ↔ ν (PMNS):** sectors differ by a whole dimension and a charge (ℓ is d=5 charged, ν is d=4 chargeless). Bigger reorientation than a within-color refraction → naturally *large* PMNS. The large-PMNS / small-CKM split falls out of "how far apart the two sectors sit," qualitatively, before any number.

## Team state (finish line — corrected)

- **α CLOSED — real, holds.** 4π = the SO(5,2)→SO(3,1) descent's Coulomb solid angle (Grace's capstone + your Gauss's-law audit, Keeper K701). Don't reopen.
- **14 banked + 6 identified-strong (m_H 0.02%, m_b=m_t/42, m_ν2, m_ν3, m_d, m_u) — ALL masses (Σ's).**
- **The last 6 (V_us, V_ub, δ_CKM, PMNS θ12/θ23/δ_PMNS) OPEN — behind the ANGULAR object, not behind one more run.** No "one run from done." The mixing construction has to be built.
- E₀ = 2 spinor weight (F551) and the d_eff grounds still stand — they were correct FOR MASSES. They just don't carry angles.

## Vol 58 opener — my lane (the well-posed question)

1. **Build the per-sector angular/boundary position — the object mixing diagonalizes — upstream of the radial projection.** What is each sector's Shilov-boundary location / eigenvector direction? Not more norms.
2. **First concrete swing (compute, don't calibrate):** does the F548/F549 refraction rotation (machinery I already have) land the Cabibbo angle θ_C? sin θ_C ≈ 0.225. If yes, the geometry-large-vs-small-mixing story is real; if it misses, learn the range.
3. **Then ℓ↔ν:** does the d=5→d=4 projection give large PMNS? Test whether it re-opens my superseded neutrino E₀=1/2 direction (it may not need to — the angle may live entirely in the projection, not the radial drop).
4. **Joint pin with Grace/Elie/Keeper:** the exact object each sector's U diagonalizes (overlap Gram carries no angle; a full-vector / textured mass matrix does). Re-verify Grace's machine against a known-large-mixing input before trusting its output.

## How to wake (Vol 58)

1. `date` FIRST. Read MEMORY.md → THIS sundown (Vol 57, supersedes Vol 56's "one run from done") → Vol 56 for the mass-side wins → RUNNING_NOTES + Grace's corrected sundown + Keeper K701/K703. SOD.
2. **The arc, carried:** Vol 56 claimed the table one run from done. Grace ran it, it missed (PMNS≈0), and the miss was a GIFT — it localized mixing to the angular/eigenvector object we'd projected away. Masses done; mixing is a separate build. Casey: "you need positions AND mass." The info was discarded by our radial projection, not by nature → recoverable.
3. **The discipline, both ways:** Grace refused a false render; I own that my F552 "broken self-check" was likely reporting a real ≈0. *A wrong render hides where the physics lives; a clean miss points at it.* Better place to be stuck. Force from geometry; build the position before taking its length; the machine is the arbiter only after it's re-verified against known-large mixing; nothing banked past what's built.

— Lyra, 2026-07-15 EOD, Vol 57. **CORRECTION to Vol 56: the mixing gate is NOT rendered. Grace's run missed (PMNS≈0). The miss localized the physics: MASSES ARE DONE (all 14+6 are radial norms Σ); MIXING IS A DISTINCT ANGULAR OBJECT** — eigenVECTOR misalignment V=U_A†U_B, never a function of the norms. My F552 "broken self-check" probably wasn't broken; it converged with Grace's independent pure-Gram ≈0. **Casey's key: you need positions AND mass — mixing info was discarded by OUR radial projection (v↦|v|), not by nature; recoverable by carrying the full position.** Vol 58 first swing: does the F548/F549 refraction rotation land the Cabibbo angle? α CLOSED and holds. We'll get through it. 🌙
