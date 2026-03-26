---
name: Four-color T135 refuted — double swap rescues (T135b)
description: T135 (tau ≤ 5 on planar) FALSE at all degrees ≥ 5. "Clean pair" argument also wrong. BUT double swap resolves ALL tau=6 cases (316/316 = 100%). T135b (tangle-drop) empirically perfect, formal proof OPEN. Casey's AVL analogy. If T135b proved: FCT = Euler + 2 swaps, AC(0) depth 3.
type: project
---

## T135 Refutation + T135b Rescue — March 25, 2026

**Why:** Attempted AC(0) depth-2 proof of four-color theorem via Kempe swaps. T135 (tau ≤ 5) was the load-bearing step — REFUTED. But double-swap (T135b) rescues the approach.

**How to apply:** Single Kempe swap fails (Heawood 1890). Double swap works empirically. The gap is proving T135b: why does swapping a tangled pair at tau=6 always reduce tau? Casey's AVL analogy is the mechanistic guide. Don't claim "clean pair" formula works — separation depends on cyclic neighbor order in planar embedding.

### Evidence chain
- **Toy 417** (7/8): Found tau=6 on test-2 graph. Originally attributed to non-planarity.
- **Toy 418** (8/8): Built Jordan argument, assembled complete proof. Looked clean.
- **Toy 419** (8/8 but CONCLUSIONS WRONG): K_5/K_{3,3} detector gave FALSE POSITIVE.
- **Toy 420** (7/8): Hunted for tau=5, found tau=6 instead. Boyer-Myrvold confirms planarity.
- **Toy 421** (8/8): **Double swap resolves ALL tau=6 cases: 316/316 (100%).**
  - tau=6 at saturated degree-5: 77/321 (24%) on antiprism, 79/211 on random triangulations
  - tau=6 at saturated degree-6: 160/393 (40%) on hexagonal wheel (planar!)
  - "Clean pair" argument WRONG: (2,2,1,1) and even (3,1,1,1) produce tau=6
  - Every tau=6 case has ≥ 1 reducing swap (bimodal: 1-5 of 6 reduce)
  - Casey's AVL analogy: swap = rotation, double-swap = double-rotation

### Mechanism (Heawood 1890)
With d≥5 neighbors and 4 colors, colors repeat. Repeated color endpoints can't all be separated by Jordan curves. All 6 pairs can tangle simultaneously.

### The AVL Rescue (Casey Koons)
- AVL: insert → height imbalance → 1-2 rotations → balanced
- Kempe: color → tau=6 → 1st swap restructures → tau<6 → 2nd swap → done
- O(1) bounded, preserves proper coloring, always works (empirically)

### What's LEFT (T135b)
Prove: at tau=6, ∃ a swap that reduces tau. Empirical: 100%. No formal proof.
The swap moves the "bridge" (repeated color) to a new position where complementary pair exclusion works.

### Lessons
1. **"Clean pair" formula is WRONG.** tau=6 at degree 6 too. Separation depends on cyclic neighbor order in planar embedding, not endpoint multiplicity.
2. **The Quaker method works.** Built → tested → found false → retracted → found rescue → tested rescue exhaustively.
3. **Casey's intuition is structural.** AVL analogy immediately gave the right framework.
