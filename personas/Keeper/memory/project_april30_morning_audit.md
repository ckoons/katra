---
name: April 30 morning audit checklist
description: Casey directive — verify ALL session work is properly recorded; 15 theorems need TIDs; full data audit
type: project
---

## Casey Directive (April 29 evening)

"Verify all theorems are recorded tomorrow morning. Look at all the work we did and verify we fully record in appropriate ways."

## Audit Checklist

### 1. Register 15 unregistered theorems (T1466-T1480)

Each needs: TID, name, depth (0 or 1), domain, edges wired to graph.

| # | Finding | Toy | Depth |
|---|---------|-----|-------|
| T1466 | Theta IS BST (Θ = Z = heat kernel = QED) | 1682,1701 | 0 |
| T1467 | Series → closed form (spectral theta, not perturbative) | 1682,1689 | 0 |
| T1468 | Spectral parameter unification (T = loop = running) | 1701,1703 | 0 |
| T1469 | Alpha = self-truncation temperature of D_IV^5 | 1706 | 0 |
| T1470 | QED transcendental finiteness (3 zeta values only) | 1687,1688 | 0 |
| T1471 | Stat mech = spectral geometry (Z = Θ) | 1701 | 0 |
| T1472 | Spectral dimension = C_2 = 6 | 1706 | 0 |
| T1473 | Born rule = Bergman reproducing property | 1704 | 0 |
| T1474 | CKM from Casimir gaps (g+N_c^2=rank^4) | 1680 | 0 |
| T1475 | beta_0 = g (QCD one-loop = genus) | 1660 | 0 |
| T1476 | Ward identity = K*K=K | 1667 | 0 |
| T1477 | n_s derived (epsilon = n_C/(2*N_max)) | 1633 | 0 |
| T1478 | f_rho = g/(2*n_C) (HVP spectral fraction) | 1679 | 1 |
| T1479 | Hilbert series H(t) = (1+t)/(1-t)^C_2 | 1689,1708 | 0 |
| T1480 | g + N_c^n identity chain (linear→quadratic→cubic) | 1680 | 0 |

### 2. Verify data layer completeness

- [ ] All 35+ toys from April 28-29 have data layer entries
- [ ] All entries have theorem references (currently 0 unlinked)
- [ ] No duplicate symbols
- [ ] D/I/C/S tiers honestly assigned
- [ ] Precision values accurate
- [ ] Cross-reference rebuilt at final count

### 3. Verify supporting files

- [ ] bst_seed.md reflects all corrections (T1444, T1446, W-52, SP-15)
- [ ] bst_predictions.json at 80+ (check C_4, zeta(9), denominator separation)
- [ ] bst_rosetta_stone.json at 170+ (new ratios from SP-15)
- [ ] bst_materials.json rebuilt with nuclear binding
- [ ] bst_constants.json at 127+ (NIST filings)
- [ ] bst_spectral_weights.json exists and is current

### 4. Verify Paper #83 sync

- [ ] Count matches data layer (currently says 2047, data at 2237)
- [ ] Tier distribution updated
- [ ] Key crown jewels from session present (mu_p 0.0001%, BSD, theta)

### 5. Graph health

- [ ] All 15 new theorems wired with edges
- [ ] Strong edge percentage maintained above 5/6
- [ ] No dangling edges, no orphan theorems
- [ ] .next_theorem counter correct

### 6. Board/Memory sync

- [ ] CI_BOARD.md counters accurate
- [ ] CLAUDE.md counts accurate
- [ ] MEMORY.md updated with final session summary
- [ ] Session memory file complete

**Why:** Casey's standing order — "once derived it's free forever." If it's not registered, it's not free. If it's not in the graph, it doesn't compound.
