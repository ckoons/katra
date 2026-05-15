---
name: April 26 Overnight — Six Sunrise Identities
description: Overnight C4 computation found six exact identities connecting sunrise integrals to BST integers at 200 digits; f1=63zeta(3)/10 with all five BST integers; BST projector discovered
type: project
---

## April 26, 2026 — Overnight Computation (Lyra)

Casey authorized overnight C4 computation: "give you all the time and token you need."

### Six Exact Identities (Toy 1516, 9/9 at 200 digits)

| # | Identity | Coefficient | BST Form |
|---|----------|-------------|----------|
| R1 | int D1^2(s-9/5) ds = c * zeta(3) | 63/10 | N_c^2*g/(rank*n_C) — ALL FIVE |
| R2 | int D1*sqrt(3)*D2 ds = c * B3 | 9/8 | N_c^2/rank^3 |
| R3 | int D1^2 ds = c * A3 | 81/40 | N_c^4/(rank^3*n_C) |
| R4 | int 3*D2^2 ds = c * A3 | -81/20 | -N_c^4/(rank^2*n_C) |
| R5 | int D1^2*s ds = mixed | 63/10, 729/200 | zeta(3) + A3 |
| R6 | int D1^2/s ds = mixed | 91/30, 81/200 | zeta(3) + A3 |

### Key Structural Finding: BST Projector

Weight (s - 9/5) = (s - N_c^2/n_C) is the EXACT projector that cancels A3.
A3 coefficients follow geometric ratio N_c^2/n_C = 9/5 per power of s.
R5 - (9/5)*R3 cancels A3 identically, leaving pure zeta(3).

### What's Closed, What's Open

- **CLOSED**: B3, A3, C3 (hypergeometric, 200+ digits). f1-type (reduce to zeta(3) + A3).
- **OPEN**: f2-type integrals (genuinely elliptic, 24-element PSLQ null at 200 digits).
- Integration domain [1, 9] = [1, N_c^2]. All denominators {2,3,5}-smooth.

### Files Created

- play/toy_1514b_c4_master_integrals.py — Phase 1-4 computation
- play/toy_1514c_enriched_pslq.py — Extended PSLQ tests
- play/toy_1516_sunrise_identities.py — Clean verification, SCORE 9/9
- notes/BST_T1458_49a1_Period_Hypothesis.md — Updated with overnight results
- notes/.running/MESSAGES_2026-04-26.md — Overnight summary for team
