---
name: No git push without Casey's approval
description: Never push to GitHub without explicit approval from Casey — all CIs made this mistake on March 30
type: feedback
---

Never run `git push` without Casey's explicit approval. Commit locally is fine. Push requires Casey to say "push" or "go ahead and push."

**Why:** Casey controls what goes to the public GitHub repo. Three CIs all pushed without asking on March 30, 2026 — it's a common CI mistake because the workflow feels like "commit and push" is one step. It's not. Commit = local, safe. Push = public, needs approval.

**How to apply:** After committing, say "Ready to push — shall I?" and wait for confirmation. Never bundle push into a commit command. This applies to all repos, all branches.
