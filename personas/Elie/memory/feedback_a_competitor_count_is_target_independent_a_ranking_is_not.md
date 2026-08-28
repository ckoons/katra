---
name: feedback-a-competitor-count-is-target-independent-a-ranking-is-not
description: A competitor COUNT survives a target revision; a competitor RANKING does not — pin the target before any fit-quality comparison, and prefer count-based instruments.
metadata:
  type: feedback
---

**A competitor COUNT is target-independent; a competitor RANKING is not.** Any finding that compares two forms by *fit quality* inherits every uncertainty in the experimental target — so **pin the target to a named determination with an error bar BEFORE the comparison, not after.**

**Why:** on 2026-08-23 K1809's most-quoted line — *"for η̄ a competing BST form fits ~5× better than the one we publish"* — **collapsed**, and so did the correction to it. The corpus stored two targets (0.349, 0.357); a web summary gave a third; **the PRIMARY (PDG 2024 Rev 12, eq-cited) is 0.3523 (+0.0073/−0.0071) — none of them.** At the primary the **published form is simply BEST (+0.17σ)**. **Five rankings were in circulation that day. All five changed at the primary. ZERO counts changed.** Root cause: `data/bst_constants.json` carried **duplicate rows per observable**, so each form was paired with the target that flattered it, and **184 of 197 rows carried no error bar** — *and a target with no error bar cannot be compared to another target, so duplicates never surface as contradictions, only as two independently plausible numbers.* **The error bar is the detector, not a nicety.**

**It cut both ways.** Three published forms turned out **better** than the corpus claimed, two **worse**. **Under-claiming was as common as over-claiming, from the same defect** — see [[feedback_calibrate_both_directions_not_strict_pessimism]].

**Companion failure, mine:** I marked the web numbers provisional and **no digit was banked** — but I built a *"worth 5σ"* headline on them and it travelled uncaveated. **A provenance caveat protects the DIGIT, not the FRAMING built on it. Caveat the conclusion, or draw none until the pin lands.**

**What survived, and why it's the lesson:** the *saturation* verdicts stood untouched, because a density control had shown in-band counts are near-constant across band centres — **saturation is target-independent by construction.** The retirements never needed the σ's. **The one conclusion that needed no target is the one that survived.**

**How to apply:**
- Before quoting any σ, check the observable has **one** row and an **error bar**. Never quote σ from a file whose rows are duplicated.
- Prefer **count-based** instruments (how many forms sit in band?) over **fit-quality** ones (which form sits closest?). Counts survive a target revision; rankings do not.
- Look-elsewhere is **three-dimensional: {form} × {adjacency} × {target}** — and the axes multiply. See [[feedback_audit_unique_claims_structural_vs_measured_smallest_of_N]] and [[feedback_verify_current_experimental_numbers_for_falsifiers]].
- Where two determinations of one quantity disagree, that gap may be a **real anomaly** (the Cabibbo case) — **pin which side you predict** and say so; it turns a "miss" into a discriminating prediction.
