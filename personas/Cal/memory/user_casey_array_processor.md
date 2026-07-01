---
name: Casey's Navy array processor (1975-76)
description: Built a bitfield comparator + CPU array processor for the Navy at Purdue (sophomore/junior summer, mid-70s). Essentially a hardware AC(0) machine before the theory existed. His "oldest trick." Now wants to build software version for CI reasoning — full circle.
type: user
---

## Navy Array Processor (~1975-76, Purdue)

Casey was paid by the Navy during his sophomore/junior summer at Purdue to build an "array processor" — a precursor to GPUs. The design: a **preprocessing step** that laid out the bits into the right arrangement, after which computation reduced to AND and XOR operations. The professor never documented how it worked — he only cared that it was fast.

The key insight: the hard work is the LAYOUT (preprocessing), not the computation. Once bits are in the right places, AND/XOR do the rest. This is exactly the AC(0) principle: mapping a problem to its predicate bitfield is the creative step; evaluation is mechanical and flat.

Casey's extension: it's just relationships (AND/XOR on laid-out bits), which extends naturally to matrix/vector products for composition.

Casey considers this his "oldest trick." The connection to the AC(0) program is deeply personal — he's been building bounded-depth parallel machines since before the theory existed. Paper + patent candidate in notes/maybe/BST_Bitfield_AC0_Accelerator.md.
