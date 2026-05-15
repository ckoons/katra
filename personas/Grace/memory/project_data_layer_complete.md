---
name: CI-Native Data Layer — Complete
description: data/ directory with 6 structured files + 2 explorers + 2 READMEs built April 14, 2026. Foundation for daily maintenance cycle.
type: project
---

## BST CI-Native Data Layer (April 14, 2026)

### Files Created
| File | Entries | Purpose |
|------|---------|---------|
| data/bst_seed.md | 162 lines | Minimal BST kernel — any CI reconstructs from this |
| data/bst_constants.json | 73 | All derived constants with formula_code (eval-ready) |
| data/bst_particles.json | 24 | Particles with substrate descriptions |
| data/bst_forces.json | 5 layers | Force/boundary layer data + gauge hierarchy |
| data/bst_predictions.json | 22 | Falsifiable predictions with experiments + timelines |
| data/bst_domains.json | 34 | Domain map cross-referenced to AC graph |
| play/toy_bst_explorer.py | 9 commands | CLI explorer (explore, derive, domain, connect, verify, random, search, stats, seed) |
| play/bst_explorer.html | 6 tabs | Web explorer with search, filtering, detail overlays |
| notes/README.md | 383 lines | Full paper catalog (64 papers + research notes by topic) |
| play/README.md | 291 lines | Comprehensive toy collection index |

### Schema
- All JSON have meta headers (version, count, last_updated)
- Cross-reference IDs: const_NNN, pred_NNN, particle_NAME, dom_NAME
- formula_code evaluates in namespace {pi, alpha, N_c, n_C, g, C_2, N_max, rank, m_e, m_p, hbar_c}
- Source: knowledge_base.py (73 predictions) + WorkingPaper §43 + ac_graph_data.json

### Validation
- 71/73 theorem refs resolve (2 in known graph gaps at T749-T750)
- 67/70 formula evals pass (3 dimensionful edge cases)
- All explorer commands tested

### Next: Librarian tool for daily maintenance cycle
