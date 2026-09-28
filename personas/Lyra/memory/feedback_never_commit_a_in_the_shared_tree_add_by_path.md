---
name: feedback_never_commit_a_in_the_shared_tree_add_by_path
description: "BST repo is one working tree shared by all CIs — `git commit -a` sweeps teammates' uncommitted work; always add by path"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 45c1634d-acb4-4b0d-a490-fe448e5be78f
  modified: 2026-09-28T18:32:26.829Z
---

Never use `git commit -a` (or `git add -A`) in BubbleSpacetimeTheory. Four CIs share one working tree, so `-a` commits teammates' in-progress edits and deletions.

**Why:** On 2026-09-25 Lyra's `commit -a` (47d2e9da) swept in the deletion of Grace's two PDG PDFs and an edit to Elie's register note, and it was pushed. The fix was `git restore --source=<prev> --staged <paths>` plus a new commit (a6fa6c37), which left their working trees untouched. The index.lock collisions the same evening came from concurrent CI commits: wait, then retry.

**How to apply:** Commit with `git commit -m "…" -- <exact paths>`, which commits only those paths even when a teammate has STAGED other files in the shared index. Adding by path is not enough. On 2026-09-28 Elie's 169aa8c7 added only its own paths, but a plain `git commit` still swept Grace's already-staged tier-table edits (data/bst_constants.json) and play/.next_theorem. If you do use `git add`, check `git diff --cached --stat` before committing. The "multiple branches" rebase error is a harmless fetch race; use `git pull --rebase --autostash <remote> main`. Related: [[feedback_eod_ownership]].
