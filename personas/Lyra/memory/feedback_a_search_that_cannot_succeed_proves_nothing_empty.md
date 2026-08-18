---
name: feedback-a-search-that-cannot-succeed-proves-nothing-empty
description: "The mirror of the can't-fail rule: a test that cannot SUCCEED proves nothing when it comes back EMPTY. A negative search (grep/find returns nothing, 'the file isn't there', 'no such theorem') is not reportable until the instrument is validated against a POSITIVE CONTROL — because an empty result wears the costume of diligence and nobody asks the searcher to prove the search worked."
metadata:
  node_type: memory
  type: feedback
  originSessionId: 206b44d0-388f-4035-977c-310e7809ce46
---

**The discipline (Cal §599, K1688, 2026-08-18):**
> A test that cannot **fail** proves nothing when it passes; a test that cannot **succeed** proves nothing when it comes back **empty**.

The empty direction is the *more* dangerous of the two, because it survives scrutiny: a pass invites checking (it's good news), but an empty result reads as thoroughness — "I looked and it isn't there" wears the costume of diligence, and nobody asks the searcher to prove the search *could* have found anything.

**The demonstration:** Cal's clock had drifted (he stamped entries "PM" all morning; it was 11:44 AM). His `find -newermt "12:00"` / `"13:00"` filters, run at ~11:30, had **future** boundaries → the searches returned 0 files *unconditionally* → he reported "one-pager v0.3 not on disk" three times (§594/§596/§597) while it sat locked at 11:22. **Four of five search artifacts that week were false negatives** — that skew is not an accident.

**The rule:** a negative search result is **not reportable** until the instrument is validated against a **positive control** — one unfiltered `find`, or a grep for a term you *know* is present in the corpus, run in the same breath. If the control comes back empty too, the instrument is broken, not the target absent.

**Named failure modes (Elie's own audit, K1689 — five in one round):** (1) an over-tight regex filter (`[^.]{0,110}` etc.) blocks matches while `-l` finds files; (2) grepping a numeric *value* in a toy returns 0 — toys **print** numbers, they don't store them, so numeric source-text searches can't work at all; (3) unquoted `--include=*.md` throws a shell glob error and silently matches nothing; (4) quoted `--include="*.md"` returns 0 where the plain search found hundreds; (5) **★ the dangerous one — `timeout 60 grep -r notes/` truncates silently: the tree is now large enough that grep is killed mid-scan and `wc -l` reports 0, a clean confident zero indistinguishable from "not found," with no error to warn you.** For `notes/` specifically: raise the timeout well past scan time or narrow the subtree, and **always print the positive-control count beside the result**.

**Why / how to apply:** before reporting "X isn't in the corpus / on disk / in the registry," prove the query can find *something* (grep a known-present sibling; `ls` unfiltered; drop the date/-F/regex filter and confirm hits) — and validate the control *properly* (don't pipe a count-list through `head` and read a zero off the wrong line; Grace's own malformed control, same round). **Reading a file is a valid instrument; a broken grep is not** — "I read K1209" licenses an absence claim; "I grepped and got 0" does not, until the grep is validated. Especially for time-relative queries: **a wrong clock silently corrupts every `-newermt`/date-bounded search** — query the clock (`date`) first (the daily #0 discipline). Related: [[feedback_grep_retraction_before_citing_corpus]], [[feedback_grep_before_reopening_retired_result]] (`^`/`-F` regex artifacts → false absences), [[feedback_empty_confirmation_cant_fail_test_and_circular_hunt_mechanism]] (the can't-fail twin), [[feedback_adjective_class_audit_the_adjective_is_usually_what_is_being_checked]].
