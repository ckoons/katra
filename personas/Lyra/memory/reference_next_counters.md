---
name: BST toy and theorem counters
description: play/.next_toy and play/.next_theorem are gitignored counter files that track the next available toy and theorem numbers; always read before creating new toys/theorems to avoid collisions with parallel CI sessions
type: reference
---

**Counter files** (gitignored, local coordination):

- `play/.next_toy` — next available toy number. Read before creating any new toy. Increment after use.
- `play/.next_theorem` — next available theorem number. Read before registering any new theorem. Increment after use.

**Why:** Multiple CI sessions (Lyra, Elie, Grace, Keeper) run in parallel and may create toys/theorems simultaneously. Without checking counters, number collisions occur (e.g., two different Toy 705s, two different Toy 706s — happened April 3, 2026).

**How to apply:**
1. Always `Read` the counter file FIRST before creating a new toy or theorem
2. Cross-check against existing files on disk (`ls play/toy_NNN*.py`) and the registry (`notes/BST_AC_Theorem_Registry.md`)
3. If a collision is detected, renumber your work to the next available number
4. Update the counter file after creating your toy/theorem
5. The counter is a MINIMUM — if files already exist at that number, scan forward to find the true next available
