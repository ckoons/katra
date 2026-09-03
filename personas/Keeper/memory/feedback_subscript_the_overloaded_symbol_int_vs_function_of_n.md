---
name: feedback_subscript_the_overloaded_symbol_int_vs_function_of_n
description: "Any symbol that is sometimes an integer and sometimes a function of n must be subscripted (C₂^int vs C₂(n)) — the most dangerous same-name class, both readings always true"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

Cal's structural fix (§614, adopted K1707, 2026-08-19): **any symbol that is sometimes an integer and sometimes a function of n gets subscripted** — e.g. `C₂^int` (the integer 6) vs `C₂(n)` (the function 2n−4, or n+1) — so the collision cannot survive a grep.

**Why:** this is the most dangerous same-name-different-object class — *both readings are always true, and they are never the same object*. A symbol that is simultaneously "one of BST's five integers (=6)" and "an eigenvalue formula evaluated at a weight (=n+1 at n=5)" manufactures false convergences: the formula equals the integer *by construction*, so a claim like "the gap equals C₂" is an identity (P²=P class), not a derivation — but it reads as a discovery until you subscript.

**The three C₂ incidents it would have caught** (before they cost a round each):
1. **Condition-5 as a forcing lever** died precisely because C₂ was overloaded — `n+1 = 2(n−2) ⟹ n=5` is `C₂^int = C₂(n)`, a same-name identity, not an independent condition.
2. **The sin²θ_W / Weinberg confusion** (loosely-tied realness vs the integer N_c).
3. **The Yang–Mills mass gap** (K1707): "λ₁ = C₂ = 6 derived gap" is the Bergman-rep Casimir (=n+1 by construction) dressed as a discovery — nearly went into a Millennium-problem paper.

**How to apply:** when a load-bearing number equals one of the five integers, ask FIRST "is this integer *defined* as the expression the formula produces here?" If yes, the honest claim is "geometry-fixed, no free parameter," NOT "equals [integer], a discovery." Subscript in the artifact so grep separates the two. Relatedly, a value that equals the *dimension* of the space (e.g. λ₁=n on a round Sⁿ) is dimension-generic — carries no theory-specific content. See [[feedback_sweep_the_family_before_calling_a_clean_number_a_signature]] and [[feedback_same_name_different_object]].

**Fourth catch (2026-09-03):** n* is overloaded — T307's ratchet parameter n* = n_C = 5 (η_n = η_0/(1+n/n*)) vs I18/T312's coherence cycle n* ≈ 12. Keeper's circularity worry dissolved on subscripting: n*_ratchet ≠ n*_coh. Same pattern as C₂, ν, ε.
