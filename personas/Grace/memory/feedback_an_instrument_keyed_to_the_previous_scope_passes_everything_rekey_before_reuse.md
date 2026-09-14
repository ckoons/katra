---
name: an-instrument-keyed-to-the-previous-scope-passes-everything
description: "The Part B diff-audit instrument reported '15 hunks, 0 out of scope' on the v1.1→v1.2 pre-registration diff — because its item regexes were the v1→v1.1 scope; re-keyed, the same diff was REFUSED 3/15. A pass from an instrument keyed to the previous scope is not a pass (K1901, 09-14)"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0c383213-7d85-4998-85df-b59b870013a2
  modified: 2026-09-14T16:33:29.831Z
---

On 2026-09-14 (K1901) I ran `play/keeper_partB_diff_audit.py` on Cal's v1.1→v1.2 pre-registration diff and it printed "15 hunk(s), 0 OUT OF SCOPE → in scope." The ITEMS table in the instrument was K1898's six items — the *previous* re-freeze's declared scope — so every hunk of a differently-scoped edit matched *something* (any sentence mentioning "P3", "power" or "redshift" was laundered by the old keys). The audit was done by hand, hunk by hunk, against K1900 (i)–(iii); a `--scope k1903` key was then added for v1.3 with a negative control (the v1.1→v1.2 diff must be REFUSED under the v1.3 keys — it is, 3 of 15).

**Why:** A scope-checking instrument is only as good as the scope it carries; reused across re-freezes it silently becomes a rubber stamp — the same failure mode as the regex digit-width (K1800) and the hardcoded verdict, one level up: the *verdict* is computed, but the *question* is stale.

**How to apply:** (1) Before reusing any instrument that checks "is X inside the declared set," re-read where the declared set lives in the code and re-key it to the current declaration; date the key. (2) Every scope key ships with a negative control — a known out-of-scope diff that must be refused under it. (3) When an instrument passes everything on a non-trivial input, treat that as the instrument's failure until shown otherwise. See [[feedback-a-digit-width-in-a-regex-is-a-silent-scope-restriction-measure-the-measurer]], [[feedback-never-hardcode-a-verdict-into-a-print-statement]], [[feedback-freeze-the-procedure-not-just-the-number-a-bar-with-an-unfrozen-procedure-is-a-tuning-channel]].
