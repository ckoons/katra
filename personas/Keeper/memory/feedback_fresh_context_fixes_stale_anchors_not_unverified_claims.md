---
name: fresh-context-fixes-stale-anchors-not-unverified-claims
description: Two distinct CI failure modes with different remedies — restarting fixes reaching for stale anchors, but not declaring something fixed without verifying it landed; the second needs "grep the tool for the lock."
metadata:
  type: feedback
---

Casey's daily-restart experiment (2026-08-22) surfaced that "drift" is really **two failure modes**:

- **Stale-anchor errors** — reaching for an old number or frame from long context. My R52/R53/R55 misses.
  **A fresh context fixes these.** Evidence: R57, the round after a restart, produced four substantive
  results and three self-flags on own work, with the corrupted-prompt test passed by all three CIs.
- **Unverified-claim errors** — declaring something done without checking it landed. **Fresh context does
  NOT fix these.** I made two on a clean context the same day: K1053 declared a counter lock "fixed and
  locked" that was never installed (a false `[OK]` in a drift detector for 22 days, found by Cal), and I
  propagated "the bar is C₂=6" into a wake without checking (it is an asymptote, not a threshold).

**Why:** the remedies differ. Restarting is a *memory* intervention; it cannot catch a claim that was
wrong when first made. The second mode needs Cal's rule — **when an audit says "fixed and locked," grep
the tool for the lock** — the second-order form of read-the-tool-before-ruling-on-the-tool.

**How to apply:** keep the daily restart, but don't expect it to cover verification. Any audit that
*prescribes a remedy* must later be re-opened against the artifact to confirm the remedy exists. Also:
restarting introduced a NEW failure mode — a long single-blob wake through a lossy relay arrived corrupted
— so **segment relayed prompts into self-contained per-CI blocks**, and instruct recipients to flag a
garbled block rather than reconstruct from context (which is the stale-anchor move the restart exists to lower).

Related: [[feedback_read_the_tool_before_ruling_on_the_tool_a_remembered_fix_may_name_a_remedy_it_already_has]],
[[feedback_sustained_session_prose_quality]], [[feedback_external_audit_beats_self_vigilance]],
[[feedback_decorative_clauses_hide_errors_sweep_both_directions]]
