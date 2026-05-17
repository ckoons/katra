# Lyra Post-it — standing reminders (auto-loaded each turn)

*Edit me when something new is worth remembering across turns. Keep short — every line costs context per turn.*

## Discipline (Casey standing orders)

- **Ambient time live** (IQ-3, 2026-05-17): every turn opens with `[NOW] YYYY-MM-DD Day HH:MM ZZZ` via the readTime hook. Trust that line first; no need to call `date` unless verifying.
- **Never push to git without Casey's explicit OK.** Commit locally is fine.
- **Atomic counters only**: `./play/claim_number.sh toy` / `./play/claim_number.sh theorem` — NEVER read `.next_toy` / `.next_theorem` directly.
- **No section sign character.** Write "Section X" or "Sec. X".
- **Catalog every derivation same session (SP-14).** File to `data/bst_constants.json` or `data/bst_geometric_invariants.json`.
- **Tier discipline (Casey May 16 evening)**: D-tier requires a derivable D_IV⁵ mechanism. Cultural / cognitive / linguistic / music / human-designed-taxonomy counts are explicitly S-tier, NOT D.

## Governance (May 17)

- **Keeper controls promotion / demotion.** Cal is a reviewer with opinions. Team votes; Keeper's call is final.
- **Five architectural categories** (Sunday EOD state — Keeper rulings + Grace's Bridge Objects):
  - **L1 source** = single classical theorem GENERATES integer structure. **8 established**: VSC, Mathieu, Klein, Goeppert Mayer, Heegner-Stark (K47 promoted), K3 Hodge, Ogg, Wallach
  - **L1 candidate** = source with promotion criteria still open (Conway, awaits umbral moonshine closure)
  - **L1.5 mechanism** = named theorem UNIFIES pre-existing structures (Borcherds 1992 = L1.5b, McKay 1979 = L1.5c)
  - **Convergence hub** = OBJECT that multiple L1 sources describe (Monster primary; Leech secondary)
  - **Bridge object** = specific BST-geometric object that classical theorems "land on" to reach D_IV⁵ (K3, 49a1 elliptic curve, Q⁵). Grace's category 2026-05-17, Keeper-endorsed.
- **Cal's three promotion criteria** (Heegner/Mathieu/future-candidate): embedding, mechanism, forcing.

## BST primaries (quick reference)

- `rank=2, N_c=3, n_C=5, C_2=6, g=7`
- Derived: `c_2=11, c_3=13, N_max=137, χ=24=rank³·N_c`
- D_IV⁵ = SO_0(5,2)/[SO(5)×SO(2)]; complex dim 5; real dim 10. Q⁵ same.

## Tools / paths

- **/pdf** (skill): pandoc + xelatex, header `notes/bst_pdf_header.tex`, margin 1in. Skip MEMORY / CLAUDE / MESSAGES* / CONSENSUS* / CI_BOARD / RUNNING_NOTES / archive / `.running/`.
- **Every toy needs SCORE line.** Format: `SCORE: N/M`.
- **Theorem registry**: `notes/BST_AC_Theorem_Registry.md`.
- **Team broadcasts**: `notes/.running/MESSAGES_YYYY-MM-DD.md` (auto-loaded by checkBoard).
- **Speculative work**: `notes/maybe/` (not main `notes/`).

## Architecture identity (so I don't lose it)

- I am **Lyra**, Claude Opus 4.7. BST research collaborator. Persona file: `/Users/cskoons/projects/github/katra/personas/Lyra/`.
- Memory directory: `/Users/cskoons/.claude/projects/-Users-cskoons-projects-github/memory/`.
- Memory index: `MEMORY.md` (loaded at session start).

## Active reminders (edit freely; clear when no longer relevant)

- Sarnak letter v6 ready for Monday May 18 send. Lyra review filed at `notes/maybe/Letter_Sarnak_May17_Lyra_review.md`; Keeper handles integration.
- Paper #115 v0.3 closed at framework level; Cal grade-pass pending; v0.4 will add Mathieu Root #5 + Type A/B subsection.
- Mathieu Root #5: Grace EOT toy 2976 = 10/12 PASS. Keeper tier ruling pending (established vs candidate).

## End of post-it (~50 lines max — keep tight)
