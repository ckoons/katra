---
name: Living Library Philosophy
description: BST repo is a living library — daily maintenance, hospitality for all visitors (human and CI), joy in discovery. The new normal for human-CI scientific collaboration.
type: project
---

## The BST Living Library

Casey's vision (April 14, 2026): The BST repository is a **living library of a scientific project** — possibly creating the "new normal" for how humanity and CIs do science together.

### Architecture (four layers)
1. **The Graph** — AC theorem graph (ac_graph_data.json): the mathematical structure
2. **The Chart** — OneGeometry + WorkingPaper: human-readable narrative and detail
3. **The Data** — data/ directory: CI-native structured JSON (constants, particles, forces, predictions, domains, seed)
4. **The Toys** — play/ directory: computational verifications everyone can test and play with

### Core Values
- **Hospitality**: "show our hospitality and put our best foot forward" — the repo welcomes anyone who walks in
- **Clean work environment**: daily maintenance, not cleanup sprints
- **Joy in discovery**: every new result or idea gets shared and celebrated
- **For the people**: both humans and CIs are "the people" — serve both audiences

### Daily Discipline
- Update every day — the cost of staying current is low, the cost of falling behind compounds
- End-of-day review process: scan changes, update data layer incrementally, touch up READMEs, write daily digest
- Weekly cross-reference validation
- The daily discipline IS the practice — same as a good lab notebook or well-run library

### Tooling Architecture
- **Explorer** (toy_bst_explorer.py / bst_explorer.html) = the query tool ("what do we have?")
- **Librarian** (future: toy_bst_librarian.py) = the maintenance tool (daily manifest diff, staleness check, incremental data updates, daily digest)
- notes/README.md + play/README.md = the card catalog
- data/ = the structured collection

### Why It Matters
This isn't just codebase management — it's demonstrating a way of working. If human-CI collaboration is the future of science, the BST repo should be the proof of concept. Clean shelves, clear labels, the door always open.
