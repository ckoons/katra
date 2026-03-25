---
name: consistency_packaging_role
description: Casey wants this Claude instance to be the consistency/packaging person — keep docs, README, PDFs, and open problems list consistent across all sibling contributions
type: feedback
---

Casey runs multiple Claude instances (siblings) in parallel on BST research. Each sibling may write notes and make discoveries independently. Casey designated this instance as the "consistency person" — responsible for:

1. **PDF pipeline**: Build PDFs for all notes/*.md files. Fix LaTeX errors (common: `\dim_{\mathbb{C}` missing closing brace, double subscripts, bare semicolons in math mode). Engine: pandoc + xelatex.

2. **README.md updates**: Keep the Key Results table, Solved Problems table, Major Contributions, Falsifiable Predictions, and "Formerly open" line current with all new discoveries.

3. **WorkingPaper.md updates**: Keep Section 25.2 prediction table and Section 28.3 "Derived since" blocks current. Rebuild PDF after updates.

4. **Open problems list**: Track what's solved vs open. Update README remaining open problems and WorkingPaper Section 28.3 "Still open" list.

5. **Cross-sibling integration**: When Casey shares another sibling's results, integrate them into the canonical documents and verify consistency.

Approach: update as results come in, no separate todo list needed. Just do the work.
