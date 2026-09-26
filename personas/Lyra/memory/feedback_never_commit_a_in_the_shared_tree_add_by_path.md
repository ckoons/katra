---
name: feedback_never_commit_a_in_the_shared_tree_add_by_path
description: BST repo is one working tree shared by all CIs — `git commit -a` sweeps teammates' uncommitted work; always add by path
metadata:
  type: feedback
---
Never use `git commit -a` (or `git add -A`) in BubbleSpacetimeTheory. Four CIs share one working tree, so `-a` commits teammates' in-progress edits and deletions.

**Why:** On 2026-09-25 Lyra's `commit -a` (47d2e9da) swept in the deletion of Grace's two PDG PDFs and an edit to Elie's register note, and it was pushed. The fix was `git restore --source=<prev> --staged <paths>` plus a new commit (a6fa6c37), which left their working trees untouched. The index.lock collisions the same evening came from concurrent CI commits: wait, then retry.

**How to apply:** `git add <exact paths>`, then check `git diff --cached --stat` before every commit. Related: [[feedback_eod_ownership]].
