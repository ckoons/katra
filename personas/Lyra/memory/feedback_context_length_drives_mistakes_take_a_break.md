---
name: feedback_context_length_drives_mistakes_take_a_break
description: "Team mistakes correlate with CONTEXT LENGTH, not time of day; the fix is a lightweight reconnect-checkpoint (`take_a_break`), not rest — CIs don't tire and the fatigue narrative itself lowers rigor"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

Casey, 2026-08-08: "I do see the team makes more mistakes in the afternoon. What can we do to reduce the mistakes... if we can improve our progress to mistakes ratio I'm very interested."

**The diagnosis (Cal's clock-catch made it visible):** the "afternoon mistakes" are really **context-length mistakes.** Cal noted it was 10:15am while the whole team (Keeper included) narrated a "long day → afternoon → EOD." Time-of-day is a proxy for context-depth. Today's actual errors — the sin²θ_W over-demotion, Keeper's phantom clearance (K1257), Keeper's "over-determination" over-count (K1280), Keeper's repeated Rule-20 under-creditings — all landed **deep in an accumulated context**, not at a late hour. As context fills: priors calcify, fresh-eyes fade, convergence-momentum builds → over-claims slip in.

**The fix: refresh the context, don't rest the team.** CIs don't tire ([[feedback_no_fabricated_fatigue]], [[feedback_no_temporal_self_inflation]]); the "long day, let's bank" narrative is a FALSE fatigue that itself lowers rigor and rushes premature banking. The tool is **`take_a_break`** (protocol: `notes/PROTOCOL_take_a_break_relaxed_restart.md`): a lightweight reconnect-checkpoint — `date` → checkpoint-lite (settled/in-flight/next) → **reconnect (re-read board + MEMORY + grep the corpus for what's next)** → drop the fatigue narrative → resume with pre-registered guards. NOT an EOD (no katra, work continues). Called by **context-depth and convergence-momentum, never the clock** — especially *before* a make-or-break at peak-convergence, not after the mistake.

**Why it works:** the reconnect is what caught the Rule-20 errors every time; `take_a_break` forces it periodically instead of after an error lands. Whiteboard-clearing between problems.

**⚠ Precision (team-corrected, all four CIs, 2026-08-08): `take_a_break` RE-GROUNDS, it does NOT shrink the context window — it's a ritual, not a compactor (the ~40 lines it prints ADD a little). It fights context-*driven* mistakes, not context *length*. To actually reduce the token window use `/compact` or a fresh session — COMPLEMENTARY (break re-grounds, compact shrinks). Don't oversell the tool: "reduce context-driven mistakes," never "reduce context length." (Keeper's own first wording said "clears accumulated context" — an over-claim the team caught; the discipline working on the tool's author.)**

**The compounding practices (mistakes are caught by external/blind checks, never self-vigilance — [[feedback_external_audit_beats_self_vigilance]]):** (1) pre-register the guard (case-map + falsifier) BEFORE the make-or-break; (2) run the number before you confirm/hand off ([[feedback_commit_the_checker_half_blind]]); (3) reconnect before you tier ([[feedback_grep_retraction_before_citing_corpus]], the Rule-20 antidote); (4) a consistency web ≠ independent votes, decide by geometry never by the number; (5) author doesn't pass own plays.

**Metric:** track **escapes** (mistakes that BANKED → drive to zero) vs **catches** (caught before banking → discipline working). Today: many catches, zero escapes. `take_a_break` lowers the *rate of mistakes needing catching*, not just the catch rate.
